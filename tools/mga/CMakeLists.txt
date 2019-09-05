# Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA


# -*- mode:cmake -*-
cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# Detect build type, fallback to release and throw a warning if use didn't specify any
if(NOT CMAKE_BUILD_TYPE)
  message(WARNING "Build type not set, falling back to Release mode.
 To specify build type use:
 -DCMAKE_BUILD_TYPE=<mode> where <mode> is Debug or Release.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
       "Choose the type of build, options are: Debug Release."
       FORCE)
endif(NOT CMAKE_BUILD_TYPE)

option(WITH_LIBCXX "Building with clang++ and libc++(in Linux). To enable with: -DWITH_LIBCXX=On" ON)
option(BUILD_WITH_COVERAGE "Build mga with coverage. To enable with: -DBUILD_WITH_COVERAGE=On" OFF)

project(MGA)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(MySQLCppConn 1.1.8 REQUIRED)
find_package(OpenSSL REQUIRED)
find_package (Threads)

set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.14.sdk)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13")

if(UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  find_package(X11 REQUIRED)
  pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.30)
  pkg_check_modules(ATSPI REQUIRED atspi-2)
  pkg_check_modules(GLIBMM REQUIRED glibmm-2.4>=2.46)
  pkg_check_modules(GIOMM REQUIRED giomm-2.4>=2.46)
  pkg_check_modules(CAIROMM REQUIRED cairomm-1.0>=1.12)
  pkg_check_modules(GTKMM REQUIRED gtkmm-3.0>=3.18)
elseif(APPLE)
  find_library(Cocoa_LIBRARIES Cocoa)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif()

file(STRINGS "version.txt" MGA_VERSION)

# Add some common compiler flags. TODO: Shouldn't this be done checking if whatever compiler
# we use supports each flag to allow for non-gcc compilers in Linux (as above)?
set(MGA_CXXFLAGS -Werror -Wall -Wextra -Wno-unused-parameter -Wno-deprecated)

if(BUILD_WITH_COVERAGE AND UNIX)
  message (STATUS "Enable coverage support due to BUILD_WITH_COVERAGE")
  set(MGA_CXXFLAGS ${MGA_CXXFLAGS} --coverage)
endif()

if("${CMAKE_VERSION}" VERSION_GREATER 3.1.0)
  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND CMAKE_SYSTEM_NAME MATCHES "Linux" AND WITH_LIBCXX)
  	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
else()
	if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
			execute_process(
					COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
			# Just g++-5.0 and greater contain <codecvt> header. (test in ubuntu)
			if (NOT (GCC_VERSION VERSION_GREATER 5.4 OR GCC_VERSION VERSION_EQUAL 5.4))
					message(FATAL_ERROR "${PROJECT_NAME} requires g++ 5.4 or greater.")
			endif ()
	elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND APPLE)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" AND CMAKE_SYSTEM_NAME MATCHES "Linux")
			execute_process(
					COMMAND ${CMAKE_CXX_COMPILER} -dumpversion OUTPUT_VARIABLE CLANG_VERSION)
			if (NOT (CLANG_VERSION VERSION_GREATER 4.2.1 OR CLANG_VERSION VERSION_EQUAL 4.2.1))
					message(FATAL_ERROR "${PROJECT_NAME} requires clang 4.2.1 or greater.")
			endif ()
			# You can use libc++ to compile this project when g++ is NOT greater than or equal to 5.0.
			if (WITH_LIBCXX)
					set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
			endif()
	else ()
			message(FATAL_ERROR "Your C++ compiler does not support C++14.")
	endif ()
endif()

##
## Installation location
##
set(INSTALL_BINDIR "bin")
set(INSTALL_LIBDIR "lib/mga")
set(INSTALL_INCLUDEDIR "include/mga")
set(INSTALL_SHAREDIR "share/mga")
set(INSTALL_DOCDIR "share/doc/mga")

add_subdirectory(src)

if(EXISTS LICENSE.txt)
  install(FILES LICENSE.txt
          DESTINATION ${INSTALL_DOCDIR})
endif()

install(FILES version.txt
      DESTINATION ${INSTALL_DOCDIR})
