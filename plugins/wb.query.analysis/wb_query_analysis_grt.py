# Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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


from wb import DefineModule, wbinputs
import grt
import mforms

from workbench.utils import Version
from workbench.graphics.cairo_utils import Context
from explain_renderer import ExplainContext, decode_json

ModuleInfo = DefineModule(name= "SQLIDEQueryAnalysis", author= "Oracle Corp.", version= "1.0")


class JSONTreeViewer(mforms.TreeNodeView):
    def __init__(self):
        mforms.TreeNodeView.__init__(self, mforms.TreeAltRowColors|mforms.TreeShowColumnLines|mforms.TreeShowRowLines)
        self.add_column(mforms.StringColumnType, "Key", 200)
        self.add_column(mforms.StringColumnType, "Value", 300)
        self.end_columns()

    def display_data(self, data):
        def add_nodes(node, create_node, data):
            if type(data) is dict:
                if node:
                    node.set_string(1, "<dict>")
                for key, value in data.items():
                    ch = create_node()
                    ch.set_string(0, key)
                    add_nodes(ch, ch.add_child, value)
            elif type(data) is list:
                if node:
                    node.set_string(1, "<list>")
                for i, value in enumerate(data):
                    ch = create_node()
                    ch.set_string(0, str(i))
                    add_nodes(ch, ch.add_child, value)
            else:
                if not node:
                    node = create_node()
                if type(data) is bool:
                    node.set_string(1, "true" if data else "false")
                else:
                    node.set_string(1, str(data))
    
        data = decode_json(data)
        self.clear()
        add_nodes(None, self.add_node, data)


class RenderBox(mforms.PyDrawBox):
    def __init__(self, context):
        mforms.PyDrawBox.__init__(self)
        self.set_managed()
        self.set_release_on_add()

        self.set_instance(self)
        self.offset = (0, 0)
        self.size = None
        self.vertical = False
        self.node_spacing = 20
        self.econtext = context
    
        self.drag_offset = None
    
    def mouse_down(self, b, x, y):
        if b == 0:
            self.drag_offset = (x, y)
        x -= self.offset[0]
        y -= self.offset[1]
        self.econtext._canvas.mouse_down(b, x, y)

    def mouse_up(self, b, x, y):
        if b == 0:
            self.drag_offset = None
        x -= self.offset[0]
        y -= self.offset[1]
        self.econtext._canvas.mouse_up(b, x, y)

    def mouse_move(self, x, y):
        if self.drag_offset:
            self.scroll.set_viewport_offset()
        x -= self.offset[0]
        y -= self.offset[1]
        self.econtext._canvas.mouse_move(x, y)

    def mouse_leave(self):
        self.econtext._canvas.mouse_leave()

    def relayout(self):
        w, h = self.econtext.layout()
        if self.get_width() != w or self.get_height() != h:
            self.set_size(w, h)

    
    def repaint(self, cr, x, y, w, h):
        c = Context(cr)

        c.set_source_rgb(1, 0xfc/255.0, 0xe5/255.0)

        try:
            dw, dh = self.econtext.size
            xx = 0
            yy = 0
            if dw < self.get_width():
                xx = (self.get_width() - dw)/2
            if dh < self.get_height():
                yy = (self.get_height() - dh)/2
            self.offset = (xx, yy)
            self.econtext.set_offset(xx, yy)
            self.econtext.repaint(c)
        except Exception:
            import traceback
            grt.log_error("vexplain", "Exception rendering explain output: %s\n" % traceback.format_exc())



def newToolBarItem(*args):
    item = mforms.ToolBarItem(*args)
    item.set_managed()
    return item


EXPLAIN_COLUMN_WIDTHS = {
  "id" : 40,
  "select_type" : 120,
  "table" : 100,
  "type" : 50,
  "possible_keys" : 200,
  "key" : 200,
  "key_len" : 40,
  "ref" : 200,
  "rows" : 50,
  "Extra" : 200
}


class QueryPlanTab(mforms.Box):
    node_spacing = 30
    vertical = True

    def __init__(self, context, server_version):
        mforms.Box.__init__(self, False)
        self.set_managed()
        self.set_release_on_add()

        self._context = context

        get_resource_path = mforms.App.get().get_resource_path

        self.toolbar = mforms.newToolBar(mforms.SecondaryToolBar)
        self.toolbar.set_back_color("#ffffff")
        
        l = newToolBarItem(mforms.LabelItem)
        l.set_text("Display Cost Info:")
        self.toolbar.add_item(l)

        item = newToolBarItem(mforms.SelectorItem)
        item.set_selector_items(["Prefix Cost (read + eval)", "Read Cost", "Eval Cost", "Data Read per Join"])
        item.add_activated_callback(self.display_cost)
        self.toolbar.add_item(item)
        cost_type_item = item

        # cost info was added in 5.7.2
        has_cost_info = server_version >= Version(5, 7)
        if not has_cost_info:
            item.set_enabled(False)

#item = newToolBarItem(mforms.SelectorItem)
#        item.set_selector_items(["Show Aggregated Costs", "Show Individual Costs"])
#        item.add_activated_callback(self.toggle_aggregated)
#        self.toolbar.add_item(item)


        #btn = newToolBarItem(mforms.SegmentedToggleItem)
        #btn.set_icon(get_resource_path("qe_resultset-tb-switcher_grid_off_mac.png"))
        #btn.set_alt_icon(get_resource_path("qe_resultset-tb-switcher_grid_on_mac.png"))
        #self.toolbar.add_item(btn)

        #btn = newToolBarItem(mforms.SegmentedToggleItem)
        #btn.set_icon(get_resource_path("qe_resultset-tb-switcher_explain_off.png"))
        #btn.set_alt_icon(get_resource_path("qe_resultset-tb-switcher_explain_on.png"))
        #self.toolbar.add_item(btn)

        s = newToolBarItem(mforms.SeparatorItem)
        self.toolbar.add_item(s)

        btn = newToolBarItem(mforms.ActionItem)
        btn.set_icon(get_resource_path("tiny_saveas.png"))
        btn.add_activated_callback(self.save)
        btn.set_tooltip("Save image to an external file.")
        self.toolbar.add_item(btn)

        self.add(self.toolbar, False, True)

        # Query Plan diagram
        self.scroll = mforms.newScrollPanel(mforms.ScrollPanelNoFlags)
        self.scroll.set_back_color("#ffffff")
        self.scroll.set_visible_scrollers(True, True)
        
        #self.img = mforms.newImageBox()
        self.drawbox = RenderBox(self._context)
        self.scroll.add(self.drawbox)
        
        self.drawbox.node_spacing = self.node_spacing
        self.drawbox.vertical = self.vertical
        self.add(self.scroll, True, True)
    
        self.display_cost(cost_type_item)
    

    def display_cost(self, item):
        text = item.get_text()
        if text:
            cost = text.lower().split()[0]
            if cost == "eval":
                self._context.show_cost_info_type("eval_cost")
            elif cost == "prefix":
                self._context.show_cost_info_type("prefix_cost")
            elif cost == "read":
                self._context.show_cost_info_type("read_cost")
            elif cost == "data":
                self._context.show_cost_info_type("data_read_per_join")
            else:
                grt.log_error("vexplain", "Unknown cost info type: %s\n" % cost)
            self.drawbox.set_needs_repaint()


    def toggle_aggregated(self, item):
        self._context.show_aggregated_cost_info("aggregated" in item.get_text().lower())
        self.drawbox.set_needs_repaint()


    def save(self, item):
        ch = mforms.FileChooser(mforms.SaveFile)
        ch.set_extensions("PNG image (*.png)|*.png", "png")
        ch.set_title("Save Image As")
        ch.set_path("explain.png")
        if ch.run_modal():
            self._context.export_to_png(ch.get_path())


    def set_needs_repaint(self, x, y, w, h):
        self.drawbox.set_needs_repaint()


class ExplainTab(mforms.AppView):
    _query_plan = None
    _costs_tree = None
    _explain_context = None

    def __init__(self, server_version, query, json_text, explain):
        mforms.AppView.__init__(self, False, "QueryExplain", False)
        self.set_managed()
        self.set_release_on_add()

        self.on_close(self.on_tab_close)

        self._form_deactivated_conn = mforms.Form.main_form().add_deactivated_callback(self.form_deactivated)
        
        self._query = query
        self.tabview = mforms.newTabView(0)
        self.tabview.add_tab_changed_callback(self.tab_changed)
        self.set_back_color("#ffffff")
        
        default_tab = grt.root.wb.state.get("wb.query.analysis:ActiveExplainTab", 0)

        if json_text:
            json = decode_json(json_text)
            self._explain_context = ExplainContext(json)
            self._query_plan = QueryPlanTab(self._explain_context, server_version)
            self._explain_context.init_canvas(self._query_plan.drawbox, self._query_plan.set_needs_repaint)
            #self._explain_context._canvas.set_background_color(1, 0xfc/255.0, 0xe5/255.0)
            # Initial layouting of the plan diagram
            self._query_plan.drawbox.relayout()
            self.tabview.add_page(self._query_plan, "Query Plan")
        
            self._raw_explain = mforms.CodeEditor()
            self._raw_explain.set_value(json_text)
            #self._raw_explain.enable_folding(True)
            self._raw_explain.set_language(mforms.LanguagePython)
            self._raw_explain.set_features(mforms.FeatureReadOnly, 1)
            self.tabview.add_page(self._raw_explain, "Raw Explain Data")

            #self._costs_tree = self.fill_costs_tree(json)
        #self.tabview.add_page(self._costs_tree, "Query Costs")

        # Good old explain
        if explain:
            self.explain_tree = mforms.newTreeNodeView(mforms.TreeFlatList|mforms.TreeShowColumnLines|mforms.TreeShowRowLines|mforms.TreeAltRowColors)
            self.explain_tree.add_column_resized_callback(self.column_resized)
            c = len(explain.columns)
            rows_column = None
            
            saved_widths = grt.root.wb.state.get("wb.query.analysis:ExplainTreeColumnWidths", None)
            if saved_widths:
                saved_widths = [int(i) for i in saved_widths.split(",")]
            
            for i, column in enumerate(explain.columns):
                width = saved_widths[i] if saved_widths and i < len(saved_widths) else EXPLAIN_COLUMN_WIDTHS.get(column.name, 100)
                if column.name == "rows":
                    rows_column = i
                    self.explain_tree.add_column(mforms.LongIntegerColumnType, column.name, width)
                else:
                    self.explain_tree.add_column(mforms.StringColumnType, column.name, width)
            self.explain_tree.end_columns()
            
            if explain.goToFirstRow():
                while True:
                    node = self.explain_tree.add_node()
                    for i in range(c):
                        value = explain.stringFieldValue(i)
                        if i == rows_column:
                            node.set_long(i, long(value) if value else 0)
                        else:
                            node.set_string(i, value)
                    if not explain.nextRow():
                        break
            self.tabview.add_page(self.explain_tree, "Explain")
            explain.reset_references()

        self.add(self.tabview, True, True)

        if self._explain_context:
            self.tabview.set_active_tab(default_tab)


    def column_resized(self, column):
        sizes = []
        for i in range(self.explain_tree.get_column_count()):
            sizes.append(self.explain_tree.get_column_width(i))
        grt.root.wb.state["wb.query.analysis:ExplainTreeColumnWidths"] = ",".join([str(i) for i in sizes])


    def tab_changed(self):
        if self._query_plan and self._explain_context:
            if self.tabview.get_active_tab() == 0:
                self._explain_context._canvas.activate()
            else:
                self._explain_context._canvas.deactivate()
                self._explain_context.close_tooltip()
            grt.root.wb.state["wb.query.analysis:ActiveExplainTab"] = self.tabview.get_active_tab()


    def fill_costs_tree(self, json):
        tree = mforms.newTreeNodeView(mforms.TreeFlatList|mforms.TreeShowColumnLines|mforms.TreeShowRowLines|mforms.TreeAltRowColors)
        return tree


    def form_deactivated(self):
        if self._query_plan and self._explain_context:
            self._explain_context.close_tooltip()

    def on_tab_close(self):
        self.form_deactivated()
        if self._form_deactivated_conn:
            self._form_deactivated_conn.disconnect()
            self._form_deactivated_conn = None


@ModuleInfo.plugin("wb.sqlide.visual_explain", caption="Visual Explain", input=[wbinputs.currentQueryEditor()])
@ModuleInfo.export(grt.INT, grt.classes.db_query_QueryEditor)
def visualExplain(editor):
    version = Version.fromgrt(editor.owner.serverVersion)
    
    statement = editor.currentStatement
    if statement:
        try:
            explain = editor.owner.executeQuery("EXPLAIN %s" % statement, 1)
        except Exception, e:
            mforms.Utilities.show_error("Explain for Connection",
                                        "Error executing explain\n%s" % (e), "OK", "", "")
            return 0
        
        if not explain:
            mforms.Utilities.show_error("Explain for Connection",
                                        "Error executing explain.", "OK", "", "")
            return 0
        
        json = None
        if version.is_supported_mysql_version_at_least(5, 6):
            rset = editor.owner.executeQuery("EXPLAIN FORMAT=JSON %s" % statement, 1)
            if rset and rset.goToFirstRow():
                json = rset.stringFieldValue(0)
                rset.reset_references()

        view = ExplainTab(version, statement, json, explain if explain else None)
        dock = mforms.fromgrt(editor.resultDockingPoint)
        dock.dock_view(view, "", 0)
        dock.select_view(view)
        dock.set_view_title(view, "Explain")
    
    return 0


@ModuleInfo.export(grt.INT, grt.classes.db_query_QueryEditor, grt.STRING, grt.STRING)
def visualExplainForConnection(editor, conn_id, the_query):
    version = Version.fromgrt(editor.owner.serverVersion)

    try:
        explain = editor.owner.executeManagementQuery("EXPLAIN FOR CONNECTION %s" % conn_id, 1)
    except grt.DBError, e:
        if e.args[1] == 0:
            mforms.Utilities.show_message("Explain for Connection",
                                          "Explain for connection %s did not generate any output." % conn_id, "OK", "", "")
        else:
            mforms.Utilities.show_error("Explain for Connection",
                                        "Error executing explain for connection %s\n%s" % (conn_id, e), "OK", "", "")
        return 0
    except Exception, e:
        mforms.Utilities.show_error("Explain for Connection",
                                    "Error executing explain for connection %s\n%s" % (conn_id, e), "OK", "", "")
        return 0

    if not explain:
        mforms.Utilities.show_error("Explain for Connection",
                                    "Error executing explain for connection %s" % conn_id, "OK", "", "")
        return 0
    
    if version.is_supported_mysql_version_at_least(5, 6):
        rset = editor.owner.executeManagementQuery("EXPLAIN FORMAT=JSON FOR CONNECTION %s" % conn_id, 1)
        if rset and rset.goToFirstRow():
            json = rset.stringFieldValue(0)
            rset.reset_references()
        else:
            json = None
    
    view = ExplainTab(version, the_query, json, explain if explain else None)
    dock = mforms.fromgrt(editor.resultDockingPoint)
    dock.dock_view(view, "", 0)
    dock.select_view(view)
    dock.set_view_title(view, "Explain for Connection")
    
    return 0

