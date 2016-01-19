# Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
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

# import the wb module
from wb import DefineModule
# import the grt module
import grt
from workbench import db_utils
from workbench.exceptions import NotConnectedError
from workbench.utils import Version


ModuleInfo = DefineModule(name= "DbMySQLFE", author= "Oracle Corp.", version="1.0")


def apply_scripts_to_catalog(catalog, create_scripts, drop_scripts):
    def comment_out(obj, sql):
        if obj.commentedOut:
            sql = "-- "+"\n-- ".join(sql.split("\n"))
        return sql
    
    def apply_script(obj, sql, drop_sql=None, delim=";"):
        if drop_sql:
            sql = drop_sql+delim+"\n"+sql
        if obj.customData.get("migration:lock_temp_sql", False):
            if sql:
                obj.customData["migration:new_temp_sql"] = comment_out(obj, sql)
                obj.customData["migration:temp_sql_changed"] = sql != comment_out(obj, sql)
        else:
            if obj.customData.has_key("migration:new_temp_sql"):
                del obj.customData["migration:new_temp_sql"]
                del obj.customData["migration:temp_sql_changed"]
            if sql is not None:
                obj.temp_sql = comment_out(obj, sql)
            else:
                obj.temp_sql = "-- no script was generated for %s" % obj.name
    
    for schema in catalog.schemata:
        apply_script(schema, create_scripts.get(schema.__id__, None), drop_scripts.get(schema.__id__, None))
        for table in schema.tables:
            apply_script(table, create_scripts.get(table.__id__, None))
            for trigger in table.triggers:
                sql = create_scripts.get(trigger.__id__, None)
                if sql:
                    sql = "DELIMITER $$\n"+sql
                apply_script(trigger, sql, delim="$$")
        for view in schema.views:
            apply_script(view, create_scripts.get(view.__id__, None))
        for routine in schema.routines:
            sql = create_scripts.get(routine.__id__, None)
            if sql:
                sql = "DELIMITER $$\n"+sql        
            apply_script(routine, sql, delim="$$")


@ModuleInfo.export(grt.INT, grt.classes.db_mysql_Catalog, grt.classes.GrtVersion, grt.DICT)
def generateSQLCreateStatements(catalog, targetVersion, objectCreationParams):
    options = grt.Dict()
    options.update(objectCreationParams)

    #        options['GenerateDrops'] = 1
    #options["GenerateUse"] = 1
    #        options['GenerateCreateIndex'] = 1
    options['GenerateWarnings'] = 1
    
    options['UseOIDAsResultDictKey'] = 1
    if targetVersion:
        options['DBSettings'] = grt.modules.DbMySQL.getTraitsForServerVersion(targetVersion.majorNumber, targetVersion.minorNumber, targetVersion.releaseNumber)
    create_scripts = grt.modules.DbMySQL.generateSQLForDifferences(grt.classes.db_mysql_Catalog(), catalog, options)
    if objectCreationParams.get("KeepSchemata", False):
        drop_scripts = {}
    else:
        drop_scripts = grt.modules.DbMySQL.generateSQLForDifferences(catalog, grt.classes.db_mysql_Catalog(), options)
    apply_scripts_to_catalog(catalog, create_scripts, drop_scripts)
    
    preamble = getSchemaCreatePreamble(catalog, objectCreationParams)
    catalog.customData["migration:preamble"] = preamble

    postamble = getSchemaCreatePostamble(catalog, objectCreationParams)
    catalog.customData["migration:postamble"] = postamble

    return 0


@ModuleInfo.export(grt.STRING, grt.classes.db_mysql_Catalog, grt.DICT)
def getSchemaCreatePreamble(catalog, objectCreationParams):
    obj = grt.classes.db_DatabaseDdlObject()
    obj.name = "Preamble"
    obj.sqlDefinition = """SET FOREIGN_KEY_CHECKS = 0;"""
    obj.temp_sql = obj.sqlDefinition
    return obj

@ModuleInfo.export(grt.STRING, grt.classes.db_mysql_Catalog, grt.DICT)
def getSchemaCreatePostamble(catalog, objectCreationParams):
    obj = grt.classes.db_DatabaseDdlObject()
    obj.name = "Postamble"
    obj.sqlDefinition = """SET FOREIGN_KEY_CHECKS = 1;"""
    obj.temp_sql = obj.sqlDefinition
    return obj

_connections = {}

def get_connection(connection_object):
    if _connections.has_key(connection_object.__id__):
        return _connections[connection_object.__id__]
    else:
        raise NotConnectedError("No open connection to %s" % connection_object.hostIdentifier)


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.STRING)
def connect(connection, password):
    try:
        con = get_connection(connection)
        try:
            con.ping()
        except Exception:
            grt.send_info("Reconnecting to %s..." % connection.hostIdentifier)
            con.disconnect()
            con.connect()
            grt.send_info("Connection reestablished")
    except NotConnectedError:
        con = db_utils.MySQLConnection(connection, password=password)
        grt.send_info("Connecting to %s..." % connection.hostIdentifier)
        con.connect()
        grt.send_info("Connected")
        _connections[connection.__id__] = con
    return 1


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def disconnect(connection):
    if _connections.has_key(connection.__id__):
        _connections[connection.__id__].disconnect()
        del _connections[connection.__id__]
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection)
def isConnected(connection):
    return 1 if _connections.has_key(connection.__id__) else 0


def execute_script(connection, script, log):
    connection = get_connection(connection)

    ranges = grt.modules.MysqlSqlFacade.getSqlStatementRanges(script)
    for start, length in ranges:
        if grt.query_status():
            raise grt.UserInterrupt()
        statement = script[start:start+length]
        try:
            grt.send_info("Execute statement", statement)
            grt.log_debug3("DbMySQLFE", "Execute %s\n" % statement)
            connection.execute(statement)
        except db_utils.QueryError, exc:
            if log:
                entry = grt.classes.GrtLogEntry()
                entry.owner = log
                entry.name = str(exc)
                entry.entryType = 2
                log.entries.append(entry)
            grt.send_warning("%s" % exc)
            grt.log_error("DbMySQLFE", "Exception executing '%s': %s\n" % (statement, exc))
            return False
        except Exception, exc:
            if log:
                entry = grt.classes.GrtLogEntry()
                entry.owner = log
                entry.name = "Exception: " + str(exc)
                entry.entryType = 2
                log.entries.append(entry)
            grt.send_warning("Exception caught: %s" % exc)
            grt.log_error("DbMySQLFE", "Exception executing '%s': %s\n" % (statement, exc))
            return False

    if log:
        entry = grt.classes.GrtLogEntry()
        entry.owner = log
        entry.entryType = 0
        log.entries.append(entry)

    return True


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection)
def getSchemaNames(connection):
    """Returns a list of schemas for the given connection object."""

    names = []
    result = get_connection(connection).executeQuery("SHOW DATABASES")
    if grt.query_status():
        raise grt.UserInterrupt()
    while result and result.nextRow():
        names.append(result.stringByIndex(1))

    return names


@ModuleInfo.export(grt.LIST, grt.classes.db_mgmt_Connection, grt.STRING)
def getTableNames(connection, schema):
    """Returns a list of the tables in the given schema for the given connection object."""

    names = []
    result = get_connection(connection).executeQuery("SHOW TABLES FROM `%s`" % schema)
    if grt.query_status():
        raise grt.UserInterrupt()
    while result and result.nextRow():
        names.append(result.stringByIndex(1))

    return names


@ModuleInfo.export(grt.INT, grt.STRING, grt.classes.db_mysql_Catalog, grt.Dict)
def createScriptForCatalogObjects(path, catalog, objectCreationParams):
    """Create a CREATE script with the catalog objects. The catalog must have been previously processed
    with generateSQLCreateStatements(), so that the objects have their temp_sql attributes set with
    their respective SQL CREATE statements.
    """

    def object_heading(type, name):
        text = """
-- ----------------------------------------------------------------------------
-- %s %s
-- ----------------------------------------------------------------------------
""" % (type, name)
        return text


    import time
    file = open(path, "w+")
    file.write("""-- ----------------------------------------------------------------------------
-- MySQL Workbench Migration
-- Migrated Schemata: %s
-- Source Schemata: %s
-- Created: %s
-- Workbench Version: %s
-- ----------------------------------------------------------------------------

""" % (", ".join([s.name for s in catalog.schemata]), ", ".join([s.oldName for s in catalog.schemata]), time.ctime(), Version.fromgrt(grt.root.wb.info.version)))

    preamble = catalog.customData["migration:preamble"]
    if preamble and preamble.temp_sql:
        #file.write(object_heading("Preamble script", ""))
        file.write(preamble.temp_sql+"\n")

    for schema in catalog.schemata:
        file.write(object_heading("Schema", schema.name))
        file.write(schema.temp_sql+";\n")

        for table in schema.tables:
            file.write(object_heading("Table", "%s.%s" % (schema.name, table.name)))
            file.write(table.temp_sql+";\n")

        for view in schema.views:
            file.write(object_heading("View", "%s.%s" % (schema.name, view.name)))
            file.write(view.temp_sql+";\n")

        for routine in schema.routines:
            file.write(object_heading("Routine", "%s.%s" % (schema.name, routine.name)))
            file.write(routine.temp_sql)

        for table in schema.tables:
            for trigger in table.triggers:
                file.write(object_heading("Trigger", "%s.%s" % (schema.name, trigger.name)))
                file.write(trigger.temp_sql+";\n")

    postamble = catalog.customData["migration:postamble"]
    if postamble and postamble.temp_sql:
        #file.write(object_heading("Postamble script", ""))
        file.write(postamble.temp_sql+"\n")

    file.close()

    return 1


@ModuleInfo.export(grt.INT, grt.classes.db_mgmt_Connection, grt.classes.db_mysql_Catalog, grt.Dict, (grt.LIST, grt.classes.GrtLogObject))
def createCatalogObjects(connection, catalog, objectCreationParams, creationLog):
    """Create catalog objects in the server for the specified connection. The catalog must have been 
    previously processed with generateSQLCreateStatements(), so that the objects have their temp_sql 
    attributes set with their respective SQL CREATE statements.
    """

    def makeLogObject(obj):
        if creationLog is not None:
            log = grt.classes.GrtLogObject()
            log.logObject = obj
            creationLog.append(log)
            return log
        else:
            return None
    
    try:
        grt.send_progress(0.0, "Creating schema in target MySQL server at %s..." % connection.hostIdentifier)
        
        preamble = catalog.customData["migration:preamble"]
        grt.send_progress(0.0, "Executing preamble script...")
        execute_script(connection, preamble.temp_sql, makeLogObject(preamble))

        i = 0.0
        for schema in catalog.schemata:
            grt.begin_progress_step(i, i + 1.0 / len(catalog.schemata))
            i += 1.0 / len(catalog.schemata)

            if schema.commentedOut:
                grt.send_progress(1.0, "Skipping schema %s... " % schema.name)
                grt.end_progress_step()
                continue

            total = len(schema.tables) + len(schema.views) + len(schema.routines) + sum([len(table.triggers) for table in schema.tables])

            grt.send_progress(0.0, "Creating schema %s..." % schema.name)
            execute_script(connection, schema.temp_sql, makeLogObject(schema))

            tcount = 0
            vcount = 0
            rcount = 0
            trcount = 0
            o = 0
            for table in schema.tables:
                if table.commentedOut:
                    grt.send_progress(float(o) / total, "Skipping table %s.%s" % (schema.name, table.name))
                else:
                    grt.send_progress(float(o) / total, "Creating table %s.%s" % (schema.name, table.name))
                o += 1
                if not table.commentedOut and execute_script(connection, table.temp_sql, makeLogObject(table)):
                    tcount += 1

            for view in schema.views:
                if view.commentedOut:
                    grt.send_progress(float(o) / total, "Skipping view %s.%s" % (schema.name, view.name))
                else:
                    grt.send_progress(float(o) / total, "Creating view %s.%s" % (schema.name, view.name))
                o += 1
                if not view.commentedOut and execute_script(connection, view.temp_sql, makeLogObject(view)):
                    vcount += 1

            for routine in schema.routines:
                if routine.commentedOut:
                    grt.send_progress(float(o) / total, "Skipping routine %s.%s" % (schema.name, routine.name))
                else:
                    grt.send_progress(float(o) / total, "Creating routine %s.%s" % (schema.name, routine.name))
                o += 1
                if not routine.commentedOut and execute_script(connection, routine.temp_sql, makeLogObject(routine)):
                    rcount += 1

            for table in schema.tables:
                for trigger in table.triggers:
                    if trigger.commentedOut:
                        grt.send_progress(float(o) / total, "Skipping trigger %s.%s.%s" % (schema.name, table.name, trigger.name))
                    else:
                        grt.send_progress(float(o) / total, "Creating trigger %s.%s.%s" % (schema.name, table.name, trigger.name))
                    o += 1
                    if not trigger.commentedOut and execute_script(connection, trigger.temp_sql, makeLogObject(trigger)):
                        trcount += 1

            grt.send_info("Scripts for %i tables, %i views and %i routines were executed for schema %s" % (tcount, vcount, rcount, schema.name))
            grt.end_progress_step()

        postamble = catalog.customData["migration:postamble"]
        grt.send_progress(1.0, "Executing postamble script...")
        execute_script(connection, postamble.temp_sql, makeLogObject(postamble))

        grt.send_progress(1.0, "Schema created")
    except grt.UserInterrupt:
        grt.send_info("Cancellation request detected, interrupting schema creation.")
        raise
    
    return 1


@ModuleInfo.export(grt.classes.GrtVersion, grt.classes.db_mgmt_Connection)
def getServerVersion(connection):
    """Returns a GrtVersion instance containing information about the server version."""
    conn = get_connection(connection)
    if conn:
        result = conn.executeQuery("SHOW VARIABLES LIKE 'version'")
        if result and result.nextRow():
            import re
            p = re.match("([0-9]*)\.([0-9]*)\.([0-9]*)", result.stringByIndex(2))
            if p and p.groups():
                version = grt.classes.GrtVersion()
                ver_parts = [int(n) for n in p.groups()] + [0]*4
                version.majorNumber, version.minorNumber, version.releaseNumber, version.buildNumber = ver_parts[:4]
                return version
    return None

@ModuleInfo.export(grt.STRING, grt.classes.db_mgmt_Connection)
def getOS(connection):
    conn = get_connection(connection)
    if conn:
        result = conn.executeQuery("SELECT @@version_compile_os")
        if result and result.nextRow():
            compile_os = result.stringByIndex(1).lower()
            if 'linux' in compile_os:
                return 'linux'
            elif 'win' in compile_os:
                return 'windows'
            elif 'osx' in compile_os:
                return 'darwin'

    return None
