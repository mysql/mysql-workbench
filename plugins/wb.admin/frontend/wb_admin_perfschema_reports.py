# Copyright (c) 2013, 2016, Oracle and/or its affiliates. All rights reserved.
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
import json

import os
import sys
from workbench.log import log_error, log_debug

from wb_admin_perfschema import WbAdminPSBaseTab

from threading import Thread


unit_formatters = {
  "us"    : lambda x: "%.2f" % (x / 1000000.0),
  "ms"    : lambda x: "%.2f" % (x / 1000000000.0),
  "s"     : lambda x: "%.2f" % (x / 1000000000000.0),
  "h:m:s" : lambda x: "%i:%02i:%.02f" % ((int)(x / (60*60*1000000000000.0)), (int)(x / (60*1000000000000.0)) % 60, (x / 1000000000000.0)%60),

  "Bytes" : lambda x: "%.0f" % x,
  "KB": lambda x: "%.2f" % (x / 1000.0),
  "MB": lambda x: "%.2f" % (x / 1000000.0),
  "GB": lambda x: "%.2f" % (x / 1000000000.0),
}

time_units = ["us", "ms", "s", "h:m:s"]
byte_units = ["Bytes", "KB", "MB", "GB"]


known_column_types = {
  "Integer" :     (mforms.IntegerColumnType, None),
  "LongInteger" : (mforms.LongIntegerColumnType, None),
  "Float" :       (mforms.FloatColumnType, None),
  
  "Time" :        (mforms.NumberWithUnitColumnType, "us"),
  "Bytes" :       (mforms.NumberWithUnitColumnType, "Bytes"),

  "String" :      (mforms.StringColumnType, None),
  "StringLT" :    (mforms.StringLTColumnType, None),
  "NumberWithUnit" : (mforms.NumberWithUnitColumnType, None)
}


class PSHelperViewTab(mforms.Box):
    category = None
    caption = None

    def __init__(self, owner):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self._owner = owner
        
        self.set_spacing(8)
        if sys.platform == 'win32' or sys.platform == 'darwin':
            self.set_back_color("#FFFFFF")

        self._refresh = None
        self._busy = False
        self._tree = None
        self._title = None
        self._check_timeout = None
        self._wait_table = None


    def __del__(self):
        if self._check_timeout:
            mforms.Utilities.cancel_timeout(self._check_timeout)
            self._check_timeout = None

    def init_ui(self):
        if self._title:
            return
        
        if self._wait_table:
            self._pbar.stop()
            self._pbar = None
            self.remove(self._wait_table)
            self._wait_table = None

        self._title = mforms.newLabel(self.caption.encode("utf8"))
        self._title.set_style(mforms.BigBoldStyle)
        self.add(self._title, False, True)

        self._column_file = None
        
        if self.description:
            self._description = mforms.newLabel(self.description.encode("utf8"))
            self.add(self._description, False, True)
            
        self._tree = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors|mforms.TreeShowColumnLines)
        self._tree.set_selection_mode(mforms.TreeSelectMultiple)
        self._tree.add_column_resized_callback(self._tree_column_resized)
        c = 0

        self._hmenu = mforms.newContextMenu()
        self._hmenu.add_will_show_callback(self._header_menu_will_show)
        self._tree.set_header_menu(self._hmenu)

        self._column_types = []
        self._column_units = []
        self._column_names = []
        self._column_titles = []
        for i, (column, cname, ctype, length) in enumerate(self.get_view_columns()):
            unit = None
            if type(ctype) is tuple:
                ctype, unit = ctype
            unit = grt.root.wb.state.get("wb.admin.psreport:unit:%s:%i" % (self.view, i), unit)

            width = min(max(length, 40), 300)
            width = grt.root.wb.state.get("wb.admin.psreport:width:%s:%i" % (self.view, i), width)

            label = self.column_label(column)
            self._column_units.append(unit)
            self._column_names.append(cname)
            self._column_titles.append(label)
            self._column_types.append(ctype)

            if unit:
                self._tree.add_column(ctype, label + " (%s)" % unit, width, False)
            else:
                self._tree.add_column(ctype, label, width, False)
            c += 1
        self._tree.end_columns()
        self._tree.set_allow_sorting(True)
        self.add(self._tree, True, True)

        bbox = mforms.newBox(True)
        bbox.set_spacing(12)

        btn = mforms.newButton()
        btn.set_text("Export...")
        btn.add_clicked_callback(self.do_export)
        bbox.add(btn, False, True)

        btn = mforms.newButton()
        btn.set_text("Copy Selected")
        btn.add_clicked_callback(self.do_copy)
        bbox.add(btn, False, True)

        btn = mforms.newButton()
        btn.set_text("Copy Query")
        btn.add_clicked_callback(self.do_copy_query)
        bbox.add(btn, False, True)

        self._refresh = mforms.newButton()
        self._refresh.set_text("Refresh")
        self._refresh.add_clicked_callback(self.do_refresh)
        bbox.add_end(self._refresh, False, True)
        self.add(bbox, False, True)


    def get_query(self):
        return "SELECT * FROM `%s`.`%s`" % (self._owner.sys, self.view)
  
    def execute(self):
        return self._owner.ctrl_be.exec_query(self.get_query())


    def run_query(self):
        try:
            self.result = self.execute()
            error = None
        except Exception, e:
            error = str(e)
            log_error("Error executing '%s': %s\n" % (self.get_query(), error))


    def check_if_finished(self):
        self._check_timeout = None
        if self.result is None:
            return True
        self.run_query_finished(None)
        return False

    def do_refresh(self):
        self._refresh.set_text("Refresh")
        self._tree.show(True)
        self.refresh()


    def _get_node_values(self, node):
        row = []
        for col in range(len(self._column_types)):
            if self._column_types[col] in [mforms.IntegerColumnType, mforms.LongIntegerColumnType]:
                row.append(str(node.get_long(col)))
            elif self._column_types[col] in [mforms.FloatColumnType]:
                row.append(str(node.get_float(col)))
            else:
                row.append(node.get_string(col))
        return row
    
    def _fmt_node(self, node):
        return ", ".join(self._get_node_values(node))

    def do_export(self):
        chooser = mforms.FileChooser(mforms.SaveFile)
        chooser.set_title("Export Report")
        chooser.add_selector_option("format", "Format:", "CSV|csv")
        if chooser.run_modal():
            save_path = "%s.csv" % chooser.get_path() if not chooser.get_path().endswith(".csv") else chooser.get_path()
            with open(save_path, 'wb') as csvfile:
                try:
                    import csv
                    output = csv.writer(csvfile, quoting = csv.QUOTE_MINIMAL)
                    output.writerow([self.caption])
                    output.writerow(self._column_titles)
                    root = self._tree.root_node()
                    for r in range(root.count()):
                        node = root.get_child(r)
                        output.writerow(self._get_node_values(node))
                except Exception, e:
                    log_error("Error exporting PS report: %s\n" % e)
                    mforms.Utilities.show_error("Export Report", "Error exporting PS report.\n%s" % e, "OK", "", "")

    def do_copy_query(self):
        mforms.Utilities.set_clipboard_text(self.get_query())

    def do_copy(self):
        text = [", ".join(self._column_titles)]
        for node in self._tree.get_selection():
            text.append(self._fmt_node(node))
        mforms.Utilities.set_clipboard_text("\n".join(text))


    def refresh(self):
        if not self._tree:
            self._pbar = mforms.newProgressBar()
            self._pbar.set_indeterminate(True)
            self._pbar.start()
            self._pbar.set_size(400, -1)
        
            self._wait_table = mforms.newTable()
            self._wait_table.set_row_spacing(8)
            self._wait_table.set_row_count(2)
            self._wait_table.set_column_count(1)
            self._wait_table.set_padding(-1)
            self._wait_table.add(mforms.newLabel("Querying performance schema %s..." % self.caption.encode("utf8")), 0, 1, 0, 1, mforms.HFillFlag)
            self._wait_table.add(self._pbar, 0, 1, 1, 2, mforms.HFillFlag)
        
            self.add(self._wait_table, True, True)
        
        if not self._busy:
            self._busy = True
            if self._refresh:
                self._refresh.set_enabled(False)

            self.result = None
            self._thr = Thread(target=self.run_query)
            self._check_timeout = mforms.Utilities.add_timeout(1.0, self.check_if_finished)
            self._thr.start()


    def run_query_finished(self, error):
        result = self.result

        self._thr.join()
        self._thr = None

        self._busy = False
        if self._refresh:
            self._refresh.set_enabled(True)

        if error:
            if self._wait_table:
                self._pbar.stop()
                self._pbar = None
                self.remove(self._wait_table)
                self._wait_table = None
            mforms.Utilities.show_error("Error Executing Report Query", error, "OK", "", "")
            return
        self.init_ui()
        self._tree.clear()
        if result is not None:
            while result.nextRow():
                node = self._tree.add_node()
                for i, cname in enumerate(self._column_names):
                    try:
                        if self._column_types[i] == mforms.IntegerColumnType:
                            s = result.intByName(self._column_names[i])
                            node.set_long(i, s or 0)
                        elif self._column_types[i] == mforms.LongIntegerColumnType:
                            s = result.stringByName(self._column_names[i])
                            node.set_long(i, long(s) if s else 0)
                        elif self._column_types[i] == mforms.FloatColumnType:
                            unit = self._column_units[i]
                            node.set_float(i, result.floatByName(self._column_names[i]))
                        elif self._column_types[i] == mforms.NumberWithUnitColumnType:
                            unit = self._column_units[i]
                            if unit and unit_formatters[unit]:
                                formatter = unit_formatters[unit]
                                node.set_string(i, formatter(float(result.stringByName(self._column_names[i]))))
                            else:
                                s = result.stringByName(self._column_names[i])
                                if i == self._column_file and self._owner.instance_info.datadir:
                                    s = s.replace(self._owner.instance_info.datadir, "<datadir>")
                                node.set_string(i, s or "")
                        else:
                            s = result.stringByName(self._column_names[i])
                            if i == self._column_file and self._owner.instance_info.datadir:
                                s = s.replace(self._owner.instance_info.datadir, "<datadir>")
                            node.set_string(i, s or "")
                    except Exception, e:
                        import traceback
                        traceback.print_exc()
                        log_error("Error handling column %i (%s) of report for %s: %s\n" % (i, cname, self.view, e))

    def get_view_columns(self):
        result = self._owner.ctrl_be.exec_query("DESCRIBE `%s`.%s" % (self._owner.sys, self.view))
        columns = []
        if result is not None:
            while result.nextRow():
                dtype = result.stringByName("Type")
                name = result.stringByName("Field")
                if name.endswith("atency") or name.endswith("ead") or name.endswith("ritten"):
                    ctype = mforms.NumberWithUnitColumnType
                    length = 80
                elif dtype.lower().startswith("char") and "(" in dtype:
                    try:
                        length = int(dtype[dtype.find("(")+1:-1]) * 10
                    except Exception, e:
                        log_error("Error parsing datatype %s from PS view %s: %s\n" % (dtype, self.view, e))
                        length = 120
                    ctype = mforms.StringColumnType
                elif dtype.lower().startswith("varchar") and "(" in dtype:
                    try:
                        length = min(int(dtype[dtype.find("(")+1:-1]) * 10, 150)
                    except Exception, e:
                        log_error("Error parsing datatype %s from PS view %s: %s\n" % (dtype, self.view, e))
                        length = 120
                    ctype = mforms.StringColumnType
                elif dtype.lower().startswith("decimal"):
                    length = 50
                    ctype = mforms.IntegerColumnType
                elif dtype.lower().startswith("bigint"):
                    length = 50
                    ctype = mforms.LongIntegerColumnType
                else:
                    length = 80
                    ctype = mforms.StringColumnType
                columns.append((name, ctype, length))
                # do some special treatment for some known columns
                if name == "file":
                    self._column_file = len(columns)-1

        return columns


    def column_label(self, colname):
        return " ".join(s.capitalize() for s in colname.replace("_", " ").split(" "))

    def _header_menu_will_show(self, parent):
        column = self._tree.get_clicked_header_column()

        # This reset should onle be done when the Context Menu will be initially shown.
        # On submenus should be skipped or they will be shown on the top left corner of the UI
        if parent is None:
            self._hmenu.remove_all()

            item = self._hmenu.add_item_with_title("Set Display Unit", lambda: None, "change_unit")
            unit = self._column_units[column]
            if unit in time_units:
                for label in time_units:
                    i = item.add_item_with_title(label, lambda self=self, column=column, label=label: self._change_unit(column, label), label)
                    if unit == label:
                        i.set_checked(True)
            elif unit in byte_units:
                for label in byte_units:
                    i = item.add_item_with_title(label, lambda self=self, column=column, label=label: self._change_unit(column, label), label)
                    if unit == label:
                        i.set_checked(True)
            else:
                item.set_enabled(False)


    def _tree_column_resized(self, column):
        if column >= 0:
            width = self._tree.get_column_width(column)
            grt.root.wb.state["wb.admin.psreport:width:%s:%i" % (self.view, column)] = width

    def _change_unit(self, column, unit):
        self._tree.set_column_title(column, self._column_titles[column] + " (%s)" % unit)
        self._column_units[column] = unit
        grt.root.wb.state["wb.admin.psreport:unit:%s:%i" % (self.view, column)] = unit
        self.refresh()



js_column_types = {
  "Integer" : (mforms.IntegerColumnType, None),
  "LongInteger" : (mforms.LongIntegerColumnType, None),
  "LongInteger:ms" : (mforms.FloatColumnType, lambda x: x / 1000000.0),
  "LongInteger:s" : (mforms.FloatColumnType, lambda x: x / 1000000000.0),
  "Float" : (mforms.FloatColumnType, None),
  "Float:ms" : (mforms.FloatColumnType, lambda x: x / 1000000.0),
  "Float:s" : (mforms.FloatColumnType, lambda x: x / 1000000000.0),
  "String" : (mforms.StringColumnType, None),
  "StringLT" : (mforms.StringLTColumnType, None),
}

class JSSourceHelperViewTab(PSHelperViewTab):
    query = None
    view = None
    limit = None

    def __init__(self, owner, data):
        PSHelperViewTab.__init__(self, owner)

        self.category = data["category"]
        self.caption = data["caption"]
        self.description = data["description"]
        self.view = data["view"]
        self.query = "select * from sys.`%s`" % self.view
        if "limit" in data:
            self.query += " limit %s" % data["limit"]
        self.columns = []
        for label, name, type, width in data["columns"]:
            self.columns.append((label, name, known_column_types[type], width))


    def column_label(self, label):
        return label.encode("utf8")


    def get_query(self):
        return self.query


    def get_view_columns(self):
        if self.columns:
            return self.columns
        else:
            if not self.view:
                log_error("report '%s' is missing column list\n" % self.caption)
                return []
            return PSHelperViewTab.get_view_columns(self)



class WbAdminPerformanceSchema(WbAdminPSBaseTab):
    min_server_version = (5,6,6)
    
    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_performance", "Performance Reports", False)
    
    @classmethod
    def identifier(cls):
        return "admin_performance_reports"
    
    def __init__(self, ctrl_be, instance_info, main_view):
        WbAdminPSBaseTab.__init__(self, ctrl_be, instance_info, main_view)

        self._selected_report = None


    def create_ui(self):
        self.create_basic_ui("title_performance_reports.png", "Performance Reports")


        known_views = []
        res = self.ctrl_be.exec_query("show full tables from sys where Table_type='VIEW'")
        while res.nextRow():
            known_views.append(res.stringByIndex(1))

        self.content = mforms.newBox(True)
        self.content.set_spacing(12)

        self.tree = mforms.newTreeView(mforms.TreeDefault)
        self.tree.add_column(mforms.IconStringColumnType, "Report", 250, False)
        self.tree.end_columns()
        self.tree.set_size(250, -1)
        self.tree.add_changed_callback(self._report_selected)
        self.content.add(self.tree, False, True)

        self.tabview = mforms.newTabView(mforms.TabViewTabless)
        self.content.add(self.tabview, True, True)
        
        self.add(self.content, True, True)
        self.relayout() # force relayout for mac

        self.pages = []

        try:
            report_data = json.load(open(os.path.join(mforms.App.get().get_resource_path("sys"), "sys_reports.js")))
        except Exception, e:
            log_error("Error loading sys_reports.js: %s\n" % e)
            mforms.Utilities.show_error("Error Loading Report Definitions",
                                        "An error occurred loading file %s\n%s" % (os.path.join(mforms.App.get().get_resource_path("sys"), "sys_reports.js"), e),
                                        "OK", "", "")
            return
        category_labels = report_data["categories"]
        reports = report_data["reports"]

        prev = None
        parent = None
        for report in reports:
            view = report["view"]
            if view not in known_views:
                log_debug("View `%s` not in sys, skipping report\n" % view)
                continue
            else:
                known_views.remove(view)

            try:
                tab = JSSourceHelperViewTab(self, report)
            except Exception, e:
                log_error("Error processing PS report definition %s:\n%s\n" % (e, report))
                continue
            setattr(self, "tab_"+tab.caption, tab)
            self.pages.append(tab)
            self.tabview.add_page(tab, str(tab.caption))

            if tab.category != prev:
                if parent:
                    parent.expand()
                prev = tab.category
                parent = self.tree.add_node()
                parent.set_string(0, category_labels.get(tab.category, tab.category))

            node = parent.add_child()
            node.set_string(0, tab.caption)
            node.set_tag(tab.caption)
        if parent:
            parent.expand()

        print "The following views are not handled", set([v for v in known_views if not v[0]=='-' and not v.endswith("_raw")]) - set(["wbversion", "version"])


    def refresh(self):
        self._report_selected()
    

    def _report_selected(self):
        node = self.tree.get_selected_node()
        if node:
            tag = node.get_tag()
            if self._selected_report == tag:
                return
            self._selected_report = tag
            if tag:
                i = 0
                for tab in self.pages:
                    if tab.caption == tag:
                        self.tabview.set_active_tab(i)
                        tab.refresh()
                        break
                    i += 1
