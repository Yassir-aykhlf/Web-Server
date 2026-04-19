#include "webserv.hpp"
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }

    std::string configFile;
    if (argc == 2)
        configFile = argv[1];
    else
        configFile = "configs/default.conf";
    try
    {
        Config config(configFile);
        config.load();

        Server server(&config);
        server.init();
        server.run();
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
    return 0;
}
