set(PY_FILES
    db_generic_migration_grt.py
    db_generic_re_grt.py
    SQLEXT.py
)

set(MODULE_DATA_FILES
    res/generic_rdbms_info.xml
    res/migration_generic_typemap.xml
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
install(FILES ${MODULE_DATA_FILES} DESTINATION ${WB_PYTHON_MODULES_DATA_DIR})
