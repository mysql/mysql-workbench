/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"

#include "wb_printing.h"
#include "mdc_canvas_view_printing.h"
#include "wbcanvas/model_diagram_impl.h"

#include "grts/structs.workbench.h"

//--------------------------------------------------------------------------------------------------

int wbprint::getPageCount(model_DiagramRef view) {
  mdc::Count xc, yc;
  view->get_data()->get_canvas_view()->get_page_layout(xc, yc);

  return xc * yc;
}

//--------------------------------------------------------------------------------------------------

void wbprint::getPageLayout(model_DiagramRef view, int &xpages, int &ypages) {
  mdc::Count xc, yc;

  view->get_data()->get_canvas_view()->get_page_layout(xc, yc);
  xpages = xc;
  ypages = yc;
}

//--------------------------------------------------------------------------------------------------

app_PageSettingsRef wbprint::getPageSettings(model_DiagramRef diagram) {
  return workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"))->pageSettings();
}

//--------------------------------------------------------------------------------------------------

#ifdef _MSC_VER

int wbprint::printPageHDC(model_DiagramRef view, int pagenum, HDC hdc, int width, int height) {
  mdc::CanvasViewExtras extras(view->get_data()->get_canvas_view());

  app_PageSettingsRef page(workbench_DocumentRef::cast_from(grt::GRT::get()->get("/wb/doc"))->pageSettings());

  // TODO: we have both paper and page margins, which is kinda confusing.
  extras.set_page_margins(page->marginTop(), page->marginLeft(), page->marginBottom(), page->marginRight());
  extras.set_paper_size(page->paperType()->width(), page->paperType()->height());
  extras.set_orientation(page->orientation() == "landscape" ? mdc::Landscape : mdc::Portrait);
  extras.set_scale(page->scale());
  // extras.set_print_border(true);

  int pages = extras.print_native(hdc, width, height, pagenum);

  return pages;
}

#endif

//--------------------------------------------------------------------------------------------------
