#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "webserv.hpp"

class Logger
{
public:
    enum Level
    {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };
    static void setLevel(Level level);
    static void debug(const std::string &message);
    static void info(const std::string &message);
    static void warning(const std::string &message);
    static void error(const std::string &message);

private:
    Logger();
    static Level _level;
    static void log(Level level, const std::string &message);
    static std::string getLevelString(Level level);
};

#endif
