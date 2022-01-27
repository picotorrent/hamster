#pragma once

#include <memory>
#include <string>

namespace hamster
{
    class Options
    {
    public:
        static std::shared_ptr<Options> Parse(int argc, char* argv[]);

        const std::string& DbFile();

    private:
        std::string m_dbFile;
    };
}
