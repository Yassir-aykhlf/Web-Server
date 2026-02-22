#include "webserv.hpp"
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char **argv)
{
    if (argc > 2)
    {
        cerr << "Usage: " << argv[0] << " [config_file]" << endl;
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

        Server server(&config);
        server.init();
        server.run();
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
    return 0;
}
