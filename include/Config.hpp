#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "webserv.hpp"
#include "ServerConfig.hpp"

class Config {
public:
    Config();
    Config(const std::string& filename);
    Config(const Config& other);
    Config& operator=(const Config& other);
    ~Config();

    const std::vector<ServerConfig>& getServers() const;
    bool load(const std::string& filename);
    bool validate() const;
    const ServerConfig* getServerByHostPort(const std::string& host, int port) const;
    std::vector<int>    getUniquePorts() const;

private:
    std::vector<ServerConfig> _servers;
};

#endif
