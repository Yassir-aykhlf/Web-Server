// #include "Server.hpp"
// #include "Logger.hpp"

// Server::Server() : _running(false) {}

// Server::~Server() {
//     cleanup();
// }

// void Server::handleSignal(int sig) {
//     if (_instance) {
//         if (sig == SIGINT || sig == SIGTERM) {
//             Logger::info("Received signal " + intToString(sig) + ", shutting down...");
//             _instance->stop();
//         }
//     }
// }


// bool Server::init(const std::string& configFile) {
//     signal(SIGINT, handleSignal);
//     signal(SIGTERM, handleSignal);
//     _instance = this;
//     Logger::info("Loading configuration from: " + configFile);
//     if (!_config.load(configFile)) {
//         Logger::error("Failed to load configuration");
//         return false;
//     }
//     // _eventLoop.setConfig(&_config);
//     if (!setupListeners()) {
//         Logger::error("Failed to set up listerners");
//         cleanup();
//         return false;
//     }
//     _running = true;
//     return true;
// }

// void Server::run() {
//     if (!_running) {
//         Logger::error("Server not initialized");
//         return ;
//     }
//     Logger::info("Server starting...");
//     // _eventLoop.run();
//     cleanup();
// }

// void Server::stop() {
//     Logger::info("Server stopping...");
//     _running = false;
//     // _eventLoop.stop();
// }

// bool Server::setupListeners() {
//     // TODO: Socket stuff;
// }

// void Server::cleanup() {
//     // close sockets;
// }

bool Server::init(const std::string& configFile) {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    _instance = this;
    Logger::info("Loading configuration from: " + configFile);
    if (!_config.load(configFile)) {
        Logger::error("Failed to load configuration");
        return false;
    }
    // _eventLoop.setConfig(&_config);

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

    Logger::info("Server listening on " + host + ":" + intToString(port) + " (fd: " + intToString(fd_socket) + ")");
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
    int fd_socket = _config.getFdSocket();

    if (listen(fd_socket, SOMAXCONN) < 0) {
        Logger::error("Failed to listen on socket");
        close(fd_socket);
        return false;
    }
    return true;
}

bool Server::setupServerSockets() {
    // Create the server socket based on configuration
    const std::string& host = _config.getHost();
    int port = _config.getPort();
    int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0) {
        Logger::error("Failed to create socket");
        return false;
    }
    _config.setFdSocket(fd_socket);
    return true;
}

bool Server::setOptions()
{
    int fd_socket = _config.getFdSocket();

    non_blocking(fd_socket);
    // Reuse address option
    int opt = 1;
    if (setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        Logger::error("Failed to set socket options");
        close(fd_socket);
        return false;
    }
    return true;
}

bool Server::bindSocket()
{
    const std::string& host = _config.getHost();
    int port = _config.getPort();
    int fd_socket = _config.getFdSocket();

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
    return true;
}

void Server::cleanup() {
    // close sockets;
    int fd_socket = _config.getFdSocket();
    if (fd_socket >= 0) {
        close(fd_socket);
        Logger::info("Closed server socket (fd: " + intToString(fd_socket) + ")");
        _config.setFdSocket(-1);
    }
}

