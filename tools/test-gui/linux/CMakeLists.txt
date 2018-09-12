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

project(hello-world)
cmake_minimum_required(VERSION 2.6)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

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

add_executable(gui-test main.cpp)
# Link the target to the GTK+ libraries
target_link_libraries(gui-test ${GTK3_LIBRARIES})
