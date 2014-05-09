set(PY_FILES
    explain_renderer.py
    optimizer_trace_renderer.py
    performance_charting.py
    wb_query_analysis_grt.py
)

install(FILES ${PY_FILES} DESTINATION ${WB_PYTHON_MODULES_DIR})
