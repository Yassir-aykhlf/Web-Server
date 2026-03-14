#pragma once

#include "webserv.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"

struct CgiProcess;

class CgiHandler {
public:
    static bool startCgi(const HttpRequest& request, const Location& location,
                         const std::string& scriptPath, CgiProcess& cgi,
                         HttpResponse& errorResponse);
    static void parseCgiOutput(const std::string& output, HttpResponse& response);

private:
    CgiHandler();
    static std::vector<std::string> buildEnv(const HttpRequest& request,
                                              const std::string& scriptPath);
    static std::string findInterpreter(const std::string& scriptPath, const Location& location);

    // CGI output parsing helpers
    static std::string findHeaderBodySeparator(const std::string& output, size_t& headerEnd);
    static void applyCgiHeader(const std::string& name, const std::string& value,
                                HttpResponse& response, bool& hasStatus, bool& hasContentType);

    // CGI process startup helpers
    static std::string resolveAbsoluteScriptPath(const std::string& scriptPath);
    static bool createCgiPipes(int pipeIn[2], int pipeOut[2], HttpResponse& errorResponse);
    static std::string extractDirectoryFromPath(const std::string& path);
    static void setupChildProcess(int pipeIn[2], int pipeOut[2],
                                   const HttpRequest& request,
                                   const std::string& interpreter,
                                   const std::string& absScriptPath);
    static bool setNonBlockingPipes(int pipeIn, int pipeOut);
    static int parseCgiTimeout(const Location& location);
    static void populateCgiProcess(CgiProcess& cgi, pid_t pid, int pipeIn, int pipeOut,
                                    const HttpRequest& request, const Location& location);
};

