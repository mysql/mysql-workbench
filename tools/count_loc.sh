#!/bin/bash

dirs="backend frontend  generated/grts internal library modules plugins prefix res"
exts="c cpp cxx mm m py cs h js g4"

headers=0
total_files=0
total=0
for ext in $exts; do
    files=`find $dirs -name \*.$ext|grep -v unit-test|wc -l`
    count=$(wc -l /dev/null `find $dirs -name \*.$ext|grep -v unit-test`|tail -1|awk '{print $1}')
    if test $ext = h; then
        headers=$files
    fi

    echo "$ext: $count loc  $files files"
    total=$(($total + $count))
    total_files=$(($total_files + $files))
done
echo "Total: $total"
echo "Total Files: $total_files ($(($total_files - $headers)) without headers)"


