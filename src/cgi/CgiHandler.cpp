#include "CgiHandler.hpp"
#include "Logger.hpp"
#include <climits>

std::string CgiHandler::findInterpreter(const std::string& scriptPath, const Location& location) {
    std::string cgiPath = location.getStringValue("cgi_path");
    if (!cgiPath.empty())
        return cgiPath;

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

std::vector<std::string> CgiHandler::buildEnv(const HttpRequest& request,
                                               const std::string& scriptPath) {
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
    const std::map<std::string, std::string>& headers = request.getHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it) {
        std::string name = "HTTP_" + toUpper(it->first);
        for (size_t i = 0; i < name.length(); i++) {
            if (name[i] == '-')
                name[i] = '_';
        }
        env.push_back(name + "=" + it->second);
    }
    return env;
}

void CgiHandler::parseCgiOutput(const std::string& output, HttpResponse& response) {
    size_t headerEnd = output.find("\r\n\r\n");
    std::string separator = "\r\n";
    if (headerEnd == std::string::npos) {
        headerEnd = output.find("\n\n");
        separator = "\n";
    }
    if (headerEnd == std::string::npos) {
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
    while (std::getline(iss, line)) {
        if (!line.empty() && line[line.length() - 1] == '\r')
            line = line.substr(0, line.length() - 1);
        if (line.empty())
            continue;
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos)
            continue;
        std::string name = trim(line.substr(0, colonPos));
        std::string value = trim(line.substr(colonPos + 1));
        if (toLower(name) == "status") {
            hasStatus = true;
            int code = stringToInt(value);
            if (code > 0)
                response.setStatus(code);
        } else if (toLower(name) == "content-type") {
            hasContentType = true;
            response.setContentType(value);
        } else if (toLower(name) == "location") {
            response.setLocation(value);
            if (!hasStatus)
                response.setStatus(STATUS_FOUND);
        } else {
            response.setHeader(name, value);
        }
    }
    if (!hasStatus)
        response.setStatus(STATUS_OK);
    if (!hasContentType)
        response.setContentType("text/html");
    response.setBody(body);
}

HttpResponse CgiHandler::execute(const HttpRequest& request, const Location& location,
                                  const std::string& scriptPath) {
    std::string interpreter = findInterpreter(scriptPath, location);
    if (interpreter.empty()) {
        Logger::error("No CGI interpreter found for: " + scriptPath);
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "No CGI interpreter configured");
    }
    char resolvedPath[PATH_MAX];
    std::string absScriptPath = scriptPath;
    if (realpath(scriptPath.c_str(), resolvedPath) != NULL) {
        absScriptPath = resolvedPath;
    }
    int pipeIn[2];
    int pipeOut[2];
    if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
        Logger::error("Failed to create pipes for CGI");
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR);
    }
    pid_t pid = fork();
    if (pid < 0) {
        Logger::error("Failed to fork for CGI");
        close(pipeIn[0]);
        close(pipeIn[1]);
        close(pipeOut[0]);
        close(pipeOut[1]);
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR);
    }
    if (pid == 0) {
        close(pipeIn[1]);
        close(pipeOut[0]);
        dup2(pipeIn[0], STDIN_FILENO);
        dup2(pipeOut[1], STDOUT_FILENO);
        close(pipeIn[0]);
        close(pipeOut[1]);
        std::string scriptDir = absScriptPath;
        size_t lastSlash = scriptDir.rfind('/');
        if (lastSlash != std::string::npos)
            scriptDir = scriptDir.substr(0, lastSlash);
        else
            scriptDir = ".";
        chdir(scriptDir.c_str());
        std::vector<std::string> envVec = buildEnv(request, absScriptPath);
        std::vector<char*> envp;
        for (size_t i = 0; i < envVec.size(); i++)
            envp.push_back(const_cast<char*>(envVec[i].c_str()));
        envp.push_back(NULL);
        char* argv[3];
        argv[0] = const_cast<char*>(interpreter.c_str());
        argv[1] = const_cast<char*>(absScriptPath.c_str());
        argv[2] = NULL;
        execve(interpreter.c_str(), argv, &envp[0]);
        _exit(1);
    }
    close(pipeIn[0]);
    close(pipeOut[1]);
    if (!request.getBody().empty()) {
        const std::string& body = request.getBody();
        size_t totalWritten = 0;
        while (totalWritten < body.length()) {
            ssize_t written = write(pipeIn[1], body.c_str() + totalWritten, body.length() - totalWritten);
            if (written <= 0) {
                Logger::error("Failed to write to CGI stdin");
                break;
            }
            totalWritten += written;
        }
    }
    close(pipeIn[1]);
    std::string cgiOutput;
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    std::string timeoutStr = location.getStringValue("cgi_timeout");
    int timeoutSec = 30;
    if (!timeoutStr.empty()) {
        std::string numStr = timeoutStr;
        if (!numStr.empty() && (numStr[numStr.length() - 1] == 's' || numStr[numStr.length() - 1] == 'S'))
            numStr = numStr.substr(0, numStr.length() - 1);
        timeoutSec = stringToInt(numStr);
        if (timeoutSec <= 0)
            timeoutSec = 30;
    }
    time_t startTime = time(NULL);
    struct pollfd cgiPfd;
    cgiPfd.fd = pipeOut[0];
    cgiPfd.events = POLLIN;
    while (true) {
        int remaining = timeoutSec - static_cast<int>(time(NULL) - startTime);
        if (remaining <= 0) {
            Logger::error("CGI timeout for: " + absScriptPath);
            kill(pid, SIGKILL);
            close(pipeOut[0]);
            waitpid(pid, NULL, 0);
            return HttpResponse::makeError(STATUS_GATEWAY_TIMEOUT, "CGI script timed out");
        }
        int pollResult = poll(&cgiPfd, 1, remaining * 1000);
        if (pollResult < 0) {
            break;
        }
        if (pollResult == 0) {
            Logger::error("CGI timeout for: " + absScriptPath);
            kill(pid, SIGKILL);
            close(pipeOut[0]);
            waitpid(pid, NULL, 0);
            return HttpResponse::makeError(STATUS_GATEWAY_TIMEOUT, "CGI script timed out");
        }
        bytesRead = read(pipeOut[0], buffer, sizeof(buffer) - 1);
        if (bytesRead <= 0)
            break;
        cgiOutput.append(buffer, bytesRead);
    }
    close(pipeOut[0]);
    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status) && WEXITSTATUS(status) != 0 && cgiOutput.empty()) {
        Logger::error("CGI script exited with error: " + scriptPath);
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "CGI script error");
    }
    HttpResponse response;
    parseCgiOutput(cgiOutput, response);
    return response;
}