#pragma once

#include <libtorrent/torrent_info.hpp>

#include <sqlite3.h>

namespace hamster::Models
{
    class Torrent
    {
    public:
        static void Insert(
            sqlite3* db,
            const libtorrent::torrent_info& torrentInfo);
    };
}
