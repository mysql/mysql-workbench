

add_library(casmine SHARED
  ansi-styles.cpp
  casmine.cpp
  describe.cpp
  matchers.cpp
  reporter.cpp
  helpers.cpp
)

target_compile_definitions(casmine
	PRIVATE
		RAPIDJSON_HAS_STDSTRING
)

target_link_libraries(casmine
 Boost::boost)


target_include_directories(casmine
PRIVATE
    ${workbench_dir}/library
    ${workbench_dir}/library/base
    ${workbench_dir}/library/base/base
    SYSTEM ${GLIB_INCLUDE_DIRS}
    SYSTEM ${LIBXML2_INCLUDE_DIR}
)
