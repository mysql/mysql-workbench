# Copyright (c) 2007, 2021, Oracle and/or its affiliates.
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

import weakref
import sys

from mforms import newLabel
import mforms
from workbench.log import log_error, log_debug3

#-------------------------------------------------------------------------------

def weakcb(object, cbname):
    """Create a callback that holds a weak reference to the object. When passing a callback
    for mforms, use this to create a ref to it and prevent circular references that are never freed.
    """
    def call(ref, cbname):
        callback = getattr(ref(), cbname, None)
        if callback is None:
            print("Object has no callback %s"%cbname)
        else:
            return callback()

    return lambda ref=weakref.ref(object): call(ref, cbname)


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
    table.set_name(title + " Panel")
    table.set_row_count(2)
    table.set_column_count(3)
    table.set_row_spacing(0)
    table.set_column_spacing(15)
    image = mforms.newImageBox()
    image.set_name(title + " Icon")
    image.set_image(mforms.App.get().get_resource_path(icon))
    image.set_image_align(mforms.TopCenter)
    table.add(image, 0, 1, 0, 2, mforms.VFillFlag | mforms.HFillFlag)
    label = mforms.newLabel(title)
    label.set_name(title)
    label.set_text_align(mforms.BottomLeft)
    label.set_style(mforms.SmallStyle)
    table.add(label, 1, 2, 0, 1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
    label = mforms.newLabel(subtitle)
    label.set_name(subtitle)
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

class WbAdminValidationBase:
    def __init__(self, error_message = ""):
        self._error_message = error_message

    def validate(self):
        pass

    def set_error_message(self, error_message):
        self._error_message = error_message

    def errorScreen(self):
        warning = newLabel("\n\n\n\n" + self._error_message)
        warning.set_style(mforms.BigStyle)
        warning.set_text_align(mforms.MiddleCenter)
        warning.show(True)
        return warning


ValidationErrorServerNotRunning = "There is no connection to the MySQL Server.\nThis functionality requires an established connection to a running MySQL server."
ValidationErrorBadConfigFile = "Location of MySQL configuration file (ie: my.cnf) not specified"


class WbAdminValidationConnection(WbAdminValidationBase):
    def __init__(self, ctrl_be):
        super().__init__(ValidationErrorServerNotRunning)
        self._ctrl_be = ctrl_be

    def validate(self):
        return self._ctrl_be.is_sql_connected()


class WbAdminValidationConfigFile(WbAdminValidationBase):
    def __init__(self, instance_info):
        super().__init__(ValidationErrorBadConfigFile)
        self._instance_info = instance_info

    def validate(self):
        return self._instance_info.config_file_path

class WbAdminValidationRemoteAccess(WbAdminValidationBase):
    def __init__(self, instance_info):
        super().__init__()
        
        self._instance_info = instance_info

        if self._instance_info.uses_ssh:
            self.set_error_message("There is no SSH connection to the server.\nTo use this functionality, the server where MySQL is located must have an SSH server running\nand you must provide its login information in the server profile.")
        elif self._instance_info.uses_wmi:
            self.set_error_message("There is no WMI connection to the server.\nTo use this functionality, the server where MySQL is located must be configured to use WMI\nand you must provide its login information in the server profile.")
        else:
            self.set_error_message("Remote Administration is disabled.\nTo use this functionality, the server where MySQL is located must have either an SSH server running,\nor if it is a computer running Windows, must have WMI enabled.\nAdditionally, you must enable remote administration in the server profile and provide login details for it.")

    def validate(self):
        return self._instance_info.admin_enabled

#
# New features can be easily implemented, now...like:
#   - Auto page update
#       - Update the page periodically (like vars screen). Several pages will benefit from this
#       - Show error screen as soon as the validation don't pass and vice-versa
#   - Show loading screen before the UI page (good for slower tabs, like config file tab, maybe others)
#   - Force minimum height/width depending on the header/body/footer minimuns
#
class WbAdminTabBase(mforms.Box):
    def __init__(self, ctrl_be, instance_info, main_view):
        mforms.Box.__init__(self, False)

        self.set_managed()
        self.set_release_on_add()

        self._instance_info = instance_info
        self._ctrl_be = ctrl_be
        self._main_view = main_view

        self._page_active = False
        self._error_screen_displayed = False

        self._page_header = mforms.newBox(False)

        if sys.platform.lower() == "darwin": # No scrollbox on macOS as this is not needed and breaks selection.
                self._page_body = mforms.newBox(False)
        else:
                self._page_body = mforms.newScrollPanel()
        self._page_body.set_padding(8)

        self._page_footer = mforms.newBox(False)

        self.add(self._page_header, False, True)
        self.add(self._page_body, True, True)
        self.add(self._page_footer, False, True)

        self._header_contents = None
        self._body_contents = None
        self._footer_contents = None

         #if sys.platform.lower() != "darwin": # No scrollbox on macOS as this is not needed and breaks selection.
                #self._body_scroller = mforms.newScrollPanel()
                #self._page_body.add(self._body_scroller, True, True)

        self._validations = []

    @property
    def editor(self):
        return self._main_view.editor

    @property
    def backend(self):
        return self._ctrl_be

    @property
    def ctrl_be(self):
        return self._ctrl_be

    @property
    def instance_info(self):
        return self._instance_info

    @property
    def server_profile(self):
        return self._instance_info

    @property
    def main_view(self):
        return self._main_view

    def validate(self):
        for validation in self._validations:
            if not validation.validate():
                return validation
        return None

    def add_validation(self, validation):
        self._validations.append(validation)

    def create_standard_header(self, icon, title, description, rightViewObject = None):
        header = mforms.newTable()
        header.set_name(description + " Header")
        header.set_row_count(2)
        header.set_column_count(3)
        header.set_row_spacing(0)
        header.set_column_spacing(15)
        header.set_padding(12)
        header.set_name("Header")

        image = mforms.newImageBox()
        image.set_image(mforms.App.get().get_resource_path(icon))
        image.set_image_align(mforms.TopCenter)
        image.set_name(description + " Image")
        label1 = mforms.newLabel(title)
        label1.set_name(title)
        label1.set_text_align(mforms.BottomLeft)
        label1.set_style(mforms.SmallStyle)

        label2 = mforms.newLabel(description)
        label2.set_name(description)
        label2.set_text_align(mforms.TopLeft)
        label2.set_style(mforms.VeryBigStyle)

        header.add(image, 0, 1, 0, 2, mforms.VFillFlag | mforms.HFillFlag)
        header.add(label1, 1, 2, 0, 1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
        header.add(label2, 1, 2, 1, 2, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)

        if rightViewObject:
            header.add(rightViewObject, 2, 3, 0, 2, mforms.VFillFlag | mforms.HFillFlag)

        return header

    def set_header(self, header):
        if self._header_contents:
            self._page_header.remove(self._header_contents)
        self._header_contents = header
        self._header_contents.set_padding(10, 10, 10, 5)
        self._page_header.add(self._header_contents, True, True)

    def set_standard_header(self, icon, title, description):
        self.set_header(self.create_standard_header(icon, title, description))

    def set_body_contents(self, body_contents):
        if self._body_contents:
            if isinstance(self._page_body, mforms.Box):
              self._page_body.remove(self._body_contents)
            else:
              self._page_body.remove()

        self._body_contents = body_contents

        if sys.platform.lower() == "darwin":
                self._page_body.add(self._body_contents, True, True)
        else:
                self._page_body.add(self._body_contents)

    def set_error_screen(self, failedValidation):
        self.set_body_contents(failedValidation.errorScreen())
        self._error_screen_displayed = True

    def set_footer(self, footer):

        if self._footer_contents:
            self._page_footer.remove(self._footer_contents)
        self._footer_contents = footer
        self._footer_contents.set_name("Footer")
        self._footer_contents.set_padding(10, 5, 10, 10)
        self._page_footer.add(self._footer_contents, True, True)

    def ui_created(self):
        return False if self._body_contents is None else True

    def needs_to_create_ui(self):
        if self._error_screen_displayed:
            return True

        if not self._body_contents:
            return True

        return False

    def validation_failed_notification(self, failed_validation):
        pass

    def validation_successful_notification(self):
        pass

    def create_ui(self):
        pass

    def update_ui(self):
        pass

    def page_active(self):
        return self._page_active

    def page_activated(self):
        self._page_active = True
        try:
            self.suspend_layout()

            failed_validation = self.validate()

            if failed_validation:
                # create error screen
                self.validation_failed_notification(failed_validation)
                self.set_error_screen(failed_validation)
            elif self.needs_to_create_ui():
                self.validation_successful_notification()
                self._error_screen_displayed = False
                self.set_body_contents(self.create_ui())
            else:
                self.update_ui()
        except Exception as e:
            import traceback
            traceback.print_exc()
            log_error("Exception activating the page - %s" % str(e))
        finally:
            self.resume_layout()

    def page_deactivated(self):
        self._page_active = False


class MessageButtonPanel(mforms.Table):
    def __init__(self, icon, title, text, button1, button2=None):
        mforms.Table.__init__(self)
        self.set_managed()
        self.set_release_on_add()

        self.set_padding(-1) # center contents
        self.set_column_spacing(12)
        self.set_row_spacing(12)
        self.set_row_count(4)
        self.set_column_count(2)
        self._buttonBox = None
        if icon:
            image = mforms.newImageBox()
            image.set_image(mforms.App.get().get_resource_path(icon))
            self.add(image, 0, 1, 0, 3, 0)


        self._label = mforms.newLabel(title)
        self._label.set_style(mforms.BigBoldStyle)
        self._label.set_text_align(mforms.MiddleCenter)
        self.add(self._label, 1, 2, 0, 1, mforms.VFillFlag | mforms.HFillFlag)

        self.add(mforms.newLabel(text), 1, 2, 1, 2, mforms.VFillFlag | mforms.HFillFlag)

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
            self.add(self._buttonBox, 1, 2, 2, 3, mforms.VFillFlag | mforms.HFillFlag)
