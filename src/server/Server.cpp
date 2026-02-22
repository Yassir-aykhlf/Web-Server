#include "Server.hpp"
#include "Logger.hpp"
#include <net/if.h>
using namespace std;

Server* Server::_instance = NULL;

Server::Server(Config *config) : _config(config), _running(false) {}

Server::~Server() {
    cleanup();
}

void Server::handleSignal(int sig) {
    if (_instance) {
        if (sig == SIGINT || sig == SIGTERM) {
            Logger::info("Received signal " + intToString(sig) + ", shutting down...");
            _instance->stop();
        }
    }
}

bool Server::init() {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    _instance = this;

    if (this->setupServerSockets() == false)
        return false;

    if (this->setOptions() == false)
        return false;

    if (this->bindSocket() == false)
        return false;

    if (this->setupListeners() == false)
        return false;

    _eventLoop = EventLoop();
    _eventLoop.setConfig(_config);

    _running = true;
    return true;
}

void Server::run() {
    if (!_running) {
        Logger::error("Server not initialized");
        return ;
    }
    Logger::info("Server starting...");
    _eventLoop.run();
    cleanup();
}

void Server::stop() {
    Logger::info("Server stopping...");
    _running = false;
    _eventLoop.stop();
}

bool Server::setupListeners() {
    vector<ServerConfig>& servers = _config->getServerConfigs();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        if (listen(fd_socket, SOMAXCONN) < 0) {
            Logger::error("Failed to listen on socket for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
            close(fd_socket);
            return false;
        }
    }
    return true;
}

bool Server::setupServerSockets()
{
    vector<ServerConfig> &servers = _config->getServerConfigs();
    for (size_t i = 0; i < servers.size(); ++i)
    {
        ServerConfig &serverConfig = servers[i];
        const std::string &host = serverConfig.getHost();
        int port = serverConfig.getPort();

        struct addrinfo hints, *result = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        std::string portStr = intToString(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || result == NULL)
        {
            Logger::error("Failed to resolve address: " + host);
            return false;
        }

        int fd_socket = socket(result->ai_family, SOCK_STREAM, 0); // AF_INET or AF_INET6 automatically
        freeaddrinfo(result);

        if (fd_socket < 0)
        {
            Logger::error("Failed to create socket for server " + intToString(i));
            return false;
        }

        serverConfig.setSocketFD(fd_socket); // no need for const_cast anymore
        Logger::info("Created socket (fd: " + intToString(fd_socket) + ") for server on " + host + ":" + portStr);
    }
    return true;
}

bool Server::setOptions()
{
    vector<ServerConfig>& servers = _config->getServerConfigs();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        non_blocking(fd_socket);
        // Reuse address option
        int opt = 1;
        if (setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            Logger::error("Failed to set socket options for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
            close(fd_socket);
            return false;
        }
    }
    return true;
}

bool Server::bindSocket()
{
    vector<ServerConfig> &servers = _config->getServerConfigs();
    for (size_t i = 0; i < servers.size(); ++i)
    {
        ServerConfig &serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        const std::string &host = serverConfig.getHost();
        int port = serverConfig.getPort();

        struct addrinfo hints, *result = NULL;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        std::string portStr = intToString(port);
        if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result) != 0 || result == NULL)
        {
            Logger::error("Failed to resolve address: " + host);
            close(fd_socket);
            return false;
        }

        // For link-local IPv6 addresses, set the scope_id if not resolved
        if (result->ai_family == AF_INET6)
        {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)result->ai_addr;
            if (addr6->sin6_scope_id == 0) {
                // Try common interface names
                unsigned int idx = if_nametoindex("eth0");
                if (idx == 0)
                    idx = if_nametoindex("en0");
                if (idx == 0)
                    idx = if_nametoindex("lo");
                if (idx == 0)
                    Logger::warning("No valid network interface found for IPv6 link-local scope_id");
                addr6->sin6_scope_id = idx;
            }
        }

        if (bind(fd_socket, result->ai_addr, result->ai_addrlen) < 0)
        {
            Logger::error("Failed to bind socket to " + host + ":" + portStr);
            freeaddrinfo(result);
            close(fd_socket);
            return false;
        }
        freeaddrinfo(result);
        Logger::info("Socket (fd: " + intToString(fd_socket) + ") bound to " + host + ":" + portStr + " successfully");
    }
    return true;
}

void Server::cleanup() {
    // close sockets;
    vector<ServerConfig>& servers = _config->getServerConfigs();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        if (fd_socket >= 0) {
            close(fd_socket);
            Logger::info("Closed server socket (fd: " + intToString(fd_socket) + ")");
            const_cast<ServerConfig&>(serverConfig).setSocketFD(-1);
        }
    }
}
