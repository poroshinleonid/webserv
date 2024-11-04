#!/usr/bin/env python3

from sys import stderr
import re
import os

ok_response = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 12\r\n\r\nuploaded\r\n\r\n"
exists_response = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 23\r\n\r\nfile already exists\r\n\r\n"
filetext = []
filename = ""

def crlf_input() -> str:
    line = input()
    if line.endswith("\r"):
        line = line[:-1]
    return line

# bound
line = crlf_input()
assert(line.startswith("----"))

# content-disposition
line = crlf_input()
assert(line.startswith("Content-Disposition"))
filename_pat = re.compile(r'.*name="(.*?)".*')
filename = filename_pat.match(line).groups()[0]

# content-type
line = crlf_input()
assert(line.startswith("Content-Type"))

# empty
line = crlf_input()
assert(line == "")

# file content
while True:
    try:
        line = crlf_input()
        filetext.append(line)
    except EOFError:
        break

# last line should be boundry
assert(filetext[-1].startswith("----"))
filetext.pop()

script_path = os.path.abspath(os.path.dirname(__file__))
uploads_path = os.path.join(script_path, "uploads")
file_path = os.path.join(uploads_path, filename)

if os.path.exists(file_path):
    print(exists_response)
    exit(0)

open(file_path, 'w').write('\n'.join(filetext))
print(ok_response)
