#!/usr/bin/env python3

import os
import re
import sys

# Define HTTP resposnses

ok_response = b"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 136\r\nConnection: close\r\n\r\n<script>\r\n    alert(\"File uploaded successfully!\");\r\n    window.location.href = \"/pictures\";\r\n</script>\r\n\r\n"

exists_response = b"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 146\r\nConnection: close\r\n\r\n<script>\r\n    alert(\"Error: File already exists. Please upload a different file.\");\r\n    window.location.href = \"/pictures\";\r\n</script>\r\n\r\n"

# Initialize variables
filename = ""
boundary = None
file_content = b""

# Read input as binary from stdin
input_data = sys.stdin.buffer.read()

# Parse boundary from the first line
first_line_end = input_data.find(b"\r\n")
boundary = input_data[:first_line_end]

# Locate and extract headers
headers_end = input_data.find(b"\r\n\r\n", first_line_end) + 4
headers = input_data[first_line_end+2:headers_end].decode()

# Extract filename from Content-Disposition header
filename_match = re.search(r'filename="(.+?)"', headers)
if filename_match:
    filename = filename_match.group(1)
else:
    print(b"HTTP/1.1 400 Bad Request\r\nContent-type: text/plain\r\n\r\nMissing filename\r\n")
    sys.exit(1)

# Extract file content after headers until the boundary
file_start = headers_end
file_end = input_data.rfind(boundary) - 2  # -2 to remove the trailing CRLF
file_content = input_data[file_start:file_end]

# Set up file paths
script_path = os.path.abspath(os.path.dirname(__file__))
uploads_path = os.path.join(script_path, "uploads")
file_path = os.path.join(uploads_path, filename)

# Check if file already exists
if os.path.exists(file_path):
    sys.stdout.buffer.write(exists_response)
    sys.exit(0)

# Write the binary content to file
with open(file_path, 'wb') as f:
    f.write(file_content)

# Send OK response
sys.stdout.buffer.write(ok_response)