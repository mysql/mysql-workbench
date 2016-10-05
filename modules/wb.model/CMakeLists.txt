if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_definitions(-DSCI_LEXER -DSCI_NAMESPACE)
endif()

set(TEXT_BASIC_FILES
    res/wb_model_reporting/Text_Basic.tpl/preview_basic.png
    res/wb_model_reporting/Text_Basic.tpl/report.txt.tpl
    res/wb_model_reporting/Text_Basic.tpl/info.xml
)
install(FILES ${TEXT_BASIC_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/wb_model_reporting/Text_Basic.tpl)

set(HTML_BASIC_FRAME_FILES
    res/wb_model_reporting/HTML_Basic_Frames.tpl/overview.html.tpl
    res/wb_model_reporting/HTML_Basic_Frames.tpl/basic.css.tpl
    res/wb_model_reporting/HTML_Basic_Frames.tpl/preview_basic.png
    res/wb_model_reporting/HTML_Basic_Frames.tpl/restrained.css.tpl
    res/wb_model_reporting/HTML_Basic_Frames.tpl/table_details.html.tpl
    res/wb_model_reporting/HTML_Basic_Frames.tpl/index.html.tpl
    res/wb_model_reporting/HTML_Basic_Frames.tpl/info.xml
)
install(FILES ${HTML_BASIC_FRAME_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/wb_model_reporting/HTML_Basic_Frames.tpl)

set(HTML_BASIC_SINGLE_FILES
    res/wb_model_reporting/HTML_Basic_Single_Page.tpl/basic.css.tpl
    res/wb_model_reporting/HTML_Basic_Single_Page.tpl/index.html.tpl
    res/wb_model_reporting/HTML_Basic_Single_Page.tpl/info.xml
)
install(FILES ${HTML_BASIC_SINGLE_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/wb_model_reporting/HTML_Basic_Single_Page.tpl)

set(HTML_DETAILED_FRAMES_FILES
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/basic.css.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/coated.css.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/index.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/info.xml
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/overview.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/overview_list.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/routine_details.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/table_details.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/table_details_list.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/table_element_details.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/top.html.tpl
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/view_details.html.tpl
)
install(FILES ${HTML_DETAILED_FRAMES_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/wb_model_reporting/HTML_Detailed_Frames.tpl)

set(HTML_DETAILED_FRAMES_IMAGES_FILES
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/images/logo.png
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/images/back.png
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/images/title-background.png
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/images/next.png
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/images/preview_coated.png
    res/wb_model_reporting/HTML_Detailed_Frames.tpl/images/preview_main.png
)
install(FILES ${HTML_DETAILED_FRAMES_IMAGES_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/wb_model_reporting/HTML_Detailed_Frames.tpl/images)

include_directories(.
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/backend/wbpublic
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/library/grt/src 
    ${PROJECT_SOURCE_DIR}/library/base
    ${PROJECT_SOURCE_DIR}/library/base/base
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/modules/interfaces
    ${PROJECT_SOURCE_DIR}/library/grt/src/diff
    ${PROJECT_SOURCE_DIR}/library/sql-parser/include
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source
    ${PROJECT_SOURCE_DIR}/library/sql-parser/source/linux
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/ext/scintilla/src
    ${PROJECT_SOURCE_DIR}/ext/scintilla/lexlib
    ${PROJECT_SOURCE_DIR}/ext
    ${PROJECT_SOURCE_DIR}/library/forms
    ${PROJECT_SOURCE_DIR}/modules/db.mysql.sqlide/src
    SYSTEM ${GTK3_INCLUDE_DIRS}
    SYSTEM ${PCRE_INCLUDE_DIRS}
    SYSTEM ${MySQL_INCLUDE_DIRS}
    SYSTEM ${GRT_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}
    SYSTEM ${Boost_INCLUDE_DIRS}
)

add_library(wb.model.grt
    src/reporting.cpp 
    src/wb_model.cpp
)

target_compile_options(wb.model.grt PUBLIC ${WB_CXXFLAGS})

target_link_libraries(wb.model.grt grt ${GRT_LIBRARIES} ${GTK3_LIBRARIES} ${PCRE_LIBRARIES} ${MYSQL_LIBRARIES})

if(BUILD_FOR_TESTS)
  target_link_libraries(wb.model.grt gcov)
endif()

set_target_properties(wb.model.grt
                      PROPERTIES PREFIX    ""
                                 VERSION   ${WB_VERSION}
                                 SOVERSION ${WB_VERSION})

if(COMMAND cotire)
    set_target_properties(wb.model.grt PROPERTIES
        COTIRE_PREFIX_HEADER_IGNORE_PATH "${PRECOMPILED_HEADERS_EXCLUDE_PATHS}")

  cotire(wb.model.grt)
endif()

install(TARGETS wb.model.grt DESTINATION ${WB_INSTALL_LIB_DIR}/modules)
