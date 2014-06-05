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

import cairo
from math import ceil, floor, fabs, atan, pi


class Surface(object):
    def __del__(self):
        cairo.cairo_surface_destroy(self.s)

    def status(self):
        return cairo.cairo_surface_status(self.s)

    def write_to_png(self, file):
        cairo.cairo_surface_write_to_png(self.s, file)


class ImageSurface(Surface):
    def __init__(self, format=cairo.CAIRO_FORMAT_ARGB32, width=None, height=None, surfobj=None):
        Surface.__init__(self)
        if surfobj:
            self.s = surfobj
        else:
            self.s = cairo.cairo_image_surface_create(format, width, height)

    def get_width(self):
        return cairo.cairo_image_surface_get_width(self.s)

    def get_height(self):
        return cairo.cairo_image_surface_get_height(self.s)


    @classmethod
    def from_png(self, png):
        return ImageSurface(surfobj = cairo.cairo_image_surface_create_from_png(png))


class Context(object):
    def __init__(self, arg):
        if isinstance(arg, Surface):
            self.cr = cairo.cairo_create(arg.s)
            self._owned = True
        else:
            self.cr = arg
            self._owned = False

    def __del__(self):
        if self._owned:
            cairo.cairo_destroy(self.cr)

    def new_path(self):
        cairo.cairo_new_path(self.cr)

    def new_sub_path(self):
        cairo.cairo_new_sub_path(self.cr)

    def close_path(self):
        cairo.cairo_close_path(self.cr)

    def set_fill_rule(self, rule):
        cairo.cairo_set_fill_rule(self.cr, rule)

    def move_to(self, x, y):
        cairo.cairo_move_to(self.cr, x, y)

    def rel_move_to(self, x, y):
        cairo.cairo_rel_move_to(self.cr, x, y)

    def line_to(self, x, y):
        cairo.cairo_line_to(self.cr, x, y)

    def curve_to(self, x1, y1, x2, y2, x3, y3):
        cairo.cairo_curve_to(self.cr, x1, y1, x2, y2, x3, y3)

    def arc(self, xc, yc, r, a1, a2):
        cairo.cairo_arc(self.cr, xc, yc, r, a1, a2)

    def arc_negative(self, xc, yc, r, a1, a2):
        cairo.cairo_arc_negative(self.cr, xc, yc, r, a1, a2)

    def scale(self, x, y):
        cairo.cairo_scale(self.cr, x, y)

    def translate(self, x, y):
        cairo.cairo_translate(self.cr, x, y)

    def rotate(self, d):
        cairo.cairo_rotate(self.cr, d)

    def fill(self):
        cairo.cairo_fill(self.cr)

    def fill_preserve(self):
        cairo.cairo_fill_preserve(self.cr)

    def paint(self):
        cairo.cairo_paint(self.cr)

    def rectangle(self, x, y, w, h):
        cairo.cairo_rectangle(self.cr, x, y, w, h)

    def save(self):
        cairo.cairo_save(self.cr)

    def restore(self):
        cairo.cairo_restore(self.cr)
 
    def set_dash(self, dashes, offset):
        cairo.cairo_set_dash(self.cr, dashes, offset)

    def set_font(self, family, italic=False, bold=False):
        cairo.cairo_select_font_face(self.cr, family, cairo.CAIRO_FONT_SLANT_ITALIC if italic else cairo.CAIRO_FONT_SLANT_NORMAL, cairo.CAIRO_FONT_WEIGHT_BOLD if bold else cairo.CAIRO_FONT_WEIGHT_NORMAL)

    def set_font_size(self, size):
        cairo.cairo_set_font_size(self.cr, size)

    def set_line_width(self, w):
        cairo.cairo_set_line_width(self.cr, w)

    def set_source(self, pat):
        cairo.cairo_set_source(self.cr, pat.p)

    def set_source_surface(self, sur, x, y):
        cairo.cairo_set_source_surface(self.cr, sur.s, x, y)

    def set_source_rgb(self, r, g, b):
        cairo.cairo_set_source_rgb(self.cr, r, g, b)

    def set_source_rgba(self, r, g, b, a):
        cairo.cairo_set_source_rgba(self.cr, r, g, b, a)

    def mask(self, pattern):
        cairo.cairo_mask(self.cr, pattern.p)

    def mask_surface(self, surface, x, y):
        cairo.cairo_mask_surface(self.cr, surface.s, x, y)

    def show_text(self, text):
        cairo.cairo_show_text(self.cr, text)

    def rounded_rect(self, x, y, w, h, r):
        self.move_to(x+r, y)
        self.line_to(x+w-r, y)
        self.curve_to(x+w, y, x+w, y, x+w, y+r)
        self.line_to(x+w, y+h-r)           
        self.curve_to(x+w, y+h, x+w, y+h, x+w-r, y+h)
        self.line_to(x+r, y+h)                   
        self.curve_to(x, y+h, x, y+h, x, y+h-r)      
        self.line_to(x, y+r)                     
        self.curve_to(x, y, x, y, x+r, y)

    def show_text_lines_at(self, x, y, text, spacing, line_height= None):
        if line_height is None:
            line_height = 0
            for line in text.split("\n"):
                extents = self.text_extents(line)
                line_height = max(line_height, int(extents.height + (extents.height + extents.y_advance + extents.y_bearing)))
        for line in text.split("\n"):
            extents = self.text_extents(line)
            self.move_to(x, y + int(line_height - (extents.height + extents.y_bearing)))
            self.show_text(line)
            y += line_height + spacing


    def show_rtext_lines_at(self, x, y, text, spacing, line_height= None):
        if line_height is None:
            line_height = 0
            for line in text.split("\n"):
                extents = self.text_extents(line)
                line_height = max(line_height, int(extents.height + (extents.height + extents.y_advance + extents.y_bearing)))

        for line in text.split("\n"):
            extents = self.text_extents(line)
            self.move_to(x - extents.width, y + int(line_height - (extents.height + extents.y_bearing)))
            self.show_text(line)
            y += line_height + spacing


    def stroke(self):
        cairo.cairo_stroke(self.cr)

    def stroke_preserve(self):
        cairo.cairo_stroke_preserve(self.cr)

    def text_extents(self, text):
        return cairo.cairo_text_extents(self.cr, text)



class Pattern(object):
    def __init__(self, surface):
        self.p = cairo.cairo_pattern_create_for_surface(surface.s)

    def __del__(self):
        cairo.cairo_pattern_destroy(self.p)

    def set_extend(self, mode):
        cairo.cairo_pattern_set_extend(self.p, mode)


def show_centered_text_with_background(c, x, y, text, bg):
    ext = c.text_extents(text)
    c.save()
    c.set_source_rgb(*bg)
    c.rectangle(x-2-ext.width/2, y-2, ext.width + ext.x_bearing + 4, ext.height + (ext.height + ext.y_bearing)+4)
    c.fill()
    c.restore()
    c.move_to(x-ext.width/2, y+ext.height + (ext.height + ext.y_bearing))
    c.show_text(text)


def show_text_with_border(c, x, y, text):
    c.save()
    c.set_source_rgba(1, 1, 1, 1)
    c.move_to(x-1, y)
    c.show_text(text)
    c.move_to(x+1, y)
    c.show_text(text)
    c.move_to(x, y-1)
    c.show_text(text)
    c.move_to(x, y+1)
    c.show_text(text)
    c.restore()
    c.move_to(x, y)
    c.show_text(text)

def show_text_lines_with_border(c, x, y, text, spacing, line_height= None):
    if line_height is None:
        line_height = 0
        for line in text.split("\n"):
            extents = c.text_extents(line)
            line_height = max(line_height, int(extents.height + (extents.height + extents.y_advance + extents.y_bearing)))

    for line in text.split("\n"):
        extents = c.text_extents(line)
        show_text_with_border(c, x + 0.5, y + line_height - (extents.height + extents.y_bearing) + 0.5, line)
        y += line_height + spacing



def angle_of_line(p1, p2):
    if p1 == p2:
        angle= 0
    else:
        if p2[1] == p1[1]:
            if p2[0] < p1[0]:
                return 180
            else:
                return 0
        elif p2[1] < p1[1]:
            angle = 90.0 + atan((p2[0]-p1[0])/(p2[1]-p1[1])) * 180.0/pi
        else:
            angle = 270.0 + atan((p2[0]-p1[0])/(p2[1]-p1[1])) * 180.0/pi
            angle = angle - floor(angle/360)*360;
    return angle

def draw_arrow_head(c, p, ps, l=10, w=3):
    c.save()
    a = angle_of_line(p, ps)
    c.translate(int(p[0])+0.5, int(p[1])+0.5)
    c.rotate(-a*pi/180)
    c.move_to(0, 0)
    c.line_to(l, w)
    c.stroke()
    c.move_to(0, 0)
    c.line_to(l, -w)
    c.stroke()                                               
    c.restore()

def line_center(p1, p2):
    return int((p1[0]+p2[0])/2), int((p1[1]+p2[1])/2)


def intersect_lines(p1s, p1e, p2s, p2e):
    e1x, e1y = p1e
    s1x, s1y = p1s
    e2x, e2y = p2e
    s2x, s2y = p2s

    a1= e1y - s1y
    b1= s1x - e1x

    a2= e2y - s2y
    b2= s2x - e2x

    d = a1*b2 - a2*b1
    if fabs(d) <= 0.000000000001:
        return None
    else:
        c1 = e1x*s1y - s1x*e1y
        c2 = e2x*s2y - s2x*e2y
        x = floor((b1*c2 - b2*c1) / d + 0.5)
        y = floor((a2*c1 - a1*c2) / d + 0.5)

        if floor(min(s1x, e1x)) <= x and x <= ceil(max(s1x, e1x)) and\
           floor(min(s1y, e1y)) <= y and y <= ceil(max(s1y, e1y)) and\
           floor(min(s2x, e2x)) <= x and x <= ceil(max(s2x, e2x)) and\
           floor(min(s2y, e2y)) <= y and y <= ceil(max(s2y, e2y)):
            return x, y
    return None
 


class Node:
    def __init__(self):
        self.pos = (0, 0)
        self.size = (0, 0)
        self.color = (0, 0, 0, 1)
        self.fill_color = (1, 1, 1, 1)
        self.line_width = 1
        self.padding = (5, 5, 5, 5) # t l b r 
    
    def center(self):
        x, y = self.pos
        w, h = self.size
        return int(x + w/2)+0.5, int(y + h/2)+0.5

    def top_vertex(self):
        x, y = self.pos
        w, h = self.size
        return (x, y), (x+w, y)

    def bottom_vertex(self):
        x, y = self.pos
        w, h = self.size
        return (x, y+h), (x+w, y+h)

    def right_vertex(self):
        x, y = self.pos
        w, h = self.size
        return (x+w, y), (x+w, y+h)

    def left_vertex(self):
        x, y = self.pos
        w, h = self.size
        return (x, y), (x, y+h)


    def set_color(self, r, g, b, a = 1.0):
        self.color = (r, g, b, a)

    def set_fill_color(self, r, g, b, a = 1.0):
        self.fill_color = (r, g, b, a)

    def apply_attributes(self, c):
        c.set_source_rgba(*self.color)
        c.set_line_width(self.line_width)

    def render(self, ctx):
        self.calc(ctx)
        self.apply_attributes(ctx)
        self.do_render(ctx)

    def render_shadow(self, c):
        p1, p2 = self.bottom_vertex()
        c.set_source_rgba(0, 0, 0, 0.2)
        c.move_to(p1[0]+0.5, p1[1]+1.5)
        c.line_to(p2[0]+0.5, p2[1]+1.5)
        c.stroke()
        c.set_source_rgba(0, 0, 0, 0.05)
        c.move_to(p1[0]+0.5, p1[1]+2.5)
        c.line_to(p2[0]+0.5, p2[1]+2.5)
        c.stroke()

        p1, p2 = self.left_vertex()
        c.set_source_rgba(0, 0, 0, 0.2)
        c.move_to(p1[0]-0.5, p1[1]+1.5)
        c.line_to(p2[0]-0.5, p2[1]+1.5)
        c.stroke()

        p1, p2 = self.right_vertex()
        c.set_source_rgba(0, 0, 0, 0.2)
        c.move_to(p1[0]+1.5, p1[1]+1.5)
        c.line_to(p2[0]+1.5, p2[1]+1.5)
        c.stroke()

    
    def stroke_line_to_parent(self, c, parent, vertical):
        if vertical:
            return self.stroke_line_to_parent_v(c, parent)
        else:
            return self.stroke_line_to_parent_h(c, parent)

    
    def stroke_line_to_parent_v(self, c, parent):
        if parent:
            c.set_line_width(1)
            c.set_source_rgba(0.6, 0.6, 0.6, 1)
            v1 = parent.bottom_vertex()
            v2 = self.top_vertex()
            p1 = line_center(*v1)
            p2 = line_center(*v2)

            if p1 and p2:
                c.move_to(p1[0]+.5, p1[1]+.5)
                c.line_to(p2[0]+.5, p2[1]+.5)
                c.stroke()
                draw_arrow_head(c, p1, p2)
            return (p1, p2)

    def stroke_line_to_parent_h(self, c, parent):
        if parent:
            c.set_line_width(1)
            c.set_source_rgba(0.6, 0.6, 0.6, 1)
            v1 = parent.right_vertex()
            v2 = self.left_vertex()
            p1 = line_center(*v1)
            p2 = line_center(*v2)
            if p1 and p2:
                c.move_to(p1[0]+.5, p1[1]+.5)
                c.line_to(p2[0]+.5, p2[1]+.5)
                c.stroke()
                draw_arrow_head(c, p1, p2)
            return (p1, p2)

    def stroke_line_from_node(self, c, node):
        if node:
            c.set_line_width(2)
            c.set_source_rgba(0.6, 0.6, 0.6, 1)
            sx, sy = self.center()
            nx, ny = node.center()
            if abs(sx-nx) < abs(sy-ny):
                if sy > ny:
                    v1 = node.bottom_vertex()
                    v2 = self.top_vertex()
                else:
                    v1 = node.top_vertex()
                    v2 = self.bottom_vertex()
            else:
                if sx < nx:
                    v1 = node.left_vertex()
                    v2 = self.right_vertex()
                else:
                    v1 = node.right_vertex()
                    v2 = self.left_vertex()

            p1 = line_center(*v1)
            p2 = line_center(*v2)
            if p1 and p2:
                c.move_to(p1[0]+.5, p1[1]+.5)
                c.line_to(p2[0]+.5, p2[1]+.5)
                c.stroke()
                draw_arrow_head(c, p1, p2)
            return (p1, p2)



class TextNode(Node):
    def __init__(self, text):
        Node.__init__(self)
        self.text = text
        self.line_spacing = 4
        self.line_height = 0
        self.font_size = 12
        self.bold = False

    def calc(self, ctx):
        ctx.save()
        ctx.set_font_size(self.font_size)
        if self.bold:
            ctx.set_font("Helvetica", False, self.bold)
        if "\n" in self.text:
            lines = self.text.split("\n")
            w, h = 0, 0
            lh = 0
            for line in lines:
                ext = ctx.text_extents(line)
                w = max(w, int(ext.x_bearing + ext.x_advance))
                lh = max(lh, int(ext.height + (ext.height + ext.y_advance + ext.y_bearing)))
            self.line_height = lh
            h = lh * len(lines) + self.line_spacing * (len(lines)-1)
            t, r, b, l = self.padding
            self.size = (w + r+l, h + t+b)
        else:
            ext = ctx.text_extents(self.text)
            self._extents = ext
            t, r, b, l = self.padding
            self.line_height = int(ext.height + (ext.height + ext.y_advance + ext.y_bearing))
            self.size = (int(ext.x_bearing + ext.x_advance) + r+l, self.line_height + t+b)
        ctx.restore()


    def do_render(self, ctx):
        ctx.save()
        ctx.set_font_size(self.font_size)
        if self.bold:
            ctx.set_font("Helvetica", False, self.bold)
        t, r, b, l = self.padding
        x, y = self.pos
        w, h = self.size
        ctx.show_text_lines_at(int(x+l)+0.5, int(y+t)+0.5, self.text, self.line_spacing, self.line_height)
        ctx.stroke()
        ctx.restore()



class TextRectangle(TextNode):
    def __init__(self, text):
        TextNode.__init__(self, text)
        self.icon = None
        self.icon_alpha = 1
        self.border_color = (125.0/255, 125.0/255, 125.0/255, 1)
        self.draw_vertices = (True, True, True, True)

    def set_border_color(self, r, g, b, a = 1.0):
        self.border_color = (r, g, b, a)

    def set_icon(self, path): 
        image = ImageSurface.from_png(path)
        if image.status() == cairo.CAIRO_STATUS_SUCCESS:
            self.icon = image
        else:
            self.icon = None

    def calc(self, ctx):
        TextNode.calc(self, ctx)
        w, h = self.size
        if self.icon:
            w += self.icon.get_width() + 3
        self.size = max(50, w), h

    def do_render(self, ctx):
        x, y = self.pos
        w, h = self.size
        if self.fill_color:
            ctx.save()
            ctx.rectangle(x+0.5, y+0.5, w, h)
            ctx.set_source_rgba(*self.fill_color)
            ctx.fill()
            ctx.restore()

        if self.icon:
            ctx.save()
            t, r, b, l = self.padding
            ctx.translate(x + l, y + int((h - self.icon.get_height())/2))
            ctx.set_source_surface(self.icon, 0, 0)
            if int(self.icon_alpha) == 1:
                ctx.paint()
            else:
                ctx.paint_with_alpha(self.icon_alpha)
            ctx.restore()

        ctx.save()
        if self.icon:
            ctx.translate(self.icon.get_width() + 4, 0)
        TextNode.do_render(self, ctx)
        ctx.restore()

        if self.border_color and self.draw_vertices:
            ctx.set_source_rgba(*self.border_color)
            if self.draw_vertices == (True, True, True, True):
                ctx.rectangle(x+0.5, y+0.5, w, h)
                ctx.stroke()
            else:
                t, l, b, r = self.draw_vertices
                if t:
                    ctx.move_to(x+0.5, y+0.5)
                    ctx.line_to(x+0.5+w, y+0.5)
                    ctx.stroke()
                if b:
                    ctx.move_to(x+0.5, y+0.5+h)
                    ctx.line_to(x+0.5+w, y+0.5+h)
                    ctx.stroke()
                if l:
                    ctx.move_to(x+0.5, y+0.5)
                    ctx.line_to(x+0.5, y+0.5+h)
                    ctx.stroke()
                if r:
                    ctx.move_to(x+0.5+w, y+0.5)
                    ctx.line_to(x+0.5+w, y+0.5+h)
                    ctx.stroke()


class VBoxNode(Node):
    def __init__(self):
        Node.__init__(self)

        self.items = []

    def set_color(self, r, g, b, a = 1.0):
        for item in self.items:
            item.set_color(r, g, b, a)

    def set_fill_color(self, r, g, b, a = 1.0):
        for item in self.items:
            item.set_fill_color(r, g, b, a)

    def layout_internal(self):
        w, h = self.size
        y = 0
        for item in self.items:
            item.pos = 0, y
            item.size = w, item.size[1]
            y += item.size[1]

    def calc(self, c):
        h = 0
        w = 0
        for item in self.items:
            item.calc(c)
            ww, hh = item.size
            w = max(ww, w)
            h += hh
        self.size = w, h
        y = 0
        for item in self.items:
            item.pos = (0, y)
            item.size = (w, item.size[1])
            y += item.size[1]

    def do_render(self, c):
        pass

    def stroke_line_to_parent(self, c, node, vertical):
        c.set_line_width(1)
        c.set_source_rgb(0, 0, 0)
        ch = self.items[0]
        pos = ch.pos
        ch.pos = pos[0] + self.pos[0], pos[1] + self.pos[1]
        p1, p2 = ch.stroke_line_to_parent(c, node, vertical)
        ch.pos = pos
        return p1, p2


    def render(self, c):
        self.do_render(c)
        c.save()
        c.translate(self.pos[0], self.pos[1])
        for item in self.items:
            item.apply_attributes(c)
            item.do_render(c)
        c.restore()



