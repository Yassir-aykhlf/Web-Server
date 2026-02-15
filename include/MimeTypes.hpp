#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include "webserv.hpp"

class MimeTypes {
public:
    static std::string getType(const std::string& extension);
    static std::string getTypeByFilename(const std::string& filename);
    static void initialize();

private:
    MimeTypes();
    static std::map<std::string, std::string> _types;
    static bool _initialized;
};

#endif