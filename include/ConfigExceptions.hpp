#pragma once
#include <stdexcept>

class ConfigException : public std::runtime_error
{
public:
  explicit ConfigException(const std::string &msg) : std::runtime_error(msg) {}
};

class ParseException : public std::runtime_error
{
public:
  explicit ParseException(const std::string &msg) : std::runtime_error(msg) {}
};

class FileException : public std::runtime_error
{
public:
  explicit FileException(const std::string &msg) : std::runtime_error(msg) {}
};
