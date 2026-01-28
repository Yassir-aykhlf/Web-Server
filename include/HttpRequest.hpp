#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include "webserv.hpp"

/*
parsed HTTP request.
*/
class HttpRequest {
private:
    HttpMethod  _method;
    std::string _methodStr;
    std::string _uri;
    std::string _path;
    std::string _query;
    std::string _body;
    std::string _version;
    std::map<std::string, std::string>  _headers;

public:
    HttpRequest();
    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);
    ~HttpRequest();

    HttpMethod  getMethod() const;
    const   std::string& getMethodString() const;
    const   std::string& getUri() const;
    const   std::string& getPath() const;
    const   std::string& getQueryString() const;
    const   std::string& getVersion() const;
    const   std::string& getBody() const;
    const   std::map<std::string, std::string>& getHeaders() const;

    void    setMethod(HttpMethod method);
    void    setMethodString(const std::string& method);
    void    setUri(const std::string& uri);
    void    setPath(const std::string& path);
    void    setQueryString(const std::string& query);
    void    setVersion(const std::string& version);
    void    setBody(const std::string& data);
    void    setHeader(const std::string& name, const std::string& value);
    void    appendBody(const std::string& data);
    bool    isChunked() const;
    bool    isKeepAlive() const;
    size_t  getContentLength() const;
    std::string getHeader(const std::string& name) const;
    const std::string&  getHost() const;
    
    void    clear();
};

#endif