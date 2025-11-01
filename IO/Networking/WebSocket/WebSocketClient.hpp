#pragma once

#include <string>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "Network/WebSocket/WebSocketServer.hpp"

namespace usagi::networking
{
class WebSocketClient
{
public:
    WebSocketClient(boost::asio::io_context &ioc, const FullEndpoint &endpoint);

    void connect();
    void send(const std::string &message);
    std::string read();
    void close();

private:
    [[maybe_unused]]
    boost::asio::io_context                                  &m_ioc;
    boost::asio::ip::tcp::resolver                            m_resolver;
    boost::beast::websocket::stream<boost::beast::tcp_stream> m_ws;
    FullEndpoint                                              m_endpoint;
    boost::beast::flat_buffer                                 m_buffer;
};
} // namespace cavia::network
