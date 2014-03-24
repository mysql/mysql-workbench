set(PY_FILES 
    db_mssql_grt.py
    db_mssql_migration_grt.py
)

set(MODULE_DATA_FILES
    res/mssql_rdbms_info.xml
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})
