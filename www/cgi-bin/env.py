#!/usr/bin/env python3
"""CGI script that reads query string parameters."""
import os

query = os.environ.get("QUERY_STRING", "")
method = os.environ.get("REQUEST_METHOD", "UNKNOWN")
content_length = os.environ.get("CONTENT_LENGTH", "0")
content_type = os.environ.get("CONTENT_TYPE", "")

print("Content-Type: text/plain")
print()
print("METHOD={}".format(method))
print("QUERY_STRING={}".format(query))
print("CONTENT_LENGTH={}".format(content_length))
print("CONTENT_TYPE={}".format(content_type))

if method == "POST" and content_length and int(content_length) > 0:
    import sys
    body = sys.stdin.read(int(content_length))
    print("BODY={}".format(body))
