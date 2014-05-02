# Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.
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

from cairo_utils import *
from math import pi, sin, cos
from random import random

class TreePieChart:
    """A pie chart that displays fractional information hierarchically."""
    def __init__(self, tree, cairo):
        self._tree = tree
        self._level_radius = 100
        self._c = cairo
        self._main_colors = [
        (0.1, 0.4, 0.1),
        (0.1, 0.4, 0.4),
        (0.4, 0.4, 0.1),
        (0.4, 0.1, 0.1),
        ]

    def plot(self):
        self._c.save()
        self._c.translate(400, 400)
        self.plot_chart_level(self._tree, 0.5, 0, 360)
        self._c.restore()

    def plot_chart_level(self, data, depth = 0, start_angle = 0, end_angle = 360, color = None):
        total = 0
        # calculate total value for the level and their fractions
        for c, v in data.values():
            total += v
        if total == 0 or depth > 2:
            return
    
        range = end_angle - start_angle
        i = 0
        text_to_show = []
        # calculate the percentage of each part in relation to the total and render it
        for k, (child_dict, v) in data.items():
            if v == 0:
                continue
            pct = float(v) / total
            end_angle = start_angle + (pct * range)
            self._c.set_line_width(0.5)
            self._c.new_path()
            self._c.arc(0, 0, self._level_radius * (depth+1), start_angle * pi / 180, end_angle * pi / 180)
            self._c.arc_negative(0, 0, (self._level_radius) * (depth + 0), end_angle * pi / 180, start_angle * pi / 180)
            self._c.close_path()
            self._c.set_source_rgba(0, 0, 0, 1)
            self._c.stroke_preserve()
            if color:
                r, g, b = color
                r = r * 1.4 + random() * 0.2
                g = g * 1.4 + random() * 0.2
                b = b * 1.4 + random() * 0.2
            else:
                r, g, b = self._main_colors[i]
            r, g, b = random(),random(),random()
            i += 1
            self._c.set_source_rgb(r, g, b)
            self._c.fill()
            self._c.save()
            text_angle = (start_angle + pct * range/2) 
            self._c.set_font_size(12)
            text = "%s (%i)" % (k, v)
            if text_angle > 90 and text_angle < 270:
                ext = self._c.text_extents(text)
                text_to_show.append((((text_angle+180)*pi / 180), -self._level_radius * (depth+0.2) - ext.width, text))
            else:
                text_to_show.append(((text_angle*pi / 180), self._level_radius * (depth+0.2), text))
            self._c.stroke()
            self._c.restore()
            self.plot_chart_level(child_dict, depth+1, start_angle, end_angle, (r, g, b))
            start_angle = end_angle

        self._c.set_font_size(12)
        self._c.set_source_rgb(1, 1, 1)
        for a, x, t in text_to_show:
            self._c.save()
            self._c.rotate(a)
            self._c.move_to(x, 0)
            self._c.show_text(t)
            self._c.restore()

def event_waits_summary_by_thread_by_event_name_to_tree(before_rows, after_rows):
    before_values = {}
    for thread_id, event_name, count_star, sum_timer_wait, min_timer_wait, avg_timer_wait, max_timer_wait in before_rows:
        before_values[event_name] = count_star, sum_timer_wait, min_timer_wait, avg_timer_wait, max_timer_wait

    root = {}
    for thread_id, event_name, count_star, sum_timer_wait, min_timer_wait, avg_timer_wait, max_timer_wait in after_rows:
        path = event_name.split("/")
        node = root
        old_count_star, old_sum_timer_wait, old_min_timer_wait, old_avg_timer_wait, old_max_timer_wait = before_values[event_name]
        for p in path:
            if node.has_key(p):
                node = node[p][0]
            else:
                new_node = {}
                node[p] = new_node, count_star-old_count_star
                node = new_node
    
    def sum_up(node):
        total = 0
        for k, (d, v) in node.items():
            s = sum_up(d)
            node[k] = d, s+v
            total += s+v
        return total
    sum_up(root)

    return root

if __name__ == "__main__":
    import string
    odata = [[int(0) if i[0] in string.digits else i.replace("wait/", "wait-") for i in s.split("\t")] for s in open("event_waits_summary_by_thread_by_event_name.tsv").readlines()]
    data = [[int(i) if i[0] in string.digits else i.replace("wait/", "wait-") for i in s.split("\t")] for s in open("event_waits_summary_by_thread_by_event_name2.tsv").readlines()]
    surf = ImageSurface(cairo.CAIRO_FORMAT_ARGB32, 800, 800)
    c = Context(surf)
    c.set_source_rgb(0,0,0)
    c.paint()
    tree = event_waits_summary_by_thread_by_event_name_to_tree(odata, data)

    xtree = {
      "a" : ({"a1":({}, 5), "a2":({},5)}, 10),
      "b" : ({"b1":({}, 3), "b2":({},5), "b3":({}, 2)}, 10)
    }
    piechart = TreePieChart(tree, c)
    piechart.plot()

    surf.write_to_png("pie.png")
