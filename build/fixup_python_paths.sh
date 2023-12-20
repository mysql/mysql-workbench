#!/bin/bash

# Fix paths from dylibs in MacOSX so that they're loaded from the app bundle
# instead of /usr/local

# <file to fix>

new_path="@executable_path/../Frameworks/Python.framework/Versions/3.11/Python"

for file in $*; do
  if [ -f $file ]; then
    install_name_tool -change "@executable_path/../Frameworks/Python" "$new_path" $file
    install_name_tool -change "@rpath/Python"                         "$new_path" $file
    install_name_tool -change "Python"                                "$new_path" $file
  fi
done
