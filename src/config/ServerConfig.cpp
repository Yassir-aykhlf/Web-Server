#include "ServerConfig.hpp"

ServerConfig::ServerConfig()
    : _host(DEFAULT_HOST)
    , _port(DEFAULT_PORT)
    , _root("./www/html")
    , _index("index.html")
    , _clientMaxBodySize(DEFAULT_MAX_BODY_SIZE) {
}

ServerConfig::ServerConfig(const ServerConfig& other) {
    *this = other;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        _host = other._host;
        _port = other._port;
        _serverNames = other._serverNames;
        _root = other._root;
        _index = other._index;
        _clientMaxBodySize = other._clientMaxBodySize;
        _errorPages = other._errorPages;
        _locations = other._locations;
    }
    return *this;
}

ServerConfig::~ServerConfig() {}

const std::string& ServerConfig::getHost() const { return _host; }
int ServerConfig::getPort() const { return _port; }
const std::vector<std::string>& ServerConfig::getServerNames() const { return _serverNames; }
const std::string& ServerConfig::getRoot() const { return _root; }
const std::string& ServerConfig::getIndex() const { return _index; }
size_t ServerConfig::getClientMaxBodySize() const { return _clientMaxBodySize; }
const std::map<int, std::string>& ServerConfig::getErrorPages() const { return _errorPages; }
const std::vector<LocationConfig>& ServerConfig::getLocations() const { return _locations; }

void ServerConfig::setHost(const std::string& host) { _host = host; }
void ServerConfig::setPort(int port) { _port = port; }
void ServerConfig::addServerName(const std::string& name) { _serverNames.push_back(name); }
void ServerConfig::setRoot(const std::string& root) { _root = root; }
void ServerConfig::setIndex(const std::string& index) { _index = index; }
void ServerConfig::setClientMaxBodySize(size_t size) { _clientMaxBodySize = size; }
void ServerConfig::addErrorPage(int code, const std::string& path) { _errorPages[code] = path; }
void ServerConfig::addLocation(const LocationConfig& location) { _locations.push_back(location); }

const LocationConfig* ServerConfig::matchLocation(const std::string& uri) const {
    const LocationConfig* match = NULL;
    size_t longestMatch = 0;
    for (size_t i = 0; i < _locations.size(); ++i) {
        const std::string& path = _locations[i].getPath();
        if (uri == path) {
            return &_locations[i];
        }
        if (uri.compare(0, path.length(), path) == 0) {
            if (path.length() > longestMatch) {
                if (path == "/" || path[path.length() - 1] == '/' ||
                    uri.length() == path.length() || uri[path.length()] == '/') {
                    longestMatch = path.length();
                    match = &_locations[i];
                }
            }
        }
    }
    
    return match;
}

std::string ServerConfig::getErrorPage(int code) const {
    std::map<int, std::string>::const_iterator it = _errorPages.find(code);
    if (it != _errorPages.end())
        return it->second;
    return "";
}

bool ServerConfig::hasServerName(const std::string& name) const {
    for (size_t i = 0; i < _serverNames.size(); ++i) {
        if (_serverNames[i] == name)
            return true;
    }
    return false;
}
