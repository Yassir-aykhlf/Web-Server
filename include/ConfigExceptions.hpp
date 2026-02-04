#pragma once
#include <stdexcept>

using namespace std;

class ConfigException : public runtime_error
{
public:
  explicit ConfigException(const string &msg) : runtime_error(msg) {}
};

class ParseException : public runtime_error
{
public:
  explicit ParseException(const string &msg) : runtime_error(msg) {}
};

class FileException : public runtime_error
{
public:
  explicit FileException(const string &msg) : runtime_error(msg) {}
};
