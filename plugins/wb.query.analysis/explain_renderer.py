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

import mforms


from workbench.log import log_error


from workbench.graphics.canvas import VBoxFigure, Canvas, DiamondShapeFigure, RectangleShapeFigure, TextFigure, HFill, draw_varrow, draw_harrow
from workbench.graphics.cairo_utils import ImageSurface, Context

def decode_json(text):
    return eval(text, {"false":False, "true":True})




def fmt_number(c):
    if c >= 1000*1000*1000:
        return "%.2fG" % (c/(1000*1000*1000.0))
    elif c >= 1000*1000:
        return "%.02fM" % (c/(1000*1000.0))
    elif c >= 1000:
        return "%.0fK" % (c/1000.0)
    return str(c)


def fmt_rows(r):
    if r == 1:
        return "%i row" % r
    else:
        if r > 1000000000:
            return "%.2fG rows" % (r/1000000000.0)
        elif r > 1000000:
            return "%.2fM rows" % (r/1000000.0)
        elif r > 1000:
            return "%.2fK rows" % (r/1000.0)
        else:
            return "%s rows" % r


class MyCanvas(Canvas):
    def __init__(self, cb):
        Canvas.__init__(self, cb)


    def figure_at(self, x, y):
        fig = Canvas.figure_at(self, x, y)
        if fig:
            return fig.get_figure_at(x, y)
        return None


class ExplainNode(VBoxFigure):
    arrow_w = 3
    arrow_h = 7
    is_operation = False
    
    def __init__(self, context):
        VBoxFigure.__init__(self)
        self._context = context
        self.parent = None
        self.cost_info = None
        
        self._figure = None

        self._aggr_cost_value = None
        self._aggr_cost_pct = None
        
        self.on_hover_in = self.handle_hover_in
        self.on_hover_out = self.handle_hover_out


    @property
    def inner_width(self):
        """Width of the figure itself, without accounting for child nodes"""
        return self._figure.width

    @property
    def inner_height(self):
        return self._figure.height


    @property
    def varrow_source(self):
        return int(self.root_x + self.vconnect_pos_offset)+0.5, self._figure.root_y
    
    
    @property
    def varrow_target(self):
        return int(self.root_x + self.vconnect_pos_offset)+0.5, self._figure.root_y + self.inner_height


    @property
    def harrow_target(self):
        return self._figure.root_x, int(self.root_y + self.hconnect_pos_offset)+0.5


    @property
    def harrow_target_right(self):
        return int(self._figure.root_x + self._figure.width)+0.5, int(self.root_y + self.hconnect_pos_offset)+0.5
    
    @property
    def harrow_source(self):
        return self._figure.root_x + self.inner_width, int(self.root_y + self.hconnect_pos_offset)+0.5


    @property
    def vconnect_pos_offset(self):
        return self._figure.x + self.inner_width/2


    @property
    def hconnect_pos_offset(self):
        return self._figure.y + self._figure.height/2


    @property
    def cost_value(self):
        name = self._context.displayed_cost_info
        if self.cost_info and name in self.cost_info:
            cost_value = self.cost_info.get(name)
            try:
                return float(cost_value)
            except ValueError: # some values have a unit suffix
                value = cost_value[:-1]
                unit = cost_value[-1]
                value = float(value)
                if unit == "K":
                    return value * 1000
                elif unit == "M":
                    return value * 1000*1000
                elif unit == "G":
                    return value * 1000*1000*1000
                else:
                    return value
        return None
    
    
    @property
    def rows_count(self):
        """The row count number to be shown in outgoing node arrows
            """
        return None
    
    
    def get_figure_at(self, x, y):
        if self.root_x < x < self.root_x + self.width and self.root_y < y < self.root_y + self.height:
            for f in self.children:
                ff = f.get_figure_at(x, y)
                if ff:
                    return ff

            if self._figure.root_x < x < self._figure.root_x + self.inner_width and self._figure.root_y < y < self._figure.root_y + self.inner_height:
                return self
        return None

    
    def dump(self, s, level=0):
        if self.children:
            s.write(level*"  "+repr(self)+" (\n")
            for x in self.children:
                x.dump(s, level+1)
                s.write("\n")
            s.write(level*"  " + ")")
        else:
            s.write(level*"  "+repr(self))


    def get_line_width(self):
        rows_count = self.rows_count
        line_width = 1
        if rows_count:
            from math import log
            line_width = max(min(int(log(rows_count, 20)+0.5), 20), 1)
        return line_width


    def render_cost(self, cr, x, y):
        cost = self.cost_value
        if cost is not None:
            cr.set_source_rgba(0, 0, 0, 1)
            cr.move_to(x, y)
            cr.set_font_size(10)
            cr.show_text(self._context.fmt_cost(cost))


    def render_row_count(self, cr, x, y):
        rows_count = self.rows_count
        if rows_count is not None:
            cr.set_source_rgba(0, 0, 0, 1)
            cr.move_to(x, y)
            cr.set_font_size(10)
            cr.show_text(fmt_rows(rows_count))


    def do_render_extras(self, cr):
        pass


    def render_extras(self, cr):
        self.do_render_extras(cr)

        for ch in self.children:
            assert ch.parent == self
            ch.render_extras(cr)

    def render(self, cr):
        self.do_render(cr)
        self.render_extras(cr)


    def do_render(self, cr):
        VBoxFigure.render(self, cr)

        cr.save()
        cr.translate(self.x, self.y)

        for ch in self.children:
            ch.do_render(cr)

        #ctx = cr
        #ctx.rectangle(0, 0, self.width, self.height)
        #ctx.set_line_width(1)
        #ctx.set_source_rgba(0.3, 0.3, 0.7, 1)
        #ctx.stroke()
        
        #ctx.set_source_rgba(1, 0, 0, 1)
        #ctx.rectangle(self.vconnect_pos_offset-1, 0, 2, 2)
        #ctx.fill()

        cr.restore()


    def do_relayout(self, ctx):
        VBoxFigure.do_relayout(self, ctx)
        
        if self.children:
            # calculate layout of the child item
            child = self.children[0]
            child.do_relayout(ctx)

            # get the offset of the point in the child item that should be
            # aligned with the parent item
            child_align_x = child.vconnect_pos_offset
            child_width = child.width

            # increase the size of this item, to allow fitting the child item
            self._width = child_width
            self._height = self.inner_height + self._context.vspacing + child.height

            # position the child
            child.move(0, self.inner_height + self._context.vspacing)
    
            # position the inner figure of the node so it aligns with the center of the child
            self._figure.move(child_align_x - self.inner_width/2, self._figure.y)


    # hover tip handling
    def handle_hover_out(self, fig, x, y):
        if self._context.tooltip:
            self._context.tooltip.close()
            self._context.tooltip = None


    @property
    def hint_pos_x(self):
        return self.root_x + self.width


    def get_hint_text(self):
        return None


    def handle_hover_in(self, fig, x, y):
        if self._context.tooltip:
            self._context.tooltip.close()
            self._context.tooltip = None
        
        text = self.get_hint_text()
        if text:
            self._context.tooltip = mforms.newPopover(mforms.PopoverStyleTooltip)

            xx, yy = self._context.client_to_screen(fig._figure.root_x + fig.inner_width, fig._figure.root_y + fig.inner_height/2)
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
        
            self._context.tooltip.set_size(max(box.get_width(), 100), max(box.get_height(), 50))
        
            self._context.tooltip.set_content(box)
            self._context.tooltip.show(xx, yy, mforms.Right)



class NestedLoopNode(ExplainNode):
    is_operation = True

    def __init__(self, context, join_buffer, left_child, right_child):
        ExplainNode.__init__(self, context)
        self.join_buffer = join_buffer
        
        left_child.parent = self
        right_child.parent = self
        
        self.child_aside = left_child
        self.child_below = right_child
        
        if join_buffer == "nested_loop":
            caption = "nested\nloop"
        elif join_buffer == "Block Nested Loop":
            caption = "block\nnested\nloop"
        elif join_buffer == "Batched Key Access":
            caption = "batched\nkey\naccess"
        elif join_buffer == "Batched Key Access (unique)":
            caption = "batched\nkey\naccess (u)"
        else:
            caption = join_buffer.replace(" ", "\n")
        self._figure = DiamondShapeFigure(caption)
        self._figure.set_layout_flags(0)
        self._figure.set_color(0.5, 0.5, 0.5, 1)
        self._figure.set_text_color(0, 0, 0, 1)
        #        self._figure.set_fill_color(0.8, 0.8, 0.8, 1)
        self._figure.set_line_width(2)
        self._figure.set_font_size(11)
        self._figure.set_usize(self._context.default_height, self._context.default_height)
        
        self.add(self._figure)


    @property
    def children(self):
        return [self.child_aside, self.child_below]


    @property
    def vconnect_pos_offset(self):
        return self.child_aside.width + self._context.hspacing + self.child_below.inner_width/2


    @property
    def rows_count(self):
        return self.child_below.rows_produced


    def __repr__(self):
        return self.join_buffer


    def do_render_extras(self, cr):
        if self.parent and not isinstance(self.parent, MaterializedTableNode):
            cr.save()
            cr.set_source_rgba(0, 0, 0, 1)
            cr.set_line_width(self.get_line_width())
            if isinstance(self.parent, NestedLoopNode):
                cr.move_to(self.harrow_source[0], self.parent.harrow_target[1])
                cr.line_to(*self.parent.harrow_target)
                cr.stroke()
                draw_harrow(cr, self.parent.harrow_target, 10, 6)
                cr.fill()
                self.render_row_count(cr, self.harrow_source[0] + 4, self.harrow_source[1] - 8)
            else:
                cr.move_to(*self.varrow_source)
                cr.line_to(self.varrow_source[0], self.parent.varrow_target[1])
                cr.stroke()
                draw_varrow(cr, (self.varrow_source[0], self.parent.varrow_target[1]), 10, 6)
                cr.fill()
                self.render_row_count(cr, self.varrow_source[0] + 4, self.varrow_source[1])
            cr.restore()


    def do_relayout(self, ctx):
        # 1 node directly under this and the other to lower/left
        # if it's another NestedLoopNode, then it should just be on the side too
        below = self.child_below
        aside = self.child_aside

        self._figure.do_relayout(ctx)
        below.do_relayout(ctx)
        aside.do_relayout(ctx)

        total_width = below.width + aside.width + self._context.hspacing
        self._width = max(self.inner_width, total_width)
        # layout other nested loop nodes horizontally
        if isinstance(aside, NestedLoopNode):
            self._height = max(self._figure.height + self._context.vspacing + below.height,
                               aside.height)
            self._figure.move(self.vconnect_pos_offset - self.inner_width/2,
                              0)
            aside.move(0, self._figure.y)
        else:
            self._height = self._figure.height + self._context.vspacing + max(below.height, aside.height)
            self._figure.move(self.vconnect_pos_offset - self.inner_width/2, 0)

            aside.move(0, self._figure.height + self._context.vspacing)
        below.move(aside.width + self._context.hspacing, self._figure.height + self._context.vspacing)



BLUE = (0.25, 0.5, 0.75, 1)
GREEN = (0.0, 0.5, 0.0, 1)
YELLOW = (0.75, 0.75, 0.0, 1)
ORANGE = (0.75, 0.5, 0.0, 1)
RED = (0.75, 0.25, 0.25, 1)
BLACK = (0, 0, 0, 1)
WHITE = (1, 1, 1, 1)

class TableNode(ExplainNode):
    col_join_types = [
                      ("system",          BLUE, "Single Row\n(system constant)", "Very low cost"),
                      ("const",           BLUE, "Single Row\n(constant)", "Very low cost"),
                      ("eq_ref",          GREEN, "Unique Key Lookup", """Low - The optimizer is able to find an index that it can use to retrieve required records.
Fast because the index search leads directly to the page with all the row data"""),
                      ("ref",             GREEN, "Non-Unique Key Lookup", "Low-medium - Low if number of matching rows is small, higher as the number of rows increases."),
                      ("fulltext",        YELLOW, "Fulltext Index Search", "Specialized FULL TEXT search. Low - for this specialized search requirement."),
                      ("ref_or_null",     GREEN, "Key Lookup +\nFetch NULL Values", """Low-medium - if number of matching rows is small, higher as the number of rows increases."""),
                      ("index_merge",     GREEN, "Index Merge", "Medium - may want to look for better index selection in the query to improve performance."),
                      ("unique_subquery", ORANGE, "Unique Key Lookup\ninto table of subquery", "Low - Used for efficient Subquery processing"),
                      ("index_subquery",  ORANGE, "Non-Unique Key Lookup\ninto table of subquery", "Low - Used for efficient Subquery processing"),
                      ("range",           ORANGE, "Index Range Scan", "Medium - partial index scan"),
                      ("index",           RED, "Full Index Scan", "High - especially for large indexes"),
                      ("ALL",             RED, "Full Table Scan", """Very High - very costly for large tables (not so much for small ones).
No usable indexes were found for the table and the optimizer must search every row.
Consider adding an index."""),
                      ("UNKNOWN",         BLACK, "unknown", ""), # the default, in case none matches
                      ]
    
    def __init__(self, context, name, access_type, attached_subqueries=None,\
                 key_name = [], info = None, cost_info = None,\
                 rows_examined = None, rows_produced = None):
        ExplainNode.__init__(self, context)
        self.name = name
        self.key_name = key_name
        self.access_type = access_type
        self.info = info
        self.cost_info = cost_info
        self.rows_examined = rows_examined
        self.rows_produced = rows_produced
        
        self.child_attached_subqueries = attached_subqueries
        if attached_subqueries:
            attached_subqueries.parent = self

        for key, color, label, hint in self.col_join_types:
            if access_type == key:
                break

        self.info["_hint"] = hint
        self.info["_access_type"] = label
        
        self.set_spacing(4)
    
        self._figure = RectangleShapeFigure(label)
        self._figure.set_layout_flags(0)
        self._figure.set_font_size(11)
        self._figure.set_font_bold(True)
        self._figure.set_fill_color(*color)
        self._figure.set_text_color(1, 1, 1, 1)
        self._figure.set_padding(10, 10, 10, 10)
        self.add(self._figure)
        
        self._figure_name = RectangleShapeFigure(self.name)
        self._figure_name.set_layout_flags(0)
        self._figure_name.set_color(1, 1, 1, 0)
        self._figure_name.set_font_size(11)
        self._figure_name.set_text_color(0, 0, 0, 1)
        self._figure_name.set_alignment(0.5, 0)
        self.add(self._figure_name)
        
        if self.key_name:
            self._figure_key = TextFigure(self.key_name)
            self._figure_key.set_layout_flags(0)
            self._figure_key.set_font_size(10)
            self._figure_key.set_font_bold(True)
            self._figure_key.set_text_color(0, 0, 0, 1)
            self._figure_key.set_alignment(0.5, 0)
            self.add(self._figure_key)
        else:
            self._figure_key = None


    @property
    def children(self):
        if self.child_attached_subqueries:
            return [self.child_attached_subqueries]
        return []

    
    @property
    def rows_count(self):
        return self.rows_examined

    @property
    def vconnect_pos_offset(self):
        if self.child_attached_subqueries:
            return self._figure.x + self._figure.width/2
        return self._figure.x + self.inner_width/2

    @property
    def inner_height(self):
        h = self._figure.height
        if self._figure_name:
            h += 4 + self._figure_name.height
        if self._figure_key:
            h += 4 + self._figure_key.height
        return h


    @property
    def hint_pos_x(self):
        return self._figure.root_x + self._figure.width

    def _hint_line(self, label, key, always_show=False, value_format=None):
        if self.info.has_key(key) or always_show:
            if value_format and self.info.has_key(key):
                value = value_format % self.info[key]
            else:
                value = self.info.get(key, "-")
            if type(value) is list:
                value = ",\n    ".join(value)
            return "%s  %s\n" % (label, value)
        return ""


    def get_hint_text(self):
        text = """*%(table_name)s
  Access Type: %(access_type)s
      %(_access_type)s
      Cost Hint: %(_hint)s
""" % self.info

        text += self._hint_line("  Used Columns:", "used_columns") # 5.7
        text += "\n"
        text += self._hint_line("*Key/Index:", "key", True)
        text += self._hint_line("  Ref.:", "ref")
        text += self._hint_line("  Used Key Parts:", "used_key_parts")
        text += self._hint_line("  Possible Keys:", "possible_keys")
        if "used_columns" in self.info or "used_key_parts" in self.info or "possible_keys" in self.info:
            text += "\n"

        text += self._hint_line("*Attached Condition:\n", "attached_condition")
        if "attached_condition" in self.info:
            text += "\n"

        text += self._hint_line("Using Join Buffer:", "using_join_buffer")

        text += self._hint_line("Rows Examined per Scan:", "rows_examined_per_scan") # 5.7
        text += self._hint_line("Rows Produced per Join:", "rows_produced_per_join") # 5.7
        text += self._hint_line("Filtered (ratio of rows produced per rows examined):", "filtered", value_format="%s%%")
        text += "    Hint: 100% is best, <= 1% is worst\n"
        text += "    A low value means the query examines a lot of rows that are not returned.\n"
        if self.cost_info: # 5.7
            text += """*Cost Info
  Read: %(read_cost)s
  Eval: %(eval_cost)s
  Prefix: %(prefix_cost)s
  Data Read: %(data_read_per_join)s
""" % self.cost_info
        
        return text
    

    def do_render_extras(self, cr):
        if self.parent and not isinstance(self.parent, MaterializedTableNode):
            cr.save()
            cr.set_source_rgba(0, 0, 0, 1)
            cr.set_line_width(self.get_line_width())
            if isinstance(self.parent, NestedLoopNode) and self.parent.child_aside == self:
                # special case
                cr.move_to(*self.varrow_source)
                cr.line_to(self.varrow_source[0], self.parent.harrow_target[1])
                cr.line_to(self.parent.harrow_target[0], self.parent.harrow_target[1])
                cr.stroke()
                draw_harrow(cr, self.parent.harrow_target, 10, 6)
            else:
                cr.move_to(*self.varrow_source)
                cr.line_to(self.varrow_source[0], self.parent.varrow_target[1])
                cr.stroke()
                draw_varrow(cr, (self.varrow_source[0], self.parent.varrow_target[1]), 10, 6)
            cr.fill()
            self.render_cost(cr, self._figure.root_x, self.varrow_source[1] - 5)
            self.render_row_count(cr, self.varrow_source[0] + 4, self.varrow_source[1] - 5)
            cr.restore()


    def do_relayout(self, ctx):
        self._figure.set_usize(None, None)
        self._figure.do_relayout(ctx)
        self._figure.set_usize(max(self._figure.width, 90), self._figure._uheight)
        if self._figure_name:
            self._figure_name.set_usize(max(self._figure.width, 90), self._figure_name._uheight)
        if self._figure_key:
            self._figure_key.set_usize(max(self._figure.width, 90), self._figure_key._uheight)

        self._height = self.inner_height
        
        VBoxFigure.do_relayout(self, ctx)

        # for subselects in the WHERE list (attached subqs), attach them from the right side
        # select * from ... where (select ...) = ...
        child = self.child_attached_subqueries
        if child:
            child.do_relayout(ctx)

            self._figure.move(0, self._figure.y)
            if self._figure_name:
                self._figure_name.move(0, self._figure_name.y)
            if self._figure_key:
                self._figure_key.move(0, self._figure_key.y)
            child.move(self._figure.x + self._figure.width + self._context.hspacing, self._figure.y)

            self._width = self._figure.width + self._context.hspacing + child.width
            self._height = max(self._height, child.height)

    
    def __repr__(self):
        return "<table: "+self.name+" ("+self.access_type+")>"



class MaterializedTableNode(TableNode):
    def __init__(self, context, name, access_type, attached_subqueries=None,\
                 materialized_from = None, materialize_attributes = None,\
                 key_name = [], info = None, cost_info = None,\
                 rows_examined = None, rows_produced = None):
        TableNode.__init__(self, context, name, access_type = access_type,
                           attached_subqueries = attached_subqueries,
                           key_name = key_name, info = info,
                           cost_info = cost_info, rows_examined = rows_examined, rows_produced = rows_produced)

        # we don't expect attached_subqueries to ever have a value for this type of node,
        # if that's confirmed we can remove the arg
        assert not attached_subqueries

        self._figure.set_layout_flags(HFill)
        self.set_spacing(0)
        
        fill = self._figure._fill_color
        self._figure.set_color(fill[0], fill[1], fill[2], 0.8)
        self._figure.set_fill_color(fill[0], fill[1], fill[2], 0.8)

        self._figure_name.set_layout_flags(HFill)
        self._figure_name.set_color(0.9, 0.9, 0.9, 0.9)
        self._figure_name.set_text("%s (materialized)" % name)
        self._figure_name.set_fill_color(0.9, 0.9, 0.9, 0.9)
        self._figure_name.set_padding(4, 4, 4, 4)

        self.materialize_attributes = materialize_attributes

        self.child_materialized_from = materialized_from
        materialized_from.parent = self


    def __repr__(self):
        return "<materialized table: "+self.name+" ("+self.access_type+")>"


    @property
    def children(self):
        return TableNode.children.fget(self) + [self.child_materialized_from]


    def do_relayout(self, ctx):
        self.child_materialized_from.do_relayout(ctx)
        TableNode.do_relayout(self, ctx)

        if self._width < self.child_materialized_from.width + self._context.frame_padding * 2:
            self._width = int(self.child_materialized_from.width + self._context.frame_padding*2)
            VBoxFigure.do_relayout(self, ctx)
        
        self._height = int(self.inner_height + self.child_materialized_from.height + self._context.frame_padding*2)

        if self.width <= self.child_materialized_from.width:
            self.child_materialized_from.move(self._context.frame_padding,
                                              self.inner_height + self._context.frame_padding)
        else:
            self.child_materialized_from.move((self.width - self.child_materialized_from.width)/2,
                                              self.inner_height + self._context.frame_padding)


    def do_render(self, ctx):
        TableNode.do_render(self, ctx)
        ctx.save()
        ctx.translate(self.x, self.y)
        ctx.rectangle(0.5, 0.5, int(self._width), int(self._height)-1)
        ctx.set_line_width(1)
        ctx.set_dash([4.0, 2.0], 0)
        ctx.set_source_rgba(0.5, 0.5, 0.5, 0.9)
        ctx.stroke()
        ctx.restore()


    @property
    def inner_width(self):
        return self.width

    def get_hint_text(self):
        d = {"dependent":False,
        "cacheable":False,
        "using_temporary_table":False
        }
        d.update(self.materialize_attributes)
        text = """*Materialized from Subquery
Using Temporary Table: %(using_temporary_table)s
Dependent: %(dependent)s
Cacheable: %(cacheable)s
""" % d
        text = text + "\n" + TableNode.get_hint_text(self)
        return text




class OperationNode(ExplainNode):
    is_operation = True

    def __init__(self, context, operation, child, cost_info = None, attributes = None, optimized_away_subnode=None):
        ExplainNode.__init__(self, context)
        self.operation = operation
        if child:
            child.parent = self
        self.child = child
        self.cost_info = cost_info
        self.attributes = attributes
    
        self.child_optimized_away = optimized_away_subnode

        if operation == "grouping_operation":
            operation_caption = "GROUP"
            score = 0
            if self.attributes.get("using_temporary_table", False):
                score += 1
            if self.attributes.get("using_filesort", False):
                score += 1
        elif operation == "duplicates_removal":
            operation_caption = "DISTINCT"
            score = 0
            if self.attributes.get("using_filesort", False):
                score += 2
        elif operation == "ordering_operation":
            operation_caption = "ORDER"
            score = 0
            if self.attributes.get("using_filesort", False):
                score += 2
        else:
            log_error("Unknown operation: %s\n" % operation)
            operation_caption = operation
            score = 0
        fill_color = YELLOW
        if score == 1:
            fill_color = ORANGE
        elif score == 2:
            fill_color = RED

        self.set_spacing(4)
        
        self._figure = RectangleShapeFigure(operation_caption)
        self._figure.set_layout_flags(0)
        self._figure.set_corner_radius(20)
        self._figure.set_color(0.0, 0.0, 0.0, 1)
        self._figure.set_color(*fill_color)
        self._figure.set_line_width(2)
        self._figure.set_padding(15, 20, 15, 20)
        self.add(self._figure)

        attrs = []
        if self.attributes.get("using_temporary_table"):
            attrs.append("tmp table")
        if self.attributes.get("using_filesort"):
            attrs.append("filesort")
        if attrs:
            self._figure_message = TextFigure(",".join(attrs))
            self._figure_message.set_font_bold(True)
            self._figure_message.set_font_size(10)
            self.add(self._figure_message)
        else:
            self._figure_message = None


    @property
    def varrow_target(self):
        if self._figure_message:
            return int(self.root_x + self.vconnect_pos_offset)+0.5, self._figure.root_y + self._figure_message.height + self.inner_height + 5
        else:
            return int(self.root_x + self.vconnect_pos_offset)+0.5, self._figure.root_y + self.inner_height + 1

    @property
    def children(self):
        return [self.child] if self.child else []


    def do_render_extras(self, cr):
        if self.parent and not isinstance(self.parent, MaterializedTableNode):
            cr.save()
            cr.set_source_rgba(0, 0, 0, 1)
            cr.set_line_width(self.get_line_width())
            if isinstance(self.parent, NestedLoopNode) and self.parent.child_aside == self:
                # special case
                cr.move_to(*self.varrow_source)
                cr.line_to(self.varrow_source[0], self.parent.harrow_target[1])
                cr.line_to(self.parent.harrow_target[0], self.parent.harrow_target[1])
                cr.stroke()
                draw_harrow(cr, self.parent.harrow_target, 10, 6)
            elif isinstance(self.parent, OperationNode):
                cr.move_to(*self.harrow_source)
                cr.line_to(self.parent.harrow_target[0], self.harrow_source[1])
                cr.stroke()
                draw_harrow(cr, self.parent.harrow_target, 10, 6)
            else:
                cr.move_to(*self.varrow_source)
                cr.line_to(self.varrow_source[0], self.parent.varrow_target[1])
                cr.stroke()
                draw_varrow(cr, (self.varrow_source[0], self.parent.varrow_target[1]), 10, 6)
            cr.fill()
            cr.restore()


    def do_relayout(self, ctx):
        VBoxFigure.do_relayout(self, ctx)
        
        child = self.child
        child.do_relayout(ctx)

        if isinstance(child, NestedLoopNode) or isinstance(child, TableNode):
            self._width = max(child.width, self.width)
            self._height = child.height + self._context.vspacing + self.inner_height
            if child.width > self._figure.width:
                self._figure.move(child.vconnect_pos_offset - self._figure.width/2,
                                  0)
                child.move(0, self._figure.height + self._context.vspacing)
            else:
                self._figure.move(0, 0)
                child.move((self._figure.width - child.width) / 2,
                           self._figure.height + self._context.vspacing)
        else:
            self._width = child.width + self._context.hspacing + self.inner_width
            self._height = max(child.height, self.inner_height)

            self._figure.move(child.width + self._context.hspacing,
                              0)
            child.move(0, 0)
        if self._figure_message:
            self._figure_message.move(self._figure.x, self._figure.y + self._figure.height + 4)


    def get_hint_text(self):
        if self.operation == "grouping_operation":
            text = "*Grouping Operation\n"
        elif self.operation == "ordering_operation":
            text = "*Ordering Operation\n"
        elif self.operation == "duplicates_removal":
            text = "*Duplicates Removal\n"
        else:
            text = self.operation+"\n"
        text += "\n"
        if "using_temporary_table" in self.attributes:
            text += "Using Temporary Table:  %s\n" % self.attributes["using_temporary_table"]
        if "using_filesort" in self.attributes:
            text += "Using Filesort:  %s\n" % self.attributes["using_filesort"]
        if self.cost_info and "sort_cost" in self.cost_info:
            text += "Sort Cost: %s\n" % self.cost_info["sort_cost"]
        return text


    @property
    def rows_count(self):
        if self.children:
            return self.children[0].rows_count
        else:
            print "Node", self, "has no children"
            return 0


    def __repr__(self):
        return self.operation


class QueryBlockNode(ExplainNode):
    def __init__(self, context, nested_loop, optimized_away_subnode=None, select_list_subqueries=None, info=None, cost_info=None):
        ExplainNode.__init__(self, context)
        if nested_loop:
            nested_loop.parent = self
        self.info = info
        self.cost_info = cost_info
        self.child = nested_loop
        self.child_optimized_away = optimized_away_subnode
        if optimized_away_subnode:
            optimized_away_subnode.parent = self

        self.select_list_subqueries = select_list_subqueries
        if select_list_subqueries:
            select_list_subqueries.parent = self

        self.set_spacing(4)

        if info and "select_id" in info:
            self._figure = RectangleShapeFigure("query_block #%s" % info["select_id"])
        else:
            self._figure = RectangleShapeFigure("query_block")
        self._figure.set_layout_flags(0)
        self._figure.set_padding(10, 10, 10, 10)
        self._figure.set_line_width(1)
        self.add(self._figure)

        if info and "message" in info:
            self._figure_message = TextFigure(info["message"])
            self._figure_message.set_font_size(11)
            self._figure_message.set_text_color(0, 0, 0, 1)
            self._figure_message.set_alignment(0.5, 0)
            self.add(self._figure_message)
        else:
            self._figure_message = None


    @property
    def inner_height(self):
        h = self._figure.height
        if self._figure_message:
            h += self._figure_message.height
        return h


    @property
    def children(self):
        result = []
        if self.child:
            result.append(self.child)
        if self.select_list_subqueries:
            result.append(self.select_list_subqueries)
        return result


    def do_relayout(self, ctx):
        VBoxFigure.do_relayout(self, ctx)

        # calculate layout of the child item
        child = self.child
        if child:
            child.do_relayout(ctx)
            
             # extra space for cost info at top
            voffset = 20
            
            # get the offset of the point in the child item that should be
            # aligned with the parent item
            child_align_x = child.vconnect_pos_offset
            child_width = child.width
            
            # increase the size of this item, to allow fitting the child item
            self._width = child_width
            self._height = self.inner_height + self._context.vspacing + child.height + voffset
            
            # position the child
            child.move(0, self.inner_height + self._context.vspacing)
            
            # position the inner figure of the node so it aligns with the center of the child
            self._figure.move(child_align_x - self.inner_width/2, voffset)

        # for subselects in the SELECT list, attach them from the right side
        # select (select ...) from ...
        child = self.select_list_subqueries
        if child:
            child.do_relayout(ctx)

            child.move(self.width, self._figure.y)

            self._width += child.width
            self._height = max(self._height, child.height)


    def get_hint_text(self):
        text = "Select ID: %s\n" % self.info["select_id"]
        if self.cost_info and "query_cost" in self.cost_info:
            text += "Query Cost: %s\n" % self.cost_info["query_cost"]
        return text


    def render_cost(self, cr, x, y):
        if self.cost_info:
            cost = self.cost_info.get("query_cost")
            if cost is not None:
                cr.set_source_rgba(0, 0, 0, 1)
                cr.move_to(x, y)
                cr.set_font_size(10)
                cr.show_text("Query cost: %s" % cost)


    def do_render_extras(self, cr):
        cr.save()
        cr.set_source_rgba(0, 0, 0, 1)
        if self.parent is None:
            cr.set_line_width(1)
            cr.rectangle(self._figure.root_x+2.5, self._figure.root_y+2.5, self._figure.width-4, self._figure.height-4)
            cr.stroke()

        self.render_cost(cr, self._figure.root_x, self._figure.root_y - 5)

        if self.parent and not isinstance(self.parent, MaterializedTableNode):
            cr.set_line_width(self.get_line_width())
            cr.move_to(*self.varrow_source)
            cr.line_to(self.varrow_source[0], self.parent.varrow_target[1])
            cr.stroke()
            draw_varrow(cr, (self.varrow_source[0], self.parent.varrow_target[1]), 10, 6)
            cr.fill()
        cr.restore()


    def __repr__(self):
        return "query_block"


class SubQueryBlockNode(QueryBlockNode):
    def __init__(self, context, nested_loop, optimized_away_subnode=None, select_list_subqueries=None, info=None, cost_info=None):
        QueryBlockNode.__init__(self, context, nested_loop, optimized_away_subnode, select_list_subqueries, info, cost_info)

        self.attributes = None
        if info and "select_id" in info:
            self._figure.set_text("subquery #%s" % info["select_id"])
        else:
            self._figure.set_text("subquery")


    def set_attributes(self, attributes):
        self.attributes = attributes
    
    
#    @property
#    def varrow_target(self):
#        return None, self.root_y + self.inner_height


    def get_hint_text(self):
        text = "Subquery\n"
        if "select_id" in self.info:
            text += "Select ID: %s\n" % self.info["select_id"]
        if self.cost_info and "query_cost" in self.cost_info:
            text += "Query Cost: %s\n" % self.cost_info["query_cost"]
        text += "\n"
        if "using_temporary_table" in self.attributes:
            text += "Using Temporary Table:  %s\n" % self.attributes["using_temporary_table"]
        if "dependent" in self.attributes:
            text += "Dependent:  %s\n" % self.attributes["dependent"]
        if "cacheable" in self.attributes:
            text += "Cacheable:  %s\n" % self.attributes["cacheable"]
        return text


    def __repr__(self):
        return "subquery"


class SubQueries(ExplainNode):
    def __init__(self, context, what, nodes):
        ExplainNode.__init__(self, context)
        self.what = what
        for n in nodes:
            n.parent = self
        self.children = nodes
        
        self._figure = RectangleShapeFigure(what)
        self._figure.set_line_dash([2.0, 2.0], 0)
        self._figure.set_padding(8, 10, 8, 10)
        self._figure.set_line_width(1)
        self.add(self._figure)

    def __repr__(self):
        return self.what


    def do_relayout(self, ctx):
        VBoxFigure.do_relayout(self, ctx)

        max_height = 0
        total_width = 0

        for child in reversed(self.children):
            child.do_relayout(ctx)
            child.move(total_width, self.inner_height + self._context.small_vspacing)
            total_width += child.width + self._context.small_hspacing
            max_height = max(max_height, child.height)
        
        if self.children:
            total_width -= self._context.small_hspacing
        
        self._width = total_width
        self._height = self.inner_height + self._context.small_vspacing + max_height


    def do_render_extras(self, cr):
        cr.save()
        cr.set_source_rgba(0, 0, 0, 1)
        self.render_cost(cr, self._figure.root_x, self._figure.root_y - 5)

        cr.set_line_width(self.get_line_width())
        if self.what in ("select_list_subqueries", "attached_subqueries"):
            # parent is a QueryBlockNode or TableNode
            cr.move_to(self.root_x, self.harrow_source[1])
            target_x = self.parent._figure.root_x + self.parent._figure.width
            cr.line_to(target_x, self.harrow_source[1])
            cr.stroke()
            draw_harrow(cr, (target_x, self.harrow_source[1]), -10, 6)
            cr.fill()
        elif self.parent and not isinstance(self.parent, MaterializedTableNode):
            cr.move_to(*self.varrow_source)
            cr.line_to(self.varrow_source[0], self.parent.varrow_target[1])
            cr.stroke()
            draw_varrow(cr, (self.varrow_source[0], self.parent.varrow_target[1]), 10, 6)
            cr.fill()
        cr.restore()


    @property
    def inner_height(self):
        return self._figure.height


# union_result nodes are the result of merging their parent query_block node and the union_result
class UnionResult(SubQueries):
    def __init__(self, context, info, queries):
        SubQueries.__init__(self, context, "UNION", queries)

        self.set_spacing(4)
        self._figure.set_line_dash(None, None)
        self._figure.set_color(0, 0, 0, 1)
        self._figure.set_fill_color(0.7, 0.7, 0.7, 1)
    
        if "table_name" in info:
            self._figure_name = TextFigure(info["table_name"])
            self._figure_name.set_font_size(11)
            self._figure_name.set_text_color(0, 0, 0, 1)
            self._figure_name.set_alignment(0.5, 0)
            self.add(self._figure_name)
        else:
            self._figure_name = None

        self.info = info

    @property
    def inner_height(self):
        h = SubQueries.inner_height.fget(self)
        if self._figure_name:
            h += 4 + self._figure_name.height
        return h

    def set_attributes(self, attributes):
        # this is called by handle_materialized_from_subquery, but the attributes don't belong
        # to the UNION node, they belong to the materialized table node
        pass
        print attributes, self, "!!!!!!"


    def get_hint_text(self):
        text = "UNION Result\n"
        text = """*%(table_name)s
Access Type: %(access_type)s
            
Using Temporary Table: %(using_temporary_table)s""" % self.info

        return text

#
# JSON Explain Data Parser
#
# Takes a JSON object (bunch of dicts and lists) and turns into internal node objects
#
class ExplainContext:
    default_height = 65
    vspacing = 50
    hspacing = 50
    small_vspacing = 30
    small_hspacing = 30
    frame_padding = 10
    
    global_padding = 20

    def __init__(self, json):
        self._json = json
        nodes = self.process_explain_output(json)
        if not nodes:
            log_error("Could not process JSON data\n")
            self._root = None
        else:
            self._root = nodes[0]
        
            if False:
                import StringIO
                s = StringIO.StringIO()
                self._root.dump(s)
                print s.getvalue()
        
        self._offset = (0, 0)
        self._canvas = None
        self._view = None
        self.size = None
        self.aggregate_costs = False
        self.displayed_cost_info = None
        self.cost_value_is_amount = False


    def unexpected(self, node, context=""):
        if context:
            log_error("While parsing JSON output: unexpected node in %s: %s\n" % (context, node))
        else:
            log_error("While parsing JSON output: unexpected node: %s\n" % node)


    def handle_table(self, table):
        name = table["table_name"]
        materialized_from_subquery = table.get("materialized_from_subquery")
        materialized_from_subquery_node = None
        materialized_attributes = {}
        if materialized_from_subquery:
            materialized_from_subquery_node, materialized_attributes = self.handle_materialized_from_subquery("materialized_from_subquery", materialized_from_subquery)
        attached_subqueries_ = table.get("attached_subqueries")
        if attached_subqueries_:
            attached_subqueries = self.handle_attached_subqueries("attached_subqueries", attached_subqueries_)
        else:
            attached_subqueries = None

        if 'rows_examined_per_scan' not in table:
            table['rows_examined_per_scan'] = table.get('rows')
        if 'rows_produced_per_join' not in table:
            table['rows_produced_per_join'] = table.get('rows') * int(table.get('filtered', 0)) / 100

        # table nodes that have a materialized_from_subquery node are not real tables
        # so show them as a container for the subquery that it executes
        if materialized_from_subquery_node:
            return MaterializedTableNode(self, name,
                             materialized_from=materialized_from_subquery_node,
                             materialize_attributes=materialized_attributes,
                             attached_subqueries=attached_subqueries, # ??
                             access_type=table['access_type'],
                             key_name=table.get('key', None),
                             info=table,
                             cost_info=table.get('cost_info'),
                             rows_examined=table.get('rows_examined_per_scan'),
                             rows_produced=table.get('rows_produced_per_join')
                             )
        else:
            return TableNode(self, name,
                             attached_subqueries=attached_subqueries,
                             access_type=table['access_type'],
                             key_name=table.get('key', None),
                             info=table,
                             cost_info=table.get('cost_info'),
                             rows_examined=table.get('rows_examined_per_scan'),
                             rows_produced=table.get('rows_produced_per_join')
                             )


    def handle_nested_loop(self, data):
        parts = []
        for node in data:
            if type(node) is dict and len(node) == 1 and 'table' in node:
                table = self.handle_table(node['table'])
                parts.append(table)
                if len(parts) == 2:
                    if 'using_join_buffer' in node['table']:
                        join_buffer = node['table']['using_join_buffer']
                    else:
                        join_buffer = "nested_loop"
                    loop_node = NestedLoopNode(self, join_buffer, parts[0], parts[1])
            
                    parts = [loop_node]
                elif len(parts) > 2:
                    raise Exception("uh oh")
            elif type(node) is dict and len(node) == 1 and 'duplicates_removal' in node:
                operation = self.handle_query_block('duplicates_removal', node['duplicates_removal'])
                parts.append(operation)
                if len(parts) == 2:
                    join_buffer = "nested_loop"
                    loop_node = NestedLoopNode(self, join_buffer, parts[0], parts[1])
                    parts = [loop_node]
                elif len(parts) > 2:
                    raise Exception("uh oh")
            else:
                self.unexpected(node, "nested_loop")

        # at the end, there should be a single node which would be the root node
        assert len(parts) == 1
        return parts[0]


    def handle_optimized_away_subqueries(self, name, subquery_list):
        subqueries = []
        for data in subquery_list:
            qblock = None
            attributes = {}
            for key, value in data.items():
                if key == "query_block":
                    qblock = self.handle_query_block(key, value, True)
                elif key in ("dependent", "cacheable", "using_temporary_table"):
                    attributes[key] = value
                else:
                    self.unexpected(key, name)
            qblock.set_attributes(attributes)
            subqueries.append(qblock)
        return SubQueries(self, name, subqueries)


    def handle_attached_subqueries(self, name, subquery_list):
        subqueries = []
        for data in subquery_list:
            qblock = None
            attributes = {}
            for key, value in data.items():
                if key == "query_block":
                    qblock = self.handle_query_block(key, value, True)
                elif key in ("dependent", "cacheable", "using_temporary_table"):
                    attributes[key] = value
                else:
                    self.unexpected(key, name)
            qblock.set_attributes(attributes)
            subqueries.append(qblock)
        return SubQueries(self, name, subqueries)


    def handle_materialized_from_subquery(self, name, data):
        inner_qblock = None
        attributes = {}
        for key, value in data.items():
            if key == "query_block":
                inner_qblock = self.handle_query_block(key, value, is_subquery=True, is_materialized=True)
            elif key in ("dependent", "cacheable", "using_temporary_table"):
                attributes[key] = value
            else:
                self.unexpected(key, name)
        return inner_qblock, attributes


    def handle_union_result(self, name, data):
        info = {}
        qblocks = []
        for key, value in data.items():
            if key in ("using_temporary_table", "access_type", "table_name"):
                info[key] = value
            elif key == "query_specifications":
                for qspec in value:
                    qspec.get("dependent")
                    qspec.get("cacheable")
                    qblocks.append(self.handle_query_block("query_block", qspec.get("query_block")))
            else:
                self.unexpected(key, name)
        return UnionResult(self, info=info, queries=qblocks)


    def handle_query_block(self, name, data, is_subquery=False, is_materialized=False):
        # used for query_block or operation
        content = None
        cost_info = None
        attributes = {}
        select_list_subqueries = None
        optimized_away_subqueries = None
        for key, value in data.items():
            if key == "nested_loop":
                assert content is None
                content = self.handle_nested_loop(value)
            elif key == "table":
                # in 5.6, it was possible that a query_block would have a table node with just message:"No tables used"
                # that was changed in 5.7, so the useless table node would be removed
                # so we do some special handling here, so that 5.6 output looks like 5.7
                if "message" in value and len(value) == 1:
                    data["message"] = value["message"]
                else:
                    content = self.handle_table(value)
            elif key == "optimized_away_subqueries":
                optimized_away_subqueries = self.handle_optimized_away_subqueries(key, value)
            elif key in ("grouping_operation", "ordering_operation", "duplicates_removal"):
                assert content is None
                content = self.handle_query_block(key, value)
            elif key in ("using_temporary_table", "using_filesort", "dependent"):
                attributes[key] = value
            elif key == "cost_info":
                cost_info = value
            elif key == "select_id":
                pass
            elif key == "message":
                pass
            elif key == "union_result":
                assert content is None
                # the parent query_block node seems to be redundant in this case, since it has only 1 child node
                content = self.handle_union_result(key, value)
            elif key == "select_list_subqueries":
                # select (select ...) from ...
                select_list_subqueries = self.handle_attached_subqueries(key, value)
            else:
                self.unexpected(key, name)
        if name == "query_block":
            if is_materialized:
                return content

            if is_subquery:
                return SubQueryBlockNode(self,
                                      content,
                                      optimized_away_subqueries,
                                      select_list_subqueries,
                                      info=data,
                                      cost_info=cost_info)
            else:
                return QueryBlockNode(self,
                                      content,
                                      optimized_away_subqueries,
                                      select_list_subqueries,
                                      info=data,
                                      cost_info=cost_info)
        else:
            if optimized_away_subqueries:
                return OperationNode(self, name, content, cost_info, attributes, optimized_away_subqueries)
            else:
                return OperationNode(self, name, content, cost_info, attributes)


    def handle(self, data):
        output = []
        for key, value in data.items():
            if key == "query_block":
                output.append(self.handle_query_block(key, value))
        return output

    
    def process_explain_output(self, data):
        return self.handle(data)


    # Public interface
    def dump(self):
        if self._root:
            import StringIO
            s = StringIO.StringIO()
            self._root.dump(s, 0)
            print s.getvalue()


    def _set_tooltip(self, v):
        self._canvas.tooltip = v

    def _get_tooltip(self):
        return self._canvas.tooltip

    tooltip = property(_get_tooltip, _set_tooltip)


    def init_canvas(self, view, queue_repaint_cb):
        self._canvas = MyCanvas(queue_repaint_cb)
        self._canvas.add(self._root)

        self._view = view

        self._root._figure.set_fill_color(0.8, 0.8, 0.8, 1)


    def set_offset(self, x, y):
        self._offset = (x, y)


    def client_to_screen(self, x, y):
        x += self._offset[0]
        y += self._offset[1]
        return self._view.client_to_screen(int(x), int(y))


    def export_to_png(self, path):
        img = ImageSurface(width=self.size[0], height=self.size[1])
        cr = Context(img)
        self._canvas.repaint(cr, 0, 0, self.size[0], self.size[1])
        img.write_to_png(path)


    def layout(self, padding = 20):
        c = Context(ImageSurface(width=1, height=1))

        self._root.do_relayout(c)
        self._root.move(self.global_padding, self.global_padding)

        w, h = self._root.size
        self.size = w + self.global_padding * 2, h + self.global_padding * 2
        
        return self.size
    
    
    def repaint(self, cr):
        cr.translate(self._offset[0], self._offset[1])
        try:
            self._canvas.repaint(cr, 0, 0, self.size[0], self.size[1])
        except Exception:
            import traceback
            log_error("Exception repainting explain: %s\n" % traceback.format_exc())


    def fmt_cost(self, value):
        if self.cost_value_is_amount:
            return fmt_number(value)
        else:
            return str(value)


    def show_cost_info_type(self, name):
        self.cost_value_is_amount = "data_read_per_join" == name
        self.displayed_cost_info = name


    def show_aggregated_cost_info(self, flag):
        self.aggregate_costs = flag



