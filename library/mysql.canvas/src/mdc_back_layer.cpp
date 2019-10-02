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

#include "base/log.h"
#include "mdc_back_layer.h"
#include "mdc_algorithms.h"
#include "mdc_canvas_view.h"
#include "mdc_draw_util.h"

DEFAULT_LOG_DOMAIN("canvas")

using namespace mdc;
using namespace base;

//----------------------------------------------------------------------------------------------------------------------

BackLayer::BackLayer(CanvasView *view) : Layer(view) {
  _grid_visible = true;
  _paper_visible = true;

  _grid1_dl = 0;
  _grid2_dl = 0;

  set_color(Color::getSystemColor(base::TextBackgroundColor));
}

//----------------------------------------------------------------------------------------------------------------------

void BackLayer::set_color(const Color &color) {
  // Setting something else but a white background color produces problems with other elements, for instance connections.
  // Would have to draw them piecewise in different colors, depending on whether a part is over the background or a
  // layer (particularly important for dark backgrounds).
  _fill_color = base::Color::white(); //color;

  // Compute grid colors that play well with the background (which can be dark or light).
  if (_fill_color.brightness() < 0.5) {
    _line2_color = _fill_color.brighten(2.4f);
    _line1_color = _fill_color.brighten(3.6f);
  } else {
    _line2_color = _fill_color.darken(0.05f);
    _line1_color = _fill_color.darken(0.10f);
  }
}

//----------------------------------------------------------------------------------------------------------------------

BackLayer::~BackLayer() {
  if (_grid1_dl)
    glDeleteLists(_grid1_dl, 1);
  if (_grid2_dl)
    glDeleteLists(_grid2_dl, 1);
}

//----------------------------------------------------------------------------------------------------------------------

void BackLayer::render_grid(const Rect &bounds) {
  bool use_gl = _owner->has_gl();

  double gsize = _owner->_grid_size;
  double x, y;
  double left, top;
  double right, bottom;
  bool regen_display_lists = false;

  float jitter = 0.5f; // use_gl ? 0 : 0.5f; Also for GL it renders sharper lines with 0.5 offset.
  left = jitter;
  right = bounds.right() + jitter;

  top = jitter;
  bottom = bounds.bottom() + jitter;

  if (_grid1_dl == 0 || Point(left, top) != _grid_dl_start || _grid_dl_size != gsize || _grid_dl_area != bounds) {
    _grid_dl_start = Point(left, top);
    _grid_dl_size = gsize;
    _grid_dl_area = bounds;

    if (_grid1_dl == 0 && use_gl) {
      _grid1_dl = glGenLists(1);
      _grid2_dl = glGenLists(2);
    }
    regen_display_lists = true;
  }

  // Small grid.
  if (gsize * _owner->get_zoom() > 4) {
    if (use_gl) {
      if (regen_display_lists) {
        glNewList(_grid1_dl, GL_COMPILE);

        glDisable(GL_TEXTURE_2D);
        glColor4d(_line2_color.red, _line2_color.green, _line2_color.blue, _line2_color.alpha);

        glBegin(GL_LINES);
        for (x = left; x < right; x += gsize) {
          glVertex2d(x, top);
          glVertex2d(x, bottom);
        }
        glEnd();

        glBegin(GL_LINES);
        for (y = top; y < bottom; y += gsize) {
          glVertex2d(left, y);
          glVertex2d(right, y);
        }
        glEnd();

        glEndList();
      }

      glCallList(_grid1_dl);
    } else {
      CairoCtx *cr = _owner->cairoctx();

      cr->set_color(_line2_color);
      cr->set_line_width(1.0);

      for (x = left; x <= right; x += gsize) {
        cr->move_to(x, top);
        cr->line_to(x, bottom);
        cr->stroke();
      }

      for (y = top; y <= bottom; y += gsize) {
        cr->move_to(left, y);
        cr->line_to(right, y);
        cr->stroke();
      }
      cr->stroke();
    }
  }

  gsize *= 8;

  // Large grid.
  if (gsize * _owner->get_zoom() >= 10) {
    if (use_gl) {
      if (regen_display_lists) {
        glNewList(_grid2_dl, GL_COMPILE);

        glDisable(GL_TEXTURE_2D);
        glColor4d(_line1_color.red, _line1_color.green, _line1_color.blue, _line1_color.alpha);

        glBegin(GL_LINES);
        for (x = left; x < right; x += gsize) {
          glVertex2d(x, top);
          glVertex2d(x, bottom);
        }
        glEnd();
        glBegin(GL_LINES);
        for (y = top; y < bottom; y += gsize) {
          glVertex2d(left, y);
          glVertex2d(right, y);
        }
        glEnd();

        glEndList();
      }

      glCallList(_grid2_dl);
    } else {
      CairoCtx *cr = _owner->cairoctx();

      cr->set_color(_line1_color);

      for (x = left; x <= right; x += gsize) {
        cr->move_to(x, top);
        cr->line_to(x, bottom);
        cr->stroke();
      }
      for (y = top; y <= bottom; y += gsize) {
        cr->move_to(left, y);
        cr->line_to(right, y);
        cr->stroke();
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void BackLayer::render_page_borders(const Rect &bounds) {
  CairoCtx *cr = _owner->cairoctx();
  bool use_gl = _owner->has_gl();

  double x, y;
  double left, top;
  double right, bottom;
  Size psize = _owner->get_page_size();

  left = 0;
  right = floor(bounds.right());

  top = 0;
  bottom = floor(bounds.bottom());

  if (use_gl) {
    glColor4d(0.5, 0.5, 0.5, 1);
    glBegin(GL_LINES);
    for (x = left; x <= right; x += psize.width) {
      glVertex2d(x, top);
      glVertex2d(x, bottom);
    }
    glEnd();
    glBegin(GL_LINES);
    for (y = top; y <= bottom; y += psize.height) {
      glVertex2d(left, y);
      glVertex2d(right, y);
    }
    glEnd();
  } else {
    cr->set_color(Color(0.5, 0.5, 0.5));
    cr->set_line_width(2.0);

    for (x = left; x <= right; x += floor(psize.width)) {
      cr->move_to(x, top);
      cr->line_to(x, bottom);
    }

    for (y = top; y <= bottom; y += floor(psize.height)) {
      cr->move_to(left, y);
      cr->line_to(right, y);
    }
    cr->stroke();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void BackLayer::repaint(const Rect &aBounds) {
  Rect vrect = _owner->get_viewport();
  CairoCtx *cr = _owner->cairoctx();
  Size total_size = _owner->get_total_view_size();
  Size view_size = _owner->get_viewable_size();
  Point extra_offset = _owner->_extra_offset;
  bool use_gl = _owner->has_gl();

  Point offs;

#ifndef WIN32
  if (_owner->debug_enabled())
    logDebug3("repaint background %s", aBounds.str().c_str());
#endif
  cr->save();

  // If paper is smaller than window, then show the non-paper background.
  if (extra_offset.x > 0 || extra_offset.y > 0) {
    if (use_gl) {
      gl_setcolor(Color::getSystemColor(base::WindowBackgroundColor));

      // Sides.
      if (extra_offset.x > 0) {
        gl_rectangle(-extra_offset.x, aBounds.top(), extra_offset.x, view_size.height, true);
        gl_rectangle(view_size.width - 2 * extra_offset.x, aBounds.top(), extra_offset.x, view_size.height, true);
      }

      // Top/Bottom.
      if (extra_offset.y > 0) {
        gl_rectangle(aBounds.left(), -extra_offset.y, view_size.width, extra_offset.y, true);
        gl_rectangle(aBounds.left(), view_size.height - 2 * extra_offset.y, view_size.width, extra_offset.y, true);
      }

      //draw_shadow_gl(Rect(Point(), total_size), Color(0.60, 0.60, 0.60));
    } else {
      cr->save();

      cr->set_color(Color::getSystemColor(base::WindowBackgroundColor));

      // sides
      if (extra_offset.x > 0) {
        cr->rectangle(-extra_offset.x + vrect.left(), -extra_offset.y + vrect.top(), extra_offset.x, view_size.height);
        cr->rectangle(vrect.left() + view_size.width - 2 * extra_offset.x, -extra_offset.y + vrect.top(),
                      extra_offset.x, view_size.height);
      }
      // top/bottom
      if (extra_offset.y > 0) {
        cr->rectangle(-extra_offset.x + vrect.left(), -extra_offset.y + vrect.top(), view_size.width, extra_offset.y);
        cr->rectangle(-extra_offset.x + vrect.left(), view_size.height - 2 * extra_offset.y + vrect.top(),
                      view_size.width, extra_offset.y);
      }
      cr->fill();

      //draw_shadow(cr, Rect(Point(), total_size), Color(0.3, 0.3, 0.3));
      cr->restore();
    }
  }

  // draw contents only if needed
  if (bounds_intersect(aBounds, vrect)) {
    if (!use_gl) {
      // clip redrawing to the exposed area
      cr->rectangle(aBounds);
      cr->clip();
    }

    int x, y, w, h;
    _owner->canvas_to_window(Rect(0, 0, total_size.width, total_size.height), x, y, w, h);

    // Draw paper background.
    Rect paper(aBounds);

    if (paper.pos.x < 0)
      paper.pos.x = 0;
    if (paper.right() > vrect.right())
      paper.set_xmax(vrect.right());

    if (paper.pos.y < 0)
      paper.pos.y = 0;
    if (paper.bottom() > vrect.bottom())
      paper.set_ymax(vrect.bottom());

    if (use_gl) {
      gl_setcolor(_fill_color);
      gl_rectangle(paper, true);
    } else {
      cr->set_color(_fill_color);
      cr->rectangle(paper);
      cr->fill();
    }

    if (extra_offset.x > 0 || extra_offset.y > 0) {
      if (!use_gl) {
        cr->rectangle(vrect.pos.x, vrect.pos.y, extra_offset.x > 0 ? total_size.width : view_size.width,
                      extra_offset.y > 0 ? total_size.height : view_size.height);
        cr->clip();
      }
    }

    if (_grid_visible)
      render_grid(paper);

    if (_paper_visible)
      render_page_borders(paper);
  }

  cr->restore();
}

//----------------------------------------------------------------------------------------------------------------------

void BackLayer::set_grid_visible(bool flag) {
  _grid_visible = flag;
  queue_repaint();
}

//----------------------------------------------------------------------------------------------------------------------

void BackLayer::set_paper_visible(bool flag) {
  _paper_visible = flag;
  queue_repaint();
}

//----------------------------------------------------------------------------------------------------------------------
