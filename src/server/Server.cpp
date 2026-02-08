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