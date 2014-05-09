set(PY_FILES
    db_sybase_re_grt.py
    db_sybase_migration_grt.py
)

set(MODULE_DATA_FILES
    res/sybase_rdbms_info.xml
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})
