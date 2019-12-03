/*
 * Copyright (c) 2013, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "mforms/app.h"

#include "about_box.h"

#include "wb_context.h"
#include "wb_version.h"

using namespace wb;

#ifdef __APPLE__
#define ABOUT_NORMAL_FONT "Helvetica Neue"
#define ABOUT_FONT_SIZE 11
#elif defined(_MSC_VER)
#define ABOUT_NORMAL_FONT "Tahoma"
#define ABOUT_FONT_SIZE 10
#else
#define ABOUT_NORMAL_FONT "Tahoma"
#define ABOUT_FONT_SIZE 10
#endif

//--------------------------------------------------------------------------------------------------

// The following helpers are just temporary. They will be replaced by a cairo context class.
static void delete_surface(cairo_surface_t *surface) {
  if (surface != NULL)
    cairo_surface_destroy(surface);
}

//--------------------------------------------------------------------------------------------------

static int image_width(cairo_surface_t *image) {
  if (image != NULL)
    return cairo_image_surface_get_width(image);
  return 0;
}

//--------------------------------------------------------------------------------------------------

static int image_height(cairo_surface_t *image) {
  if (image != NULL)
    return cairo_image_surface_get_height(image);
  return 0;
}

//--------------------------------------------------------------------------------------------------

AboutBox::AboutBox(const std::string &edition) : Popup(mforms::PopupPlain), _edition(edition) {
  base::Size size;
  _scale_factor = mforms::App::get()->backing_scale_factor();
  if (_scale_factor > 1) {
    _back_image = mforms::Utilities::load_icon("MySQL-WB-about-screen@2x.png");
    size = base::Size(image_width(_back_image) / _scale_factor, image_height(_back_image) / _scale_factor);
  } else {
    _back_image = mforms::Utilities::load_icon("MySQL-WB-about-screen.png");
    size = base::Size(image_width(_back_image), image_height(_back_image));
  }

  set_size((int)size.width, (int)size.height);
  base::Rect bounds = mforms::App::get()->get_application_bounds();

  // On macOS the vertical screen coordinate points up.
  int left = (int)(bounds.left() + (bounds.width() - size.width) / 2);
#ifdef __APPLE__
  int top = (int)(bounds.top() - (bounds.height() - size.height) / 2);
#else
  int top = (int)(bounds.top() + (bounds.height() - size.height) / 2);
#endif
  show(left, top);
}

//--------------------------------------------------------------------------------------------------

AboutBox::~AboutBox() {
  delete_surface(_back_image);
}

//--------------------------------------------------------------------------------------------------

#define TEXT_BASE_LINE 125
#define BUILD_TEXT_OFFSET 107
#define RELEASE_TYPE_OFFSET 375 // Right border.

void AboutBox::repaint(cairo_t *cr, int x, int y, int w, int h) {
  cairo_scale(cr, 1 / _scale_factor, 1 / _scale_factor);
  cairo_set_source_surface(cr, _back_image, 0, 0);
  cairo_paint(cr);
  cairo_identity_matrix(cr);

  std::string version = base::strfmt(_("Version %i.%i.%i build %i %s (%i bits)"), APP_MAJOR_NUMBER, APP_MINOR_NUMBER,
                                     APP_RELEASE_NUMBER, APP_BUILD_NUMBER, APP_RELEASE_TYPE, (int)sizeof(void *) * 8);

  cairo_select_font_face(cr, ABOUT_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, ABOUT_FONT_SIZE);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_move_to(cr, BUILD_TEXT_OFFSET, TEXT_BASE_LINE);
  cairo_show_text(cr, version.c_str());

  cairo_text_extents_t extents;
  cairo_text_extents(cr, _edition.c_str(), &extents);
  cairo_move_to(cr, RELEASE_TYPE_OFFSET - extents.width, TEXT_BASE_LINE);
  cairo_show_text(cr, _edition.c_str());

  cairo_stroke(cr);
}

//--------------------------------------------------------------------------------------------------

static AboutBox *singleton = NULL;

bool AboutBox::mouse_up(mforms::MouseButton button, int x, int y) {
  singleton->set_modal_result(1);
  return false;
}

//--------------------------------------------------------------------------------------------------

void AboutBox::closed() {
  delete singleton;
  singleton = NULL;
}

//--------------------------------------------------------------------------------------------------

void AboutBox::show_about(const std::string &edition) {
  if (singleton != NULL)
    return;

  singleton = new AboutBox(edition);
  singleton->on_close()->connect(std::bind(&AboutBox::closed));
}

//--------------------------------------------------------------------------------------------------
