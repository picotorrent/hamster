#include "indexer.hpp"

#include <filesystem>
#include <random>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/session.hpp>
#include <sqlite3.h>

#include "models/torrent.hpp"

namespace fs = std::filesystem;
namespace lt = libtorrent;
using hamster::LibtorrentIndexer;
using namespace std::literals::chrono_literals;

struct LibtorrentIndexer::NodeInfo
{
    lt::time_point nextRequest;
};

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

LibtorrentIndexer::~LibtorrentIndexer() noexcept
{
    m_session->set_alert_notify([] {});
    m_timer.cancel();
}

void LibtorrentIndexer::PopAlerts()
{
    static const lt::clock_type::duration minRequestInterval = 5min;

    std::vector<lt::alert*> alerts;
    m_session->pop_alerts(&alerts);

    auto const now = lt::clock_type::now();

    for (const auto& alert : alerts)
    {
        // BOOST_LOG_TRIVIAL(info) << alert->message();

        switch (alert->type())
        {
            case lt::dht_pkt_alert::alert_type:
            {
                auto const a = lt::alert_cast<lt::dht_pkt_alert>(alert);

                if (m_nodes.find(a->node) == m_nodes.end())
                {
                    m_nodes.insert({ a->node, { lt::time_point::min() }});
                }
            } break;

            case lt::dht_sample_infohashes_alert::alert_type:
            {
                auto const a = lt::alert_cast<lt::dht_sample_infohashes_alert>(alert);

                for (const auto& hash : a->samples())
                {
                    auto const ih = lt::info_hash_t(hash);

                    if (m_hashes.find(ih) != m_hashes.end())
                    {
                        continue;
                    }

                    BOOST_LOG_TRIVIAL(debug) << "Added torrent " << ih;

                    lt::add_torrent_params params;
                    params.flags &= ~lt::torrent_flags::auto_managed;
                    params.flags &= ~lt::torrent_flags::need_save_resume;
                    params.flags &= ~lt::torrent_flags::paused;
                    params.flags &= ~lt::torrent_flags::update_subscribe;
                    params.flags |= lt::torrent_flags::upload_mode;
                    params.info_hashes = ih;
                    params.save_path = fs::temp_directory_path();

                    m_session->async_add_torrent(params);
                    m_hashes.insert(ih);
                }

                for (auto const& [_, endpoint] : a->nodes())
                {
                    auto it = m_nodes.find(endpoint);

                    if (it == m_nodes.end())
                    {
                        it = m_nodes.insert({ endpoint, { lt::time_point::min() }}).first;
                    }

                    it->second.nextRequest = now + std::max(a->interval, minRequestInterval);
                }
            } break;

            case lt::metadata_received_alert::alert_type:
            {
                auto const a = lt::alert_cast<lt::metadata_received_alert>(alert);

                Models::Torrent::Insert(
                    m_db,
                    *a->handle.torrent_file());

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
    if (ec) { return; }

    static std::random_device dev;
    static std::mt19937 rng(dev());
    static std::uniform_int_distribution<std::mt19937::result_type> dist(
        std::numeric_limits<std::uint8_t>::min(),
        std::numeric_limits<std::uint8_t>::max());

    auto const now = lt::clock_type::now();

    auto it = std::find_if(
        m_nodes.begin(),
        m_nodes.end(),
        [now](auto const& n) { return n.second.nextRequest < now; });

    int sampled = 0;

    for (;it != m_nodes.end(); it++)
    {
        it->second.nextRequest = now + 1h;

        lt::sha1_hash hash;
        for (auto& b : hash) { b = dist(rng); }

        m_session->dht_sample_infohashes(it->first, hash);
        sampled += 1;
    }

    BOOST_LOG_TRIVIAL(debug) << "Sampled " << sampled << " of " << m_nodes.size() << " node(s)";

    m_timer.expires_from_now(boost::posix_time::seconds(5), ec);
    m_timer.async_wait([this](auto && PH1) { SampleInfohashes(std::forward<decltype(PH1)>(PH1)); });
}
