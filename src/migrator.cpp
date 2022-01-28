#include "migrator.hpp"

#include <vector>

#include <boost/log/trivial.hpp>

int Migration_0001_Init(sqlite3* db)
{
    int res = sqlite3_exec(
        db,
        "CREATE TABLE nodes ("
        "   id           INTEGER PRIMARY KEY,"
        "   remote_addr  TEXT    NOT NULL,"
        "   port         INTEGER NOT NULL,"
        "   last_seen    INTEGER NOT NULL,"
        "   next_request INTEGER NOT NULL,"
        "   UNIQUE (remote_addr, port)"
        ");",
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) { return res; }

    res = sqlite3_exec(
        db,
        "CREATE TABLE samples ("
        "   id INTEGER NOT NULL PRIMARY KEY,"
        "   info_hash_v1 TEXT NULL UNIQUE,"
        "   info_hash_v2 TEXT NULL UNIQUE,"
        "   timestamp INTEGER NOT NULL"
        ");",
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) { return res; }

    res = sqlite3_exec(
        db,
        "CREATE TABLE torrents ("
        "   id INTEGER PRIMARY KEY,"
        "   info_hash_v1 TEXT NULL UNIQUE,"
        "   info_hash_v2 TEXT NULL UNIQUE,"
        "   name TEXT NOT NULL,"
        "   size INTEGER NOT NULL"
        ");",
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) { return res; }

    res = sqlite3_exec(
        db,
        "CREATE TABLE torrentfiles ("
        "   id INTEGER PRIMARY KEY,"
        "   torrent_id INTEGER NOT NULL REFERENCES torrents(id),"
        "   path TEXT NOT NULL,"
        "   size INTEGER NOT NULL"
        ");",
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) { return res; }

    return SQLITE_OK;
}

int Migration_0002_RemoveUnnecessaryTables(sqlite3* db)
{
    int res = sqlite3_exec(
        db,
        "DROP TABLE nodes;",
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) return res;

    res = sqlite3_exec(
        db,
        "DROP TABLE samples;",
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) return res;

    return SQLITE_OK;
}

bool hamster::MigrateDatabase(sqlite3* db)
{
    static std::vector<std::function<int(sqlite3*)>> migrations =
    {
        { &Migration_0001_Init },
        { &Migration_0002_RemoveUnnecessaryTables }
    };

    // Get current user_version
    int userVersion = -1;
    int res = sqlite3_exec(
        db,
        "PRAGMA user_version",
        [](void* user, int columns, char** values, char** names)
        {
            int* uv = static_cast<int*>(user);
            *uv = std::stoi(values[0]);
            return SQLITE_OK;
        },
        &userVersion,
        nullptr);

    if (res != SQLITE_OK)
    {
        BOOST_LOG_TRIVIAL(error) << "Failed to fetch user version.";
        return false;
    }

    BOOST_LOG_TRIVIAL(info)
        << "Database version is "
        << userVersion << ", running "
        << migrations.size() - userVersion
        << " migration(s)";

    for (int i = userVersion; i < migrations.size(); i++)
    {
        res = migrations[i](db);

        if (res != SQLITE_OK)
        {
            BOOST_LOG_TRIVIAL(error) << "Failed to run migration #" << i << ": " << sqlite3_errmsg(db);
            return false;
        }
    }

    std::string setUserVersion = "PRAGMA user_version=" + std::to_string(migrations.size());

    res = sqlite3_exec(
        db,
        setUserVersion.c_str(),
        nullptr,
        nullptr,
        nullptr);

    if (res != SQLITE_OK) { return false; }

    BOOST_LOG_TRIVIAL(info) << "Database migrated and up to date.";

    return true;
}
