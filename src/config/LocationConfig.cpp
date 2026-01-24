#include "LocationConfig.hpp"

LocationConfig::LocationConfig()
    : _path("/")
    , _root("")
    , _index("index.html")
    , _autoindex(false)
    , _uploadStore("")
    , _redirectUrl("")
    , _redirectCode(0)
    , _alias("") {
    _methods.insert("GET");
}

LocationConfig::LocationConfig(const LocationConfig& other) {
    *this = other;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        _path = other._path;
        _root = other._root;
        _index = other._index;
        _methods = other._methods;
        _autoindex = other._autoindex;
        _uploadStore = other._uploadStore;
        _redirectUrl = other._redirectUrl;
        _redirectCode = other._redirectCode;
        _cgiExtensions = other._cgiExtensions;
        _alias = other._alias;
    }
    return *this;
}

LocationConfig::~LocationConfig() {}

const std::string& LocationConfig::getPath() const { return _path; }
const std::string& LocationConfig::getRoot() const { return _root; }
const std::string& LocationConfig::getIndex() const { return _index; }
const std::set<std::string>& LocationConfig::getMethods() const { return _methods; }
bool LocationConfig::getAutoindex() const { return _autoindex; }
const std::string& LocationConfig::getUploadStore() const { return _uploadStore; }
const std::string& LocationConfig::getRedirectUrl() const { return _redirectUrl; }
int LocationConfig::getRedirectCode() const { return _redirectCode; }
const std::map<std::string, std::string>& LocationConfig::getCgiExtensions() const { return _cgiExtensions; }
const std::string& LocationConfig::getAlias() const { return _alias; }

void LocationConfig::setPath(const std::string& path) { _path = path; }
void LocationConfig::setRoot(const std::string& root) { _root = root; }
void LocationConfig::setIndex(const std::string& index) { _index = index; }

void LocationConfig::addMethod(const std::string& method) {
    _methods.insert(toUpper(method));
}

void LocationConfig::setAutoindex(bool autoindex) { _autoindex = autoindex; }
void LocationConfig::setUploadStore(const std::string& uploadStore) { _uploadStore = uploadStore; }

void LocationConfig::setRedirect(int code, const std::string& url) {
    _redirectCode = code;
    _redirectUrl = url;
}

void LocationConfig::addCgiExtension(const std::string& ext, const std::string& handler) {
    _cgiExtensions[ext] = handler;
}

void LocationConfig::setAlias(const std::string& alias) { _alias = alias; }

bool LocationConfig::isMethodAllowed(const std::string& method) const {
    return _methods.find(toUpper(method)) != _methods.end();
}

bool LocationConfig::hasCgiExtension(const std::string& ext) const {
    return _cgiExtensions.find(ext) != _cgiExtensions.end();
}

std::string LocationConfig::getCgiHandler(const std::string& ext) const {
    std::map<std::string, std::string>::const_iterator it = _cgiExtensions.find(ext);
    if (it != _cgiExtensions.end())
        return it->second;
    return "";
}

bool LocationConfig::hasRedirect() const {
    return _redirectCode != 0 && !_redirectUrl.empty();
}
