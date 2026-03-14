#!/usr/bin/env python3
"""CGI script that sets and reads cookies."""
import os

cookie = os.environ.get("HTTP_COOKIE", "")
print("Content-Type: text/html")
print("Set-Cookie: test_session=abc123; Path=/")
print()
print("<html><body>")
print("<h1>Cookie Test</h1>")
print("<p>Received cookies: {}</p>".format(cookie))
print("</body></html>")
