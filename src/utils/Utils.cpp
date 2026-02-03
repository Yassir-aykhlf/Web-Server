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
    return result;
}