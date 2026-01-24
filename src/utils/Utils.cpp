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