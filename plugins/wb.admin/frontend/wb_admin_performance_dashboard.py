# Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

from workbench.graphics.charting import DBTimeLineGraph, DBSimpleCounter, DBRoundMeter, DBLevelMeter, DBImage, DBText
from workbench.graphics.canvas import Canvas, TextFigure
from workbench.graphics.cairo_utils import Context

from wb_admin_utils import WbAdminBaseTab
import re
from workbench.log import log_error
from workbench.utils import Version


class MyDict:
    def __init__(self, d):
    
        self.d = d


    def __contains__(self, k):
        print "contains", k
        return self.d.__contains__(k)


    def __getitem__(self, k):
        print "getit", k
        return self.d.__getitem__(k)



class RenderBox(mforms.PyDrawBox):
    def __init__(self, parent):
        mforms.PyDrawBox.__init__(self)
        self.set_managed()
        self.set_release_on_add()

        self.parent = parent
        self.canvas = None

        self.set_instance(self)
        self.drag_offset = None
        self.drag_object = None
        self.tooltip = None

        self.offset = (0, 0)
        self.current_pos = (0, 0)

        self.variable_values = None
    
        self.layouting_mode = False


    def __del__(self):
        if self.tooltip:
            self.tooltip.close()
            self.tooltip = None


    def mouse_down(self, b, x, y):
        x -= self.offset[0]
        y -= self.offset[1]
        if b == 0:
            if self.layouting_mode:
                self.drag_object = self.canvas.figure_at(x, y)
                if self.drag_object:
                    self.drag_offset = x - self.drag_object.x, y - self.drag_object.y

    def mouse_up(self, b, x, y):
        x -= self.offset[0]
        y -= self.offset[1]
        if b == 0:
            if self.drag_object:
                xx = x - self.drag_offset[0]
                yy = y - self.drag_offset[1]
                xx -= xx % 2
                yy -= yy % 2
                self.drag_object.move(xx, yy)
                print self.drag_object.pos

            self.drag_offset = None
            self.drag_object = None


    def mouse_move(self, x, y):
        x -= self.offset[0]
        y -= self.offset[1]

        if self.drag_object:
            xx = x - self.drag_offset[0]
            yy = y - self.drag_offset[1]
            xx -= xx % 2
            yy -= yy % 2
            self.drag_object.move(xx, yy)

        self.current_pos = x, y
        self.canvas.mouse_move(x, y)


    def repaint(self, cr, x, y, w, h):
        xoffs, yoffs = self.parent.relayout()
        self.offset = xoffs, yoffs

        c = Context(cr)
        try:
            self.canvas.repaint(c, xoffs, yoffs, w, h)
        except Exception:
            import traceback
            log_error("Exception rendering dashboard: %s\n" % traceback.format_exc())


    def add(self, figure):
        self.canvas.add(figure)
        figure.on_hover_in = self.handle_hover_in
        figure.on_hover_out = self.handle_hover_out


    def make_tooltip_text(self, figure, template):
        text = template % self.variable_values

        # find and evaluate all embedded ${expressions}
        for m in re.findall("(\${[^}]*})", text):
            value = eval(m[2:-1] % self.variable_values)
            text = text.replace(m, str(value))

        return text


    def close_tooltip(self):
        if self.tooltip:
            self.tooltip.close()
            self.tooltip = None


    def handle_hover_out(self, fig, x, y):
        self.close_tooltip()


    def handle_hover_in(self, fig, x, y):
        if self.tooltip:
            self.tooltip.close()
            self.tooltip = None

        if not mforms.Form.main_form().is_active():
            return

        if fig and getattr(fig, 'hover_text_template', None):
            text = self.make_tooltip_text(fig, fig.hover_text_template)
            if text:
                self.tooltip = mforms.newPopover(mforms.PopoverStyleTooltip)

                fx = fig.x + self.offset[0] + 2 * fig.width / 3
                fy = fig.y + self.offset[1] + 2 * fig.height / 3

                xx, yy = self.client_to_screen(fx, fy)
                box = mforms.newBox(False)
                box.set_spacing(0)
                t = ""
                for line in text.split("\n"):
                    if line.startswith("*"):
                        if t:
                            if t.endswith("\n"):
                                t = t[:-1]
                            label = mforms.newLabel(t)
                            label.set_style(mforms.SmallStyle)
                            box.add(label, False, False)
                            t = ""
                        label = mforms.newLabel(line[1:].rstrip("\n"))
                        label.set_style(mforms.SmallBoldStyle)
                        box.add(label, False, False)
                    else:
                        t += line+"\n"
                if t:
                    label = mforms.newLabel(t.rstrip("\n"))
                    label.set_style(mforms.SmallStyle)
                    box.add(label, False, False)
            
                self.tooltip.set_size(max(box.get_preferred_width(), 100), max(box.get_preferred_height(), 50))
            
                self.tooltip.set_content(box)
                self.tooltip.add_close_callback(self.close_tooltip)
                self.tooltip.show_and_track(self, xx, yy, mforms.StartRight)


class CDifferencePerSecond(object):
    def __init__(self, expr):
        self.expr = expr

        self.reset()


    def reset(self):
        self.old_value = None
        self.old_value_timestamp = None
      
      
    def calculate(self, value, timestamp):
        pass
      
      
    def handle(self, values, timestamp):

        result = None

        if not self.expr:
            return result
      
        value = eval(self.expr % values)
      
        if self.old_value and self.old_value_timestamp:
            if timestamp > self.old_value_timestamp:
                result = self.calculate(value, timestamp)

        self.old_value = value
        self.old_value_timestamp = timestamp

        return result


class CSingleDifferencePerSecond(CDifferencePerSecond):
    def __init__(self, expr):
       super(CSingleDifferencePerSecond, self).__init__(expr)


    def calculate(self, value, timestamp):
        return float(value - self.old_value) / (timestamp - self.old_value_timestamp)


class CTupleDifferencePerSecond(CDifferencePerSecond):
    def __init__(self, expr):
       super(CTupleDifferencePerSecond, self).__init__(expr)

        

    def calculate(self, value, timestamp):
        result = []
        for i in range(len(value)):
                result.append(float(value[i] - self.old_value[i]) / (timestamp - self.old_value_timestamp))
        return tuple(result)

        
class CRawValue(object):
    def __init__(self, expr):
        self.expr = expr        
    
    def handle(self, values, timestamp):
        if not self.expr:
            return None
        value = eval(self.expr % values)

        return value


class CMakeTuple(object):
    def __init__(self, *items):
        self.items = items


    def handle(self, values, timestamp):
        l = []
        for i in self.items:
            l.append(i.handle(values, timestamp))
        return tuple(l)


READ_COLOR = (60/255.0, 178/255.0, 191/255.0)
WRITE_COLOR = (253/255.0, 138/255.0, 39/255.0)

GLOBAL_DASHBOARD_WIDGETS_NETWORK = \
[
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_header_network.png"),), None, (None, None),
  (0, 0, 0), (85, 5),
  ""),
 (None, DBText, ("Statistics for network traffic sent and received\nby the MySQL Server over client connections.",), None, (None, None),
  (0.4, 0.4, 0.4), (65, 72),
  ""),
 
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_arrow_in_static.png"),), None, (None, None),
  (0, 0, 0), (228, 196),
  ""),

 (None, DBSimpleCounter, ("receiving\n%.2f %sB/s", True), None, (CSingleDifferencePerSecond, "%(Bytes_received)s"),
  READ_COLOR, (200, 240),
  """Bytes Received
Number of bytes received by the MySQL server at the network level.

Total bytes received: %(Bytes_received)s"""),

 ("Incoming Network Traffic (Bytes/Second)", DBTimeLineGraph, ("%.1f %sB", True), None, (CSingleDifferencePerSecond, "%(Bytes_received)s"),
  READ_COLOR, (30, 160),
  None),

 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_arrow_out_static.png"),), None, (None, None),
  (0, 0, 0), (228, 368),
  ""),

 (None, DBSimpleCounter, ("sending\n%.2f %sB/s", True), None, (CSingleDifferencePerSecond, "%(Bytes_sent)s"),
  WRITE_COLOR, (200, 410),
  """Bytes Sent
Number of bytes sent by the MySQL server at the network level.
      
Total bytes sent: %(Bytes_sent)s"""),
 
 ("Outgoing Network Traffic (Bytes/Second)", DBTimeLineGraph, ("%.1f %sB", True), None, (CSingleDifferencePerSecond, "%(Bytes_sent)s"),
  WRITE_COLOR, (30, 330),
  None),

 ("Client Connections (Total)", DBTimeLineGraph, ("%.1f %s", 1, True), None, (CRawValue, "%(Threads_connected)s"),
  (124/255.0, 193/255.0, 80/255.0), (30, 500),
  None),
 
 (None, DBLevelMeter, tuple(), (CRawValue, "%(max_connections)s"), (CRawValue, "%(Threads_connected)s"),
  (124/255.0, 193/255.0, 80/255.0), (200, 520),
  """Connections
Client connections/threads to the MySQL server.

Threads connected: %(Threads_connected)s
Threads running: %(Threads_running)s

Total connection attempts: %(Connections)s
Connection errors (accept): %(Connection_errors_accept)s
Connection errors (internal): %(Connection_errors_internal)s
Connection errors (max connections reached): %(Connection_errors_max_connections)s
Connection errors (peer address): %(Connection_errors_peer_address)s
Connection errors (select): %(Connection_errors_select)s
Connection errors (tcpwrap): %(Connection_errors_tcpwrap)s"""),

 
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_separator.png"),), None, (None, None),
  (0, 0, 0), (310, 120),
  ""),
]

GLOBAL_DASHBOARD_WIDGETS_MYSQL_PRE_80 = \
[
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_header_mysql.png"),), None, (None, None),
  (0, 0, 0), (380, 5),
  ""),
 (None, DBText, ("Primary MySQL Server activity\nand performance statistics.",), None, (None, None),
  (0.4, 0.4, 0.4), (376, 72),
  ""),
 
 ("Table Open Cache", DBRoundMeter, ("Efficiency",), None, (CRawValue, "%(Table_open_cache_hits)s/(%(Table_open_cache_hits)s+%(Table_open_cache_misses)s+0.0)"),
  (124/255.0, 193/255.0, 80/255.0), (380, 150),
  """Table Open Cache
Cache for minimizing number of times MySQL 
will open database tables when accessed.

Table open cache hits: %(Table_open_cache_hits)s
Table open cache misses: %(Table_open_cache_misses)s"""),
 
 ("SQL Statements Executed (#)", DBTimeLineGraph, ("%.1f %s", 3, True), None, (CTupleDifferencePerSecond, "(%(Com_select)s,%(Com_insert)s+%(Com_update)s+%(Com_delete)s,%(Com_create_db)s+%(Com_create_event)s+%(Com_create_function)s+%(Com_create_index)s+%(Com_create_procedure)s+%(Com_create_server)s+%(Com_create_table)s+%(Com_create_trigger)s+%(Com_create_udf)s+%(Com_create_user)s+%(Com_create_view)s+%(Com_alter_db)s+%(Com_alter_db_upgrade)s+%(Com_alter_event)s+%(Com_alter_function)s+%(Com_alter_procedure)s+%(Com_alter_server)s+%(Com_alter_table)s+%(Com_alter_tablespace)s+%(Com_alter_user)s+%(Com_drop_db)s+%(Com_drop_event)s+%(Com_drop_function)s+%(Com_drop_index)s+%(Com_drop_procedure)s+%(Com_drop_server)s+%(Com_drop_table)s+%(Com_drop_trigger)s+%(Com_drop_user)s+%(Com_drop_view)s)"),
  [(255/255.0, 201/255.0, 2/255.0), (126/255.0, 142/255.0, 207/255.0), (194/255.0, 123/255.0, 206/255.0)], (350, 330),
    None),

 (None, DBSimpleCounter, ("SELECT\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_select)s"),
  (255/255.0, 201/255.0, 2/255.0), (350, 470),
  """SELECT Statements Executed
    
Total since start: %(Com_select)s"""),

 (None, DBSimpleCounter, ("INSERT\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_insert)s"),
  (126/255.0, 142/255.0, 207/255.0), (350, 520),
  """INSERT Statements Executed
      
Total since start: %(Com_insert)s"""),
 (None, DBSimpleCounter, ("UPDATE\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_update)s"),
  (126/255.0, 142/255.0, 207/255.0), (350, 560),
  """UPDATE Statements Executed
      
Total since start: %(Com_update)s"""),
 (None, DBSimpleCounter, ("DELETE\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_delete)s"),
  (126/255.0, 142/255.0, 207/255.0), (350, 600),
  """DELETE Statements Executed
      
Total since start: %(Com_delete)s"""),

 (None, DBSimpleCounter, ("CREATE\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_create_db)s+%(Com_create_event)s+%(Com_create_function)s+%(Com_create_index)s+%(Com_create_procedure)s+%(Com_create_server)s+%(Com_create_table)s+%(Com_create_trigger)s+%(Com_create_udf)s+%(Com_create_user)s+%(Com_create_view)s"),
  (194/255.0, 123/255.0, 206/255.0), (445, 520),
"""CREATE Statements Executed
Number of CREATE statements executed by the server (since server was started).

Create DB: %(Com_create_db)s
Create Event: %(Com_create_event)s
Create Function: %(Com_create_function)s
Create Index: %(Com_create_index)s
Create Procedure: %(Com_create_procedure)s
Create Server: %(Com_create_server)s
Create Table: %(Com_create_table)s
Create Trigger: %(Com_create_trigger)s
Create UDF: %(Com_create_udf)s
Create User: %(Com_create_user)s
Create View: %(Com_create_view)s"""),
 (None, DBSimpleCounter, ("ALTER\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_alter_db)s+%(Com_alter_db_upgrade)s+%(Com_alter_event)s+%(Com_alter_function)s+%(Com_alter_procedure)s+%(Com_alter_server)s+%(Com_alter_table)s+%(Com_alter_tablespace)s+%(Com_alter_user)s"),
  (194/255.0, 123/255.0, 206/255.0), (445, 560),
"""ALTER Statements Executed
Number of ALTER statements executed by the server (since server was started).
    
Alter DB: %(Com_alter_db)s
Alter DB Upgrade: %(Com_alter_db_upgrade)s
Alter Event: %(Com_alter_event)s
Alter Function: %(Com_alter_function)s
Alter Procedure: %(Com_alter_procedure)s
Alter Server: %(Com_alter_server)s
Alter Table: %(Com_alter_table)s
Alter Tablespace: %(Com_alter_tablespace)s
Alter User: %(Com_alter_user)s"""),
 
 (None, DBSimpleCounter, ("DROP\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_drop_db)s+%(Com_drop_event)s+%(Com_drop_function)s+%(Com_drop_index)s+%(Com_drop_procedure)s+%(Com_drop_server)s+%(Com_drop_table)s+%(Com_drop_trigger)s+%(Com_drop_user)s+%(Com_drop_view)s"),
  (194/255.0, 123/255.0, 206/255.0), (445, 600),
  """DROP Statements Executed
Number of DROP statements executed by the server (since server was started).
      
Drop DB: %(Com_drop_db)s
Drop Event: %(Com_drop_event)s
Drop Function: %(Com_drop_function)s
Drop Index: %(Com_drop_index)s
Drop Procedure: %(Com_drop_procedure)s
Drop Server: %(Com_drop_server)s
Drop Table: %(Com_drop_table)s
Drop Trigger: %(Com_drop_trigger)s
Drop User: %(Com_drop_user)s
Drop View: %(Com_drop_view)s"""),
 
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_separator.png"),), None, (None, None),
  (0, 0, 0), (570, 120),
  ""),
]

GLOBAL_DASHBOARD_WIDGETS_MYSQL_POST_80 = \
[
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_header_mysql.png"),), None, (None, None),
  (0, 0, 0), (380, 5),
  ""),
 (None, DBText, ("Primary MySQL Server activity\nand performance statistics.",), None, (None, None),
  (0.4, 0.4, 0.4), (376, 72),
  ""),
 
 ("Table Open Cache", DBRoundMeter, ("Efficiency",), None, (CRawValue, "%(Table_open_cache_hits)s/(%(Table_open_cache_hits)s+%(Table_open_cache_misses)s+0.0)"),
  (124/255.0, 193/255.0, 80/255.0), (380, 150),
  """Table Open Cache
      Cache for minimizing number of times MySQL
      will open database tables when accessed.
      
      Table open cache hits: %(Table_open_cache_hits)s
      Table open cache misses: %(Table_open_cache_misses)s"""),
 
 ("SQL Statements Executed (#)", DBTimeLineGraph, ("%.1f %s", 3, True), None, (CTupleDifferencePerSecond, "(%(Com_select)s,%(Com_insert)s+%(Com_update)s+%(Com_delete)s,%(Com_create_db)s+%(Com_create_event)s+%(Com_create_function)s+%(Com_create_index)s+%(Com_create_procedure)s+%(Com_create_server)s+%(Com_create_table)s+%(Com_create_trigger)s+%(Com_create_udf)s+%(Com_create_user)s+%(Com_create_view)s+%(Com_create_role)s+%(Com_alter_db)s+%(Com_alter_event)s+%(Com_alter_function)s+%(Com_alter_procedure)s+%(Com_alter_server)s+%(Com_alter_table)s+%(Com_alter_tablespace)s+%(Com_alter_user)s+%(Com_alter_user_default_role)s+%(Com_drop_db)s+%(Com_drop_event)s+%(Com_drop_function)s+%(Com_drop_index)s+%(Com_drop_procedure)s+%(Com_drop_server)s+%(Com_drop_table)s+%(Com_drop_trigger)s+%(Com_drop_user)s+%(Com_drop_view)s+%(Com_drop_role)s)"),
  [(255/255.0, 201/255.0, 2/255.0), (126/255.0, 142/255.0, 207/255.0), (194/255.0, 123/255.0, 206/255.0)], (350, 330),
  None),
 
 (None, DBSimpleCounter, ("SELECT\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_select)s"),
  (255/255.0, 201/255.0, 2/255.0), (350, 470),
  """SELECT Statements Executed
      
      Total since start: %(Com_select)s"""),
 
 (None, DBSimpleCounter, ("INSERT\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_insert)s"),
  (126/255.0, 142/255.0, 207/255.0), (350, 520),
  """INSERT Statements Executed
      
      Total since start: %(Com_insert)s"""),
 (None, DBSimpleCounter, ("UPDATE\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_update)s"),
  (126/255.0, 142/255.0, 207/255.0), (350, 560),
  """UPDATE Statements Executed
      
      Total since start: %(Com_update)s"""),
 (None, DBSimpleCounter, ("DELETE\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_delete)s"),
  (126/255.0, 142/255.0, 207/255.0), (350, 600),
  """DELETE Statements Executed
      
      Total since start: %(Com_delete)s"""),
 
 (None, DBSimpleCounter, ("CREATE\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_create_db)s+%(Com_create_event)s+%(Com_create_function)s+%(Com_create_index)s+%(Com_create_procedure)s+%(Com_create_server)s+%(Com_create_table)s+%(Com_create_trigger)s+%(Com_create_udf)s+%(Com_create_user)s+%(Com_create_view)s+%(Com_create_role)s"),
  (194/255.0, 123/255.0, 206/255.0), (445, 520),
"""CREATE Statements Executed
    Number of CREATE statements executed by the server (since server was started).
    
    Create DB: %(Com_create_db)s
    Create Event: %(Com_create_event)s
    Create Function: %(Com_create_function)s
    Create Index: %(Com_create_index)s
    Create Procedure: %(Com_create_procedure)s
    Create Role: %(Com_create_role)s
    Create Server: %(Com_create_server)s
    Create Table: %(Com_create_table)s
    Create Trigger: %(Com_create_trigger)s
    Create UDF: %(Com_create_udf)s
    Create User: %(Com_create_user)s
    Create View: %(Com_create_view)s"""),
 (None, DBSimpleCounter, ("ALTER\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_alter_db)s+%(Com_alter_event)s+%(Com_alter_function)s+%(Com_alter_procedure)s+%(Com_alter_server)s+%(Com_alter_table)s+%(Com_alter_tablespace)s+%(Com_alter_user)s+%(Com_alter_user_default_role)s"),
  (194/255.0, 123/255.0, 206/255.0), (445, 560),
"""ALTER Statements Executed
    Number of ALTER statements executed by the server (since server was started).
    
    Alter DB: %(Com_alter_db)s
    Alter Event: %(Com_alter_event)s
    Alter Function: %(Com_alter_function)s
    Alter Procedure: %(Com_alter_procedure)s
    Alter Server: %(Com_alter_server)s
    Alter Table: %(Com_alter_table)s
    Alter Tablespace: %(Com_alter_tablespace)s
    Alter User: %(Com_alter_user)s
    Alter User Default Role: %(Com_alter_user_default_role)s"""),
 
 (None, DBSimpleCounter, ("DROP\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Com_drop_db)s+%(Com_drop_event)s+%(Com_drop_function)s+%(Com_drop_index)s+%(Com_drop_procedure)s+%(Com_drop_server)s+%(Com_drop_table)s+%(Com_drop_trigger)s+%(Com_drop_user)s+%(Com_drop_view)s+%(Com_drop_role)s"),
  (194/255.0, 123/255.0, 206/255.0), (445, 600),
  """DROP Statements Executed
      Number of DROP statements executed by the server (since server was started).
      
      Drop DB: %(Com_drop_db)s
      Drop Event: %(Com_drop_event)s
      Drop Function: %(Com_drop_function)s
      Drop Index: %(Com_drop_index)s
      Drop Procedure: %(Com_drop_procedure)s
      Drop Role: %(Com_drop_role)s
      Drop Server: %(Com_drop_server)s
      Drop Table: %(Com_drop_table)s
      Drop Trigger: %(Com_drop_trigger)s
      Drop User: %(Com_drop_user)s
      Drop View: %(Com_drop_view)s"""),
 
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_separator.png"),), None, (None, None),
  (0, 0, 0), (570, 120),
  ""),
]

GLOBAL_DASHBOARD_WIDGETS_INNODB = \
[
 # InnoDB
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_header_innodb.png"),), None, (None, None),
  (0, 0, 0), (710, 5),
  ""),
 (None, DBText, ("Overview of the InnoDB Buffer Pool and disk activity\ngenerated by the InnoDB storage engine.",), None, (None, None),
  (0.4, 0.4, 0.4), (655, 72),
  ""),

 # Buffer Pool
 (None, DBSimpleCounter, ("read reqs.\n%.0f %s\npages/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_buffer_pool_read_requests)s"),
  READ_COLOR, (610, 160),
  """InnoDB Buffer Pool Read Requests
The number of logical read requests InnoDB has done to the buffer pool.

Total: %(Innodb_buffer_pool_read_requests)s"""),
 (None, DBSimpleCounter, ("write reqs.\n%.0f %s\npages/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_buffer_pool_write_requests)s"),
  WRITE_COLOR, (610, 220),
  """InnoDB Buffer Pool Write Requests
The number of logical write requests InnoDB has done to the buffer pool.

Total: %(Innodb_buffer_pool_write_requests)s"""),

 ("InnoDB Buffer Pool", DBRoundMeter, ("Usage",), None, (CRawValue, "(%(Innodb_buffer_pool_pages_data)s+%(Innodb_buffer_pool_pages_misc)s)/(%(Innodb_buffer_pool_pages_total)s+0.0)"),
  (124/255.0, 193/255.0, 80/255.0), (720, 150),
"""InnoDB Buffer Pool Usage Rate
How much of the InnoDB buffer pool is in use, from the amount allocated to it.

Usage Rate: ${(%(Innodb_buffer_pool_pages_data)s+%(Innodb_buffer_pool_pages_misc)s)/(%(Innodb_buffer_pool_pages_total)s+0.0)}

Total Pages Available: %(Innodb_buffer_pool_pages_total)s
Pages Used for Data: %(Innodb_buffer_pool_pages_data)s
Pages Used Internally by InnoDB: %(Innodb_buffer_pool_pages_misc)s
Pages Free: %(Innodb_buffer_pool_pages_free)s"""),

 (None, DBSimpleCounter, ("disk reads\n%.0f %s\n#/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_buffer_pool_reads)s"),
  READ_COLOR, (890, 180),
"""InnoDB Buffer Pool Reads
The number of logical reads that InnoDB could not satisfy from the buffer pool, and had to read directly from the disk.
    
Total: %(Innodb_buffer_pool_reads)s"""),

 ("Redo Log", DBSimpleCounter, ("data written\n%.0f %sB/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_os_log_written)s"),
  WRITE_COLOR, (606, 330),
"""Bytes Written to InnoDB Redo Log
The number of bytes written to the InnoDB redo log files.
    
Total: %(Innodb_os_log_written)s"""),

 (None, DBSimpleCounter, ("writes\n%.0f %s#/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_log_writes)s"),
  WRITE_COLOR, (606, 380),
"""Writes to InnoDB Redo Log
The number of physical writes to the InnoDB redo log file.

Total: %(Innodb_log_writes)s"""),

 ("Doublewrite Buffer", DBSimpleCounter, ("writes\n%.0f %s/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_dblwr_writes)s"),
  WRITE_COLOR, (606, 480),
"""Write Operations to InnoDB Doublewrite Buffer
The number of doublewrite operations that have been performed.

Total: %(Innodb_dblwr_writes)s"""),
 
 # Storage
 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_arrow_out_static.png"),), None, (None, None),
  (0, 0, 0), (934, 360),
  ""),

 ("InnoDB Disk Writes", DBTimeLineGraph, ("%.2f %sB", True), None, (CSingleDifferencePerSecond, "%(Innodb_data_written)s"),
  WRITE_COLOR, (738, 330),
  None),
 (None, DBSimpleCounter, ("writing\n%.2f %sB/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_data_written)s"),
  WRITE_COLOR, (916, 410),
  """InnoDB Data Written
Total amount of data in bytes written in file operations by the InnoDB storage engine.

Total: %(Innodb_data_written)s"""),

 (None, DBImage, (mforms.App.get().get_resource_path("dashboard_arrow_in_static.png"),), None, (None, None),
  (0, 0, 0), (934, 535),
  ""),

 ("InnoDB Disk Reads", DBTimeLineGraph, ("%.2f %sB", True), None, (CSingleDifferencePerSecond, "%(Innodb_data_read)s"),
  READ_COLOR, (738, 500),
  None),
 (None, DBSimpleCounter, ("reading\n%.2f %sB/s", True), None, (CSingleDifferencePerSecond, "%(Innodb_data_read)s"),
  READ_COLOR, (916, 580),
  """InnoDB Data Read
Total amount of data in bytes read in file operations by the InnoDB storage engine.

Total: %(Innodb_data_read)s"""),
]


class WbAdminDashboard(WbAdminBaseTab):
    min_server_version = (5,6,6)
    
    _refresh_tm = None
    drawbox = None
    _form_deactivated_conn = None
    
    @classmethod
    def wba_register(cls, admin_context):
        admin_context.register_page(cls, "wba_performance", "Dashboard", False)
    
    @classmethod
    def identifier(cls):
        return "admin_dashboard"

    def set_needs_repaint(self, x, y, w, h):
        self.drawbox.set_needs_repaint()

    def create_ui(self):
        #self.create_basic_ui("title_dashboard.png", "Dashboard")


        self._form_deactivated_conn = mforms.Form.main_form().add_deactivated_callback(self.form_deactivated)


        self.content = mforms.newScrollPanel(0)

        self.drawbox = RenderBox(self)
        self.canvas = Canvas(self.set_needs_repaint)
        self.drawbox.canvas = self.canvas
        self.drawbox.set_size(1024, 700)
        self.content.add(self.drawbox)

        self.add(self.content, True, True)

        self.widgets = []
        
        self.last_refresh_time = None
        
        #
        self.drawbox.variable_values = self.ctrl_be.server_variables
        server_version = Version.fromgrt(self.ctrl_be.target_version)
        GLOBAL_DASHBOARD_WIDGETS = GLOBAL_DASHBOARD_WIDGETS_NETWORK + GLOBAL_DASHBOARD_WIDGETS_MYSQL_PRE_80 + GLOBAL_DASHBOARD_WIDGETS_INNODB
        if server_version and server_version.is_supported_mysql_version_at_least(8, 0, 0):
            GLOBAL_DASHBOARD_WIDGETS = GLOBAL_DASHBOARD_WIDGETS_NETWORK + GLOBAL_DASHBOARD_WIDGETS_MYSQL_POST_80 + GLOBAL_DASHBOARD_WIDGETS_INNODB
        # create all widgets
        for caption, wclass, args, init, (calc, calc_expr), color, pos, hover_text in GLOBAL_DASHBOARD_WIDGETS:
            if caption:
                fig = TextFigure(caption)
                fig.set_text_color(0.5, 0.5, 0.5)
                fig.set_font_size(11)
                fig.set_font_bold(True)
                self.drawbox.add(fig)
                fig.move(pos[0], pos[1] - 20)
        
            w = wclass(calc(calc_expr) if calc else None, *args)
            self.drawbox.add(w)
            w.set_main_color(color)
            w.move(*pos)
            if hover_text:
                w.hover_text_template = hover_text
            
            if init:
                init_calc, init_expr = init
                w.init(init_calc(init_expr).handle(self.ctrl_be.server_variables, None))

            self.widgets.append(w)


        self.refresh()
        self._refresh_tm = mforms.Utilities.add_timeout(self.ctrl_be.status_variable_poll_interval, self.refresh)
            
        self.ctrl_be.add_me_for_event("server_started", self)
        self.ctrl_be.add_me_for_event("server_stopped", self)


    def server_started_event(self):
        for widget in self.widgets:
            if not hasattr(widget, "calc"):
                continue
              
            if issubclass(type(widget.calc), CDifferencePerSecond):
                widget.calc.reset()

    #---------------------------------------------------------------------------
    def server_stopped_event(self):
        pass

    def repaint(self):
        self.drawbox.set_needs_repaint()
        return True


    def shutdown(self):
        if self._form_deactivated_conn:
            self._form_deactivated_conn.disconnect()
            self._form_deactivated_conn = None

        if self._refresh_tm:
            mforms.Utilities.cancel_timeout(self._refresh_tm)
            self._refresh_tm = None


    def refresh(self):
        status_variables, timestamp = self.ctrl_be.status_variables, self.ctrl_be.status_variables_time
        if self.last_refresh_time != timestamp:
            for w in self.widgets:
                if hasattr(w, 'process'):
                    w.process(status_variables, timestamp)

            self.drawbox.variable_values.update(status_variables)

            self.drawbox.set_needs_repaint()

        return True


    def form_deactivated(self):
        if self.drawbox:
            self.drawbox.close_tooltip()


    def page_deactivated(self):
        if self.drawbox:
            self.drawbox.close_tooltip()

    def relayout(self):
        full_width = max(1024, self.content.get_width())
        full_height = max(700, self.content.get_height())

        # return offset
        return (full_width - 1024) / 2, (full_height - 700) / 2

