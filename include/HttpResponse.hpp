#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include "webserv.hpp"

class HttpResponse {
public:
    HttpResponse();
    HttpResponse(const HttpResponse& other);
    HttpResponse& operator=(const HttpResponse& other);
    ~HttpResponse();

    void setStatus(int code);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void appendBody(const std::string& data);

    int getStatus() const;
    const std::string& getBody() const;
    std::string getHeader(const std::string& name) const;

    std::string build() const;

    void setContentType(const std::string& type);
    void setContentLength(size_t length);
    void setLocation(const std::string& url);
    void setConnection(bool keepAlive);

    void clear();

    static HttpResponse makeError(int code, const std::string& message = "");
    static HttpResponse makeRedirect(int code, const std::string& url);

private:
    int _statusCode;
    std::map<std::string, std::string> _headers;
    std::string _body;
};

#endif