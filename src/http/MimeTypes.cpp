#include "MimeTypes.hpp"

std::map<std::string, std::string> MimeTypes::_types;
bool MimeTypes::_initialized = false;

MimeTypes::MimeTypes() {}

void MimeTypes::initialize() {
    if (_initialized)
        return;
    // Text
    _types[".html"] = "text/html";
    _types[".htm"] = "text/html";
    _types[".css"] = "text/css";
    _types[".js"] = "text/javascript";
    _types[".txt"] = "text/plain";
    _types[".xml"] = "text/xml";
    _types[".json"] = "application/json";
    // Images
    _types[".png"] = "image/png";
    _types[".jpg"] = "image/jpeg";
    _types[".jpeg"] = "image/jpeg";
    _types[".gif"] = "image/gif";
    _types[".ico"] = "image/x-icon";
    _types[".svg"] = "image/svg+xml";
    // Binary
    _types[".pdf"] = "application/pdf";
    _types[".zip"] = "application/zip";
    // Script
    _types[".py"] = "text/x-python";
    _types[".sh"] = "application/x-sh";
    _types[".php"] = "application/x-httpd-php";
    
    _initialized = true;
}

std::string MimeTypes::getType(const std::string& extension) {
    initialize();
    std::string ext = toLower(extension);
    if (ext.empty() || ext[0] != '.')
        ext = "." + ext;
    std::map<std::string, std::string>::const_iterator it = _types.find(ext);
    if (it != _types.end())
        return it->second;
    return "application/octet-stream";
}

std::string MimeTypes::getTypeByFilename(const std::string& filename) {
    std::string ext = getFileExtension(filename);
    return getType(ext);
}