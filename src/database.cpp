#include "database.hpp"

sqlite3* hamster::OpenDatabase(const std::string& file)
{
    sqlite3* db;
    int res = sqlite3_open(file.c_str(), &db);
    if (res != SQLITE_OK) throw hamster::DatabaseException(db);

    res = sqlite3_exec(
        db,
        "PRAGMA journal_mode=wal;",
        [](void*, int, char**, char**){ return SQLITE_OK; },
        nullptr,
        nullptr);

    if (res != SQLITE_OK) throw hamster::DatabaseException(db);

    res = sqlite3_exec(
        db,
        "PRAGMA foreign_keys=ON;",
        [](void*, int, char**, char**) { return SQLITE_OK; },
        nullptr,
        nullptr);

    if (res != SQLITE_OK) throw hamster::DatabaseException(db);

    return db;
}
