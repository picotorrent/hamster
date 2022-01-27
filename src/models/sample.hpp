#pragma once

#include <libtorrent/info_hash.hpp>
#include <sqlite3.h>

namespace hamster::Models
{
    class Sample
    {
    public:
        static void Delete(
            sqlite3* db,
            const libtorrent::info_hash_t& hash);

        static bool Exists(
            sqlite3* db,
            const libtorrent::info_hash_t& hash);

        static void Insert(
            sqlite3* db,
            const libtorrent::info_hash_t& hash);
    };
}
