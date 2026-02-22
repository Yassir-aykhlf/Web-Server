#include "webserv.hpp"

std::string toLower(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
        result[i] = std::tolower(static_cast<unsigned char>(result[i]));
    return result;
}

int stringToInt(const std::string& str) {
    std::istringstream iss(str);
    int value;
    iss >> value;
    return value;
}

std::string intToString(int value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i)
        result[i] = std::toupper(static_cast<unsigned char>(result[i]));
    return result;
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

/* e.g. Hello%21+You -> Hello! You */
std::string urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length() &&
            std::isxdigit(static_cast<unsigned char>(str[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(str[i + 2]))) {
                int value;
                std::istringstream iss(str.substr(i + 1, 2));
                if (iss >> std::hex >> value) {
                    result += static_cast<char>(value);
                    i += 2;
                }
        }
        else if (str[i] =='+') {
            result += ' ';
        }
        else {
            result += str[i];
        }
    }
    return result;
}

/* 
path normalizer, stack-based implementation.
LeetCode: 71. Simplify Path
*/
std::string normalizePath(const std::string& path) {
    if (path.empty()) return ".";
    std::vector<std::string> parts;
    std::istringstream iss(path);
    std::string part;
    bool isAbsolute = (path[0] == '/');
    bool startsWithDot = (path.length() >= 2 && path[0] == '.' && path[1] == '/');
    bool endsWithSlash = (path.length() > 1 && path[path.length() - 1] == '/');
    while (std::getline(iss, part, '/')) {
        if (part.empty() || part == ".") {
            continue;
        }
        if (part == "..") {
            if (!parts.empty() && parts.back() != "..")
                parts.pop_back();
            else if (!isAbsolute)
                parts.push_back("..");
        }
        else {
            parts.push_back(part);
        }
    }
    std::string result;
    if (isAbsolute) result = "/";
    else if (startsWithDot) result = "./";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0 || result.empty()) {
            result += (i > 0) ? "/" : "";
        }
        result += parts[i];
    }
    if (result.empty()) result = isAbsolute ? "/": ".";
    if (endsWithSlash && result.length() > 1 && result[result.length() - 1] != '/')
        result += "/";
    return result;
}

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

void non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    int result = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (result == -1) {
        std::cerr << "Error setting non-blocking mode for fd: " << fd << std::endl;
    }
}

std::string longToString(long value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

std::string getCurrentTime() {
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    char buf[128];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    return std::string(buf);
}

std::string getStatusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Timeout";
        default:  return "Unknown";
    }
}

std::string getFileExtension(const std::string& filename) {
    size_t pos = filename.rfind('.');
    if (pos == std::string::npos)
        return "";
    return filename.substr(pos);
}

