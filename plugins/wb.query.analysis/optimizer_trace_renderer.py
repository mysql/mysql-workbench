# Copyright (c) 2012, 2013, Oracle and/or its affiliates. All rights reserved.
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

import cairo

import cairo_utils
from cairo_utils import Context, ImageSurface, Pattern
from cairo_utils import VBoxNode, TextRectangle, intersect_lines, draw_arrow_head



class TreeNode(VBoxNode):
    def __init__(self, parent, name, title, data):
        VBoxNode.__init__(self)
        self.padding = (0, 0, 0, 0)
        self.expand_to_fill = False
        self.name = name
        self.parent = parent
        self.data = data
        self.children = []
        self.is_step_list = False
        if title is not None:
            self.content = TextRectangle(title)
            self.content.set_fill_color(0.83137254901960789, 0.92941176470588238, 0.99215686274509807, 1)
            self.items.append(self.content)
        self.extra_bottom_space = 0

    def __repr__(self):
        return "<node %s>"%self.content.text

    def process(self):
        for ch in self.children:
            ch.process()
        self.calc()

    def render(self, c):
        VBoxNode.render(self, c)
        self.render_shadow(c)

    def minimal_space_required_space_for_ref_to(self, c, ch):
        return 0

    def layout(self, c, x, y):
        self.calc(c)
        self.pos = (x, y)

class TreeStepNode(TreeNode):
    def __init__(self, parent, name, title, data):
        TreeNode.__init__(self, parent, name, title, data)
        self.set_fill_color(0.5, 0.7, 0.5, 1)
        self.set_color(1, 1, 1, 1)

class StepListNode(TreeNode):
    def __init__(self, parent, steps):
        TreeNode.__init__(self, parent, "steps", None, steps)

    def layout(self, c, x, y):
        self.calc(c)
        for ch in self.children:
            ch.layout(c, x, y)
            y += ch.size[1] + 40

    def render(self, c):
        for ch in self.children:
            ch.render(c)

    def process(self):
        for step in self.data:
            name, value = step.items()[0]
            print "#"*20,name, value
            node = TreeStepNode(self, name, name, value)
            self.children.append(node)


class TreeRootNode(StepListNode):
    def __init__(self, steps):
        StepListNode.__init__(self, None, steps["steps"]) 



class TableDependency(TreeNode):
    def __init__(self, parent, data):
        title = "%s.%s" % (data["database"], data["table"])
        TreeNode.__init__(self, parent, "TableDependency", title, data)

# Layouter

class TreeLayouter:
    def __init__(self, root):
        self.root = root
        self.yspacing = 30
        self.xspacing = 50

    def layout(self, ctx, node, x = 0, y = 0):
        t, l, b, r = node.padding

        w, h = node.size
        twidth = 0
        theight = 0
        # calc total height occupied by children
        if node.is_step_list:
            for step in node.children:
                step.pos = x, y
                if node.name == "join_optimization":
                    print "put ", step.name, x + self.xspacing
                if step.is_step_list:
                    sw, sh = self.layout(ctx, step, x, y)
                else:
                    sw, sh = self.layout_tree(ctx, step, x + step.size[0] + self.xspacing, y)
                step.gsize = (sw, sh)
                y += max(step.size[1], sh) + self.yspacing
                theight += sh + self.yspacing
                twidth = max(sw, twidth)
            twidth = max(w, twidth)
        else:
            twidth, theight = self.layout_tree(ctx, node, x, y)
    
        theight += h
        return twidth, theight

    def layout_tree(self, ctx, node, x, y):
        w, h = node.size
        twidth = 0
        theight = h
        xx = x
        for ch in node.children:
            sw, sh = self.layout_tree(ctx, ch, xx, y + h + self.yspacing)
            xx += sw + self.xspacing
            twidth += sw
            theight = max(sh, theight)
        node.pos = x, y
        return max(twidth, w), theight + self.yspacing


    def adjust_child_layout(self, node, offset):
        for ch in node.children:
            cx, cy = ch.pos
            ch.pos = int(cx + offset), int(cy)
            self.adjust_child_layout(ch, offset)                                       

    def get_total_size(self, ctx):
        def calc_all(c, node):
            node.calc(c)
            for ch in node.children:
                calc_all(c, ch)
        calc_all(ctx, self.root)
 
        return self.layout(ctx, self.root, 0, 0)


    def render(self, c, x, y):
        def calc_all(c, node):
            node.calc(c)
            for ch in node.children:
                calc_all(c, ch)
        calc_all(c, self.root)
        self.layout(c, self.root, x, y)
        self.do_render(c, self.root)
        self.do_render_lines(c, self.root)

    def do_render_lines(self, c, node):
        if node.is_step_list:
            if node.children:
                prev = node.children[0]
                node.stroke_line_from_node(c, prev)
                for ch in node.children[1:]:
                    prev.stroke_line_from_node(c, ch)
                    prev = ch
        else:
            for ch in node.children:
                 ch.stroke_line_from_node(c, node)

        for ch in node.children:
            self.do_render_lines(c, ch)

    def do_render(self, c, node):
        node.render(c)
        for ch in node.children:
            self.do_render(c, ch)



def decode_json(text):
    return eval(text, {"false":False, "true":True})



def process_node(parent, name, data):
    if type(data) is not dict or name in ["depends_on_map_bits"]:
        return None
    oper = name
    node = TreeNode(parent, name, oper, data)
    
    return node


def tree_from_json(parent, name, json):
    node = process_node(parent, name, json)
    if not node:
        return None
    for key, value in json.items():
        if type(value) is dict:
            ch = tree_from_json(node, key, value)
            if ch:
                node.children.append(ch)
        elif type(value) is list:
            if key == "steps":
                previous = None
                node.is_step_list = True
                for item in value:
                    ch = tree_from_json(node, "step", item)
                    if ch.children: # remove useless intermediate step node
                        ch = ch.children[0]
                        node.children.append(ch)
                    ch.set_fill_color(0.8, 0.5, 0.5, 1)
                    ch.previous = previous
                    if previous:
                        previous.next = ch
                    previous = ch
            if key == "table_dependencies":
                interm = TreeNode(node, key, key, value)
                node.children.append(interm)
                for item in value:
                    ch = TableDependency(interm, item)
                    interm.children.append(ch)
            else:
                interm = TreeNode(node, key, key, {})
                node.children.append(interm)
                for item in value:
                    ch = tree_from_json(interm, key+" item", item)
                    if ch:
                        interm.children.append(ch)
    return node


def render_json_data(json_text, background_image, png_file):
    padding = 50

    json = decode_json(json_text)
    tree = TreeRootNode(json)
    tree.process()
    layout = TreeLayouter(tree)
    img = ImageSurface(cairo.CAIRO_FORMAT_ARGB32, 10, 100)
    ctx = Context(img)
    w, h = 500, 500
    img = ImageSurface(cairo.CAIRO_FORMAT_ARGB32, w+padding*2, h+padding*2)
    ctx = Context(img)
    ctx.set_font_size(12)
    if background_image:
        bgimage = ImageSurface.from_png(background_image)
    else:
        bgimage = None
    if bgimage and bgimage.status() == cairo.CAIRO_STATUS_SUCCESS:
        ctx.save()
        pat = Pattern(bgimage)
        pat.set_extend(cairo.CAIRO_EXTEND_REPEAT)
        ctx.set_source(pat)
        ctx.paint()
        ctx.restore()
    else:
        ctx.set_source_rgb(1,1,1)
        ctx.paint()
#layout.render(ctx, 0, 0)
    tree.layout(ctx, 0, 0)
    tree.render(ctx)
    img.write_to_png(png_file)

    return w+padding*2, h+padding*2

if __name__ == "__main__":
    import sys
    f = sys.argv[1]
    render_json_data(open(f).read(), None, f+".png")
