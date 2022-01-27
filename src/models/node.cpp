#include "node.hpp"

using hamster::Models::Node;

void Node::BatchUpdate(
    sqlite3* db,
    const std::vector<boost::asio::ip::udp::endpoint>& endpoints,
    const std::chrono::system_clock::time_point& lastSeen,
    const std::chrono::system_clock::time_point& nextRequest)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "UPDATE nodes SET last_request = $1, next_request = $2 WHERE remote_addr = $3 AND port = $4",
        -1,
        &stmt,
        nullptr);

    for (auto const& endpoint : endpoints)
    {
        auto const addr = endpoint.address().to_string();
        auto const port = endpoint.port();
        auto const last = std::chrono::duration_cast<std::chrono::seconds>(lastSeen.time_since_epoch());
        auto const next = std::chrono::duration_cast<std::chrono::seconds>(nextRequest.time_since_epoch());

        sqlite3_bind_int64(stmt, 1, last.count());
        sqlite3_bind_int64(stmt, 2, next.count());
        sqlite3_bind_text(stmt,  3, addr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,   4, port);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
}

void Node::BatchUpdateNextRequest(
    sqlite3* db,
    const std::vector<boost::asio::ip::udp::endpoint>& endpoints,
    const std::chrono::system_clock::time_point& nextRequest)
{
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "UPDATE nodes SET next_request = $1 WHERE remote_addr = $2 AND port = $3",
        -1,
        &stmt,
        nullptr);

    for (auto const& endpoint : endpoints)
    {
        auto const addr = endpoint.address().to_string();
        auto const port = endpoint.port();
        auto const next = std::chrono::duration_cast<std::chrono::seconds>(nextRequest.time_since_epoch());

        sqlite3_bind_int64(stmt, 1, next.count());
        sqlite3_bind_text(stmt,  2, addr.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt,   3, port);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }

    sqlite3_finalize(stmt);
}

void Node::Create(
    sqlite3* db,
    const boost::asio::ip::udp::endpoint& endpoint,
    const std::chrono::system_clock::time_point& lastSeen,
    const std::chrono::system_clock::time_point& nextRequest)
{
    auto const addr = endpoint.address().to_string();
    auto const port = endpoint.port();
    auto const last = std::chrono::duration_cast<std::chrono::seconds>(lastSeen.time_since_epoch());
    auto const next = std::chrono::duration_cast<std::chrono::seconds>(nextRequest.time_since_epoch());

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "INSERT INTO nodes (remote_addr, port, last_seen, next_request) VALUES ($1,$2,$3,$4);",
        -1,
        &stmt,
        nullptr);
    sqlite3_bind_text(stmt,  1, addr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,   2, port);
    sqlite3_bind_int64(stmt, 3, last.count());
    sqlite3_bind_int64(stmt, 4, next.count());
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool Node::Exists(
    sqlite3* db,
    const boost::asio::ip::udp::endpoint& endpoint)
{
    auto const addr = endpoint.address().to_string();
    auto const port = endpoint.port();

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "SELECT 1 FROM nodes WHERE remote_addr = $1 AND port = $2",
        -1,
        &stmt,
        nullptr);
    sqlite3_bind_text(stmt,  1, addr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,   2, port);

    bool exists = false;

    switch (sqlite3_step(stmt))
    {
        case SQLITE_ROW:
            exists = true;
            break;
    }

    sqlite3_finalize(stmt);

    return exists;
}

void Node::GetWhereNextRequestExpired(
    sqlite3 *db,
    const std::chrono::system_clock::time_point &nextRequest,
    const std::function<void(const boost::asio::ip::udp::endpoint &)> &callback)
{
    auto const next = std::chrono::duration_cast<std::chrono::seconds>(nextRequest.time_since_epoch());

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "SELECT remote_addr,port FROM nodes WHERE next_request < $1",
        -1,
        &stmt,
        nullptr);
    sqlite3_bind_int64(stmt, 1, next.count());

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        auto const addr = sqlite3_column_text(stmt, 0);
        auto const port = sqlite3_column_int(stmt,  1);

        callback(
            boost::asio::ip::udp::endpoint(
                boost::asio::ip::make_address(reinterpret_cast<const char*>(addr)),
                port));
    }

    sqlite3_finalize(stmt);
}

void Node::Update(
    sqlite3* db,
    const boost::asio::ip::udp::endpoint& endpoint,
    const std::chrono::system_clock::time_point& lastSeen,
    const std::chrono::system_clock::time_point& nextRequest)
{
    auto const addr = endpoint.address().to_string();
    auto const port = endpoint.port();
    auto const last = std::chrono::duration_cast<std::chrono::seconds>(lastSeen.time_since_epoch());
    auto const next = std::chrono::duration_cast<std::chrono::seconds>(nextRequest.time_since_epoch());

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(
        db,
        "UPDATE nodes SET last_seen = $4, next_request = $3 WHERE remote_addr = $1 AND port = $2;",
        -1,
        &stmt,
        nullptr);
    sqlite3_bind_text(stmt,  1, addr.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt,   2, port);
    sqlite3_bind_int64(stmt, 3, last.count());
    sqlite3_bind_int64(stmt, 4, next.count());
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
