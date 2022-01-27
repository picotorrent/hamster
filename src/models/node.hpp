#pragma once

#include <boost/asio.hpp>
#include <sqlite3.h>

namespace hamster::Models
{
    class Node
    {
    public:
        static void BatchUpdate(
            sqlite3* db,
            const std::vector<boost::asio::ip::udp::endpoint>& endpoints,
            const std::chrono::system_clock::time_point& lastSeen,
            const std::chrono::system_clock::time_point& nextRequest);

        static void BatchUpdateNextRequest(
            sqlite3* db,
            const std::vector<boost::asio::ip::udp::endpoint>& endpoints,
            const std::chrono::system_clock::time_point& nextRequest);

        static void Create(
            sqlite3* db,
            const boost::asio::ip::udp::endpoint& endpoint,
            const std::chrono::system_clock::time_point& lastSeen,
            const std::chrono::system_clock::time_point& nextRequest);

        static bool Exists(
            sqlite3* db,
            const boost::asio::ip::udp::endpoint& endpoint);

        static void GetWhereNextRequestExpired(
            sqlite3* db,
            const std::chrono::system_clock::time_point& nextRequest,
            const std::function<void(const boost::asio::ip::udp::endpoint& endpoint)>& callback);

        static void Update(
            sqlite3* db,
            const boost::asio::ip::udp::endpoint& endpoint,
            const std::chrono::system_clock::time_point& lastSeen,
            const std::chrono::system_clock::time_point& nextRequest);
    };
}
