/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifndef _BADGE_FIGURE_H_
#define _BADGE_FIGURE_H_

#include "mdc.h"

#include <algorithm>
#include <boost/signals2.hpp>

#include "wbpublic_public_interface.h"

class WBPUBLICBACKEND_PUBLIC_FUNC BadgeFigure : public mdc::Figure {
  mdc::FontSpec _font;
  std::string _badge_id;
  std::string _text;
  base::Color _fill_color2;
  base::Color _text_color;
  cairo_pattern_t *_gradient;
  base::Size _text_size;

  virtual base::Size calc_min_size();

public:
  BadgeFigure(mdc::Layer *layer);
  virtual ~BadgeFigure();
  virtual void draw_contents(mdc::CairoCtx *cr);

  void set_badge_id(const std::string &bid);
  std::string badge_id() const {
    return _badge_id;
  }

  void set_text(const std::string &text);
  void set_gradient_from_color(const base::Color &color);
  void set_fill_color2(const base::Color &color);
  void set_text_color(const base::Color &color);

  boost::signals2::scoped_connection updater_connection;
};

#endif
