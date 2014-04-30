#!/bin/bash

#
# Compile iodbc and pyodbc from sources
#

function build_iodbc()
{
	iodbc_version=$(echo $1|sed -e 's/.*\([0-9]\+\.[0-9]\+\.[0-9]\+\).*/\1/')
	rm -fr libiodbc-$iodbc_version
	tar xzf $1
	pushd libiodbc-$iodbc_version
	./configure --prefix=/usr
	make
	make install DESTDIR=`pwd`/..
	popd
}


function build_pyodbc()
{
	pyodbc_version=$(echo $1|sed -e 's/.*\([0-9]\+\.[0-9]\+\.[0-9]\+\|latest\).*/\1/')
	rm -fr pyodbc-$pyodbc_version
	unzip $1
	pushd pyodbc-$pyodbc_version
	sed -e "s/'odbc'/'iodbc'/" -i setup.py
	CFLAGS=-I`pwd`/../usr/include LDFLAGS=-L`pwd`/../usr/lib python setup.py build
	cp `find . -name pyodbc.so ` ..
	popd
}


build_iodbc libiodbc*.tar.gz
build_pyodbc pyodbc*.zip
