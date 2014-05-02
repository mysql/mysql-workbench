pushd ..\..\..\mysql-gui-win-res\bin
bison.exe -v -dl --output ..\..\trunk\library\sql-parser\source\windows\myx_sql_parser.tab.cc ..\..\trunk\library\sql-parser\source\myx_sql_parser.yy
popd

pause
