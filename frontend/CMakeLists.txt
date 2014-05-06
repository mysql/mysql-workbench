add_definitions(${USE_BUNDLED_MYSQLDUMP})

if (UNIX)
	add_subdirectory(linux)
elseif(WIN32)
	add_subdirectory(windows)
elseif(APPLE)
	add_subdirectory(mac)
endif()
