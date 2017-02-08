# Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

import weakref

from mforms import newLabel
import mforms

#-------------------------------------------------------------------------------

def weakcb(object, cbname):
    """Create a callback that holds a weak reference to the object. When passing a callback
    for mforms, use this to create a ref to it and prevent circular references that are never freed.
    """
    def call(ref, cbname):
        callback = getattr(ref(), cbname, None)
        if callback is None:
            print "Object has no callback %s"%cbname
        else:
            return callback()

    return lambda ref=weakref.ref(object): call(ref, cbname)


not_running_warning_label_text = "There is no connection to the MySQL Server.\nThis functionality requires an established connection to a running MySQL server."
def not_running_warning_label():
    warning = newLabel("\n\n\n\n"+not_running_warning_label_text)
    warning.set_style(mforms.BigStyle)
    warning.set_text_align(mforms.MiddleCenter)
    warning.show(False)
    return warning

def no_remote_admin_warning_label(server_instance_settings):
    if server_instance_settings.uses_ssh:
        warning = newLabel("There is no SSH connection to the server.\nTo use this functionality, the server where MySQL is located must have a SSH server running\nand you must provide its login information in the server profile.")
    else:
        if server_instance_settings.uses_wmi:
            warning = newLabel("There is no WMI connection to the server.\nTo use this functionality, the server where MySQL is located must be configured to use WMI\nand you must provide its login information in the server profile.")
        else:
            warning = newLabel("Remote Administration is disabled.\nTo use this functionality, the server where MySQL is located must either have an SSH server running,\nor if it is a Windows machine, must have WMI enabled.\nAdditionally you must enable remote administration in the server profile, providing login details for it.")
    warning.set_style(mforms.BigStyle)
    warning.set_text_align(mforms.MiddleCenter)
    return warning


def make_panel_header(icon, title, subtitle, button=None):
    table = mforms.newTable()
    table.set_row_count(2)
    table.set_column_count(3)
    table.set_row_spacing(0)
    table.set_column_spacing(15)
    image = mforms.newImageBox()
    image.set_image(mforms.App.get().get_resource_path(icon))
    image.set_image_align(mforms.TopCenter)
    table.add(image, 0, 1, 0, 2, mforms.VFillFlag | mforms.HFillFlag)
    label = mforms.newLabel(title)
    label.set_text_align(mforms.BottomLeft)
    label.set_style(mforms.SmallStyle)
    table.add(label, 1, 2, 0, 1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
    label = mforms.newLabel(subtitle)
    label.set_text_align(mforms.TopLeft)
    label.set_style(mforms.VeryBigStyle)
    table.add(label, 1, 2, 1, 2, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
    if button:
        table.add(button, 2, 3, 0, 2, mforms.VFillFlag | mforms.HFillFlag)
    return table

def show_error_page(parent, text):

    if not hasattr(parent, '_error_label'):
        parent._error_label = None

    if parent._error_label is None:
        parent._error_label = mforms.newLabel(text)
        parent._error_label.set_style(mforms.BoldStyle)
        parent._error_label.set_text_align(mforms.MiddleCenter)
        parent.add(parent._error_label, True, True)

def remove_error_page_if_exists(parent):

    if not hasattr(parent, '_error_label'):
        parent._error_label = None

    if parent._error_label:
        parent.remove(parent._error_label)



class WbAdminBaseTab(mforms.Box):
    ui_created = False
    warning_panel = None
    content = None

    def __init__(self, ctrl_be, instance_info, main_view):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        
        self.set_spacing(12)
        
        self.instance_info = instance_info
        self.ctrl_be = ctrl_be
        self.main_view = main_view
        
        self.heading = None
        self.warning_panel = None
    
    
    @property
    def editor(self):
        return self.main_view.editor
    

    def create_basic_ui(self, icon, title, button=None):
        self.heading = make_panel_header(icon, self.instance_info.name, title, button)
        self.heading.set_padding(12)
        self.add(self.heading, False, True)
        
        self.content = None
    
    
    def page_activated(self):
        if self.warning_panel:
            self.remove(self.warning_panel)
            self.warning_panel = None

        button_data = None
        self.page_active = True
        if not self.ctrl_be.is_sql_connected():
            if self.content:
                self.content.show(False)
            text = ("There is no connection to the MySQL Server.", "This functionality requires an established connection to a running MySQL server to work.")
        else:
            text = None
          
        if text:
            title, text = text
            self.warning_panel = MessageButtonPanel("", title, text, button_data)
            self.add(self.warning_panel, True, True)
        else:
            if not self.ui_created:
              self.suspend_layout()
              self.create_ui()
              self.resume_layout()
              self.ui_created = True

    def get_select_int_result(self, query, column=0):
        res = self.main_view.editor.executeManagementQuery(query, 0)
        if res and res.goToFirstRow():
            return res.intFieldValue(column)
        return None


class MessageButtonPanel(mforms.Table):
    def __init__(self, icon, title, text, button1, button2=None):
        mforms.Table.__init__(self)
        self.set_managed()
        self.set_release_on_add()
        
        self.set_padding(-1) # center contents
        self.set_column_spacing(12)
        self.set_row_spacing(12)
        self.set_row_count(3)
        self.set_column_count(2)
        self._buttonBox = None
        if icon:
            image = mforms.newImageBox()
            image.set_image(mforms.App.get().get_resource_path(icon))
            self.add(image, 0, 1, 0, 3, 0)
        
        
        self._label = mforms.newLabel(title)
        self._label.set_style(mforms.BigBoldStyle)
        self._label.set_text_align(mforms.MiddleCenter)
        self.add(self._label, 1, 2, 0, 1, mforms.VFillFlag|mforms.HFillFlag)
        
        self.add(mforms.newLabel(text), 1, 2, 1, 2, mforms.VFillFlag|mforms.HFillFlag)

        if button1 or button2:
            self._buttonBox = mforms.newBox(True)
            self._buttonBox.set_spacing(12)

        if button1:
            button_caption, button_action = button1
            self._button = mforms.newButton()
            self._button.set_text(button_caption)
            self._button.add_clicked_callback(button_action)
            self._buttonBox.add_end(self._button, False, True)
        else:
            self._button = None

        if button2:
            button_caption, button_action = button2
            self._alt_button = mforms.newButton()
            self._alt_button.set_text(button_caption)
            self._alt_button.add_clicked_callback(button_action)
            self._buttonBox.add_end(self._alt_button, False, True)
        else:
            self._alt_button = None

        if button1 or button2:
            self.add(self._buttonBox, 1, 2, 2, 3, mforms.VFillFlag|mforms.HFillFlag)
