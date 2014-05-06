set(PY_FILES
    db_sql92_re_grt.py
    db_sql92_migration_grt.py
)

set(MODULE_DATA_FILES
    res/sql92_rdbms_info.xml
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})
