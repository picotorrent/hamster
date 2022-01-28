#include "indexer.hpp"

#include <filesystem>
#include <random>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/session.hpp>
#include <sqlite3.h>

#include "models/node.hpp"
#include "models/sample.hpp"
#include "models/torrent.hpp"

namespace fs = std::filesystem;
namespace lt = libtorrent;
using hamster::LibtorrentIndexer;

LibtorrentIndexer::LibtorrentIndexer(boost::asio::io_context &io, sqlite3* db)
    : m_io(io),
      m_timer(io),
      m_db(db)
{
    lt::session_params params;
    params.settings.set_int(lt::settings_pack::alert_mask, lt::alert::all_categories);
    params.settings.set_str(
        lt::settings_pack::dht_bootstrap_nodes,
        "router.bittorrent.com:6881,"
        "dht.transmissionbt.com:6881,"
        "dht.libtorrent.org:25401");

    m_session = std::make_unique<lt::session>(params);
    m_session->set_alert_notify(
        [&]
        {
            boost::asio::post(m_io, [&] { PopAlerts(); });
        });

    boost::system::error_code ec;
    m_timer.expires_from_now(boost::posix_time::seconds(5), ec);
    m_timer.async_wait([this](auto && PH1) { SampleInfohashes(std::forward<decltype(PH1)>(PH1)); });
}

LibtorrentIndexer::~LibtorrentIndexer() noexcept = default;

void LibtorrentIndexer::PopAlerts()
{
    std::vector<lt::alert*> alerts;
    m_session->pop_alerts(&alerts);

    auto const now = std::chrono::system_clock::now();

    for (const auto& alert : alerts)
    {
        // BOOST_LOG_TRIVIAL(info) << alert->message();

        switch (alert->type())
        {
            case lt::dht_pkt_alert::alert_type:
            {
                auto const a = lt::alert_cast<lt::dht_pkt_alert>(alert);

                Models::Node::Exists(m_db, a->node)
                    ? Models::Node::Update(m_db, a->node, now, std::chrono::system_clock::time_point::min())
                    : Models::Node::Create(m_db, a->node, now, std::chrono::system_clock::time_point::min());
            } break;

            case lt::dht_sample_infohashes_alert::alert_type:
            {
                auto const a = lt::alert_cast<lt::dht_sample_infohashes_alert>(alert);

                for (const auto& hash : a->samples())
                {
                    auto const ih = lt::info_hash_t(hash);

                    lt::add_torrent_params params;
                    params.flags &= ~lt::torrent_flags::auto_managed;
                    params.flags &= ~lt::torrent_flags::need_save_resume;
                    params.flags &= ~lt::torrent_flags::paused;
                    params.flags &= ~lt::torrent_flags::update_subscribe;
                    params.flags |= lt::torrent_flags::upload_mode;
                    params.info_hashes = ih;
                    params.save_path = fs::temp_directory_path();

                    m_session->async_add_torrent(params);

                    if (!Models::Sample::Exists(m_db, ih))
                    {
                        Models::Sample::Insert(m_db, ih);
                    }
                }

                std::vector<boost::asio::ip::udp::endpoint> endpoints;
                for (auto const& [_, value] : a->nodes()) { endpoints.push_back(value); }

                auto const interval = std::chrono::seconds(
                    lt::duration_cast<lt::seconds>(a->interval).count());

                Models::Node::BatchUpdate(
                    m_db,
                    endpoints,
                    now,
                    now + std::max(interval, std::chrono::seconds(300)));
            } break;

            case lt::metadata_received_alert::alert_type:
            {
                auto const a = lt::alert_cast<lt::metadata_received_alert>(alert);

                Models::Torrent::Insert(
                    m_db,
                    *a->handle.torrent_file());

                Models::Sample::Delete(
                    m_db,
                    a->handle.info_hashes());

                BOOST_LOG_TRIVIAL(info) << "Torrent indexed: " << a->torrent_name();

                m_session->remove_torrent(
                    a->handle,
                    lt::session::delete_files);
            } break;
        }
    }
}

void LibtorrentIndexer::SampleInfohashes(boost::system::error_code ec)
{
    static std::random_device dev;
    static std::mt19937 rng(dev());
    static std::uniform_int_distribution<std::mt19937::result_type> dist(
        std::numeric_limits<std::uint8_t>::min(),
        std::numeric_limits<std::uint8_t>::max());

    auto const now = std::chrono::system_clock::now();

    std::vector<boost::asio::ip::udp::endpoint> endpoints;

    Models::Node::GetWhereNextRequestExpired(
        m_db,
        now,
        [&](const boost::asio::ip::udp::endpoint& endpoint)
        {
            lt::sha1_hash hash;
            for (auto& b : hash) { b = dist(rng); }
            m_session->dht_sample_infohashes(endpoint, hash);
            endpoints.push_back(endpoint);
        });

    Models::Node::BatchUpdateNextRequest(
        m_db,
        endpoints,
        now + std::chrono::minutes(5));

    m_timer.expires_from_now(boost::posix_time::seconds(5), ec);
    m_timer.async_wait([this](auto && PH1) { SampleInfohashes(std::forward<decltype(PH1)>(PH1)); });
}
