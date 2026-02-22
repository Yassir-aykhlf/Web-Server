#pragma once

#include "Config.hpp"
#include "EventLoop.hpp"
#include "Server.hpp"

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <algorithm>
#include <iostream>
#include <unistd.h>
#include <dirent.h>
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <csignal>
#include <netdb.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <poll.h>
#include <cerrno>
#include <ctime>
#include <map>
#include <set>
#include <new>

class   Server;
class   Client;
class   Config;
class   EventLoop;
class   HttpRequest;
class   HttpResponse;
class   HttpParser;
class   RequestHandler;
class   CgiHandler;

#define ONE_MB 1048576
#define BUFFER_SIZE 65536
#define MAX_CLIENTS 1024
#define DEFAULT_PORT 8080
#define DEFAULT_HOST "0.0.0.0"
#define DEFAULT_MAX_BODY_SIZE ONE_MB
#define DEFAULT_TIMEOUT 60
#define SERVER_NAME "webserv/1.0"
#define HTTP_VERSION "HTTP/1.1"

enum HttpStatus {
    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_NO_CONTENT = 204,
    STATUS_MOVED_PERMANENTLY = 301,
    STATUS_FOUND = 302,
    STATUS_SEE_OTHER = 303,
    STATUS_TEMPORARY_REDIRECT = 307,
    STATUS_PERMANENT_REDIRECT = 308,
    STATUS_BAD_REQUEST = 400,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_METHOD_NOT_ALLOWED = 405,
    STATUS_REQUEST_TIMEOUT = 408,
    STATUS_PAYLOAD_TOO_LARGE = 413,
    STATUS_URI_TOO_LONG = 414,
    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_NOT_IMPLEMENTED = 501,
    STATUS_BAD_GATEWAY = 502,
    STATUS_SERVICE_UNAVAILABLE = 503,
    STATUS_GATEWAY_TIMEOUT = 504
};

std::string toLower(const std::string& str);
std::string intToString(int value);
std::string longToString(long value);
int         stringToInt(const std::string& str);
std::string toUpper(const std::string& str);
std::vector<std::string> split(const std::string& str, char delimiter);
std::string urlDecode(const std::string& str);
std::string normalizePath(const std::string& path);
std::string trim(const std::string& str);
std::string getCurrentTime();
std::string getStatusText(int code);
std::string getFileExtension(const std::string& filename);
void non_blocking(int fd);

