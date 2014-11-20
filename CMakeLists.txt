cmake_minimum_required (VERSION 2.8)
# 2.8 needed because of ExternalProject

project(WORKBENCH)

# Retrieve current Workbench version variables
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/get_wb_version.sh major
                OUTPUT_VARIABLE WB_MAJOR_VERSION)
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/get_wb_version.sh minor
                OUTPUT_VARIABLE WB_MINOR_VERSION)
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/get_wb_version.sh revision
                OUTPUT_VARIABLE WB_RELEASE_VERSION)
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/get_wb_version.sh full_version
                OUTPUT_VARIABLE WB_VERSION)

string(STRIP ${WB_MAJOR_VERSION} WB_MAJOR_VERSION)
string(STRIP ${WB_MINOR_VERSION} WB_MINOR_VERSION)
string(STRIP ${WB_RELEASE_VERSION} WB_RELEASE_VERSION)
string(STRIP ${WB_VERSION} WB_VERSION)


#  About MySQL and CMake
#  http://dev.mysql.com/doc/internals/en/autotools-to-cmake.html
#  http://www.lenzg.net/archives/291-Building-MySQL-Server-with-CMake-on-LinuxUnix.html

#-----------------------------------------------------------------------
# Find packages and other useful checks
#-----------------------------------------------------------------------

# Look for our custom modules in cmake/Modules:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build/cmake/Modules/")

find_package(PkgConfig REQUIRED)

find_package(GTK2 2.20 REQUIRED gtk gtkmm)
pkg_check_modules(GTHREAD REQUIRED gthread-2.0)
pkg_check_modules(GMODULE REQUIRED gmodule-2.0)
set(GTK2_INCLUDE_DIRS ${GTK2_INCLUDE_DIRS} ${GTHREAD_INCLUDE_DIRS} ${GMODULE_INCLUDE_DIRS})
list(REMOVE_DUPLICATES GTK2_INCLUDE_DIRS)
set(GTK2_LIBRARIES ${GTK2_LIBRARIES} ${GTHREAD_LIBRARIES} ${GMODULE_LIBRARIES})
list(REMOVE_DUPLICATES GTK2_LIBRARIES)

pkg_check_modules(GLIB REQUIRED glib-2.0)

if (USE_BUNDLED_MYSQLDUMP)
  set(USE_BUNDLED_MYSQLDUMP -DHAVE_BUNDLED_MYSQLDUMP=1)
endif ()


find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})

# All known publicly available versions of Antlr3C are buggy

if (SKIP_BUNDLED_ANTLR)
  find_package(Antlr3C)
endif ()
if (ANTLR3C_FOUND)
  set(ANTLR3C_REQUIRED_VERSION "3.4")
  if (${ANTLR3C_VERSION} VERSION_LESS ${ANTLR3C_REQUIRED_VERSION})
    message("Required antlr3c version ${ANTLR3C_REQUIRED_VERSION} but ${ANTLR3C_VERSION} found, using bundled version")
    unset(ANTLR3C_FOUND)
  endif ()
endif ()

# In Ubuntu 13.04, the built-in check for python fails.. so we have to resort to the check with pkg-config (which doesn't work in many distros)
find_package(PythonLibs 2.6)
if (NOT PYTHONLIBS_FOUND)
    pkg_check_modules(PYTHON REQUIRED python)
endif ()

find_package(MySQL REQUIRED)
if (UNIX)
	set(MYSQL_DEFINITIONS "-DBIG_JOINS=1"  "-fno-strict-aliasing")
endif()

find_package(LibXml2 REQUIRED)

find_package(OpenGL REQUIRED)

find_package(CTemplate)
find_package(MySQLCppConn REQUIRED)
find_package(VSqlite REQUIRED)
find_package(TinyXML REQUIRED)
find_package(GDAL REQUIRED)

if (MYSQLCPPCONN_VERSION_1_1_4)
  add_definitions(-DMYSQLCPPCONN_VERSION_1_1_4)
endif ()

if (MYSQLCPPCONN_VERSION_1_1_5 EQUAL "1")
  add_definitions(-DMYSQLCPPCONN_VERSION_1_1_5)
endif (MYSQLCPPCONN_VERSION_1_1_5 EQUAL "1")


if (UNIX)
  option(USE_UNIXODBC "Use unixODBC instead of iODBC" OFF)
  find_package(IODBC)
  find_package(UNIXODBC)

  if (USE_UNIXODBC)
    if (UNIXODBC-NOTFOUND)
      message(FATAL_ERROR "You've selected Unixodbc, but it cannot be found, CMake will exit.")
    else()
		  set(ODBC_LIBRARIES ${UNIXODBC_LIBRARIES})
  		set(ODBC_INCLUDE_DIRS ${UNIXODBC_INCLUDE_DIRS})
	  	set(ODBC_DEFINITIONS ${UNIXODBC_DEFINITIONS})
    endif()
  else()
    if (IODBC_FOUND)
      message("Found iodbc, using it as ODBC library.")
      set(ODBC_LIBRARIES ${IODBC_LIBRARIES})
      set(ODBC_INCLUDE_DIRS ${IODBC_INCLUDE_DIRS})
      set(ODBC_DEFINITIONS ${IODBC_DEFINITIONS})
    elseif (UNIXODBC_FOUND)
      message("Found unixodbc, using it as ODBC library.")
      set(ODBC_LIBRARIES ${UNIXODBC_LIBRARIES})
      set(ODBC_INCLUDE_DIRS ${UNIXODBC_INCLUDE_DIRS})
      set(ODBC_DEFINITIONS ${UNIXODBC_DEFINITIONS})
    else()
      message(FATAL_ERROR "Workbench required iodbc or Unixodbc, none was found, CMake will exit.")
    endif()
  endif(USE_UNIXODBC)

endif(UNIX)


pkg_check_modules(PCRE REQUIRED libpcre libpcrecpp)
pkg_check_modules(CAIRO REQUIRED cairo>=1.5.12)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(LIBZIP REQUIRED libzip)
if (UNIX)
	pkg_check_modules(GNOME_KEYRING gnome-keyring-1)
	if (GNOME_KEYRING_FOUND)
		set(KEYRING_DEFINITION     "HAVE_GNOME_KEYRING")
	else()
		pkg_check_modules(GNOME_KEYRING gnome-keyring)
		if (GNOME_KEYRING_FOUND)
			set(KEYRING_DEFINITION     "HAVE_OLD_GNOME_KEYRING")
		endif()
	endif()
endif()

if (UNIX)
  include (CheckIncludeFiles)
  CHECK_INCLUDE_FILES ("sys/prctl.h" HAVE_PRCTL)
  if (HAVE_PRCTL)
    add_definitions(-DHAVE_PRCTL_H)
  endif()
endif()

include (CheckFunctionExists)
check_function_exists (strtol HAVE_STRTOL)
check_function_exists (strtoll HAVE_STRTOLL)
check_function_exists (strtold HAVE_STRTOLD)
check_function_exists (strtoull HAVE_STRTOULL)
check_function_exists (strtoimax HAVE_STRTOIMAX)
check_function_exists (strtoumax HAVE_STRTOUMAX)

if (UNIX)
	if (NOT LIB_INSTALL_DIR)
		set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")
	endif()
	if (NOT SHARE_INSTALL_PREFIX)
		set(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share")
	endif()
	if (NOT ETC_INSTALL_DIR)
		set(ETC_INSTALL_DIR "/etc")
	endif()
	if (NOT REAL_EXECUTABLE_DIR)
		set(REAL_EXECUTABLE_DIR "${CMAKE_INSTALL_PREFIX}/bin")
	endif()

	set(WB_INSTALL_DIR              "${CMAKE_INSTALL_PREFIX}")
	set(WB_INSTALL_DIR_EXECUTABLE   "${REAL_EXECUTABLE_DIR}")
	set(WB_INSTALL_BIN_DIR          "${CMAKE_INSTALL_PREFIX}/bin")
	set(WB_INSTALL_LIB_DIR          "${LIB_INSTALL_DIR}/mysql-workbench")
	set(WB_PYTHON_MODULES_DIR       "${WB_INSTALL_LIB_DIR}/modules")
	set(WB_INSTALL_SHARED_DIR       "${SHARE_INSTALL_PREFIX}")
	set(WB_INSTALL_ETC_DIR          "${ETC_INSTALL_DIR}")
	set(WB_PACKAGE_SHARED_DIR       "${SHARE_INSTALL_PREFIX}/mysql-workbench")
	set(WB_PACKAGE_PLUGINS_DIR      "${LIB_INSTALL_DIR}/mysql-workbench/plugins")
	set(WB_PYTHON_MODULES_DATA_DIR  "${SHARE_INSTALL_PREFIX}/mysql-workbench/modules/data")
	set(WB_INSTALL_DOC_DIR          "${SHARE_INSTALL_PREFIX}/doc/mysql-workbench")
endif()

include(ExternalProject)

if (NOT ANTLR3C_FOUND)
  message("Using bundled Antlr C runtime")
  #  Use for Antlr3 the one we have in /ext:
  set(ANTLR3C_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/ext/antlr-runtime ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/include)
  set(ANTLR3C_LIBRARIES "-L${PROJECT_BINARY_DIR}/antlr-wb -lantlr3c_wb")

  ExternalProject_Add(antlr-wb
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/ext/antlr-runtime
    CONFIGURE_COMMAND autoreconf -i -s && ${PROJECT_SOURCE_DIR}/ext/antlr-runtime/configure
        --prefix=${PROJECT_BINARY_DIR}/antlr-wb
        --libdir=${PROJECT_BINARY_DIR}/antlr-wb
	--disable-abiflags
    BUILD_COMMAND make
    BUILD_IN_SOURCE 2
    INSTALL_COMMAND make install
  )

  link_directories(${PROJECT_BINARY_DIR}/antlr-wb)
endif ()

# Uncomment these lines if at any time you want to force the configure and/or build
# of antlr-wb:
#ExternalProject_Add_Step(antlr-wb forceconfigure
#    COMMAND ${CMAKE_COMMAND} -E echo "Force configure of antlr-wb"
#    DEPENDEES update
#    DEPENDERS configure
#    ALWAYS 1
#)
#ExternalProject_Add_Step(antlr-wb forcebuild
#    COMMAND ${CMAKE_COMMAND} -E echo "Force build of antlr-wb"
#    DEPENDEES configure
#    DEPENDERS build
#    ALWAYS 1
#)


#include(CheckCXXCompilerFlag)
#check_cxx_compiler_flag(-Wall  HAS_ALL_WARNS)
#if (HAS_ALL_WARNS)
#	add_definitions(-Wall)
#endif()
#check_cxx_compiler_flag(-Wsign-compare  HAS_SIGN_COMPARE)
#if (HAS_SIGN_COMPARE)
#	add_definitions(-Wsign-compare)
#endif()
add_definitions(-D__STDC_FORMAT_MACROS)

set(BUILD_SHARED_LIBS ON)  # By default all libs will be built as shared libs

# Add some common compiler flags. TODO: Shouldn't this be done checking if whatever compiler
# we use supports each flag to allow for non-gcc compilers in Linux (as above)?
if (CMAKE_COMPILER_IS_GNUCXX)
	set(MY_CXXFLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-deprecated -DTIXML_USE_STL=1")

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror -ggdb3 ${MY_CXXFLAGS}")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${MY_CXXFLAGS}") 
endif()


#-----------------------------------------------------------------------
# Define some useful variables and options
#-----------------------------------------------------------------------

#set(NCORES 3)  # Use this number of cores when compiling
#if(CMAKE_COMPILER_IS_GNUCXX)
#	set(CMAKE_CXX_FLAGS "${CMAKE_CKK_FLAGS} -j${NCORES}")
#	message(STATUS "CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")
#	#set(CMAKE_EXE_LINKER_FLAGS "-s")  ## Strip binary
#endif()

set(GRT_LIBRARIES ${PYTHON_LIBRARIES} ${LIBXML2_LIBRARIES})
set(GRT_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

configure_file(${PROJECT_SOURCE_DIR}/mysql-workbench.desktop.in mysql-workbench.desktop)

# FIXME: Should this go here? ---v
include_directories(${PROJECT_SOURCE_DIR}/plugins/migration/copytable ${PROJECT_SOURCE_DIR}/library/base)
include_directories(${GTK2_INCLUDE_DIRS})

add_subdirectory(tools)
add_subdirectory(backend)
add_subdirectory(ext)
add_subdirectory(frontend)
add_subdirectory(images)
add_subdirectory(library)
add_subdirectory(modules)
add_subdirectory(plugins)
add_subdirectory(res)
if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/internal)
  add_subdirectory(internal)
endif()

install(FILES ${CMAKE_BINARY_DIR}/mysql-workbench.desktop DESTINATION ${WB_INSTALL_SHARED_DIR}/applications)

if (EXISTS ${PROJECT_SOURCE_DIR}/COPYING)
    install(FILES README COPYING DESTINATION ${WB_INSTALL_DOC_DIR})
    install(FILES build/mysql-workbench-community.sharedmimeinfo DESTINATION ${WB_INSTALL_SHARED_DIR}/mime/packages RENAME mysql-workbench.xml)
    install(FILES build/mysql-workbench-community.mime DESTINATION ${WB_INSTALL_SHARED_DIR}/mime-info RENAME mysql-workbench.mime)
else ()
    install(FILES README LICENSE.mysql DESTINATION ${WB_INSTALL_DOC_DIR})
    install(FILES build/mysql-workbench-commercial.sharedmimeinfo DESTINATION ${WB_INSTALL_SHARED_DIR}/mime/packages RENAME mysql-workbench.xml)
    install(FILES build/mysql-workbench-commercial.mime DESTINATION ${WB_INSTALL_SHARED_DIR}/mime-info RENAME mysql-workbench.mime)
endif ()

install(FILES build/build_freetds.sh
              samples/models/sakila_full.mwb
        DESTINATION ${WB_PACKAGE_SHARED_DIR}/extras
       )
       
