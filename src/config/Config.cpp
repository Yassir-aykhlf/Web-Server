#include "Config.hpp"
#include "ConfigParser.hpp"
#include "Logger.hpp"

Config::Config() {}

Config::Config(const std::string& filename) {
    load(filename);
}

Config::Config(const Config& other) {
    *this = other;
}

Config& Config::operator=(const Config& other) {
    if (this != &other) {
        _servers = other._servers;
    }
    return *this;
}

Config::~Config() {}

bool Config::load(const std::string& filename) {
    ConfigParser parser;
    if (!parser.parse(filename, _servers)) {
        Logger::error("Failed to parse configuration: " + parser.getError());
        return false;
    }
    if (!validate()) {
        return false;
    }
    Logger::info("Configuration loaded: " + intToString(_servers.size()) + " server(s)");
    return true;
}

bool Config::validate() const {
    if (_servers.empty()) {
        Logger::error("No servers configured");
        return false;
    }
    
    std::map<int, std::vector<std::string> > portNames;
    
    for (size_t i = 0; i < _servers.size(); ++i) {
        int port = _servers[i].getPort();
        
        if (port < 1 || port > 65535) {
            Logger::error("Invalid port number: " + intToString(port));
            return false;
        }
        
        const std::vector<std::string>& names = _servers[i].getServerNames();
        if (names.empty()) {
            if (portNames.find(port) != portNames.end()) {
                std::vector<std::string>& existingNames = portNames[port];
                for (size_t j = 0; j < existingNames.size(); ++j) {
                    if (existingNames[j].empty()) {
                        Logger::error("Duplicate default server on port " + intToString(port));
                        return false;
                    }
                }
            }
            portNames[port].push_back("");
        } else {
            for (size_t j = 0; j < names.size(); ++j) {
                if (portNames.find(port) != portNames.end()) {
                    std::vector<std::string>& existingNames = portNames[port];
                    for (size_t k = 0; k < existingNames.size(); ++k) {
                        if (existingNames[k] == names[j]) {
                            Logger::error("Duplicate server_name '" + names[j] + 
                                        "' on port " + intToString(port));
                            return false;
                        }
                    }
                }
                portNames[port].push_back(names[j]);
            }
        }
    }
    
    return true;
}

const ServerConfig* Config::getServerByHostPort(const std::string& host, int port) const {
    const ServerConfig* defaultServer = NULL;
    
    for (size_t i = 0; i < _servers.size(); ++i) {
        if (_servers[i].getPort() == port) {
            if (_servers[i].hasServerName(host)) {
                return &_servers[i];
            }
            if (defaultServer == NULL) {
                defaultServer = &_servers[i];
            }
        }
    }
    
    return defaultServer;
}

std::vector<int> Config::getUniquePorts() const {
    std::set<int> portSet;
    for (size_t i = 0; i < _servers.size(); ++i) {
        portSet.insert(_servers[i].getPort());
    }
    return std::vector<int>(portSet.begin(), portSet.end());
}
