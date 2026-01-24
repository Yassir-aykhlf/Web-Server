#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include "webserv.hpp"

class LocationConfig {
public:
    LocationConfig();
    LocationConfig(const LocationConfig& other);
    LocationConfig& operator=(const LocationConfig& other);
    ~LocationConfig();

    const std::string& getPath() const;
    const std::string& getRoot() const;
    const std::string& getIndex() const;
    const std::set<std::string>& getMethods() const;
    bool getAutoindex() const;
    const std::string& getUploadStore() const;
    const std::string& getRedirectUrl() const;
    int getRedirectCode() const;
    const std::map<std::string, std::string>& getCgiExtensions() const;
    const std::string& getAlias() const;

    void setPath(const std::string& path);
    void setRoot(const std::string& root);
    void setIndex(const std::string& index);
    void addMethod(const std::string& method);
    void setAutoindex(bool autoindex);
    void setUploadStore(const std::string& uploadStore);
    void setRedirect(int code, const std::string& url);
    void addCgiExtension(const std::string& ext, const std::string& handler);
    void setAlias(const std::string& alias);

    bool isMethodAllowed(const std::string& method) const;
    bool hasCgiExtension(const std::string& ext) const;
    std::string getCgiHandler(const std::string& ext) const;
    bool hasRedirect() const;

private:
    std::string _path;
    std::string _root;
    std::string _index;
    std::set<std::string> _methods;
    bool _autoindex;
    std::string _uploadStore;
    std::string _redirectUrl;
    int _redirectCode;
    std::map<std::string, std::string> _cgiExtensions;
    std::string _alias;
};

#endif
