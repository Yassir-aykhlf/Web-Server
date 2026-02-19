#include "Server.hpp"
#include "Logger.hpp"

Server* Server::_instance = NULL;

Server::Server(Config *config) : _config(config), _running(false) {}

Server::~Server() {
    cleanup();
}

void non_blockingServer(int fd) {
    std::cout << "Setting non-blocking mode for fd: " << fd << std::endl;
    int flags = fcntl(fd, F_GETFL, 0);
    int result = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (result == -1) {
        perror("Error setting non-blocking mode");
        exit(EXIT_FAILURE);
    }
}

void Server::handleSignal(int sig) {
    if (_instance) {
        if (sig == SIGINT || sig == SIGTERM) {
            Logger::info("Received signal " + intToString(sig) + ", shutting down...");
            _instance->stop();
        }
    }
}
// _config.getHost(), _config.getPort(), config.getFdSocket()

bool Server::init() {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    _instance = this;
    // Logger::info("Loading configuration from: " + configFile);
    // _config = Config();
    // if (!_config.load(configFile)) {
    //     Logger::error("Failed to load configuration");
    //     return false;
    // }

    // create server sockets
    if (this->setupServerSockets() == false)
        return false;

    // set non blocking and options
    if (this->setOptions() == false)
        return false;

   // bind socket
    if (this->bindSocket() == false)
        return false;

    // listen on socket
    if (this->setupListeners() == false)
        return false;

    _eventLoop = EventLoop();
    _eventLoop.setConfig(_config);

    // if (!setupListeners()) {
    //     Logger::error("Failed to set up listerners");
    //     cleanup();
    //     return false;
    // }
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
    vector<ServerConfigue>& servers = _config->getServerConfigues();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfigue& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        if (listen(fd_socket, SOMAXCONN) < 0) {
            Logger::error("Failed to listen on socket for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
            close(fd_socket);
            return false;
        }
    }
    return true;
}

bool Server::setupServerSockets() {
    // Create the server socket based on configuration
    vector<ServerConfigue>& servers = _config->getServerConfigues();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfigue& serverConfig = servers[i];
        int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_socket < 0) {
            Logger::error("Failed to create socket for server " + intToString(i));
            return false;
        }
        // Store the socket fd in the server config
        const_cast<ServerConfigue&>(serverConfig).setSocketFD(fd_socket);
        Logger::info("Created socket (fd: " + intToString(fd_socket) + ") for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
        Logger::info("Server socket (fd: " + intToString(serverConfig.getSocketFD()) + ") created successfully for server on " + serverConfig.getHost() + ":" + intToString(serverConfig.getPort()));
    }
    return true;
}

bool Server::setOptions()
{
    vector<ServerConfigue>& servers = _config->getServerConfigues();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfigue& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        non_blockingServer(fd_socket);
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
    vector<ServerConfigue>& servers = _config->getServerConfigues();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfigue& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        const std::string& host = serverConfig.getHost();
        int port = serverConfig.getPort();

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = inet_addr(host.c_str()); // Convert IP address string to binary
        server_addr.sin_port = htons(port);
        if (bind(fd_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            Logger::error("Failed to bind socket to " + host + ":" + intToString(port));
            close(fd_socket);
            return false;
        }
    }
    return true;
}

void Server::cleanup() {
    // close sockets;
    vector<ServerConfigue>& servers = _config->getServerConfigues();
    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfigue& serverConfig = servers[i];
        int fd_socket = serverConfig.getSocketFD();
        if (fd_socket >= 0) {
            close(fd_socket);
            Logger::info("Closed server socket (fd: " + intToString(fd_socket) + ")");
            const_cast<ServerConfigue&>(serverConfig).setSocketFD(-1);
        }
    }
}
