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
        configFile = "ConfigTestFiles/default.conf";

    try
    {
        Config config(configFile);
        config.matchServer(8080, "localhost");
    }
    catch (const Config::ConfigException &e)
    {
        std::cerr << "Config error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
//! TODO:
// add comments in deauflt.conf to explain each directive , spicificaly cgi
// and keep understanding the config file structure
// add matchLocation method in Config class to match location based on request path
// add error handling for missing or invalid configuration directives
// add default values for missing configuration options
// define parsing logic in parse method to populate servers vector
// define validation logic in validate method to ensure configuration integrity
// integrate Config class with Server class to initialize server instances based on loaded configuration
// test heavelly the configuration loading and matching logic

//?
// Here is what I did so far:
// - Created Config class with load, parse, and validate methods
// - Created ServerConfig and LocationConfig structures to hold server settings
// - Implemented basic error handling with ConfigException
// - Matched server based on port and host

// You will use my matchServer method in main to get the right server config

// So you will have a request that will have a host and a port
// And you will use matchServer to get the right server config for that request
// This is what I think at least

// Default host is "localhost"
// Default port is 8080
// "listen 8080;" means that host is "localhost" by default, like in Nginx
// So if you have a request with host "localhost" and port 8080
// You will get the server config that has "listen 8080;" in the config file