set(PY_FILES
    sql_reformatter.py
    sqlide_catalogman_ext.py
    sqlide_tableman_ext.py
    sqlide_grt.py
    sqlide_resultset_ext.py
    sqlide_schematree_ext.py
    text_output.py
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
