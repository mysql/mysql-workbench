# Copyright (c) 2013, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import cairo
import cairo_utils

def draw_varrow(cr, tip_pos, ah = 4, aw = 4):
    cr.new_path()
    x, y = tip_pos
    cr.move_to(x, y)
    cr.line_to(x - aw/2, y + ah - 0.5)
    cr.line_to(x + aw/2, y + ah - 0.5)
    cr.close_path()


def draw_harrow(cr, tip_pos, ah = 4, aw = 4):
    cr.new_path()
    x, y = tip_pos
    cr.move_to(x, y)
    cr.line_to(x - ah - 0.5, y - aw/2)
    cr.line_to(x - ah - 0.5, y + aw/2)
    cr.close_path()

#

class Settings:
    default_font = "Helvetica"


settings = Settings()


class Canvas(object):
    def __init__(self, set_needs_repaint_cb):
        self._size = (0, 0)

        self.tooltip = None

        self.set_needs_repaint = set_needs_repaint_cb

        self._figures = []

        self._mouse_down_figures = [None] * 5
        self._hover_figure = None
        self._background_color = (1,1,1)


    def deactivate(self):
        if self.tooltip:
            self.tooltip.close()
            self.tooltip = None


    def activate(self):
        pass


    def set_background_color(self, r, g, b):
        self._background_color = r, g, b


    def add(self, obj):
        obj.set_canvas(self)
        self._figures.append(obj)
        obj.invalidate()


    def remove(self, obj):
        self.invalidate(*obj.bounds)
        self.set_needs_repaint()
        obj.set_canvas(None)
        self._figures.remove(obj)


    def repaint(self, cr, x, y, w, h):
        cr.set_source_rgb(*self._background_color)
        cr.rectangle(0, 0, w, h)
        cr.fill()
        cr.translate(x, y)
        for f in self._figures:
            f.relayout(cr)
            f.render(cr)


    def invalidate(self, x, y, w, h):
        self.set_needs_repaint(x, y, w, h)


    def figure_at(self, x, y):
        for f in self._figures:
            if f.contains_point(x, y):
                return f
        return None


    # event handling
    def mouse_down(self, button, x, y):
        fig = self.figure_at(x, y)
        if fig:
            self._mouse_down_figures[button] = fig
            fig.mouse_down(button, x, y)
            return True
        return False

    def mouse_up(self, button, x, y):
        if self._mouse_down_figures[button]:
            self._mouse_down_figures[button].mouse_up(button, x, y)
            self._mouse_down_figures[button] = None


    def mouse_leave(self):
        #if self.tooltip:
        #    self.tooltip.close()
        #    self.tooltip = None
        pass


    def mouse_move(self, x, y):
        dragged = False
        for b, f in enumerate(self._mouse_down_figures):
            if f:
                dragged = True
                f.mouse_drag(b, x, y)
                break
        if not dragged:
            fig = self.figure_at(x, y)
            if fig != self._hover_figure:
                if self._hover_figure:
                    self._hover_figure.hover_out(x, y)
                if fig:
                    fig.hover_in(x, y)
                self._hover_figure = fig
            else:
                if self._hover_figure:
                    self._hover_figure.hover(x, y)
            if self._hover_figure:
                self._hover_figure.mouse_move(x, y)

# Base Classes

class Element(object):
    def __init__(self):
        self.canvas = None
        self.parent = None
        self._layout_dirty = False
        self._color = (0, 0, 0, 1)
    
        self.on_hover_in = None
        self.on_hover_out = None

    def set_canvas(self, canvas):
        self.canvas = canvas

    def invalidate(self, repaint_only = False):
        if not repaint_only:
            self._layout_dirty = True
        if self.canvas:
            self.canvas.invalidate(*self.bounds)

    def relayout(self, cr):
        if self._layout_dirty:
            self.do_relayout(cr)
        self._layout_dirty = False

    def do_relayout(self, cr):
        pass

    def render(self, cr):
        pass

    def set_color(self, r, g, b, a = 1.0):
        self._color = (r, g, b, a)

    # event handling
    def mouse_down(self, button, x, y):
        pass

    def mouse_up(self, button, x, y):
        pass

    def mouse_drag(self, button, x, y):
        pass

    def mouse_move(self, x, y):
        pass
    
    def hover_in(self, x, y):
        if self.on_hover_in:
            self.on_hover_in(self, x, y)

    def hover_out(self, x, y):
        if self.on_hover_out:
            self.on_hover_out(self, x, y)

    def hover(self, x, y):
        pass



HFill = 1 << 0
VFill = 1 << 1


class Figure(Element):
    def __init__(self):
        Element.__init__(self)
        self._x = 0
        self._y = 0
        self._width = 0
        self._height = 0
        self._uwidth = None
        self._uheight = None
        self._fill_color = (1, 1, 1, 1)
        self._line_width = 1
        self._dash = (None, None)
        self._padding = (0, 0, 0, 0)
        self._layout_flags = HFill | VFill

    @property
    def x(self):
        return self._x

    @property
    def y(self):
        return self._y

    @property
    def root_x(self):
        return self.x + (self.parent.root_x if self.parent else 0)

    @property
    def root_y(self):
        return self.y + (self.parent.root_y if self.parent else 0)

    @property
    def width(self):
        return self._width if self._uwidth is None else self._uwidth

    @property
    def height(self):
        return self._height if self._uheight is None else self._uheight

    @property
    def pos(self):
        return (self._x, self._y)

    @property
    def size(self):
        return (self.width, self.height)

    @property
    def bounds(self):
        return 0, 0, self.width, self.height

    @property
    def frame(self):
        return self.x, self.y, self.width, self.height

    @property
    def padding_top(self):
        return self._padding[0]

    @property
    def padding_left(self):
        return self._padding[1]

    @property
    def padding_bottom(self):
        return self._padding[2]

    @property
    def padding_right(self):
        return self._padding[3]

    def move(self, x, y):
        if self.canvas:
            self.canvas.invalidate(*self.frame)
        self._x = x
        self._y = y
        if self.canvas:
            self.canvas.invalidate(*self.frame)

    def contains_point(self, x, y):
        pos = self.pos
        size = self.size
        return x >= pos[0] and x < pos[0] + size[0] and y >= pos[1] and y < pos[1] + size[1]
    
    def center(self):
        x, y = self.pos
        w, h = self.size
        return int(x + w/2), int(y + h/2)

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
    

    def set_padding(self, t, l, b, r):
        self._padding = t, l, b, r

    def set_layout_flags(self, flags):
        self._layout_flags = flags
    
    def set_line_dash(self, dash, offset):
        self._dash = (dash, offset)
        self.invalidate()

    def set_line_width(self, l):
        self._line_width = l
        self.invalidate()

    def set_fill_color(self, r, g, b, a = 1.0):
        self._fill_color = (r, g, b, a)
        self.invalidate()

    def apply_attributes(self, c):
        c.set_source_rgba(*self._color)
        if self._dash[0]:
            c.set_dash(self._dash[0], self._dash[1])
        c.set_line_width(self._line_width)

    def apply_fill_attributes(self, c):
        c.set_source_rgba(*self._fill_color)

    def set_usize(self, w, h):
        self._uwidth = w
        self._uheight = h


class Line(object):
    def __init__(self):
        Element.__init__(self)
        self._end1 = None
        self._end2 = None

    @property
    def bounds(self):
        return 0, 0, 0, 0

    def contains_point(self, x, y):
        return None


#
#

class Container(Figure):
    def __init__(self):
        Figure.__init__(self)
        self._items = []

    def add(self, item):
        item.parent = self
        self._items.append(item)


    def remove(self, item):
        item.parent = None
        self._items.remove(item)


    def render(self, cr):
        cr.save()
        cr.translate(self.x, self.y)
        for i in self._items:
            i.render(cr)
        cr.restore()


class VBoxFigure(Container):
    def __init__(self):
        Container.__init__(self)
        self._spacing = 0
    
    
    def set_spacing(self, sp):
        self._spacing = sp

    def do_relayout(self, cr):
        lp, tp, rp, bp = self._padding
        y = tp
        x = lp
        max_width = self._width
        for i in self._items:
            i.do_relayout(cr)
            i.move(x, y)
            max_width = max(max_width, i.width)
            y += int(i.height) + self._spacing
        if self._items:
            y -= self._spacing

        for i in self._items:
            if i._layout_flags & HFill:
                i.set_usize(max_width, i._uheight)
                i.do_relayout(cr)
            else:
                i.move((max_width - i.width) / 2, i.y)

        self._width = max_width + lp + rp
        self._height = y + bp

        self.invalidate()

#
#

class TextFigure(Figure):
    def __init__(self, text=""):
        Figure.__init__(self)
        self._text = text
        self._font = settings.default_font
        self._font_size = 12
        self._text_color = (0, 0, 0, 1)
        self._line_spacing = 2
        self._bold = False
        self._xalignment = 0.0
        self._yalignment = 0.0

        self._line_height = 14
        self._text_height = 0

    def set_text_color(self, r, g, b, a=1.0):
        self._text_color = r, g, b, a
        self.invalidate()
    
    def set_font_size(self, s):
        self._font_size = s

    def set_font_bold(self, s):
        self._bold = s

    def set_alignment(self, x, y):
        self._xalignment = x
        self._yalignment = y
        self.invalidate(False)


    def set_text(self, text):
        self._text = text
        self.invalidate(False)


    def do_relayout(self, ctx):
        ctx.save()
        ctx.set_font(self._font, False, self._bold)
        ctx.set_font_size(self._font_size)

        if "\n" in self._text:
            lines = self._text.split("\n")
            w, h = 0, 0
            lh = 0
            self._text_height = 0
            for line in lines:
                ext = ctx.text_extents(line)
                w = max(w, int(ext.x_bearing + ext.x_advance))
                lh = max(lh, int(ext.height + ext.height + ext.y_bearing))
                self._text_height += ext.height + self._line_spacing
            if lines:
                self._text_height -= self._line_spacing
            self._line_height = int(lh)
            t, l, b, r = self._padding
            
            self._width, self._height = (w + r+l, self._text_height + t+b)
        else:
            ext = ctx.text_extents(self._text)
            self._extents = ext
            t, l, b, r = self._padding
            self._line_height = int(ext.height) + int(ext.height + ext.y_bearing)
            self._width = int(ext.x_bearing + ext.x_advance) + r+l
            self._height = self._line_height + t+b
            self._text_height = self._line_height
        ctx.restore()


    def render(self, ctx):
        ctx.save()
        ctx.translate(self.x, self.y)
        ctx.set_source_rgba(*self._text_color)
        ctx.set_font(self._font, False, self._bold)
        ctx.set_font_size(self._font_size)
        t, l, b, r = self._padding
        w, h = self.size
        x = int(l) + 0.5
        y = int(t) + 0.5 + int((self.height - t - b - self._text_height) * self._yalignment)

        for line in self._text.split("\n"):
            extents = ctx.text_extents(line)
            ctx.move_to(x + int((self.width - l - r - extents.width) * self._xalignment),
                        y + int(extents.height - (extents.height + extents.y_bearing)))
            ctx.show_text(line)
            y += extents.height + self._line_spacing

        ctx.stroke()
        ctx.restore()


class ImageFigure(Figure):
    def __init__(self, file):
        Figure.__init__(self)
        
        self.set_line_width(1)

        # if this is a @2x image (for hidpi), we need to scale it by half when painting on screen
        if "@2x" in file:
            self._scale = 0.5
        else:
            self._scale = 1.0
        self._image = cairo_utils.ImageSurface.from_png(file)
        self._width = self._image.get_width() * self._scale
        self._height = self._image.get_height() * self._scale


    def set_image_file(self, path):
        self._image = cairo_utils.ImageSurface.from_png(path)
        self._width = self._image.get_width() * self._scale
        self._height = self._image.get_height() * self._scale


    def render(self, ctx):
        ctx.save()
        ctx.translate(self.x, self.y)
        if self._scale != 1.0:
            ctx.scale(self._scale, self._scale)

        ctx.set_source_surface(self._image, 0, 0)
        ctx.paint()
        
        ctx.restore()



class ShapeFigure(TextFigure):
    def __init__(self, caption):
        TextFigure.__init__(self, caption)
        
        self.set_alignment(0.5, 0.5)
        self.set_line_width(1)
    
    
    def render(self, ctx):
        ctx.save()
        ctx.translate(self.x, self.y)
        
        self.make_path(ctx)
        
        self.apply_fill_attributes(ctx)
        ctx.fill_preserve()
        self.apply_attributes(ctx)
        ctx.stroke()
        ctx.restore()

        TextFigure.render(self, ctx)


    def make_path(self, ctx):
        ctx.rectangle(*self.bounds)


class DiamondShapeFigure(ShapeFigure):
    def make_path(self, ctx):
        ctx.move_to(0, self.height/2)
        ctx.line_to(self.width/2, 0)
        ctx.line_to(self.width, self.height/2)
        ctx.line_to(self.width/2, self.height)
        ctx.close_path()


class RectangleShapeFigure(ShapeFigure):
    _corner_radius = 0
    
    def make_path(self, ctx):
        if self._corner_radius == 0:
            ctx.rectangle(0.5, 0.5, self.width, self.height)
        else:
            ctx.rounded_rect(0.5, 0.5, self.width, self.height, self._corner_radius)
    
    def set_corner_radius(self, r):
        self._corner_radius = r


#
#

class HoverTip(Figure):
    def __init__(self, text):
        Figure.__init__(self)
        self.text = text
        self.font_size = 12
        self.bold = False
        self.line_spacing = 4
        self.line_height = 10
        self.padding = (4, 4, 4, 4)


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
            t, l, b, r = self.padding
            self._width, self._height = (w + r+l, h + t+b)
        else:
            ext = ctx.text_extents(self.text)
            self._extents = ext
            t, l, b, r = self.padding
            self.line_height = int(ext.height + (ext.height + ext.y_advance + ext.y_bearing))
            self._width = int(ext.x_bearing + ext.x_advance) + r+l
            self._height = self.line_height + t+b
        ctx.restore()


    def render(self, ctx):
        ctx.save()

        ctx.rectangle(*self.bounds)
        ctx.set_source_rgb(0.9, 0.9, 0.8)
        ctx.stroke_preserve()
        ctx.set_source_rgb(1, 1, 0.9)
        ctx.fill()

        ctx.set_source_rgb(0.2, 0.2, 0.2)
        ctx.set_font_size(self.font_size)
        if self.bold:
            ctx.set_font("Helvetica", False, self.bold)
        t, l, b, r = self.padding
        x, y = self.pos
        w, h = self.size
        ctx.show_text_lines_at(int(x+l)+0.5, int(y+t)+0.5, self.text, self.line_spacing, self.line_height)
        ctx.stroke()
        ctx.restore()


    def do_relayout(self, cr):
        self.calc(cr)
