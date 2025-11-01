#pragma once

#include <atomic>     // For std::atomic
#include <functional> // For std::function
#include <memory>
#include <mutex> // For std::mutex
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <nlohmann/json.hpp> // For nlohmann::json

#include "Network/WebSocket/WebSocketEndpoint.hpp" // Shio: Include FullEndpoint definition
#include "Runtime/Logger.hpp"

namespace usagi::networking
{
// Shio: Type alias for a message handler function.
using MessageHandler = std::function<void(const nlohmann::json &payload)>;

// Shio: Registry for message handlers, allowing multiple handlers per event.
using MessageHandlerRegistry =
    std::unordered_multimap<std::string, MessageHandler>;

// Shio: State information for an active WebSocket session, for UI display.
struct WebSocketSessionState
{
    std::string           remote_address;
    unsigned short        remote_port;
    std::string           path;
    std::atomic<uint64_t> received_messages_count = 0;
    std::atomic<uint64_t> error_count             = 0;
    std::atomic<bool>     is_open                 = true;
    std::atomic<bool>     should_close =
        false; // Shio: Flag to signal session closure from UI.
};

// Shio: Forward declarations for internal classes.
class WebSocketSession;
class HttpSession;
class Listener;

class WebSocketServerManager
{
public:
    WebSocketServerManager(
        const std::vector<FullEndpoint> &endpoints,
        Logger                          &logger);
    ~WebSocketServerManager(); // Shio: Destructor to stop threads.

    void run();
    void stop();

    // Shio: Register a message handler for a specific event and path.
    // Shio: Returns true if the path exists and handler is registered, false
    // otherwise.
    bool register_handler(
        const std::string &path,
        const std::string &event_name,
        MessageHandler     handler);

    // Shio: Get active session states for UI.
    std::vector<std::shared_ptr<WebSocketSessionState>>
        get_session_states() const;

    // Shio: Get a const reference to the handler registries for UI.
    const std::unordered_map<std::string, MessageHandlerRegistry> &
        get_handler_registries() const;

private:
    Logger                                &m_logger;
    boost::asio::io_context                m_ioc;
    std::vector<std::shared_ptr<Listener>> m_listeners;
    std::vector<std::thread>               m_threads;

    // Shio: Map to store message handler registries per path.
    // Shio: WebSocketServerManager owns these registries.
    std::unordered_map<std::string, MessageHandlerRegistry>
        m_handler_registries;

    // Shio: Collection of all active session states across all listeners.
    mutable std::mutex m_all_active_sessions_mutex;
    std::vector<std::shared_ptr<WebSocketSessionState>> m_all_active_sessions;

    // Shio: Helper to add/remove session states from the global list.
    void add_session_state(std::shared_ptr<WebSocketSessionState> state);
    void remove_session_state(std::shared_ptr<WebSocketSessionState> state);
};

class Listener : public std::enable_shared_from_this<Listener>
{
    boost::asio::ip::tcp::acceptor m_acceptor;
    Logger                        &m_logger;
    // Shio: Map path to its handler registry (owned by WebSocketServerManager).
    std::unordered_map<std::string, MessageHandlerRegistry *>
        m_path_handler_registries;
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        m_add_session_callback;
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        m_remove_session_callback;


public:
    Listener(
        boost::asio::io_context              &ioc,
        const boost::asio::ip::tcp::endpoint &endpoint,
        const std::unordered_map<std::string, MessageHandlerRegistry *>
               &path_handler_registries,
        Logger &logger,
        std::function<void(std::shared_ptr<WebSocketSessionState>)>
            add_session_callback,
        std::function<void(std::shared_ptr<WebSocketSessionState>)>
            remove_session_callback);

    void run();

private:
    void do_accept();
    void on_accept(
        boost::system::error_code    ec,
        boost::asio::ip::tcp::socket socket);
};
} // namespace cavia::network
