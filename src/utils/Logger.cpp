#include "Logger.hpp"
#include <iostream>

// Use a static member variable for the log level
// Default log level
Logger::Level currentLevel = Logger::INFO; // Assuming a global or static member approach, but Logger has static methods.

void Logger::setLevel(Level level) {
    currentLevel = level;
}

void Logger::debug(const std::string& message) {
    if (currentLevel <= DEBUG) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

void Logger::info(const std::string& message) {
    if (currentLevel <= INFO) {
        std::cout << "[INFO] " << message << std::endl;
    }
}

void Logger::warning(const std::string& message) {
    if (currentLevel <= WARNING) {
        std::cerr << "[WARNING] " << message << std::endl;
    }
}

void Logger::error(const std::string& message) {
    if (currentLevel <= ERROR) {
        std::cerr << "[ERROR] " << message << std::endl;
    }
}

// void Logger::warning(const std::string& message) {
//     if (_level <= WARNING) {
//         log(WARNING, message);
//     }
// }


// void Logger::error(const std::string& message) {
//     if (_level <= ERROR) {
//         log(ERROR, message);
//     }
// }

// void Logger::log(Level level, const std::string& message) {
//     time_t now = time(NULL);
//     struct tm *tm_info = localtime(&now);
//     char timeBuffer[32];
//     strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", tm_info);
//     std::ostream& out = (level >= WARNING) ? std::cerr : std::cout;

//     out << "[" << timeBuffer << "] [" << getLevelString(level) << "] " << message << std::endl;
// }

// std::string Logger::getLevelString(Level level) {
//     switch (level) {
//         case DEBUG:     return "DEBUG";
//         case INFO:      return "INFO";
//         case WARNING:   return "WARN";
//         case ERROR:     return "ERROR";
//         default:        return "UNKNOWN";
//     }
// }