#include "Server.hpp"
#include "Logger.hpp"
#include "socket_utils.h"
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
    signal(SIGPIPE, SIG_IGN);
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
        if (!startListening(fd_socket)) {
            Logger::error("Failed to listen on socket for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
            closeSocket(fd_socket);
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
        int fd_socket = createTcpSocket(host, port);
        if (fd_socket < 0)
        {
            Logger::error("Failed to create socket for server " + intToString(i));
            return false;
        }
        serverConfig.setSocketFD(fd_socket);
        Logger::info("Created socket (fd: " + intToString(fd_socket) + ") for server on " + host + ":" + intToString(port));
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
        if (!enableAddressReuse(fd_socket)) {
            Logger::error("Failed to set socket options for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
            closeSocket(fd_socket);
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
        if (!bindToAddress(fd_socket, host, port))
        {
            closeSocket(fd_socket);
            return false;
        }
        Logger::info("Socket (fd: " + intToString(fd_socket) + ") bound to " + host + ":" + intToString(port) + " successfully");
    }
    return true;
}

void Server::cleanup() {
    vector<ServerConfig>& servers = _config->getServerConfigs();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        if (fd_socket >= 0) {
            closeSocket(fd_socket);
            Logger::info("Closed server socket (fd: " + intToString(fd_socket) + ")");
            const_cast<ServerConfig&>(serverConfig).setSocketFD(-1);
        }
    }
}
