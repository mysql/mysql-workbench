set(PY_FILES
    sql_reformatter.py
    sqlide_catalogman_ext.py
    sqlide_tableman_ext.py
    sqlide_grt.py
    sqlide_resultset_ext.py
    sqlide_schematree_ext.py
    sqlide_import_spatial.py
    sqlide_power_import_export_be.py
    sqlide_power_export_wizard.py
    sqlide_power_import_wizard.py
    text_output.py
    run_script.py
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
