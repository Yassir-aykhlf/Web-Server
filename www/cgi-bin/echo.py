#!/usr/bin/env python3
"""CGI script that reads POST body and echoes it back."""
import os
import sys

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

print("Content-Type: text/html")
print("")
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI POST Echo</title></head>")
print("<body>")
print("<h1>CGI POST Echo</h1>")
print("<p>Method: {}</p>".format(os.environ.get("REQUEST_METHOD", "unknown")))
print("<p>Content-Length: {}</p>".format(content_length))
print("<p>Body: {}</p>".format(body))
print("</body>")
print("</html>")
