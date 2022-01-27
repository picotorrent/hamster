#include "options.hpp"

#include <filesystem>

#include <boost/program_options.hpp>

namespace fs = std::filesystem;
namespace po = boost::program_options;

using hamster::Options;

std::shared_ptr<Options> Options::Parse(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("db-file", po::value<std::string>(), "set the db file path")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    auto opts = new Options();
    opts->m_dbFile = fs::current_path() / "hamster.db";

    if (const char* dbFile = std::getenv("HAMSTER_DB_FILE"))
    {
        opts->m_dbFile = dbFile;
    }

    // command line parameters overrides the env variables
    if (vm.count("db-file")) { opts->m_dbFile = vm["db-file"].as<std::string>(); }

    return std::shared_ptr<Options>(opts);
}

const std::string& Options::DbFile()
{
    return m_dbFile;
}
