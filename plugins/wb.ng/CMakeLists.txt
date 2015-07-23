include_directories(.
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/modules/
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/backend/wbprivate
    ${SIGC++_INCLUDE_DIRS}
    ${GRT_INCLUDE_DIRS}
    ${MYSQLNG_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
)

add_library(wb.ng.wbp
 backend/register_plugin.cpp
 backend/ng_sheet.cpp
 backend/ngide.cpp
)

target_link_libraries(wb.ng.wbp wbscintilla ${MYSQLNG_LIBRARY} ${GRT_LIBRARIES} ${SIGC++_LIBRARIES})

set_target_properties(wb.ng.wbp PROPERTIES
                      PREFIX ""
                      VERSION   ${WB_VERSION}
                      SOVERSION ${WB_VERSION})

install(TARGETS wb.ng.wbp DESTINATION ${WB_PACKAGE_PLUGINS_DIR})
