#include "Config.hpp"

#include <sstream>

Config::Config(string filename) : filename_(filename) {};

// Debugging
// ─── ANSI Colors ──────────────────────────────────
#define COLOR_RESET "\033[0m"
#define COLOR_CYAN "\033[36m"   // Simple directives (listen, root, method...)
#define COLOR_RED "\033[31m"    // Block directives (http, server, location)
#define COLOR_GREEN "\033[32m"  // Values / parameters
#define COLOR_GRAY "\033[90m"   // Braces { } and semicolons ;
#define COLOR_YELLOW "\033[33m" // Location path (first arg of location)

// ───────────────────────────────────────────────────

void Config::printAST(const ConfigNode &node, int indent) const
{
    string indentation(indent * 2, ' ');

    bool isBlock = (node.getType() == ROOT || node.getType() == BLOCK);

    // Print the directive name
    cout << indentation << (isBlock ? COLOR_RED : COLOR_CYAN)
         << node.getName() << COLOR_RESET;

    // Print arguments on the same line
    if (!node.getArguments().empty())
    {
        cout << " ";
        for (size_t i = 0; i < node.getArguments().size(); ++i)
        {
            // Location path gets yellow, everything else green
            if (node.getName() == "location" && i == 0)
                cout << COLOR_YELLOW;
            else
                cout << COLOR_GREEN;

            cout << node.getArguments()[i] << COLOR_RESET;

            if (i < node.getArguments().size() - 1)
                cout << " ";
        }
    }

    // Block directive → open brace and recurse
    if (!node.getChildren().empty())
    {
        cout << " " << COLOR_GRAY << "{" << COLOR_RESET << endl;

        vector<ConfigNode> children = node.getChildren();
        for (size_t i = 0; i < children.size(); ++i)
            printAST(children[i], indent + 1);

        cout << indentation << COLOR_GRAY << "}" << COLOR_RESET
             << endl;
    }
    else
    {

        if (isBlock)
            cout << " " << COLOR_GRAY << "{" << COLOR_RESET << " "
                 << COLOR_GRAY << "}" << COLOR_RESET << endl;
        else
            cout << COLOR_GRAY << ";" << COLOR_RESET << endl;
    }
}

// Overload for easy usage
void Config::printAST(void) const
{
    printAST(ast_, 0);
}

void Config::load()
{
    try
    {
        ConfigParser parser;
        ast_ = parser.parse(filename_);

        ConfigValidator validator;
        validator.validate(ast_);
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

    // Split by '.'
    while (getline(ss, octet, '.'))
    {
        octets.push_back(octet);
    }

    // Must have exactly 4 octets
    if (octets.size() != 4)
        return false;

    // Validate each octet
    for (size_t i = 0; i < octets.size(); i++)
    {
        if (octets[i].empty())
            return false;

        // Check all characters are digits
        for (size_t j = 0; j < octets[i].length(); j++)
        {
            if (!isdigit(octets[i][j]))
                return false;
        }

        // Convert to integer and check range
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
    int errorNumber = 0;

    if (getnameinfo(addr, addrlen, _host, NI_MAXHOST, service,
                    NI_MAXSERV, NI_NUMERICHOST) == 0)
    {
        host = _host;
        port = service;
    }
    else
        errorNumber = 1;
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
        return make_pair(host, 80);
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
            return make_pair(getIpByHost(arg), 80);
        return make_pair(arg, 80);
    }
    host = arg.substr(0, colonPos);
    if (!isIPv4(host))
        host = getIpByHost(host);
    int port = atoi(arg.substr(colonPos + 1).c_str());
    return make_pair(host, port);
}

vector<pair<string, int> > Config::getAllListenInfo(const ConfigNode &serverNode)
{
    vector<pair<string, int> > listenList;
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
        listenList.push_back(make_pair("0.0.0.0", 80));
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

vector<ServerConfigue> Config::getServerConfigues()
{
    map<string, ServerConfigue> socketMap;

    vector<ConfigNode> children = ast_.getChildren();

    if (children.empty())
    {
        ServerConfigue defaultConfig("0.0.0.0", 80);
        ConfigNode emptyServer;
        emptyServer.setType(BLOCK);
        emptyServer.setName("server");
        defaultConfig.addServerNode(emptyServer);

        vector<ServerConfigue> result;
        result.push_back(defaultConfig);
        return result;
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
                socketMap[key] = ServerConfigue(host, port);
            socketMap[key].addServerNode(serverNode);
        }
    }

    // Converting the map to vector
    vector<ServerConfigue> result;
    for (map<string, ServerConfigue>::iterator it = socketMap.begin();
         it != socketMap.end(); ++it)
    {
        result.push_back(it->second);
    }

    return result;
}