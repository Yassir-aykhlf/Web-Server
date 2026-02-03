#include "Server.hpp"
#include "Logger.hpp"

int main(int argc, char **argv) {
    std::string configFile;
    Server      server; // TODO: create server class

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }

    std::string configFile;
    if (argc == 2)
        configFile = argv[1];
    else
        configFile = "Configs/default.conf";
    try
    {
        Config config(configFile);
        config.load();
        config.printAST(); //? print the AST for debugging
    }
    catch (const ConfigException &e)
    {
        std::cerr << "Configuration Error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    server.run(); 
    Logger::info("Webserv shut down successfully");
    return 0;
}

// TODO: learn about
// add error handling for missing or invalid configuration directives
// add default values for missing configuration options
// define parsing logic in parse method to populate servers vector
// define validation logic in validate method to ensure configuration integrity
// integrate Config class with Server class to initialize server instances based
// on loaded configuration test heavelly the configuration loading and matching
// logic
