set(PY_FILES
    db_sqlanywhere_re_grt.py
    db_sqlanywhere_migration_grt.py
    sqlanydbwrapper.py
)

set(MODULE_DATA_FILES
    res/sqlanywhere_rdbms_info.xml
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})
