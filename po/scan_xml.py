#!/usr/bin/python

import re

keywords=["tooltip", "caption"]

def scan_file(file):
    for lnum, line in enumerate(open(file)):
        for k in keywords:
            r = re.match('.*<.*"%s".*>(.*)</.*>'%k, line)
            if r:
                print()
                print('#: %s:%i'%(file, lnum+1))
                print('msgid "%s"' % r.groups()[0])
                print('msgstr ""')
    
import sys
for f in sys.argv[1:]:
    scan_file(f)
