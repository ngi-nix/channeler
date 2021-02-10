#!/usr/bin/env python

def protoid(version_string):
  from hashlib import sha512
  h = sha512(version_string)
  ver = '0x' + h.hexdigest()[0:8]
  return ver

if __name__ == '__main__':
  import sys
  version_string = 'Testing v0.1'
  if len(sys.argv) > 1:
    version_string = sys.argv[1]
  print('Version string:  ', version_string)
  print('Proto identifier:', protoid(version_string))
