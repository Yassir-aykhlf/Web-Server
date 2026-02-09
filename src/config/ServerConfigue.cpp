#include "ServerConfigue.hpp"



ServerConfigue::ServerConfigue(const string &host, int port)
: host_(host), port_(port), socket_fd_(-1) {}

void   ServerConfigue::setHost(const string &host) { host_ = host; }
const string &  ServerConfigue::getHost() const { return host_; }
void   ServerConfigue::setPort(int port) { port_ = port; }
int   ServerConfigue::getPort() const { return port_; }
void   ServerConfigue::setSocketFD(int fd) { socket_fd_ = fd; }
int   ServerConfigue::getSocketFD() const { return socket_fd_; }

void ServerConfigue::addServerNode(const ConfigNode &node) {
    nodes_.push_back(node);
}

const vector<ConfigNode>& ServerConfigue::getNodes() const {
    return nodes_;
}


vector<string> ServerConfigue::getServerNames(const ConfigNode& serverNode) const {
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