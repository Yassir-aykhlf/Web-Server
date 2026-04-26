#include "MimeTypes.hpp"

std::map<std::string, std::string> MimeTypes::_types;
bool MimeTypes::_initialized = false;

MimeTypes::MimeTypes() {}

void MimeTypes::initialize() {
    if (_initialized)
        return;
        
    _types[".html"] = "text/html";
    _types[".htm"] = "text/html";
    _types[".css"] = "text/css";
    _types[".js"] = "text/javascript";
    _types[".txt"] = "text/plain";
    _types[".xml"] = "text/xml";
    _types[".json"] = "application/json";

    _types[".png"] = "image/png";
    _types[".jpg"] = "image/jpeg";
    _types[".jpeg"] = "image/jpeg";
    _types[".gif"] = "image/gif";
    _types[".ico"] = "image/x-icon";
    _types[".svg"] = "image/svg+xml";
    _types[".webp"] = "image/webp";
    _types[".bmp"] = "image/bmp";
    
    _types[".mp3"] = "audio/mpeg";
    _types[".mp4"] = "video/mp4";
    _types[".webm"] = "video/webm";
    _types[".ogg"] = "audio/ogg";
    _types[".wav"] = "audio/wav";
    _types[".avi"] = "video/x-msvideo";
    
    _types[".ttf"] = "font/ttf";
    _types[".otf"] = "font/otf";
    _types[".woff"] = "font/woff";
    _types[".woff2"] = "font/woff2";
    _types[".eot"] = "application/vnd.ms-fontobject";
    
    _types[".pdf"] = "application/pdf";
    _types[".doc"] = "application/msword";
    _types[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    _types[".xls"] = "application/vnd.ms-excel";
    _types[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    
    _types[".zip"] = "application/zip";
    _types[".tar"] = "application/x-tar";
    _types[".gz"] = "application/gzip";
    _types[".rar"] = "application/vnd.rar";
    _types[".7z"] = "application/x-7z-compressed";
    
    _types[".php"] = "application/x-httpd-php";
    _types[".py"] = "text/x-python";
    _types[".sh"] = "application/x-sh";
    
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
