#include "webserv.hpp"
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char **argv)
{
    if (argc > 2)
        return printUsage(argv[0]);

    const std::string config_path = resolveConfigPath(argc, argv);

    try {
        return runServer(config_path);
    }
    catch (const ConfigException &e) {
        std::cerr << "Configuration error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
