#!/usr/bin/env python3
import socket
import sys

def send_100mb_post():
    # Create a socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(('127.0.0.1', 1027))
    
    # Create the body: 100MB of 'Y' characters
    body_size = 100000000
    print(f"Creating request with {body_size} bytes body...")
    
    # Send HTTP request with chunked encoding
    request_line = "POST /directory/youpi.bla HTTP/1.1\r\n"
    headers = (
        "Host: 127.0.0.1:1027\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: test/file\r\n"
        "Connection: close\r\n"
        "\r\n"
    )
    
    sock.send(request_line.encode())
    sock.send(headers.encode())
    
    # Send body in chunks
    chunk_size = 1024 * 1024  # 1MB chunks
    bytes_sent = 0
    
    while bytes_sent < body_size:
        to_send = min(chunk_size, body_size - bytes_sent)
        # Send chunk size in hex
        sock.send(f"{to_send:x}\r\n".encode())
        # Send chunk data
        data = 'Y' * to_send
        sock.send(data.encode())
        sock.send(b"\r\n")
        bytes_sent += to_send
        if bytes_sent % (10 * 1024 * 1024) == 0:
            print(f"  Sent {bytes_sent / 1024 / 1024:.0f}MB")
    
    # Send final chunk
    sock.send(b"0\r\n\r\n")
    print(f"Total sent: {bytes_sent} bytes")
    
    # Read response
    print("Reading response...")
    response_data = b''
    response_size = 0
    while True:
        chunk = sock.recv(1024 * 1024)
        if not chunk:
            break
        response_data += chunk
        response_size += len(chunk)
        if response_size % (10 * 1024 * 1024) == 0 or response_size < 10 * 1024 * 1024:
            print(f"  Received {response_size / 1024 / 1024:.1f}MB")
    
    sock.close()
    
    # Parse response headers
    response_str = response_data.decode('utf-8', errors='ignore')
    header_end = response_str.find("\r\n\r\n")
    if header_end == -1:
        header_end = response_str.find("\n\n")
        headers_only = response_str[:header_end]
        body_start = header_end + 2
    else:
        headers_only = response_str[:header_end]
        body_start = header_end + 4
    
    print("\nResponse Headers:")
    print(headers_only)
    
    # Count body size
    body_size_received = len(response_data) - body_start
    print(f"\nBody size received: {body_size_received} bytes")
    print(f"Body size expected: {body_size} bytes")
    print(f"Difference: {body_size - body_size_received} bytes")
    
    # Check if body matches
    body_bytes = response_data[body_start:]
    y_count = body_bytes.count(b'Y')
    print(f"'Y' characters in body: {y_count}")

if __name__ == '__main__':
    send_100mb_post()
