set(PY_FILES
    db_sqlite_re_grt.py
    db_sqlite_migration_grt.py
)

set(MODULE_DATA_FILES
    res/sqlite_rdbms_info.xml
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})
