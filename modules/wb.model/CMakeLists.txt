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

add_library(wb.model.grt
    src/reporting.cpp 
    src/wb_model.cpp
)

target_include_directories(wb.model.grt
  PRIVATE 
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/modules
    ${PROJECT_SOURCE_DIR}/library
    ${PROJECT_SOURCE_DIR}/ext/scintilla/include
    ${PROJECT_SOURCE_DIR}/ext/scintilla/src
    ${PROJECT_SOURCE_DIR}/ext/scintilla/lexlib
)

if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_definitions(wb.model.grt PRIVATE -DSCI_LEXER -DSCI_NAMESPACE)
endif()

target_compile_options(wb.model.grt PRIVATE ${WB_CXXFLAGS})

target_link_libraries(wb.model.grt 
  PRIVATE 
    grt
    mforms
    parsers
    mtemplate
    wbpublic::wbpublic
)

if(BUILD_FOR_GCOV)
  target_link_libraries(wb.model.grt PRIVATE gcov)
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
