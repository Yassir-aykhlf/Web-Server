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

#define BUFFER_SIZE 65536
#define DEFAULT_MAX_BODY_SIZE 1048576
#define SERVER_NAME "webserv/1.0"
#define HTTP_VERSION "HTTP/1.1"

#define MAX_URI_LENGTH 8192
#define BYTES_PER_KILOBYTE 1024
#define BYTES_PER_MEGABYTE 1048576
#define BYTES_PER_GIGABYTE 1073741824
#define DEFAULT_CGI_TIMEOUT_SECONDS 30
#define DEFAULT_HTTP_PORT 80
#define DEFAULT_ROOT_PATH "/var/www/html"
#define POLL_TIMEOUT_MS 200
#define MAX_KEEP_ALIVE_CLIENTS 10000

enum HttpStatus
{
    STATUS_OK = 200,
    STATUS_CREATED = 201,
    STATUS_NO_CONTENT = 204,
    STATUS_MOVED_PERMANENTLY = 301,
    STATUS_FOUND = 302,
    STATUS_SEE_OTHER = 303,
    STATUS_TEMPORARY_REDIRECT = 307,
    STATUS_BAD_REQUEST = 400,
    STATUS_FORBIDDEN = 403,
    STATUS_NOT_FOUND = 404,
    STATUS_METHOD_NOT_ALLOWED = 405,
    STATUS_PAYLOAD_TOO_LARGE = 413,
    STATUS_URI_TOO_LONG = 414,
    STATUS_INTERNAL_SERVER_ERROR = 500,
    STATUS_NOT_IMPLEMENTED = 501,
    STATUS_BAD_GATEWAY = 502,
    STATUS_GATEWAY_TIMEOUT = 504,
    STATUS_HTTP_VERSION_NOT_SUPPORTED = 505
};

std::string toLower(const std::string &str);
std::string intToString(int value);
std::string longToString(long value);
int stringToInt(const std::string &str);
std::string toUpper(const std::string &str);
std::vector<std::string> split(const std::string &str, char delimiter);
std::string urlDecode(const std::string &str);
std::string normalizePath(const std::string &path);
std::string trim(const std::string &str);
std::string getCurrentTime();
std::string getStatusText(int code);
std::string getFileExtension(const std::string &filename);
void non_blocking(int fd);
bool isRedirectStatusCode(int code);
bool hasTrailingSlash(const std::string &str);
std::string ensureTrailingSlash(const std::string &str);
std::string stripTrailingSlash(const std::string &str);
std::string extractFilenameFromPath(const std::string &path);
std::string resolveRootPath(const Location &location);
std::string convertHeaderToCgiEnvName(const std::string &headerName);
