#include "torrent.hpp"

using hamster::Models::Torrent;

template<typename T>
static std::string InfoHashString(T hash)
{
    std::stringstream h;
    h << hash;
    return h.str();
}

void Torrent::Insert(
    sqlite3 *db,
    const libtorrent::torrent_info &torrentInfo)
{
    auto const hashes = torrentInfo.info_hashes();

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "INSERT INTO torrents (info_hash_v1, info_hash_v2, name, size) VALUES ($1,$2,$3,$4);",
        -1,
        &stmt,
        nullptr);

    if (hashes.has_v1())
    {
        std::string hash = InfoHashString(hashes.v1);
        sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_bind_null(stmt, 1);
    }

    if (hashes.has_v2())
    {
        std::string hash = InfoHashString(hashes.v2);
        sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    sqlite3_bind_text(stmt,  3, torrentInfo.name().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, torrentInfo.total_size());
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_int64 id = sqlite3_last_insert_rowid(db);

    // insert files
    sqlite3_prepare_v2(
        db,
        "INSERT INTO torrentfiles (torrent_id, path, size) VALUES ($1,$2,$3);",
        -1,
        &stmt,
        nullptr);

    auto const& files = torrentInfo.files();

    for (int i = 0; i < files.num_files(); i++)
    {
        sqlite3_bind_int64(stmt, 1, id);
        sqlite3_bind_text(stmt,  2, files.file_path(lt::file_index_t{i}).c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, files.file_size(lt::file_index_t{i}));
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
}
