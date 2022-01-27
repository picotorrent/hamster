#include "sample.hpp"

#include <sstream>

#include <boost/log/trivial.hpp>

#include <chrono>

using hamster::Models::Sample;

template<typename T>
static std::string InfoHashString(T hash)
{
    std::stringstream h;
    h << hash;
    return h.str();
}

void Sample::Delete(
    sqlite3 *db,
    const libtorrent::info_hash_t& hashes)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "DELETE FROM samples WHERE (info_hash_v1 = $1 OR info_hash_v1 IS $1) AND (info_hash_v2 = $2 OR info_hash_v2 IS $2)",
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

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool Sample::Exists(
    sqlite3* db,
    const libtorrent::info_hash_t& hashes)
{
    sqlite3_stmt* stmt = nullptr;
    int res = sqlite3_prepare_v2(
        db,
        "SELECT 1 FROM samples WHERE (info_hash_v1 = $1 OR info_hash_v2 IS $1) AND (info_hash_v2 = $2 OR info_hash_v2 IS $2)",
        -1,
        &stmt,
        nullptr);

    if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);

    if (hashes.has_v1())
    {
        std::string hash = InfoHashString(hashes.v1);
        res = sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
        if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);
    }
    else
    {
        sqlite3_bind_null(stmt, 1);
    }

    if (hashes.has_v2())
    {
        std::string hash = InfoHashString(hashes.v2);
        res = sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
        if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    bool exists = false;

    switch (sqlite3_step(stmt))
    {
        case SQLITE_ERROR:
            BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);
            break;
        case SQLITE_ROW:
            exists = true;
            break;
    }

    sqlite3_finalize(stmt);
    return exists;
}

void Sample::Insert(
    sqlite3 *db,
    const libtorrent::info_hash_t& hashes)
{
    sqlite3_stmt* stmt = nullptr;
    int res = sqlite3_prepare_v2(
        db,
        "INSERT INTO samples (info_hash_v1, info_hash_v2, timestamp) VALUES ($1,$2,$3);",
        -1,
        &stmt,
        nullptr);

    if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);

    if (hashes.has_v1())
    {
        std::string hash = InfoHashString(hashes.v1);
        res = sqlite3_bind_text(stmt, 1, hash.c_str(), -1, SQLITE_TRANSIENT);
        if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);
    }
    else
    {
        sqlite3_bind_null(stmt, 1);
    }

    if (hashes.has_v2())
    {
        std::string hash = InfoHashString(hashes.v2);
        res = sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
        if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);
    }
    else
    {
        sqlite3_bind_null(stmt, 2);
    }

    auto const now = std::chrono::system_clock::now();
    auto const sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    res = sqlite3_bind_int64(stmt, 3, sec.count());
    if (res != SQLITE_OK) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db);

    res = sqlite3_step(stmt);
    if (res != SQLITE_DONE) BOOST_LOG_TRIVIAL(error) << sqlite3_errmsg(db) << ", " << hashes.v1 << ", " << hashes.v2;

    sqlite3_finalize(stmt);
}
