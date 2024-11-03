#!/usr/bin/env python3

from sys import stderr
import re
import os

ok_response = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 12\r\n\r\nuploaded\r\n\r\n"
exists_response = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 23\r\n\r\nfile already exists\r\n\r\n"
content = ""

while True:
    try:
        content += input()
    except EOFError:
        break

print(content, file=stderr)
pat = re.compile(r"filename=(.*)&filetext=(.*)$", re.DOTALL)
assert(pat.search(content))
filename, filetext = pat.search(content).groups()

script_path = os.path.abspath(os.path.dirname(__file__))
uploads_path = os.path.join(script_path, "uploads")
file_path = os.path.join(uploads_path, filename)

if os.path.exists(filename):
    print(exists_response)
    exit(0)

open(file_path, 'w').write(filetext)
print(ok_response)
