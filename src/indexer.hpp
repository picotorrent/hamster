#pragma once

#include <map>
#include <memory>
#include <unordered_set>

#include <boost/asio.hpp>
#include <libtorrent/fwd.hpp>
#include <libtorrent/info_hash.hpp>
#include <sqlite3.h>

namespace hamster
{
    class IIndexer
    {
    public:
        virtual ~IIndexer() = default;
    };

    class LibtorrentIndexer : public IIndexer
    {
    public:
        LibtorrentIndexer(boost::asio::io_context& io, sqlite3* db);
        ~LibtorrentIndexer() noexcept override;

    private:
        struct NodeInfo;

        void PopAlerts();
        void SampleInfohashes(boost::system::error_code ec);

        boost::asio::io_context& m_io;
        boost::asio::deadline_timer m_timer;

        sqlite3* m_db;
        std::unique_ptr<libtorrent::session> m_session;
        std::map<boost::asio::ip::udp::endpoint, NodeInfo> m_nodes;
        std::unordered_set<lt::info_hash_t> m_hashes;
    };
}
