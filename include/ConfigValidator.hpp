#pragma once

#include "ConfigNode.hpp"
#include "ConfigExceptions.hpp"
#include <string>
#include <vector>
#include <map>
#include <sstream>

#define MAX_PARAMS_SIZE 1000

struct DirectiveRule
{
    size_t minParams;
    size_t maxParams;
    bool allowsChildren;
    std::vector<std::string> allowedContexts;
    bool (*paramValidator)(const std::vector<std::string> &);

    DirectiveRule() : minParams(0), maxParams(0), allowsChildren(false), paramValidator(NULL) {};
    DirectiveRule(size_t minP, size_t maxP, bool allowsCh, const std::vector<std::string> &contexts,
                  bool (*validator)(const std::vector<std::string> &))
        : minParams(minP), maxParams(maxP), allowsChildren(allowsCh), allowedContexts(contexts), paramValidator(validator) {}
};

class ConfigValidator
{
private:
    std::map<std::string, DirectiveRule> rules_;

    // Parameter validation functions
    static bool isValidIPv4(const std::string &ip);
    static bool isValidIPv6(const std::string &ip);
    static bool isValidLocationPath(const std::string &path);
    static bool isValidPort(const std::string &port);
    static bool validateListen(const std::vector<std::string> &params);
    static bool validatePath(const std::vector<std::string> &params);
    static bool validateMethod(const std::vector<std::string> &params);
    static bool validateBoolean(const std::vector<std::string> &params);
    static bool validateSize(const std::vector<std::string> &params);
    static bool validateServerName(const std::vector<std::string> &params);
    static bool validateIndex(const std::vector<std::string> &params);
    static bool validateErrorPage(const std::vector<std::string> &params);
    static bool validateRedirect(const std::vector<std::string> &params);
    static bool validateCgiExt(const std::vector<std::string> &params);
    static bool validateCgiPath(const std::vector<std::string> &params);
    static bool validateUploadStore(const std::vector<std::string> &params);
    static bool validateAlias(const std::vector<std::string> &params);
    static bool validateTimeout(const std::vector<std::string> &params);
    static bool noValidation(const std::vector<std::string> &params);
    static bool validateLocation(const std::vector<std::string> &params);
    // Validation helpers
    void validateNode(const ConfigNode &node, const std::string &context);
    bool isAllowedInContext(const std::string &directive, const std::string &context);

    // Initialize rules
    void initializeRules();

public:
    ConfigValidator();

    // Main validation method
    void validate(const ConfigNode &root);
};
