#!/usr/bin/env python3
import os

print("Content-Type: text/html")
print("")
print("<!DOCTYPE html>")
print("<html>")
print("<head><title>CGI Test</title></head>")
print("<body>")
print("<h1>Hello from CGI!</h1>")
print("<p>Request Method: {}</p>".format(os.environ.get("REQUEST_METHOD", "unknown")))
print("<p>Query String: {}</p>".format(os.environ.get("QUERY_STRING", "")))
print("<p>Script Name: {}</p>".format(os.environ.get("SCRIPT_NAME", "")))
print("</body>")
print("</html>")
