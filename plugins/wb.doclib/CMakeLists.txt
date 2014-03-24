set(PY_FILES 
    mysqldoclib.py
    wb_doclib_grt.py
)

set(SHARED_FILES
    res/DocLibrary/mysqldoclib.sqlite
    res/DocLibrary/mysqldoclib_webui.sqlite
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})

# Check if the documentation data will be installed
if (EXISTS ${CMAKE_SOURCE_DIR}/plugins/wb.doclib/res/DocLibrary/mysqldoclib.sqlite)
  MESSAGE("Install DocLib")
  install(FILES ${SHARED_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/modules/data/DocLibrary)
else()
  MESSAGE("Skipping installation of DocLib data")
endif()
