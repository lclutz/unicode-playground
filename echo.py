#!/usr/bin/env python3

import sys

UTF8BOM = b"\xef\xbb\xbf"
sys.stdout.buffer.write(UTF8BOM)

for arg in sys.argv[1:]:
    sys.stdout.buffer.write(arg.encode('utf-8'))
    sys.stdout.buffer.write("\n".encode('utf-8'))
