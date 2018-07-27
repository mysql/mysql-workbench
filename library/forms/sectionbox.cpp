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

#include "mforms/mforms.h"

using namespace mforms;

#ifdef __APPLE__
#define HEADER_FONT "Lucida Grande"
#define HEADER_FONT_SIZE 12
#elif _MSC_VER
#define HEADER_FONT "Tahoma"
#define HEADER_FONT_SIZE 13
#else
#define HEADER_FONT "Helvetica"
#define HEADER_FONT_SIZE 13
#endif

#define HEADER_TEXT_SPACING 8

//----------------- HeaderBox ----------------------------------------------------------------------

HeaderBox::HeaderBox(SectionBox* owner, bool header_mode) : DrawBox(), _caption_offset(0) {
  _owner = owner;
  _header_mode = header_mode;
}

//--------------------------------------------------------------------------------------------------

void HeaderBox::draw_background(cairo_t* cr, int width, int height) {
#ifndef _MSC_VER
  cairo_set_source_rgb(cr, 235 / 255.0, 235 / 255.0, 235 / 255.0);
  cairo_paint(cr);

  cairo_set_source_rgb(cr, 207 / 255.0, 211 / 255.0, 218 / 255.0);

  cairo_set_line_width(cr, 1);
  cairo_move_to(cr, 0, 0.5);
  cairo_line_to(cr, width, 0.5);
  cairo_stroke(cr);
  cairo_move_to(cr, 0, height - 0.5);
  cairo_line_to(cr, width, height - 0.5);
  cairo_stroke(cr);

#else
  // Window style is more blueish and can also be completely different if header mode is set.
  if (_header_mode) {
    // Erase parts which will later shine through (rounded corners) in dark blue.
    cairo_set_source_rgb(cr, 40 / 255.0, 55 / 255.0, 82 / 255.0);
    cairo_paint(cr);

    cairo_pattern_t* gradient = cairo_pattern_create_linear(0, 0, 0, height);
    cairo_pattern_add_color_stop_rgb(gradient, 0, 0xf9 / 255.0, 0xfc / 255.0, 0xff / 255.0);
    cairo_pattern_add_color_stop_rgb(gradient, 0.49, 0xe1 / 255.0, 0xe4 / 255.0, 0xeb / 255.0);
    cairo_pattern_add_color_stop_rgb(gradient, 0.58, 0xcd / 255.0, 0xd3 / 255.0, 0xdf / 255.0);
    cairo_pattern_add_color_stop_rgb(gradient, 1, 0xcc / 255.0, 0xd4 / 255.0, 0xdf / 255.0);
    cairo_set_source(cr, gradient);

    // Define the rectangle with two rounded corners.
    double corner_size = 4;

    cairo_new_path(cr);
    cairo_move_to(cr, 0, height);
    cairo_arc(cr, corner_size, corner_size, corner_size, M_PI, 3 * M_PI / 2);
    cairo_arc(cr, width - corner_size, corner_size, corner_size, 3 * M_PI / 2, 2 * M_PI);
    cairo_line_to(cr, width, height);
    cairo_close_path(cr);

    // Fill the defined path without our gradient.
    cairo_fill(cr);

    cairo_pattern_destroy(gradient);
  } else {
    cairo_set_source_rgb(cr, 225 / 255.0, 231 / 255.0, 240 / 255.0);
    cairo_paint(cr);

    cairo_set_source_rgb(cr, 195 / 255.0, 205 / 255.0, 219 / 255.0);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, 0, height - 0.5);
    cairo_line_to(cr, width, height - 0.5);
    cairo_stroke(cr);
  }

#endif

  cairo_move_to(cr, 0, height - 0.5);
  cairo_line_to(cr, width, height - 0.5);
  cairo_stroke(cr);
}

//--------------------------------------------------------------------------------------------------

void HeaderBox::repaint(cairo_t* cr, int x, int y, int w, int h) {
  int height = get_height();
  int width = get_width();

  draw_background(cr, width, height);

  double offset = 10;

// Expand icon. On Windows this is right aligned and no icon is shown at all if
// the section is not expandable. We also draw a simple triangle in that case (no image).
#ifdef _MSC_VER
  double icon_width = 8;
  double icon_height = 7;
  _icon_left = width - icon_width - offset + 0.5;

  _icon_right = _icon_left + icon_width;
  _icon_top = (height - icon_height) / 2;
  _icon_bottom = _icon_top + icon_height;

  if (_owner->_expandable) {
    if (_owner->_expanded) {
      cairo_move_to(cr, _icon_left, _icon_top);
      cairo_line_to(cr, _icon_right, _icon_top);
      cairo_line_to(cr, (_icon_left + _icon_right) / 2, _icon_bottom);
      cairo_set_source_rgb(cr, 64 / 255.0, 61 / 255.0, 72 / 255.0);
      cairo_fill(cr);
    } else {
      cairo_move_to(cr, _icon_left, _icon_top);
      cairo_line_to(cr, _icon_right, (_icon_top + _icon_bottom) / 2);
      cairo_line_to(cr, _icon_left, _icon_bottom);
      cairo_set_source_rgb(cr, 64 / 255.0, 61 / 255.0, 72 / 255.0);
      cairo_fill(cr);
    }
  }

#else

  cairo_surface_t* icon = _owner->_unexpandable_icon;

  if (_owner->_expandable)
    icon = _owner->_expanded ? _owner->_expanded_icon : _owner->_unexpanded_icon;
  if (icon != NULL) {
    double icon_width = cairo_image_surface_get_width(icon);
    double icon_height = cairo_image_surface_get_height(icon);
    _icon_left = offset;

    _icon_right = _icon_left + icon_width;
    _icon_top = (height - icon_height) / 2;
    _icon_bottom = _icon_top + icon_height;

    offset += icon_width + HEADER_TEXT_SPACING;
    cairo_set_source_surface(cr, icon, _icon_left, _icon_top);
    cairo_paint(cr);
  } else {
    _icon_left = 0;
    _icon_right = 0;
    _icon_top = 0;
    _icon_bottom = 0;
  }

#endif

  // Caption.
  if (_owner->_title != "") {
    cairo_select_font_face(cr, HEADER_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, HEADER_FONT_SIZE);
    if (_caption_offset == 0) {
      cairo_text_extents_t extents;

      cairo_text_extents(cr, _owner->_title.c_str(), &extents);
      _caption_offset = floor((height - extents.height) / 2 - extents.y_bearing);
    }

    cairo_set_source_rgb(cr, 25 / 255.0, 25 / 255.0, 25 / 255.0);
    cairo_move_to(cr, offset, _caption_offset);
    cairo_show_text(cr, _owner->_title.c_str());
    cairo_stroke(cr);
  }
}

//--------------------------------------------------------------------------------------------------

bool HeaderBox::mouse_down(mforms::MouseButton button, int x, int y) {
  // Check if the mouse position is on the icon and toggle the section box if that is the case.
  if (_owner->_expandable && x >= _icon_left && x <= _icon_right && y >= _icon_top && y <= _icon_bottom)
    _owner->toggle();

  return true;
}

//----------------- SectionBox ---------------------------------------------------------------------

SectionBox::SectionBox(bool expandable, const std::string& title, bool header_mode)
  : Box(false), _content(NULL), _expandable(expandable), _expanded(true) {
  _title = title;

  _header = new HeaderBox(this, header_mode);
  _header->set_size(300, 23);
  add(_header, false, true);

  _unexpandable_icon = Utilities::load_icon("section_unexpandable.png");
  _unexpanded_icon = Utilities::load_icon("section_unexpanded.png");
  _expanded_icon = Utilities::load_icon("section_expanded.png");
}

//--------------------------------------------------------------------------------------------------

SectionBox::~SectionBox() {
  cairo_surface_destroy(_unexpandable_icon);
  cairo_surface_destroy(_unexpanded_icon);
  cairo_surface_destroy(_expanded_icon);

  delete _header;
}

//--------------------------------------------------------------------------------------------------

/**
 * Set what is to be displayed in the box as content (it can still be hidden).
 */
void SectionBox::set_content(View* content) {
  if (_content != content) {
    if (_content != NULL)
      remove(_content);
    _content = content; // Weak reference.
    add(_content, true, true);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Toggles the expand state of the box.
 */
void SectionBox::toggle() {
  set_expanded(!_expanded);
}

//--------------------------------------------------------------------------------------------------

void SectionBox::set_expanded(bool expanded) {
  if (_expanded != expanded) {
    _expanded = expanded;

    _header->set_needs_repaint();
    if (_content != NULL)
      _content->show(_expanded);
    relayout();
  }
}

//--------------------------------------------------------------------------------------------------
