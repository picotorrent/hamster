#pragma once

#include <string>
#include <utility>
#include <sqlite3.h>

namespace hamster
{
    class DatabaseException : public std::exception
    {
    public:
        explicit DatabaseException(sqlite3* db)
            : m_what(sqlite3_errmsg(db)) {}

        explicit DatabaseException(std::string what)
            : m_what(std::move(what)) {}

        [[nodiscard]] const char* what() const noexcept override
        {
            return m_what.c_str();
        }

    private:
        std::string m_what;
    };

    sqlite3* OpenDatabase(const std::string& file);
}
