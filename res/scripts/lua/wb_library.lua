-- Lua Utility Library for MySQL Workbench 5.1
-- Copyright (c) 2008, 2009 Sun Microsystems, Inc
-- 
-- This library contains various functions for writing
-- scripts and plugins for MySQL Workbench.
-- It is licensed under the Lesser General Public License, LGPL.
--
-- 2008-11-07 Initial Version 
-- Author: Alfredo Kojima <alfredo.kojima@sun.com>
--
--


-- Plugin registration
-------------------------------------------------------------------------------

--- Create a list of plugins to be returned by getPluginInfo
function new_plugin_list()
  return grtV.newList("object", "app.Plugin")
end

--- Create a plugin object with the values in the supplied table
function new_plugin(params)
      local plugin
      plugin= grtV.newObj("app.Plugin", {
                name= params.name,
                caption= params.caption,
                moduleName= params.moduleName,

                pluginType= params.pluginType,
                moduleFunctionName= params.moduleFunctionName,
                rating= 100,                                  
                showProgress= 0
      })

      for k,arg in pairs(params.inputValues) do
        arg.owner= plugin
        grtV.insert(plugin.inputValues, arg)
      end
      
      for i,group in pairs(params.groups) do
        group= params.groups[i]
        grtV.insert(plugin.groups, group)
      end
        
      return plugin
end


--- Create a plugin input/argument descriptor for object arguments
function plugin_arg_object(type)
        return grtV.newObj("app.PluginObjectInput", {objectStructName= type})
end

--- Create a plugin input/argument descriptor for the current catalog
function plugin_arg_current_catalog()
        return grtV.newObj("app.PluginObjectInput", {objectStructName= "db.Catalog"})
end

--- Create a plugin input/argument descriptor for the current diagram
function plugin_arg_current_diagram()
        return grtV.newObj("app.PluginObjectInput", {objectStructName= "workbench.physical.Diagram"})
end

--- Create a plugin input/argument descriptor for the current physical model
function plugin_arg_current_model()
        return grtV.newObj("app.PluginObjectInput", {objectStructName= "workbench.physical.Model"})
end




-- Accessing default objects
-------------------------------------------------------------------------------

--- Return the model object for the current document
-- Returns: GRT object of type workbench.physical.Model
function get_default_model()
	return grtV.getGlobal("/wb/doc/physicalModels/0")
end


--- Return the default DB object catalog for the current document
-- Returns: GRT object of type db.Catalog (db.mysql.Catalog)
function get_default_catalog()
	return get_default_model().catalog
end

--- Return the list of schemata for the current document
-- Returns: GRT list with objects of type db.Schema
function get_default_schemata()
	return get_default_catalog().schemata
end

--- Return the number of diagrams for the current document
-- Returns: int number of diagrams
function count_diagrams()
	return grtV.getn(get_default_model().views)
end


--- Return the diagram for the current document at the given index
-- Arguments:
--   diagram_index int a 1 based index for the wanted diagram
-- Returns: GRT object of type workbench.physical.View
function get_diagram(diagram_index)
	return get_default_model().views[diagram_index]
end


--- Return the number of schemata for the current document
-- Returns: int number of schemata
function count_schemata()
	return grtV.getn(get_default_catalog().schemata)
end


--- Return the schema for the current document at the given index
-- Arguments:
--   schema_index int a 1 based index for the wanted schema
-- Returns: GRT object of type db.Schema
function get_schema(schema_index)
	return get_default_schemata()[schema_index]
end


-- Locating specific catalog objects
-------------------------------------------------------------------------------

--- Find the schema with the given name
-- Arguments:
--   schema_name string the name of the wanted schema
-- Returns: GRT object of type db.Schema or nil if not found
function find_schema_named(schema_name)
	local c= count_schemata()
	local i
	local schemata= get_default_schemata()
	for i= 1, c do
		if schemata[i].name == schema_name then
			return schemata[i]
		end
	end
	return nil
end


--- Find the named table from the given schema 
-- Arguments:
--   schema_object db.Schema object to search in
--   table_name string the name of the wanted table
-- Returns: GRT object of type db.Table or nil if not found
function find_table_named(schema_object, table_name)
	local tables= schema_object.tables
	local c= grtV.getn(tables)
	local i
	for i=1, c do
		if tables[i].name == table_name then
			return tables[i]
		end
	end
	return nil
end


--- Find the named column from the given table
-- Arguments:
--   table_object db.Table object to search in
--   column_name string the name of the wanted column
-- Returns: GRT object of type db.Column or nil if not found
function find_table_column_named(table_object, column_name)
	local columns= table_object.columns
	local c= grtV.getn(columns)
	local i
	for i=1, c do
		if columns[i].name == column_name then
			return columns[i]
		end
	end
	return nil
end


--- Finds tables that match the given regular expression in a schema
-- Arguments:
--   schema_object db.Schema object to search in
--   table_name_regex string with the regular expression of the name to match
-- Returns: lua table/list of objects of type db.Table
function find_tables_matching(schema_object, table_name_regex)
	local table_i
	local tables= schema_object.tables
	local table_count= grtV.getn(tables)
	local found_tables= {}

	for table_i=1, table_count do
		if string.match(tables[table_i].name, table_name_regex) ~= nil then
			table.insert(found_tables, tables[table_i])
		end
	end
	return found_tables
end


--- Finds tables that match the given regular expression in all schemata
-- Arguments:
--   table_name_regex string with the regular expression of the name to match
-- Returns: lua table/list of objects of type db.Table
function find_all_tables_matching(table_name_regex)
	local schema_count= count_schemas()
	local schema_i
	local table_count
	local table_i
	local tables
	local found_tables= {}
	
	for schema_i=1, schema_count do
		tables= get_schema(schema_i).tables
		table_count= grtV.getn(tables)
		for table_i=1, table_count do
			if string.match(tables[table_i].name, table_name_regex) ~= nil then
				table.insert(found_tables, tables[table_i])
			end
		end
	end
	return found_tables
end


-- Catalog object iterators
-------------------------------------------------------------------------------

--- Iterate through a list of objects calling the given callback
-- The callback is called as:
--   callback(list_value, user_data)
-- The iteration will continue as long as the callback returns nil,
-- as soon as any other value is returned the loop is interrupted and the
-- value from the callback is returned.
--
-- Arguments:
--   list GRT list of values to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_list_object(list, callback, user_data)
	local c= grtV.getn(list)
	local i
	local ret= nil
	for i= 1, c do
		ret= callback(list[i], user_data)
		if ret ~= nil then
			return ret
		end
	end
	return ret
end



--- Iterate through the list of db.Schemas calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_schema(callback, user_data)
	return foreach_list_object(get_default_schemata(), callback, user_data)
end


--- Iterate through the list of db.Tables in a schema calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   schema_object db.Schema object to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_table(schema_object, callback, user_data)
	return foreach_list_object(schema_object.tables, callback, user_data)
end


--- Iterate through the list of all db.Tables calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_table_all(callback, user_data)
	local schemata= get_default_schemata()
	local c= grtV.getn(schemata)
	local i
	local ret= nil
	for i= 1, c do
		ret= foreach_table(schemata[i], callback, user_data)
		if ret ~= nil then
			return ret
		end
	end
	return ret
end


--- Iterate through the list of db.Columns in a table calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   table_object db.Table with the db.Column's to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_table_column(table_object, callback, user_data)
	return foreach_list_object(table_object.columns, callback, user_data)
end


--- Iterate through the list of db.Views calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   schema_object db.Schema with the db.View's to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_view(schema_object, callback, user_data)
	return foreach_list_object(schema_object.views, callback, user_data)
end


--- Iterate through the list of db.Routines calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   schema_object db.Schema with the db.Routines's to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_routine(schema_object, callback, user_data)
	return foreach_list_object(schema_object.routines, callback, user_data)
end



--- Iterate through the list of db.RoutineGroups calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   schema_object db.Schema with the db.RoutineGroup's to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_routine_group(schema_object, callback, user_data)
	return foreach_list_object(schema_object.routineGroups, callback, user_data)
end


-- Diagram object iterators
-------------------------------------------------------------------------------

--- Iterate through the list of model.Views calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_diagram(callback, user_data)
	return foreach_list_object(get_default_model().views, callback, user_data)
end


--- Iterate through the list of model.Figures in a diagram calling the given callback
-- See foreach_list_object() for details.
--
-- Arguments:
--   diagram_object model.View to iterate
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_figure(diagram_object, callback, user_data)
	return foreach_list_object(diagram_object.figures, callback, user_data)
end


--- Iterate through the list of model.Figures in all diagrams
-- See foreach_list_object() for details.
--
-- Arguments:
--   callback a Lua function to call for each object in the list
--   user_data an arbitrary value to pass to the callback
-- Returns: Value returned by callback
function foreach_figure_all(callback, user_data)
	local diagrams= get_default_model().views
	local c= grtV.getn(diagrams)
	local i
	local ret= nil
	for i= 1, c do
		ret= foreach_figure(diagrams[i], callback, user_data)
		if ret ~= nil then
			return ret
		end
	end
	return ret
end


-- Misc. checks about objects
-------------------------------------------------------------------------------

--- Checks whether the given GRT object is a db.Schema
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_schema(object)
 	if grtV.isOrInheritsFrom("db.Schema", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a db.Table
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_table(object)
	if grtV.isOrInheritsFrom("db.Table", grtS.get(object)) then
		return true
	end
	return false
end

--- Checks whether the given GRT object is a db.Column
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_table_column(object)
	if grtV.isOrInheritsFrom("db.Column", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a db.View
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_view(object)
	if grtV.isOrInheritsFrom("db.View", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a db.Routine
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_routine(object)
	if grtV.isOrInheritsFrom("db.Routine", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a model.Figure
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_figure(object)
	if grtV.isOrInheritsFrom("model.Figure", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a table figure
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_table_figure(object)
	if grtV.isOrInheritsFrom("workbench.physical.TableFigure", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a view figure
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_view_figure(object)
	if grtV.isOrInheritsFrom("workbench.physical.ViewFigure", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given GRT object is a table figure
--- Arguments:
--   object GRT object
-- Returns: boolean
function is_routine_group_figure(object)
	if grtV.isOrInheritsFrom("workbench.physical.RoutineGroupFigure", grtS.get(object)) then
		return true
	end
	return false
end


--- Checks whether the given db.Column is a primary key in its table
--- Arguments:
--   column_object db.Column 
-- Returns: boolean
function is_column_a_primary_key(column_object)
	return DbUtils:isPrimaryKey(column_object.owner, column_object)
end


--- Checks whether the given db.Column is a foreign key in its table
--- Arguments:
--   column_object db.Column 
-- Returns: boolean
function is_column_a_foreign_key(column_object)
	return DbUtils:isForeignKey(column_object.owner, column_object)
end


--- Return the type of the db.Column formatted as a string
--- Arguments:
--   column_object db.Column 
-- Returns: string
function format_column_type(column_object)
	return DbUtils:getColumnDatatype(column_object)
end


--- Return SQL code for the given database object object
--- Arguments:
--   db_object a database object (db.Table, db.View, db.Routine, db.Schema)
-- Returns: string with SQL code
function get_object_sql(db_object)
	return MySQLModuleDbMySQL:makeCreateScriptForObject(db_object)
end


-- Mapping between catalog objects and figures
-------------------------------------------------------------------------------

--- Find the figure that represents the given object in a diagram
--- Arguments:
--   diagram_object model.View to look for the object
--   db_object a db.Table, db.View or db.RoutineGroup to search for 
-- Returns: model.Figure instance or nil if the object is not found
function find_figure_for_object(diagram_object, db_object)
	if is_table(db_object) then
		foreach_figure(diagram_object, function (figure, db_table) 
			if is_table_figure(figure) and figure.table == db_table then
				return figure
			end
		end, db_object)
	elseif is_view(db_object) then
		foreach_figure(diagram_object, function (figure, db_view) 
			if is_view_figure(figure) and figure.view == db_view then
				return figure
			end
		end, db_object)
	elseif is_routine_group(db_object) then
		foreach_figure(diagram_object, function (figure, db_rgroup) 
			if is_routine_group_figure(figure) and figure.routineGroup == db_rgroup then
				return figure
			end
		end, db_object)
	else
		error("Unhandled object type in find_figure_for_object()")
	end
	return nil
end


--- Return the db object that's represented by a diagram figure
-- Arguments:
--   figure_object model.Figure instance
-- Returns: a db.Table, db.View or db.RoutineGroup instance
function get_object_for_figure(figure_object)
	if is_table_figure(figure_object) then
		return figure_object.table
	elseif is_view_figure(figure_object) then
		return figure_object.view
	elseif is_routine_group_figure(figure_object) then
		return figure_object.routineGroup
	else
		error("Unhandled figure type in get_object_for_figure()")
	end
end


--[[ TODO
--
-- Object Manipulation
-------------------------------------------------------------------------------
--
-- WARNING: Directly modifying your model from scripts can lead to
-- inconsistencies and make it corrupt and unusable. 
-- Use these functions at your own risk and make sure to backup often
-- if you do use them. Plugins and scripts that do not make changes to the
-- model are safe.
--
-- A note about undo.
-- Undo information must be saved manually and carefully.
-- If some operation does not have a proper undo record, the undo stack 
-- will become inconsistent. Performing undo/redo in such cases will lead 
-- to a corrupt model. If you're not sure if your script/plugin saves
-- proper undo records, just save your model and restart Workbench before
-- calling undo. In any case, *always make backups*.
-- 
--

function begin_changes()
end


function end_changes(description)
end


function set_object_name()
end


function add_new_table()
end


function add_new_table_column()
end


function set_column_type()
end


function set_column_type_flag(column_object)
end


function set_column_primary_key(column_object, flag)
end

--]]
