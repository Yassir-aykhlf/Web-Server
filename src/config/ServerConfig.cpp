#include "ServerConfig.hpp"
using namespace std;

ServerConfig::ServerConfig(const string &host, int port)
: host_(host), port_(port), socket_fd_(-1) {}

void   ServerConfig::setHost(const string &host) { host_ = host; }
const string &  ServerConfig::getHost() const { return host_; }
void   ServerConfig::setPort(int port) { port_ = port; }
int   ServerConfig::getPort() const { return port_; }
void   ServerConfig::setSocketFD(int fd) { socket_fd_ = fd; }
int   ServerConfig::getSocketFD() const { return socket_fd_; }

void ServerConfig::addServerNode(const ConfigNode &node) {
    nodes_.push_back(node);
}

const vector<ConfigNode>& ServerConfig::getNodes() const {
    return nodes_;
}

vector<string> ServerConfig::getServerNames(const ConfigNode& serverNode) const {
    vector<string> serverNames;
    const vector<ConfigNode>& directives = serverNode.getChildren();
    
    for (size_t i = 0; i < directives.size(); ++i) {
        if (directives[i].getName() == "server_name") {
            const vector<string>& args = directives[i].getArguments();
            serverNames.insert(serverNames.end(), args.begin(), args.end());
        }
    }
    if (serverNames.empty()) {
        serverNames.push_back("");
    }
    
    return serverNames;
}