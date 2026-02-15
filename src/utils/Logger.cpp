#include "Logger.hpp"

Logger::Level Logger::_level = Logger::INFO;

Logger::Logger() {}

void Logger::setLevel(Level level) {
    _level = level;
}

void Logger::debug(const std::string& message) {
    if (_level <= DEBUG) {
        log(DEBUG, message);
    }
}

void Logger::info(const std::string& message) {
    if (_level <= INFO) {
        log(INFO, message);
    }
}

void Logger::warning(const std::string& message) {
    if (_level <= WARNING) {
        log(WARNING, message);
    }
}


void Logger::error(const std::string& message) {
    if (_level <= ERROR) {
        log(ERROR, message);
    }
}

void Logger::log(Level level, const std::string& message) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timeBuffer[32];
    strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tm_info);
    std::ostream& out = (level >= WARNING) ? std::cerr : std::cout;

    out << "[" << timeBuffer << "] [" << getLevelString(level) << "] " << message << std::endl;
}

std::string Logger::getLevelString(Level level) {
    switch (level) {
        case DEBUG:     return "DEBUG";
        case INFO:      return "INFO";
        case WARNING:   return "WARN";
        case ERROR:     return "ERROR";
        default:        return "UNKNOWN";
    }
}