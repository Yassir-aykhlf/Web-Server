#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>

class   Server;
class   Client;
class   Config;
class   ServerConfig;
class   LocationConfig;
class   EvenLoop;
class   HttpRequest;
class   HttpResponse;
class   HttpParser;
class   RequestHandler;
class   CgiHandler;

enum HttpMethod {
    METHOD_GET,
    METHOD_POST,
    METHOD_DELETE,
    METHOD_UNKNOWN
};

std::string toLower(const std::string& str);
int         stringToInt(const std::string& str);

#endif // WEBSERV_HPP