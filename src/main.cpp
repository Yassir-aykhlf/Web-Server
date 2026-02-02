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

// 1 - programe take a confige file (.conf)
// 2 - open it check it's valid to open and existe if not throw an excpetion
// 3 - proccess file , i need to throw an error sepicifique to that problem
// 4 - after parsing it , it will be stored in a data structure that the the others will use to proccess things
//  i think this is my main goal for now

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
    return 0;
}

// TODO: learn about
// a line end with # or newline or EOF in code
// add comments in deauflt.conf to explain each directive , spicificaly cgi
// add error handling for missing or invalid configuration directives
// add default values for missing configuration options
// define parsing logic in parse method to populate servers vector
// define validation logic in validate method to ensure configuration integrity
// integrate Config class with Server class to initialize server instances based on loaded configuration
// test heavelly the configuration loading and matching logic
