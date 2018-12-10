/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/file_utilities.h"

#include "mdc_canvas_view.h"
#include "mdc_canvas_view_printing.h"

#ifndef _MSC_VER
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#endif

using namespace mdc;
using namespace base;

CanvasViewExtras::CanvasViewExtras(CanvasView *view) : _view(view) {
  _custom_layout = false;
  _orientation = Portrait;
  _print_border = false;
  _print_page_numbers = false;

  _xscale = 1;
  _yscale = 1;

  _page_width = 0;
  _page_height = 0;

  _margin_top = 0;
  _margin_left = 0;
  _margin_bottom = 0;
  _margin_right = 0;
}

void CanvasViewExtras::set_progress_callback(const std::function<void(int, int)> &progress) {
  _progress_cb = progress;
}

void CanvasViewExtras::enable_custom_layout() {
}

void CanvasViewExtras::set_show_print_guides(bool flag) {
}

void CanvasViewExtras::get_page_counts(Count &xpages, Count &ypages) {
  _view->get_page_layout(xpages, ypages);
}

void CanvasViewExtras::set_paper_size(double width, double height) {
  _page_width = width;
  _page_height = height;
}

void CanvasViewExtras::get_paper_size(double &width, double &height) {
  width = _page_width;
  height = _page_height;
}

void CanvasViewExtras::set_page_margins(double top, double left, double bottom, double right) {
  _margin_top = top;
  _margin_bottom = bottom;
  _margin_left = left;
  _margin_right = right;
}

void CanvasViewExtras::set_orientation(PageOrientation orientation) {
  _orientation = orientation;
}

void CanvasViewExtras::set_print_border(bool flag) {
  _print_border = flag;
}

void CanvasViewExtras::set_print_page_numbers(bool flag) {
  _print_page_numbers = flag;
}

void CanvasViewExtras::set_scale(double scale) {
  _xscale = _yscale = scale;
}

void CanvasViewExtras::set_scale(double xscale, double yscale) {
  _xscale = xscale;
  _yscale = yscale;
}

void CanvasViewExtras::set_scale_to_fit() {
}

void CanvasViewExtras::set_print_area(const Rect &area) {
}

Size CanvasViewExtras::get_adjusted_paper_size() {
  Size size(_page_width, _page_height);

  // if (_orientation == Landscape)
  //  std::swap(size.width, size.height);

  return size;
}

Rect CanvasViewExtras::get_adjusted_printable_area() {
  Rect rect;

  rect.pos.x = _margin_left;
  rect.pos.y = _margin_top;
  rect.size = Size(_page_width, _page_height);

  rect.size.width -= _margin_left + _margin_right;
  rect.size.height -= _margin_top + _margin_bottom;

  /*
  if (_orientation == Landscape)
  {
    std::swap(rect.pos.x, rect.pos.y);
    std::swap(rect.size.width, rect.size.height);
  }*/

  return rect;
}

int CanvasViewExtras::print_to_pdf(const std::string &path) {
  Size paper_size = get_adjusted_paper_size();
  int count;

  _view->lock();

  // 1 pt = 1/72in, 1 mm = 0.039
  cairo_surface_t *surf;
  try {
    FileHandle fh(path.c_str(), "wb");
    surf = cairo_pdf_surface_create_for_stream(&write_to_surface, fh.file(), MM_TO_PT(paper_size.width),
                                               MM_TO_PT(paper_size.height));

    PDFSurface surface(surf);

    {
      Rect bounds;

      CairoCtx ctx(surface);
      ctx.check_state();

      count = render_pages(&ctx, MM_TO_PT(1), -1, true);

      ctx.check_state();
    }
  } catch (...) {
    _view->unlock();
    throw;
  }

  _view->unlock();
  return count;
}

PDFSurface *CanvasViewExtras::create_pdf_surface(FileHandle &fh) {
  Size paper_size = get_adjusted_paper_size();
  return new PDFSurface(cairo_pdf_surface_create_for_stream(&write_to_surface, fh.file(), MM_TO_PT(paper_size.width),
                                                            MM_TO_PT(paper_size.height)));
}

PSSurface *CanvasViewExtras::create_ps_surface(FileHandle &fh) {
  Size paper_size = get_adjusted_paper_size();
  return new PSSurface(cairo_ps_surface_create_for_stream(&write_to_surface, fh.file(), MM_TO_PT(paper_size.width),
                                                          MM_TO_PT(paper_size.height)));
}

int CanvasViewExtras::print_to_surface(Surface *surf, const std::string &header_text, const std::string &footer_text,
                                       int gpage_start, int gtotal_pages) {
  int count;

  _view->lock();

  // 1 pt = 1/72in, 1 mm = 0.039
  try {
    {
      Rect bounds;

      CairoCtx ctx(*surf);
      ctx.check_state();

      count = render_pages(&ctx, MM_TO_PT(1), -1, true, header_text, footer_text, gpage_start, gtotal_pages);

      ctx.check_state();
    }
  } catch (...) {
    _view->unlock();
    throw;
  }

  _view->unlock();
  return count;
}

int CanvasViewExtras::print_to_ps(const std::string &path) {
  Size paper_size = get_adjusted_paper_size();
  int count;

  _view->lock();

  cairo_surface_t *surf;
  try {
    FileHandle fh(path.c_str(), "wb");
    surf = cairo_ps_surface_create_for_stream(&write_to_surface, fh.file(), MM_TO_PT(paper_size.width),
                                              MM_TO_PT(paper_size.height));

    PSSurface surface(surf);

    {
      Rect bounds;

      CairoCtx ctx(surface);
      ctx.check_state();

      count = render_pages(&ctx, MM_TO_PT(1), -1, true);

      ctx.check_state();
    }
  } catch (...) {
    _view->unlock();
    throw;
  }
  _view->unlock();
  return count;
}

#ifdef _MSC_VER
int CanvasViewExtras::print_native(HDC hdc, int paper_width, int paper_height, int page) {
  int count;

  _view->lock();

  try {
    Win32Surface surface(hdc, true);

    {
      Rect bounds;

      CairoCtx ctx(surface);
      ctx.check_state();

      ctx.set_color(Color::white());
      ctx.paint();

      double scale = (paper_width / _page_width);

      count = render_pages(&ctx, scale * _xscale, page, false);

      ctx.check_state();
    }
  } catch (...) {
    _view->unlock();
    throw;
  }
  _view->unlock();
  return count;
}
#endif

int CanvasViewExtras::render_pages(CairoCtx *cr, double render_scale, int page, bool rotate_for_landscape,
                                   const std::string &header_text, const std::string &footer_text, int gpage_start,
                                   int gtotal_pages) {
  Size paper_size = get_adjusted_paper_size();
  Rect content_area = get_adjusted_printable_area();
  Count xc, yc;
  Rect bounds;
  int count = 0, printed = 0;
  mdc::FontSpec header_font(_view->get_default_font());

  _view->get_page_layout(xc, yc);

  bounds.size = content_area.size;
  bounds.size.width = bounds.size.width * _xscale;
  bounds.size.height = bounds.size.height * _yscale;

  if (_orientation == Landscape)
    std::swap(bounds.size.width, bounds.size.height);

  for (Count y = 0; y < yc; ++y) {
    bounds.pos.x = 0;
    for (Count x = 0; x < xc; ++x) {
      if (page < 0 || page == count) {
        cr->save();

        if (_orientation == Landscape && rotate_for_landscape) {
          cr->translate(render_scale * paper_size.width / 2, render_scale * paper_size.height / 2);
          cr->rotate(90 * M_PI / 180);
          cr->translate(-render_scale * paper_size.height / 2, -render_scale * paper_size.width / 2);

          cr->translate(render_scale * content_area.left(), render_scale * content_area.top());

          cr->scale(render_scale / _xscale, render_scale / _yscale);
        } else {
          cr->scale(render_scale / _xscale, render_scale / _yscale);
          cr->translate(render_scale * content_area.left(), render_scale * content_area.top());
        }
        if (!header_text.empty()) {
          _view->set_printout_mode(true);
          cr->save();
          cr->set_font(header_font);
          cr->set_color(base::Color::black());
          std::string text = header_text;
          base::replaceStringInplace(text, "$page", base::strfmt("%i", printed + 1));
          base::replaceStringInplace(text, "$total_pages", base::strfmt("%i", page < 0 ? xc * yc : 1));
          base::replaceStringInplace(text, "$doc_page", base::strfmt("%i", gpage_start + printed + 1));
          base::replaceStringInplace(text, "$doc_total_pages", base::strfmt("%i", gtotal_pages));
          cairo_text_extents_t extents;
          cr->get_text_extents(header_font, text, extents);
          cr->move_to(5, 5 + extents.height + extents.y_bearing);
          cr->show_text(text);
          cr->restore();
          _view->set_printout_mode(false);
        }
        if (!footer_text.empty()) {
          _view->set_printout_mode(true);
          cr->save();
          cr->set_font(header_font);
          cr->set_color(base::Color::black());
          std::string text = footer_text;
          base::replaceStringInplace(text, "$page", base::strfmt("%i", printed + 1));
          base::replaceStringInplace(text, "$total_pages", base::strfmt("%i", page < 0 ? xc * yc : 1));
          base::replaceStringInplace(text, "$doc_page", base::strfmt("%i", gpage_start + printed + 1));
          base::replaceStringInplace(text, "$doc_total_pages", base::strfmt("%i", gtotal_pages));
          cairo_text_extents_t extents;
          cr->get_text_extents(header_font, text, extents);
          cr->move_to(5, bounds.bottom() - (5 + extents.height + extents.y_bearing));
          cr->show_text(text);
          cr->restore();
          _view->set_printout_mode(false);
        }
        _view->render_for_export(bounds, cr);
        if (_print_border) {
          cr->set_color(Color(0.5, 0.5, 0.5));
          cr->set_line_width(1);
          cr->rectangle(0, 0, bounds.width(), bounds.height());
          cr->stroke();
        }
        cr->check_state();
        cr->show_page();
        cr->check_state();
        cr->restore();
        ++printed;

        if (_progress_cb)
          _progress_cb(x, y);
      }
      ++count;

      bounds.pos.x += bounds.width();
    }
    bounds.pos.y += bounds.height();
  }

  return printed;
}

// used in macosx (and linux)
void CanvasViewExtras::render_page(CairoCtx *cr, int x, int y) {
  Rect content_area = get_adjusted_printable_area();
  Rect bounds;

  bounds.pos.x += content_area.width() * x;
  bounds.pos.y += content_area.height() * y;

  if (_orientation == Landscape)
    std::swap(content_area.size.width, content_area.size.height);

  bounds.size = content_area.size;

  _view->set_printout_mode(true);

  cr->save();
  cr->scale(_xscale, _yscale);
  cr->translate(content_area.left(), content_area.top());
  _view->render_for_export(bounds, cr);

  cr->restore();

  if (_print_border) {
    cr->save();
    cr->scale(_xscale, _yscale);
    cr->set_color(Color(0.5, 0.5, 0.5));
    cr->set_line_width(0.1);
    cr->rectangle(content_area.left(), content_area.top(), content_area.width(), content_area.height());
    cr->stroke();
    cr->restore();
  }

  // needed in mac but not in linux
  //  cr->show_page();

  _view->set_printout_mode(false);
}
