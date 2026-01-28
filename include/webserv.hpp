#ifndef WEBSERV_HPP
#define WEBSERV_HPP

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
class   ServerConfig;
class   LocationConfig;
class   EvenLoop;
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

enum HttpMethod {
    METHOD_GET,
    METHOD_POST,
    METHOD_DELETE,
    METHOD_UNKNOWN
};

std::string toLower(const std::string& str);
std::string intToString(int value);
int         stringToInt(const std::string& str);
std::string toUpper(const std::string& str);

#endif // WEBSERV_HPP