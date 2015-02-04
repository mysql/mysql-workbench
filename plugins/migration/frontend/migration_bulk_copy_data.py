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
        return "LOAD DATA INFILE '%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '';" % (path_to_file, table['target_table'])

    def get_script_ext(self): 
        return 'cmd'

    def generate_import_script(self, connection_args, path_to_file, schema_name):
        output = ['@ECHO OFF']
        output.append('echo Started load data. Please wait.')
        
        output.append('SET MYPATH=%%~dp0')
        output.append("SET command=mysql.exe -h127.0.0.1 -P%s -u%s -p -s -N information_schema -e \"SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = 'datadir'\"" % (connection_args['target_port'], connection_args['target_user']))
        
        output.append('FOR /F "tokens=* USEBACKQ" %%%%F IN ^(^`%%command%%^`^) DO ^(')
        output.append('    SET DADADIR=%%%%F')
        output.append('^)')
        output.append('cd %%DADADIR%%')

        output.append('xcopy %%%%MYPATH%%%%*.csv %s' % schema_name)
        output.append('xcopy %%%%MYPATH%%%%*.sql %s' % schema_name)
        
        
        output.append('mysql.exe -h127.0.0.1 -P%s -u%s -p ^< %s\%s' % (connection_args['target_port'], connection_args['target_user'], schema_name, path_to_file))
        output.append('echo Finished load data')
        output.append('pause')
        return output



class ImportScriptLinux(ImportScript):
    def get_import_cmd(self, table, path_to_file):
        return "LOAD DATA INFILE '%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '';" % (path_to_file, table['target_table'])

    def get_script_ext(self): 
        return 'sh'

    def generate_import_script(self, connection_args, path_to_file, schema_name):
        output = ['#!/bin/bash']
        output.append("TARGET_DIR=\`MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s -s -N information_schema -e 'SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = \\\"datadir\\\"'\`" % (connection_args['target_port'], connection_args['target_user']))
        output.append('cp *.csv \$TARGET_DIR/%s/' % schema_name)
        output.append('echo Started load data. Please wait.')
        output.append('MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s < %s' % (connection_args['target_port'], connection_args['target_user'], path_to_file))
        output.append('echo Finished load data')
        #output.append('read -p "Press [Enter] key to continue..."')
        return output



class ImportScriptDarwin(ImportScript):
    def get_import_cmd(self, table, path_to_file):
        return "LOAD DATA INFILE '%s' INTO TABLE %s FIELDS TERMINATED BY ',' ENCLOSED BY '';" % (path_to_file, table['target_table'])

    def get_script_ext(self): 
        return 'sh'

    def generate_import_script(self, connection_args, path_to_file, schema_name):
        output = ['#!/bin/bash']
        output.append("TARGET_DIR=\`MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s -s -N information_schema -e 'SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = \\\"datadir\\\"'\`" % (connection_args['target_port'], connection_args['target_user']))
        output.append('cp *.csv \$TARGET_DIR/%s/' % schema_name)
        output.append('echo Started load data. Please wait.')
        output.append('MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s < %s' % (connection_args['target_port'], connection_args['target_user'], path_to_file))
        output.append('echo Finished load data')
        #output.append('read -p "Press [Enter] key to continue..."')
        return output

    def get_basedir_cmd(self, connection_args):
        return "MYSQL_PWD=$arg_target_password mysql -h127.0.0.1 -P%s -u%s -s -N -p information_schema -e 'SELECT Variable_Value FROM GLOBAL_VARIABLES WHERE Variable_Name = \"basedir\"'" % (connection_args['target_port'], connection_args['target_user'])



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
        else:
            return None

    def get_copy_table_cmd(self, table):pass



class SourceRDBMSMssql(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        return 'bcp "SELECT %(select_expression)s FROM %(unquoted_source_schema)s.%(unquoted_source_table)s" queryout %(source_table)s.csv -c -t, -T -S .\%(source_instance)s -U %(source_user)s -P %%arg_source_passwords' % dict(table.items() + connection_args.items())



class SourceRDBMSMysql(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        if self.source_os == 'windows':
            return 'MYSQL_PWD=%%arg_source_password mysqldump.exe -h127.0.0.1 -P5615 -u%(source_user)s -t  -T. %(source_schema)s %(source_table)s --fields-terminated-by=\',\'' % dict(table.items() + connection_args.items())
        else:
            return 'MYSQL_PWD=$arg_source_password mysqldump -h127.0.0.1 -P5615 -u%(source_user)s -t  -T. %(source_schema)s %(source_table)s --fields-terminated-by=\',\'' % dict(table.items() + connection_args.items())



class SourceRDBMSPostgresql(SourceRDBMS):
    def get_copy_table_cmd(self, table, connection_args):
        return 'psql -U %(source_user)s -d %(source_schema)s -c "COPY %(source_table)s TO stdout DELIMITER \',\';" > %(source_table)s.csv' % dict(table.items() + connection_args.items())



class DataCopyScript:
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
        dir_name = 'dump_%s' % source_schema
        log_file = '%s.log' % dir_name
        import_file_name = 'import_%s.%s' % (source_schema, import_script.get_script_ext())
        import_sql_file_name = 'import_%s.sql' % source_schema

        f = open(script_path, 'w+')
        f.write('@ECHO OFF\r\n')

        f.write("REM Source and target DB passwords\r\n")
        f.write("set arg_source_password=\"<put source password here>\"\r\n")
        f.write("set arg_target_password=\"<put source password here>\"\r\n")
        f.write("""
IF [%arg_source_password%] == [] (
    IF [%arg_target_password%] == [] (
        ECHO WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
    )
)
\r\n""")
    
        f.write('SET MYPATH=%~dp0\r\n')
        f.write('echo [%d %%%%] Creating directory %s\r\n' % (progress, dir_name))
        f.write('mkdir %s\r\n' % dir_name)
        f.write('cd %s\r\n' % dir_name)
    
        progress = progress + 1

        f.write('copy NUL %s\r\n' % import_sql_file_name)
        f.write('echo %s >> %s\r\n' % ('SET SESSION UNIQUE_CHECKS=0;', import_sql_file_name))
        f.write('echo %s >> %s\r\n' % ('SET SESSION FOREIGN_KEY_CHECKS=0;', import_sql_file_name))
        f.write('echo %s >> %s\r\n' % ('use %s;' % source_schema, import_sql_file_name))
        
        f.write('echo [%d %%%%] Start dumping tables\r\n' % (progress * 100 / total_progress))
     
        for table in tables:
            f.write('%s\r\n' % source_rdbms.get_copy_table_cmd(table, connection_args))
            #if isinstance(source_rdbms, SourceRDBMSMysql):
            #    f.write('mv %s.txt %s.csv\n' % (table['source_table'], table['source_table']))
            #    f.write('rm %s.sql\n' % table['source_table'])
            f.write('echo %s >> %s\r\n' % (import_script.get_import_cmd(table, '%s.csv' % (table['source_table'])), import_sql_file_name))
            progress = progress + 1
            f.write('echo [%d %%%%] Dumped table %s\r\n' % (progress * 100 / total_progress, table['source_table']))

        f.write('copy NUL %s\r\n' % import_file_name)
        import_file_lines = import_script.generate_import_script(connection_args, import_sql_file_name, source_schema)
        for line in import_file_lines:
            f.write('(echo %s) >> %s\r\n' % (line, import_file_name))

        progress = progress + 1
        f.write('echo [%d %%%%] Generated import script %s\r\n' % (progress * 100 / total_progress, import_file_name))
    
        f.write('cd ..\r\n')
        f.write('set TEMPDIR=%%MYPATH%%%s\r\n' % dir_name)
        f.write('echo Set objArgs = WScript.Arguments > _zipIt.vbs\r\n')
        f.write('echo InputFolder = objArgs(0) >> _zipIt.vbs\r\n')
        f.write('echo ZipFile = objArgs(1) >> _zipIt.vbs\r\n')
        f.write('echo CreateObject("Scripting.FileSystemObject").CreateTextFile(ZipFile, True).Write "PK" ^& Chr(5) ^& Chr(6) ^& String(18, vbNullChar) >> _zipIt.vbs\r\n')
        f.write('echo Set objShell = CreateObject("Shell.Application") >> _zipIt.vbs\r\n')
        f.write('echo Set source = objShell.NameSpace(InputFolder).Items >> _zipIt.vbs\r\n')
        f.write('echo objShell.NameSpace(ZipFile).CopyHere(source) >> _zipIt.vbs\r\n')
        f.write('echo wScript.Sleep 2000 >> _zipIt.vbs\r\n')
        f.write('CScript  _zipIt.vbs  %%TEMPDIR%%  %%MYPATH%%%s.zip\r\n' % dir_name)
    
        progress = progress + 1

        f.write('echo [%d %%%%] Zipped all files to %s.zip file\n' % (progress * 100 / total_progress, dir_name))
    
        f.write('echo Now you can copy %%MYPATH%%%s.zip file to target server and run import script.\r\n' % dir_name)
        f.write('pause\r\n')



class DataCopyScriptLinux(DataCopyScript):
    def generate(self, tables, connection_args, script_path, source_rdbms, import_script):
        progress = 0 
        total_progress = (3 + len(tables))
        source_schema = tables[0]['source_schema']
        dir_name = 'dump_%s' % source_schema
        log_file = '%s.log' % dir_name
        import_file_name = 'import_%s.%s' % (source_schema, import_script.get_script_ext())
        import_sql_file_name = 'import_%s.sql' % source_schema
    
        f = open(script_path, 'w+')
        os.chmod(script_path, 0700)
        f.write('#!/bin/bash\n\n')
        
        f.write("arg_source_password=\"<put source password here>\"\n")
        f.write("arg_target_password=\"<put target password here>\"\n")
        f.write("""
if [ -z "$arg_source_password" ] && [ -z "$arg_target_password" ] ; then
    echo WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
    exit 1
fi
""")


        f.write('echo [%d %%] Creating directory %s\n' % (progress, dir_name))
        f.write('mkdir %s\n' % dir_name)
        f.write('cd %s\n' % dir_name)

        progress = progress + 1

        f.write('echo "%s" > %s\n' % ('SET SESSION UNIQUE_CHECKS=0;', import_sql_file_name))
        f.write('echo "%s" >> %s\n' % ('SET SESSION FOREIGN_KEY_CHECKS=0;', import_sql_file_name))
        f.write('echo "%s" >> %s\n' % ('use %s;' % source_schema, import_sql_file_name))


        f.write('echo [%d %%] Start dumping tables\n' % (progress * 100 / total_progress))

        #load_files = []
        for table in tables:
            f.write('%s\n' % source_rdbms.get_copy_table_cmd(table, connection_args))
            if isinstance(source_rdbms, SourceRDBMSMysql):
                f.write('mv %s.txt %s.csv\n' % (table['source_table'], table['source_table']))
                f.write('rm %s.sql\n' % table['source_table'])
            f.write('echo "%s" >> %s\n' % (import_script.get_import_cmd(table, '%s.csv' % (table['source_table'])), import_sql_file_name))
            #load_files.append('%s.csv' % table['source_table'])
            progress = progress + 1
            f.write('echo [%d %%] Dumped table %s\n' % (progress * 100 / total_progress, table['source_table']))

        f.write('touch %s\n' % import_file_name)
        if isinstance(import_script, ImportScriptDarwin) or isinstance(import_script, ImportScriptLinux):
            f.write('chmod +x %s' % import_file_name)
        import_file_lines = import_script.generate_import_script(connection_args, import_sql_file_name, source_schema)
        for line in import_file_lines:
            f.write('echo "%s" >> %s\n' % (line, import_file_name))

        progress = progress + 1
        f.write('echo [%d %%] Generated import script %s\n' % (progress * 100 / total_progress, import_file_name))

        f.write('cd ..\n')
        f.write('zip -r %s.zip %s\n' % (dir_name, dir_name))

        progress = progress + 1
        f.write('echo [%d %%] Zipped all files to %s.zip file\n' % (progress * 100 / total_progress, dir_name))

        f.write('rm -rf %s\n' % dir_name)

        f.write('echo Now you can copy %s.zip file to target server, unzip it and run import script.\n' % dir_name)
        f.write('read -p "Press [Enter] key to continue..."')



class DataCopyScriptDarwin(DataCopyScript):
    def generate(self, tables, connection_args, script_path, source_rdbms, import_script):
        progress = 0 
        total_progress = (3 + len(tables))
        source_schema = tables[0]['source_schema']
        dir_name = 'dump_%s' % source_schema
        log_file = '%s.log' % dir_name
        import_file_name = 'import_%s.%s' % (source_schema, import_script.get_script_ext())
        import_sql_file_name = 'import_%s.sql' % source_schema
    
        f = open(script_path, 'w+')
        os.chmod(script_path, 0700)
        f.write('#!/bin/bash\n\n')
        
        f.write("arg_source_password=\"<put source password here>\"\n")
        f.write("arg_target_password=\"<put target password here>\"\n")
        f.write("""
if [ -z "$arg_source_password" ] && [ -z "$arg_target_password" ] ; then
    echo WARNING: Both source and target RDBMSes passwords are empty. You should edit this file to set them.
    exit 1
fi
""")


        f.write('echo [%d %%] Creating directory %s\n' % (progress, dir_name))
        f.write('mkdir %s\n' % dir_name)
        f.write('cd %s\n' % dir_name)

        progress = progress + 1

        f.write('echo "%s" > %s\n' % ('SET SESSION UNIQUE_CHECKS=0;', import_sql_file_name))
        f.write('echo "%s" >> %s\n' % ('SET SESSION FOREIGN_KEY_CHECKS=0;', import_sql_file_name))
        f.write('echo "%s" >> %s\n' % ('use %s;' % source_schema, import_sql_file_name))


        f.write('echo [%d %%] Start dumping tables\n' % (progress * 100 / total_progress))

        load_files = []
        for table in tables:
            f.write('%s\n' % source_rdbms.get_copy_table_cmd(table, connection_args))
            if isinstance(source_rdbms, SourceRDBMSMysql):
                f.write('mv %s.txt %s.csv\n' % (table['source_table'], table['source_table']))
                f.write('rm %s.sql\n' % table['source_table'])
            f.write('echo "%s" >> %s\n' % (import_script.get_import_cmd(table, '%s.csv' % (table['source_table'])), import_sql_file_name))
            load_files.append('%s.csv' % table['source_table'])
            progress = progress + 1
            f.write('echo [%d %%] Dumped table %s\n' % (progress * 100 / total_progress, table['source_table']))

        f.write('touch %s\n' % import_file_name)
        if isinstance(import_script, ImportScriptDarwin) or isinstance(import_script, ImportScriptLinux):
            f.write('chmod +x %s' % import_file_name)
        import_file_lines = import_script.generate_import_script(connection_args, import_sql_file_name, source_schema)
        for line in import_file_lines:
            f.write('echo "%s" >> %s\n' % (line, import_file_name))
    
        progress = progress + 1
        f.write('echo [%d %%] Generated import script %s\n' % (progress * 100 / total_progress, import_file_name))
    
        f.write('cd ..\n')
        f.write('zip -r %s.zip %s\n' % (dir_name, dir_name))
    
        progress = progress + 1
        f.write('echo [%d %%] Zipped all files to %s.zip file\n' % (progress * 100 / total_progress, dir_name))
        
        f.write('rm -rf %s\n' % dir_name)
    
        f.write('echo Now you can copy %s.zip file to target server, unzip it and run import script.\n' % dir_name)
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

