# Copyright (c) 2012, 2020, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

import grt
#from grt.modules import Workbench
import mforms
from wb import wbinputs
from wb_utils_grt import ModuleInfo

@ModuleInfo.plugin('wb.util.copySQLToClipboard', caption='Copy SQL to Clipboard', input= [wbinputs.objectOfClass('db.DatabaseObject')], groups= ['Catalog/Utilities', 'Menu/Objects'], accessibilityName="Copy SQL to Clipboard")
@ModuleInfo.export(grt.INT, grt.classes.GrtNamedObject)
def copySQLToClipboard(obj):
        script = []        
        # workaround until diff sql generator handles routine groups
        if isinstance(obj, grt.classes.db_RoutineGroup):
            for routine in obj.routines:
                script.append(grt.modules.DbMySQL.makeCreateScriptForObject(routine))
                script.append(';\n\n')
        else:
            script.append(grt.modules.DbMySQL.makeCreateScriptForObject(obj))

        grt.modules.Workbench.copyToClipboard(''.join(script))
        return 0
          
@ModuleInfo.plugin('wb.util.copyColumnNamesToClipboard', caption='Copy Column Names to Clipboard', input= [wbinputs.objectOfClass('db.Table')], groups= ['Catalog/Utilities', 'Menu/Objects'], accessibilityName="Copy Column Names to Clipboard")
@ModuleInfo.export(grt.INT, grt.classes.db_Table)
def copyColumnNamesToClipboard(table):
        data = ', '.join([column.name for column in table.columns])
        grt.modules.Workbench.copyToClipboard(data)
        return 0

@ModuleInfo.plugin('wb.util.copyTableListToClipboard', caption='Copy Table List to Clipboard', input= [wbinputs.currentCatalog()], groups= ['Catalog/Utilities', 'Menu/Catalog'], accessibilityName="Copy Table List to Clipboard")
@ModuleInfo.export(grt.INT, grt.classes.db_Catalog)
def copyTableListToClipboard(cat):    
    #insert = ['`'+schema.name+'`.`'+tbl.name+'`' for tbl in schema.tables for schema in cat.schemata ]
    insert = '' 
    for schema in cat.schemata:
        insert = insert + ', '.join(['`'+schema.name+'`.`'+tbl.name+'`' for tbl in schema.tables])
             
    grt.modules.Workbench.copyToClipboard(insert)
    return 0

def generateName(name_prefix, names_map):
     name_suffix = 0
     byte_a = ord('a')
     while True:
         for i in range(0, 25):
             next_name = ''
             if name_prefix == '':
                 next_name = chr(byte_a + i) + str(name_suffix)
             else:
                 next_name = name_prefix + str(name_suffix)
             
             if not next_name in names_map:
                 names_map[next_name] = 1
                 return next_name

         name_suffix = name_suffix + 1
         
@ModuleInfo.plugin('wb.util.obfuscateCatalog', caption='Obfuscate Object Names in Catalog', input= [wbinputs.currentModel()], groups= ['Catalog/Utilities', 'Menu/Utilities'], accessibilityName="Obsfuscate Object Names in Catalog")
@ModuleInfo.export(grt.INT, grt.classes.workbench_physical_Model)
def obfuscateCatalog(model):     
 
     result = mforms.Utilities.show_warning('Warning', '''This operation will change names of all schemata and tables in the model.
Would you like to also clear SQL code for views and stored procedures?
This action cannot be undone!''', 'Clear SQL', 'Cancel', 'Leave SQL')

     if result == mforms.ResultCancel:
         return 1

     clear_sql_code = (result == mforms.ResultOk)
     schemata_map = {}
     objects_map = {}
     
     def createFiguresMap(model):
         figures_map = {}
         for diagram in model.diagrams:        
             for figure in diagram.figures:
                 if hasattr(figure, 'table'):
                     figures_map[figure.table.__id__] = figure
                 elif hasattr(figure, 'view'):
                     figures_map[figure.view.__id__] = figure
                 elif hasattr(figure, 'routine'):
                     figures_map[figure.routine.__id__] = figure
                 elif hasattr(figure, 'routineGroup'):
                     figures_map[figure.routineGroup.__id__] = figure
         return figures_map
    
     figures_map = createFiguresMap(model)
     
     def renameFigure(obj, figures_map):
         if obj.__id__ in figures_map:
             figures_map[obj.__id__].name = obj.name
     
     def renameDbObject(obj, obj_map=objects_map, clear_sql_code=clear_sql_code, figures_map=figures_map):
         obj.name = generateName('', obj_map)
         if clear_sql_code and hasattr(obj, 'sqlBody'):
             obj.sqlBody = ''
         if clear_sql_code and hasattr(obj, 'sqlDefinition'):
             obj.sqlDefinition = ''
         renameFigure(obj, figures_map)                      
        
     for schema in model.catalog.schemata:
        schema.name = generateName('', schemata_map)        
        for table in schema.tables:
            renameDbObject(table)
        for view in schema.views:
            renameDbObject(view)
        for routine in schema.routines:
            renameDbObject(routine)
        for routineGroup in schema.routineGroups:
            renameDbObject(routineGroup)
            for routine in routineGroup.routines:
                renameDbObject(routine)
   
     return 0

@ModuleInfo.plugin('wb.util.prefixTables', caption='Give a Prefix to All Tables in Catalog', input= [wbinputs.currentCatalog()], groups= ['Catalog/Utilities', 'Menu/Catalog'], accessibilityName="Prefix Tables in Catalog")
@ModuleInfo.export(grt.INT, grt.classes.db_Catalog)
def prefixTables(cat):
     
     ret, prefix = mforms.Utilities.request_input("Give a Prefix to All Tables in Catalog", "Please specify the prefix:copy table ", "")
     
     if not ret:
         return 1

     for schema in cat.schemata:
         for tbl in schema.tables:
             tbl.name = prefix + tbl.name
             
     return 0             

@ModuleInfo.plugin('wb.util.changeStorageEngines', caption='Change the Storage Engine of All Tables', input= [wbinputs.currentCatalog()], groups= ['Catalog/Utilities', 'Menu/Catalog'], accessibilityName="Change Tables Storage Engine")
@ModuleInfo.export(grt.INT, grt.classes.db_Catalog)
def changeStorageEngines(cat):
     ret, new_engine = mforms.Utilities.request_input("Change the Storage Engine of All Tables", "Type the new storage engine name:", "")
     
     if not ret:
         return 1
 
     def getTableEngines():
        result = grt.root.wb.options.options['@db.mysql.Table:tableEngine/Items']
        items = [item.strip(' \t') for item in result.split(',')]
        return items

     # validate the engine name and fix its case
     engines = getTableEngines()
     engine_found = False
     for engine_name in engines:
        if engine_name.find(':') != -1:
            engine_name = engine_name[engine_name.find(':') + 1:]
        if new_engine.lower() == engine_name.lower():
            engine_found = True
            new_engine = engine_name

     if not engine_found:
        grt.modules.Workbench.confirm('Change Storage Engines', 'Invalid storage engine name: ' + new_engine)
        return 2

     for schema in cat.schemata:
         for tbl in schema.tables:
            tbl.tableEngine = new_engine
            
     return 0
