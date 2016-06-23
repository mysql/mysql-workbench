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

from mforms import newBarGraphWidget, newLineDiagramWidget
import mforms

import wba_monitor_be

UPDATE_INTERVAL = 3

class WbAdminMonitor(mforms.Box):
    mon_be      = None
    ctrl_be          = None
    server_info      = None
    cpu_usage        = None
    memory           = None
    heartbeat        = None
    connection_usage = None
    traffic          = None
    hitrate          = None
    key_efficiency   = None
    ib_usage         = None
    innodb_reads     = None
    innodb_writes    = None

    widgets = None

    def __init__(self, server_profile, ctrl_be):
        mforms.Box.__init__(self, False)

        self.widgets = {}

        self.server_profile = server_profile
        self.set_managed()
        self.set_release_on_add()

        self.suspend_layout()

        self.ctrl_be = ctrl_be

        def bigLabel(text="---\n"):
            l = mforms.newLabel(text)
            l.set_style(mforms.VeryBigStyle)
            return l

        # Status icon
        self.box1 = mforms.newBox(True)
        self.box1.set_spacing(28)

        health_text = mforms.newBox(True)
        health_text.set_spacing(20)

        self.status_icon = mforms.newServerStatusWidget()
        self.status_icon.set_description("Server Status")
        self.status_icon.set_size(86, -1)
        self.box1.add(self.status_icon, False, True)

        self.status_label = bigLabel("Unknown\n")
        self.status_label.set_size(86, -1)
        health_text.add(self.status_label, False, True)

        # System.
        system_box = mforms.newBox(True)
        system_box.set_spacing(28)
        self.cpu_usage = newBarGraphWidget()
        if self.server_profile.target_is_windows:
            self.cpu_usage.set_description("CPU")
        else:
            self.cpu_usage.set_description("Load")
        self.cpu_usage.set_right_align(True)
        self.cpu_usage.set_size(31, -1);
        system_box.add(self.cpu_usage, False, True)

        label = bigLabel()
        label.set_size(47, -1)
        label.set_text_align(mforms.TopRight)
        health_text.add(label, False, True)
        if self.server_profile.target_is_windows:
            self.cpu_widget = (self.cpu_usage, label, lambda x: str(int(x*100)) + "%\n", None)
        else:
            self.cpu_usage.enable_auto_scale(True)
            self.cpu_widget = (self.cpu_usage, label, lambda x: str(x)+"\n", None)

        sql = {}
        self.connection_usage= newLineDiagramWidget()
        self.connection_usage.set_description("Connections")
        self.connection_usage.enable_auto_scale(True)
        self.connection_usage.set_thresholds([0.0], [10.0, 50.0, 100.0, 500.0, 1000.0])
        system_box.add(self.connection_usage, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_connections'] = (self.connection_usage, label, lambda x: "%s\n"%str(int(x)), None)
        sql['get_connections'] = {'query' : ("Threads_connected",), 'min' : 0, 'max' : 10, 'calc' : None}
        self.box1.add(system_box, True, True)

        self.add(self.box1, False, True)
        self.add(health_text, False, True)

        # Server health.
        health = mforms.newBox(True)
        health.set_spacing(28)
        health_text = mforms.newBox(True)
        health_text.set_homogeneous(True)
        health_text.set_spacing(24) # 4px less as for the widgets (the labels have a bit leading space).

        self.traffic = newLineDiagramWidget()
        self.traffic.set_description("Traffic")
        self.traffic.enable_auto_scale(True)
        self.traffic.set_thresholds([0.0], [100000.0, 1000000.0, 10000000.0, 100000000.0])
        health.add(self.traffic, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_traffic'] = (self.traffic, label, lambda x: "%s\n"%self.format_value(x), None)
        self.last_traffic = 0
        sql['get_traffic'] = {'query' : ("Bytes_sent",), 'min' : 0, 'max' : 100, 'calc' : self.calc_traffic}

        self.key_efficiency= newLineDiagramWidget()
        self.key_efficiency.set_description("Key Efficiency")
        health.add(self.key_efficiency, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_key_efficiency'] = (self.key_efficiency, label, lambda x: ("%.1f%%\n" % x), None)
        sql['get_key_efficiency'] = {'query' : ("Key_reads","Key_read_requests"), 'min' : 0, 'max' : 100, 'calc' : self.calc_key_efficiency}

        self.add(health, False, True)
        self.add(health_text, False, True)

        # Query
        health= mforms.newBox(True)
        health.set_spacing(28)
        health_text = mforms.newBox(True)
        health_text.set_homogeneous(True)
        health_text.set_spacing(24)

        self.qps= newLineDiagramWidget()
        self.qps.set_description("Selects per Second")
        self.qps.enable_auto_scale(True)
        self.qps.set_thresholds([0.0], [50.0, 100.0, 200.0, 500.0, 1000.0, 5000.0, 10000.0])
        health.add(self.qps, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_qps'] = (self.qps, label, lambda x: ("%.0f\n" % x), None)
        self.last_qcount = 0
        sql['get_qps'] = {'query' : ("Com_select",), 'min' : 0, 'max' : 100, 'calc' : self.calc_qps}

#        self.hitrate= newLineDiagramWidget()
#        self.hitrate.set_description("Query Cache Hitrate")
#        health.add(self.hitrate, True, True)
#        label = bigLabel()
#        health_text.add(label, True, True)
#        self.widgets['get_hitrate'] = (self.hitrate, label, lambda x: ("%.1f%%\n" % x), None)
#        sql['get_hitrate'] = {'query' : ("Qcache_hits", "Qcache_inserts", "Qcache_not_cached"), 'min' : 0, 'max' : 100, 'calc' : self.calc_hitrate}
#
#        self.add(health, False, True)
#        self.add(health_text, False, True)
#
#        # Cache/buffer
#        health = mforms.newBox(True)
#        health.set_spacing(28)
#        health_text = mforms.newBox(True)
#        health_text.set_homogeneous(True)
#        health_text.set_spacing(24)
#
#        self.qcache_usage = newLineDiagramWidget()
#        self.qcache_usage.set_description("Query Cache Usage")
#        health.add(self.qcache_usage, True, True)
#        label = bigLabel()
#        health_text.add(label, True, True)
#        self.widgets['get_qcache_usage'] = (self.qcache_usage, label, lambda x: ("%.1f%%\n" % x), None)
#        sql['get_qcache_usage'] = {'query' : ("Qcache_free_blocks", "Qcache_total_blocks"), 'min' : 0, 'max' : 100, 'calc' : self.calc_qcache_usage}

        self.ib_usage = newLineDiagramWidget()
        self.ib_usage.set_description("InnoDB Buffer Usage")
        health.add(self.ib_usage, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_ib_usage'] = (self.ib_usage, label, lambda x: ("%.1f%%\n" % x), None)
        sql['get_ib_usage'] = {'query' : ("Innodb_buffer_pool_pages_free", "Innodb_buffer_pool_pages_total"), 'min' : 0, 'max' : 100, 'calc' : self.calc_ib_usage}
        
        self.add(health, False, True)
        self.add(health_text, False, True)

        # InnoDB Reads/Writes per second:
        health= mforms.newBox(True)
        health.set_spacing(28)
        health_text = mforms.newBox(True)
        health_text.set_homogeneous(True)
        health_text.set_spacing(24)

        self.innodb_reads = newLineDiagramWidget()
        self.innodb_reads.set_description('InnoDB Reads per Second')
        self.innodb_reads.enable_auto_scale(True)
        self.innodb_reads.set_thresholds([0.0], [50.0, 100.0, 200.0, 500.0, 1000.0, 5000.0, 10000.0])
        health.add(self.innodb_reads, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_innodb_reads'] = (self.innodb_reads, label, lambda x: ('%.0f\n' % x), None)
        self.last_ircount = 0
        sql['get_innodb_reads'] = {'query' : ('Innodb_data_reads',), 'min' : 0, 'max' : 100, 'calc' : self.calc_innodb_reads_per_second}

        self.innodb_writes = newLineDiagramWidget()
        self.innodb_writes.set_description('InnoDB Writes per Second')
        self.innodb_writes.enable_auto_scale(True)
        self.innodb_writes.set_thresholds([0.0], [50.0, 100.0, 200.0, 500.0, 1000.0, 5000.0, 10000.0])
        health.add(self.innodb_writes, True, True)
        label = bigLabel()
        health_text.add(label, True, True)
        self.widgets['get_innodb_writes'] = (self.innodb_writes, label, lambda x: ('%.0f\n' % x), None)
        self.last_iwcount = 0
        sql['get_innodb_writes'] = {'query' : ('Innodb_data_writes',), 'min' : 0, 'max' : 100, 'calc' : self.calc_innodb_writes_per_second}
        
        self.add(health, False, True)
        self.add(health_text, False, True)

        self.resume_layout()

        self.mon_be = wba_monitor_be.WBAdminMonitorBE(UPDATE_INTERVAL, server_profile, ctrl_be, self.widgets, self.cpu_widget, sql)

    def calc_traffic(self, x):
        tx = int(x[0])
        if self.last_traffic == 0:
            self.last_traffic = tx
            return 0
        ret = tx - self.last_traffic
        self.last_traffic = tx
        return ret/UPDATE_INTERVAL

    def calc_key_efficiency(self, (key_reads, key_read_requests)):
        key_read_requests = float(key_read_requests)
        if key_read_requests == 0.0:
            return 0
        return 100 - (((float(key_reads) / key_read_requests * 100))/UPDATE_INTERVAL)

#    def calc_hitrate(self, (hits, inserts, not_cached)):
#        hits = float(hits)
#        inserts = int(inserts)
#        not_cached = int(not_cached)
#        t = hits + inserts + not_cached
#        if t == 0:
#            return 0
#        return (hits/t)*100

    def calc_qps(self, counts):
        c = sum([int(c) for c in counts])
        if self.last_qcount == 0:
            self.last_qcount = c
            return 0
        ret = c - self.last_qcount
        self.last_qcount = c
        return ret/UPDATE_INTERVAL

#    def calc_qcache_usage(self, (free_blocks, total_blocks)):
#        free_blocks, total_blocks = float(free_blocks), float(total_blocks)
#        if -0.00001 <= total_blocks <= 0.00001:
#            return 0
#        return 100 * (total_blocks - free_blocks) / total_blocks

    def calc_ib_usage(self, (free_pages, total_pages)):
        free_pages, total_pages = float(free_pages), float(total_pages)
        if -0.00001 <= total_pages <= 0.00001:
            return 0
        return 100 * ((total_pages - free_pages) / total_pages)

    def calc_innodb_reads_per_second(self, (count,)):
        if self.last_ircount == 0:
            self.last_ircount = count
            return 0
        ret = count - self.last_ircount
        self.last_ircount = count
        return ret/UPDATE_INTERVAL

    def calc_innodb_writes_per_second(self, (count,)):
        if self.last_iwcount == 0:
            self.last_iwcount = count
            return 0
        ret = count - self.last_iwcount
        self.last_iwcount = count
        return ret/UPDATE_INTERVAL

    def refresh_status(self, status):
        if status == "running" or status == "started":
            self.mon_be.note_server_running()
            self.status_icon.set_server_status(1)
            self.status_label.set_text("Running\n")
        elif status == "offline":
            self.mon_be.note_server_running()
            self.status_icon.set_server_status(2)
            self.status_label.set_text("Offline\n")
        elif status == "stopped":
            self.status_icon.set_server_status(0)
            self.status_label.set_text("Stopped\n")

    def format_value(self, value):
        if value < 1024:
            return str(value) + " B/s"
        else:
            if value < 1024 * 1024:
                return "%.2f KB/s" % (value / 1024)
            else:
                return "%.2f MB/s" % (value / 1024 / 1024)


    def stop(self):
        self.mon_be.stop()

