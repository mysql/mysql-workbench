# Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

from mforms import newTreeNodeView, newButton, newBox, newSelector, newCheckBox, newLabel, Utilities
import mforms
import grt

from functools import partial

from wb_common import dprint_ex
from wb_admin_utils import not_running_warning_label, weakcb, make_panel_header
import json

from workbench.log import log_error

class WBThreadStack(mforms.Form):
    enable_debug_info = False

    def __init__(self, ctrl_be, thread_id):
        mforms.Form.__init__(self, mforms.Form.main_form())
        self.set_title("Thread Stack for %d" % thread_id)
        self.ctrl_be = ctrl_be

        vbox = mforms.newBox(False)
        vbox.set_padding(20)
        vbox.set_spacing(18)

        self.thread_id = thread_id

        splitter = mforms.newSplitter(True, False)

        self.tree = mforms.newTreeNodeView(mforms.TreeDefault)
        self.tree.add_column(mforms.IntegerColumnType, "Event Id", 50, False)
        self.tree.add_column(mforms.StringColumnType, "Event info", 200, False)
        self.tree.add_column(mforms.StringColumnType, "Type", 100, False)
        self.tree.add_column(mforms.StringColumnType, "Timer wait [\xC2\xB5s]", 80, False)
        if self.enable_debug_info:
            self.tree.add_column(mforms.StringColumnType, "Source", 200, False)
        self.tree.end_columns()
        self.tree.set_size(400, -1)
        self.tree.add_changed_callback(self.event_selected)
        splitter.add(self.tree, 500);

        l = mforms.newLabel("Wait info")
        l.set_style(mforms.BoldStyle)
        tbox = newBox(False)
        lbox = newBox(False)
        lbox.set_spacing(5)
        lbox.set_padding(5)
        lbox.add(l, False, False)
        tbox.add(lbox, False, False)

        self.text = mforms.newTextBox(mforms.VerticalScrollBar)
        self.text.set_read_only(True)
        self.text.set_size(150, -1)

        tbox.add(self.text, True, True)
        splitter.add(tbox, 150)

        vbox.add(splitter, True, True)

        self.set_content(vbox)

        bbox = newBox(True)
        bbox.set_spacing(8)
        self.ok = newButton()
        self.ok.set_text("Close")
        self.ok.add_clicked_callback(self.close_form)
        bbox.add_end(self.ok, False, False)
        vbox.add_end(bbox, False, True)

        self.set_size(800, 600)
        self.center()

    def close_form(self):
        self.close()

    def event_selected(self):
        node = self.tree.get_selected_node()
        if node:
            tag = node.get_tag()
            if tag:
                self.text.set_value(tag)
            else:
                self.text.clear()

    def load_data(self):
        data = None
        try:
            result = self.ctrl_be.exec_query("SELECT sys.ps_thread_stack(%d, %s)" % (self.thread_id, "TRUE" if self.enable_debug_info else "FALSE"))
            if result is not None:
                if result.nextRow():
                    data = result.stringByIndex(1)
            if data:
                data = data.replace("\0", "") # workaround for server bug
                return self.parse_data(json.loads(data))
        except Exception, e:
            import traceback
            #open("/tmp/data.js", "w+").write(data)
            log_error("Exception during sys.ps_thread_stack(%d, %s):\n%s\n" % (self.thread_id, "TRUE" if self.enable_debug_info else "FALSE", traceback.format_exc()))
            mforms.Utilities.show_error("Error Getting Thread Stack", "The thread stack for thread %d can't be loaded, please check if your sys schema is properly installed and available.\n%s" % (self.thread_id, e), "OK",  "", "")
            return None
        return False

    def parse_data(self, datatree):
        treecache = {}
        if datatree.get('events') == None or len(datatree['events']) == 0:
            return False
        for item in datatree['events']:
            node = None
            if int(item['nesting_event_id']) == 0:
                node = self.tree.add_node()
                treecache['event_%d' % int(item['event_id'])] = node
            else: #Check if there is already node, if not, we create one
                if treecache.get('event_%d' % int(item['nesting_event_id'])):
                     parent = treecache.get('event_%d' % int(item['nesting_event_id']))
                     node = parent.add_child()
                     treecache['event_%d' % int(item['event_id'])] = node
                else:
                    continue

            node.set_string(0, item['event_id'])
            node.set_string(1, item['event_info'].strip())
            node.set_string(2, item['event_type'].strip())
            if 'timer_wait' in item:
                node.set_string(3, str(item['timer_wait']))
                pnode = node.get_parent()
                if pnode:
                    pnode_twait = 0.0
                    try:
                        pnode_twait = float(pnode.get_string(3))
                    except:
                        #Parent node can be empty
                        pass
                    pnode.set_string(3, str(item['timer_wait'] + pnode_twait))

            if self.enable_debug_info:
                node.set_string(4, item['source'].strip())
            if item['wait_info'] and item['wait_info'].strip() != item['source'].strip(): 
                node.set_tag(item['wait_info'])
        #Set the textbox text as the first node
        if self.tree.count():
            tag = self.tree.node_at_row(0).get_tag()
            if tag:
                self.text.set_value(tag)
        return True

    def run(self):
        r = self.load_data()
        if not r:
            if r is not None:
                mforms.Utilities.show_error("Error Getting Thread Stack", "Thread stack is not available for thread %d, please enable Performance Schema instrumentation (Statement and Stage instrumentations and respective History consumers)." % self.thread_id, "OK",  "", "")
            self.close()
        else:
            self.show()



class WbAdminConnections(mforms.Box):
    ui_created = False
    serial = 0

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_management", "Client Connections", False)

    @classmethod
    def identifier(cls):
        return "admin_connections"

    def __init__(self, ctrl_be, instance_info, main_view):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.set_padding(12)
        self.set_spacing(15)
        self.instance_info = instance_info
        self.ctrl_be = ctrl_be
        self.page_active = False
        self.main_view = main_view
        self._new_processlist = self.check_if_ps_available()
        self._refresh_timeout = None
        
        if self.new_processlist():
            self.columns = [("PROCESSLIST_ID", mforms.LongIntegerColumnType, "Id", 50),
                            ("PROCESSLIST_USER", mforms.StringColumnType, "User", 80),
                            ("PROCESSLIST_HOST", mforms.StringColumnType, "Host", 120),
                            ("PROCESSLIST_DB", mforms.StringColumnType, "DB", 100),
                            ("PROCESSLIST_COMMAND", mforms.StringColumnType, "Command", 80),
                            ("PROCESSLIST_TIME", mforms.LongIntegerColumnType, "Time", 60),
                            ("PROCESSLIST_STATE", mforms.StringColumnType, "State", 80),
                            ("THREAD_ID", mforms.LongIntegerColumnType, "Thread Id", 50),
                            ("TYPE", mforms.StringColumnType, "Type", 80),
                            ("NAME", mforms.StringColumnType, "Name", 80),
                            ("PARENT_THREAD_ID", mforms.LongIntegerColumnType, "Parent Thread", 50),
                            ("INSTRUMENTED", mforms.StringColumnType, "Instrumented", 80),
                            ("PROCESSLIST_INFO", mforms.StringColumnType, "Info", 80),
                            ]
            self.long_int_columns = [0, 5, 7, 10]
            self.info_column = 12
        else:
            self.columns = [("Id", mforms.LongIntegerColumnType, "Id", 50),
                            ("User", mforms.StringColumnType, "User", 80),
                            ("Host", mforms.StringColumnType, "Host", 120),
                            ("DB", mforms.StringColumnType, "DB", 100),
                            ("Command", mforms.StringColumnType, "Command", 80),
                            ("Time", mforms.LongIntegerColumnType, "Time", 60),
                            ("State", mforms.StringColumnType, "State", 80),
                            ("Info", mforms.StringColumnType, "Info", 80),
                            ]
            self.long_int_columns = [0, 5]
            self.info_column = 7

    def create_ui(self):
        dprint_ex(4, "Enter")
        self.suspend_layout()

        self.heading = make_panel_header("title_connections.png", self.instance_info.name, "Client Connections")
        self.add(self.heading, False, False)

        self.warning = not_running_warning_label()
        self.add(self.warning, False, True)

        if self.new_processlist():
            widths = grt.root.wb.state.get("wb.admin:ConnectionListColumnWidthsPS", None)
        else:
            widths = grt.root.wb.state.get("wb.admin:ConnectionListColumnWidths", None)
        if widths:
            column_widths = [int(i) for i in widths.split(",")]
        else:
            column_widths = None

        self.connection_list = newTreeNodeView(mforms.TreeDefault|mforms.TreeFlatList|mforms.TreeAltRowColors)
        self.connection_list.set_selection_mode(mforms.TreeSelectMultiple)
        self.connection_list.add_column_resized_callback(self.column_resized)
        for i, (field, type, caption, width) in enumerate(self.columns):
            if column_widths:
                width = column_widths[i]
            self.connection_list.add_column(type, caption, width, False)

        self.connection_list.end_columns()
        self.connection_list.set_allow_sorting(True)
        
        self.connection_list.add_changed_callback(weakcb(self, "connection_selected"))

        info_table = mforms.newTable()
        info_table.set_row_count(2)
        info_table.set_column_count(5)
        info_table.set_row_spacing(4)
        info_table.set_column_spacing(20)

        info_table.add(self.create_labeled_info("Threads Connected:", "lbl_Threads_connected"),                     0, 1, 0, 1, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Threads Running:", "lbl_Threads_running"),                          1, 2, 0, 1, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Threads Created:", "lbl_Threads_created"),                         2, 3, 0, 1, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Threads Cached:", "lbl_Threads_cached"),                           3, 4, 0, 1, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Rejected (over limit):", "lbl_Connection_errors_max_connections"), 4, 5, 0, 1, mforms.HFillFlag)

        info_table.add(self.create_labeled_info("Total Connections:", "lbl_Connections"),                           0, 1, 1, 2, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Connection Limit:", "lbl_max_connections"),                        1, 2, 1, 2, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Aborted Clients:", "lbl_Aborted_clients"),                         2, 3, 1, 2, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Aborted Connections:", "lbl_Aborted_connects"),                    3, 4, 1, 2, mforms.HFillFlag)
        info_table.add(self.create_labeled_info("Errors:", "lbl_errors", "tooltip_errors"),                         4, 5, 1, 2, mforms.HFillFlag)

        self.info_table = info_table
        self.add(info_table, False, True)

        #self.set_padding(8)
        self.add(self.connection_list, True, True)


        box = newBox(True)
        self.button_box = box
        self.add_end(box, False, True)

        box.set_spacing(12)
        
        refresh_button = newButton()
        refresh_button.set_text("Refresh")
        box.add_end(refresh_button, False, True)
        refresh_button.add_clicked_callback(weakcb(self, "refresh"))

        self.kill_button = newButton()
        self.kill_button.set_text("Kill Connection(s)")
        box.add_end(self.kill_button, False, True)
        self.kill_button.add_clicked_callback(weakcb(self, "kill_connection"))
        
        self.killq_button = newButton()
        self.killq_button.set_text("Kill Query(s)")
        box.add_end(self.killq_button, False, True)
        self.killq_button.add_clicked_callback(weakcb(self, "kill_query"))
        
        refresh_label = newLabel("Refresh Rate:")
        box.add(refresh_label, False, True)

        self._menu = mforms.newContextMenu()
        self._menu.add_will_show_callback(self.menu_will_show)
        self.connection_list.set_context_menu(self._menu)

        
        self.refresh_values = [0.5, 1, 2, 3, 4, 5, 10, 15, 30]
        self.refresh_values_size = len(self.refresh_values)
        
        self.refresh_selector = newSelector()
        self.refresh_selector.set_size(100,-1)
        
        for s in self.refresh_values:
            self.refresh_selector.add_item(str(s) + " seconds")
        
        self.refresh_selector.add_item("Don't Refresh")
        
        refresh_rate_index = grt.root.wb.options.options.get('Administrator:refresh_connections_rate_index', 9)
        self.refresh_selector.set_selected(refresh_rate_index)
        self.update_refresh_rate()
        self.refresh_selector.add_changed_callback(weakcb(self, "update_refresh_rate"))
        box.add(self.refresh_selector, False, True)

        self.check_box = newBox(True)
        self.check_box.set_spacing(12)

        self.hide_sleep_connections = newCheckBox()
        self.hide_sleep_connections.set_text('Hide sleeping connections')
        self.hide_sleep_connections.add_clicked_callback(self.refresh)
        self.check_box.add(self.hide_sleep_connections, False, True)
        
        if self.new_processlist():
            self.hide_background_threads = newCheckBox()
            self.hide_background_threads.set_active(True)
            self.hide_background_threads.set_text('Hide background threads')
            self.hide_background_threads.add_clicked_callback(self.refresh)
            self.check_box.add(self.hide_background_threads, False, True)
            
            self.truncate_info = newCheckBox()
            self.truncate_info.set_active(True)
            self.truncate_info.set_text('Don\'t load full thread info')
            self.truncate_info.add_clicked_callback(self.refresh)
            self.check_box.add(self.truncate_info, False, True)

        self.add(self.check_box, False, True)
        
        self.resume_layout()
        
        self.connection_selected()
        
        dprint_ex(4, "Leave")


    def shutdown(self):
        if self._refresh_timeout:
            Utilities.cancel_timeout(self._refresh_timeout)
            self._refresh_timeout = None

        
    def create_labeled_info(self, lbl_txt, lbl_name, tooltip_name = None):
        lbox = newBox(True)
        lbox.set_spacing(5)
        l = mforms.newLabel(lbl_txt)
        lbox.add(l, False, False)
        setattr(self, lbl_name, mforms.newLabel(""))
        l.set_style(mforms.BoldStyle)
        lbox.add(getattr(self, lbl_name), False, False)
        if tooltip_name != None:
            i = mforms.newImageBox()
            i.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
            i.set_tooltip("")
            lbox.add(i, False, False)
            setattr(self, tooltip_name, i)

        return lbox

    def load_info_panel_data(self):
        vars = ['Threads_connected', 'Threads_running', 'Threads_created', 'Threads_cached', 'Connections', 'Aborted_clients', 'Aborted_connects']
        errors = []
        if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 5):
            vars.append('Connection_errors_max_connections')
            errors = ['Connection_errors_accept', 'Connection_errors_internal', 'Connection_mac_connections', 'Connection_per_addr', 'Connection_errors_select', 'Connection_errors_tcpwrap']
        error_tooltip = []
        error_count = 0
        result = self.ctrl_be.exec_query("SHOW GLOBAL STATUS")
        if result is not None:
            while result.nextRow():
                if result.stringByIndex(1) in vars:
                    obj = getattr(self, "lbl_%s" % result.stringByIndex(1), None)
                    if obj != None:
                        obj.set_text(result.stringByIndex(2))
                if result.stringByIndex(1) in errors:
                    error_count += int(result.stringByIndex(2))
                    error_tooltip.append("%s: %d" % (result.stringByIndex(1), int(result.stringByIndex(2))))
        
        if error_tooltip:
            self.lbl_errors.show(True)
            self.tooltip_errors.set_tooltip("\n".join(error_tooltip))
            self.lbl_errors.set_text("%d" % error_count)
        else:
            self.lbl_errors.show(False)
        
        self.lbl_max_connections.set_text(self.ctrl_be.server_variables['max_connections'])

    def menu_will_show(self, item):
        if item is None:
            self._menu.remove_all()
            
            selected_conn = self.connection_list.get_selection()

            if selected_conn:
                user_thread = self.new_processlist() == False
                if not user_thread:
                    user_thread = len([sel for sel in selected_conn if not sel.get_string(8).startswith('BACKGROUND')]) == 0
                         
                item = self._menu.add_item_with_title("Copy", self.copy_selected, "copy_selected")
                item = self._menu.add_item_with_title("Copy Info", self.copy_selected_info, "copy_selected_info")
                item = self._menu.add_item_with_title("Show in Editor", self.edit_selected, "edit_selected")
                item.set_enabled(user_thread)

                if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7):
                    self._menu.add_separator()
                    item = self._menu.add_item_with_title("Explain for Connection", self.explain_selected, "explain")
                    if len(selected_conn) != 1:
                        item.set_enabled(False)
                    else:
                        info = selected_conn[0].get_string(self.info_column)
                        if not info or info == "NULL":
                            item.set_enabled(False)

                if self.new_processlist():
                    item = self._menu.add_item_with_title("View Thread Stack", self.view_thread_stack, "view_thread_stack")
                    if len(selected_conn) != 1:
                        item.set_enabled(False)
                    instr_caption = "Enable Instrumentation for Thread"
                    start_with_yes =  [sel.get_string(11).startswith('YES') for sel in selected_conn]

                    if all(start_with_yes):
                        instr_caption = "Disable Instrumentation for Thread"
                    item = self._menu.add_item_with_title(instr_caption, self.enable_disable_instrumentation, "enable_disable_instrumentation")
                    if any(start_with_yes) and not all(start_with_yes):
                        item.set_enabled(False)                    

                self._menu.add_separator()
                item = self._menu.add_item_with_title("Kill Query(s)", self.kill_query, "kill_query")
                item.set_enabled(user_thread)
                item = self._menu.add_item_with_title("Kill Connection(s)", self.kill_connection, "kill_connection")
                item.set_enabled(user_thread)
                self._menu.add_separator()
            self._menu.add_item_with_title("Refresh", self.refresh, "refresh")

    def column_resized(self, col):
        widths = []
        for c in range(self.connection_list.get_column_count()):
            widths.append(self.connection_list.get_column_width(c))
        widths = ",".join([str(w) for w in widths])
        if self.new_processlist():
            grt.root.wb.state["wb.admin:ConnectionListColumnWidthsPS"] = widths
        else:
            grt.root.wb.state["wb.admin:ConnectionListColumnWidths"] = widths


    def check_if_ps_available(self):
        if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6):
            result = self.ctrl_be.exec_query("select @@performance_schema")
            if result:
                if result.nextRow():
                    if result.intByIndex(1) == 1:
                        return True
        return False
            
    def new_processlist(self):
        return self._new_processlist
    
    def connection_selected(self):
        dprint_ex(4, "Enter")
        if not self.connection_list.get_selection():
            self.kill_button.set_enabled(False)
            self.killq_button.set_enabled(False)
        else:
            self.kill_button.set_enabled(True)
            self.killq_button.set_enabled(True)
        dprint_ex(4, "Leave")
    
    def page_activated(self):
        if not self.ui_created:
            self.create_ui()
            self.ui_created = True
        
        self.page_active = True
        if self.ctrl_be.is_sql_connected():
            self.warning.show(False)
            self.heading.show(True)
            self.connection_list.show(True)
            self.info_table.show(True)
            self.button_box.show(True)
            self.check_box.show(True)
        else:
            self.warning.show(True)
            self.heading.show(False)
            self.connection_list.show(False)
            self.info_table.show(False)
            self.button_box.show(False)
            self.check_box.show(False)
        
        self.refresh()
    
    def page_deactivated(self):
        self.page_active = False
    
    def get_process_list(self):
        if self.new_processlist():
            return self.get_process_list_new()
        else:
            return self.get_process_list_old()
        
    def get_process_list_new(self):
        cols = []
        for field, type, caption, width in self.columns:
            if field == "PROCESSLIST_USER":
                cols.append("IF (NAME = 'thread/sql/event_scheduler','event_scheduler',PROCESSLIST_USER) PROCESSLIST_USER")
            elif field == "INFO" and self.truncate_info.get_active():
                cols.append("SUBSTR(INFO, 0, 255) INFO")
            else: 
                cols.append(field)

        if self.hide_background_threads.get_active():
            result = self.ctrl_be.exec_query("SELECT %s FROM performance_schema.threads WHERE TYPE <> 'BACKGROUND'" % ",".join(cols))
        else:
            result = self.ctrl_be.exec_query("SELECT %s FROM performance_schema.threads" % ",".join(cols))
        if result is not None:
            result_rows = []
            while result.nextRow():
                row = []
                for i, (field, type, caption, width) in enumerate(self.columns):
                    value = result.stringByName(field)
                    row.append(value)
                result_rows.append(row)
            return result_rows
        return None
    
    def get_process_list_old(self):
        result = self.ctrl_be.exec_query("SHOW FULL PROCESSLIST")
        if result is not None:
            result_rows = []
            while result.nextRow():
                row = []
                for field, type, caption, width in self.columns:
                    value = result.stringByName(field)
                    row.append(value)
                result_rows.append(row)
            return result_rows
        
        return None
    
    
    def update_refresh_rate(self):
        index = int(self.refresh_selector.get_selected_index())
        grt.root.wb.options.options['Administrator:refresh_connections_rate_index'] = index
        self.serial += 1
        if self._refresh_timeout:
            Utilities.cancel_timeout(self._refresh_timeout)
            self._refresh_timeout = None
        if (index < self.refresh_values_size):
            self._refresh_timeout = Utilities.add_timeout(self.refresh_values[index], partial(self.refresh, my_serial = self.serial))

    def enable_disable_instrumentation(self):
        selected_conn = self.connection_list.get_selection()
        if not selected_conn:
            return
        instr_state = 'YES'
        if selected_conn[0].get_string(11).startswith('YES'):
            instr_state = 'No'

        for sel in selected_conn:
            connid = sel.get_long(7)
            try:
                self.ctrl_be.exec_sql("UPDATE performance_schema.threads SET instrumented = '%s' WHERE thread_id = %d LIMIT 1" % (instr_state, connid))
            except Exception, e:
                mforms.Utilities.show_error("Update Thread Instrumentation", "Error setting instrumentation for thread %d: %s" % (connid, e), "OK",  "", "")
                break

        self.refresh()


    def view_thread_stack(self):
        sel = self.connection_list.get_selected_node()
        if not sel:
            return

        view = WBThreadStack(self.ctrl_be, sel.get_long(7))
        view.run()

    def copy_selected_info(self):
        selected_conn = self.connection_list.get_selection()
        if not selected_conn:
            return

        info = ', '.join([sel.get_tag() for sel in selected_conn if sel.get_tag()])
        if len(info) > 250 and self.new_processlist() and self.truncate_info.get_active():
            info += " /* statement may be truncated */"
        mforms.Utilities.set_clipboard_text(info)


    def _node_text(self, sel):
        text = []
        text.append("-- Connection Id: %s\n" % sel.get_long(0))
        text.append("-- User: %s\n" % sel.get_string(1))
        text.append("-- Host: %s\n" % sel.get_string(2))
        text.append("-- DB: %s\n" % sel.get_string(3))
        text.append("-- Command: %s\n" % sel.get_string(4))
        text.append("-- Time: %s\n" %  sel.get_long(5))
        text.append("-- State: %s\n" % sel.get_string(6))
        info = sel.get_tag()
        if len(info) > 250 and self.new_processlist() and self.truncate_info.get_active():
            info += " /* statement may be truncated */"
        text.append(info)
        return "".join(text)


    def copy_selected(self):
        selected_conn = self.connection_list.get_selection()
        if not selected_conn:
            return
        mforms.Utilities.set_clipboard_text('\n'.join([self._node_text(sel) for sel in selected_conn]))


    def edit_selected(self):
        selected_conn = self.connection_list.get_selection()
        if not selected_conn:
            return
        editor = self.main_view.editor.addQueryEditor()
        editor.replaceContents('\n'.join([self._node_text(sel) for sel in selected_conn]))
        return editor

    def explain_selected(self):
        sel = self.connection_list.get_selected_node()
        if not sel:
            return

        editor = self.edit_selected()
        grt.modules.SQLIDEQueryAnalysis.visualExplainForConnection(editor, str(sel.get_long(0)), sel.get_string(self.info_column))


    def refresh(self, query_result = None, my_serial = 0):
        if not self.page_active:
            dprint_ex(2, "Leave. Page is inactive")
            return True
 
        if not self.ctrl_be.is_sql_connected():
            dprint_ex(2, "Leave. SQL connection is offline")
            self._refresh_timeout = None
            return False
       
        self.load_info_panel_data()

        if self.new_processlist():
            id_column = 7
        else:
            id_column = 0

        node = self.connection_list.get_selected_node()
        if node:
            old_selected = node.get_long(id_column)
        else:
            old_selected = None
        old_selected_node = None
        
        if query_result is None:
            query_result = self.get_process_list()
        
        if query_result is not None:
            self.connection_list.freeze_refresh()
            self.connection_list.clear()
            no_sleep_connections = self.hide_sleep_connections.get_active()
            
            no_bg_threads = False
            if self.new_processlist():
                no_bg_threads = self.hide_background_threads.get_active()
            try:
                for row in query_result:
                    if no_sleep_connections and str(row[4]).startswith('Sleep'):
                        continue
                    if no_bg_threads and str(row[8]).startswith('BACKGROUND'):
                         continue
                    r = self.connection_list.add_node()
                    for c, field in enumerate(row):
                        if c in self.long_int_columns:
                            try:
                                field = long(field)
                            except Exception:
                                field = 0
                            r.set_long(c, field)
                            if field == old_selected and id_column == c:
                              old_selected_node = r
                        elif c == self.info_column:
                            # truncate Info column to 255 chars for display, since they can be REALLY long
                            # which causes GDI trouble in Windows... so just store the full info in the tag
                            if field is not None:
                                r.set_string(c, field[:255])
                            else:
                                r.set_string(c, "NULL")
                            r.set_tag(field or "")
                        else:
                            field = str(field)
                            r.set_string(c, field)
            
            finally:
                self.connection_list.thaw_refresh()
            
            if old_selected_node:
                self.connection_list.select_node(old_selected_node)
            
            self.connection_selected()
        
        cont = (my_serial == self.serial)
        if not cont:
            self._refresh_timeout = None
        return cont
    
    
    def kill_connection(self):
        if not self.ctrl_be.is_sql_connected():
            return
          
        selections = self.connection_list.get_selection()
        if not selections:
            return
        
        for sel in selections:
            connid = sel.get_long(0)
            if self.new_processlist() and sel.get_string(8).startswith('BACKGROUND'):
                mforms.Utilities.show_error("Error Killing Connection", "Thread %s cannot be killed" % connid, "OK",  "", "")
                return

            try:
                self.ctrl_be.exec_sql("KILL CONNECTION %s"%connid)
            except Exception, e:
                mforms.Utilities.show_error("Error Killing Connection", "%s" % e, "OK",  "", "")
                break
                
        self.refresh()
    
    
    def kill_query(self):
        if not self.ctrl_be.is_sql_connected():
            return
        
        selections = self.connection_list.get_selection()
        if not selections:
            return
        
        for sel in selections:
            connid = sel.get_long(0)
            if self.new_processlist() and sel.get_string(8).startswith('BACKGROUND'):
                mforms.Utilities.show_error("Error Killing Connection", "Thread %s cannot be killed" % connid, "OK",  "", "")
                return

            try:
                self.ctrl_be.exec_sql("KILL QUERY %s"%connid)
            except Exception, e:
                mforms.Utilities.show_error("Error Killing Connection", "Error executing KILL QUERY on thread %d: %s" % (connid, e), "OK",  "", "")
                break
          
        self.refresh()

