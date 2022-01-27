#pragma once

#include <sqlite3.h>

namespace hamster
{
    bool MigrateDatabase(sqlite3* db);
}
