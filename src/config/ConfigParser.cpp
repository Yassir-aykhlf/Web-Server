#include "ConfigParser.hpp"
#include "Logger.hpp"

ConfigParser::ConfigParser() : _pos(0), _line(1) {}

ConfigParser::~ConfigParser() {}

bool ConfigParser::parse(const std::string& filename, std::vector<ServerConfig>& servers) {
    std::ifstream file(filename.c_str());
    if (!file.is_open()) {
        setError("Cannot open configuration file: " + filename);
        return false;
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    _content = oss.str();
    file.close();
    _pos = 0;
    _line = 1;
    while (_pos < _content.length()) {
        skipWhitespace();
        if (_pos >= _content.length())
            break;
        
        std::string token = readToken();
        if (token.empty())
            break;
        
        if (token == "server") {
            ServerConfig server;
            if (!parseServerBlock(server))
                return false;
            servers.push_back(server);
        } else {
            setError("Expected 'server', got '" + token + "'");
            return false;
        }
    }
    if (servers.empty()) {
        setError("No server blocks found in configuration");
        return false;
    }
    return true;
}

const std::string& ConfigParser::getError() const {
    return _error;
}

void ConfigParser::skipWhitespace() {
    while (_pos < _content.length()) {
        char c = _content[_pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            _pos++;
        } else if (c == '\n') {
            _pos++;
            _line++;
        } else if (c == '#') {
            skipComment();
        } else {
            break;
        }
    }
}

void ConfigParser::skipComment() {
    while (_pos < _content.length() && _content[_pos] != '\n')
        _pos++;
}

std::string ConfigParser::readToken() {
    skipWhitespace();
    if (_pos >= _content.length())
        return "";
    if (_content[_pos] == '"' || _content[_pos] == '\'')
        return readQuotedString();
    if (_content[_pos] == '{' || _content[_pos] == '}' || _content[_pos] == ';') {
        return std::string(1, _content[_pos++]);
    }
    size_t start = _pos;
    while (_pos < _content.length()) {
        char c = _content[_pos];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' ||
            c == '{' || c == '}' || c == ';' || c == '#')
            break;
        _pos++;
    }
    return _content.substr(start, _pos - start);
}

std::string ConfigParser::readQuotedString() {
    char quote = _content[_pos++];
    size_t start = _pos;
    while (_pos < _content.length() && _content[_pos] != quote) {
        if (_content[_pos] == '\n')
            _line++;
        _pos++;
    }
    std::string result = _content.substr(start, _pos - start);
    if (_pos < _content.length())
        _pos++;
    return result;
}

bool ConfigParser::expect(const std::string& token) {
    std::string got = readToken();
    if (got != token) {
        setError("Expected '" + token + "', got '" + got + "'");
        return false;
    }
    return true;
}

bool ConfigParser::parseServerBlock(ServerConfig& server) {
    if (!expect("{"))
        return false;
    while (true) {
        skipWhitespace();
        if (_pos >= _content.length()) {
            setError("Unexpected end of file in server block");
            return false;
        }
        std::string token = readToken();
        if (token == "}")
            break;
        if (token == "listen") {
            std::string value = readToken();
            size_t colonPos = value.find(':');
            if (colonPos != std::string::npos) {
                server.setHost(value.substr(0, colonPos));
                server.setPort(stringToInt(value.substr(colonPos + 1)));
            } else {
                server.setPort(stringToInt(value));
            }
            if (!expect(";"))
                return false;
        }
        else if (token == "server_name") {
            while (true) {
                std::string name = readToken();
                if (name == ";")
                    break;
                server.addServerName(name);
            }
        }
        else if (token == "root") {
            server.setRoot(readToken());
            if (!expect(";"))
                return false;
        }
        else if (token == "index") {
            server.setIndex(readToken());
            if (!expect(";"))
                return false;
        }
        else if (token == "client_max_body_size") {
            server.setClientMaxBodySize(parseSize(readToken()));
            if (!expect(";"))
                return false;
        }
        else if (token == "error_page") {
            int code = stringToInt(readToken());
            std::string path = readToken();
            server.addErrorPage(code, path);
            if (!expect(";"))
                return false;
        }
        else if (token == "location") {
            LocationConfig location;
            location.setPath(readToken());
            if (!parseLocationBlock(location))
                return false;
            server.addLocation(location);
        }
        else {
            setError("Unknown directive in server block: " + token);
            return false;
        }
    }
    if (server.getLocations().empty()) {
        LocationConfig defaultLocation;
        defaultLocation.setPath("/");
        server.addLocation(defaultLocation);
    }
    return true;
}

bool ConfigParser::parseLocationBlock(LocationConfig& location) {
    if (!expect("{"))
        return false;
    bool methodsSet = false;
    while (true) {
        skipWhitespace();
        if (_pos >= _content.length()) {
            setError("Unexpected end of file in location block");
            return false;
        }
        std::string token = readToken();
        if (token == "}")
            break;
        if (token == "root") {
            location.setRoot(readToken());
            if (!expect(";"))
                return false;
        }
        else if (token == "alias") {
            location.setAlias(readToken());
            if (!expect(";"))
                return false;
        }
        else if (token == "index") {
            location.setIndex(readToken());
            if (!expect(";"))
                return false;
        }
        else if (token == "methods" || token == "allow_methods" || token == "limit_except") {
            if (!methodsSet) {
                std::string savedPath = location.getPath();
                location = LocationConfig();
                location.setPath(savedPath);
                methodsSet = true;
            }
            while (true) {
                std::string method = readToken();
                if (method == ";")
                    break;
                location.addMethod(method);
            }
        }
        else if (token == "autoindex") {
            std::string value = readToken();
            location.setAutoindex(value == "on" || value == "true" || value == "1");
            if (!expect(";"))
                return false;
        }
        else if (token == "upload_store" || token == "upload_path") {
            location.setUploadStore(readToken());
            if (!expect(";"))
                return false;
        }
        else if (token == "return") {
            int code = stringToInt(readToken());
            std::string url = readToken();
            location.setRedirect(code, url);
            if (!expect(";"))
                return false;
        }
        else if (token == "cgi_extension" || token == "cgi_pass") {
            std::string ext = readToken();
            std::string handler = readToken();
            location.addCgiExtension(ext, handler);
            if (!expect(";"))
                return false;
        }
        else if (token == "cgi") {
            std::string ext = readToken();
            std::string handler = readToken();
            location.addCgiExtension(ext, handler);
            if (!expect(";"))
                return false;
        }
        else {
            setError("Unknown directive in location block: " + token);
            return false;
        }
    }
    return true;
}

size_t ConfigParser::parseSize(const std::string& str) {
    size_t value = 0;
    size_t i = 0;
    while (i < str.length() && std::isdigit(str[i])) {
        value = value * 10 + (str[i] - '0');
        i++;
    }
    if (i < str.length()) {
        char suffix = std::toupper(str[i]);
        if (suffix == 'K')
            value *= 1024;
        else if (suffix == 'M')
            value *= 1024 * 1024;
        else if (suffix == 'G')
            value *= 1024 * 1024 * 1024;
    }
    return value;
}

void ConfigParser::setError(const std::string& msg) {
    std::ostringstream oss;
    oss << "Line " << _line << ": " << msg;
    _error = oss.str();
    Logger::error(_error);
}
