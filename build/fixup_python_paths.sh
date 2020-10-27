#!/bin/bash

# Fix paths from dylibs in MacOSX so that they're loaded from the app bundle
# instead of /usr/local

# <file to fix>

for file in $*; do

  if [ -f $file ]; then
    install_name_tool -change "@rpath/Python3.framework/Versions/3.7/Python3" "/Library/Frameworks/Python.framework/Versions/3.7/Python" $file
  fi

done
