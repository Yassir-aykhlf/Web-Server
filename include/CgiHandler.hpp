#pragma once

#include "webserv.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"

class CgiHandler {
public:
    static HttpResponse execute(const HttpRequest& request, const Location& location,
                                const std::string& scriptPath);

private:
    CgiHandler();
    static std::vector<std::string> buildEnv(const HttpRequest& request,
                                              const std::string& scriptPath);
    static std::string findInterpreter(const std::string& scriptPath, const Location& location);
    static void parseCgiOutput(const std::string& output, HttpResponse& response);
};

