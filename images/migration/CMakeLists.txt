set(IMAGE_FILES 
        migration.png
        migration_background.png
	migration_check_current.png
	migration_check_done.png
	migration_check_open.png
	migration_check_point.png
	migration_icon_32x32.png
	migration_logo.png
	migration_title.png
)

install(FILES ${IMAGE_FILES} DESTINATION ${WB_PACKAGE_SHARED_DIR}/images)
