#include "RequestHandler.hpp"
#include "CgiHandler.hpp"
#include "Logger.hpp"

size_t RequestHandler::parseBodySize(const std::string& sizeStr) {
    if (sizeStr.empty())
        return DEFAULT_MAX_BODY_SIZE;
    size_t len = sizeStr.length();
    char suffix = sizeStr[len - 1];
    std::string numPart = sizeStr;
    size_t multiplier = 1;
    if (suffix == 'm' || suffix == 'M') {
        multiplier = 1048576;
        numPart = sizeStr.substr(0, len - 1);
    } else if (suffix == 'k' || suffix == 'K') {
        multiplier = 1024;
        numPart = sizeStr.substr(0, len - 1);
    } else if (suffix == 'g' || suffix == 'G') {
        multiplier = 1073741824;
        numPart = sizeStr.substr(0, len - 1);
    }
    return static_cast<size_t>(stringToInt(numPart)) * multiplier;
}

bool RequestHandler::isMethodAllowed(const HttpRequest& request, const Location& location) {
    std::vector<std::string> methods = location.getListValue("method");
    if (methods.empty())
        return true;
    std::string reqMethod = request.getMethodString();
    for (size_t i = 0; i < methods.size(); i++) {
        if (toUpper(methods[i]) == reqMethod)
            return true;
    }
    return false;
}

std::string RequestHandler::resolveFilePath(const HttpRequest& request, const Location& location) {
    std::string root = location.getStringValue("root");
    std::string path = request.getPath();
    if (root.empty())
        root = "/var/www/html";
    // Remove trailing slash from root if present
    if (root.length() > 1 && root[root.length() - 1] == '/')
        root = root.substr(0, root.length() - 1);
    return root + path;
}

bool RequestHandler::isDirectory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0)
        return S_ISDIR(st.st_mode);
    return false;
}

bool RequestHandler::fileExists(const std::string& path) {
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

bool RequestHandler::isCgiRequest(const std::string& path, const Location& location) {
    std::vector<std::string> cgiExts = location.getListValue("cgi_ext");
    if (cgiExts.empty())
        return false;
    std::string ext = getFileExtension(path);
    for (size_t i = 0; i < cgiExts.size(); i++) {
        if (cgiExts[i] == ext)
            return true;
    }
    return false;
}

std::string RequestHandler::readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

HttpResponse RequestHandler::applyCustomErrorPage(const HttpResponse& errorResponse,
                                                   const Location& location) {
    int statusCode = errorResponse.getStatus();
    std::string errorPagePath = location.findErrorPagePath(statusCode);
    if (errorPagePath.empty())
        return errorResponse;

    // Resolve the error page path using root
    std::string root = location.getStringValue("root");
    if (root.empty())
        root = "/var/www/html";
    if (root.length() > 1 && root[root.length() - 1] == '/')
        root = root.substr(0, root.length() - 1);

    std::string errorFilePath = root + errorPagePath;
    if (fileExists(errorFilePath)) {
        std::string content = readFile(errorFilePath);
        HttpResponse customResp;
        customResp.setStatus(statusCode);
        customResp.setContentType("text/html");
        customResp.setBody(content);
        return customResp;
    }

    return errorResponse;
}

HttpResponse RequestHandler::handleRequest(const HttpRequest& request, const Location& location) {
    std::pair<int, std::string> returnDirective = location.getPairVal("return");
    if (returnDirective.first != 0) {
        if (returnDirective.first >= 300 && returnDirective.first < 400 && !returnDirective.second.empty())
            return HttpResponse::makeRedirect(returnDirective.first, returnDirective.second);
        return HttpResponse::makeError(returnDirective.first, returnDirective.second);
    }
    if (!isMethodAllowed(request, location)) {
        Logger::warning("Method not allowed: " + request.getMethodString());
        return applyCustomErrorPage(HttpResponse::makeError(STATUS_METHOD_NOT_ALLOWED), location);
    }
    std::string maxBodyStr = location.getStringValue("client_max_body_size");
    size_t maxBody = parseBodySize(maxBodyStr);
    if (request.getBody().length() > maxBody) {
        return applyCustomErrorPage(HttpResponse::makeError(STATUS_PAYLOAD_TOO_LARGE), location);
    }
    HttpResponse response;
    switch (request.getMethod()) {
        case METHOD_GET:
            response = handleGet(request, location);
            break;
        case METHOD_POST:
            response = handlePost(request, location);
            break;
        case METHOD_DELETE:
            response = handleDelete(request, location);
            break;
        default:
            response = HttpResponse::makeError(STATUS_NOT_IMPLEMENTED);
            break;
    }
    if (response.getStatus() >= 400)
        return applyCustomErrorPage(response, location);
    return response;
}

HttpResponse RequestHandler::serveFile(const std::string& filePath) {
    std::string content = readFile(filePath);
    if (content.empty() && !fileExists(filePath))
        return HttpResponse::makeError(STATUS_NOT_FOUND);

    HttpResponse response;
    response.setStatus(STATUS_OK);
    response.setContentType(MimeTypes::getTypeByFilename(filePath));
    response.setBody(content);
    return response;
}

HttpResponse RequestHandler::serveDirectory(const std::string& dirPath, const std::string& uriPath,
                                            const Location& location) {
    std::vector<std::string> indexFiles = location.getListValue("index");
    for (size_t i = 0; i < indexFiles.size(); i++) {
        std::string indexPath = dirPath;
        if (indexPath[indexPath.length() - 1] != '/')
            indexPath += "/";
        indexPath += indexFiles[i];
        if (fileExists(indexPath))
            return serveFile(indexPath);
    }
    bool autoindex = location.getBoolValue("autoindex");
    if (autoindex)
        return generateDirectoryListing(dirPath, uriPath);

    return HttpResponse::makeError(STATUS_FORBIDDEN);
}

HttpResponse RequestHandler::generateDirectoryListing(const std::string& dirPath,
                                                       const std::string& uriPath) {
    DIR* dir = opendir(dirPath.c_str());
    if (!dir)
        return HttpResponse::makeError(STATUS_FORBIDDEN);
    std::string body = "<!DOCTYPE html>\n<html>\n<head><title>Index of "
                       + uriPath + "</title></head>\n<body>\n<h1>Index of "
                       + uriPath + "</h1><hr><pre>\n";
    if (uriPath != "/")
        body += "<a href=\"../\">../</a>\n";
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;
        if (name == "." || name == "..")
            continue;

        std::string fullPath = dirPath;
        if (fullPath[fullPath.length() - 1] != '/')
            fullPath += "/";
        fullPath += name;

        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode))
                name += "/";
            body += "<a href=\"" + name + "\">" + name + "</a>\n";
        }
    }
    closedir(dir);
    body += "</pre><hr></body>\n</html>\n";
    HttpResponse response;
    response.setStatus(STATUS_OK);
    response.setContentType("text/html");
    response.setBody(body);
    return response;
}

HttpResponse RequestHandler::handleGet(const HttpRequest& request, const Location& location) {
    std::string filePath = resolveFilePath(request, location);
    Logger::info("Handling GET request for path: " + filePath);
    if (isCgiRequest(filePath, location)) {
        if (!fileExists(filePath))
            return HttpResponse::makeError(STATUS_NOT_FOUND);
        return CgiHandler::execute(request, location, filePath);
    }
    std::string uploadStore = location.getStringValue("upload_store");
    if (!fileExists(filePath) && !uploadStore.empty() && isDirectory(uploadStore)) {
        if (isDirectory(uploadStore))
            return serveDirectory(uploadStore, request.getPath(), location);
    }
    if (isDirectory(filePath)) {
        std::string reqPath = request.getPath();
        if (reqPath[reqPath.length() - 1] != '/') {
            return HttpResponse::makeRedirect(STATUS_MOVED_PERMANENTLY, reqPath + "/");
        }
        return serveDirectory(filePath, request.getPath(), location);
    }
    if (!fileExists(filePath))
        return HttpResponse::makeError(STATUS_NOT_FOUND);

    return serveFile(filePath);
}

HttpResponse RequestHandler::handlePost(const HttpRequest& request, const Location& location) {
    std::string filePath = resolveFilePath(request, location);
    if (isCgiRequest(filePath, location)) {
        if (!fileExists(filePath))
            return HttpResponse::makeError(STATUS_NOT_FOUND);
        return CgiHandler::execute(request, location, filePath);
    }
    std::string uploadStore = location.getStringValue("upload_store");
    if (uploadStore.empty()) {
        return HttpResponse::makeError(STATUS_FORBIDDEN, "Upload not configured for this location");
    }
    if (!isDirectory(uploadStore)) {
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "Upload directory does not exist");
    }
    std::string filename;
    std::string path = request.getPath();
    size_t lastSlash = path.rfind('/');
    if (lastSlash != std::string::npos && lastSlash + 1 < path.length())
        filename = path.substr(lastSlash + 1);
    else
        filename = "upload_" + longToString(static_cast<long>(time(NULL)));

    std::string uploadPath = uploadStore;
    if (uploadPath[uploadPath.length() - 1] != '/')
        uploadPath += "/";
    uploadPath += filename;

    std::ofstream outFile(uploadPath.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        Logger::error("Failed to create upload file: " + uploadPath);
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "Failed to save uploaded file");
    }
    outFile.write(request.getBody().c_str(), request.getBody().length());
    outFile.close();

    Logger::info("File uploaded: " + uploadPath);

    HttpResponse response;
    response.setStatus(STATUS_CREATED);
    response.setContentType("text/html");
    std::string body = "<!DOCTYPE html>\n<html>\n<body>\n<h1>201 Created</h1>\n"
                       "<p>File uploaded successfully: " + filename + "</p>\n"
                       "</body>\n</html>\n";
    response.setBody(body);
    return response;
}

HttpResponse RequestHandler::handleDelete(const HttpRequest& request, const Location& location) {
    std::string filePath = resolveFilePath(request, location);
    if (!fileExists(filePath)) {
        std::string uploadStore = location.getStringValue("upload_store");
        if (!uploadStore.empty()) {
            std::string path = request.getPath();
            size_t lastSlash = path.rfind('/');
            if (lastSlash != std::string::npos && lastSlash + 1 < path.length()) {
                std::string filename = path.substr(lastSlash + 1);
                std::string uploadPath = uploadStore;
                if (uploadPath[uploadPath.length() - 1] != '/')
                    uploadPath += "/";
                uploadPath += filename;
                if (fileExists(uploadPath))
                    filePath = uploadPath;
            }
        }
    }

    if (!fileExists(filePath))
        return HttpResponse::makeError(STATUS_NOT_FOUND);

    if (isDirectory(filePath))
        return HttpResponse::makeError(STATUS_FORBIDDEN, "Cannot delete a directory");

    if (remove(filePath.c_str()) != 0) {
        Logger::error("Failed to delete file: " + filePath);
        return HttpResponse::makeError(STATUS_INTERNAL_SERVER_ERROR, "Failed to delete file");
    }

    Logger::info("File deleted: " + filePath);

    HttpResponse response;
    response.setStatus(STATUS_OK);
    response.setContentType("text/html");
    std::string body = "<!DOCTYPE html>\n<html>\n<body>\n<h1>200 OK</h1>\n"
                       "<p>File deleted successfully.</p>\n"
                       "</body>\n</html>\n";
    response.setBody(body);
    return response;
}