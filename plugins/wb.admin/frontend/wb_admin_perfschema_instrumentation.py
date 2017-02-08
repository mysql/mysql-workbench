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

import grt
import mforms
from workbench.change_tracker import ChangeCounter
from workbench.graphics.cairo_utils import Context
from workbench.graphics.canvas import ImageFigure

from wb_admin_perfschema_instrumentation_be import PSConfiguration, PSTimerType, PSObject, PSActor

from workbench.log import log_info, log_error

from wb_admin_perfschema import WbAdminPSBaseTab

MYSQL_ERR_TABLE_DOESNT_EXIST = 1146


DEFAULT_INSTRUMENTS_57 = [
'wait/io/file/%',
'wait/io/table/%',
'wait/lock/table/sql/handler',
'statement/%',
'idle'
]

DEFAULT_INSTRUMENTS_56 = [
'wait/io/file/%',
'wait/io/table/%',
'statement/%',
'wait/lock/table/sql/handler',
'idle'
]

DEFAULT_CONSUMERS_57 = [
'events_statements_current', 'events_transactions_current', 'global_instrumentation', 'thread_instrumentation', 'statements_digest'
]

DEFAULT_CONSUMERS_56 = [
'events_statements_current', 'global_instrumentation', 'thread_instrumentation', 'statements_digest'
]


class BigSwitch(mforms.PyDrawBox):
    def __init__(self):
        mforms.PyDrawBox.__init__(self)
        self.set_managed()
        self.set_release_on_add()

        self.set_instance(self)

        self.state = "default"

        self.hovering_state = None
        self.mouse_pos = None, None

        self.text_image = ImageFigure(mforms.App.get().get_resource_path("ps_switcher_text.png"))
        self.legend = ImageFigure(mforms.App.get().get_resource_path("ps_switcher_legende.png"))

        self.state_images = []
        for item in ["fully", "custom", "default", "disabled"]:
            icons = []
            for state in ["off", "hoover", "on"]:
                icon = ImageFigure(mforms.App.get().get_resource_path("ps_switcher_%s_%s.png" % (item, state)))
                icons.append(icon)
            self.state_images.append((item, tuple(icons)))



    def repaint(self, cr, x, y, w, h):
        c = Context(cr)
        self.hovering_state = None
        yy = 0
        for state, (off, hoover, on) in self.state_images:
            image = off
            xx = (self.get_width() - image.width - self.text_image.width) / 2
            if state == self.state:
                image = on
            else:
                if self.mouse_pos[0] > xx and self.mouse_pos[0] < xx + image.width and self.mouse_pos[1] >= yy and self.mouse_pos[1] <= yy + image.height and state != "custom":
                    image = hoover
                    self.hovering_state = state

            image.move(xx, yy)
            image.render(c)

            yy += image.height
        self.text_image.move(xx + image.width, 0)
        self.text_image.render(c)

        self.legend.move(xx + image.width + self.text_image.width + 100, self.get_height() - self.legend.height)
        self.legend.render(c)



    def set_state(self, state):
        self.state = state
        self.set_needs_repaint()


    def mouse_move(self, x, y):
        self.mouse_pos = x, y
        self.set_needs_repaint()


    def mouse_click(self, button, x, y):
        if self.hovering_state:
            self.callback(self.hovering_state)


class EasySetupPage(mforms.Box):
    def __init__(self, owner):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.set_spacing(12)
        self.set_padding(12)
        self.owner = owner

        self.create_ui()


    def check_instrumentation_level(self):
        try:
            r = self.owner.get_select_int_result("SELECT COUNT(*) FROM performance_schema.setup_consumers WHERE enabled='NO'")
            if r == 0:
                # Exclude results from 'memory/%' because they can't be disabled
                r = self.owner.get_select_int_result("SELECT COUNT(*) FROM performance_schema.setup_instruments WHERE NAME NOT LIKE 'memory/%' AND (enabled='NO' OR timed='NO')")
                if r == 0:
                    return "fully"

            r = self.owner.get_select_int_result("SELECT COUNT(*) FROM performance_schema.setup_consumers WHERE enabled='YES'")
            if r == 0:
                # Exclude results from 'memory/%' because they can't be disabled
                r = self.owner.get_select_int_result("SELECT COUNT(*) FROM performance_schema.setup_instruments WHERE NAME NOT LIKE 'memory/%' AND (enabled='YES' OR timed='YES')")
                if r == 0:
                    return "disabled"

            if self.owner.target_version.is_supported_mysql_version_at_least(5, 7, 0):
                instruments = DEFAULT_INSTRUMENTS_57
                consumers = DEFAULT_CONSUMERS_57
            else:
                instruments = DEFAULT_INSTRUMENTS_56
                consumers = DEFAULT_CONSUMERS_56

            # this will return 0 if all consumers have the expected value
            r = self.owner.get_select_int_result("""
                SELECT sum(IF(%s, 1, 0)) + sum(IF(%s, 1, 0)) FROM performance_schema.setup_consumers
                """ % ("ENABLED='NO' AND NAME IN (%s)" % ",".join(["'%s'" % c for c in consumers]),
                       "ENABLED='YES' AND NAME NOT IN (%s)" % ",".join(["'%s'" % c for c in consumers])))
            if r == 0:
                nlikes = []
                likes = []
                ins = []
                # Exclude results from 'memory/%' because they can't be disabled
                nlikes.append("'memory/%'")
                for i in instruments:
                   if '%' in i:
                       likes.append("NAME LIKE '%s'" % i)
                       nlikes.append("NAME NOT LIKE '%s'" % i)
                   else:
                       ins.append(i)
                exp = " OR ".join(["NAME IN (%s)" % ",".join(["'%s'" % i for i in ins])] + likes)
                nexp = " AND ".join(["NAME NOT IN (%s)" % ",".join(["'%s'" % i for i in ins])] + nlikes)
                r = self.owner.get_select_int_result("""
                    SELECT sum(IF(ENABLED='NO' AND (%(cond)s), 1, 0)) +
                        sum(IF(ENABLED='YES' AND (%(notcond)s), 1, 0)) +
                        sum(IF(TIMED='NO' AND (%(cond)s), 1, 0)) +
                        sum(IF(TIMED='YES' AND (%(notcond)s), 1, 0))
                    FROM performance_schema.setup_instruments
                    """ % {"cond" : exp, "notcond" : nexp})
                if r == 0:
                    return "default"

            return "custom"

        except grt.DBError, e:
            log_error("MySQL error querying PS instrumentation/consumers: %s\n" % e)
            mforms.Utilities.show_error("Check Performance Schema Configuration State", "Error checking configuration state of Performance Schema: %s" % e, "OK", "", "")

        return None


    def generate_updates_for_reset(self, instruments, consumers):
        likes = []
        ins = []
        for i in instruments:
            if '%' in i:
                likes.append("NAME LIKE '%s'" % i)
            else:
                ins.append(i)
        isql = """
            UPDATE performance_schema.setup_instruments
                SET ENABLED = IF(%s, 'YES', 'NO'),
                    TIMED = ENABLED
        """ % " OR ".join(["NAME IN (%s)" % ",".join(["'%s'" % i for i in ins])] + likes)

        csql = """
            UPDATE performance_schema.setup_consumers
                SET ENABLED = IF(%s, 'YES', 'NO')
        """ % "NAME IN (%s)" % ",".join(["'%s'" % c for c in consumers])

        return [isql, csql]


    def change_instrumentation(self, state):
        try:
            if state == "fully":
                if mforms.Utilities.show_warning("Performance Warning",
                                                 "While enabling all performance_schema instrumentation allows collecting a lot of information from MySQL, it will also impose a significant performance and memory overhead on it.\nDo not enable this option if your server is a production server under heavy load, unless you know what you're doing.", "Leave Unchanged", "Enable Everything", "") == mforms.ResultOk:
                    return

                sql = ["UPDATE performance_schema.setup_consumers SET enabled='YES'",
                       "UPDATE performance_schema.setup_instruments SET enabled='YES', timed='YES'"]
                mforms.App.get().set_status_text("Enabling Full Performance Schema Instrumentation...")
            elif state == "disabled":
                sql = ["UPDATE performance_schema.setup_consumers SET enabled='NO'",
                       "UPDATE performance_schema.setup_instruments SET enabled='NO', timed='NO'"]
                mforms.App.get().set_status_text("Disabling Full Performance Schema Instrumentation...")
            elif state == "default":
                if self.owner.target_version.is_supported_mysql_version_at_least(5, 7, 0):
                    instruments = DEFAULT_INSTRUMENTS_57
                    consumers = DEFAULT_CONSUMERS_57
                else:
                    instruments = DEFAULT_INSTRUMENTS_56
                    consumers = DEFAULT_CONSUMERS_56
                mforms.App.get().set_status_text("Resetting Performance Schema settings to default instrumentation...")
                sql = self.generate_updates_for_reset(instruments, consumers)
                print sql
            for s in sql:
                log_info("Executing %s...\n" % s)
                self.owner.main_view.editor.executeManagementCommand(s, 0)
            if state:
                mforms.App.get().set_status_text("Changed Performance Schema Settings")
            else:
                mforms.App.get().set_status_text("Disabled Performance Schema Settings")

            self.owner.rebuild_ui(False)
        except grt.DBError, e:
            log_error("MySQL error toggling (%s) PS Instrumentation\n" % e)
            mforms.App.get().set_status_text("Error toggling Performance Schema Instrumentation: %s" % e)
            mforms.Utilities.show_error("Toggle Performance Schema Instrumentation",
                                        "MySQL Error: %s" % e,
                                        "OK", "", "")



    def create_ui(self):
        self.logo = mforms.newImageBox()
        self.logo.set_image(mforms.App.get().get_resource_path("ps_easysetup_logo.png"))
        self.add(self.logo, False, True)

        self.switch_image = BigSwitch()
        self.switch_image.callback = self.change_instrumentation
        self.switch_image.set_size(200, 152)
        self.add(self.switch_image, False, True)

        image = mforms.newImageBox()
        image.set_image(mforms.App.get().get_resource_path("separator_vertical.png"))
        self.add(image, False, True)

        label = mforms.newLabel("\n\nThe MySQL Performance Schema allows you to:\n- instrument MySQL to collect statistics and performance data\n- log collected events into tables, so they can be analyzed\n\nUse the switch above to change Performance Schema instrumentation or disable it.")
        label.set_text_align(mforms.MiddleCenter)
        self.add(label, False, True)


    def reload(self):
        state = self.check_instrumentation_level()
        print state
        self.logo.set_image(mforms.App.get().get_resource_path("ps_easysetup_logo_enabled.png" if state != "disabled" else "ps_easysetup_logo.png"))
        self.switch_image.set_state(state)


class SetupInstruments(mforms.Box):
    def __init__(self, owner, data):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.set_spacing(12)
        self.set_padding(12)
        self.owner = owner
        self._data = data

        self.create_ui()

    def create_ui(self):
        l = mforms.newLabel("Select Enabled Instrumentation Points")
        l.set_style(mforms.BoldStyle)
        self.add(l, False, True)
      
        description = \
        """An instrument refers to a specific section of the MySQL code that has been enabled for monitoring.
When an instrument is enabled and is executed on the server, it will generate events than can be used for monitoring.

The top level elements on the table below represent the "Event Type" generated by the instrument.

The enabled column indicates whether the events for the instrument are generated.
The timed column indicates whether information about the event duration is recorded.
"""
        label = mforms.newLabel(description)
        self.add(label, False, True)
        
        self._instruments = mforms.newTreeView(mforms.TreeAltRowColors)
        self._instruments.add_column(mforms.IconStringColumnType, "Instrument", 400, False)
        self._instruments.add_column(mforms.TriCheckColumnType, "Enabled", 50, True)
        self._instruments.add_column(mforms.TriCheckColumnType, "Timed", 50, True)
        self._instruments.end_columns()
        self.add(self._instruments, True, True)

        self._instruments.set_cell_edited_callback(self.cell_edited)

        self.tag_models = {}

        self.insert_nodes(self._data.instruments)

    def refresh(self):
        """
        Reverts back any change on the UI that has not been committed to the database.
        """
        changes = self._data.get_changes()

        for item in changes.keys():
            tokens = item.split('/')
            node = self._instruments.root_node()
            for token in tokens:
                found = False
                index = 0
                while not found and index < node.count():
                    child = node.get_child(index)
                    if child.get_string(0) == token:
                        node = child
                        found = True
                    else:
                        index += 1

                if not found:
                    node = None

            if node:
                for change in changes[item]:
                    (att, old, new) = change
                    column = 1 if att == 'enabled' else 2
                    node.set_bool(column, old)

    def insert_nodes(self, model, parent = None):
        """
        Inserts nodes based on the model which is an instance
        of PSInstrumentGroup.
        """
        for child in model.keys():
            node = None
            tag = child
            if parent:
                node = parent.add_child()
                tag = '/'.join([parent.get_tag(), child])
            else: 
                node = self._instruments.add_node()

            # Creates the node
            node.set_int(1, model[child].enabled)
            node.set_int(2, model[child].timed)
            node.set_string(0, child)
            node.set_tag('%s' % tag)

            update_value_cb = lambda attr, value, node = node: self.model_value_set_callback(attr, value, node)
            model[child].set_value_set_notification(update_value_cb)

            # Fills the node/model directory
            self.tag_models[tag] = model[child]

            # Inserts children
            if not model[child].has_key('_data_'):
                self.insert_nodes(model[child], node)



    def model_value_set_callback(self, attr, value, node):
        """
        Callback to update the UI when an element of the
        model has been changed.
        """
        if attr == 'enabled':
            node.set_int(1, value)
        elif attr == 'timed':
            node.set_int(2, value)
        

    def cell_edited(self, node, column, value):
        """
        Callback used to updat the model when an element
        of the UI has been updated.
        """
        if column in [1,2]:
            # Gets the associated node's model
            model = self.tag_models[node.get_tag()]


            value = 1 if value == "1" else 0
            attr = "enabled" if column == 1 else "timed"

            # Performs the update at the model
            model.set_state(attr, value)

        else:
            node.set_string(column, value)


class SetupDataCollection(mforms.Box):
    def __init__(self, owner, data, variables):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.set_spacing(12)
        self.set_padding(12)

        self.owner = owner
        self._data = data
        self._variables = variables

        self.row_labels = []
        self.row_labels.append('Current events')
        self.row_labels.append('History (%s events)')
        self.row_labels.append('Long History (%s events)')

        self.variables = []
        self.variables.append('events_%s_current')
        self.variables.append('events_%s_history')
        self.variables.append('events_%s_history_long')

        self._controls = {}
        self._child_controls = {}

        self.create_ui()

    def update_data_collection(self, name, caller):
        is_active = caller.get_active()
        self._data[name].enabled = is_active

        # Enables/disables child controls based on paren's status
        if self._child_controls.has_key(caller):
            for child in self._child_controls[caller]:
                child.set_enabled(is_active)


    def create_dc_checkbox(self, container, var_lbl, parent = None):

        # Determines if this control should be enabled or not
        enabled = True
        if parent:
            enabled = parent.get_active()

        checkbox = mforms.newCheckBox()
        checkbox.set_text(var_lbl[1])
        checkbox.set_enabled(enabled)
        checkbox.set_active(self._data[var_lbl[0]].enabled)

        checkbox.add_clicked_callback(lambda: self.update_data_collection(var_lbl[0], checkbox))
        
        self._controls[var_lbl[0]] = checkbox

        # If the control has a parent, adds the relation
        if parent:
            if not self._child_controls.has_key(parent):
                self._child_controls[parent] = []
            
            self._child_controls[parent].append(checkbox)

        # Finally adds the control on the container
        container.add(checkbox, False, True)

        return checkbox


    def create_section_row(self, container, element, name, records, enabled = True):
        checkbox = mforms.newCheckBox()
        if records:
            caption =  self.row_labels[element] % self._variables['performance_schema_' + name + '_size'].value
        else:
            caption = self.row_labels[element]

        checkbox.set_text(caption)
        checkbox.set_enabled(bool(enabled))

        checkbox.set_active(self._data[name].enabled)

        checkbox.add_clicked_callback(lambda: self.update_data_collection(name, checkbox))
        self.ui_fields[name] = checkbox

        container.add(checkbox, False, True)

        return checkbox

    def get_var_name_and_label(self, base_name, index, var_prefix_name = ""):
        # Index equal to -1 indicates label should be retrieved from the varname
        if index == -1:

            var = base_name
            if var_prefix_name:
                var = '_'.join([var_prefix_name, base_name])

            tokens = base_name.split('_')
            caption = ' '.join(tokens).capitalize()
        # Index equal to 0 indicates data for a Current consumer was requested
        elif index == 0:
            var = self.variables[index] % base_name
            caption = self.row_labels[index]
        # Finally data for one of the history consumers has been requested
        else:
            var = self.variables[index] % base_name
            caption =  self.row_labels[index] % self._variables['performance_schema_' + var + '_size'].value

        return var, caption


    def create_section(self, container, base_name, elements = None):
        panel = mforms.newPanel(mforms.TitledBoxPanel)
        if elements:
            if base_name == 'events_waits_summary': #XXX where are these shown?
                panel.set_title('Wait Events Summary')
            elif base_name == 'file_summary':
                panel.set_title('File Summary')
        else:
            panel.set_title("%s Events" % base_name[:-1].capitalize())

        vbox = mforms.newBox(False)
        vbox.set_padding(8)
        vbox.set_spacing(8)

        if elements:
            for element in elements:
                self.create_dc_checkbox(vbox, self.get_var_name_and_label(element, -1, base_name))

        else:
            current_ctrl = self.create_dc_checkbox(vbox, self.get_var_name_and_label(base_name, 0))
            self.create_dc_checkbox(vbox, self.get_var_name_and_label(base_name, 1), current_ctrl)
            self.create_dc_checkbox(vbox, self.get_var_name_and_label(base_name, 2), current_ctrl)
        
        panel.add(vbox)

        container.add(panel, True, True)

        if self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 3):
            panel.set_enabled(self.thread_check.get_active())

            # Adds the panel as a child of the thread check
            self._child_controls[self.thread_check].append(panel)


    def create_ui(self):
        l = mforms.newLabel("Select performance_schema Event Types to be Collected")
        l.set_style(mforms.BoldStyle)
        self.add(l, False, True)

        if self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 3):
            description = "Performance Schema generates execution events only if a consumer for such events is active. A consumer is considered active if it is\n"\
                          "enabled and the consumers it depends on are active based on the next hierarchy:\n\n"\
                          "- Global Instrumentation is the top level consumer.\n"\
                          "- Thread Instrumentation and Statement Digest depend on Global Instrumentation.\n"\
                          "- The Current consumers for Statement, Stage and Wait events depend on Thread Instrumentation\n"\
                          "- The History and History Long consumers depend on it's associated Current consumer\n\n"\
                          "The options you toggle determine which of the performance_schema.events_* tables are fed with data"
        else:
            description = "Performance Schema generates execution events only if a consumer for such events is active."

        desc_label = mforms.newLabel(description)

        self.add(desc_label, False, True)

        # Container will be the main box itself if global instrumentation
        # is unavailable, but will be changed to an inner box otherwise
        # This to ease enabling/disabling the rest of the controls
        # based on Global Instrumentation state
        self.__container = self

        # Global Instrumentation was available up to 5.6
        if self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 0):
            self.global_check = self.create_dc_checkbox(self, self.get_var_name_and_label('global_instrumentation', -1))

            # Creates an inner box for the rest of the controls
            self.__container = mforms.newBox(False)
            self.add(self.__container, False, True)
            self._child_controls[self.global_check] = [self.__container]

        # Adds the thread instrumentation checkbox 
        if self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 3):
            self.thread_check = self.create_dc_checkbox(self.__container, self.get_var_name_and_label('thread_instrumentation', -1))
            self._child_controls[self.thread_check] = []

        # Adds the statements, stages and waits sections
        hbox = mforms.newBox(True)
        hbox.set_padding(12)
        hbox.set_spacing(12)

        # PS monitoring for statements and stages was available up to 5.6.3
        if self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 3):
            self.create_section(hbox, 'statements')
            self.create_section(hbox, 'stages')

        self.create_section(hbox, 'waits')
        if self.owner.target_version.is_supported_mysql_version_at_least(5, 7, 3):
            self.create_section(hbox, 'transactions')

        self.__container.add(hbox, False, True)

        if not self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 3):
            self.create_section(hbox, 'events_waits_summary', ['by_thread_by_event_name', 'by_event_name', 'by_instance'])
            self.create_section(hbox, 'file_summary', ['by_event_name', 'by_instance'])

        # Finally adds the statements digets controls
        # Statements digest was also available until 5.6.5
        if self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 5):
            digest_description = "Digesting normalizes statements in a way that permits grouping similar statements and collecting information about how often they occur.\n"
            digest_label = mforms.newLabel(digest_description)

            self.__container.add(digest_label, False, True)

            self.create_dc_checkbox(self.__container, self.get_var_name_and_label('statements_digest', -1))

    def refresh(self):
        """
        Reverts back any change on the UI that has not been committed to the database.
        """
        changes = self._data.get_changes()

        for item in changes.keys():
            # There's only one possible change
            (att, old, new) = changes[item][0]

            # This validation is needed cuz not all the consumers
            # have a checkbox on this screen
            if self._controls.has_key(item):
                item_ctrl = self._controls[item]

                item_ctrl.set_active(bool(old))

                if self._child_controls.has_key(item_ctrl):
                    for control in self._child_controls[item_ctrl]:
                        control.set_enabled(bool(old))
    
    def add_child_control(self, parent_id, child):
        parent = None

        if parent_id == 'thread':
            parent = self.thread_check

        if parent:
            self._child_controls[parent].append(child)
                        
class ActorInfoDialog(mforms.Form):
    def __init__(self, validate_cb = None):
        mforms.Form.__init__(self, None)
        self.set_title("Monitor Events for User")
        self._validate = validate_cb

        vbox = mforms.newBox(False)
        vbox.set_padding(20)
        vbox.set_spacing(18)

        caption = 'Define the user/host for which performance schema will monitor for events.\n'\
                  'You can use the % to indicate any user and/or any hosts.'

        l = mforms.newLabel(caption)
        vbox.add(l, False, True)

        table = mforms.newTable()
        table.set_padding(1)
        table.set_row_count(3)
        table.set_column_count(2)
        table.set_column_spacing(7)
        table.set_row_spacing(8)

        self.user = mforms.newTextEntry()
        table.add(mforms.newLabel("User:", True), 0, 1, 0, 1, mforms.HFillFlag)
        table.add(self.user, 1, 2, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)

        self.host = mforms.newTextEntry()
        table.add(mforms.newLabel("Host:", True), 0, 1, 1, 2, mforms.HFillFlag)
        table.add(self.host, 1, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)

        bbox = mforms.newBox(True)
        bbox.set_spacing(8)
        self.ok = mforms.newButton()
        self.ok.set_text("OK")

        self.cancel = mforms.newButton()
        self.cancel.set_text("Cancel")
        vbox.add(table, True, True)

        mforms.Utilities.add_end_ok_cancel_buttons(bbox, self.ok, self.cancel)

        vbox.add_end(bbox, False, True)

        self.set_content(vbox)

        self.set_size(500, 200)
        self.center()


    def run(self):
        while True:
            cancelled = True
            if self.run_modal(self.ok, self.cancel):
                cancelled = False
                user = self.user.get_string_value()
                host = self.host.get_string_value()
                error = self._validate(user, host)
                if error is None:
                    return (user, host)
                else:
                    mforms.Utilities.show_message("User/Host Definition Error", error, "Ok", "", "")
            # Only exists if the answer was valid
            # Or the user cancelled
            if cancelled or error is None:
                break
        return None


class ObjectInfoDialog(mforms.Form):
    def __init__(self, owner, validate_cb = None):
        mforms.Form.__init__(self, None)
        self.set_title("Monitor Events Database Object")
        self.owner = owner
        self._validate = validate_cb

        vbox = mforms.newBox(False)
        vbox.set_padding(20)
        vbox.set_spacing(18)

        caption = 'Define the database objects which performance schema will monitor for events.\n'\
                  'You can use the % to indicate any schema and/or any object name.'

        l = mforms.newLabel(caption)
        vbox.add(l, False, True)

        table = mforms.newTable()
        table.set_padding(1)
        table.set_row_count(5)
        table.set_column_count(4)
        table.set_column_spacing(7)
        table.set_row_spacing(8)

        self.type = mforms.newSelector()

        # Before 5.7.2 the only available value was TABLE
        if not self.owner.target_version.is_supported_mysql_version_at_least(5, 7, 2):
            self.type.add_item('TABLE')
            self.type.set_enabled(False)
        else:
            self.type.add_items(['EVENT', 'FUNCTION', 'PROCEDURE', 'TABLE', 'TRIGGER'])

        table.add(mforms.newLabel("Type:", True), 0, 1, 0, 1, mforms.HFillFlag)
        table.add(self.type, 1, 2, 0, 1, mforms.HFillFlag|mforms.HExpandFlag)

        self.schema = mforms.newTextEntry()
        table.add(mforms.newLabel("Schema:", True), 0, 1, 1, 2, mforms.HFillFlag)
        table.add(self.schema, 1, 2, 1, 2, mforms.HFillFlag|mforms.HExpandFlag)

        self.name = mforms.newTextEntry()
        table.add(mforms.newLabel("Name:", True), 0, 1, 2, 3, mforms.HFillFlag)
        table.add(self.name, 1, 2, 2, 3, mforms.HFillFlag|mforms.HExpandFlag)

        chbox = mforms.newBox(True)

        # Previous to 5.6.3 PS was enabled for all records matching an
        # object definition on the table, after 5.6.3 the field was included
        self.enabled = mforms.newCheckBox()
        self.enabled.set_text('Enabled')
        if not self.owner.target_version.is_supported_mysql_version_at_least(5, 6, 3):
            self.enabled.set_active(True)
            self.enabled.set_enabled(False)

        chbox.add(self.enabled, True, True)
        

        self.timed = mforms.newCheckBox()
        self.timed.set_text('Timed')
        chbox.add(self.timed, True, True)

        table.add(chbox, 1, 3, 3, 4, mforms.VFillFlag|mforms.HFillFlag|mforms.HExpandFlag)

        bbox = mforms.newBox(True)
        bbox.set_spacing(8)
        self.ok = mforms.newButton()
        self.ok.set_text("OK")

        self.cancel = mforms.newButton()
        self.cancel.set_text("Cancel")
        vbox.add(table, True, True)

        mforms.Utilities.add_end_ok_cancel_buttons(bbox, self.ok, self.cancel)

        vbox.add_end(bbox, False, True)

        self.set_content(vbox)

        self.set_size(500, 260)
        self.center()


    def run(self):
        while True:
            cancelled = True
            if self.run_modal(self.ok, self.cancel):
                cancelled = False
                type = self.type.get_string_value()
                schema = self.schema.get_string_value()
                name = self.name.get_string_value()
                enabled = self.enabled.get_active()
                timed = self.timed.get_active()
                error = self._validate(type, schema, name)
                if error is None:
                    return (type, schema, name, enabled, timed)
                else:
                    mforms.Utilities.show_message("Database Object Definition Error", error, "Ok", "", "")
            # Only exists if the answer was valid
            # Or the user cancelled
            if cancelled or error is None:
                break
        return None

class SetupFiltering(mforms.Box):
    def __init__(self, owner, actors, objects):
        mforms.Box.__init__(self, False)
        self.set_spacing(12)
        self.set_padding(12)

        self.set_managed()
        self.set_release_on_add()

        self.owner = owner
        self._actors = actors
        self._objects = objects
        
        self.create_ui()

    @property
    def target_version(self):
        return self.owner.target_version

    def make_description_box(self, text, tooltip):
        box = mforms.newBox(True)
        box.set_spacing(12)
        box.add(mforms.newLabel(text), False, True)
        l = mforms.newImageBox()
        l.set_image(mforms.App.get().get_resource_path("mini_notice.png"))
        l.set_tooltip(tooltip)
        box.add(l, False, True)
        return box

    def create_ui(self):
        l = mforms.newLabel("Filter Users and Objects to be Monitored")
        l.set_style(mforms.BoldStyle)
        self.add(l, False, True)

        user_panel = mforms.newPanel(mforms.TitledBoxPanel)
        user_panel.set_title('Users')

        users_box = mforms.newBox(False)
        users_box.set_spacing(12)
        users_box.set_padding(12)

        self.users = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors)
        self.users.add_column(mforms.StringColumnType, "User", 150, False)
        self.users.add_column(mforms.StringColumnType, "Host", 150, False)
        self.users.end_columns()
        self.users.add_changed_callback(self.selected_user_changed)
        users_box.add(self.users, True, True)

        user_buttons = mforms.newBox(True)
        user_buttons.set_spacing(12)
        
        description = ('Performance Schema allows defining filters to determine the connections for which data will be collected.\n'
                       'New connections having a user@host matching an entry below will be enabled for monitoring.')
        tooltip = 'Use % to indicate either any user or any host.'
        user_buttons.add(self.make_description_box(description, tooltip), False, True)

        self.user_del_button = mforms.newButton()
        self.user_del_button.set_text('Remove')
        self.user_del_button.add_clicked_callback(self.remove_user)
        self.user_del_button.set_enabled(False)
        user_buttons.add_end(self.user_del_button, False, True)

        user_add_button = mforms.newButton()
        user_add_button.set_text('Add')
        user_add_button.add_clicked_callback(self.add_user)
        user_buttons.add_end(user_add_button, False, True)

        users_box.add(user_buttons, False, True)
        
        user_panel.add(users_box)

        self.add(user_panel, True, True)

        # Database Objects Table
        db_panel = mforms.newPanel(mforms.TitledBoxPanel)
        db_panel.set_title('Database Objects')

        db_box = mforms.newBox(False)
        db_box.set_spacing(12)
        db_box.set_padding(12)
        
        self.objects = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors)
        self.objects.add_column(mforms.StringColumnType, "Type", 100, False)
        self.objects.add_column(mforms.StringColumnType, "Schema", 200, False)
        self.objects.add_column(mforms.StringColumnType, "Object", 200, False)
        self.objects.add_column(mforms.CheckColumnType, "Enabled", 50, False)
        self.objects.add_column(mforms.CheckColumnType, "Timed", 50, False)
        self.objects.end_columns()
        self.objects.add_changed_callback(self.selected_object_changed)
        self.objects.set_cell_edited_callback(self.object_edited)
        db_box.add(self.objects, True, True)

        db_buttons = mforms.newBox(True)
        db_buttons.set_spacing(12)
        
        description = ('Performance Schema allows defining filters to determine the objects for which data will be collected.\n' 
                       'Any schema/object matching a combination defined above will be enabled for monitoring.')
        
        tooltip = ('Use % to indicate either any schema or any object.\n\n'
                   'The enabled column indicates whether events for the matching objects are instrumented.\n'
                   'The timed column indicates whether information about the events duration is recorded.')
        
        
        db_buttons.add(self.make_description_box(description, tooltip), False, True)

        self.db_del_button = mforms.newButton()
        self.db_del_button.set_text('Remove')
        self.db_del_button.add_clicked_callback(self.remove_object)
        self.db_del_button.set_enabled(False)
        db_buttons.add_end(self.db_del_button, False, True)

        db_add_button = mforms.newButton()
        db_add_button.set_text('Add')
        db_add_button.add_clicked_callback(self.add_object)
        db_buttons.add_end(db_add_button, False, True)

        db_box.add(db_buttons, False, True)

        db_panel.add(db_box)

        
        self.add(db_panel, True, True)

        self.refresh()

    def refresh(self):
        self.users.clear()

        for item in self._actors:
            node = self.users.add_node()
            node.set_string(0, item.user)
            node.set_string(1, item.host)

        self.objects.clear()

        for item in self._objects:
            node = self.objects.add_node()
            node.set_string(0, item.type)
            node.set_string(1, item.schema)
            node.set_string(2, item.name)
            node.set_bool(3, item.enabled)
            node.set_bool(4, item.timed)

    def selected_user_changed(self):
        node = self.users.get_selected_node()
        enabled = node is not None

        self.user_del_button.set_enabled(enabled)

    def remove_user(self):
        node = self.users.get_selected_node()
        if not node is None:
            user = node.get_string(0)
            host = node.get_string(1)
            self._actors.remove(PSActor(user, host))
            node.remove_from_parent()

            self.selected_user_changed()

    def add_user(self):
        dlg = ActorInfoDialog(self.validate_user)
        actor = dlg.run()

        if actor:
            node = self.users.add_node()
            node.set_string(0, actor[0])
            node.set_string(1, actor[1])

            self._actors.append(PSActor(actor[0], actor[1]))

    def validate_user(self, user, host):
        error = None

        if user == '' and host == '':
            error = 'Both user and host are required fields'
        elif self._actors.count(PSActor(user, host)) != 0:
            error = 'The specified user/host is already setup for performance schema data collection.'

        return error

    def selected_object_changed(self):
        node = self.objects.get_selected_node()
        enabled = node is not None

        self.db_del_button.set_enabled(enabled)

    def object_edited(self, node, column, value):
        """
        This method will be used to enable/disable the instruments.
        """
        if column in [3,4]:
            # Not validating the possible assertion as the edited
            # object MUST exist on the list
            index = self._objects.index(PSObject(node.get_string(0), node.get_string(1), node.get_string(2), None, None))
            
            set = True if value == "1" else False

            if column == 3:
                self._objects[index].enabled = set
            else:
                self._objects[index].timed = set

            node.set_bool(column, set)  

    def remove_object(self):
        node = self.objects.get_selected_node()
        if not node is None:
            type = node.get_string(0)
            schema = node.get_string(1)
            name = node.get_string(2)
            self._objects.remove(PSObject(type, schema, name, None, None))
            node.remove_from_parent()

            self.selected_object_changed()

    def add_object(self):
        dlg = ObjectInfoDialog(self, self.validate_object)
        object = dlg.run()

        if object:
            node = self.objects.add_node()
            node.set_string(0, object[0])
            node.set_string(1, object[1])
            node.set_string(2, object[2])
            node.set_bool(3, object[3])
            node.set_bool(4, object[4])

            self._objects.append(PSObject(object[0], object[1], object[2], object[3], object[4]))

    def validate_object(self, type, schema, name):
        error = None

        if schema == '' or name == '':
            error = 'The next fields are required: object, name.'
        elif self._objects.count(PSObject(type, schema, name, None, None)) != 0:
            error = 'The specified database object is already setup for performance schema data collection.'

        return error


class SetupOptions(mforms.Box):
    def __init__(self, owner, timers, timer_types):
        mforms.Box.__init__(self, False)

        self.set_spacing(12)
        self.set_padding(12)

        self.set_managed()
        self.set_release_on_add()

        self._timer_types = timer_types

        self.owner = owner
        self._timers = timers
        self._timer_names = self._timers.keys()
        self._timer_names.sort()

        self._controls = {}
        self._descriptions = {}
        self._descriptions['CYCLE']= 'Occurs based on the CPU speed - frequency: %s, resolution: %s, overhead: %s'
        self._descriptions['TICK']= 'It is an established frequency on each platform - frequency: %s, resolution: %s, overhead: %s.'
        self._descriptions['MILLISECOND']= 'Occurs a thousand times in a second - frequency: %s, resolution: %s, overhead: %s.'
        self._descriptions['MICROSECOND']= 'Occurs a million times in a second - frequency: %s, resolution: %s, overhead: %s.'
        self._descriptions['NANOSECOND']= 'Occurs a billion times in a second - frequency: %s, resolution: %s, overhead: %s.'

        self.create_ui()


    def update_timer(self, name, caller):
        value = caller.get_string_value().split()[0]
        value = value[:-1].upper()
        self._timers[name].timer = value

    def get_timer_type_text(self, timer):
        description = self._descriptions[timer.name] % (timer.frequency, timer.resolution, timer.overhead)
        return "%ss        (%s)" % (timer.name.lower().capitalize(), description)
    
    def create_timer_row(self, table, offset):
        table.add(mforms.newLabel("%s Events" % (self._timer_names[offset].capitalize()), True), 0, 1, offset, offset + 1, mforms.HFillFlag)

        selector = mforms.newSelector(mforms.SelectorPopup)
        selector.add_items([self.get_timer_type_text(timer) for timer in self._timer_types])

        timer_name = self._timer_names[offset]
        ps_timer = self._timers[timer_name].timer

        index = self._timer_types.index(PSTimerType(ps_timer, None, None, None))

        selector.set_selected(index)
        selector.add_changed_callback(lambda: self.update_timer(self._timer_names[offset], selector))

        self._controls[timer_name] = selector

        table.add(selector, 1, 2, offset, offset + 1, mforms.VFillFlag|mforms.HFillFlag|mforms.HExpandFlag)

        
    def create_ui(self):
        panel = mforms.newPanel(mforms.TitledBoxPanel)
        panel.set_title("Event Timers")

        vbox = mforms.newBox(False)
        vbox.set_padding(12)

        description = "Instruments measure the duration of events, for which they can use different timers.\n"\
                      "A timer has characteristics that need to be considered when setting up the timer to be used on the\n"\
                      "different instruments:\n"\
                      "\n"\
                      "     - Frequency: Indicates the number of timer units per second.\n"\
                      "     - Resolution: Indicates the size used to increase a timer value at a time.\n"\
                      "     - Overhead: Minimal number of cycles of overhead to obtain one timing.\n"\
                      "\n"\
                      "Here you can configure which timer will be used for each instrument type.\n\n"
        vbox.add(mforms.newLabel(description), False, False)

        table = mforms.newTable()
        table.set_padding(1)
        table.set_row_count(5)
        table.set_column_count(2)
        table.set_column_spacing(7)
        table.set_row_spacing(8)

        for i in range(len(self._timer_names)):
            self.create_timer_row(table, i)

        vbox.add(table, False, True)
        
        panel.add(vbox)

        self.add(panel, False, True)

    def refresh(self):
        for name in self._controls:
            ps_timer = self._timers[name].timer
            index = self._timer_types.index(PSTimerType(ps_timer, None, None, None))
            self._controls[name].set_selected(index)


class IntroPage(mforms.ScrollPanel):
    def __init__(self, owner):
      mforms.ScrollPanel.__init__(self, 0)
      self.set_managed()
      self.set_release_on_add()

      self.box = mforms.newBox(False)
      self.box.set_spacing(12)
      self.box.set_padding(20)

      l = mforms.newLabel("Performance Schema Basics")
      l.set_style(mforms.BigBoldStyle)
      self.box.add(l, False, True)
      
      self.box.add(mforms.newLabel("""The performance schema collects data from various aspects of MySQL performance and gives
very detailed information about what exactly is happening inside your MySQL database server.
For each statement executed, the PS instruments will gather various statistics and timing information in different
levels of granularity and from different subsystems, from network to disk storage, and keep them in the 
performance_schema.events_* tables."""), False, True)

      image = mforms.newImageBox()
      image.set_image(mforms.App.get().get_resource_path("ps_overview.png"))
      self.box.add(image, False, True)

      label = mforms.newLabel("Configuring Performance Schema")
      label.set_style(mforms.BoldStyle)
      self.box.add(label, False, True)

      self.box.add(mforms.newLabel("""To control the trade-off between data collected and overhead, the performance schema gives
you a few fine grained configuration options.
 
You can configure what, when and how much will be instrumented by the Performance Schema by tweaking three option categories:

  * Actors - filters the users, hosts and DB objects to collect data for the performance_schema. This was introduced in MySQL 5.6.
    
  * Instruments - allow fine-tuning of what kind of stats are gathered for whatever is being monitored

  * Consumers - toggle which of the performance_schema.event_* tables should be filled

You can also use the simplified configuration interface for one-click setup of the performance schema for some common use cases.
"""), False, True)

      label = mforms.newLabel("Performance Schema Instrument Types")
      label.set_style(mforms.BoldStyle)
      self.box.add(label, False, True)

      self.box.add(mforms.newLabel("""The Performance Schema will monitor the following types of events that occur internally in the MySQL server, as client requests are executed.

  * Statement monitoring starts when a new request arrives into a server thread and ends once all processing is complete.
    These are logged in the event_statement_* tables.

  * Stage monitors the different stages that occur during the statement execution. Depends on Thread Instrumentation.
    These are logged in the event_stage_* tables.

  * Wait monitoring is focused on wait events that occur during the statement execution.
    These are logged in the event_wait_* tables.


For more information, refer to the MySQL manuals section on the Performance Schema.
"""), False, True)

      self.add(self.box)
        

class SetupThreads(mforms.Box):
    def __init__(self, data):
        mforms.Box.__init__(self, False)
        self.set_spacing(12)
        self.set_padding(12)

        self.set_managed()
        self.set_release_on_add()

        self._threads = data
        
        self.create_ui()

    def create_ui(self):
        l = mforms.newLabel("Threads to Instrument")
        l.set_style(mforms.BoldStyle)
        self.add(l, False, True)
        
        description = mforms.newLabel('Performance Schema allows enabling/disabling the monitoring of events that occur on specific threads in the server.\n\n'
            'Changes on the instrumentation status are only effective if Thread Instrumentation is enabled on the Consumers Tab.')

        self.add(description, False, False)

        self.threads = mforms.newTreeView(mforms.TreeFlatList|mforms.TreeAltRowColors)
        self.threads.add_column(mforms.LongIntegerColumnType, "Id", 50, False)
        self.threads.add_column(mforms.StringColumnType, "Name", 250, False)
        self.threads.add_column(mforms.CheckColumnType, "Instrumented", 80, True)
        self.threads.add_column(mforms.StringColumnType, "Type", 50, False)
        self.threads.add_column(mforms.StringColumnType, "Process Id", 80, False)
        self.threads.add_column(mforms.StringColumnType, "Account", 100, False)
        self.threads.add_column(mforms.StringColumnType, "Command", 100, False)
        self.threads.add_column(mforms.StringColumnType, "Time", 80, False)
        self.threads.add_column(mforms.StringColumnType, "State", 100, False)
        self.threads.add_column(mforms.StringColumnType, "Info", 100, False)
        self.threads.add_column(mforms.StringColumnType, "DB", 100, False)
        self.threads.end_columns()
        self.threads.set_cell_edited_callback(self.thread_edited)
        self.add(self.threads, True, True)

        self.refresh()

        bbox = mforms.newBox(True)
        bbox.set_spacing(12)
        
        self._enable_all = mforms.newButton()
        self._enable_all.set_text('Instrument All')
        self._enable_all.add_clicked_callback(self.enable_all)
        bbox.add(self._enable_all, False, True)

        self._disable_all = mforms.newButton()
        self._disable_all.set_text('Instrument None')
        self._disable_all.add_clicked_callback(self.enable_none)
        bbox.add(self._disable_all, False, True)

        self._refresh_button = mforms.newButton()
        self._refresh_button.set_text('Refresh')
        self._refresh_button.add_clicked_callback(self.refresh_data)

        bbox.add_end(self._refresh_button, False, True)
        self.add(bbox, False, True)

    def refresh_data(self):
        # Refreshes the model class from the database
        self._threads.refresh()

        # Refreshes the frontend
        self.refresh()
    
    def enable_all(self):
        root = self.threads.root_node()
        for row in range(self.threads.count()):
            node = root.get_child(row)
            self._threads[node.get_long(0)].instrumented = True
            node.set_bool(2, True)

    def enable_none(self):
        root = self.threads.root_node()
        for row in range(self.threads.count()):
            node = root.get_child(row)
            self._threads[node.get_long(0)].instrumented = False
            node.set_bool(2, False)

    def refresh(self):
        """
        Refreshes the UI based on the model class
        """
        self.threads.clear()

        items = self._threads.keys()
        items.sort()
        for item in items:
            node = self.threads.add_node()
            node.set_long(0, self._threads[item].id)
            node.set_string(1, self._threads[item].name)
            node.set_bool(2, self._threads[item].instrumented)
            if self._threads[item].thread_type == 'BACKGROUND':
                node.set_string(3, "BG")
            elif self._threads[item].thread_type == 'FOREGROUND':
                node.set_string(3, "FG")
            else:
                node.set_string(3, self._threads[item].thread_type)
            node.set_string(4, self._threads[item].plist_id)
            if self._threads[item].thread_type == 'BACKGROUND':
                node.set_string(5, "<system>")
            else:
                node.set_string(5, "%s@%s" % (self._threads[item].plist_user, self._threads[item].plist_host))
            node.set_string(6, self._threads[item].plist_command)
            node.set_string(7, self._threads[item].plist_time)
            node.set_string(8, self._threads[item].plist_state)
            node.set_string(9, self._threads[item].plist_info)
            node.set_string(10, self._threads[item].plist_db)

    def thread_edited(self, node, column, value):
        """
        This method will be used to enable/disable the instruments.
        """
        if column == 2:
            value = True if value == "1" else False
            self._threads[node.get_long(0)].instrumented = value
            
            node.set_bool(2, value)  


class WbAdminPerformanceSchemaInstrumentation(WbAdminPSBaseTab, ChangeCounter):
    min_server_version = (5,6,6)

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_performance", "Performance Schema Setup", False)
    
    @classmethod
    def identifier(cls):
        return "admin_instrumentation_setup"

    def __init__(self, ctrl_be, instance_info, main_view):
        WbAdminPSBaseTab.__init__(self, ctrl_be, instance_info, main_view)
        ChangeCounter.__init__(self)

        self.ctrl_be = ctrl_be
        self.content = None


    @property
    def target_version(self):
        return self.ctrl_be.target_version


    def count_change(self, change, attr, value):
        ChangeCounter.count_change(self, change, attr, value)
        
        # Enables/Disables the buttons accordingly
        self.apply_button.set_enabled(self.change_count != 0)
        #self.cancel_button.set_enabled(self.change_count)

    def create_ui(self):
        self.advanced_button = mforms.newButton()
        self.advanced_button.add_clicked_callback(self.switch_advanced)
        self.advanced_button.set_text("Show Advanced")
        self.create_basic_ui("title_instrumentation_setup.png", "Performance Schema - Setup", self.advanced_button)

        self.content = mforms.newBox(False)
        self.config_tab = mforms.TabView(mforms.TabViewSystemStandard)

        self.easy_setup = EasySetupPage(self)
        self.config_tab.add_page(self.easy_setup, "Easy Setup")
        self.content.add(self.config_tab, True, True)

        self.rebuild_ui(True)
        self.add(self.content, True, True)

        self._showing_advanced = True
        self.switch_advanced()


    def switch_config_tabs(self, flag):
        if flag:
            self.config_tab.add_page(self.intro, "Introduction")
            self.config_tab.add_page(self.instruments, "Instruments")
            self.config_tab.add_page(self.data_collection, "Consumers")
            if self.filtering:
                self.config_tab.add_page(self.filtering, "Actors & Objects")
            if self.threads:
                self.config_tab.add_page(self.threads, "Threads")
            self.config_tab.add_page(self.options, "Options")
        else:
            self.config_tab.remove_page(self.intro)
            self.config_tab.remove_page(self.instruments)
            self.config_tab.remove_page(self.data_collection)
            if self.filtering:
              self.config_tab.remove_page(self.filtering)
            if self.threads:
              self.config_tab.remove_page(self.threads)
            self.config_tab.remove_page(self.options)


    def switch_advanced(self):
        self._showing_advanced = not self._showing_advanced

        if self._showing_advanced:
            self.switch_config_tabs(True)
            self.apply_button.show(True)
            self.cancel_button.show(True)
        else:
            self.switch_config_tabs(False)
            self.apply_button.show(False)
            self.cancel_button.show(False)

        self.advanced_button.set_text("Hide Advanced" if self._showing_advanced else "Show Advanced")


    def rebuild_ui(self, from_scratch):
        if not from_scratch:
            self.switch_config_tabs(False)

        self.easy_setup.reload()

        # The configuration will be needed to create the UI
        self.config_data = PSConfiguration(self.ctrl_be)
        self.config_data.load()
        self.count_changes_on(self.config_data)

        self.intro = IntroPage(self)

        self.instruments = SetupInstruments(self, self.config_data.sections['instruments'])

        self.data_collection = SetupDataCollection(self, self.config_data.sections['consumers'], self.config_data.variables)


        # setup_actors and setup_objects was available up to 5.6
        if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6):
            self.filtering = SetupFiltering(self, self.config_data.sections['actors'], self.config_data.sections['objects'])
        else:
            self.filtering = None

        # setup_actors and setup_objects was available up to 5.6
        if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6):
            self.threads = SetupThreads(self.config_data.sections['threads'])
            
            self.data_collection.add_child_control('thread', self.threads)
        else:
            self.threads = None

        self.options = SetupOptions(self, self.config_data.sections['timers'], self.config_data.timer_types)


        if from_scratch:
            self.buttons = mforms.newBox(True)
            self.buttons.set_padding(6)
            self.buttons.set_spacing(12)

            self.reset_button = mforms.newButton()
            self.reset_button.set_tooltip("Resets/truncates all performance_schema event tables, deleting all performance schema data collected so far.")
            self.reset_button.set_text('Clear Event Tables')
            self.reset_button.add_clicked_callback(self.on_reset_button_clicked)
            self.buttons.add(self.reset_button, False, True)

            self.apply_button = mforms.newButton()
            self.apply_button.set_text('Apply')
            self.apply_button.set_tooltip("Apply changes to the performance_schema configurations to the MySQL server.")
            self.apply_button.add_clicked_callback(self.on_apply_button_clicked)
            self.apply_button.set_enabled(False)

            self.cancel_button = mforms.newButton()
            self.cancel_button.set_text('Revert')
            self.cancel_button.set_tooltip("Revert to the configuration currently saved in the MySQL server.")
            self.cancel_button.add_clicked_callback(self.on_revert_button_clicked)
            #self.cancel_button.set_enabled(False)

            self.buttons.add_end(self.apply_button, False, True)
            self.buttons.add_end(self.cancel_button, False, True)

            if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6):
                self.reset_to_factory_button = mforms.newButton()
                self.reset_to_factory_button.set_tooltip("Reset all performance_schema settings (instruments, threads, actors, objects etc) to factory defaults.")
                self.reset_to_factory_button.set_text('Full Reset to Factory Defaults')
                self.reset_to_factory_button.add_clicked_callback(self.on_reset_to_factory_button_clicked)
                self.buttons.add_end(self.reset_to_factory_button, False, True)

            self.content.add(self.buttons, False, True)

        if not from_scratch and self._showing_advanced:
            self.switch_config_tabs(True)


    def on_revert_button_clicked(self):
        # revert changes to DB state by rebuilding the UI
        tab = self.config_tab.get_active_tab()
        self.rebuild_ui(False)
        self.config_tab.set_active_tab(tab)


    def on_cancel_button_clicked(self):
        ## XXX unused for now, this only changes the UI state, when we need to reload the state from DB and
        # display, which is not currently supported

        # Reverts the changes at UI level
        self.refresh()
        self.instruments.refresh()
        self.data_collection.refresh()

        # Reverts the changes in the model
        self.config_data.revert_changes()

        # Reverts the changes on actors and objects
        # setup_actors and setup_objects was available up to 5.6
        if self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 6, 0):
            self.filtering.refresh()

        # Reverts the changes on options
        self.options.refresh()

        self.threads.refresh()

    def on_apply_button_clicked(self):
        # Commits the changes to the database
        self.config_data.commit_changes()

    def on_reset_button_clicked(self):
        if mforms.Utilities.show_message("Clear Event Tables",
                                         "performance_schema event summary and history tables will be cleared (truncated).\nPlease confirm.",
                                         "Clear Tables", "Cancel", "") == mforms.ResultOk:
            try:
                self.main_view.editor.executeManagementCommand('call sys.ps_truncate_all_tables(0)', 1)
            except Exception, e:
                log_error("Error calling sys.ps_truncate_all_tables(0): %s\n" % e)
                mforms.Utilities.show_error("Clear Event Tables", "An error occurred truncating performance_schema event tables.\n%s"%e, "OK", "", "")

    def on_reset_to_factory_button_clicked(self):
        if mforms.Utilities.show_message("Reset Performance Schema Settings",
                                         "All performance_schema instrumentation settings will be reset to factory configurations.\nPlease confirm.",
                                         "Reset", "Cancel", "") == mforms.ResultOk:
            try:
                self.main_view.editor.executeManagementCommand('call sys.ps_setup_reset_to_default(0)', 1)
            except Exception, e:
                log_error("Error calling sys.ps_reset_to_default(0): %s\n" % e)
                mforms.Utilities.show_error("Reset Performance Schema Settings", "An error occurred resetting performance_schema instrumentation settings.\n%s"%e, "OK", "", "")
                return
            self.on_revert_button_clicked()

    def refresh(self):
        pass


