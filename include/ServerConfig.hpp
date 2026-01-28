#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP

#include "webserv.hpp"
#include "LocationConfig.hpp"

class ServerConfig {
public:
    ServerConfig();
    ServerConfig(const ServerConfig& other);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();

    const std::string& getHost() const;
    int getPort() const;
    int getFdSocket() const;
    const std::vector<std::string>& getServerNames() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    size_t getClientMaxBodySize() const;
    const std::map<int, std::string>& getErrorPages() const;
    const std::vector<LocationConfig>& getLocations() const;

    void setHost(const std::string& host);
    void setPort(int port);
    void setFdSocket(int fd);
    void addServerName(const std::string& name);
    void setRoot(const std::string& root);
    void setIndex(const std::string& index);
    void setClientMaxBodySize(size_t size);
    void addErrorPage(int code, const std::string& path);
    void addLocation(const LocationConfig& location);

    const LocationConfig* matchLocation(const std::string& uri) const;
    std::string getErrorPage(int code) const;
    bool hasServerName(const std::string& name) const;

private:
    std::string _host;
    int _port;
    std::vector<std::string> _serverNames;
    std::string _root;
    std::string _index;
    size_t _clientMaxBodySize;
    std::map<int, std::string> _errorPages;
    std::vector<LocationConfig> _locations;
    int fd_socket;
};

#endif
