#!/bin/bash

# Fix paths from dylibs in MacOSX so that they're loaded from the app bundle
# instead of /usr/local

# <file to fix>

if test "$NEWTARGETPATH" = ""; then
  NEWTARGETPATH="@executable_path/../Frameworks"
fi

if test "$CHANGEPATHS" = ""; then
  CHANGEPATHS="/usr/local"
fi

for file in $*; do

  if [ -f $file ]; then
    new_targetid=$NEWTARGETPATH/$(basename $file)

    # change the id of the library if its a dylib
    if echo $new_targetid|grep dylib > /dev/null; then
      # and not already a relative path
      if ! otool -DX $file|grep @ > /dev/null; then
        install_name_tool -id $new_targetid $file
        install_name_tool -change $(otool -DX $file|head -1) $new_targetid $file
      fi
    fi

    for lib in $(otool -L $file|egrep "$CHANGEPATHS"|grep .dylib|cut -f2|cut -f1 -d\  ); do
      new_id=$NEWTARGETPATH/$(basename $lib)
      install_name_tool -change $lib $new_id $file
    done

  fi

done
