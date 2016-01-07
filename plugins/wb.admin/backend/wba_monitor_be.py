# Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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

import StringIO
import socket
import threading
import time

from mforms import App
from wb_server_management import wbaOS
from wb_common import Users

from workbench.log import log_info, log_error, log_debug, log_debug2, log_debug3


#===============================================================================
class DataSource(object):
    def __init__(self, name, mon_be, widget):
        self.widget   = None
        self.label_cb = None # method to create description
        self.calc_cb  = None
        self.name    = name
        self.mon_be  = mon_be
        self.ctrl_be = mon_be.get_ctrl_be()
        if widget is not None:
            self.widget   = widget[0]
            self.label    = widget[1]
            self.label_cb = widget[2]
            self.calc_cb  = widget[3]

    def poll(self):
        pass

    def shutdown_event(self):
        pass


#===============================================================================
class DBWidgetHandler(object):
    def __init__(self, name, ctrl_be, widget, variables, calc):
        self.name      = name
        self.ctrl_be   = ctrl_be
        self.widget    = widget[0]
        self.label     = widget[1]
        self.label_cb  = widget[2]
        self.variables = variables
        self.calc      = calc
        self.vars_set  = 0
        self.values    = [0] * len(variables)
        self.vars_len = len(variables)

    def set_var(self, i, value):
        self.values[i] = value
        self.vars_set += 1

    def calculate(self):
        result = None
        if self.vars_len <= self.vars_set:
            self.vars_set = 0
            args = tuple(self.values)
            if self.calc is not None:
                result = self.calc(args)
            else:
                result = args[0]

            if result is not None and self.widget is not None:
                try:
                    result = float(result)
                except (ValueError, TypeError):
                    print("Error! Calculation returned returned wrong value. Expected int or float but got", result)
                    result = 0.0
                self.widget.set_value(result)
                if self.label_cb is not None:
                    lbl = self.label_cb(result)
                    self.ctrl_be.uitask(self.label.set_text, lbl)

#===============================================================================
class DBStatusDataSource(DataSource):
    def __init__(self, mon_be):
        DataSource.__init__(self, "sql_source", mon_be, None)
        self.mon_be = mon_be
        self.sources = {}
        self.rev_sources = {}
        log_debug2('DBStatusDataSource created.\n')

    def add_source(self, name, definition, widget):
        # Source is a dict which contains names of status variables, formula to calc
        # result, min and max values. Formula is a method(lambda) which gets params as
        # specified in the list of status_variables. For example:
        # add_source({'query' : ('Key_reads', 'Key_read_reqs'), 'calc' : lambda x: x[0] - Key_reads, x[1] - Key_read_reqs})
        status_variables = definition['query']
        # We need to store data we need, like names of status variables, calc function, and temporary data from status_variables
        # As data from show status is not parsed as a whole but row by row, we have to gather all status variables which are involved
        # in calc before calling calc.
        src = DBWidgetHandler(name, self.mon_be.get_ctrl_be(), widget, status_variables, definition['calc'])
        self.sources[name] = src

        # Add reverse index of all status variables. Each status variable name may refer several
        # calc formulas, and each formula may have several variables. This rev index helps to find formulas
        # where status variable is used. Each entry in this reverse index contain reference to source tuple (see
        # above, where source tuple is added to self.sources) and also the entry has index of variable in formula
        for i, status_variable_name in enumerate(src.variables):
            rev_src = None
            if status_variable_name in self.rev_sources:
                rev_src = self.rev_sources[status_variable_name]
            else:
                rev_src = []
                self.rev_sources[status_variable_name] = rev_src
            rev_src.append((src, i))

        if 'min' in definition and 'max' in definition:
            widget[0].set_value_range(definition['min'], definition['max'])

    def poll(self):
        #log_debug3('%s:%s.poll()' % (_this_file, self.__class__.__name__), 'DBStatusDataSource poll.\n')
        if self.ctrl_be.status_variables:
            # update monitor stuff
            for name in self.rev_sources:
                rev_src = self.rev_sources[name]
                value = float(self.ctrl_be.status_variables[name])
                # rev_src contains list of sources and index of current variable in the sources
                for (src, i) in rev_src:
                    src.set_var(i, value)
                    src.calculate()

#===============================================================================
class ShellDataSource(DataSource):
    cmds = None

    def __init__(self, name, detected_os_name, mon_be, cpu_widget):
        DataSource.__init__(self, name, mon_be, cpu_widget)
        self.os_name = detected_os_name
        self._cpu_stat_return = None

    def poll(self):
        output = StringIO.StringIO()
        if self.ctrl_be.server_helper.execute_command("/usr/bin/uptime", output_handler=output.write) == 0:
            data = output.getvalue().strip(" \r\t\n,:.").split("\n")[-1]
            self._cpu_stat_return = None
            load_value = data.split()[-3]
            # in some systems, the format is x.xx x.xx x.xx and in others, it's x.xx, x.xx, x.xx
            load_value = load_value.rstrip(",")
            try:
                result = float(load_value.replace(',','.'))
            except (ValueError, TypeError):
                log_error("Shell source %s returned wrong value. Expected int or float but got '%s'\n" % (self.name, load_value))
                result = 0

            if self.widget is not None:
                self.widget.set_value(self.calc_cb(result) if self.calc_cb else result)
                if self.label_cb is not None:
                    self.ctrl_be.uitask(self.label.set_text, self.label_cb(result))
        else:
            value = output.getvalue()
            if value != self._cpu_stat_return:
                self._cpu_stat_return = value
                log_debug("CPU stat command returned error: %s\n" % value)


#===============================================================================
class WinRemoteStats(object):
    def __init__(self, ctrl_be, server_profile, running, cpu_widget):
        self.ctrl_be = ctrl_be
        self.ssh = None
        self.cpu = 0
        self.mtx = threading.Lock()
        self.running = running
        self.cpu_widget = cpu_widget
        self.settings = server_profile
        self.remote_admin_enabled = self.settings.uses_ssh

        if not self.remote_admin_enabled:
            return

        self.ctrl_be.add_me_for_event("shutdown", self)

        #upload script. Get local name, open ftp session and upload to the directory
        # where mysql.ini is.
        self.script = None

        self.ssh   = ctrl_be.open_ssh_session_for_monitoring()

        (dirpath, code) = self.ssh.exec_cmd("cmd /C echo %USERPROFILE%") # %APPDATA% is n/a for LocalService
                                                                         # which is a user sshd can be run
        dirpath = dirpath.strip(" \r\t\n")

        if code == 0 and dirpath is not None and dirpath != "%USERPROFILE%":
            script_path = App.get().get_resource_path("mysql_system_status_rmt.vbs")
            filename = "\"" + dirpath + "\\mysql_system_status_rmt.vbs\""
            log_debug('Script local path is "%s". Will be uploaded to "%s"\n' % (script_path, filename) )
            if script_path is not None and script_path != "":
                #print "Uploading file to ", filename
                try:
                    f = open(script_path)
                    self.ssh.exec_cmd("cmd /C echo. > " + filename)
                    maxsize = 1800
                    cmd = ""
                    for line in f:
                        line = line.strip("\r\n")
                        tline = line.strip(" \t")
                        if len(tline) > 0:
                            if tline[0] != "'":
                                if len(cmd) > maxsize:
                                    self.ssh.exec_cmd("cmd /C " + cmd.strip(" &"))
                                    self.ssh.exec_cmd("cmd /C echo " + line + " >> " + filename)
                                    cmd = ""
                                else:
                                    cmd += "echo " + line + " >> " + filename
                                    cmd += " && "

                    if len(cmd) > 0:
                        self.ssh.exec_cmd("cmd /C " + cmd.strip(" &"))
                        cmd = ""

                    self.script = "cscript //NoLogo " + filename + " /DoStdIn"
                    #run ssh in a thread

                    log_debug2('About to run "%s"\n' % self.script)

                    self.chan = None
                    self.out = ""

                    self.read_thread = threading.Thread(target=self.ssh.exec_cmd, args=(self.script, Users.CURRENT, None, self.reader, 1, self.save_channel))
                    self.read_thread.setDaemon(True)
                    self.read_thread.start()
                except IOError, e:
                    self.ssh.close()
                    self.ssh = None
                    raise e
        else:
            print "Can't find a place to upload script dirpath='%s'"%dirpath


    #-----------------------------------------------------------------------------
    def shutdown_event(self):
        if self.ssh:
            self.running[0] = False
            try:
                if self.read_thread:
                    self.read_thread.join()
            except:
                pass
            self.ssh.close()
            self.ssh = None

    #-----------------------------------------------------------------------------
    def save_channel(self, chan):
        self.chan = chan

    #-----------------------------------------------------------------------------
    def poll(self):
        value = self.get_cpu()
        if value is not None:
            self.cpu_widget[0].set_value(value / 100)
            self.ctrl_be.uitask(self.cpu_widget[1].set_text, str(int(value)) + "%")

    #-----------------------------------------------------------------------------
    def parse_cpu(self, text):
        text = text.strip(" \r\t\n")
        value = 0.0
        try:
            value = float(text)
        except ValueError:
            value = 0.0

        try:
            self.mtx.acquire()
            self.cpu = value
        finally:
            self.mtx.release()

    #-----------------------------------------------------------------------------
    def reader(self, ssh_session):
        what = None
        out = ""
        timeouts = 12
        while self.running[0]: # running is passed in list via "refernce"
            try:
                ch = ssh_session.recv(1)
                timeouts = 12
                if ch == "C":
                    what = self.parse_cpu
                elif ch == "\r" or ch == "\n":
                    if what is not None:
                        what(out)
                    what = None
                    out = ""
                elif ch in "0123456789. ":
                    out += ch
                elif ch == ",":
                    out += "."
                else:
                    what = None
                    out = ""
            except socket.timeout:
                timeouts -= 1
                if timeouts <= 0:
                    ssh_session.close()
                    raise Exception("Can't read from remote Windows script")

        log_debug('Leaving monitor thread which polls remote windows\n')

    #-----------------------------------------------------------------------------
    def get_cpu(self):
        ret = ""
        self.mtx.acquire()
        ret = self.cpu
        self.mtx.release()

        return ret

#===============================================================================
class WMIStats(object):
    def __init__(self, ctrl_be, server_profile, cpu_widget):
        if not hasattr(ctrl_be.server_control, 'wmi'):
            raise Exception("Current profile has no WMI enabled") # TODO Should be better message

        self.ctrl_be = ctrl_be
        self.server_profile = server_profile
        self.cpu_widget = cpu_widget
        self.cpu_mon_id = None
        self.wmi = ctrl_be.server_control.wmi

    def query(self, session, attr, query):
        value = None
        res = self.wmi.wmiQuery(session, query)
        if len(res) > 0:
            if hasattr(res[0], attr):
                value = getattr(res[0], attr)
                try:
                    value = int(value)
                except:
                    print "Wmi query: can't cast '%s' to int" % str(value)
                    value = 1
            else:
                print "Wmi query: expected '%s' result attribute, got:" % attr
                print res
                value = 1
        return value

    def poll(self):
        wmi_session = self.ctrl_be.server_control.wmi_session_id_for_current_thread
        # Calc CPU
        value = self.query(wmi_session, 'PercentProcessorTime', "SELECT PercentProcessorTime FROM Win32_PerfFormattedData_PerfOS_Processor WHERE Name = '_Total'")
        if value is not None:
            self.cpu_widget[0].set_value(value/100.0)
            self.ctrl_be.uitask(self.cpu_widget[1].set_text, str(int(value)) + "%")


#===============================================================================
class WBAdminMonitorBE(object):
    def __init__(self, interval, server_profile, ctrl_be, widgets, cpu_widget, sql):
        self.ctrl_be   = ctrl_be
        self.sources     = [] # List of sources
        self.running     = [True]
        self.poll_thread = None
        self.interval  = interval

        self.ctrl_be.add_me_for_event("server_started", self)
        self.ctrl_be.add_me_for_event("server_offline", self)
        self.ctrl_be.add_me_for_event("server_stopped", self)

        if server_profile.target_os != wbaOS.windows:
            if server_profile.is_local or server_profile.remote_admin_enabled:
                cmd = ShellDataSource("host_stats", server_profile.detected_os_name, self, cpu_widget)
                self.sources.append(cmd)
            else:
                log_info("WBAMonBE: Data sources were not added. Profile set to non-local or remote admin is disabled.")
        else:
            if server_profile.uses_wmi: # Force WMI for local boxes - pointless to force WMI, if it was not setup earlier (throws exc)
                self.wmimon = WMIStats(ctrl_be, server_profile, cpu_widget)
                self.sources.append(self.wmimon)
            else:
                if server_profile.remote_admin_enabled and server_profile.connect_method == 'ssh':
                    stats = WinRemoteStats(ctrl_be, server_profile, self.running, cpu_widget)
                    self.sources.append(stats)

        sql_sources = DBStatusDataSource(self)
        for name, query in sql.iteritems():
            widget = None
            if name in widgets:
                widget = widgets[name]
            sql_sources.add_source(name, query, widget)

        self.sources.append(sql_sources)

        self.poll_thread = None

    def __del__(self):
        self.running[0] = False
        try:
            self.poll_thread.join()
        except:
            pass
        self.sources = []

    def note_server_running(self):
        self.server_started_event()
        for x in self.sources:
            if hasattr(x, 'server_started_event'):
                x.server_started_event()

    def get_ctrl_be(self):
        return self.ctrl_be

    def server_started_event(self):
        log_debug3('Enter\n')

        if self.poll_thread and self.running[0]:
            # no need to restart the poll thread
            return

        # This is needed to ensure an existing polling thread is finished
        # Before creating the new one
        if self.poll_thread:
            self.running[0] = False
            self.poll_thread.join()
            self.poll_thread = None

        self.running[0] = True
        self.poll_thread = threading.Thread(target = self.poll_sources)
        self.poll_thread.start()
        log_debug3('Leave\n')
        
    def server_offline_event(self):
        self.server_started_event() #offline even can be sent only if server is running

    def server_stopped_event(self):
        log_debug3('Enter\n')
        self.running[0] = False
        self.poll_thread = None
        log_debug3('Leave\n')

    def stop(self):
        self.running[0] = False
        for cmd in self.sources:
            if hasattr(cmd, 'shutdown_event'):
                cmd.shutdown_event()

    def poll_sources(self):
        while self.running[0] and self.ctrl_be.running:
            #sleep here
            for cmd in self.sources:
                cmd.poll()
            time.sleep(self.interval)
        log_debug('Exiting monitor thread...\n')
