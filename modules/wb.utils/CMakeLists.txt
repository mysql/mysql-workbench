set(PY_FILES	
    code_utils_grt.py
    table_templates.py
    table_utils_grt.py
    text_grt.py
    wb_dev_utils_grt.py
    wb_utils_grt.py
    wb_model_utils.py
    wb_catalog_utils.py
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
