#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "webserv.hpp"
#include "Config.hpp"
#include "ServerConfig.hpp"
#include "LocationConfig.hpp"

class ConfigParser {
public:
    ConfigParser();
    ~ConfigParser();

    bool parse(const std::string& filename, std::vector<ServerConfig>& servers);
    const std::string& getError() const;

private:
    std::string _content;
    size_t _pos;
    int _line;
    std::string _error;

    void skipWhitespace();
    void skipComment();
    std::string readToken();
    std::string readQuotedString();
    bool expect(const std::string& token);
    bool parseServerBlock(ServerConfig& server);
    bool parseLocationBlock(LocationConfig& location);
    bool parseDirective(ServerConfig& server);
    bool parseLocationDirective(LocationConfig& location);
    size_t parseSize(const std::string& str);
    void setError(const std::string& msg);
};

#endif
