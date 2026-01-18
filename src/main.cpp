#include <string>
#include <iostream>
#include "webserv.hpp"

/* 
handles command-line arguments,
instantiates the Server object,
triggers initialization with the configuration file,
and starts the main execution loop.
*/


// this is my world now , let's go and start working on it 

//1 - programe take a confige file (.cfg)
//2 - open it check it's valid to open and existe if not throw an excpetion
//3 - proccess file , i need to throw an error sepicifique to that problem
//4 - after parsing it , it will be stored in a data structure that the the others will use to proccess things 
// i think this is my 


int main(int argc, char **argv) {
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [config_file]" << std::endl;
        return 1;
    }

    std::string configFile;
    if (argc == 2)
        configFile = argv[1];
    else
        configFile = "ConfigTestFiles/default.conf";

    try {
        Config config;
        config.load(configFile);      // open + parse + validate

        //ServerConfig server(config);     // here Config structure will be read
        // server.run();
    }
    catch (const ConfigException& e) {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
