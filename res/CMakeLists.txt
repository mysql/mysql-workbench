add_subdirectory(grt)
add_subdirectory(grtdata)
add_subdirectory(mysql.profiles)
add_subdirectory(scripts)
add_subdirectory(sqlidedata)
add_subdirectory(wbdata)

# Copy files from the snippets dir:
install(FILES "snippets/SQL DDL Statements.txt" DESTINATION ${WB_PACKAGE_SHARED_DIR}/snippets RENAME SQL_DDL_Statements.txt)
install(FILES "snippets/SQL DML Statements.txt" DESTINATION ${WB_PACKAGE_SHARED_DIR}/snippets RENAME SQL_DML_Statements.txt)
install(FILES "snippets/DB Management.txt" DESTINATION ${WB_PACKAGE_SHARED_DIR}/snippets RENAME DB_Management.txt)
