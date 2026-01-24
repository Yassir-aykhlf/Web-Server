#include "Server.hpp"
#include "Logger.hpp"

int main(int argc, char** argv) {
    std::string configFile;
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }
    if (argc == 2) {
        configFile = argv[1];
    } else {
        configFile = "config/default.conf";
    }
    Logger::info("Webserv " SERVER_NAME " starting...");
    Server server;
    if (!server.init(configFile)) {
        Logger::error("Failed to initialize server");
        return 1;
    }
    server.run(); 
    Logger::info("Webserv shut down successfully");
    return 0;
}