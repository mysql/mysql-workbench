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

from mforms import newButton, newLabel, newBox, newCheckBox, newTextBox, Utilities
import mforms

from wb_common import dprint_ex, OperationCancelledError
import datetime
from wb_admin_utils import no_remote_admin_warning_label, make_panel_header

from wb_log_reader import ErrorLogFileReader
from workbench.notifications import nc

class WbAdminConfigurationStartup(mforms.Box):
    long_status_msg = None
    short_status_msg = None
    start_stop_btn = None
    offline_mode_btn = None
    startup_msgs_log = None
    is_server_running_prev_check = None
    copy_to_clipboard_button = None
    clear_messages_button = None
    ui_created = False
    error_log_reader = None

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_instance", "Startup / Shutdown", True)

    @classmethod
    def identifier(cls):
        return "admin_start_stop"

    #---------------------------------------------------------------------------
    def print_output_cb(self, text):
        self.ctrl_be.uitask(self.print_output, text)

    #---------------------------------------------------------------------------
    def print_output(self, text):
        ts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S - ")
        if self.startup_msgs_log:
            self.startup_msgs_log.append_text_with_encoding(ts + text + "\n", self.ctrl_be.server_helper.cmd_output_encoding, True)

    #---------------------------------------------------------------------------
    def __init__(self, ctrl_be, server_profile, main_view):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()
        self.main_view = main_view
        self.ctrl_be = ctrl_be
        self.ctrl_be.server_profile = server_profile

    #---------------------------------------------------------------------------
    @property
    def server_profile(self):
        return self.ctrl_be.server_profile

    #---------------------------------------------------------------------------
    @property
    def server_control(self):
        return self.ctrl_be.server_control

    #---------------------------------------------------------------------------
    def create_ui(self):
        self.suspend_layout()

        if not self.server_profile.admin_enabled:
            self.add(no_remote_admin_warning_label(self.server_profile), False, True)
            self.resume_layout()
            return

        self.set_padding(12)
        self.set_spacing(8)

        # Left pane (start/stop).
        self.heading = make_panel_header("title_startup.png", self.server_profile.name, "Startup / Shutdown MySQL Server")
        self.add(self.heading, False, True)

        self.add(newLabel(" "), False, True)

        self.long_status_msg = newLabel("The database server is stopped")
        self.long_status_msg.set_style(mforms.SmallStyle)

        status_message_part = newLabel("The database server instance is ")
        self.short_status_msg = newLabel("...")
        self.short_status_msg.set_color("#DD0000")

        self.start_stop_btn = newButton()
        self.start_stop_btn.set_text("Start server")
        self.start_stop_btn.add_clicked_callback(self.start_stop_clicked)
        
        self.offline_mode_btn = newButton()
        self.offline_mode_btn.set_text("Bring Offline")
        self.offline_mode_btn.add_clicked_callback(self.offline_mode_clicked)

        start_stop_hbox = newBox(True)
        start_stop_hbox.add(status_message_part, False, True)
        start_stop_hbox.add(self.short_status_msg, False, True)
        start_stop_hbox.add(self.start_stop_btn, False, True)
        start_stop_hbox.add(self.offline_mode_btn, False, True)

        if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 5):
            self.offline_mode_btn.show(True)
        else:
            self.offline_mode_btn.show(False)

        self.add(self.long_status_msg, False, True)
        self.add(start_stop_hbox, False, True)

        description = newLabel("If you stop the server, you and your applications will not be able to use the database and all current connections will be closed\n")
        description.set_style(mforms.SmallStyle)
        self.add(description, False, True)

        auto_start_checkbox = newCheckBox()
        auto_start_checkbox.set_text("Automatically Start Database Server on Startup")
        auto_start_checkbox.set_active(True)

        description = newLabel("You may select to have the Database server start automatically whenever the computer starts up.")
        description.set_style(mforms.SmallStyle)
        description.set_wrap_text(True)

        # Right pane (log).
        heading = newLabel("\nStartup Message Log")
        heading.set_style(mforms.BoldStyle)
        self.add(heading, False, True)

        self.startup_msgs_log = newTextBox(mforms.BothScrollBars)
        self.startup_msgs_log.set_name('StartupMessagesLog')
        self.startup_msgs_log.set_read_only(True)
        self.add(self.startup_msgs_log, True, True)

        button_box = newBox(True)
        self.refresh_button = newButton()
        self.refresh_button.set_size(150, -1)
        self.refresh_button.set_text("Refresh Status")
        self.refresh_button.add_clicked_callback(lambda:self.refresh(True))
        button_box.add(self.refresh_button, False, True)

        self.copy_to_clipboard_button = newButton()
        self.copy_to_clipboard_button.set_size(150, -1)
        self.copy_to_clipboard_button.set_text("Copy to Clipboard")
        self.copy_to_clipboard_button.add_clicked_callback(self.copy_to_clipboard)
        button_box.add_end(self.copy_to_clipboard_button, False, True)

        self.clear_messages_button = newButton()
        self.clear_messages_button.set_size(150, -1)
        self.clear_messages_button.set_text("Clear Messages")
        self.clear_messages_button.add_clicked_callback(self.clear_messages)
        button_box.add_end(self.clear_messages_button, False, True)
        self.add(button_box, False, True)

        self.resume_layout()

        self.ctrl_be.add_me_for_event("server_started", self)
        self.ctrl_be.add_me_for_event("server_offline", self)
        self.ctrl_be.add_me_for_event("server_stopped", self)

    #---------------------------------------------------------------------------
    def page_activated(self):
        if not self.server_profile.admin_enabled:
            self.add(no_remote_admin_warning_label(self.server_profile), True, True)
            return
        
        if not self.ui_created:
            self.create_ui()
            self.ui_created = True

        if self.server_control:
            self.server_control.set_output_handler(self.print_output_cb)

        if self.is_server_running_prev_check is None:
            self.is_server_running_prev_check = self.ctrl_be.is_server_running()
            self.update_ui(self.is_server_running_prev_check)
            self.print_new_error_log_entries()
        else:
            self.ctrl_be.query_server_info() 
            self.update_ui(self.ctrl_be.is_server_running())

    #---------------------------------------------------------------------------
    def server_started_event(self):
        dprint_ex(2, "Handling server start event in start/stop page")
        self.update_ui("running")

    #---------------------------------------------------------------------------
    def server_offline_event(self):
        dprint_ex(2, "Handling server offline event in start/stop page")
        self.update_ui("offline")

    #---------------------------------------------------------------------------
    def server_stopped_event(self):
        dprint_ex(2, "Handling server stop event in start/stop page")
        self.update_ui("stopped")

    #---------------------------------------------------------------------------
    def update_ui(self, server_status):
        dprint_ex(3, "server_status on enter is %s" % str(server_status))
        
        if not self.server_profile.admin_enabled:
            return

        self.is_server_running_prev_check = server_status
        if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 5):
            self.offline_mode_btn.show(True)
        else:
            self.offline_mode_btn.show(False)
                
        if server_status in ("running", "starting", "offline"):
            if server_status == "starting":
                self.long_status_msg.set_text("The database server is starting...")
                self.start_stop_btn.set_enabled(False)
                self.short_status_msg.set_color("#DDCC00")
                self.offline_mode_btn.set_text("Bring Offline")
                self.offline_mode_btn.set_enabled(False)
            elif server_status == "offline":
                self.offline_mode_btn.set_text("Bring Online")
                self.offline_mode_btn.set_enabled(True)
                server_status = "offline"
                self.short_status_msg.set_color("#0000B0")
                self.long_status_msg.set_text("The database server is in offline mode. To put it back into online mode, use the \"Online mode\" button")
            else:
                self.offline_mode_btn.set_enabled(True)
                self.offline_mode_btn.set_text("Bring Offline")
                self.start_stop_btn.set_enabled(True)
                self.short_status_msg.set_color("#00B000")
                self.long_status_msg.set_text("The database server is started and ready for client connections. To shut the server down, use the \"Stop Server\" button")
            self.short_status_msg.set_text(server_status)
            self.start_stop_btn.set_text("Stop Server")
        elif server_status in ("stopped", "stopping"):
            if server_status == "stopping":
                self.long_status_msg.set_text("The database server is stopping...")
                self.start_stop_btn.set_enabled(False)
                self.offline_mode_btn.set_enabled(False)
                self.short_status_msg.set_color("#DDCC00")
            else:
                self.start_stop_btn.set_enabled(True)
                self.offline_mode_btn.set_enabled(False)
                self.short_status_msg.set_color("#B00000")
                self.long_status_msg.set_text("The database server is stopped. To start the Server, use the \"Start Server\" button")
            self.short_status_msg.set_text(server_status)
            self.start_stop_btn.set_text("Start Server")
        else:
            self.long_status_msg.set_text("The state of the database server could not be determined. Please verify server profile settings.")
            self.short_status_msg.set_text("unknown")
            self.short_status_msg.set_color("#FF0000")
            self.start_stop_btn.set_text("Start Server")
            self.start_stop_btn.set_enabled(False)

        self.relayout()
        dprint_ex(3, "Leave")


    #---------------------------------------------------------------------------
    def start_error_log_tracking(self):
        if self.ctrl_be.server_profile.error_log_file_path:
            try:
                self.error_log_reader = ErrorLogFileReader(self.ctrl_be, self.server_profile.error_log_file_path)
                self.error_log_position = self.error_log_reader.file_size
            except OperationCancelledError, e:
                self.startup_msgs_log.append_text_with_encoding("Cancelled password input to open error log file: %s\n" % e,
                                                              self.ctrl_be.server_helper.cmd_output_encoding, True)
                raise
            except Exception, e:
                self.startup_msgs_log.append_text_with_encoding("Could not open error log file: %s\n" % e,
                                                                self.ctrl_be.server_helper.cmd_output_encoding, True)

    #---------------------------------------------------------------------------
    def print_new_error_log_entries(self):
        if self.error_log_reader:
            self.error_log_reader.refresh()
            if self.error_log_position != self.error_log_reader.file_size:
                self.error_log_reader.chunk_start = self.error_log_position
                self.error_log_reader.chunk_end = self.error_log_reader.file_size
                self.error_log_position = self.error_log_reader.file_size
                records = self.error_log_reader.current()
                if records:
                    self.startup_msgs_log.append_text_with_encoding('\nFROM %s:\n' % self.server_profile.error_log_file_path,
                                                                    self.ctrl_be.server_helper.cmd_output_encoding, True)
                    self.startup_msgs_log.append_text_with_encoding('    '+'\n    '.join( ["  ".join(line) for line in records]) + '\n',
                                                                    self.ctrl_be.server_helper.cmd_output_encoding, True)


    #---------------------------------------------------------------------------
    def start_stop_clicked(self):
        try:
            self.start_error_log_tracking()
        except OperationCancelledError:
            # we could abort everything if we knew that start/stop server also needs sudo password
            # to avoid user having to cancel that twice, but since we're not sure if the start/stop will
            # indeed require the sudo password, we can't give up yet
            pass
        status = self.ctrl_be.is_server_running(verbose=1)
        # Check if server was started/stoped from outside
        if self.is_server_running_prev_check == status:
            if status == "running" or status == "offline":
                if status == "offline":
                    self.print_output("Server is in offline mode.")
                self.start_stop_btn.set_enabled(False)
                self.refresh_button.set_enabled(False)

                try:
                    if self.server_control and not self.server_control.stop_async(self.async_stop_callback, True):
                        if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 5):
                            self.offline_mode_btn.show(True)

                        self.start_stop_btn.set_enabled(True)
                        self.refresh_button.set_enabled(True)
                        return
                except Exception, exc:
                    if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 5):
                        self.offline_mode_btn.show(True)

                    self.start_stop_btn.set_enabled(True)
                    self.refresh_button.set_enabled(True)
                    Utilities.show_error("Stop Server",
                              "An error occurred while attempting to stop the server.%s %s\n" % (type(exc).__name__, exc),
                              "OK", "", "")
                    return
            elif status == "stopped":
                self.start_stop_btn.set_enabled(False)
                self.refresh_button.set_enabled(False)
                self.offline_mode_btn.set_enabled(False)

                try:
                    if self.server_control and not self.server_control.start_async(self.async_start_callback, True):
                        self.start_stop_btn.set_enabled(True)
                        self.refresh_button.set_enabled(True)
                        return
                except Exception, exc:
                    self.start_stop_btn.set_enabled(True)
                    self.refresh_button.set_enabled(True)
                    Utilities.show_error("Start Server",
                              "An error occurred while attempting to start the server.%s %s\n" % (type(exc).__name__, exc),
                              "OK", "", "")
                    return

            elif status == "stopping":
                self.print_output("Server is stopping; please wait...")
            elif status == "starting":
                self.print_output("Server is starting; please wait...")
            else:
                self.print_output("Unable to detect server status.")
            self.refresh()

            if self.ctrl_be.target_version and self.ctrl_be.target_version.is_supported_mysql_version_at_least(5, 7, 5):
                self.offline_mode_btn.show(True)

    def offline_mode_clicked(self):
        info = { "state" : -1, "connection" : self.ctrl_be.server_profile.db_connection_params }
        if self.ctrl_be.is_server_running() == "offline":
            self.ctrl_be.exec_query("SET GLOBAL offline_mode = off")
            self.ctrl_be.event_from_main("server_started")
            info['state'] = 1
        else:
            self.ctrl_be.exec_query("SET GLOBAL offline_mode = on")
            self.ctrl_be.event_from_main("server_offline")
            info['state'] = -1
            
        #we need to send notification that server state has changed,
        nc.send("GRNServerStateChanged", self.ctrl_be.editor, info)
             

    #---------------------------------------------------------------------------
    def async_stop_callback(self, status):
        self.ctrl_be.uitask(self.async_stop_finished, status)

    #---------------------------------------------------------------------------
    def async_stop_finished(self, status): # status can be one of success, bad_password or error message
        if status == "success":
            self.print_output("Server stop done.")
        elif status == "bad_password":
            r = Utilities.show_error("Stop Server",
                              "A permission error occurred while attempting to stop the server.\n"
                              "Administrator password was possibly wrong.",
                              "Retry", "Cancel", "")
            if r == mforms.ResultOk:
                if self.server_control.stop_async(self.async_stop_callback):
                    return
            else:
                self.print_output("Could not stop server. Permission denied")
        elif status == "need_password":
            if self.server_control.stop_async(self.async_stop_callback, False):
                return
        else:
            self.print_output("Could not stop server: %s" % (status or "unknown error"))
            Utilities.show_error("Could not stop server", str(status), "OK", "", "")
        self.refresh()
        self.refresh_button.set_enabled(True)
        self.start_stop_btn.set_enabled(True)
        self.print_new_error_log_entries()
        #self.error_log_reader = None


    #---------------------------------------------------------------------------
    def async_start_callback(self, status):
        self.ctrl_be.uitask(self.async_start_finished, status)

    #---------------------------------------------------------------------------
    def async_start_finished(self, status):
        if status == "success":
            self.ctrl_be.event_from_main("server_started")
            self.print_output("Server start done.")
        elif status == "bad_password":
            r = Utilities.show_error("Start Server",
                              "A permission error occurred while attempting to start the server.\n"
                              "Administrator password was possibly wrong.",
                              "Retry", "Cancel", "")
            if r == mforms.ResultOk:
                self.server_control.start_async(self.async_start_callback)
                return
            else:
                self.print_output("Could not stop server. Permission denied")
        elif status == "need_password":
            self.server_control.start_async(self.async_start_callback, False)
            return
        else:
            self.print_output("Could not start server: %s" % (status or "unknown error"))
            Utilities.show_error("Could not start server", str(status), "OK", "", "")
        self.refresh()
        self.refresh_button.set_enabled(True)
        self.start_stop_btn.set_enabled(True)
        self.print_new_error_log_entries()
        #self.error_log_reader = None

    #---------------------------------------------------------------------------
    def refresh(self, verbose=False):
        new_state = self.ctrl_be.force_check_server_state(verbose=verbose)
        if new_state:
            self.is_server_running_prev_check = new_state
            self.update_ui(self.is_server_running_prev_check)
            self.print_new_error_log_entries()

    #---------------------------------------------------------------------------
    def copy_to_clipboard(self):
        Utilities.set_clipboard_text(self.startup_msgs_log.get_string_value())

    #---------------------------------------------------------------------------
    def clear_messages(self):
        self.startup_msgs_log.clear()
