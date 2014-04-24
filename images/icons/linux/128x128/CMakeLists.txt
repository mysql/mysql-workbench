set(THEME_DIR ${WB_INSTALL_SHARED_DIR}/icons/hicolor)
set(SIZE 128x128)

set(MIME_ICONS 
    mimetypes/application-vnd.mysql-workbench-model.png
    mimetypes/application-vnd.mysql-workbench-plugin.png
)

install(FILES ${MIME_ICONS} DESTINATION ${THEME_DIR}/${SIZE}/mimetypes)

set(APP_ICONS 
    apps/mysql-workbench.png
)

install(FILES ${APP_ICONS} DESTINATION ${THEME_DIR}/${SIZE}/apps)
