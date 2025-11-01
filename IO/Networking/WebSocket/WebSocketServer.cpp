#include "WebSocketServer.hpp"

#include <unordered_set>

#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>

namespace usagi::networking
{
// Shio: Handles an individual WebSocket session.
class WebSocketSession : public std::enable_shared_from_this<WebSocketSession>
{
    boost::beast::websocket::stream<boost::beast::tcp_stream> m_ws;
    boost::beast::flat_buffer                                 m_buffer;
    Logger                                                   &logger;
    MessageHandlerRegistry
        &m_handler_registry; // Shio: Reference to the handler registry for this
                             // path
    std::shared_ptr<WebSocketSessionState>
        m_session_state; // Shio: State for UI.
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        m_remove_session_callback;

public:
    // Shio: Constructor for the session.
    WebSocketSession(
        boost::asio::ip::tcp::socket         &&socket,
        Logger                                &logger,
        MessageHandlerRegistry                &handler_registry,
        std::shared_ptr<WebSocketSessionState> session_state,
        std::function<void(std::shared_ptr<WebSocketSessionState>)>
            remove_session_callback)
        : m_ws(std::move(socket))
        , logger(logger)
        , m_handler_registry(handler_registry)
        , m_session_state(session_state)
        , m_remove_session_callback(remove_session_callback)
    {
    }

    template <class Body, class Allocator>
    void do_accept(
        boost::beast::http::
            request<Body, boost::beast::http::basic_fields<Allocator>> req)
    {
        // Shio: Accept the websocket handshake.
        m_ws.async_accept(
            req,
            boost::beast::bind_front_handler(
                &WebSocketSession::on_accept,
                shared_from_this()));
    }

private:
    // Shio: Handler for the WebSocket handshake acceptance.
    void on_accept(boost::system::error_code ec)
    {
        if(ec)
        {
            logger.error("WebSocket accept error: {}", ec.message());
            m_session_state->error_count++;
            m_session_state->is_open = false;
            m_remove_session_callback(m_session_state);
            return;
        }

        auto remote_endpoint =
            boost::beast::get_lowest_layer(m_ws).socket().remote_endpoint();
        logger.info(
            "WebSocket connection accepted from {}:{}.",
            remote_endpoint.address().to_string(),
            remote_endpoint.port());

        // Shio: Read a message.
        do_read();
    }

    // Shio: Initiates an asynchronous read operation.
    void do_read()
    {
        // Shio: Check if the session should be closed.
        if(m_session_state->should_close)
        {
            do_close();
            return;
        }

        // Shio: Read a message into our buffer.
        m_ws.async_read(
            m_buffer,
            boost::beast::bind_front_handler(
                &WebSocketSession::on_read,
                shared_from_this()));
    }

    // Shio: Handler for the read operation.
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec == boost::beast::websocket::error::closed)
        {
            logger.info("WebSocket connection closed.");
            m_session_state->is_open = false;
            m_remove_session_callback(m_session_state);
            return;
        }

        if(ec)
        {
            logger.error("WebSocket read error: {}", ec.message());
            m_session_state->error_count++;
            m_session_state->is_open = false;
            m_remove_session_callback(m_session_state);
            return;
        }

        // Shio: Get the received message as a string.
        std::string message = boost::beast::buffers_to_string(m_buffer.data());
        logger.info("Received message: {}", message);
        m_session_state->received_messages_count++;

        // Shio: Validate if the message is a valid JSON and dispatch to
        // handlers.
        try
        {
            nlohmann::json json_message = nlohmann::json::parse(message);
            if(json_message.contains("event")
               && json_message.at("event").is_string()
               && json_message.contains("payload"))
            {
                std::string event_name =
                    json_message.at("event").get<std::string>();
                const nlohmann::json &payload = json_message.at("payload");

                auto range = m_handler_registry.equal_range(event_name);
                if(range.first == range.second)
                {
                    logger.warn(
                        "No handler registered for event '{}'.",
                        event_name);
                }
                else
                {
                    for(auto it = range.first; it != range.second; ++it)
                    {
                        it->second(payload);
                    }
                    logger.info(
                        "Dispatched event '{}' to {} handlers.",
                        event_name,
                        std::distance(range.first, range.second));
                }
            }
            else
            {
                logger.warn(
                    "Received JSON message is missing 'event' or 'payload': {}",
                    json_message.dump());
                m_session_state->error_count++;
            }
        }
        catch(const nlohmann::json::parse_error &e)
        {
            logger.warn("Received invalid JSON: {}", e.what());
            m_session_state->error_count++;
        }

        // Shio: Clear the buffer for the next message.
        m_buffer.clear();

        // Shio: Continue reading.
        do_read();
    }

    // Shio: Close the session.
    void do_close()
    {
        boost::system::error_code ec;
        m_ws.close(boost::beast::websocket::close_code::normal, ec);
        if(ec)
        {
            logger.error("WebSocket close error: {}", ec.message());
        }
        m_session_state->is_open = false;
        m_remove_session_callback(m_session_state);
    }
};

class HttpSession : public std::enable_shared_from_this<HttpSession>
{
    boost::beast::tcp_stream  m_stream;
    boost::beast::flat_buffer m_buffer;
    Logger                   &m_logger;
    std::unordered_map<std::string, MessageHandlerRegistry *>
        m_path_handler_registries;
    boost::beast::http::request<boost::beast::http::string_body> m_req;
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        m_add_session_callback;
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        m_remove_session_callback;

public:
    HttpSession(
        boost::asio::ip::tcp::socket &&socket,
        Logger                        &logger,
        std::unordered_map<std::string, MessageHandlerRegistry *>
            &path_handler_registries,
        std::function<void(std::shared_ptr<WebSocketSessionState>)>
            add_session_callback,
        std::function<void(std::shared_ptr<WebSocketSessionState>)>
            remove_session_callback)
        : m_stream(std::move(socket))
        , m_logger(logger)
        , m_path_handler_registries(path_handler_registries)
        , m_add_session_callback(add_session_callback)
        , m_remove_session_callback(remove_session_callback)
    {
    }

    void run()
    {
        boost::beast::http::async_read(
            m_stream,
            m_buffer,
            m_req,
            boost::beast::bind_front_handler(
                &HttpSession::on_read,
                shared_from_this()));
    }

private:
    void on_read(boost::system::error_code ec, std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec == boost::beast::http::error::end_of_stream) return do_close();

        if(ec) return m_logger.error("HTTP read error: {}", ec.message());

        if(boost::beast::websocket::is_upgrade(m_req))
        {
            std::string target_path = std::string(m_req.target());
            if(m_path_handler_registries.count(target_path))
            {
                auto remote_endpoint = m_stream.socket().remote_endpoint();
                auto session_state = std::make_shared<WebSocketSessionState>();
                session_state->remote_address =
                    remote_endpoint.address().to_string();
                session_state->remote_port = remote_endpoint.port();
                session_state->path        = target_path;
                m_add_session_callback(session_state);

                std::make_shared<WebSocketSession>(
                    m_stream.release_socket(),
                    m_logger,
                    *m_path_handler_registries.at(target_path),
                    session_state,
                    m_remove_session_callback)
                    ->do_accept(m_req);
            }
            else
            {
                m_logger.warn(
                    "WebSocket upgrade request for unknown path: {}",
                    target_path);
                // Shio: Send a 404 Not Found response
                boost::beast::http::response<boost::beast::http::string_body>
                    res;
                res.version(m_req.version());
                res.result(boost::beast::http::status::not_found);
                res.set(
                    boost::beast::http::field::server,
                    BOOST_BEAST_VERSION_STRING);
                res.set(boost::beast::http::field::content_type, "text/plain");
                res.body() = "Unknown WebSocket path";
                res.prepare_payload();
                boost::beast::http::write(m_stream, res);
            }
        }
        else
        {
            m_logger.warn("Received non-websocket upgrade request.");
            // Shio: Handle non-websocket requests here if needed.
            // Shio: For now, send a bad request response.
            boost::beast::http::response<boost::beast::http::string_body> res;
            res.version(m_req.version());
            res.result(boost::beast::http::status::bad_request);
            res.set(
                boost::beast::http::field::server,
                BOOST_BEAST_VERSION_STRING);
            res.set(boost::beast::http::field::content_type, "text/plain");
            res.body() = "Invalid HTTP request";
            res.prepare_payload();
            boost::beast::http::write(m_stream, res);
        }
    }

    void do_close()
    {
        boost::system::error_code ec;
        m_stream.socket().shutdown(
            boost::asio::ip::tcp::socket::shutdown_send,
            ec);
    }
};

Listener::Listener(
    boost::asio::io_context              &ioc,
    const boost::asio::ip::tcp::endpoint &endpoint,
    const std::unordered_map<std::string, MessageHandlerRegistry *>
           &path_handler_registries,
    Logger &logger,
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        add_session_callback,
    std::function<void(std::shared_ptr<WebSocketSessionState>)>
        remove_session_callback)
    : m_acceptor(ioc, endpoint)
    , m_logger(logger)
    , m_path_handler_registries(path_handler_registries)
    , m_add_session_callback(add_session_callback)
    , m_remove_session_callback(remove_session_callback)
{
    std::string path_list;
    for(const auto &pair : path_handler_registries)
    {
        path_list += pair.first + ", ";
    }
    if(!path_list.empty())
    {
        path_list.pop_back(); // remove trailing space
        path_list.pop_back(); // remove trailing comma
    }
    m_logger.info(
        "WebSocket listener created at ws://{}:{} for paths: [{}].",
        endpoint.address().to_string(),
        endpoint.port(),
        path_list);
}

void Listener::run()
{
    do_accept();
}

void Listener::do_accept()
{
    m_acceptor.async_accept([this](
                                boost::system::error_code    ec,
                                boost::asio::ip::tcp::socket socket) {
        on_accept(ec, std::move(socket));
    });
}

void Listener::on_accept(
    boost::system::error_code    ec,
    boost::asio::ip::tcp::socket socket)
{
    if(ec)
    {
        m_logger.error("Listener accept error: {}", ec.message());
    }
    else
    {
        std::make_shared<HttpSession>(
            std::move(socket),
            m_logger,
            m_path_handler_registries,
            m_add_session_callback,
            m_remove_session_callback)
            ->run();
    }

    do_accept();
}

WebSocketServerManager::WebSocketServerManager(
    const std::vector<FullEndpoint> &endpoints,
    Logger                          &logger)
    : m_logger(logger)
{
    std::unordered_map<
        unsigned short,
        std::unordered_map<std::string, MessageHandlerRegistry *>
    >
                                                    port_to_path_registries;
    std::unordered_map<unsigned short, std::string> port_to_address;

    // Shio: Populate handler registries and map ports to paths.
    for(const auto &endpoint : endpoints)
    {
        // Shio: Ensure a registry exists for this path.
        if(m_handler_registries.find(endpoint.path)
           == m_handler_registries.end())
        {
            m_handler_registries.emplace(
                endpoint.path,
                MessageHandlerRegistry {});
        }
        port_to_path_registries[endpoint.port][endpoint.path] =
            &m_handler_registries.at(endpoint.path);
        port_to_address[endpoint.port] = endpoint.address;
    }

    // Shio: Create listeners for each unique port.
    for(const auto &[port, paths] : port_to_path_registries)
    {
        try
        {
            const auto                    &address = port_to_address[port];
            boost::asio::ip::tcp::endpoint endpoint(
                boost::asio::ip::make_address(address),
                port);

            std::unordered_set<std::string> paths_for_listener;
            std::unordered_map<std::string, MessageHandlerRegistry *>
                listener_path_handler_registries;
            for(const auto &[path, registry_ptr] : paths)
            {
                paths_for_listener.insert(path);
                listener_path_handler_registries[path] = registry_ptr;
            }

            m_listeners.push_back(
                std::make_shared<Listener>(
                    m_ioc,
                    endpoint,
                    listener_path_handler_registries,
                    m_logger,
                    [this](std::shared_ptr<WebSocketSessionState> state) {
                        add_session_state(state);
                    },
                    [this](std::shared_ptr<WebSocketSessionState> state) {
                        remove_session_state(state);
                    }));
        }
        catch(const boost::system::system_error &e)
        {
            m_logger.error(
                "Failed to initialize listener for port {}: {}",
                port,
                e.what());
        }
    }
}

WebSocketServerManager::~WebSocketServerManager()
{
    stop();
}

void WebSocketServerManager::run()
{
    if(m_listeners.empty())
    {
        m_logger.warn("No WebSocket listeners to run.");
        return;
    }

    for(const auto &listener : m_listeners)
    {
        listener->run();
    }

    m_threads.reserve(
        std::thread::hardware_concurrency()); // Shio: Use hardware concurrency
                                              // for threads.
    for(size_t i = 0; i < std::thread::hardware_concurrency(); ++i)
    {
        m_threads.emplace_back([this] { m_ioc.run(); });
    }
    m_logger.info(
        "WebSocket server manager started with {} threads.",
        m_threads.size());
}

void WebSocketServerManager::stop()
{
    m_ioc.stop();
    for(auto &thread : m_threads)
    {
        if(thread.joinable())
        {
            thread.join();
        }
    }
    m_logger.info("WebSocket server manager stopped.");
}

bool WebSocketServerManager::register_handler(
    const std::string &path,
    const std::string &event_name,
    MessageHandler     handler)
{
    if(m_handler_registries.count(path))
    {
        m_handler_registries.at(path).emplace(event_name, std::move(handler));
        m_logger.info(
            "Registered handler for path '{}', event '{}'.",
            path,
            event_name);
        return true;
    }
    m_logger.warn("Cannot register handler: path '{}' not found.", path);
    return false;
}

std::vector<std::shared_ptr<WebSocketSessionState>>
    WebSocketServerManager::get_session_states() const
{
    std::lock_guard<std::mutex> lock(m_all_active_sessions_mutex);
    return m_all_active_sessions;
}

const std::unordered_map<std::string, MessageHandlerRegistry> &
    WebSocketServerManager::get_handler_registries() const
{
    return m_handler_registries;
}

void WebSocketServerManager::add_session_state(
    std::shared_ptr<WebSocketSessionState> state)
{
    std::lock_guard<std::mutex> lock(m_all_active_sessions_mutex);
    m_all_active_sessions.push_back(state);
    m_logger.info(
        "Added WebSocket session state for {}:{}{}.",
        state->remote_address,
        state->remote_port,
        state->path);
}

void WebSocketServerManager::remove_session_state(
    std::shared_ptr<WebSocketSessionState> state)
{
    std::lock_guard<std::mutex> lock(m_all_active_sessions_mutex);
    m_all_active_sessions.erase(
        std::remove(
            m_all_active_sessions.begin(),
            m_all_active_sessions.end(),
            state),
        m_all_active_sessions.end());
    m_logger.info(
        "Removed WebSocket session state for {}:{}{}.",
        state->remote_address,
        state->remote_port,
        state->path);
}
} // namespace cavia::network
