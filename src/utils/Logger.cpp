#include "Logger.hpp"
#include <iostream>

Logger::Level Logger::_level = Logger::INFO;

void Logger::setLevel(Level level) {
    _level = level;
}

void Logger::debug(const std::string& message) {
    if (_level <= DEBUG) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

void Logger::info(const std::string& message) {
    if (_level <= INFO) {
        std::cout << "[INFO] " << message << std::endl;
    }
}

void Logger::warning(const std::string& message) {
    if (_level <= WARNING) {
        std::cerr << "[WARNING] " << message << std::endl;
    }
}

void Logger::error(const std::string& message) {
    if (_level <= ERROR) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}