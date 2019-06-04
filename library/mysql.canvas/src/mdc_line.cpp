/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "mdc_line.h"
#include "mdc_interaction_layer.h"
#include "mdc_vertex_handle.h"
#include "mdc_box_handle.h"
#include "mdc_connector.h"
#include "mdc_magnet.h"
#include "mdc_algorithms.h"
#include "mdc_canvas_view.h"

#define LINE_POINT_THRESHOLD 5

using namespace mdc;
using namespace base;

LineLayouter::LineLayouter() {
}

LineLayouter::~LineLayouter() {
}

std::vector<ItemHandle *> LineLayouter::create_handles(Line *line, InteractionLayer *ilayer) {
  std::vector<ItemHandle *> handles;
  ItemHandle *hdl;

  Connector *conn;

  conn = get_start_connector();
  if (conn && conn->is_draggable()) {
    hdl = new VertexHandle(ilayer, line, line->get_layouter()->get_start_point(), true);
    hdl->set_tag(1);
    handles.push_back(hdl);
  }

  conn = get_end_connector();
  if (conn && conn->is_draggable()) {
    hdl = new VertexHandle(ilayer, line, line->get_layouter()->get_end_point(), true);
    hdl->set_tag(2);
    handles.push_back(hdl);
  }

  return handles;
}

void LineLayouter::update_handles(Line *line, std::vector<ItemHandle *> &handles) {
  if (!handles.empty()) {
    for (std::vector<ItemHandle *>::iterator hdl = handles.begin(); hdl != handles.end(); ++hdl) {
      if ((*hdl)->get_tag() == 1)
        (*hdl)->move(get_start_connector()->get_position());
      else if ((*hdl)->get_tag() == 2)
        (*hdl)->move(get_end_connector()->get_position());
    }
  }
}

bool LineLayouter::handle_dragged(Line *line, ItemHandle *handle, const Point &pos, bool dragging) {
  return false;
}

//------------------------------------------------------------------------

Line::Line(Layer *layer, LineLayouter *layouter) : Figure(layer), _layouter(0) {
  _start_type = NormalEnd;
  _end_type = NormalEnd;

  _hop_crossings = true;

  _line_pattern = SolidPattern;
  _line_width = 1.0;

  set_auto_sizing(false);

  set_accepts_focus(true);
  set_accepts_selection(true);

  _vertices.push_back(Point(0, 0));
  _vertices.push_back(Point(100, 200));

  if (layouter)
    set_layouter(layouter);
}

Line::~Line() {
  delete _layouter;
}

void Line::set_layouter(LineLayouter *layouter) {
  _layouter = layouter;

  scoped_connect(_layouter->signal_changed(), std::bind(&Line::update_layout, this));

  _layouter->update();
}

void Line::update_layout() {
  set_vertices(_layouter->get_points());

  if (_hop_crossings)
    get_view()->update_line_crossings(this);

  _layout_changed();
}

//--------------------------------------------------------------------------------------------------

static double dashes[9][5] = {
  //# items  paint-skip-paint-skip...
  {0, 0},           // SolidPattern
  {2, 2, 2},        // Dotted1Pattern
  {2, 2, 4},        // Dotted2Pattern
  {2, 5, 4},        // Dashed1Pattern
  {2, 10, 4},       // Dashed2Pattern
  {2, 4, 5},        // Dashed3Pattern
  {2, 4, 10},       // Dashed4Pattern
  {4, 10, 2, 2, 2}, // DashDot1Pattern
  {4, 10, 2, 4, 2}  // DashDot2Pattern
};

void Line::set_line_pattern(CairoCtx *cr, LinePatternType pattern) {
  if (pattern != SolidPattern) {
    if (dashes[pattern][0] != 0.0)
      cr->set_dash(dashes[pattern] + 1, (int)dashes[pattern][0], 0.0);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Converts the given internal patter type to a bit pattern that can be used by OpenGL.
 */
GLushort Line::get_gl_pattern(LinePatternType pattern) {
  if (pattern == SolidPattern)
    return 0xFFFF;

  GLushort result = 0;
  int remaining_bits = sizeof(GLushort) * 8;
  int index = 1;
  while (remaining_bits > 0) {
    // Wrap around if we already reached the end of the bitmap pattern.
    if (index > (int)dashes[pattern][0])
      index = 1;

    // Set as many bits as given by the current index but not more than we have room in the result.
    int bit_count = (int)dashes[pattern][index++];
    if (bit_count > remaining_bits)
      bit_count = remaining_bits;
    GLushort subpattern = 0xFFFF << bit_count;
    remaining_bits -= bit_count;
    result <<= bit_count;
    result |= ~subpattern;

    // Shift to create 0 bits.
    if (index > (int)dashes[pattern][0])
      index = 1;
    bit_count = (int)dashes[pattern][index++];
    if (bit_count > remaining_bits)
      bit_count = remaining_bits;
    result <<= bit_count;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

void Line::draw_contents(CairoCtx *cr) {
  cr->translate(get_position());

#if 0
  // draws debugging bounding box
  cr->save();
  cr->set_color(mdc::Color(0.8,0.8,0.8));
  cr->move_to(_segments.front().pos.x+0.5, _segments.front().pos.y+0.5);
  cr->line_to(_segments.back().pos.x+0.5, _segments.back().pos.y+0.5);
  cr->stroke();
  cr->restore();
  cr->save();
  cr->set_color(mdc::Color(0.8,0.8,0.8));
  cr->rectangle(0, 0, get_size().width, get_size().height);
  cr->stroke(); 

  
  cr->set_color(mdc::Color(1,0,0));
  cr->move_to(_segments.front().pos);
  cr->line_to(_segments.back().pos);
  cr->stroke();

  cr->restore();
#endif

  stroke_outline(cr);

  cr->set_line_width(_line_width);
  cr->set_color(_pen_color);
  cr->set_line_cap(CAIRO_LINE_CAP_SQUARE);
  set_line_pattern(cr, _line_pattern);

  cr->stroke();

  cr->set_dash(0, 0, 0);

  draw_line_ends(cr);
}

bool Line::contains_point(const Point &point) const {
  Point p;

  // if the line is horizontal or vertical, the bounding box is too small, so we give some slack
  if (!CanvasItem::contains_point(point)) {
    Rect r = get_root_bounds();
    bool flag = false;

    if (r.width() <= 2) {
      r.pos.x -= (3 - r.width()) / 2;
      r.size.width += 4 - r.width();
      flag = true;
    }
    if (r.height() <= 2) {
      r.pos.y -= (3 - r.height()) / 2;
      r.size.height += 4 - r.height();
      flag = true;
    }

    if (flag)
      return bounds_contain_point(r, point.x, point.y);

    return false;
  }

  p = convert_point_from(point, get_parent());

  // check if the point is in any of the segments of this line

  std::vector<SegmentPoint>::const_iterator iter = _segments.begin();
  Point prev = iter->pos, cur;
  ++iter;
  for (; iter != _segments.end(); ++iter) {
    cur = iter->pos;

    if (cur.x == prev.x || cur.y == prev.y) {
      Point corner =
        Point(std::min(cur.x, prev.x) - LINE_POINT_THRESHOLD, std::min(cur.y, prev.y) - LINE_POINT_THRESHOLD);
      Size size =
        Size(fabs(cur.x - prev.x) + LINE_POINT_THRESHOLD * 2, fabs(cur.y - prev.y) + LINE_POINT_THRESHOLD * 2);

      // orthogonal segment, do a simple bounds check
      if (bounds_contain_point(Rect(corner, size), p.x, p.y))
        return true;
    } else {
      double d = point_line_distance(prev, cur, p);
      if (fabs(d) <= LINE_POINT_THRESHOLD)
        return true;
    }

    prev = cur;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

void Line::draw_outline_ring(CairoCtx *cr, const Color &color) {
  Point position = get_position();
  cr->save();

  cr->translate(position);

  cr->set_color(color, 0.6);
  cr->set_line_width(4);
  stroke_outline(cr);
  cr->stroke_preserve();

  cr->set_color(color, 0.3);
  cr->set_line_width(8);
  cr->stroke();

  cr->restore();
}

//--------------------------------------------------------------------------------------------------

void Line::draw_outline_ring_gl(const Color &color) {
#ifndef __APPLE__
  Point position = get_position();
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslated(position.x, position.y, 0);

  gl_setcolor(color, 0.6);
  glLineWidth(3);
  stroke_outline_gl();

  gl_setcolor(color, 0.3);
  glLineWidth(7);
  stroke_outline_gl();

  glPopMatrix();
#endif
}

//--------------------------------------------------------------------------------------------------

static void draw_line_end(CairoCtx *cr, LineEndType type, const Color &lcolor, const Color &bcolor) {
  switch (type) {
    case DashedNormalEnd:
    case NormalEnd:
      break;

    case FilledTriangleEnd:
      cr->move_to(0.5, 0.5);
      cr->line_to(-3.5, 7.5);
      cr->line_to(3.5, 7.5);
      cr->line_to(0.5, 0.5);
      cr->close_path();
      cr->set_color(lcolor);
      cr->stroke_preserve();
      cr->fill();
      break;

    case HollowTriangleEnd:
      cr->move_to(0, 0);
      cr->line_to(-4, 8);
      cr->line_to(4, 8);
      cr->close_path();
      cr->set_color(bcolor);
      cr->fill_preserve();
      cr->set_color(lcolor);
      cr->stroke_preserve();
      break;

    case DashedChickenFootEnd:
    case ChickenFootEnd:
      cr->set_color(lcolor);
      cr->move_to(-5, 0);
      cr->line_to(0, 10);
      cr->line_to(5, 0);
      cr->stroke();
      break;

    case ChickenFoot0End:
      cr->set_color(lcolor);
      cr->move_to(-5, 0);
      cr->line_to(0, 10);
      cr->line_to(5, 0);
      cr->stroke();
      cr->arc(0, 12, 4, 0, 2 * M_PI);
      cr->set_color(bcolor);
      cr->fill_preserve();
      cr->set_color(lcolor);
      cr->stroke();
      break;

    case ChickenFoot1End:
      cr->move_to(-5, 0);
      cr->line_to(0, 10);
      cr->line_to(5, 0);
      cr->set_color(lcolor);
      cr->stroke();
      cr->move_to(-5, 12);
      cr->line_to(5, 12);
      cr->stroke();
      break;

    case Cross0End:
      cr->move_to(-4, 6);
      cr->line_to(4, 6);
      cr->set_color(lcolor);
      cr->stroke();
      cr->arc(0, 12, 4, 0, 2 * M_PI);
      cr->set_color(bcolor);
      cr->fill_preserve();
      cr->set_color(lcolor);
      cr->stroke();
      break;

    case Cross1End:
      cr->move_to(-4, 6);
      cr->line_to(4, 6);
      cr->set_color(lcolor);
      cr->stroke();
      cr->move_to(-4, 10);
      cr->line_to(4, 10);
      cr->stroke();
      break;

    case HollowDiamondEnd:
      cr->new_path();
      cr->move_to(0.5, 0.5);
      cr->line_to(-3.5, 6.5);
      cr->line_to(0.5, 13);
      cr->line_to(4.5, 6.5);
      cr->close_path();
      cr->set_color(bcolor);
      cr->fill_preserve();
      cr->set_color(lcolor);
      cr->stroke();
      break;

    case FilledDiamondEnd:
      cr->new_path();
      cr->move_to(0.5, 0.5);
      cr->line_to(-3.5, 6.5);
      cr->line_to(0.5, 13);
      cr->line_to(4.5, 6.5);
      cr->close_path();
      cr->set_color(lcolor);
      cr->fill_preserve();
      cr->stroke();
      break;

    case HollowCircleEnd:
      cr->arc(0, 4, 4, 0, 2 * M_PI);
      cr->set_color(lcolor);
      cr->stroke();
      break;

    case FilledCircleEnd:
      cr->arc(0, 4, 4, 0, 2 * M_PI);
      cr->set_color(lcolor);
      cr->fill_preserve();
      cr->stroke();
      break;

    case BoldStickEnd:
      cr->move_to(0.5, 0.5);
      cr->line_to(0.5, 15.5);
      cr->set_color(lcolor);
      cr->set_line_width(3);
      cr->stroke();
      break;
  }
}

//--------------------------------------------------------------------------------------------------

#ifndef __APPLE__
static void draw_line_end_gl(LineEndType type, const Color &lcolor, const Color &bcolor) {
  switch (type) {
    case DashedNormalEnd:
    case NormalEnd:
      break;

    case FilledTriangleEnd: {
      Point vertices[] = {
        Point(0, 0), Point(-4, 8), Point(4, 0), Point(0, 0),
      };
      gl_polygon(vertices, 4, lcolor, bcolor);
      break;
    }

    case HollowTriangleEnd: {
      Point vertices[] = {
        Point(0, 0), Point(-4, 8), Point(4, 8),
      };
      gl_polygon(vertices, 3, lcolor, bcolor);
      break;
    }

    case DashedChickenFootEnd:
    case ChickenFootEnd:
      gl_setcolor(lcolor);
      glBegin(GL_LINE_STRIP);
      glVertex2d(-5, 0);
      glVertex2d(0, 10);
      glVertex2d(5, 0);
      glEnd();
      break;

    case ChickenFoot0End:
      gl_setcolor(lcolor);
      glBegin(GL_LINE_STRIP);
      glVertex2d(-5, 0);
      glVertex2d(0, 10);
      glVertex2d(5, 0);
      glEnd();

      gl_setcolor(bcolor);
      gl_arc(0, 12, 4, 0, 2 * M_PI, true);
      gl_setcolor(lcolor);
      gl_arc(0, 12, 4, 0, 2 * M_PI, false);
      break;

    case ChickenFoot1End:
      gl_setcolor(lcolor);
      glBegin(GL_LINE_STRIP);
      glVertex2d(-5, 0);
      glVertex2d(0, 10);
      glVertex2d(5, 0);
      glEnd();

      glBegin(GL_LINE_STRIP);
      glVertex2d(-5, 12);
      glVertex2d(5, 12);
      glEnd();
      break;

    case Cross0End:
      gl_setcolor(lcolor);
      glBegin(GL_LINE_STRIP);
      glVertex2d(-4, 6);
      glVertex2d(4, 6);
      glEnd();

      gl_setcolor(bcolor);
      gl_arc(0, 12, 4, 0, 2 * M_PI, true);
      gl_setcolor(lcolor);
      gl_arc(0, 12, 4, 0, 2 * M_PI, false);
      break;

    case Cross1End:
      gl_setcolor(lcolor);
      glBegin(GL_LINE_STRIP);
      glVertex2d(-4, 6);
      glVertex2d(4, 6);
      glEnd();

      glBegin(GL_LINE_STRIP);
      glVertex2d(-4, 10);
      glVertex2d(4, 10);
      glEnd();
      break;

    case HollowDiamondEnd: {
      Point vertices[] = {
        Point(0, 2), // 2 pixels distance for figure highlights/selection.
        Point(-4, 9), Point(0, 16), Point(4, 9),
      };
      gl_polygon(vertices, 4, lcolor, bcolor);
      break;
    }

    case FilledDiamondEnd: {
      // Same as hollow diamond. Do we really need a separate style for this?
      Point vertices[] = {
        Point(0, 2), // 2 pixels distance for figure highlights/selection.
        Point(-4, 9), Point(0, 16), Point(4, 9),
      };
      gl_polygon(vertices, 4, lcolor, bcolor);
      break;
    }

    case HollowCircleEnd:
      gl_setcolor(lcolor);
      gl_arc(0, 4, 4, 0, 2 * M_PI, false);
      break;

    case FilledCircleEnd:
      gl_setcolor(lcolor);
      gl_arc(0, 4, 4, 0, 2 * M_PI, true);
      break;

    case BoldStickEnd:
      gl_setcolor(lcolor);
      glLineWidth(3);
      glBegin(GL_LINE_STRIP);
      glVertex2d(0.5, 0.5);
      glVertex2d(0.5, 15.5);
      glEnd();

      glLineWidth(1);
      break;
  }
}
#endif

//--------------------------------------------------------------------------------------------------

void Line::stroke_outline(CairoCtx *cr, float offset) const {
  std::vector<SegmentPoint>::const_iterator pv, v = _segments.begin();

  if (v == _segments.end())
    return;

  cr->move_to(v->pos.x + 0.5, v->pos.y + 0.5);
  pv = v;
  while (++v != _segments.end()) {
    Point pos(v->pos.round());
    if (v->hop) {
      Point p(pos);
      double angle = -angle_of_line(pv->pos, v->pos);
      double rangle = angle * M_PI / 180;
      double dx, dy;

      dx = cos(rangle) * 5;
      dy = sin(rangle) * 5;

      p = p - Point(dx, dy).round();
      cr->line_to(p.x + 0.5, p.y + 0.5);

      cr->arc(pos.x, pos.y, 5, (angle + 180) * M_PI / 180, (angle)*M_PI / 180);

      p = p + Point(dx, dy);
      p = p + Point(dx, dy);
    } else
      cr->line_to(pos.x + 0.5, pos.y + 0.5);
    pv = v;
  }
}

//--------------------------------------------------------------------------------------------------

void Line::stroke_outline_gl(float offset) const {
#ifndef __APPLE__
  glBegin(GL_LINE_STRIP);

  std::vector<SegmentPoint>::const_iterator pv, v = _segments.begin();

  if (v == _segments.end())
    return;

  glVertex2d(v->pos.x, v->pos.y);
  pv = v;
  while (++v != _segments.end()) {
    Point pos(v->pos.round());
    if (v->hop) {
      Point p(pos);
      double angle = -angle_of_line(pv->pos, v->pos);
      double rangle = angle * M_PI / 180;
      double dx, dy;

      dx = cos(rangle) * 5;
      dy = sin(rangle) * 5;

      p = p - Point(dx, dy).round();
      glVertex2d(p.x, p.y);

      // cr->arc(pos.x, pos.y, 5, (angle+180)*M_PI/180, (angle)*M_PI/180);

      p = p + Point(dx, dy);
      p = p + Point(dx, dy);
    } else
      glVertex2d(pos.x, pos.y);
    pv = v;
  }

  glEnd();
#endif
}

//--------------------------------------------------------------------------------------------------

double Line::get_line_start_angle() {
  std::vector<Point>::const_iterator iter = _vertices.begin();
  ++iter;

  return angle_of_line(_vertices.front(), *iter);
}

//--------------------------------------------------------------------------------------------------

double Line::get_line_end_angle() {
  std::vector<Point>::const_reverse_iterator iter = _vertices.rbegin();
  ++iter;

  return angle_of_line(_vertices.back(), *iter);
}

//--------------------------------------------------------------------------------------------------

void Line::draw_line_ends(CairoCtx *cr) {
  cr->save();

  cr->translate(_segments.front().pos);
  cr->rotate(M_PI * (270 - get_line_start_angle()) / 180.0);
  draw_line_end(cr, _start_type, _pen_color, _fill_color);
  cr->restore();

  cr->save();

  cr->translate(_segments.back().pos);
  cr->rotate(M_PI * (270 - get_line_end_angle()) / 180.0);
  draw_line_end(cr, _end_type, _pen_color, _fill_color);

  cr->restore();
}

//--------------------------------------------------------------------------------------------------

void Line::draw_line_ends_gl() {
#ifndef __APPLE__
  glMatrixMode(GL_MODELVIEW);

  glPushMatrix();
  Point position = _segments.front().pos;
  glTranslated(position.x, position.y, 0);
  glRotated(270 - get_line_start_angle(), 0, 0, 1);
  draw_line_end_gl(_start_type, _pen_color, _fill_color);
  glPopMatrix();

  glPushMatrix();
  position = _segments.back().pos;
  glTranslated(position.x, position.y, 0);
  glRotated(270 - get_line_end_angle(), 0, 0, 1);
  draw_line_end_gl(_end_type, _pen_color, _fill_color);
  glPopMatrix();
#endif
}

//--------------------------------------------------------------------------------------------------

void Line::set_hops_crossings(bool flag) {
  _hop_crossings = flag;
}

void Line::set_end_type(LineEndType start, LineEndType end) {
  _start_type = start;
  _end_type = end;
  set_needs_render();
}

void Line::set_line_pattern(LinePatternType pattern) {
  _line_pattern = pattern;
  if (_line_pattern >= LastPattern)
    _line_pattern = SolidPattern;
}

void Line::set_vertices(const std::vector<Point> &points) {
  _vertices = points;

  update_bounds();
  set_needs_render();
}

void Line::add_vertex(const Point &pos) {
  _vertices.push_back(pos);

  update_bounds();
  set_needs_render();
}

void Line::set_vertex(size_t vertex, const Point &pos) {
  _vertices[vertex] = pos;

  update_bounds();
  set_needs_render();
}

void Line::mark_crossings(Line *line) {
  // nothing to cross with
  if (_segments.size() < 2)
    return;
  // can't cross with one pointed line
  // remove any hops we could have on it
  if (line->_segments.size() < 2) {
    size_t i = 0;
    while (i < _segments.size())
      if (_segments[i].hop == line)
        _segments.erase(_segments.begin() + i);
      else
        i++;
    return;
  }

  std::list<SegmentPoint> new_segs;

  // look for intersections between our segments and the other line's segments
  Point p1, p2;
  Point op1, op2;
  Point offset = line->get_root_position() - get_root_position();
  Point intersection;

  p1 = _segments.front().pos;
  new_segs.push_back(_segments.front());

  for (std::vector<SegmentPoint>::const_iterator segiter = _segments.begin() + 1; segiter != _segments.end();
       ++segiter) {
    if (segiter->hop) {
      // don't pick previous interceptions with given line
      if (segiter->hop != line)
        new_segs.push_back(*segiter);
      continue;
    }

    p2 = segiter->pos;
    op1 = line->_segments.front().pos + offset;
    for (std::vector<SegmentPoint>::const_iterator oiter = line->_segments.begin() + 1; oiter != line->_segments.end();
         ++oiter) {
      // intercept with whole line not with each hop to hop interval
      if (oiter->hop)
        continue;
      op2 = oiter->pos + offset;
      if (intersect_hv_lines(p1, p2, op1, op2, intersection)) {
        // got intersection with a straight line seg,
        // but we need to put new hop on its place
        // among rest of hops in this straingt segment
        std::list<SegmentPoint>::reverse_iterator rIt = new_segs.rbegin();
        while (rIt->hop && intersect_hv_lines(p1, rIt->pos, op1, op2, intersection) && (rIt != new_segs.rend()))
          ++rIt;
        new_segs.insert(rIt.base(), SegmentPoint(intersection, line));
      }
      op1 = op2;
    }
    p1 = p2;
    new_segs.push_back(*segiter);
  }

  if (new_segs.size() == _segments.size() && (std::equal(_segments.begin(), _segments.end(), new_segs.begin())))
    return; // no chnages

  _segments.clear();
  _segments.reserve(new_segs.size());
  _segments.insert(_segments.begin(), new_segs.begin(), new_segs.end());
  set_needs_render();
}

void Line::update_bounds() {
  if (_vertices.size() <= 1) {
    set_bounds(Rect());
  } else {
    double xmin = INFINITY, ymin = INFINITY, xmax = 0, ymax = 0;

    // find new boundaries
    for (std::vector<Point>::const_iterator v = _vertices.begin(); v != _vertices.end(); ++v) {
      xmin = std::min(v->x, xmin);
      ymin = std::min(v->y, ymin);
      xmax = std::max(v->x, xmax);
      ymax = std::max(v->y, ymax);
    }

    Point new_pos(xmin, ymin);

    // update the line's bounding box to the new bounds
    set_bounds(Rect(xmin, ymin, xmax - xmin, ymax - ymin));

    // update the segments. crossings should be handled by a listener of layout_changed
    _segments.clear();
    for (std::vector<Point>::const_iterator iter = _vertices.begin(); iter != _vertices.end(); ++iter) {
      _segments.push_back(SegmentPoint(*iter - new_pos, 0));
    }
  }

  update_handles();

  _layout_changed();
}

void Line::resize_to(const Size &size) {
  CanvasItem::resize_to(size);
}

void Line::move_to(const Point &pos) {
  CanvasItem::move_to(pos);
}

void Line::create_handles(InteractionLayer *ilayer) {
  if (_layouter) {
    _handles = _layouter->create_handles(this, ilayer);

    for (std::vector<ItemHandle *>::const_iterator iter = _handles.begin(); iter != _handles.end(); ++iter)
      ilayer->add_handle(*iter);
  }
}

void Line::update_handles() {
  if (_layouter)
    _layouter->update_handles(this, _handles);
}

bool Line::on_drag_handle(ItemHandle *handle, const Point &pos, bool dragging) {
  if (_layouter) {
    bool flag = _layouter->handle_dragged(this, handle, pos, dragging);
    if (flag) {
      update_layout();
      set_needs_render();
    }
    return flag;
  }
  return false;
}
