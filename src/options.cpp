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
        ("log-level", po::value<std::string>(), "set log level")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    auto opts = new Options();
    opts->m_dbFile = fs::current_path() / "hamster.db";
    opts->m_logLevel = boost::log::trivial::severity_level::info;

    if (const char* dbFile = std::getenv("HAMSTER_DB_FILE"))
    {
        opts->m_dbFile = dbFile;
    }

    // command line parameters overrides the env variables
    if (vm.count("db-file")) { opts->m_dbFile = vm["db-file"].as<std::string>(); }

    if (vm.count("log-level"))
    {
        std::string level = vm["log-level"].as<std::string>();
        if (level == "trace") { opts->m_logLevel = boost::log::trivial::trace; }
        if (level == "debug") { opts->m_logLevel = boost::log::trivial::debug; }
        if (level == "info") { opts->m_logLevel = boost::log::trivial::info; }
        if (level == "warning") { opts->m_logLevel = boost::log::trivial::warning; }
        if (level == "error") { opts->m_logLevel = boost::log::trivial::error; }
        if (level == "fatal") { opts->m_logLevel = boost::log::trivial::fatal; }
    }

    return std::shared_ptr<Options>(opts);
}

const std::string& Options::DbFile()
{
    return m_dbFile;
}

boost::log::trivial::severity_level Options::LogLevel()
{
    return m_logLevel;
}
