# Copyright (c) 2007, 2016, Oracle and/or its affiliates. All rights reserved.
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

from __future__ import with_statement
from workbench.log import log_info, log_error, log_warning, log_debug3

from workbench.utils import format_duration, Version
from workbench.db_utils import QueryError
import mforms
import time
import wb_admin_monitor

def stradd(table, y, label, value):
    t = mforms.newLabel(label)
    table.add(t, 0, 1, y, y+1, mforms.VFillFlag|mforms.HFillFlag)

    t = mforms.newLabel(value)
    t.set_style(mforms.BoldStyle)
    t.set_color("#555555")
    table.add(t, 1, 2, y, y+1, mforms.VFillFlag|mforms.HFillFlag)
    return t



class StateIcon(mforms.Box):
    on_icon = None
    off_icon = None

    def __init__(self):
        mforms.Box.__init__(self, True)
        self.set_release_on_add()
        self.set_managed()

        if not self.on_icon:
            self.on_icon = mforms.App.get().get_resource_path("mysql-status-on.png")
            self.off_icon = mforms.App.get().get_resource_path("mysql-status-off.png")

        self.set_spacing(8)
        self.image = mforms.newImageBox()
        self.image.set_image(self.off_icon)
        self.add(self.image, False, True)

        self.label = mforms.newLabel("n/a")
        self.add(self.label, False, True)

        self.text = None


    def set_state(self, state):
        if state:
            self.image.set_image(self.on_icon)
            self.label.set_text("On")
        elif state is None:
            self.image.set_image(self.off_icon)
            self.label.set_text("n/a")
        else:
            self.image.set_image(self.off_icon)
            self.label.set_text("Off")


    def set_text(self, text):
        if not self.text:
            self.text = mforms.newLabel(text)
            self.text.set_style(mforms.BoldStyle)
            self.text.set_color("#555555")
            self.add(self.text, False, True)
        else:
            self.text.set_text(text)



class ConnectionInfo(mforms.Box):
    def __init__(self, owner):
        mforms.Box.__init__(self, True)
        self.set_release_on_add()
        self.set_managed()
        
        self.owner = owner

        self.set_spacing(35)

        self.icon = mforms.newImageBox()
        self.icon.set_image(mforms.App.get().get_resource_path("mysql-logo-00.png"))

        self.add(self.icon, False, True)

        vbox = mforms.newBox(False)
        self.vbox = vbox
        self.add(vbox, True, True)
        vbox.set_spacing(2)
        vbox.add(mforms.newLabel("Connection Name"), False, True)

        self.connection_name = mforms.newLabel("?")
        self.connection_name.set_style(mforms.VeryBigStyle)
        vbox.add(self.connection_name, False, True)

        self.info_table = None


    def update(self, ctrl_be):
        self.suspend_layout()
        self.connection_name.set_text(ctrl_be.server_profile.name)

        info = ctrl_be.server_variables
        status = ctrl_be.status_variables

        if self.info_table:
            self.vbox.remove(self.info_table)

        self.info_table = mforms.newTable()
        self.info_table.set_column_count(2)
        self.info_table.set_row_count(8)
        self.info_table.set_column_spacing(18)
        self.info_table.set_row_spacing(5)

        stradd(self.info_table, 0, "\nHost:", "\n"+info.get("hostname", "n/a"))
        stradd(self.info_table, 1, "Socket:", info.get("socket", "n/a"))
        stradd(self.info_table, 2, "Port:", info.get("port", "n/a"))
        stradd(self.info_table, 3, "Version:", "%s\n%s" % (info.get("version", "n/a"), info.get("version_comment", "")))
        stradd(self.info_table, 4, "Compiled For:", "%s   (%s)" % (info.get("version_compile_os", "n/a"), info.get("version_compile_machine", "n/a")))

        stradd(self.info_table, 5, "Configuration File:", ctrl_be.server_profile.config_file_path or "unknown")

        uptime = status.get("Uptime", None)
        if uptime:
            uptime = long(uptime)
            stradd(self.info_table, 6, "Running Since:", "%s (%s)" % (time.ctime(ctrl_be.status_variables_time-uptime), format_duration(uptime, True)))
        else:
            stradd(self.info_table, 6, "Running Since:", "n/a")
        self.vbox.add(self.info_table, True, True)

        box = mforms.newBox(True)
        refresh = mforms.newButton()
        refresh.set_text("Refresh")
        refresh.set_tooltip("Refresh server status information")
        refresh.add_clicked_callback(self.owner.refresh_status)
        box.add(refresh, False, False)
        self.info_table.add(box, 1, 2, 7, 8, mforms.VFillFlag)

        version = ctrl_be.target_version
        if version and info:
            icon = mforms.App.get().get_resource_path("mysql-logo-%i%i.png" % (version.majorNumber, version.minorNumber))
            if icon:
                self.icon.set_image(icon)
        self.resume_layout()



#===============================================================================
#
#===============================================================================
class WbAdminServerStatus(mforms.Box):
    status      = None
    connections = None
    _update_timeout = None

    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_management", "Server Status", False)

    @classmethod
    def identifier(cls):
        return "admin_server_status"


    #---------------------------------------------------------------------------
    def __init__(self, ctrl_be, server_profile, main_view):
        mforms.Box.__init__(self, True)
        self.set_managed()
        self.set_release_on_add()

        self.ui_created = False

        self.set_spacing(24)

        self.ctrl_be = ctrl_be
        self.server_profile = server_profile
        self.main_view = main_view

        lbox = mforms.newBox(False)
        self.add(lbox, True, True)

        self.connection_info = ConnectionInfo(self)
        self.connection_info.set_padding(24)
        lbox.add(self.connection_info, False, True)

        self.scrollbox = mforms.newScrollPanel(mforms.ScrollPanelDrawBackground)
        self.scrollbox.set_padding(24)
        self.content = mforms.newBox(False)
        self.content.set_padding(20)
        self.content.set_spacing(4)
        self.scrollbox.add(self.content)
        lbox.add(self.scrollbox, True, True)

        image = mforms.newImageBox()
        if self.server_profile.host_os == "linux":
            image.set_image(mforms.App.get().get_resource_path("mysql-status-separator-linux.png"))
        else:
          image.set_image(mforms.App.get().get_resource_path("mysql-status-separator.png"))
        image.set_image_align(mforms.MiddleCenter)
        self.add(image, False, True)

        self.status = wb_admin_monitor.WbAdminMonitor(server_profile, self.ctrl_be)
        self.status.set_size(360, -1)
        self.status.set_padding(0, 24, 24, 24)
        self.add(self.status, False, True)

        self.controls = {}

        self.currently_started = None
        self.ctrl_be.add_me_for_event("server_started", self)
        self.ctrl_be.add_me_for_event("server_offline", self)
        self.ctrl_be.add_me_for_event("server_stopped", self)

        self.connection_info.update(self.ctrl_be)

    #---------------------------------------------------------------------------
    def server_started_event(self):
        if self.currently_started != True:
            self.refresh("started")
            self.currently_started = True
            if not self._update_timeout:
                self._update_timeout = mforms.Utilities.add_timeout(0.5, self.update)

    def server_offline_event(self):
        if self.currently_started != True:
            self.refresh("offline")
            self.currently_started = True
            if not self._update_timeout:
                self._update_timeout = mforms.Utilities.add_timeout(0.5, self.update)
    #---------------------------------------------------------------------------
    def server_stopped_event(self):
        if self.currently_started != False:
            self.refresh("stopped")
            self.currently_started = False
            if not self._update_timeout:
                self._update_timeout = mforms.Utilities.add_timeout(0.5, self.update)

    #---------------------------------------------------------------------------
    def refresh(self, status):
        self.status.refresh_status(status)


    #---------------------------------------------------------------------------
    def refresh_status(self):
        if not self._update_timeout:
            status = self.ctrl_be.force_check_server_state()
            if (status == "running" or not status  or status == "offline" ) and self.currently_started:
                self.ctrl_be.query_server_info()

            self._update_timeout = mforms.Utilities.add_timeout(0.5, self.update)


    #---------------------------------------------------------------------------
    def page_activated(self):
        self.suspend_layout()
        try:
            if not self.ui_created:
                self.create_info_sections()
                self.ui_created = True

                if not self._update_timeout:
                    self._update_timeout = mforms.Utilities.add_timeout(0.5, self.update)
        finally:
            self.resume_layout()

        if self.currently_started is None:
            if self.ctrl_be.is_server_running() == "running":
                self.server_started_event()
            elif self.ctrl_be.is_server_running() == "offline":
                self.server_offline_event()
            else:
                self.server_stopped_event()
        else:
            self.ctrl_be.query_server_info()

        self.refresh(self.ctrl_be.is_server_running())

    def update(self):
        self._update_timeout = None
        self.connection_info.update(self.ctrl_be)
        self.status.refresh_status(self.ctrl_be.is_server_running(verbose=False))
        info = self.ctrl_be.server_variables
        status = self.ctrl_be.status_variables
        plugins = dict(self.ctrl_be.server_active_plugins) # plugin -> type

        repl_error = None
        res = None
        try:
            res = self.ctrl_be.exec_query("SHOW SLAVE STATUS")
        except QueryError, e:
            if e.error == 1227:
                repl_error = "Insufficient privileges to view slave status"
            else:
                repl_error = "Error querying status: %s" % str(e)
        repl = {}
        if res and res.nextRow():
            for field in ["Slave_IO_State", "Master_Host"]:
                repl[field] = res.stringByName(field)

        disk_space = "unable to retrieve"
        if self.ctrl_be.server_control and info.get("datadir"):
            disk_space = self.ctrl_be.server_helper.get_available_space(info.get("datadir"))

        # Update the controls in the UI
        self.suspend_layout()
        self.controls["Disk Space in Data Dir:"][0].set_text(disk_space)

        table = self.controls["Replication Slave"][0]

        if repl:
            table.remove(self.controls[""][0])
            self.setup_info_table(table,
                                  [("Slave IO State:", repl.get("Slave_IO_State")),
                                   ("Master Host:", repl.get("Master_Host")),
                                   ("GTID Mode:", info.get("gtid_mode"))],
                                  plugins)
        else:
            self.controls[""][0].set_text(repl_error or "this server is not a slave in a replication setup")
        table.relayout()

        for key, (control, value_source) in self.controls.items():
            if callable(value_source):
                if isinstance(control, mforms.Label):
                    resp = value_source(info, plugins, status)
                    control.set_text(resp if resp else "n/a")
                else:
                    value = value_source(info, plugins, status)
                    if type(value) is tuple:
                        control.set_state(value[0])
                        if value[0] and value[1]:
                            control.set_text(value[1])
                    else:
                        control.set_state(value)

        self.resume_layout()

        mforms.Utilities.driver_shutdown()
        return False

    def create_info_sections(self):
        info = self.ctrl_be.server_variables
        status = self.ctrl_be.status_variables
        plugins = dict(self.ctrl_be.server_active_plugins) # plugin -> type

        repl = {}
        disk_space = "checking..."

        def tristate(value, true_value = None):
            if true_value is not None and value == true_value:
                return True
            if value == "OFF" or value == "NO":
                return False
            elif value and true_value is None:
                return True
            return None

        semi_sync_master = tristate(info.get("rpl_semi_sync_master_enabled"))
        semi_sync_slave = tristate(info.get("rpl_semi_sync_slave_enabled"))
        semi_sync_status = (semi_sync_master or semi_sync_slave, "(%s)"% ", ".join([x for x in [semi_sync_master and "master", semi_sync_slave and "slave"] if x]))
        memcached_status = True if plugins.has_key('daemon_memcached') else None
        
        if not repl:
            if semi_sync_master:
                semi_sync_master = False
            if semi_sync_slave:
                semi_sync_slave = False

        # the params to be passed to the lambdas
        params = (info, plugins, status)

        self.add_info_section_2("Available Server Features",
                              [("Performance Schema:", lambda info, plugins, status: tristate(info.get("performance_schema"))),
                               ("Thread Pool:", lambda info, plugins, status: tristate(info.get("thread_handling"), "loaded-dynamically")),
                               ("Memcached Plugin:", lambda info, plugins, status: memcached_status),
                               ("Semisync Replication Plugin:", lambda info, plugins, status: semi_sync_status),
                               ("SSL Availability:", lambda info, plugins, status: info.get("have_openssl") == "YES" or info.get("have_ssl") == "YES"),
                               ("Windows Authentication:", lambda info, plugins, status: plugins.has_key("authentication_windows")) if self.server_profile.target_is_windows else ("PAM Authentication:", lambda info, plugins, status: plugins.has_key("authentication_pam")),
                               ("Password Validation:", lambda info, plugins, status: (tristate(info.get("validate_password_policy")), "(Policy: %s)" % info.get("validate_password_policy"))),
                               ("Audit Log:", lambda info, plugins, status: (tristate(info.get("audit_log_policy")), "(Log Policy: %s)" % info.get("audit_log_policy"))),
                               ("Firewall:", lambda info, plugins, status: tristate(info.get("mysql_firewall_mode"))),
                               ("Firewall Trace:", lambda info, plugins, status: tristate(info.get("mysql_firewall_trace")))],
                                params)

        log_output = info.get("log_output", "FILE")

        self.add_info_section("Server Directories",
                              [("Base Directory:", lambda info, plugins, status: info.get("basedir")),
                               ("Data Directory:", lambda info, plugins, status: info.get("datadir")),
                               ("Disk Space in Data Dir:", disk_space),
                               ("InnoDB Data Directory:", lambda info, plugins, status: info.get("innodb_data_home_dir")) if info.get("innodb_data_home_dir") else None,
                               ("Plugins Directory:", lambda info, plugins, status: info.get("plugin_dir")),
                               ("Tmp Directory:", lambda info, plugins, status: info.get("tmpdir")),
                               ("Error Log:", lambda info, plugins, status: (info.get("log_error") and info.get("log_error")!="OFF", info.get("log_error"))),
                               ("General Log:", lambda info, plugins, status: (info.get("general_log")!="OFF" and log_output != "NONE", info.get("general_log_file") if "FILE" in log_output else "[Stored in database]")),
                               ("Slow Query Log:", lambda info, plugins, status: (info.get("slow_query_log")!="OFF" and log_output != "NONE", info.get("slow_query_log_file") if "FILE" in log_output else "[Stored in database]"))],
                              params)

        self.add_info_section("Replication Slave",
                              [("", "checking...")],
                              params)

        self.add_info_section("Authentication",
                              [("SHA256 password private key:", lambda info, plugins, status: info.get("sha256_password_private_key_path")),
                               ("SHA256 password public key:", lambda info, plugins, status: info.get("sha256_password_public_key_path"))],
                              params)

        self.add_info_section("SSL",
                              [("SSL CA:", lambda info, plugins, status: info.get("ssl_ca") or "n/a"),
                               ("SSL CA path:", lambda info, plugins, status: info.get("ssl_capath") or "n/a"),
                               ("SSL Cert:", lambda info, plugins, status: info.get("ssl_cert") or "n/a"),
                               ("SSL Cipher:", lambda info, plugins, status: info.get("ssl_cipher") or "n/a"),
                               ("SSL CRL:", lambda info, plugins, status: info.get("ssl_crl") or "n/a"),
                               ("SSL CRL path:", lambda info, plugins, status: info.get("ssl_crlpath") or "n/a"),
                               ("SSL Key:", lambda info, plugins, status: info.get("ssl_key") or "n/a")],
                              params)

        log_debug3("mysql_firewall_trace: %s\n" % info.get("mysql_firewall_trace"))
        log_debug3("Firewall_access_denied: %s\n" % status.get("Firewall_access_denied"))
        log_debug3("Firewall_access_granted: %s\n" % status.get("Firewall_access_granted"))
        log_debug3("Firewall_cached_entries: %s\n" % status.get("Firewall_cached_entries"))

        if info.get("mysql_firewall_mode") == "ON":
            self.add_info_section("Firewall",
                                  [("Access denied:", lambda info, plugins, status: str(status.get("Firewall_access_denied")) or "n/a"),
                                  ("Access granted:", lambda info, plugins, status: str(status.get("Firewall_access_granted")) or "n/a"),
                                  ("Access suspicious:", lambda info, plugins, status: str(status.get("Firewall_access_suspicious")) or "n/a"),
                                  ("Cached entries:", lambda info, plugins, status: str(status.get("Firewall_cached_entries")) or "n/a")],
                                  params)

    def add_info_section_2(self, title, info, params):
        label = mforms.newLabel(title)
        label.set_style(mforms.BigBoldStyle)
        label.set_color("#5f5f5f")
        self.content.add(label, False, True)
        sep = mforms.newBox(False)
        sep.set_back_color("#b2b2b2")
        sep.set_size(-1, 1)
        self.content.add(sep, False, True)

        hbox = mforms.newBox(True)

        info_table = self.make_info_table(info[:len(info)/2], params)
        hbox.add(info_table, True, True)
        info_table = self.make_info_table(info[len(info)/2:], params)
        hbox.add(info_table, True, True)

        self.content.add(hbox, False, True)
        self.content.get_parent().relayout()


    def add_info_section(self, title, info, params):
        label = mforms.newLabel(title)
        label.set_style(mforms.BigBoldStyle)
        label.set_color("#5f5f5f")
        self.content.add(label, False, True)
        sep = mforms.newBox(False)
        sep.set_back_color("#b2b2b2")
        sep.set_size(-1, 1)
        self.content.add(sep, False, True)

        info_table = self.make_info_table([x for x in info if x], params)
        self.content.add(info_table, False, True)
        self.controls[title] = (info_table, None)
        self.content.get_parent().relayout()


    def make_info_table(self, info, params):
        info_table = mforms.newTable()
        info_table.set_column_spacing(8)
        info_table.set_row_spacing(6)
        info_table.set_column_count(2)
        return self.setup_info_table(info_table, info, params)


    def setup_info_table(self, info_table, info, params):
        info_table.set_row_count(len(info)+1)
        for i, item in enumerate(info):
            (label, value_source) = item
            if callable(value_source):
                value = value_source(*params)
            else:
                value = value_source

            if self.controls.has_key(label):
                info_table.remove(self.controls[label][0])
            else:
                info_table.add(mforms.newLabel(label), 0, 1, i, i+1, mforms.VFillFlag|mforms.HFillFlag)
            is_gtid_mode_setable = label == 'GTID Mode:' and self.ctrl_be.target_version >= Version(5, 7, 6)
            if type(value) is bool or value is None:
                b = StateIcon()
                b.set_state(value)
                info_table.add(b, 1, 2, i, i+1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
                self.controls[label] = (b, value_source)
            elif type(value) is tuple:
                b = StateIcon()
                b.set_state(value[0])
                if value[0] and value[1]:
                    b.set_text(value[1])
                info_table.add(b, 1, 2, i, i+1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
                self.controls[label] = (b, value_source)
            else:
                if is_gtid_mode_setable:
                    self.gtid_mode_selector = mforms.newSelector()
                    self.gtid_mode_selector.add_items(["OFF", "UPGRADE_STEP_1", "UPGRADE_STEP_1", "ON"])
                    self.gtid_mode_selector.set_selected(self.gtid_mode_selector.index_of_item_with_title(value_source))
                    self.gtid_mode_selector.add_changed_callback(self._gtid_mode_changed)
                    info_table.add(self.gtid_mode_selector, 1, 2, i, i + 1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
                    self.controls[label] = (self.gtid_mode_selector, value_source)
                else:
                    l2 = mforms.newLabel(value or "")
                    l2.set_style(mforms.BoldStyle)
                    l2.set_color("#1c1c1c")
                    info_table.add(l2, 1, 2, i, i + 1, mforms.HFillFlag | mforms.HExpandFlag | mforms.VFillFlag)
                    self.controls[label] = (l2, value_source)
        info_table.add(mforms.newLabel(""), 0, 1, len(info), len(info)+1, mforms.HFillFlag) # blank space
        return info_table


    #---------------------------------------------------------------------------
    def page_deactivated(self):
        pass
    
    #---------------------------------------------------------------------------
    def shutdown(self):
        if self._update_timeout:
            mforms.Utilities.cancel_timeout(self._update_timeout)
            self._update_timeout = None
        self.status.stop()

    #---------------------------------------------------------------------------
    def _gtid_mode_changed(self):
        new_value = self.gtid_mode_selector.get_string_value()
        try:
            self.ctrl_be.exec_query("SET @@GLOBAL.GTID_MODE = %s;" % new_value)
        except QueryError, e:
            log_error("Error update GTID mode: %s" % str(e))
