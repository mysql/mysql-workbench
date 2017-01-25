# Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

from mforms import newTreeView, newButton, newBox, newSelector, newCheckBox, newLabel, Utilities
import mforms
import grt

from workbench.db_utils import escape_sql_string, QueryError

from functools import partial

from wb_common import dprint_ex
from wb_admin_utils import weakcb, WbAdminBaseTab
import json

from workbench.log import log_error

from wb_admin_utils import not_running_warning_label

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

        self.tree = mforms.newTreeView(mforms.TreeDefault)
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
                mforms.Utilities.show_error("Error Getting Thread Stack", "Thread stack is not available for thread %d. Please enable Performance Schema instrumentation (Statement and Stage instrumentations and respective History consumers)." % self.thread_id, "OK",  "", "")
            self.close()
        else:
            self.show()



class ConnectionDetailsPanel(mforms.Table):
    def __init__(self, owner):
        mforms.Table.__init__(self)

        self.owner = owner

        self.set_padding(8)
        self.set_row_spacing(4)
        self.set_column_spacing(4)
        self.set_column_count(2)
        self.set_row_count(16)

        self.labels = {}
        self.make_line("Processlist Id:", "PROCESSLIST_ID")
        self.make_line("Thread Id:", "THREAD_ID")
        self.make_line("Name:", "NAME")
        self.make_line("Type:", "TYPE")
        self.make_line("User:", "PROCESSLIST_USER")
        self.make_line("Host:", "PROCESSLIST_HOST")
        self.make_line("Schema:", "PROCESSLIST_DB")
        self.make_line("Command:", "PROCESSLIST_COMMAND")
        self.make_line("Time:", "PROCESSLIST_TIME")
        self.make_line("State:", "PROCESSLIST_STATE")
        self.make_line("Role:", "ROLE")
        self.make_line("Instrumented:", "INSTRUMENTED")
        self.make_line("Parent Thread Id:", "PARENT_THREAD_ID")

        l = mforms.newLabel("Info:")
        l.set_style(mforms.BoldStyle)
        self.add(l, 0, 1, 13, 14, mforms.HFillFlag|mforms.HExpandFlag)
        self.info = mforms.newCodeEditor()
        self.info.set_features(mforms.FeatureGutter, False)
        self.info.set_features(mforms.FeatureReadOnly, True)
        self.info.set_features(mforms.FeatureWrapText, True)
        self.info.set_language(mforms.LanguageMySQL56)
        self.info.set_size(0, 50)
        self.add(self.info, 0, 2, 14, 15, mforms.HFillFlag|mforms.VFillFlag|mforms.HExpandFlag|mforms.VExpandFlag)

        if self.owner.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 2):
            self.explain = mforms.newButton()
            self.explain.set_enabled(False)
            self.explain.add_clicked_callback(self.owner.explain_selected)
            self.explain.set_text("Explain for Connection")
            self.add(self.explain, 1, 2, 15, 16, mforms.HFillFlag)
        else:
            self.explain = None



    def make_line(self, caption, name):
        i = len(self.labels)
        l = mforms.newLabel(caption)
        l.set_text_align(mforms.MiddleLeft)
        l.set_style(mforms.BoldStyle)
        self.add(l, 0, 1, i, i+1, mforms.HFillFlag|mforms.HExpandFlag)
        l = mforms.newLabel("")
        self.add(l, 1, 2, i, i+1, mforms.HFillFlag|mforms.HExpandFlag)
        self.labels[name] = l


    def update(self, node):
        self.labels["PROCESSLIST_ID"].set_text("%s" % node.get_long(0) if node else "")
        self.labels["PROCESSLIST_USER"].set_text(node.get_string(1) if node else "")
        self.labels["PROCESSLIST_HOST"].set_text(node.get_string(2) if node else "")
        self.labels["PROCESSLIST_DB"].set_text(node.get_string(3) if node else "")
        self.labels["PROCESSLIST_COMMAND"].set_text(node.get_string(4) if node else "")
        self.labels["PROCESSLIST_TIME"].set_text("%s" % node.get_long(5) if node else "")
        self.labels["PROCESSLIST_STATE"].set_text(node.get_string(6) if node else "")
        self.labels["THREAD_ID"].set_text("%s" % node.get_long(7) if node else "")
        self.labels["TYPE"].set_text(node.get_string(8) if node else "")
        self.labels["NAME"].set_text(node.get_string(9) if node else "")
        self.labels["PARENT_THREAD_ID"].set_text("%s" % node.get_long(10) if node else "")
        self.labels["INSTRUMENTED"].set_text(node.get_string(11) if node else "")

        self.info.set_features(mforms.FeatureReadOnly, False)
        self.info.set_value(node.get_tag() if node else "")
        self.info.set_features(mforms.FeatureReadOnly, True)
        if self.explain:
            self.explain.set_enabled(True if node and node.get_tag() else False)


class WbAdminConnections(WbAdminBaseTab):
    ui_created = False
    serial = 0

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_management", "Client Connections", False)

    @classmethod
    def identifier(cls):
        return "admin_connections"

    def __init__(self, ctrl_be, instance_info, main_view):
        WbAdminBaseTab.__init__(self, ctrl_be, instance_info, main_view)
        self.set_managed()
        self.set_release_on_add()
        self.set_padding(12)
        self.set_spacing(15)
        self.page_active = False
        self._new_processlist = self.check_if_ps_available()
        self._refresh_timeout = None
        self.warning = None
        
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
                            ("PROCESSLIST_INFO", mforms.StringColumnType, "Info", 200),
                            ]
            self.long_int_columns = [0, 5, 7, 10]
            self.info_column = 12
            if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 6):
                self.columns.append(("ATTR_VALUE", mforms.StringColumnType, "Program", 150))

            get_path = mforms.App.get().get_resource_path
            self.icon_for_object_type = {
                "GLOBAL" : "",
                "SCHEMA" : get_path("db.Schema.16x16.png"),
                "TABLE" : get_path("db.Table.16x16.png"),
                "FUNCTION" : get_path("db.Role.16x16.png"),
                "PROCEDURE" : get_path("db.Role.16x16.png"),
                "TRIGGER" : get_path("db.Trigger.16x16.png"),
                "EVENT" : get_path("GrtObject.16x16.png"),
                "COMMIT" : get_path("db.Column.16x16.png")
            }
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

        self.create_basic_ui("title_connections.png", "Client Connections")

        if self.new_processlist():
            widths = grt.root.wb.state.get("wb.admin:ConnectionListColumnWidthsPS", None)
        else:
            widths = grt.root.wb.state.get("wb.admin:ConnectionListColumnWidths", None)
        if widths:
            column_widths = [int(i) for i in widths.split(",")]
        else:
            column_widths = None

        self.connection_box = mforms.newBox(True)
        self.connection_box.set_spacing(8)
        self.connection_list = newTreeView(mforms.TreeDefault|mforms.TreeFlatList|mforms.TreeAltRowColors)
        self.connection_list.set_selection_mode(mforms.TreeSelectMultiple)
        self.connection_list.add_column_resized_callback(self.column_resized)
        for i, (field, type, caption, width) in enumerate(self.columns):
            if column_widths and i < len(column_widths):
                width = column_widths[i]
            self.connection_list.add_column(type, caption, width, False)

        self.connection_list.end_columns()
        self.connection_list.set_allow_sorting(True)
        
        self.connection_list.add_changed_callback(weakcb(self, "connection_selected"))

        self.connection_box.add(self.connection_list, True, True)

        info_table = mforms.newTable()
        info_table.set_row_count(2)
        info_table.set_column_count(5)
        info_table.set_row_spacing(4)
        info_table.set_column_spacing(20)

        info_table.add(self.create_labeled_info("Threads Connected:", "lbl_Threads_connected"),                     0, 1, 0, 1, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Threads Running:", "lbl_Threads_running"),                          1, 2, 0, 1, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Threads Created:", "lbl_Threads_created"),                         2, 3, 0, 1, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Threads Cached:", "lbl_Threads_cached"),                           3, 4, 0, 1, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Rejected (over limit):", "lbl_Connection_errors_max_connections"), 4, 5, 0, 1, mforms.HFillFlag | mforms.VFillFlag)

        info_table.add(self.create_labeled_info("Total Connections:", "lbl_Connections"),                           0, 1, 1, 2, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Connection Limit:", "lbl_max_connections"),                        1, 2, 1, 2, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Aborted Clients:", "lbl_Aborted_clients"),                         2, 3, 1, 2, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Aborted Connections:", "lbl_Aborted_connects"),                    3, 4, 1, 2, mforms.HFillFlag | mforms.VFillFlag)
        info_table.add(self.create_labeled_info("Errors:", "lbl_errors", "tooltip_errors"),                         4, 5, 1, 2, mforms.HFillFlag | mforms.VFillFlag)

        self.info_table = info_table
        self.add(info_table, False, True)

        #self.set_padding(8)
        self.add(self.connection_box, True, True)


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
        self.hide_sleep_connections.set_tooltip('Remove connections in the Sleeping state from the connection list.')
        self.check_box.add(self.hide_sleep_connections, False, True)

        self.mdl_locks_page = None
        self._showing_extras = False
        if self.new_processlist():
            self.hide_background_threads = newCheckBox()
            self.hide_background_threads.set_active(True)
            self.hide_background_threads.set_text('Hide background threads')
            self.hide_background_threads.set_tooltip('Remove background threads (internal server threads) from the connection list.')
            self.hide_background_threads.add_clicked_callback(self.refresh)
            self.check_box.add(self.hide_background_threads, False, True)
            
            self.truncate_info = newCheckBox()
            self.truncate_info.set_active(True)
            self.truncate_info.set_text('Don\'t load full thread info')
            self.truncate_info.set_tooltip('Toggle whether to load the entire query information for all connections or just the first 255 characters.\nEnabling this can have a large impact in busy servers or server executing large INSERTs.')
            self.truncate_info.add_clicked_callback(self.refresh)
            self.check_box.add(self.truncate_info, False, True)

            # tab with some extra info, only available if PS exists
            self.extra_info_tab = mforms.newTabView(mforms.TabViewSystemStandard)
            self.extra_info_tab.set_size(350, -1)
            self.extra_info_tab.add_tab_changed_callback(self.extra_tab_changed)

            self.connection_details_scrollarea = mforms.newScrollPanel()
            self.connection_details = ConnectionDetailsPanel(self)
            self.connection_details_scrollarea.add(self.connection_details)
            self.details_page = self.extra_info_tab.add_page(self.connection_details_scrollarea, "Details")

            self.mdl_list_box = None
            if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 3):
                self.mdl_list_box_scrollarea = mforms.newScrollPanel()
                self.mdl_list_box = mforms.newBox(False)
                self.mdl_list_box_scrollarea.add(self.mdl_list_box)

                self.mdl_label = mforms.newLabel('Metadata locks (MDL) protect concurrent access to\nobject metadata (not table row/data locks)')
                self.mdl_list_box.add(self.mdl_label, False, True)

                label = mforms.newLabel("\nGranted Locks (and threads waiting on them)")
                label.set_style(mforms.BoldStyle)
                self.mdl_list_box.add(label, False, True)
                label = mforms.newLabel("Locks this connection currently owns and\nconnections that are waiting for them.")
                label.set_style(mforms.SmallHelpTextStyle)
                self.mdl_list_box.add(label, False, True)

                self.mdl_list_held = mforms.newTreeView(mforms.TreeAltRowColors)
                self.mdl_list_held.add_column(mforms.IconStringColumnType, "Object", 130, False)
                self.mdl_list_held.add_column(mforms.StringColumnType, "Type", 100, False)
                self.mdl_list_held.add_column(mforms.StringColumnType, "Duration", 100, False)
                self.mdl_list_held.end_columns()
                self.mdl_list_held.set_size(0, 100)
                self.mdl_list_box.add(self.mdl_list_held, True, True)

                label = mforms.newLabel("\nPending Locks")
                label.set_style(mforms.BoldStyle)
                self.mdl_list_box.add(label, False, True)
                hbox = mforms.newBox(True)
                hbox.set_spacing(4)
                self.mdl_blocked_icon = mforms.newImageBox()
                self.mdl_blocked_icon.set_image(mforms.App.get().get_resource_path("message_warning.png"))
                hbox.add(self.mdl_blocked_icon, False, True)
                self.mdl_waiting_label = mforms.newLabel("Locks this connection is currently waiting for.")
                hbox.add(self.mdl_waiting_label, True, True)
                self.mdl_list_box.add(hbox, False, True)
                self.mdl_locks_page = self.extra_info_tab.add_page(self.mdl_list_box_scrollarea, "Locks")

            if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 0):
                self.attributes_list = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors)
                self.attributes_list.add_column(mforms.StringColumnType, "Attribute", 150, False)
                self.attributes_list.add_column(mforms.StringColumnType, "Value", 200, False)
                self.attributes_list.end_columns()
                self.attributes_page = self.extra_info_tab.add_page(self.attributes_list, "Attributes")

            self.connection_box.add(self.extra_info_tab, False, True)
            self.extra_info_tab.show(False)

            self.show_extras = newButton()
            self.show_extras.set_text('Show Details')
            self.show_extras.add_clicked_callback(self.toggle_extras)
            self.check_box.add_end(self.show_extras, False, True)

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
                    user_thread = len([sel for sel in selected_conn if not sel.get_string(8).startswith('BACKGROUND')]) > 0

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


    def check_if_mdl_available(self):
        if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 3):
            result = self.ctrl_be.exec_query("select count(*) from performance_schema.setup_instruments where name = 'wait/lock/metadata/sql/mdl' and enabled='YES'")
            if result:
                if result.nextRow():
                    if result.intByIndex(1) == 1:
                        return True
        return False


    def new_processlist(self):
        return self._new_processlist
    
    def connection_selected(self):
        dprint_ex(4, "Enter")
        sel = self.connection_list.get_selection()
        if not sel:
            self.kill_button.set_enabled(False)
            self.killq_button.set_enabled(False)
        else:
            self.kill_button.set_enabled(True)
            self.killq_button.set_enabled(True)

        if self._showing_extras:
            if sel and len(sel) == 1:
                self.connection_details.update(sel[0])
            else:
                self.connection_details.update(None)
            tab = self.extra_info_tab.get_active_tab()
            if tab == self.mdl_locks_page:
                self.refresh_mdl_list()
            elif tab == self.attributes_page:
                self.refresh_attr_list()

        dprint_ex(4, "Leave")


    def refresh_attr_list(self):
        self.attributes_list.clear()

        try:
            nodes = self.connection_list.get_selection()
            if nodes and len(nodes) == 1:
                connid = nodes[0].get_long(0)

                result = self.ctrl_be.exec_query("SELECT * FROM performance_schema.session_connect_attrs WHERE processlist_id = %s ORDER BY ORDINAL_POSITION" % connid)
                while result and result.nextRow():
                    node = self.attributes_list.add_node()
                    node.set_string(0, result.stringByName("ATTR_NAME"))
                    node.set_string(1, result.stringByName("ATTR_VALUE"))
        except Exception, e:
            import traceback
            log_error("Error looking up attribute information: %s\n" % traceback.format_exc())
            mforms.Utilities.show_error("Lookup Connection Attributes", "Error looking up connection attributes: %s" % e, "OK", "", "")


    def refresh_mdl_list(self):
        self.mdl_list_held.clear()

        self.mdl_blocked_icon.show(False)
        waiting_label_text = "This connection is not waiting for any locks."

        try:
            nodes = self.connection_list.get_selection()
            if nodes and len(nodes) == 1:
                thread_id = nodes[0].get_long(7)

                result = self.ctrl_be.exec_query("SELECT * FROM performance_schema.metadata_locks WHERE owner_thread_id = %s" % thread_id)
                if result is not None:
                    while result.nextRow():
                        lock_status = result.stringByName("LOCK_STATUS")
                        if lock_status == "PENDING":
                            otype = result.stringByName("OBJECT_TYPE")
                            oschema = result.stringByName("OBJECT_SCHEMA")
                            oname = result.stringByName("OBJECT_NAME")
                            obj_name = [oschema, oname]
                            if otype == "GLOBAL":
                                obj_name = "<global>"
                            else:
                                obj_name =  ".".join([o for o in obj_name if o is not None])
                            self.mdl_blocked_icon.show(True)

                            sub_expr = "OBJECT_TYPE = '%s'" % otype
                            if oschema:
                                sub_expr += " AND OBJECT_SCHEMA = '%s'" % escape_sql_string(oschema)
                            if oname:
                                sub_expr += " AND OBJECT_NAME = '%s'" % escape_sql_string(oname)

                            lock_type = result.stringByName("LOCK_TYPE")
                            lock_duration = result.stringByName("LOCK_DURATION")

                            subresult = self.ctrl_be.exec_query("""SELECT *
                                FROM performance_schema.metadata_locks
                                WHERE %s AND LOCK_STATUS = 'GRANTED'""" % sub_expr)
                            owners = []
                            while subresult and subresult.nextRow():
                                owners.append(subresult.intByName("OWNER_THREAD_ID"))
                            owner_list = ", ".join([str(i) for i in owners])
                            if len(owners) == 1:
                                waiting_label_text = "The connection is waiting for a lock on\n%s %s,\nheld by thread %s." % (otype.lower(), obj_name, owner_list)
                            else:
                                waiting_label_text = "The connection is waiting for a lock on\n%s %s,\nheld by threads %s" % (otype.lower(), obj_name, owner_list)
                            waiting_label_text += "\nType: %s\nDuration: %s" % (lock_type, lock_duration)
                        elif lock_status == "GRANTED":
                            node = self.mdl_list_held.add_node()

                            otype = result.stringByName("OBJECT_TYPE")
                            oschema = result.stringByName("OBJECT_SCHEMA")
                            oname = result.stringByName("OBJECT_NAME")
                            obj_name = [oschema, oname]
                            if otype == "GLOBAL":
                                node.set_string(0, "<global>")
                            else:
                                node.set_string(0, ".".join([o for o in obj_name if o is not None]))
                            node.set_icon_path(0, self.icon_for_object_type.get(otype))

                            sub_expr = "OBJECT_TYPE = '%s'" % otype
                            if oschema:
                                sub_expr += " AND OBJECT_SCHEMA = '%s'" % escape_sql_string(oschema)
                            if oname:
                                sub_expr += " AND OBJECT_NAME = '%s'" % escape_sql_string(oname)

                            node.set_string(1, result.stringByName("LOCK_TYPE"))
                            node.set_string(2, result.stringByName("LOCK_DURATION"))

                            subresult = self.ctrl_be.exec_query("""SELECT OWNER_THREAD_ID, LOCK_TYPE, LOCK_DURATION
                                        FROM performance_schema.metadata_locks
                                        WHERE %s AND LOCK_STATUS = 'PENDING'""" % sub_expr)
                            while subresult and subresult.nextRow():
                                subnode = node.add_child()
                                subnode.set_string(0, "thread %s" % subresult.intByName("OWNER_THREAD_ID"))
                                subnode.set_string(1, subresult.stringByName("LOCK_TYPE"))
                                subnode.set_string(2, subresult.stringByName("LOCK_DURATION"))
            else:
                waiting_label_text = ""
        except Exception, e:
            import traceback
            log_error("Error looking up metadata lock information: %s\n" % traceback.format_exc())
            mforms.Utilities.show_error("Lookup Metadata Locks", "Error looking up metadata lock information: %s" % e, "OK", "", "")

        self.mdl_waiting_label.set_text(waiting_label_text)

        
    def show_warning_message(self, text):
        if not self.heading:
            self.create_basic_ui("title_connections.png", "Client Connections")
      
        if self.warning:
            self.remove(self.warning)
            self.warning = None

        self.warning = not_running_warning_label()
        self.warning.set_text("\n\n\n\n%s" % text)
        self.warning.show(True)
        self.add(self.warning, False, True)

    def show_no_permission(self):
        self.show_warning_message("The account you are currently using does not have sufficient privileges to view the client connections.")
        
    def show_generic_error(self):
        self.show_warning_message("There was a problem opening the Client Connections. Please check the error log for more details.")
        
    def page_activated(self):

        try:
            self.ctrl_be.exec_query("SELECT COUNT(*) FROM performance_schema.threads")
        except QueryError, e:
            import traceback
            log_error("QueryError in Admin for Client Connections:\n%s\n\n%s\n" % (e, traceback.format_exc()))
            if e.error == 1142:
                self.show_no_permission()
            else:
                self.show_generic_error()
            return
        except Exception, e:
            import traceback
            log_error("Exception in Admin for Client Connections:\n%s\n\n%s\n" % (e, traceback.format_exc()))
            self.show_generic_error()
            return

        WbAdminBaseTab.page_activated(self)

        if not self.ui_created:
            self.create_ui()
            self.ui_created = True
        
        self.page_active = True
        if self.ctrl_be.is_sql_connected():
            self.connection_box.show(True)
            self.info_table.show(True)
            self.button_box.show(True)
            self.check_box.show(True)
        else:
            self.connection_box.show(False)
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
                cols.append("IF (NAME = 'thread/sql/event_scheduler','event_scheduler',t.PROCESSLIST_USER) PROCESSLIST_USER")
            elif field == "INFO" and self.truncate_info.get_active():
                cols.append("SUBSTR(t.INFO, 0, 255) INFO")
            else:
                if field == "ATTR_VALUE":
                    cols.append("a."+field)
                else:
                    cols.append("t."+field)

        if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 6):
            JOIN = " LEFT OUTER JOIN performance_schema.session_connect_attrs a ON t.processlist_id = a.processlist_id AND (a.attr_name IS NULL OR a.attr_name = 'program_name')"
        else:
            JOIN = ""
        if self.hide_background_threads.get_active():
            result = self.ctrl_be.exec_query("SELECT %s FROM performance_schema.threads t %s WHERE t.TYPE <> 'BACKGROUND'" % (",".join(cols), JOIN))
        else:
            result = self.ctrl_be.exec_query("SELECT %s FROM performance_schema.threads t %s WHERE 1=1" % (",".join(cols), JOIN))
        
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
                log_error("Error enabling thread instrumentation: %s\n" % e)
                mforms.Utilities.show_error("Toggle Thread Instrumentation", "Error setting instrumentation for thread %d: %s" % (connid, e), "OK",  "", "")
                break

        self.refresh()


    def extra_tab_changed(self):
        self.connection_selected()


    def enable_mdl_instrumentation(self):
        try:
            self.ctrl_be.exec_sql("UPDATE performance_schema.setup_instruments SET enabled='YES' WHERE name = 'wait/lock/metadata/sql/mdl'")
        except Exception, e:
            log_error("Error enabling MDL instrumentation: %s\n" % e)
            mforms.Utilities.show_error("Enable MDL Instrumentation", "Error enabling performance_schema MDL instrumentation.\n%s" % e, "OK",  "", "")
            return

        self._mdl_enabled = self.check_if_mdl_available()
        if self._mdl_enabled:
            self.mdl_list_box.remove(self.mdl_enable_button_sep)
            self.mdl_list_box.remove(self.mdl_enable_button)
            self.mdl_enable_button_sep = None
            self.mdl_enable_button = None
        else:
            log_error("MDL instrumentation enabled, but it's still disabled!?\n")


    _mdl_enabled = None
    def toggle_extras(self):
        self._showing_extras = not self._showing_extras
        if self._showing_extras:
            if self._mdl_enabled is None:
                self._mdl_enabled = self.check_if_mdl_available()

            if not self._mdl_enabled and self.mdl_list_box:
                self.mdl_enable_button_sep = mforms.newLabel("\n\nMDL instrumentation is currently disabled.\nClick [Enable Instrumentation] to enable it.")
                self.mdl_list_box.add(self.mdl_enable_button_sep, False, True)

                self.mdl_enable_button = mforms.newButton()
                self.mdl_enable_button.add_clicked_callback(self.enable_mdl_instrumentation)
                self.mdl_enable_button.set_text("Enable Instrumentation")
                self.mdl_list_box.add(self.mdl_enable_button, False, True)

            self.extra_info_tab.show(True)
            self.connection_selected()
            self.show_extras.set_text("Hide Details")
        else:
            self.extra_info_tab.show(False)
            self.show_extras.set_text("Show Details")


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

