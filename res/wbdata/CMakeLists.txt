set(DATA_FILES 
    data.db
    main_menu.xml
    dbquery_toolbar.xml
    default_toolbar.xml
    model_toolbar.xml
    model_diagram_toolbar.xml
    model_option_toolbar_layer.xml
    model_option_toolbar_note.xml
    model_option_toolbar_physical_relationship.xml
    model_option_toolbar_physical_routinegroup.xml
    model_option_toolbar_physical_table.xml
    model_option_toolbar_physical_view.xml
    model_option_toolbar_selection.xml
    paper_types.xml
    shortcuts.xml
    shortcuts_basic.xml
    shortcuts_physical.xml
    tools_toolbar.xml
    tools_toolbar_basic.xml
    tools_toolbar_physical.xml
    code_editor.xml
    table_templates.xml
)

install(FILES ${DATA_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/data)
