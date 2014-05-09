#!/bin/bash

echo "Scanning XML files"
./scan_xml.py ../res/wbdata/*.xml > wbcore.po

echo "Scanning library, backend and modules code"
xgettext -j -LC++ --default-domain=wbcore --add-comments --keyword=_ --keyword=N_ --keyword=C_:1c,2 --keyword=NC_:1c,2 \
           --flag=strfmt:1:c-format --from-code=UTF-8 \
            `find ../library ../backend ../frontend/common ../modules ../plugins -name \*.cpp -o -name \*.c `

echo "Scanning Python code"
xgettext -j -LPython --default-domain=wbcore --add-comments --keyword=_ --keyword=N_ \
            `find ../modules ../plugins -name \*.py`


