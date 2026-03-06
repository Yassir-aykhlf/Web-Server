#pragma once

#include "webserv.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"

struct CgiProcess;

class CgiHandler {
public:
    // Start a CGI process asynchronously. Populates the CgiProcess struct
    // with pipe fds, pid, timeout, etc. Returns true on success, false on error.
    // On failure, errorResponse is populated with an appropriate error.
    static bool startCgi(const HttpRequest& request, const Location& location,
                         const std::string& scriptPath, CgiProcess& cgi,
                         HttpResponse& errorResponse);

    // Parse completed CGI output into an HttpResponse
    static void parseCgiOutput(const std::string& output, HttpResponse& response);

private:
    CgiHandler();
    static std::vector<std::string> buildEnv(const HttpRequest& request,
                                              const std::string& scriptPath);
    static std::string findInterpreter(const std::string& scriptPath, const Location& location);
};

