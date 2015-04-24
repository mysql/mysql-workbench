# Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

import os


class ImportScript:
    def __init__(self):
        self.error_log_name = 'import_errors.log'
        
    @staticmethod
    def create(target_os):
        if target_os == 'windows':
            return ImportScriptWindows()
        elif target_os == 'linux':
            return ImportScriptLinux()
        elif target_os == 'darwin':
            return ImportScriptDarwin()
        else:
            return None

    def generate_import_script(self, connection_args, path_to_file, schema_name): pass

    def get_script_ext(self): pass

    def get_import_cmd(self, table, path_to_file): pass



class ImportScriptWindows(ImportScript):
    def get_import_cmd(self, table, path_to_file):
        return "LOAD DATA INFILE '%s_#####_import/%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '';" % (table['target_schema'], path_to_file, table['target_table'])

    def get_script_ext(self): 
        return 'cmd'

    def generate_import_script(self, connection_args, path_to_file, schema_name):
        output = ['@ECHO OFF']

        output.append('echo Started load data. Please wait.')
        
        output.append('SET MYPATH=%%~dp0')
        
        output.append('IF EXIST %%%%MYPATH%%%%%s del /F %%%%MYPATH%%%%%s' % (self.error_log_name, self.error_log_name))
        output.append("SET command=mysql.exe -h127.0.0.1 -P%s -u%s -p -s -N information_schema -e \"SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = 'datadir'\" 2^>^> %%%%MYPATH%%%%%s" % (connection_args['target_port'], connection_args['target_user'], self.error_log_name))

        output.append('FOR /F "tokens=* USEBACKQ" %%%%F IN ^(^`%%command%%^`^) DO ^(')
        output.append('    SET DADADIR=%%%%F')
        output.append('^)')

        output.append('if %%ERRORLEVEL%% GEQ 1 ^(')
        output.append('    echo Script has failed. See the log file for details.')
        output.append('    exit /b 1')
        output.append('^)')

        output.append('pushd %%DADADIR%%')

        output.append('mkdir %s_#####_import' % schema_name)
        
        output.append('xcopy %%%%MYPATH%%%%*.csv %s_#####_import\* 2^>^> %%%%MYPATH%%%%%s' % (schema_name, self.error_log_name))
        output.append('if %%ERRORLEVEL%% GEQ 1 ^(')
        output.append('    echo Script has failed. See the log file for details.')
        output.append('    exit /b 1')
        output.append('^)')
        
        output.append('xcopy %%%%MYPATH%%%%*.sql %s_#####_import\* 2^>^> %%%%MYPATH%%%%%s' % (schema_name, self.error_log_name))
        output.append('if %%ERRORLEVEL%% GEQ 1 ^(')
        output.append('    echo Script has failed. See the log file for details.')
        output.append('    exit /b 1')
        output.append('^)')
        
        
        output.append('mysql.exe -h127.0.0.1 -P%s -u%s -p ^< %s_#####_import\%s 2^>^> %%%%MYPATH%%%%%s' % (connection_args['target_port'], connection_args['target_user'], schema_name, path_to_file, self.error_log_name))
        output.append('if %%ERRORLEVEL%% GEQ 1 ^(')
        output.append('    echo Script has failed. See the log file for details.')
        output.append('    exit /b 1')
        output.append('^)')
        
        output.append('rmdir %s_#####_import /s /q' % schema_name)
        output.append('echo Finished load data')
        output.append('popd')
        output.append('pause')
        return output



class ImportScriptLinux(ImportScript):
    def get_import_cmd(self, table, path_to_file):
        return "LOAD DATA INFILE '%s_#####_import/%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '';" % (table['target_schema'], path_to_file, table['target_table'])

    def get_script_ext(self): 
        return 'sh'

    def generate_import_script(self, connection_args, path_to_file, schema_name):
        output = ['#!/bin/bash']
        output.append('MYPATH=\`pwd\`')
        
        output.append('if [ -f \$MYPATH/%s ] ; then' % self.error_log_name)
        output.append('    rm \$MYPATH/%s' % self.error_log_name) 
        output.append('fi')
        
        output.append("TARGET_DIR=\`MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s -s -N information_schema -e 'SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = \\\"datadir\\\"'\` 2>> \$MYPATH/%s" % (connection_args['target_port'], connection_args['target_user'], self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('pushd \$TARGET_DIR')
        
        output.append('mkdir %s_#####_import' % schema_name)
        
        output.append('cp \$MYPATH/*.csv %s_#####_import/ 2>> \$MYPATH/%s' % (schema_name, self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('cp \$MYPATH/*.sql %s_#####_import/ 2>> \$MYPATH/%s' % (schema_name, self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('echo Started load data. Please wait.')
        output.append('MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s < %s_#####_import/%s 2>> \$MYPATH/%s' % (connection_args['target_port'], connection_args['target_user'], schema_name, path_to_file, self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('echo Finished load data')
        
        output.append('rm -rf %s_#####_import' % schema_name)
        
        output.append('popd')
        #output.append('read -p Press [Enter] key to continue...')
        return output



class ImportScriptDarwin(ImportScript):
    def get_import_cmd(self, table, path_to_file):
        return "LOAD DATA INFILE '%s_#####_import/%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '';" % (table['target_schema'], path_to_file, table['target_table'])

    def get_script_ext(self): 
        return 'sh'

    def generate_import_script(self, connection_args, path_to_file, schema_name):
        output = ['#!/bin/bash']
        output.append('MYPATH=\`pwd\`')
        
        output.append('if [ -f \$MYPATH/%s ] ; then' % self.error_log_name)
        output.append('    rm \$MYPATH/%s' % self.error_log_name) 
        output.append('fi')
        
        output.append("TARGET_DIR=\`MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s -s -N information_schema -e 'SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = \\\"datadir\\\"'\` 2>> \$MYPATH/%s" % (connection_args['target_port'], connection_args['target_user'], self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('pushd \$TARGET_DIR')
        
        output.append('mkdir %s_#####_import' % schema_name)
        
        output.append('cp \$MYPATH/*.csv %s_#####_import/ 2>> \$MYPATH/%s' % (schema_name, self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('cp \$MYPATH/*.sql %s_#####_import/ 2>> \$MYPATH/%s' % (schema_name, self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('echo Started load data. Please wait.')
        output.append('MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s < %s_#####_import/%s 2>> \$MYPATH/%s' % (connection_args['target_port'], connection_args['target_user'], schema_name, path_to_file, self.error_log_name))
        output.append('if [ \$? -ne 0 ];then')
        output.append('   echo Script has failed. See the log file for details.')
        output.append('   exit 1')
        output.append('fi')
        
        output.append('echo Finished load data')
        
        output.append('rm -rf %s_#####_import' % schema_name)
        
        output.append('popd')
        #output.append('read -p Press [Enter] key to continue...')
        return output



class SourceRDBMS:
    def __init__(self, source_os):
        self.source_os = source_os

    @staticmethod
    def create(source_rdbms, source_os):
        if source_rdbms == 'mssql':
            return SourceRDBMSMssql(source_os)
        elif source_rdbms == 'mysql':
            return SourceRDBMSMysql(source_os)
        elif source_rdbms == 'postgresql':
            return SourceRDBMSPostgresql(source_os)
        elif source_rdbms == 'sqlanywhere':
            return SourceRDBMSSqlAnywhere(source_os)
        else:
            return None

    def get_copy_table_cmd(self, table):pass



class SourceRDBMSMssql(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        return 'bcp "SELECT %(select_expression)s FROM %(unquoted_source_schema)s.%(unquoted_source_table)s" queryout %(source_table)s.csv -c -t, -T -S .\%(source_instance)s -U %(source_user)s -P %%arg_source_passwords' % dict(table.items() + connection_args.items())



class SourceRDBMSMysql(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        if self.source_os == 'windows':
            return 'mysqldump.exe --login-path=wb_migration_source -t --tab=. %(source_schema)s %(source_table)s --fields-terminated-by=,' % dict(table.items() + connection_args.items())
        else:
            return 'MYSQL_PWD=$arg_source_password mysqldump -h127.0.0.1 -P%(source_port)s -u%(source_user)s -t  --tab=/tmp %(source_schema)s %(source_table)s --fields-terminated-by=\',\'' % dict(table.items() + connection_args.items())

    def get_cfg_editor_cmd(self, connection_args):
        if self.source_os == 'windows':
            return 'mysql_config_editor.exe set --login-path=wb_migration_source -h127.0.0.1 -P%(source_port)s -u%(source_user)s -p' % connection_args


class SourceRDBMSPostgresql(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        return 'psql -U %(source_user)s -d %(source_schema)s -c "COPY %(source_table)s TO stdout DELIMITER \',\';" > %(source_table)s.csv' % dict(table.items() + connection_args.items())


class SourceRDBMSSqlAnywhere(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        if self.source_os == 'windows':
            return """dbisql.exe -c "DBN=%(source_schema)s;UID=%(source_user)s;PWD=%arg_source_password%" "SELECT * FROM %(source_table)s; OUTPUT TO '%(source_table)s.csv' FORMAT TEXT DELIMITED BY ',' QUOTE '';""" % dict(table.items() + connection_args.items())
        else:
            return """dbisql -c "DBN=%(source_schema)s;UID=%(source_user)s;PWD=$arg_source_password" "SELECT * FROM %(source_table)s; OUTPUT TO '%(source_table)s.csv' FORMAT TEXT DELIMITED BY ',' QUOTE '';""" % dict(table.items() + connection_args.items())



class DataCopyScript:
    def __init__(self):
        self.error_log_name = 'bulk_copy_errors.log'

    @staticmethod
    def create(source_os):
        if source_os == 'windows':
            return DataCopyScriptWindows()
        elif source_os == 'linux':
            return DataCopyScriptLinux()
        elif source_os == 'darwin':
            return DataCopyScriptDarwin()
        else:
            return None

    def generate(self, tables, script_path, source_rdbms):pass



class DataCopyScriptWindows(DataCopyScript):
    def generate(self, tables, connection_args, script_path, source_rdbms, import_script):
        progress = 0 
        total_progress = (3 + len(tables))
        source_schema = tables[0]['source_schema']
        target_schema = tables[0]['target_schema']
        dir_name = 'dump_%s' % source_schema
        log_file = '%s.log' % dir_name
        import_file_name = 'import_%s.%s' % (source_schema, import_script.get_script_ext())
        import_sql_file_name = 'import_%s.sql' % source_schema

        with open(script_path, 'wb+') as f:
            f.write('@ECHO OFF\r\n')
            f.write('SET MYPATH=%~dp0\r\n')
            f.write('IF EXIST %%MYPATH%%%s del /F %%MYPATH%%%s\r\n' % (self.error_log_name, self.error_log_name))
    
            if isinstance(source_rdbms, SourceRDBMSMysql):
                f.write('mysql_config_editor.exe remove --login-path=wb_migration_source 2>> "%%MYPATH%%%s"\r\n' % self.error_log_name)
                f.write('if %ERRORLEVEL% GEQ 1 (\r\n')
                f.write('    echo Script has failed. See the log file for details.\r\n')
                f.write('    exit /b 1\r\n')
                f.write(')\r\n')

                f.write('%s 2>> "%%MYPATH%%%s"\r\n' % (source_rdbms.get_cfg_editor_cmd(connection_args), self.error_log_name))
                f.write('if %ERRORLEVEL% GEQ 1 (\r\n')
                f.write('    echo Script has failed. See the log file for details.\r\n')
                f.write('    exit /b 1\r\n')
                f.write(')\r\n')
                
                f.write("SET command=mysql.exe -h127.0.0.1 -P%s -u%s -p -s -N information_schema -e \"SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = 'datadir'\" 2>> \"%%MYPATH%%%s\"\r\n" % (connection_args['source_port'], connection_args['source_user'], self.error_log_name))

                f.write('FOR /F "tokens=* USEBACKQ" %%F IN (`%command%`) DO (\r\n')
                f.write('    SET DADADIR=%%F\r\n')
                f.write(')\r\n')
        
                f.write('if %ERRORLEVEL% GEQ 1 (\r\n')
                f.write('    echo Script has failed. See the log file for details.\r\n')
                f.write('    exit /b 1\r\n')
                f.write(')\r\n')
            else:
                f.write("set arg_source_password=\"<put source password here>\"\r\n")
                f.write('SET DADADIR=%TEMP%\\\r\n')
    
            if not isinstance(import_script, ImportScriptWindows):
                f.write("set arg_target_password=\"<put target password here>\"\r\n")
    
            f.write('pushd %DADADIR%\r\n')
            
            f.write('echo [%d %%%%] Creating directory %s\r\n' % (progress, dir_name))
            f.write('mkdir %s\r\n' % dir_name)
            f.write('pushd %s\r\n' % dir_name)
        
            progress = progress + 1
    
            f.write('copy NUL %s\r\n' % import_sql_file_name)
            f.write('echo %s >> %s\r\n' % ('SET SESSION UNIQUE_CHECKS=0;', import_sql_file_name))
            f.write('echo %s >> %s\r\n' % ('SET SESSION FOREIGN_KEY_CHECKS=0;', import_sql_file_name))
            f.write('echo %s >> %s\r\n' % ('use %s;' % target_schema, import_sql_file_name))
            
            f.write('echo [%d %%%%] Start dumping tables\r\n' % (progress * 100 / total_progress))
         
            for table in tables:
                f.write('%s 2>> "%%MYPATH%%%s"\r\n' % (source_rdbms.get_copy_table_cmd(table, connection_args), self.error_log_name))
                f.write('if %ERRORLEVEL% GEQ 1 (\r\n')
                f.write('    echo Script has failed. See the log file for details.\r\n')
                f.write('    exit /b 1\r\n')
                f.write(')\r\n')
                if isinstance(source_rdbms, SourceRDBMSMysql):
                    f.write('rename %s.txt %s.csv\r\n' % (table['source_table'], table['source_table']))
                    f.write('del %s.sql\r\n' % table['source_table'])
                f.write('echo %s >> %s\r\n' % (import_script.get_import_cmd(table, '%s.csv' % (table['source_table'])), import_sql_file_name))
                progress = progress + 1
                f.write('echo [%d %%%%] Dumped table %s\r\n' % (progress * 100 / total_progress, table['source_table']))
    
            f.write('copy NUL %s\r\n' % import_file_name)
            import_file_lines = import_script.generate_import_script(connection_args, import_sql_file_name, target_schema)
            for line in import_file_lines:
                f.write('(echo %s) >> %s\r\n' % (line, import_file_name))
    
            progress = progress + 1
            f.write('echo [%d %%%%] Generated import script %s\r\n' % (progress * 100 / total_progress, import_file_name))
        
            f.write('popd\r\n')
            f.write('set TEMPDIR=%%DADADIR%%%s\r\n' % dir_name)
            f.write('echo Set fso = CreateObject("Scripting.FileSystemObject") > _zipIt.vbs\r\n')
            f.write('echo InputFolder = fso.GetAbsolutePathName(WScript.Arguments.Item(0)) >> _zipIt.vbs\r\n')
            f.write('echo ZipFile = fso.GetAbsolutePathName(WScript.Arguments.Item(1)) >> _zipIt.vbs\r\n')
            f.write('echo CreateObject("Scripting.FileSystemObject").CreateTextFile(ZipFile, True).Write "PK" ^& Chr(5) ^& Chr(6) ^& String(18, vbNullChar) >> _zipIt.vbs\r\n')
            f.write('echo Set objShell = CreateObject("Shell.Application") >> _zipIt.vbs\r\n')
            f.write('echo Set source = objShell.NameSpace(InputFolder).Items >> _zipIt.vbs\r\n')
            f.write('echo objShell.NameSpace(ZipFile).CopyHere(source) >> _zipIt.vbs\r\n')
            f.write('echo Do Until objShell.NameSpace( ZipFile ).Items.Count ^= objShell.NameSpace( InputFolder ).Items.Count >> _zipIt.vbs\r\n')
            f.write('echo wScript.Sleep 200 >> _zipIt.vbs\r\n')
            f.write('echo Loop >> _zipIt.vbs\r\n')
            
            f.write('CScript  _zipIt.vbs  "%%TEMPDIR%%"  "%%DADADIR%%%s.zip" 2>> "%%MYPATH%%%s"\r\n' % (dir_name, self.error_log_name))
            f.write('if %ERRORLEVEL% GEQ 1 (\r\n')
            f.write('    echo Script has failed. See the log file for details.\r\n')
            f.write('    exit /b 1\r\n')
            f.write(')\r\n')
        
            progress = progress + 1
    
            f.write('echo [%d %%%%] Zipped all files to %s.zip file\r\n' % (progress * 100 / total_progress, dir_name))
    
            f.write('xcopy %s.zip %%MYPATH%% 2>> "%%MYPATH%%%s"\r\n' % (dir_name, self.error_log_name))
            f.write('if %ERRORLEVEL% GEQ 1 (\r\n')
            f.write('    echo Script has failed. See the log file for details.\r\n')
            f.write('    exit /b 1\r\n')
            f.write(')\r\n')
            
            f.write('del %s.zip\r\n' % dir_name)    
            f.write('del _zipIt.vbs\r\n')
            f.write('del /F /Q %s\*.*\r\n' % dir_name)
            f.write('rmdir %s\r\n' % dir_name)
            f.write('popd\r\n')
        
            f.write('echo Now you can copy %%MYPATH%%%s.zip file to the target server and run the import script.\r\n' % dir_name)
            f.write('pause\r\n')



class DataCopyScriptLinux(DataCopyScript):
    def generate(self, tables, connection_args, script_path, source_rdbms, import_script):
        progress = 0 
        total_progress = (3 + len(tables))
        source_schema = tables[0]['source_schema']
        target_schema = tables[0]['target_schema']
        dir_name = 'dump_%s' % source_schema
        log_file = '%s.log' % dir_name
        import_file_name = 'import_%s.%s' % (source_schema, import_script.get_script_ext())
        import_sql_file_name = 'import_%s.sql' % source_schema
    
        with open(script_path, 'w+') as f:
            os.chmod(script_path, 0700)
            f.write('#!/bin/bash\n\n')
            f.write('MYPATH=`pwd`\n')
            
            f.write("arg_source_password=\"<put source password here>\"\n")
            f.write("arg_target_password=\"<put target password here>\"\n")
            f.write("""
    if [ -z "$arg_source_password" ] && [ -z "$arg_target_password" ] ; then
        echo WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
        exit 1
    fi
    """)

            f.write('if [ -f $MYPATH/%s ] ; then\n' % self.error_log_name)
            f.write('    rm $MYPATH/%s\n' % self.error_log_name) 
            f.write('fi\n')

            for table in tables:
                f.write('if [ -f /tmp/%s.txt ];\n' % table['source_table'])
                f.write('then\n')
                f.write('   rm /tmp/%s.txt 2> /dev/null\n' % table['source_table'])
                f.write('   if [ $? -ne 0 ];then\n')
                f.write('       echo "File /tmp/%s.txt already exists. You should remove file before running this script."\n' % table['source_table'])
                f.write('       exit 1\n')
                f.write('   fi\n')
                f.write('fi\n')
            
            f.write('pushd /tmp\n')
            f.write('echo [%d %%] Creating directory %s\n' % (progress, dir_name))
            f.write('mkdir %s\n' % dir_name)
            f.write('pushd %s\n' % dir_name)
    
            progress = progress + 1
    
            f.write('echo "%s" > %s\n' % ('SET SESSION UNIQUE_CHECKS=0;', import_sql_file_name))
            f.write('echo "%s" >> %s\n' % ('SET SESSION FOREIGN_KEY_CHECKS=0;', import_sql_file_name))
            f.write('echo "%s" >> %s\n' % ('use %s;' % target_schema, import_sql_file_name))
    
    
            f.write('echo [%d %%] Start dumping tables\n' % (progress * 100 / total_progress))
    
            for table in tables:
                f.write('%s 2>> $MYPATH/%s\n' % (source_rdbms.get_copy_table_cmd(table, connection_args), self.error_log_name))
                f.write('if [ $? -ne 0 ];then\n')
                f.write('   echo Script has failed. See the log file for details.\n')
                f.write('   exit 1\n')
                f.write('fi\n')
                
                if isinstance(source_rdbms, SourceRDBMSMysql):
                    f.write('cp /tmp/%s.txt %s.csv\n' % (table['source_table'], table['source_table']))
                    f.write('if [ -f /tmp/%s.sql ];\n' % table['source_table'])
                    f.write('then\n')
                    f.write('   rm /tmp/%s.sql\n' % table['source_table'])
                    f.write('fi\n')
                f.write('echo "%s" >> %s\n' % (import_script.get_import_cmd(table, '%s.csv' % (table['source_table'])), import_sql_file_name))
                progress = progress + 1
                f.write('echo [%d %%] Dumped table %s\n' % (progress * 100 / total_progress, table['source_table']))
    
            f.write('touch %s\n' % import_file_name)
            if isinstance(import_script, ImportScriptDarwin) or isinstance(import_script, ImportScriptLinux):
                f.write('chmod +x %s\n' % import_file_name)
            import_file_lines = import_script.generate_import_script(connection_args, import_sql_file_name, target_schema)
            for line in import_file_lines:
                f.write('echo "%s" >> %s\n' % (line, import_file_name))
    
            progress = progress + 1
            f.write('echo [%d %%] Generated import script %s\n' % (progress * 100 / total_progress, import_file_name))
    
            f.write('popd\n')
            f.write('zip -r %s.zip %s 2>> $MYPATH/%s\n' % (dir_name, dir_name, self.error_log_name))
            f.write('if [ $? -ne 0 ];then\n')
            f.write('   echo Script has failed. See the log file for details.\n')
            f.write('   exit 1\n')
            f.write('fi\n')
    
            progress = progress + 1
            f.write('echo [%d %%] Zipped all files to %s.zip file\n' % (progress * 100 / total_progress, dir_name))
    
            f.write('rm -rf %s\n' % dir_name)
            f.write('cp %s.zip $MYPATH 2>> $MYPATH/%s\n' % (dir_name, self.error_log_name))
            f.write('if [ $? -ne 0 ];then\n')
            f.write('   echo Script has failed. See the log file for details.\n')
            f.write('   exit 1\n')
            f.write('fi\n')
            
            f.write('popd\n')
    
            f.write('echo Now you can copy $MYPATH/%s.zip file to the target server, unzip it and run the import script.\n' % dir_name)
            f.write('read -p "Press [Enter] key to continue..."')



class DataCopyScriptDarwin(DataCopyScript):
    def generate(self, tables, connection_args, script_path, source_rdbms, import_script):
        progress = 0 
        total_progress = (3 + len(tables))
        source_schema = tables[0]['source_schema']
        target_schema = tables[0]['target_schema']
        dir_name = 'dump_%s' % source_schema
        log_file = '%s.log' % dir_name
        import_file_name = 'import_%s.%s' % (source_schema, import_script.get_script_ext())
        import_sql_file_name = 'import_%s.sql' % source_schema
    
        with open(script_path, 'w+') as f:
            os.chmod(script_path, 0700)
            f.write('#!/bin/bash\n\n')
            f.write('MYPATH=`pwd`\n')
            
            f.write("arg_source_password=\"<put source password here>\"\n")
            f.write("arg_target_password=\"<put target password here>\"\n")
            f.write("""
    if [ -z "$arg_source_password" ] && [ -z "$arg_target_password" ] ; then
        echo WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
        exit 1
    fi
    """)

            f.write('if [ -f $MYPATH/%s ] ; then\n' % self.error_log_name)
            f.write('    rm $MYPATH/%s\n' % self.error_log_name) 
            f.write('fi\n')

            for table in tables:
                f.write('if [ -f /tmp/%s.txt ];\n' % table['source_table'])
                f.write('then\n')
                f.write('   rm /tmp/%s.txt 2> /dev/null\n' % table['source_table'])
                f.write('   if [ $? -ne 0 ];then\n')
                f.write('       echo "File /tmp/%s.txt already exists. You should remove file before running this script."\n' % table['source_table'])
                f.write('       exit 1\n')
                f.write('   fi\n')
                f.write('fi\n')
            
            f.write('pushd /tmp\n')
            f.write('echo [%d %%] Creating directory %s\n' % (progress, dir_name))
            f.write('mkdir %s\n' % dir_name)
            f.write('pushd %s\n' % dir_name)
    
            progress = progress + 1
    
            f.write('echo "%s" > %s\n' % ('SET SESSION UNIQUE_CHECKS=0;', import_sql_file_name))
            f.write('echo "%s" >> %s\n' % ('SET SESSION FOREIGN_KEY_CHECKS=0;', import_sql_file_name))
            f.write('echo "%s" >> %s\n' % ('use %s;' % target_schema, import_sql_file_name))
    
    
            f.write('echo [%d %%] Start dumping tables\n' % (progress * 100 / total_progress))
    
            for table in tables:
                f.write('%s 2>> $MYPATH/%s\n' % (source_rdbms.get_copy_table_cmd(table, connection_args), self.error_log_name))
                f.write('if [ $? -ne 0 ];then\n')
                f.write('   echo Script has failed. See the log file for details.\n')
                f.write('   exit 1\n')
                f.write('fi\n')
                
                if isinstance(source_rdbms, SourceRDBMSMysql):
                    f.write('cp /tmp/%s.txt %s.csv\n' % (table['source_table'], table['source_table']))
                    f.write('if [ -f /tmp/%s.sql ];\n' % table['source_table'])
                    f.write('then\n')
                    f.write('   rm /tmp/%s.sql\n' % table['source_table'])
                    f.write('fi\n')
                f.write('echo "%s" >> %s\n' % (import_script.get_import_cmd(table, '%s.csv' % (table['source_table'])), import_sql_file_name))
                progress = progress + 1
                f.write('echo [%d %%] Dumped table %s\n' % (progress * 100 / total_progress, table['source_table']))
    
            f.write('touch %s\n' % import_file_name)
            if isinstance(import_script, ImportScriptDarwin) or isinstance(import_script, ImportScriptLinux):
                f.write('chmod +x %s\n' % import_file_name)
            import_file_lines = import_script.generate_import_script(connection_args, import_sql_file_name, target_schema)
            for line in import_file_lines:
                f.write('echo "%s" >> %s\n' % (line, import_file_name))
    
            progress = progress + 1
            f.write('echo [%d %%] Generated import script %s\n' % (progress * 100 / total_progress, import_file_name))
    
            f.write('popd\n')
            f.write('zip -r %s.zip %s 2>> $MYPATH/%s\n' % (dir_name, dir_name, self.error_log_name))
            f.write('if [ $? -ne 0 ];then\n')
            f.write('   echo Script has failed. See the log file for details.\n')
            f.write('   exit 1\n')
            f.write('fi\n')
    
            progress = progress + 1
            f.write('echo [%d %%] Zipped all files to %s.zip file\n' % (progress * 100 / total_progress, dir_name))
    
            f.write('rm -rf %s\n' % dir_name)
            f.write('cp %s.zip $MYPATH 2>> $MYPATH/%s\n' % (dir_name, self.error_log_name))
            f.write('if [ $? -ne 0 ];then\n')
            f.write('   echo Script has failed. See the log file for details.\n')
            f.write('   exit 1\n')
            f.write('fi\n')
            
            f.write('popd\n')
    
            f.write('echo Now you can copy $MYPATH/%s.zip file to the target server, unzip it and run the import script.\n' % dir_name)
            f.write('read -p "Press [Enter] key to continue..."')



class DataCopyFactory:
    def __init__(self, source_os, target_os, source_rdbms):
        self.source_rdbms = SourceRDBMS.create(source_rdbms, source_os)
        self.datacopy_script = DataCopyScript.create(source_os)
        self.import_script = ImportScript.create(target_os)

    def _unquoteIdentifier(self, identifier):
        if not '.' in identifier:
            return identifier[1:-1]
        else:
            return identifier[identifier.index('.') + 2:-1]

    def unquoteIdentifiers(self, tables):
        for table in tables:
            table['unquoted_source_schema'] = table['source_schema']
            table['unquoted_source_table'] = table['source_table']
            table['source_schema'] = self._unquoteIdentifier(table['source_schema'])
            table['target_schema'] = self._unquoteIdentifier(table['target_schema'])
            table['source_table'] = self._unquoteIdentifier(table['source_table'])
            table['target_table'] = self._unquoteIdentifier(table['target_table'])

    def generate(self, tables, connection_args, script_path):
        self.unquoteIdentifiers(tables)
        self.datacopy_script.generate(tables, connection_args, script_path, self.source_rdbms, self.import_script)

