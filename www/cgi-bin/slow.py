#!/usr/bin/env python3
"""CGI script that takes 3 seconds to respond (for non-blocking testing)."""
import time

time.sleep(3)

print("Content-Type: text/plain")
print("")
print("slow CGI done after 3s")
