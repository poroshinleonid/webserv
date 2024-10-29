#!/usr/local/bin/python3
import os
import sys

content = False
for name, value in os.environ.items():
  if (name == "CONTENT_LENGTH"):
    content = True
  print("{0}: {1}".format(name, value))
  print("{0}: {1}".format(name, value), file=sys.stderr)
  
if (content == True):
  s = input()
  print(s)
