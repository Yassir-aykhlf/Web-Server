#include "CgiHandler.hpp"
#include "Client.hpp"
#include "Logger.hpp"
#include <climits>
#include <sys/wait.h>

std::string CgiHandler::findInterpreter(const std::string &scriptPath, const Location &location)
{
    std::string cgiPath = location.getStringValue("cgi_path");
    if (!cgiPath.empty())
        return resolveAbsoluteScriptPath(cgiPath);
    std::string ext = getFileExtension(scriptPath);
    if (ext == ".py")
        return "/usr/bin/python3";
    if (ext == ".sh")
        return "/bin/sh";
    if (ext == ".php")
        return "/usr/bin/php-cgi";
    if (ext == ".pl")
        return "/usr/bin/perl";
    return "";
}

std::vector<std::string> CgiHandler::buildEnv(const HttpRequest &request,
                                              const std::string &scriptPath)
{
    std::vector<std::string> env;
    env.push_back("REQUEST_METHOD=" + request.getMethodString());
    env.push_back("QUERY_STRING=" + request.getQueryString());
    env.push_back("CONTENT_TYPE=" + request.getHeader("content-type"));
    env.push_back("CONTENT_LENGTH=" + longToString(static_cast<long>(request.getBody().length())));
    env.push_back("SCRIPT_FILENAME=" + scriptPath);
    env.push_back("SCRIPT_NAME=" + request.getPath());
    env.push_back("PATH_INFO=" + request.getPath());
    env.push_back("SERVER_PROTOCOL=" + request.getVersion());
    env.push_back("SERVER_SOFTWARE=" SERVER_NAME);
    env.push_back("SERVER_NAME=" + request.getHost());
    env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    env.push_back("REDIRECT_STATUS=200");
    const std::map<std::string, std::string> &headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
    {
        env.push_back(convertHeaderToCgiEnvName(it->first) + "=" + it->second);
    }
    return env;
}

std::string CgiHandler::findHeaderBodySeparator(const std::string &output, size_t &headerEnd)
{
    headerEnd = output.find("\r\n\r\n");
    if (headerEnd != std::string::npos)
        return "\r\n";
    headerEnd = output.find("\n\n");
    if (headerEnd != std::string::npos)
        return "\n";
    return "";
}

void CgiHandler::applyCgiHeader(const std::string &name, const std::string &value,
                                HttpResponse &response, bool &hasStatus, bool &hasContentType)
{
    std::string lowerName = toLower(name);
    if (lowerName == "status")
    {
        hasStatus = true;
        int code = stringToInt(value);
        if (code > 0)
            response.setStatus(code);
    }
    else if (lowerName == "content-type")
    {
        hasContentType = true;
        response.setContentType(value);
    }
    else if (lowerName == "location")
    {
        response.setLocation(value);
        if (!hasStatus)
            response.setStatus(STATUS_FOUND);
    }
    else
    {
        response.setHeader(name, value);
    }
}

void CgiHandler::parseCgiOutput(const std::string &output, HttpResponse &response)
{
    size_t headerEnd;
    std::string separator = findHeaderBodySeparator(output, headerEnd);
    if (separator.empty())
    {
        response.setStatus(STATUS_OK);
        response.setContentType("text/html");
        response.setBody(output);
        return;
    }
    std::string headerSection = output.substr(0, headerEnd);
    size_t bodyStart = headerEnd + (separator == "\r\n" ? 4 : 2);
    std::string body = (bodyStart < output.length()) ? output.substr(bodyStart) : "";
    std::istringstream iss(headerSection);
    std::string line;
    bool hasStatus = false;
    bool hasContentType = false;
    while (std::getline(iss, line))
    {
        if (!line.empty() && line[line.length() - 1] == '\r')
            line = line.substr(0, line.length() - 1);
        if (line.empty())
            continue;
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        applyCgiHeader(name, value, response, hasStatus, hasContentType);
    }
    if (!hasStatus)
        response.setStatus(STATUS_OK);
    if (!hasContentType)
        response.setContentType("text/html");
    response.setBody(body);
}

std::string CgiHandler::resolveAbsoluteScriptPath(const std::string &scriptPath)
{
    char resolvedPath[PATH_MAX];
    if (realpath(scriptPath.c_str(), resolvedPath) != NULL)
        return std::string(resolvedPath);
    return scriptPath;
}

bool CgiHandler::createCgiPipes(int pipeIn[2], int pipeOut[2], HttpResponse &errorResponse)
{
    if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0)
    {
        Logger::error("Failed to create pipes for CGI");
        errorResponse = HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR);
        return false;
    }
    return true;
}

std::string CgiHandler::extractDirectoryFromPath(const std::string &path)
{
    size_t lastSlash = path.rfind('/');
    if (lastSlash != std::string::npos)
        return path.substr(0, lastSlash);
    return ".";
}

void CgiHandler::setupChildProcess(int pipeIn[2], int pipeOut[2],
                                   const HttpRequest &request,
                                   const std::string &interpreter,
                                   const std::string &absScriptPath)
{
    close(pipeIn[1]);
    close(pipeOut[0]);
    dup2(pipeIn[0], STDIN_FILENO);
    dup2(pipeOut[1], STDOUT_FILENO);
    close(pipeIn[0]);
    close(pipeOut[1]);
    std::string scriptDir = extractDirectoryFromPath(absScriptPath);
    chdir(scriptDir.c_str());
    std::vector<std::string> envVec = buildEnv(request, absScriptPath);
    std::vector<char *> envp;
    for (size_t i = 0; i < envVec.size(); i++)
        envp.push_back(const_cast<char *>(envVec[i].c_str()));
    envp.push_back(NULL);
    char *argv[3];
    argv[0] = const_cast<char *>(interpreter.c_str());
    argv[1] = const_cast<char *>(absScriptPath.c_str());
    argv[2] = NULL;
    execve(interpreter.c_str(), argv, &envp[0]);
    _exit(1);
}

bool CgiHandler::setNonBlockingPipes(int pipeIn, int pipeOut)
{
    int inFlags = fcntl(pipeIn, F_GETFL, 0);
    int outFlags = fcntl(pipeOut, F_GETFL, 0);
    if (inFlags < 0 || outFlags < 0 ||
        fcntl(pipeIn, F_SETFL, inFlags | O_NONBLOCK) < 0 ||
        fcntl(pipeOut, F_SETFL, outFlags | O_NONBLOCK) < 0)
    {
        return false;
    }
    return true;
}

int CgiHandler::parseCgiTimeout(const Location &location)
{
    std::string timeoutStr = location.getStringValue("cgi_timeout");
    if (timeoutStr.empty())
        return DEFAULT_CGI_TIMEOUT_SECONDS;
    std::string numStr = timeoutStr;
    if (!numStr.empty() && (numStr[numStr.length() - 1] == 's' || numStr[numStr.length() - 1] == 'S'))
        numStr = numStr.substr(0, numStr.length() - 1);
    int timeout = stringToInt(numStr);
    if (timeout <= 0)
        return DEFAULT_CGI_TIMEOUT_SECONDS;
    return timeout;
}

void CgiHandler::populateCgiProcess(CgiProcess &cgi, pid_t pid, int pipeIn, int pipeOut,
                                    const HttpRequest &request, const Location &location)
{
    cgi.pid = pid;
    cgi.pipeIn = pipeIn;
    cgi.pipeOut = pipeOut;
    cgi.outputBuffer.clear();
    cgi.bodyToWrite = request.getBody();
    cgi.bytesWritten = 0;
    cgi.stdinDone = cgi.bodyToWrite.empty();
    cgi.startTime = time(NULL);
    cgi.location = location;
    cgi.timeoutSec = parseCgiTimeout(location);
    if (cgi.stdinDone)
    {
        close(cgi.pipeIn);
        cgi.pipeIn = -1;
    }
}

bool CgiHandler::startCgi(const HttpRequest &request, const Location &location,
                          const std::string &scriptPath, CgiProcess &cgi,
                          HttpResponse &errorResponse)
{
    std::string interpreter = findInterpreter(scriptPath, location);
    if (interpreter.empty())
    {
        Logger::error("No CGI interpreter found for: " + scriptPath);
        errorResponse = HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "No CGI interpreter configured");
        return false;
    }
    std::string absScriptPath = resolveAbsoluteScriptPath(scriptPath);
    int pipeIn[2];
    int pipeOut[2];
    if (!createCgiPipes(pipeIn, pipeOut, errorResponse))
        return false;
    pid_t pid = fork();
    if (pid < 0)
    {
        Logger::error("Failed to fork for CGI");
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        errorResponse = HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR);
        return false;
    }
    if (pid == 0)
    {
        setupChildProcess(pipeIn, pipeOut, request, interpreter, absScriptPath);
    }
    close(pipeIn[0]);
    close(pipeOut[1]);
    if (!setNonBlockingPipes(pipeIn[1], pipeOut[0]))
    {
        Logger::error("Failed to configure CGI pipes as non-blocking");
        close(pipeIn[1]);
        close(pipeOut[0]);
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        errorResponse = HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "CGI pipe setup failed");
        return false;
    }
    populateCgiProcess(cgi, pid, pipeIn[1], pipeOut[0], request, location);
    Logger::info("CGI process started (pid: " + intToString(pid) + ") for: " + absScriptPath);
    return true;
}