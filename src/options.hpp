#pragma once

#include <memory>
#include <string>

#include <boost/log/trivial.hpp>

namespace hamster
{
    class Options
    {
    public:
        static std::shared_ptr<Options> Parse(int argc, char* argv[]);

        const std::string& DbFile();
        boost::log::trivial::severity_level LogLevel();

    private:
        std::string m_dbFile;
        boost::log::trivial::severity_level m_logLevel;
    };
}
