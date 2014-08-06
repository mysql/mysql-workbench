set(PY_FILES 
    __init__.py
    change_tracker.py
    database_object_selector_widget.py
    database_schema_selector_widget.py
    db_driver.py
    db_utils.py
    tcp_utils.py
    exceptions.py
    notifications.py
    template.py
    ui.py
    utils.py
    plugins.py
    wizard_form.py
    wizard_page_widget.py
    wizard_progress_page_widget.py
    tcp_utils.py
    os_utils.py
    client_utils.py
    log.py
)

install(FILES ${PY_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/libraries/workbench)

set(PY_G_FILES
    graphics/__init__.py
    graphics/cairo_utils.py
    graphics/canvas.py
    graphics/charting.py
)

install(FILES ${PY_G_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/libraries/workbench/graphics)
