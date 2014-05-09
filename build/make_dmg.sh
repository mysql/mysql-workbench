#!/bin/sh

app="MySQLWorkbench.app"
srcdir="DerivedData/MySQLWorkbench/Build/Products/Release"
edition=$1
ver=$2
if test "$ver" == ""; then
        echo "./make_dmg.sh <edition> <wbversion>"
        exit 1
fi
templatedmg=~/guibuild/mysqlworkbench-$edition-template.dmg
finaldmg=mysql-workbench-$edition-$ver-osx-i686

if [ ! -d $srcdir ]; then
    srcdir="build/Release"
fi

###############################################################################################

echo "Packaging $finaldmg"

#oifs=$IFS
#IFS=,

echo "Attaching template $templatedmg"
hdiutil attach "$templatedmg" -noautoopen -mountpoint template 

echo "Copying app"
rm -fr "template/$app"
ditto "$srcdir/$app" "template/$app"
if [ $? -ne 0 ]; then
        hdiutil detach template -force -quiet
        echo "Could not copy .app to template dmg"
        exit 1
fi
if test "$edition" != "community"; then
        echo "Copying LGPL srcs to commercial pkg"
        cp ~/guibuild/lgpl-deps/* "template/$app"
fi

echo "Detaching template"
hdiutil detach template -force -quiet

rm -f $finaldmg.*

echo "Creating dmg"
hdiutil convert "$templatedmg" -format UDBZ -imagekey zlib-level=9 -o "$finaldmg" -ov
hdiutil internet-enable -yes "$finaldmg".dmg

