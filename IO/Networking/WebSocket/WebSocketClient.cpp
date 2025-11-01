#include "WebSocketClient.hpp"

namespace usagi::networking
{
WebSocketClient::WebSocketClient(
    boost::asio::io_context &ioc,
    const FullEndpoint      &endpoint)
    : m_ioc(ioc)
    , m_resolver(ioc)
    , m_ws(ioc)
    , m_endpoint(endpoint)
{
}

void WebSocketClient::connect()
{
    // Shio: Look up the domain name.
    auto const results =
        m_resolver.resolve(m_endpoint.address, std::to_string(m_endpoint.port));

    // Shio: Make the connection on the IP address we get.
    boost::beast::get_lowest_layer(m_ws).connect(results);

    // Shio: Perform the websocket handshake.
    m_ws.handshake(m_endpoint.address, m_endpoint.path);
}

void WebSocketClient::send(const std::string &message)
{
    m_ws.write(boost::asio::buffer(message));
}

std::string WebSocketClient::read()
{
    m_buffer.clear();
    m_ws.read(m_buffer);
    return boost::beast::buffers_to_string(m_buffer.data());
}

void WebSocketClient::close()
{
    m_ws.close(boost::beast::websocket::close_code::normal);
}
} // namespace cavia::network
