#!/usr/bin/env python3

# print("HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 49\r\n\r\nhere should be a script for viewing pictures\r\n\r\n")
import os

response_header = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n"

script_path = os.path.abspath(os.path.dirname(__file__))
uploads_path = os.path.join(script_path, "uploads")

def wrap_tag(tag, s):
    return f'<{tag}>' + s + f'</{tag}>'

def absolute_file_paths(dir):
    for dirpath, _, filenames in os.walk(dir):
        for filename in filenames:
            abs_path = os.path.abspath(os.path.join(dir, filename))
            yield abs_path

def make_content():
    head = '''<meta charset="utf-8">
<meta name="awesome webserver">'''
    head = wrap_tag('head', head)
    body = ""
    for abs_path in absolute_file_paths(uploads_path):
        extension = os.path.splitext(abs_path)[1]
        if (extension not in ['.jpeg', '.jpg', '.png', '.gif']):
            continue
        body +=  f'<img src="./uploads/{os.path.basename(abs_path)}" width="100" height="100">'
    body = wrap_tag('body', body)
    return head + body

content = make_content()
content_length = len(content)
response_header += f"Content-length: {content_length}\r\n\r\n"
print(response_header + content, end='\r\n\r\n')
