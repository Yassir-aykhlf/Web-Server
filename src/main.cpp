#include <string>
#include <iostream>
#include "webserv.hpp"

/* 
handles command-line arguments,
instantiates the Server object,
triggers initialization with the configuration file,
and starts the main execution loop.
*/

int main(int argc, char **argv) {
    std::string configFile;
    Server      server; // TODO: create server class

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }
    if (argc == 2) {
        configFile = argv[1];
    } else {
        configFile = "config/default.conf";
    }

    if (!server.init(configFile)) {
        // log error
        return 1;
    }
    server.run();
    // log server shut down
    return 0;
}