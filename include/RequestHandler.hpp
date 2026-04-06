#pragma once

#include "webserv.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Location.hpp"
#include "MimeTypes.hpp"

class RequestHandler {
public:
    static HttpResponse handleRequest(const HttpRequest& request, const Location& location);
    static HttpResponse applyCustomErrorPage(const HttpResponse& errorResponse, const Location& location);

    static std::string resolveFilePath(const HttpRequest& request, const Location& location);
    static bool isCgiRequest(const std::string& path, const Location& location);
    static bool fileExists(const std::string& path);
    static bool isMethodAllowed(const HttpRequest& request, const Location& location);
    static size_t parseBodySize(const std::string& sizeStr);
    static bool isBodyTooLarge(const HttpRequest& request, const Location& location);

private:
    RequestHandler();

    static bool hasReturnDirective(const Location& location);
    static HttpResponse handleReturnDirective(const std::pair<int, std::string>& returnDirective);

    static HttpResponse handleGet(const HttpRequest& request, const Location& location);
    static HttpResponse handlePost(const HttpRequest& request, const Location& location);
    static HttpResponse handleDelete(const HttpRequest& request, const Location& location);

    static HttpResponse serveFile(const std::string& filePath);
    static HttpResponse serveDirectory(const std::string& dirPath, const std::string& uriPath,
                                       const Location& location);
    static HttpResponse generateDirectoryListing(const std::string& dirPath,
                                                  const std::string& uriPath);
    static std::string buildDirectoryListingHeader(const std::string& uriPath);
    static std::string readDirectoryEntries(DIR* dir, const std::string& dirPath,
                                             const std::string& uriPath);
    static HttpResponse tryServeFromUploadStore(const HttpRequest& request, const Location& location);

    static bool isDirectory(const std::string& path);
    static std::string readFile(const std::string& path);

    static std::string generateUploadFilename(const HttpRequest& request);
    static HttpResponse writeUploadFile(const std::string& uploadPath, const std::string& body,
                                         const std::string& filename);
    static HttpResponse makeUploadSuccessResponse(const std::string& filename);
    static HttpResponse makeDeleteSuccessResponse();

    static std::string resolveDeleteFilePath(const HttpRequest& request, const Location& location);
};

