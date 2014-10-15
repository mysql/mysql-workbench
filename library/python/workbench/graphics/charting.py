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


from workbench.graphics.canvas import Figure, ImageFigure, RectangleShapeFigure, TextFigure

import re
import math

#from grt import log_error
#_this_file = os.path.basename(__file__)


def scale_value(value):
    if value >= 1024*1024*1024*1024:
        unit = "T"
        value /= 1024*1024*1024*1024
    elif value >= 1024*1024*1024:
        unit = "G"
        value /= 1024*1024*1024
    elif value >= 1024*1024:
        unit = "M"
        value /= 1024*1024
    elif value >= 1024:
        unit = "K"
        value /= 1024
    else:
        value = int(round(value))
        unit = ""
    return value, unit


class DBTimeLineGraph(Figure):
    def __init__(self, calc, vaxis_format="%s", ndatasets=1, scale_unit=True, auto_scale_vaxis=True):
        Figure.__init__(self)
        
        self.calc = calc
        
        self._vaxis_format = vaxis_format
        
        self._ndatasets = ndatasets
        self._points = [[] for i in range(ndatasets)]
        
        self._width = 160
        self._height = 120
        
        self._vertical_dividers = 4
        self._auto_scale_vaxis = auto_scale_vaxis
        self._scale = 100
        
        self._scale_unit = scale_unit
        
        self._detail_line_pos = None
        
        # how many seconds each pixel represents
        self._seconds_per_hpixel = 1
    
        self._colors = [(0, 0, 0) for i in range(ndatasets)]
    
    
    def set_max_value(self, value):
        self._scale = value
    
    
    def set_main_color(self, colors):
        assert self._ndatasets == 1 or (type(colors) is list and len(colors) == self._ndatasets)

        if self._ndatasets == 1 and type(colors) is not list:
            self._colors = [colors]
        else:
            self._colors = colors

    def format_vaxis_value(self, value):
        if self._auto_scale_vaxis:
            v, u = scale_value(value)
            if not u:
                format = self._vaxis_format
                i = format.find("%")
                format = format[:i]+re.sub(r"^%(\.|[0-9])*f", "%i", format[i:])
                return format % (v, u)
            else:
                return self._vaxis_format % (v, u)
        else:
            # if the value is not scaled, replace the %f format string with %i,
            # so we don't end up with smething like 5.00 Bytes
            format = self._vaxis_format
            i = format.find("%")
            format = format[:i]+re.sub(r"^%(\.|[0-9])?f", "%i", format[i:])
            return format % (value, "")


    def render(self, c):
        if self._detail_line_pos is not None:
            time_offset, detail_values = self.values_at_offset(self._detail_line_pos)
        else:
            time_offset, detail_values = None, None

        c.save()
        c.translate(self.x, self.y)
        
        # draw axis
        c.set_line_width(2)
        c.set_source_rgb(0, 0, 0)
        c.move_to(0, self.height - 0.5)
        c.line_to(self.width, self.height - 0.5)
        c.stroke()
        
        # draw scale lines
        c.set_line_width(1)
        c.set_font_size(10)
        for i in range(1, self._vertical_dividers+1):
            y = self.height - i * (self.height - 20) / self._vertical_dividers
            if i > 0:
                c.set_source_rgb(0.8, 0.8, 0.8)
                c.move_to(0, y + 0.5)
                c.line_to(self.width, y + 0.5)
                c.stroke()
            
            if not detail_values:
                c.set_source_rgb(0, 0, 0)
                c.move_to(0, y - 2.5)
                c.show_text(self.format_vaxis_value(i * self._scale / self._vertical_dividers))
        
        if detail_values:
            y = 10
            c.set_font_size(12)
            for i, value in enumerate(detail_values):
                c.move_to(0, y)
                y += 14
                c.set_source_rgb(*self._colors[i])
                c.show_text("%s - %i seconds ago" % (self.format_vaxis_value(value), time_offset))

        if self._points[0]:
            # draw data
            c.set_line_width(2)
            
            for i, points in enumerate(self._points):
                r, g, b = self._colors[i]
                c.set_source_rgba(r, g, b, 1)

                x = self.width + 0.5
                p0, t0 = points[-1]
                tp = t0

                c.move_to(x, self.height - (float(p0) / self._scale) * (self.height - 20))
                for p, t in reversed(points[:-1]):
                    x -= round((tp - t) / self._seconds_per_hpixel)
                    tp = t
                    
                    y = int(self.height - (float(p) / self._scale) * (self.height - 20)) + 0.5
                    c.line_to(x, y)
                c.stroke()

        if self._detail_line_pos is not None:
            c.set_source_rgba(1, 0, 0, 0.8 if detail_values else 0.1)
            c.set_line_width(1)
            c.move_to(self._detail_line_pos + 0.5, 0)
            c.line_to(self._detail_line_pos + 0.5, self.height)
            c.stroke()

        c.restore()


    def values_at_offset(self, x):
        if self._points and self._points[0]:
            values = []
            visible_width = (self._points[0][-1][1] - self._points[0][0][1]) * self._seconds_per_hpixel
            if visible_width < self.width:
                offs = x - (self.width - visible_width)
                if offs < 0:
                    return None, None
            else:
                offs = x
            
            time_offset = None
            for points in self._points:
                p, t0 = points[0]
                ts = offs / self._seconds_per_hpixel
                for p, t in points:
                    if ts <= t - t0:
                        values.append(p)
                        time_offset = points[-1][1] - t
                        break
            return time_offset, values
        return None, None


    def process(self, data, timestamp):
        values = self.calc.handle(data, timestamp)
        if values is None:
            return
        assert type(values) in (tuple, list) and len(values) == self._ndatasets or self._ndatasets == 1

        if self._ndatasets == 1 and type(values) not in (list, tuple):
            self._points[0].append((values, timestamp))
                
            # trim the list of points to fit what we can display
            while (self._points[0][-1][1] - self._points[0][0][1]) / self._seconds_per_hpixel > self.width:
                del self._points[0][0]
        else:
            for i, value in enumerate(values):
                self._points[i].append((value, timestamp))

                # trim the list of points to fit what we can display
                while (self._points[i][-1][1] - self._points[i][0][1]) / self._seconds_per_hpixel > self.width:
                    del self._points[i][0]


        if self._auto_scale_vaxis:
            max_value = None
            for points in self._points:
                # recalculate scale
                if len(points) > 1:
                    max_value = max(max_value, reduce(max, points)[0])
                else:
                    max_value = max(max_value, points[0][0])

            if max_value is not None:
                lexvalue = "%i" % int(max_value)
                scale = int(str(int(lexvalue[0]) + 1) + "0"*len(lexvalue[1:]))
                self._scale = max(scale, 100)


    def hover_out(self, x, y):
        Figure.hover_out(self, x, y)
        self._detail_line_pos = None


    def mouse_move(self, x, y):
        Figure.mouse_move(self, x, y)

        if self._detail_line_pos != x - self.x:
            self._detail_line_pos = x - self.x
            self.invalidate(repaint_only=True)




class DBSimpleCounter(RectangleShapeFigure):
    def __init__(self, calc, format, scale_unit):
        RectangleShapeFigure.__init__(self, "N/A")

        self.calc = calc
        
        self.set_line_width(2)
        self.set_font_size(11)
        self.set_font_bold(True)
        self.set_corner_radius(10)
        self._line_spacing = 2
        self.set_text_color(0.4, 0.4, 0.4, 1)
        self._format = format or "%.2f %s"
        self._scale_unit = scale_unit

        self.set_usize(80, 35)
    

    def set(self, value):
        if value is None:
            value = 0
        unit = ""
        if self._scale_unit:
            value, unit = scale_value(value)
        self.set_text(self._format % (value, unit))


    def process(self, data, timestamp):
        self.set(self.calc.handle(data, timestamp))


    def set_main_color(self, colors):
        self.set_color(*colors)


class DBRoundMeter(TextFigure):
    def __init__(self, calc, caption):
        TextFigure.__init__(self, "")

        self.calc = calc
        
        self._value = 0
    
        self._caption = caption
        self.set_alignment(0.5, 0.5)
        
        self.set_line_width(25)
        self.set_font_bold(True)
        self.set_font_size(18)

        self.set_padding(0, 0, 0, 20)
        self.set_usize(120, 100)


    def set_main_color(self, color):
        self.set_color(*color)
        self.set_text_color(*color)


    def set(self, value):
        if value is not None:
            self._value = value
            self.set_text("%i%%" % int(self._value * 100))


    def process(self, data, timestamp):
        self.set(self.calc.handle(data, timestamp))


    def render(self, c):
        TextFigure.render(self, c)
        
        content_diameter = min(self.width, self.height)
        
        c.save()
        c.translate(self.x, self.y)
        self.apply_attributes(c)
        
        c.save()
        c.set_source_rgb(0.93, 0.93, 0.93)
        c.arc(content_diameter/2, content_diameter/2, min(self.width - self._line_width, self.height - self._line_width)/2,
          0, math.pi*2)
        c.stroke()
        c.restore()

        c.arc(content_diameter/2, content_diameter/2, min(self.width - self._line_width, self.height - self._line_width)/2,
              0, max(self._value * math.pi*2, 0.02))
        c.stroke()

        c.set_font(self._font, False, True)
        c.set_font_size(16)
        c.set_source_rgba(*self._text_color)
        c.move_to(content_diameter + 4 + 16, self.height - (content_diameter - c.text_extents(self._caption).width)/2)
        c.rotate(-math.pi/2)
        c.show_text(self._caption)
        c.stroke()
        c.restore()


class DBHBarMeter(Figure):
    def __init__(self, calc, label1, label2):
        Figure.__init__(self)
        self.calc = calc
        self._label1 = label1
        self._label2 = label2
        self._value1 = 0
        self._value2 = 0
        self.set_usize(150, 56)

    
    def render(self, c):
        c.save()
        c.translate(self.x, self.y)
        if self._value2 > 0:
            p = self.width * (float(self._value1) / self._value2)
            c.set_source_rgb(0.3, 0.7, 0.3)
            c.rectangle(0, 0, p, self.height - 25)
            c.fill()
            c.set_source_rgb(0.7, 0.3, 0.3)
            c.rectangle(p, 0, self.width - p, self.height - 25)
            c.fill()
        elif self._value1 > 0:
            c.set_source_rgb(0.3, 0.7, 0.3)
            c.rectangle(0, 0, self.width, self.height - 25)
            c.fill()
        else:
            c.set_source_rgb(0.5, 0.5, 0.5)
            c.rectangle(0, 0, self.width, self.height - 25)
            c.fill()
        c.set_source_rgb(0.4, 0.4, 0.4)
        c.set_font_size(11)
        c.show_text_lines_at(0, self.height - 22,
                             self._label1 % self._value1,
                             0, 12)
        
        c.show_rtext_lines_at(self.width, self.height - 22,
                              self._label2 % self._value2,
                              0, 12)
        c.restore()


    def set(self, value_pair):
        self._value1, self._value2 = value_pair


    def process(self, data, timestamp):
        self.set(self.calc.handle(data, timestamp))


    def set_main_color(self, color):
        self.set_color(*color)


class DBLevelMeter(Figure):
    def __init__(self, calc):
        Figure.__init__(self)
        self.calc = calc


        self._max_value = 100
        self._max_seen_value = 0
        self._value = 0
        
        self.set_usize(100, 100)


    def set_main_color(self, color):
        self.set_color(*color)


    def init(self, max_value):
        self._max_value = max_value


    def set(self, value):
        self._value = value
        self._max_seen_value = max(self._max_seen_value, value)


    def process(self, data, timestamp):
        self.set(self.calc.handle(data, timestamp))


    def render(self, cr):
        cr.save()
        cr.translate(self.x, self.y)
        cr.set_source_rgb(0.8, 0.8, 0.8)
        cr.rectangle(0, 0, 30, self.height)
        cr.fill()

        p1 = (float(self._max_seen_value)/self._max_value)*self.height
        p2 = (float(self._value)/self._max_value)*self.height
        
        cr.set_source_rgb(0.5, 0.5, 0.5)

        limit_y = cr.text_extents("limit %s" % self._max_value).height

        cr.move_to(34, limit_y)
        cr.show_text("limit %s" % self._max_value)

        if p1 != p2:
            max_h = cr.text_extents("max %s" % self._max_seen_value).height
            max_y = self.height - p1 - max_h
            if max_y < limit_y:
                cr.move_to(34, limit_y + max_h + 2)
                cr.show_text("max %s" % self._max_seen_value)
            else:
                cr.move_to(34, self.height - p1)
                cr.show_text("max %s" % self._max_seen_value)


        cr.set_source_rgb(0.9, 0.9, 0.3)
        cr.rectangle(0, self.height - p1, 30, p1)
        cr.fill()
        
        self.apply_attributes(cr)
        cr.rectangle(0, self.height - p2, 30, p2)
        cr.fill()

        cr.set_source_rgb(0, 0, 0)
        cr.move_to((30 - cr.text_extents("%s" % self._value).width)/2, self.height - 1 - p2)
        cr.show_text("%s" % self._value)

        cr.restore()


class DBImage(ImageFigure):
    def __init__(self, dummy, path):
        ImageFigure.__init__(self, path)
    
    def set(self, value):
        if value is None:
            value = 0
        unit = ""
        if self._scale_unit:
            value, unit = scale_value(value)
        self.set_text(self._format % (value, unit))
    
    
    def set_alignment(self, x, y):
        pass
    
    def process(self, data, timestamp):
        pass
    
    
    def set_main_color(self, colors):
        pass


class DBVLine(Figure):
    def __init__(self, dummy, height):
        Figure.__init__(self)
    
        self._height = height
    
    
    def set(self, value):
        pass
    
    
    def set_alignment(self, x, y):
        pass
    
    def process(self, data, timestamp):
        pass
    
    
    def set_main_color(self, colors):
        self.set_color(colors[0], colors[1], colors[2], 1)


    def render(self, cr):
        cr.save()
        cr.translate(self.x, self.y)
        self.apply_attributes(cr)
        cr.move_to(0, 0)
        cr.line_to(0, self.height)
        cr.stroke()
        cr.restore()


class DBText(TextFigure):
    def __init__(self, dummy, text):
        TextFigure.__init__(self)
    
        self.set_font_size(10)
        self.set_alignment(0.5, 0)
        self.set_text(text)
    
    
    def set(self, value):
        pass
        
    def process(self, data, timestamp):
        pass
    
    def set_main_color(self, colors):
        self.set_text_color(colors[0], colors[1], colors[2], 1)

