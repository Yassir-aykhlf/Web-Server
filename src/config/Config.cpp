/* 
The top-level container for the application's configuration.
It loads the config file via ConfigParser,
stores a list of ServerConfig objects,
and performs global validation. 
*/
#include "Config.hpp"


Config::Config(const std::string& path) {
    load(path);
}

Congfig::load(const std::string &path) {
std::ifstream file(path);
if (!file.is_open())
    throw ConfigException("Cannot open config file: " + path);
parse(file);
validate();
file.close();
}

Config::parse(std::istream &in) {
    // Parsing logic to populate 'servers' vector
    // This is a placeholder; actual parsing code would go here
}

Config::validate() const {
    // Validation logic for the loaded configuration
    // This is a placeholder; actual validation code would go here
}