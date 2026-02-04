#pragma once

#include "ConfigExceptions.hpp"
#include "ConfigNode.hpp"
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#define MAX_PARAMS_SIZE 1000

struct DirectiveRule
{
  size_t minParams;
  size_t maxParams;
  bool allowsChildren;
  vector<string> allowedContexts;
  bool (*paramValidator)(const vector<string> &);

  DirectiveRule()
      : minParams(0),
        maxParams(0),
        allowsChildren(false),
        paramValidator(NULL) {};

  DirectiveRule(
      size_t minP,
      size_t maxP,
      bool allowsCh,
      const vector<string> &contexts,
      bool (*validator)(const vector<string> &))
      : minParams(minP),
        maxParams(maxP),
        allowsChildren(allowsCh),
        allowedContexts(contexts),
        paramValidator(validator)
  {
  }
};

class ConfigValidator
{
private:
  map<string, DirectiveRule> rules_;

  // Parameter validation functions
  static bool isValidHostname(const string &hostname);
  static bool isValidIPv4(const string &ip);
  static bool isValidIPv6(const string &ip);
  static bool isValidLocationPath(const string &path);
  static bool isValidPort(const string &port);
  static bool validateListen(const vector<string> &params);
  static bool validatePath(const vector<string> &params);
  static bool validateMethod(const vector<string> &params);
  static bool validateBoolean(const vector<string> &params);
  static bool validateSize(const vector<string> &params);
  static bool validateServerName(const vector<string> &params);
  static bool validateIndex(const vector<string> &params);
  static bool isValidErrorPagePath(const string &path);
  static bool validateErrorPage(const vector<string> &params);
  static bool validateReturn(const vector<string> &params);
  static bool validateCgiExt(const vector<string> &params);
  static bool isReturnUrl(const string &value);
  static bool isValidReturnCode(const string &value);
  static bool isRedirectCode(int code);
  static bool validateUploadStore(const vector<string> &params);
  static bool validateTimeout(const vector<string> &params);
  static bool noValidation(const vector<string> &params);
  static bool validateLocation(const vector<string> &params);
  static bool validateInternal(const vector<string> &params);
  static bool isAbsolutePath(const string &path);
  static bool isValidFilesystemPath(const string &path);
  static bool validateCgiPath(const vector<string> &params);

  // Validation helpers
  void validateNode(const ConfigNode &node, const string &context);
  bool isAllowedInContext(
      const string &directive, const string &context);

  // Initialize rules
  void initializeRules();

public:
  ConfigValidator();

  // Main validation method
  void validate(const ConfigNode &root);
};
