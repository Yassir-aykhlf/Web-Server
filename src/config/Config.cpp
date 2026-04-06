#include "Config.hpp"
#include "webserv.hpp"

#include <sstream>
using namespace std;

Config::Config(string filename) : filename_(filename) {};

void Config::load()
{
    try
    {
        ConfigParser parser;
        ast_ = parser.parse(filename_);

        ConfigValidator validator;
        validator.validate(ast_);

        setServerConfigs();
    }
    catch (const ParseException &e)
    {
        throw ConfigException(string("Parsing error: ") + e.what());
    }
    catch (const ConfigException &e)
    {
        throw;
    }
}

bool Config::isIPv4(const string &ip)
{
    if (ip.empty())
        return false;
    vector<string> octets;
    stringstream ss(ip);
    string octet;
    while (getline(ss, octet, '.'))
    {
        octets.push_back(octet);
    }
    if (octets.size() != 4)
        return false;
    for (size_t i = 0; i < octets.size(); i++)
    {
        if (octets[i].empty())
            return false;
        for (size_t j = 0; j < octets[i].length(); j++)
        {
            if (!isdigit(octets[i][j]))
                return false;
        }
        stringstream octetSS(octets[i]);
        int num;
        if (!(octetSS >> num) || !octetSS.eof())
            return false;

        if (num < 0 || num > 255)
            return false;
    }
    return true;
}

void Config::inetAddressStr(sockaddr *addr, socklen_t addrlen, string &host, string &port)
{
    stringstream addrStr;
    char _host[NI_MAXHOST], service[NI_MAXSERV];
    if (getnameinfo(addr, addrlen, _host, NI_MAXHOST, service,
                    NI_MAXSERV, NI_NUMERICHOST) == 0)
    {
        host = _host;
        port = service;
    }
}

string Config::getIpByHost(const string &host)
{
    addrinfo hints, *result = NULL;
    string ip = "";
    string dummyPort = "";
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), NULL, &hints, &result) != 0)
        throw ConfigException("Hostname '" + host + "' does not resolve to an IP address");
    if (result != NULL)
    {
        inetAddressStr(result->ai_addr, result->ai_addrlen, ip, dummyPort);
        freeaddrinfo(result);
    }
    return ip;
}

pair<string, int> Config::parseListenArgument(const string &arg)
{
    string host;
    if (arg[0] == '[')
    {
        size_t closeBracket = arg.find(']');

        host = arg.substr(1, closeBracket - 1);

        if (closeBracket + 1 < arg.length())
        {
            int port = atoi(arg.substr(closeBracket + 2).c_str());
            return make_pair(host, port);
        }
        return make_pair(host, DEFAULT_HTTP_PORT);
    }
    size_t colonPos = arg.find(':');
    if (colonPos == string::npos)
    {
        for (size_t i = 0; i < arg.length(); i++)
        {
            if (!isdigit(arg[i]))
                break;
            if (i == arg.length() - 1)
                return make_pair("0.0.0.0", atoi(arg.c_str()));
        }
        if (!isIPv4(arg))
            return make_pair(getIpByHost(arg), DEFAULT_HTTP_PORT);
        return make_pair(arg, DEFAULT_HTTP_PORT);
    }
    host = arg.substr(0, colonPos);
    if (!isIPv4(host))
        host = getIpByHost(host);
    int port = atoi(arg.substr(colonPos + 1).c_str());
    return make_pair(host, port);
}

vector<pair<string, int> > Config::getAllListenInfo(const ConfigNode &serverNode)
{
    vector<pair<string, int>  > listenList;
    const vector<ConfigNode> &directives = serverNode.getChildren();

    for (size_t i = 0; i < directives.size(); ++i)
    {
        if (directives[i].getName() == "listen")
        {
            const vector<string> &args = directives[i].getArguments();

            if (!args.empty())
            {
                pair<string, int> listen = parseListenArgument(args[0]);
                if (find(listenList.begin(), listenList.end(), listen) != listenList.end())
                    throw ConfigException("Duplicate listen directive: " + args[0]);
                listenList.push_back(listen);
            }
        }
    }
    if (listenList.empty())
    {
        listenList.push_back(make_pair("0.0.0.0", DEFAULT_HTTP_PORT));
    }
    return listenList;
}

vector<string> Config::getServerNames(const ConfigNode &serverNode)
{
    vector<string> serverNames;
    const vector<ConfigNode> &directives = serverNode.getChildren();
    for (size_t i = 0; i < directives.size(); ++i)
    {
        if (directives[i].getName() == "server_name")
        {
            const vector<string> &args = directives[i].getArguments();
            serverNames.insert(serverNames.end(), args.begin(), args.end());
        }
    }
    return serverNames;
}

vector<ServerConfig>& Config::getServerConfigs()
{
    return ServerConfigs_;
}

void Config::setServerConfigs()
{
    map<string, ServerConfig> socketMap;
    vector<ConfigNode> children = ast_.getChildren();
    if (children.empty())
    {
        ServerConfig defaultConfig("0.0.0.0", DEFAULT_HTTP_PORT);
        ConfigNode emptyServer;
        emptyServer.setType(BLOCK);
        emptyServer.setName("server");
        defaultConfig.addServerNode(emptyServer);

        vector<ServerConfig> result;
        result.push_back(defaultConfig);

        ServerConfigs_ = result;
        return;
    }

    for (size_t i = 0; i < children.size(); ++i)
    {
        const ConfigNode &serverNode = children[i];
        vector<pair<string, int> > listens = getAllListenInfo(serverNode);
        for (size_t j = 0; j < listens.size(); ++j)
        {
            string host = listens[j].first;
            int port = listens[j].second;
            ostringstream oss;
            oss << host << ":" << port;
            string key = oss.str();
            if (socketMap.find(key) == socketMap.end())
                socketMap[key] = ServerConfig(host, port);
            socketMap[key].addServerNode(serverNode);
        }
    }
    vector<ServerConfig> result;
    for (map<string, ServerConfig>::iterator it = socketMap.begin();
         it != socketMap.end(); ++it)
    {
        result.push_back(it->second);
    }
    ServerConfigs_ = result;
}