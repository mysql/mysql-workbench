cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

# Detect build type, fallback to release and throw a warning if use didn't specify any
if(NOT CMAKE_BUILD_TYPE)
  message(WARNING "Build type not set, falling back to Release mode.
 To specify build type use:
 -DCMAKE_BUILD_TYPE=<mode> where <mode> is Debug or Release.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
       "Choose the type of build, options are: Debug Release RelWithDebInfo"
       FORCE)
endif(NOT CMAKE_BUILD_TYPE)


# Retrieve current Workbench version
execute_process(COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tools/get_wb_version.sh full_version
                OUTPUT_VARIABLE WB_VERSION)

string(STRIP ${WB_VERSION} WB_VERSION)

project(WORKBENCH LANGUAGES C CXX)

option(ENABLE_COTIRE "Enable cotire to generate precompiled headers" OFF)
option(TEST_BUILD "Enable building additonal debug libs" OFF)
option(BUILD_FOR_GCOV "Enable gcov instrumentation" OFF)
option(USE_BUNDLED_MYSQL "Use bundled MySQL tools" OFF)


#  About MySQL and CMake
#  http://dev.mysql.com/doc/internals/en/autotools-to-cmake.html
#  http://www.lenzg.net/archives/291-Building-MySQL-Server-with-CMake-on-LinuxUnix.html

#-----------------------------------------------------------------------
# Find packages and other useful checks
#-----------------------------------------------------------------------

# Look for our custom modules in cmake/Modules:
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/build/cmake/Modules/")

if(ENABLE_COTIRE)
  include(cotire)
  set_property(DIRECTORY PROPERTY COTIRE_UNITY_LINK_LIBRARIES_INIT "COPY_UNITY")
endif()

if(POLICY CMP0072)
  cmake_policy(SET CMP0072 NEW) # cmake-3.11
endif()
if(POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD) # cmake-3.13
endif()
if(POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD) # cmake-3.14
endif()

find_package(PkgConfig REQUIRED)
find_package(Java QUIET)
find_package(X11 REQUIRED)
find_package(Threads REQUIRED)
find_package(SWIG REQUIRED)
include(${SWIG_USE_FILE})
find_package(ANTLR4 REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(GTHREAD REQUIRED gthread-2.0)
pkg_check_modules(GMODULE REQUIRED gmodule-2.0)
pkg_check_modules(GLIB REQUIRED glib-2.0>=2.30)

if (USE_BUNDLED_MYSQLDUMP)
  set(USE_BUNDLED_MYSQLDUMP -DHAVE_BUNDLED_MYSQLDUMP=1)
endif ()

set(Python_ADDITIONAL_VERSIONS 3.8)
find_package(PythonLibs 3.7 REQUIRED)

# We need to be able to detect python module versions and packages hence this function
find_program(PYTHON_EXEC "python3")
include(FindPackageHandleStandardArgs)

find_package(MySQL 5.6.0 REQUIRED)
find_package(LibXml2 REQUIRED)


find_package(OpenGL REQUIRED)
find_package(MySQLCppConn 1.1.8 REQUIRED)
find_package(VSqlite REQUIRED)
find_package(GDAL REQUIRED)
find_package(Boost REQUIRED)
find_package(libssh 0.9.5 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(Rapidjson 1.1.0 REQUIRED)

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
  pkg_check_modules(LIBSECRET REQUIRED libsecret-1)
	if (LIBSECRET_FOUND)
		set(KEYRING_DEFINITION     "HAVE_LIBSECRET_KEYRING")
	endif()
endif()

if (UNIX)
  include (CheckIncludeFiles)
  CHECK_INCLUDE_FILES ("sys/prctl.h" HAVE_PRCTL)
endif()

include (CheckFunctionExists)
check_function_exists (strtol HAVE_STRTOL)
check_function_exists (strtoll HAVE_STRTOLL)
check_function_exists (strtold HAVE_STRTOLD)
check_function_exists (strtoull HAVE_STRTOULL)
check_function_exists (strtoimax HAVE_STRTOIMAX)
check_function_exists (strtoumax HAVE_STRTOUMAX)

set(ANTLR_JAR_FILENAME "antlr-4.11.1-complete.jar")
get_filename_component(SOURCE_PARENT_DIR ${CMAKE_SOURCE_DIR} DIRECTORY)
set(LINUX_RES_BIN_DIR ${SOURCE_PARENT_DIR}/linux-res/bin)
message("WITH_ANTLR_JAR: ${WITH_ANTLR_JAR}")
if(WITH_ANTLR_JAR)
  get_filename_component(ANTLR_JAR_USER_DIR ${WITH_ANTLR_JAR} DIRECTORY)
  find_path(ANTLR_JAR_PATH ${ANTLR_JAR_FILENAME} ${LINUX_RES_BIN_DIR} ${ANTLR_JAR_USER_DIR})
else(WITH_ANTLR_JAR)
  find_path(ANTLR_JAR_PATH ${ANTLR_JAR_FILENAME} ${LINUX_RES_BIN_DIR})
endif(WITH_ANTLR_JAR)

if(ANTLR_JAR_PATH AND Java_JAVA_EXECUTABLE) 
  set(ANTLR_JAR_PATH ${ANTLR_JAR_PATH}/${ANTLR_JAR_FILENAME})
  message(STATUS "ANTLR jar: " ${ANTLR_JAR_PATH})
else()
  message(WARNING ${ANTLR_JAR_FILENAME} " or java runtime wasn't found which is required if you'd like to rebuild Lexer and Parser files")
endif()


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

add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DGTK_DISABLE_SINGLE_INCLUDES)
add_definitions(-DGDK_DISABLE_DEPRECATED)
add_definitions(-DGTK_DISABLE_DEPRECATED)

set(BUILD_SHARED_LIBS ON)  # By default all libs will be built as shared libs
# SET(GCOV OFF CACHE BOOL "Add gcov flags")

# Determine std c++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(WB_CXXFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-deprecated -Wno-deprecated-copy -Wno-deprecated-declarations -Wno-overloaded-virtual)
set(WB_CFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-deprecated -Wno-deprecated-copy -Wno-deprecated-declarations -Wno-overloaded-virtual)

if(BUILD_FOR_GCOV)
    message ("Enable GCOV due to BUILD_FOR_GCOV")
    set(WB_CXXFLAGS ${WB_CXXFLAGS} -fprofile-arcs -ftest-coverage )
    set(WB_CFLAGS ${WB_CFLAGS} -fprofile-arcs -ftest-coverage)
else()
    set(WB_CXXFLAGS ${WB_CXXFLAGS} -Werror -Wno-nonnull)
    set(WB_CFLAGS ${WB_CFLAGS} -Werror)
endif()

#-----------------------------------------------------------------------
# Define some useful variables and options
#-----------------------------------------------------------------------
configure_file(${PROJECT_SOURCE_DIR}/mysql-workbench.desktop.in mysql-workbench.desktop)

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

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/internal)
    install(FILES README-commercial.md License-commercial.txt DESTINATION ${WB_INSTALL_DOC_DIR}-commercial)
else()
    install(FILES README.md License.txt DESTINATION ${WB_INSTALL_DOC_DIR}-community)
endif()
install(FILES build/mysql-workbench-commercial.sharedmimeinfo DESTINATION ${WB_INSTALL_SHARED_DIR}/mime/packages RENAME mysql-workbench.xml)
install(FILES build/mysql-workbench-commercial.mime DESTINATION ${WB_INSTALL_SHARED_DIR}/mime-info RENAME mysql-workbench.mime)


install(FILES build/build_freetds.sh
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
        DESTINATION ${WB_PACKAGE_SHARED_DIR}/extras)

install(FILES samples/models/sakila_full.mwb
        DESTINATION ${WB_PACKAGE_SHARED_DIR}/extras)
       
