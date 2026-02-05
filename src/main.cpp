#include "Server.hpp"
#include "Logger.hpp"

int main(int argc, char **argv) {
    std::string configFile;
    Server      server; // TODO: create server class

    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }

    string configFile;
    if (argc == 2)
        configFile = argv[1];
    else
        configFile = "Configs/default.conf";
    try
    {
        Config config(configFile);
        config.load();
        config.printAST(); //? print the AST for debugging
        vector<ServerConfigue> serverConfs = config.getServerConfigues();
        // here example of how to route data
        ConfigRouter router(serverConfs[0]);
        // Location location = router.route("/some/path"); //TODO : make this 
        // string root = location["root"]; // TODO : Fix this
    }
    catch (const ConfigException &e)
    {
        cerr << "Configuration Error: " << e.what() << endl;
        return 1;
    }
    catch (const exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    server.run(); 
    Logger::info("Webserv shut down successfully");
    return 0;
}

// 
