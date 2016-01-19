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

import mforms
import grt

from workbench.ui import WizardPage
from workbench.template import MiniTemplate





text_template = """
------------------------------------------------------------------------------------
MySQL Workbench Migration Wizard Report

Date: {{date}}
Source: {{sourceRdbmsName}} {{sourceRdbmsVersion}}
Target: {{targetRdbmsName}} {{targetRdbmsVersion}}
------------------------------------------------------------------------------------

I. Migration

1. Summary

Number of migrated schemas: {{#catalog.schemata}}
[[catalog.schemata]]
{{:#}}. {{name}}
Source Schema:   {{sourceName}}

- Tables:             {{#tables}}
- Triggers:           {{#triggers}}
- Views:              {{#views}}
- Stored Procedures:  {{#procedures}}
- Functions:          {{#functions}}
[[/catalog.schemata]]

2. Migration Issues
[[migrationLog]]  - {{refObject.name}}
    [[entries]][[?name]]{{entryType}}  {{name}}[[/name]][[/entries]]
[[/migrationLog]]

3. Object Creation Issues
[[creationLog]]  - {{refObject.name}}
    [[entries]][[?name]]{{entryType}}  {{name}}[[/name]][[/entries]]
[[/creationLog]]

4. Migration Details
[[catalog.schemata]]\
[[tables]]
4.{{:#}}. [[?name]]Table {{owner.name}}.{{name}} ({{sourceName}})
[[?comment]]{{comment}}[[/comment]]
Columns:
[[columns]]\
[[?name]]  - {{name}} {{formattedRawType}} {{flags}} {{defaultValue}}  [[?comment]]#{{comment}}[[/comment]]
[[!name]]column not migrated
[[/name]]\
[[/columns]]
Foreign Keys:
[[foreignKeys]]\
  - {{name}} ([[columns]]{{name}}[[/columns]]) ON {{referencedTable.name}} ([[referencedColumns]]{{name}}[[/referencedColumns]])
[[/foreignKeys]]
Indices:
[[indices]]\
  - {{name}} ([[columns]]{{referencedColumn.name}}[[?if|columnLength > 0]]({{columnLength}})[[/if]][[?needsep]], [[/needsep]][[/columns]])
[[/indices]]\
[[!name]]\
4.{{:#}} Table {{sourceName}} was not migrated
[[/name]]
[[/tables]]\
[[/catalog.schemata]]\

II. Data Copy

[[dataTransferLog]]  - {{logObject.name}}
    [[entries]]\
      [[?name]]{{entryType}}  {{name}}[[/name]]\
    [[/entries]]
[[/dataTransferLog]]

"""


class FinalReportView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, "Migration Report")

        self.main.add_wizard_page(self, "Report", "Migration Report")

        self._report = mforms.newTextBox(mforms.VerticalScrollBar)
        self.content.add(self._report, True, True)
        
        self.next_button.set_text("Finish")
        
        
    def page_activated(self, advancing):
        if advancing:
            self.generate_migration_report()
            if "GenerateBulkCopyScript" in self.main.plan.state.dataBulkTransferParams.keys():
                self.advanced_button.set_text("Open folder that contains generated script")
                self.advanced_button.show(True)
            else:
                self.advanced_button.show(False)
        WizardPage.page_activated(self, advancing)
        
        
    def generate_report_data(self):
        state = self.main.plan.state
        source_catalog = state.sourceCatalog
        target_catalog = state.targetCatalog

        def find_migrated_object(object, state):
            for log in state.migrationLog:
                if log.logObject == object:
                    return log.refObject
            return None

        def reportize_object(object, state):
            logEntries = []
            for log in state.migrationLog:
                if log.logObject == object:
                    logEntries = log.entries
                    break
            createEntries = []
            for log in state.creationLog:
                if log.logObject == object:
                    createEntries = log.entries
                    break
            o = {
            "migrationMessages" : [ {"type" : ["note", "warning", "error"][max(min(e.entryType, 2), 0)], "message" : e.name } for e in logEntries],
            "createMessages" : [ {"type" : ["note", "warning", "error"][max(min(e.entryType, 2), 0)], "message" : e.name } for e in createEntries]
            }
            if object:
                for member in object.__grtmembers__:
                    v = getattr(object, member)
                    if type(v) in (int, float, str, unicode):
                        o[member] = v
                    elif type(v) is grt.List:
                        if v.__contenttype__[0] in (grt.STRING, grt.INT):
                            o[member] = ", ".join(v)
                        else:
                            o[member] = [reportize_object(x, state) for x in v]
                    elif isinstance(v, grt.Object):
                        o[member] = v
                    else:
                        o[member] = v
            return o
  
        def reportize_table(table, state):
            migrated = find_migrated_object(table, state)
            obj = reportize_object(migrated, state)
            obj.update({
            "sourceName" : table.name,
            "columns" : [reportize_object(column, state) for column in migrated.columns] if migrated else [],
            "foreignKeys" : [reportize_object(fk, state) for fk in migrated.foreignKeys] if migrated else [],
            "indices" : [reportize_object(index, state) for index in migrated.indices] if migrated else [],
            })
            return obj

        def reportize_schema(schema, state):
            tschema = find_migrated_object(schema, state)
            schema_data = reportize_object(tschema, state)
            schema_data.update({
            "sourceName" : schema.name,
            "tables" : [reportize_table(table, state) for table in schema.tables],
            "triggers" : [reportize_object(trigger, state) for table in schema.tables for trigger in table.triggers],
            "views" : [reportize_object(view, state) for view in schema.views],
            "functions" : [reportize_object(func, state) for func in schema.routines if func.routineType == "FUNCTION"],
            "procedures" : [reportize_object(sp, state) for sp in schema.routines if sp.routineType == "PROCEDURE"],
            })
            return schema_data
            
        def reportize_log(log):
            return {
            "logObject" : {"name" : log.logObject.name },
            "refObject" : {"name" : log.refObject.name if log.refObject else "" },
            "entries" : [ {"entryType" : ["note", "warning", "error"][max(min(e.entryType, 2), 0)], "name" : e.name } for e in log.entries]
            }

        def reportize_transfer_log(log):
            return {
            "logObject" : {"name" : log.name },
            "entries" : [ {"entryType" : ["", "warning", "error"][max(min(e.entryType, 2), 0)], "name" : e.name } for e in log.entries]
            }

        import time
        report_data = {
        "date" : time.ctime(),
        "sourceRdbmsName" : state.sourceConnection.driver.owner.caption,
        "sourceRdbmsVersion" : "%s.%s.%s" % (state.sourceDBVersion.majorNumber, state.sourceDBVersion.minorNumber, state.sourceDBVersion.releaseNumber),
        "targetRdbmsName" : state.targetConnection.driver.owner.caption,
        "targetRdbmsVersion" : "%s.%s.%s" % (state.targetDBVersion.majorNumber, state.targetDBVersion.minorNumber, state.targetDBVersion.releaseNumber),
        "sourceServer" : state.sourceConnection.hostIdentifier,
        "targetServer" : state.targetConnection.hostIdentifier,
        
        "creationLog" : [reportize_log(o) for o in state.creationLog if len(o.entries) > 1],
        "migrationLog" : [reportize_log(o) for o in state.migrationLog if len(o.entries) > 1],
        "dataTransferLog" : [reportize_transfer_log(o) for o in state.dataTransferLog if len(o.entries) > 0],
        
        "catalog" : {
            "sourceName" : source_catalog.name,
            "name" : target_catalog.name,
            "schemata" : [reportize_schema(schema, state) for schema in source_catalog.schemata],
        }
        }
        return report_data

    def go_next(self):
        self.main.close()


    def generate_migration_report(self):
        report_data = self.generate_report_data()
    
        try:
            report = MiniTemplate(text_template).render(report_data)
        except Exception, exc:
            report = "Error generating report: %s" % exc
        self._report.set_value(report)


    def go_advanced(self):
        mforms.Utilities.reveal_file(self.main.plan.state.dataBulkTransferParams["GenerateBulkCopyScript"])
