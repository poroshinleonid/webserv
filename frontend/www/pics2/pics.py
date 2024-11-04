#!/usr/bin/env python3

# import sys

# # Read all data from standard input as binary
# data = sys.stdin.buffer.read()

# # Step 1: Save the first line that ends with CRLF (\r\n)
# # Find the position of the first CRLF in the data
# first_crlf_crlf_pos = data.find(b"\r\n\r\n")
# first_crlf_pos = data.find(b"\r\n")

# first_line = ""

# # Check if we found a CRLF
# if first_crlf_pos != -1 and first_crlf_crlf_pos != -1:
#     first_line = data[:first_crlf_pos + 2]  # Include one CRLF
#     data = data[first_crlf_crlf_pos + 2:]  # Skip over the first part
#     last_line_start = data.find(first_line)
#     last_line_end = last_line_start + len(first_line) + 2 # include one CRLF
#     data = data[:last_line_start] + data[last_line_end:]

# with open("out1", "wb") as outfile:
#     outfile.write(data)

# print("HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 13\r\n\r\nuploaded_\r\n\r\n")

# import sys

# # Read all data from standard input as binary
# data = sys.stdin.buffer.read()
# from werkzeug.datastructures import FileStorage
# from werkzeug.formparser import parse_form_data
# import io

# # Simulated binary data received from a multipart/form-data HTTP request
# binary_data = (b'--boundary\r\n'
#                 b'Content-Disposition: form-data; name="image"; filename="uploaded_image.jpg"\r\n'
#                 b'Content-Type: image/jpeg\r\n\r\n'
#                 b'\xFF\xD8\xFF\xE0\x00\x10\x4A\x46\x49\x46\x00\x01\x01\x01\x00\x00\x00\x00'  # Simulated JPEG data
#                 b'\r\n--boundary--\r\n')
# content_type = 'multipart/form-data; boundary=boundary'

# # Function to simulate the request environment
# def make_stream(data):
#     return io.BytesIO(data)

# # Simulate the WSGI environment
# stream = make_stream(binary_data)
# environ = {
#     'REQUEST_METHOD': 'POST',
#     'CONTENT_TYPE': content_type,
#     'wsgi.input': stream,
# }

# # Parse the form data
# form, files = parse_form_data(environ)

# # Access and save the uploaded image
# if 'image' in files:
#     file_storage: FileStorage = files['image']
#     # Save the image to a file
#     with open(file_storage.filename, 'wb') as f:
#         f.write(file_storage.read())  # Read the file content and save it

# # The image should now be saved successfully without any errors.

# print("HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 13\r\n\r\nuploaded_\r\n\r\n")





























#---------------------------------
from sys import stdin

# Open "out1" in binary write mode
with open("out1", "wb") as outfile:
    # Read all binary data from standard input until EOF
    data = sys.stdin.buffer.read()
    # Write the binary data to the file
    outfile.write(data)

print("HTTP/1.1 200 OK\r\nContent-type: text/plain\r\nContent-length: 13\r\n\r\nuploaded_\r\n\r\n")
#---------------------------------


# for line in stdin:
#   print(ok_response)


# import cgi
# import os
# import sys

# # Define the directory where uploaded files will be saved
# UPLOAD_DIRECTORY = "uploads"

# # Create the uploads directory if it doesn't exist
# os.makedirs(UPLOAD_DIRECTORY, exist_ok=True)

# # Set the content type for the response
# print("Content-Type: text/html")
# print()  # End of headers

# # Handle file upload
# form = cgi.FieldStorage()

# # Check if a file is uploaded
# if "file" not in form:
#     print("<html><body><h2>No file part</h2></body></html>")
#     sys.exit()

# file_item = form["file"]

# # If a file was uploaded
# if file_item.filename:
#     file_path = os.path.join(UPLOAD_DIRECTORY, os.path.basename(file_item.filename))
#     with open(file_path, "wb") as f:
#         f.write(file_item.file.read())
#     print("<html><body><h2>File uploaded successfully</h2></body></html>")
# else:
#     print("<html><body><h2>No file selected</h2></body></html>")