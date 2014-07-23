#!/bin/bash
# Script to compile FreeTDS against ODBC in distributions supported by WB

function determine_distro()
{
    if [ -d /var/lib/rpm ]; then
       # RPM based distro
       echo el
    else
       # deb based distro
       echo ubuntu
    fi
}

distro=`determine_distro`

if test "$distro" = "el"; then
	libiodbc=libiodbc-devel
elif test "$distro" = "fc"; then
	libiodbc=libiodbc-devel
elif test "$distro" = "ubuntu"; then
	libiodbc=libiodbc2-dev
fi

echo "Checking requisites..."
# check if deps are installed
if ! type -a iodbc-config; then
    echo "ERROR: Package $libiodbc is not installed. Install it and try again"
    exit 1
fi

if ! type -a gcc; then
	echo "ERROR: gcc is not installed. Install it and try again"
	exit 1
fi

freetds_tarball=`find . -maxdepth 1 -type f -regex '\./freetds-.*\(\.tar\.gz\|\.tgz\)' -printf "%f" -quit`
if [ -z $freetds_tarball ]; then
    echo "ERROR: Please download the latest freetds (requires 0.92 or newer) source package to this directory"
    exit 1
else
    echo "FreeTDS tarball found: $freetds_tarball"
fi

echo "Compiling freetds..."
rm -fr /tmp/freetdsbuild
mkdir /tmp/freetdsbuild

cp $freetds_tarball /tmp/freetdsbuild

cd /tmp/freetdsbuild

echo "Extracting files from $freetds_tarball..."
tar xzf $freetds_tarball

if `uname` = darwin; then
# force 32bit build
export CFLAGS="-arch i386"
export LDFLAGS="-arch i386"
fi

if iodbc-config --cflags|grep -- -I; then
    ln -s `iodbc-config --cflags|sed -e "s/.*-I\([^ ]*\).*/\1/"` include
else
    ln -s /usr/include .
fi

if iodbc-config --libs|grep -- -L; then
	ln -s `iodbc-config --libs|sed -e "s/.*-L\([^ ]*\).*/\1/"` .
else
	if test `arch` = x86_64; then
		ln -s /usr/lib64 .
	else
		ln -s /usr/lib .
	fi
fi

cd freetds-*[0-9]
./configure --disable-apps --disable-server --disable-pool --with-iodbc=/tmp/freetdsbuild --enable-odbc-wide
make
echo "Go to `pwd` and type make install as the root user"
echo "After that, install the driver which will be installed as /usr/local/lib/libtdsodbc.so"

