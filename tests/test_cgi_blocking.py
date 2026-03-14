#!/usr/bin/env python3
"""Test if CGI execution is non-blocking in the web server."""
import subprocess
import threading
import time
import sys
import urllib.request
import urllib.error

def timed_request(url, label, timeout=35):
    """Make a request and return (label, status_code, elapsed_seconds)."""
    start = time.time()
    try:
        req = urllib.request.urlopen(url, timeout=timeout)
        status = req.getcode()
        req.read()
        req.close()
    except urllib.error.HTTPError as e:
        status = e.code
    except Exception as e:
        status = str(e)
    elapsed = time.time() - start
    return (label, status, elapsed)

results = {}

def request_thread(url, label, timeout=35):
    results[label] = timed_request(url, label, timeout)

print("=" * 60)
print("CGI Non-Blocking Test")
print("=" * 60)

# Test 1: Sanity check - normal CGI
print("\n[Test 1] Quick CGI sanity check (hello.py)...")
label, status, elapsed = timed_request("http://localhost:8080/cgi-bin/hello.py", "hello", 5)
print(f"  Status: {status}, Time: {elapsed:.3f}s")
if status == 200 and elapsed < 2:
    print("  ✅ PASS: CGI works correctly")
else:
    print("  ❌ FAIL: CGI not working properly")

# Test 2: The critical non-blocking test
print("\n[Test 2] Non-blocking test: slow CGI + fast static request in parallel")
print("  Launching slow CGI request (infinite.py, will timeout in ~30s)...")

# Start slow request in a thread
slow_thread = threading.Thread(target=request_thread,
                                args=("http://localhost:8080/cgi-bin/infinite.py", "slow", 35))
slow_thread.start()

# Wait a moment to ensure slow request has reached the server
time.sleep(0.5)

# Now send a fast request
print("  Sending fast static request while slow CGI is running...")
fast_label, fast_status, fast_elapsed = timed_request("http://localhost:8080/", "fast", 10)
print(f"  Fast request: Status={fast_status}, Time={fast_elapsed:.3f}s")

if fast_elapsed < 2.0:
    print("  ✅ PASS: Fast request completed in {:.3f}s — CGI is NON-BLOCKING".format(fast_elapsed))
else:
    print("  ❌ FAIL: Fast request took {:.3f}s — CGI is BLOCKING the event loop!".format(fast_elapsed))

# Test 3: Multiple concurrent fast requests while slow CGI is running
print("\n[Test 3] Multiple concurrent fast requests during slow CGI...")
all_fast = True
for i in range(1, 4):
    label, status, elapsed = timed_request("http://localhost:8080/", f"multi_{i}", 10)
    print(f"  Request {i}: Status={status}, Time={elapsed:.3f}s")
    if elapsed > 2.0:
        all_fast = False

if all_fast:
    print("  ✅ PASS: All fast requests completed quickly")
else:
    print("  ❌ FAIL: Some fast requests were delayed")

# Wait for the slow CGI to finish
print("\n  Waiting for slow CGI to timeout...")
slow_thread.join()
slow_label, slow_status, slow_elapsed = results["slow"]
print(f"  Slow CGI: Status={slow_status}, Time={slow_elapsed:.3f}s")

print("\n" + "=" * 60)
print("SUMMARY:")
if fast_elapsed < 2.0 and all_fast:
    print("  CGI execution is NON-BLOCKING ✅")
    print("  (But note: the CGI itself blocks its OWN client connection)")
    print("  Other clients are served concurrently while CGI runs.")
else:
    print("  CGI execution is BLOCKING ❌")
    print("  The server cannot serve other clients while a CGI script runs.")
print("=" * 60)
