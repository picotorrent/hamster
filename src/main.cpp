#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/log/trivial.hpp>

#include <sqlite3.h>

#include "database.hpp"
#include "indexer.hpp"
#include "migrator.hpp"
#include "options.hpp"

int main(int argc, char* argv[])
{
    auto const opts = hamster::Options::Parse(argc, argv);

    BOOST_LOG_TRIVIAL(info) << "Hamster";
    BOOST_LOG_TRIVIAL(info) << "- Database: " << (opts->DbFile() == ":memory:" ? "(in-memory)" : opts->DbFile());

    sqlite3* db = hamster::OpenDatabase(opts->DbFile());

    if (!hamster::MigrateDatabase(db))
    {
        BOOST_LOG_TRIVIAL(fatal)
            << "Failed to migrate database: "
            << sqlite3_errmsg(db)
            << ". Exiting...";
        return -1;
    }

    boost::asio::io_context io;
    boost::asio::signal_set signals(io, SIGINT, SIGTERM);

    signals.async_wait(
        [&](boost::system::error_code ec, int signal)
        {
            BOOST_LOG_TRIVIAL(info) << "Interrupt (" << signal << ") received - shutting down...";
            io.stop();
        });

    hamster::LibtorrentIndexer indexer(io, db);

    io.run();

    sqlite3_close(db);

    return 0;
}
