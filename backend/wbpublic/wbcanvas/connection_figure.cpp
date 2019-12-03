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

#include "connection_figure.h"
#include "table_figure.h"
#include "mdc_box_side_magnet.h"

using namespace std;
using namespace wbfig;
using namespace mdc;
using namespace base;

#define SPLIT_LINE_LENGTH 30

ConnectionLineLayouter::ConnectionLineLayouter(mdc::Connector *sconn, mdc::Connector *econn)
  : mdc::OrthogonalLineLayouter(sconn, econn) {
  _type = NormalLine;
}

std::vector<mdc::ItemHandle *> ConnectionLineLayouter::create_handles(mdc::Line *line, mdc::InteractionLayer *ilayer) {
  if (_type == ZLine)
    return mdc::LineLayouter::create_handles(line, ilayer);

  return super::create_handles(line, ilayer);
}

bool ConnectionLineLayouter::handle_dragged(mdc::Line *line, mdc::ItemHandle *handle, const Point &pos, bool dragging) {
  /*
  if (handle->get_tag() == 1)
  {
  }
*/
  return super::handle_dragged(line, handle, pos, dragging);
}

static void set_connector_side(mdc::BoxSideMagnet *magnet, mdc::Connector *conn, double angle) {
  if (magnet == NULL)
    return;

  if (angle == 0)
    magnet->set_connector_side(conn, mdc::BoxSideMagnet::Right);
  else if (angle == 180)
    magnet->set_connector_side(conn, mdc::BoxSideMagnet::Left);
  else if (angle == 90)
    magnet->set_connector_side(conn, mdc::BoxSideMagnet::Top);
  else
    magnet->set_connector_side(conn, mdc::BoxSideMagnet::Bottom);
}

bool ConnectionLineLayouter::update_start_point() {
  mdc::CanvasItem *start_item = _linfo.start_connector()->get_connected_item();

  if (_type == ZLine) {
    if (start_item) {
    }
  } else if (start_item && dynamic_cast<mdc::BoxSideMagnet *>(_linfo.start_connector()->get_connected_magnet())) {
    double angle;

    angle =
      angle_of_intersection_with_rect(start_item->get_root_bounds(), _linfo.subline_end_point(_linfo.start_subline()));

    set_connector_side(dynamic_cast<mdc::BoxSideMagnet *>(_linfo.start_connector()->get_connected_magnet()),
                       _linfo.start_connector(), angle);
  }
  return super::update_start_point();
}

bool ConnectionLineLayouter::update_end_point() {
  mdc::CanvasItem *end_item = _linfo.end_connector()->get_connected_item();

  if (_type == ZLine) {
    if (end_item) {
    }
  } else if (end_item && dynamic_cast<mdc::BoxSideMagnet *>(_linfo.end_connector()->get_connected_magnet())) {
    double angle;

    angle =
      angle_of_intersection_with_rect(end_item->get_root_bounds(), _linfo.subline_start_point(_linfo.end_subline()));

    set_connector_side(dynamic_cast<mdc::BoxSideMagnet *>(_linfo.end_connector()->get_connected_magnet()),
                       _linfo.end_connector(), angle);
  }

  return super::update_end_point();
}

std::vector<Point> ConnectionLineLayouter::get_points_for_subline(int subline) {
  if (_type == ZLine) {
    if (subline == _linfo.start_subline()) {
      std::vector<Point> points;
      Point p;

      p = _linfo.subline_start_point(subline).round();
      points.push_back(p);
      if (_linfo.subline_start_angle(subline) == 0)
        points.push_back(Point(p.x + 15, p.y));
      else
        points.push_back(Point(p.x - 15, p.y));

      p = _linfo.subline_end_point(subline).round();
      if (_linfo.subline_end_angle(subline) == 0)
        points.push_back(Point(p.x + 15, p.y));
      else
        points.push_back(Point(p.x - 15, p.y));
      points.push_back(p);

      return points;
    }
  }

  return super::get_points_for_subline(subline);
}

void ConnectionLineLayouter::set_type(ConnectionLineLayouter::Type type) {
  _type = type;
  update();
}

//---------------------------------------------------------------------------------------------------

Connection::Connection(mdc::Layer *layer, FigureEventHub *hub, model_Object *represented_object)
  : mdc::Line(layer), _represented_object(represented_object), _hub(hub) {
  set_cache_toplevel_contents(false);
  set_accepts_selection(true);
  set_draggable(false);

  _split = false;
  _center_captions = false;

  _start_dashed = false;
  _end_dashed = false;

  _diamond = None;

  _start_figure = 0;
  _end_figure = 0;

  set_pen_color(base::Color::black());
  set_fill_color(base::Color::white());
}

void Connection::set_start_figure(mdc::CanvasItem *item) {
  _start_figure = item;

  update_layouter();
}

void Connection::set_end_figure(mdc::CanvasItem *item) {
  _end_figure = item;

  update_layouter();
}

void Connection::update_layouter() {
  if (_start_figure && _end_figure) {
    if (!get_layouter()) {
      mdc::Connector *sc, *ec;
      TableColumnItem *item;

      sc = new mdc::Connector(this);
      sc->set_draggable(false);
      item = dynamic_cast<TableColumnItem *>(_start_figure);
      if (item)
        sc->connect(item->get_item_magnet());
      else {
        wbfig::Table *table = dynamic_cast<wbfig::Table *>(_start_figure);
        sc->connect(table->get_sides_magnet());
      }
      ec = new mdc::Connector(this);
      ec->set_draggable(false);
      item = dynamic_cast<TableColumnItem *>(_end_figure);
      if (item)
        ec->connect(item->get_item_magnet());
      else {
        wbfig::Table *table = dynamic_cast<wbfig::Table *>(_end_figure);
        if (table != NULL)
          ec->connect(table->get_sides_magnet());
      }

      ConnectionLineLayouter *layouter = new ConnectionLineLayouter(sc, ec);
      set_layouter(layouter);
    } else
      get_layouter()->update();
  }
}

void Connection::set_diamond_type(DiamondType type) {
  _diamond = type;
  set_needs_render();
}

void Connection::set_splitted(bool flag) {
  _split = flag;
  set_needs_render();
}

double Connection::get_segment_offset(int subline) {
  ConnectionLineLayouter *l = dynamic_cast<ConnectionLineLayouter *>(get_layouter());
  if (l)
    return l->get_segment_offset(subline);
  return 0.0;
}

void Connection::set_segment_offset(int subline, double offset) {
  ConnectionLineLayouter *l = dynamic_cast<ConnectionLineLayouter *>(get_layouter());
  if (l)
    l->set_segment_offset(subline, offset);
}

bool Connection::contains_point(const Point &point) const {
  if (!super::contains_point(point))
    return false;

  if (_split) {
    Point v1, v2;
    Rect bounds;

    v1 = convert_point_to(_segments[0].pos, get_parent());
    v2 = convert_point_to(_segments[1].pos, get_parent());
    points_reorder(v1, v2);

    if (v2.y == v1.y) {
      v1.y -= 2;
      v2.y += 2;
      if (v2.x > v1.x)
        v2.x = v1.x + 20;
      else
        v2.x = v1.x - 20;
      bounds = Rect(v1, v2);
    } else {
      v1.x -= 2;
      v2.x += 2;
      if (v2.y > v1.y)
        v2.y = v1.y + 20;
      else
        v2.y = v1.y - 20;
      bounds = Rect(v1, v2);
    }

    if (bounds_contain_point(bounds, point.x, point.y))
      return true;

    v1 = convert_point_to(_segments[_segments.size() - 1].pos, get_parent());
    v2 = convert_point_to(_segments[_segments.size() - 2].pos, get_parent());
    points_reorder(v1, v2);

    if (v2.y == v1.y) {
      v1.y -= 2;
      v2.y += 2;
      if (v2.x > v1.x)
        v2.x = v1.x + 20;
      else
        v2.x = v1.x - 20;
      bounds = Rect(v1, v2);
    } else {
      v1.x -= 2;
      v2.x += 2;
      if (v2.y > v1.y)
        v2.y = v1.y + 20;
      else
        v2.y = v1.y - 20;
      bounds = Rect(v1, v2);
    }

    if (bounds_contain_point(bounds, point.x, point.y))
      return true;
    return false;
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void Connection::stroke_outline(mdc::CairoCtx *cr, float offset) const {
  if (!_split || _segments.size() < 2)
    mdc::Line::stroke_outline(cr, offset);
  else // Draw a split connection (only show start and end).
  {
    Point v1, v2;
    {
      v1 = _segments[0].pos;
      v2 = _segments[1].pos;

      if (v2.y == v1.y) {
        cr->move_to(v1);
        if (v2.x > v1.x)
          cr->line_to(v1.x + 20, v2.y);
        else
          cr->line_to(v1.x - 20, v2.y);
      } else {
        cr->move_to(v1);
        if (v2.y > v1.y)
          cr->line_to(v1.x, v1.y + 20);
        else
          cr->line_to(v1.x, v1.y - 20);
      }
    }

    {
      v1 = _segments[_segments.size() - 1].pos;
      v2 = _segments[_segments.size() - 2].pos;

      if (v2.y == v1.y) {
        cr->move_to(v1);
        if (v2.x > v1.x)
          cr->line_to(v1.x + 20, v2.y);
        else
          cr->line_to(v1.x - 20, v2.y);
      } else {
        cr->move_to(v1);
        if (v2.y > v1.y)
          cr->line_to(v1.x, v1.y + 20);
        else
          cr->line_to(v1.x, v1.y - 20);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Connection::stroke_outline_gl(float offset) const {
#ifndef __APPLE__
  if (!_split || _segments.size() < 2)
    mdc::Line::stroke_outline_gl(offset);
  else {
    // Draw a split connection (only show start and end).
    glTranslated(offset, offset, 0);
    glBegin(GL_LINES);

    Point v1, v2;

    // Begin of the connection.
    v1 = _segments[0].pos;
    v2 = _segments[1].pos;

    if (v2.y == v1.y) {
      glVertex2d(v1.x, v1.y);
      if (v2.x > v1.x)
        glVertex2d(v1.x + 20 + offset, v2.y);
      else
        glVertex2d(v1.x - 20, v2.y);
    } else {
      glVertex2d(v1.x, v1.y);
      if (v2.y > v1.y)
        glVertex2d(v1.x, v1.y + 20);
      else
        glVertex2d(v1.x, v1.y - 20);
    }

    // End of the connection.
    v1 = _segments[_segments.size() - 1].pos;
    v2 = _segments[_segments.size() - 2].pos;

    if (v2.y == v1.y) {
      glVertex2d(v1.x, v1.y);
      if (v2.x > v1.x)
        glVertex2d(v1.x + 20, v2.y);
      else
        glVertex2d(v1.x - 20, v2.y);
    } else {
      glVertex2d(v1.x, v1.y);
      if (v2.y > v1.y)
        glVertex2d(v1.x, v1.y + 20);
      else
        glVertex2d(v1.x, v1.y - 20);
    }

    glEnd();
  }
#endif
}

//--------------------------------------------------------------------------------------------------

Point Connection::get_middle_caption_pos(const Size &size, CaptionPos pos) {
  if (_segments.size() >= 2) {
    Point midpoint;
    Point p1, p2;

    if (_segments.size() == 2) {
      p1 = _segments.front().pos;
      p2 = _segments.back().pos;

      if (fabs(p1.x - p2.x) < fabs(p1.y - p2.y)) // vertical
      {
        midpoint.y = (p1.y + p2.y) / 2 - size.height / 2;
        if (pos == Above && !_center_captions)
          midpoint.x = (p1.x + p2.x) / 2 - size.width - 10;
        else if (pos == Below && !_center_captions)
          midpoint.x = (p1.x + p2.x) / 2 + 10;
        else
          midpoint.x = (p1.x + p2.x) / 2;
      } else {
        if (!_center_captions)
          midpoint.x = (p1.x + p2.x) / 2 - size.width / 2;
        else
          midpoint.x = (p1.x + p2.x) / 2;

        if (pos == Above)
          midpoint.y = (p1.y + p2.y) / 2 - size.height - 10;
        else if (pos == Below)
          midpoint.y = (p1.y + p2.y) / 2 + 10;
        else
          midpoint.y = (p1.y + p2.y) / 2;
      }
    } else if ((_segments.size() & 1) == 1) {
      size_t i = _segments.size() / 2;
      std::vector<SegmentPoint>::const_iterator iter = _segments.begin();
      while (i-- > 0)
        ++iter;
      midpoint = iter->pos;

      // XXX
      //      throw std::logic_error("not handled yet");
    } else {
      size_t i = _segments.size() / 2;

      std::vector<SegmentPoint>::const_iterator iter = _segments.begin();

      while (--i > 0)
        ++iter;
      p1 = iter->pos;
      ++iter;
      p2 = iter->pos;
      midpoint = (p1 + p2);
      midpoint.x /= 2;
      midpoint.y /= 2;

      if (fabs(p1.x - p2.x) <= fabs(p1.y - p2.y) && !_center_captions) // verticalish line
      {
        if (fabs(p1.y - p2.y) < (size.height + 2)) {
          if (pos == Above)
            midpoint.y = min(p1.y, p2.y) - 10 - size.height;
          else if (pos == Below)
            midpoint.y = max(p1.y, p2.y) + 10;

          midpoint.x -= size.width / 2;
        } else {
          if (pos == Above)
            midpoint.x -= size.width + 10;
          else if (pos == Below)
            midpoint.x += 10;

          midpoint.y -= size.height / 2;
        }
      } else // horizontal-ish line
      {
        if (fabs(p1.x - p2.x) < (size.width + 2) && !_center_captions) {
          if (pos == Above)
            midpoint.x = min(p1.x, p2.x) - 10 - size.width;
          else if (pos == Below)
            midpoint.x = max(p1.x, p2.x) + 10;

          midpoint.y -= size.height / 2;
        } else {
          if (pos == Above)
            midpoint.y -= size.height + 10;
          else if (pos == Below)
            midpoint.y += 10;

          midpoint.x -= size.width / 2;
        }
      }
    }

    return convert_point_to(midpoint, 0);
  }
  return get_position();
}

double Connection::get_middle_segment_angle() {
  if (_segments.size() == 2)
    return mdc::angle_of_line(_segments.front().pos, _segments.back().pos);
  else if (_segments.size() > 2) {
    size_t i = _segments.size() / 2;

    std::vector<SegmentPoint>::const_iterator iter = _segments.begin();

    while (--i > 0)
      ++iter;

    Point pos = iter->pos;
    ++iter;

    return mdc::angle_of_line(pos, iter->pos);
  }
  return 0.0;
}

Point Connection::get_start_caption_pos(const Size &size) {
  Point p = _segments.front().pos;
  Point next_point = (++_segments.begin())->pos;

  if (fabs(next_point.x - p.x) <= fabs(next_point.y - p.y)) {
    if (next_point.y < p.y)
      p.y -= size.height + 10;
    else
      p.y += 10;
    p.x -= size.width + 5;
  } else {
    p.y -= size.height + 5;
    if (next_point.x < p.x)
      p.x -= size.width + 10;
    else
      p.x += 10;
  }

  return convert_point_to(p, 0);
}

Point Connection::get_end_caption_pos(const Size &size) {
  Point p = _segments.back().pos;
  Point next_point = (++_segments.rbegin())->pos;

  if (fabs(next_point.x - p.x) <= fabs(next_point.y - p.y)) {
    if (next_point.y < p.y)
      p.y -= size.height + 10;
    else
      p.y += 10;
    p.x -= size.width + 5;
  } else {
    p.y -= size.height + 5;
    if (next_point.x < p.x)
      p.x -= size.width + 10;
    else
      p.x += 10;
  }

  return convert_point_to(p, 0);
}

void Connection::set_start_dashed(bool flag) {
  _start_dashed = flag;
}

void Connection::set_end_dashed(bool flag) {
  _end_dashed = flag;
}

void Connection::set_center_captions(bool flag) {
  _center_captions = flag;
}

//--------------------------------------------------------------------------------------------------

void Connection::render(mdc::CairoCtx *cr) {
  if (_segments.empty())
    return;

  draw_state(cr);

  cr->translate(get_position());

  cr->set_line_width(_line_width);
  cr->set_color(_pen_color);

  stroke_outline(cr);
  set_line_pattern(cr, _line_pattern);
  cr->stroke();

  cr->set_dash(0, 0, 0);

  draw_line_ends(cr);

  cr->save();
  cr->translate(get_middle_caption_pos(Size(1, 1), Middle) - get_position());

  double angle = get_middle_segment_angle();

  // for some reason the angle gets flipped in some cases
  if (angle == 90 || angle == 270)
    angle -= 180;

  cr->rotate(M_PI * angle / 180.0);
  switch (_diamond) {
    case None:
      break;
    case Filled:
      cr->new_path();
      cr->move_to(0, -10);
      cr->line_to(10, 0);
      cr->line_to(0, 10);
      cr->line_to(-10, 0);
      cr->line_to(0, -10);
      cr->close_path();
      cr->set_color(Color::black());
      cr->fill();
      break;
    case LeftEmpty:
      cr->new_path();
      cr->move_to(0, -10);
      cr->line_to(10, 0);
      cr->line_to(0, 10);
      cr->line_to(-10, 0);
      cr->line_to(0, -10);
      cr->close_path();
      cr->set_color(Color::white());
      cr->fill_preserve();
      cr->set_color(Color::black());
      cr->stroke();
      cr->new_path();
      cr->move_to(0, -10);
      cr->line_to(10, 0);
      cr->line_to(0, 10);
      cr->line_to(0, -10);
      cr->close_path();
      cr->fill();
      break;
    case RightEmpty:
      cr->new_path();
      cr->move_to(0, -10);
      cr->line_to(10, 0);
      cr->line_to(0, 10);
      cr->line_to(-10, 0);
      cr->line_to(0, -10);
      cr->close_path();
      cr->set_color(Color::white());
      cr->fill_preserve();
      cr->set_color(Color::black());
      cr->stroke();
      cr->new_path();
      cr->move_to(0, -10);
      cr->line_to(-10, 0);
      cr->line_to(0, 10);
      cr->line_to(0, -10);
      cr->close_path();
      cr->fill();
      break;
    case Empty:
      cr->new_path();
      cr->move_to(0, -10);
      cr->line_to(10, 0);
      cr->line_to(0, 10);
      cr->line_to(-10, 0);
      cr->line_to(0, -10);
      cr->close_path();
      cr->set_color(Color::white());
      cr->fill_preserve();
      cr->set_color(Color::black());
      cr->stroke();
      break;
  }
  cr->restore();
}

//--------------------------------------------------------------------------------------------------

void Connection::render_gl(mdc::CairoCtx *cr) {
#ifndef __APPLE__
  if (_segments.empty())
    return;

  // sanity/debugging check
  if (_content_cache)
    throw std::logic_error("Connection figure is caching its contents in OpenGL mode.");

  draw_state_gl();

  glPushMatrix();
  glTranslated(get_position().x, get_position().y, 0);
  glLineWidth(_line_width);
  glEnable(GL_LINE_SMOOTH);
  gl_setcolor(_pen_color);

  GLushort gl_pattern = get_gl_pattern(_line_pattern);
  if (gl_pattern == 0xFFFF)
    glDisable(GL_LINE_STIPPLE);
  else {
    glLineStipple(1, gl_pattern);
    glEnable(GL_LINE_STIPPLE);
  }
  stroke_outline_gl();

  glDisable(GL_LINE_STIPPLE);
  draw_line_ends_gl();

  Point effective_position = get_middle_caption_pos(Size(1, 1), Middle) - get_position();
  glTranslated(effective_position.x, effective_position.y, 0);

  double angle = get_middle_segment_angle();

  // For some reason the angle gets flipped in some cases.
  if (angle == 90 || angle == 270)
    angle -= 180;

  glRotated(angle, 0, 0, 1);
  switch (_diamond) {
    case None:
      break;
    case Filled: {
      Point vertices[] = {
        Point(0, -10), Point(10, 0), Point(0, 10), Point(-10, 0), Point(0, -10),
      };
      gl_setcolor(Color::black());
      gl_polygon(vertices, 5, true);
      break;
    }

    case LeftEmpty: {
      Point vertices[] = {
        Point(0, -10), Point(10, 0), Point(0, 10), Point(-10, 0), Point(0, -10),
      };
      gl_setcolor(Color::black());
      gl_polygon(vertices, 5, Color::black(), Color::white());
    }

      {
        Point vertices[] = {
          Point(0, -10), Point(10, 0), Point(0, 10), Point(0, -10),
        };
        gl_polygon(vertices, 4, Color::black(), Color::white());
      }
      break;

    case RightEmpty: {
      Point vertices[] = {
        Point(0, -10), Point(10, 0), Point(0, 10), Point(-10, 0), Point(0, -10),
      };
      gl_polygon(vertices, 5, Color::black(), Color::white());
    }

      {
        Point vertices[] = {
          Point(0, -10), Point(10, 0), Point(0, 10), Point(0, -10),
        };
        gl_setcolor(Color::black());
        gl_polygon(vertices, 4, true);
      }

      break;
    case Empty: {
      Point vertices[] = {
        Point(0, -10), Point(10, 0), Point(0, 10), Point(-10, 0), Point(0, -10),
      };
      gl_polygon(vertices, 5, Color::black(), Color::white());
    } break;
  }

  glPopMatrix();
#endif
}

//--------------------------------------------------------------------------------------------------

bool Connection::on_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button, mdc::EventState state) {
  if (!_hub->figure_click(_represented_object, target, point, button, state))
    return super::on_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool Connection::on_double_click(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (!_hub->figure_double_click(_represented_object, target, point, button, state))
    return super::on_double_click(target, point, button, state);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool Connection::on_enter(mdc::CanvasItem *target, const Point &point) {
  if (!_hub->figure_enter(_represented_object, target, point))
    return super::on_enter(target, point);
  return false;
}

bool Connection::on_leave(mdc::CanvasItem *target, const Point &point) {
  if (!_hub->figure_leave(_represented_object, target, point))
    return super::on_leave(target, point);
  return false;
}

bool Connection::on_button_press(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                 mdc::EventState state) {
  if (!_hub->figure_button_press(_represented_object, target, point, button, state))
    return super::on_button_press(target, point, button, state);
  return false;
}

bool Connection::on_button_release(mdc::CanvasItem *target, const Point &point, mdc::MouseButton button,
                                   mdc::EventState state) {
  if (!_hub->figure_button_release(_represented_object, target, point, button, state))
    return super::on_button_release(target, point, button, state);
  return false;
}

void Connection::mark_crossings(mdc::Line *line) {
  ConnectionLineLayouter *cLineLayouter = dynamic_cast<ConnectionLineLayouter *>(_layouter);
  if (cLineLayouter != NULL && cLineLayouter->get_type() == ConnectionLineLayouter::ZLine)
    return;

  if (_split || !_visible)
    return;

  Connection *conn = dynamic_cast<Connection *>(line);
  if (conn != NULL && conn->_split)
    return;

  super::mark_crossings(line);
}
