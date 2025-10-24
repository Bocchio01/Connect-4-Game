#include <iostream>
#include <cstring>

#include <spdlog/spdlog.h>

#include "protocol/protocol.hpp"
#include "server/connection.hpp"

Connection::Connection(sockpp::tcp_socket socket, uint32_t id)
    : socket_(std::move(socket)), id_(id), running_(false)
{
}

Connection::~Connection()
{
    close();
}

void Connection::start()
{
    running_ = true;

    try
    {
        while (running_ && socket_.is_open())
        {
            std::string message = readMessage();

            if (message.empty())
            {
                // Connection closed or error
                break;
            }

            if (message_callback_)
            {
                message_callback_(message);
            }
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error("Connection {} error: {}", id_, e.what());
    }

    running_ = false;

    if (disconnect_callback_)
    {
        disconnect_callback_();
    }
}

bool Connection::send(const std::string &message)
{
    if (!socket_.is_open())
    {
        return false;
    }

    return writeMessage(message);
}

void Connection::close()
{
    running_ = false;
    if (socket_.is_open())
    {
        socket_.close();
    }
}

std::string Connection::readMessage()
{
    // Read 4-byte length prefix
    uint32_t length = 0;
    ssize_t n = socket_.read_n(&length, sizeof(length));

    if (n != sizeof(length))
    {
        return ""; // Connection closed or error
    }

    // Convert from network byte order
    length = ntohl(length);

    // Validate message size
    if (length == 0 || length > Protocol::MAX_MESSAGE_SIZE)
    {
        spdlog::error("Connection ID({}): Invalid message length: {}", id_, length);
        return "";
    }

    // Read message data
    std::vector<char> buffer(length);
    n = socket_.read_n(buffer.data(), length);

    if (n != static_cast<ssize_t>(length))
    {
        return ""; // Connection closed or error
    }

    return std::string(buffer.begin(), buffer.end());
}

bool Connection::writeMessage(const std::string &message)
{
    // Validate message size
    if (message.size() > Protocol::MAX_MESSAGE_SIZE)
    {
        spdlog::error("Connection ID({}): Message too large: {}", id_, message.size());
        return false;
    }

    // Write 4-byte length prefix
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));
    ssize_t n = socket_.write_n(&length, sizeof(length));

    if (n != sizeof(length))
    {
        return false;
    }

    // Write message data
    n = socket_.write_n(message.data(), message.size());

    return n == static_cast<ssize_t>(message.size());
}