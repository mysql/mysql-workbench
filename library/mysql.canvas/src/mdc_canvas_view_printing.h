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

#ifndef _MDC_CANVAS_VIEW_PRINTING_H_
#define _MDC_CANVAS_VIEW_PRINTING_H_

#include "mdc_canvas_public.h"
#include "mdc_common.h"

namespace base {
  class FileHandle;
};

namespace mdc {

  class CanvasView;

  enum PageOrientation { Landscape, Portrait };

  class MYSQLCANVAS_PUBLIC_FUNC CanvasViewExtras {
  public:
    CanvasViewExtras(CanvasView *view);

    void set_progress_callback(const std::function<void(int, int)> &progress);

    void enable_custom_layout();
    void set_show_print_guides(bool flag);

    void set_paper_size(double width, double height);
    void get_paper_size(double &width, double &height);

    void set_page_margins(double top, double left, double bottom, double right);
    void set_page_counts(Count xpages, Count ypages);

    void set_orientation(PageOrientation orientation);

    void set_print_border(bool flag);
    void set_print_page_numbers(bool flag);

    void set_scale(double scale);
    void set_scale(double xscale, double yscale);
    void set_scale_to_fit();

    void set_print_area(const base::Rect &area);

    PDFSurface *create_pdf_surface(base::FileHandle &fh);
    PSSurface *create_ps_surface(base::FileHandle &fh);
    int print_to_surface(Surface *surf, const std::string &header_text, const std::string &footer_text, int gpage_start,
                         int gtotal_pages);
    int print_to_pdf(const std::string &path);
    int print_to_ps(const std::string &path);

#ifdef _MSC_VER
    int print_native(HDC hdc, int width, int height, int page);
#endif

    int render_pages(CairoCtx *cr, double render_scale, int page = -1, bool rotate_for_landscape = false,
                     const std::string &header_text = "", const std::string &footer_text = "", int gpage_start = 0,
                     int gtotal_pages = 0);

    // final version
    void render_page(CairoCtx *cr, int x, int y);

    void get_page_counts(Count &xpages, Count &ypages);

  protected:
    base::Size get_adjusted_paper_size();
    base::Rect get_adjusted_printable_area();

    CanvasView *_view;

    std::function<void(int, int)> _progress_cb;

    double _page_width;  // in mm
    double _page_height; // in mm

    double _xscale;
    double _yscale;

    double _margin_left;
    double _margin_right;
    double _margin_top;
    double _margin_bottom;

    PageOrientation _orientation;
    bool _custom_layout;
    bool _print_border;
    bool _print_page_numbers;
  };

} // end of mdc namespace

#endif
