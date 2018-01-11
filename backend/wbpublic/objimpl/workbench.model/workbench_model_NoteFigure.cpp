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

#include <grts/structs.workbench.model.h>

#include <grtpp_util.h>

#include "base/string_utilities.h"
#include "wbcanvas/workbench_model_notefigure_impl.h"

//================================================================================
// workbench_model_NoteFigure

void workbench_model_NoteFigure::init() {
  if (!_data)
    _data = new workbench_model_NoteFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_model_NoteFigure::set_data(ImplData *data) {
}

workbench_model_NoteFigure::~workbench_model_NoteFigure() {
  delete _data;
}

void workbench_model_NoteFigure::text(const grt::StringRef &value) {
  grt::ValueRef ovalue(_text);
  _text = value;
  _data->set_text(_text);
  member_changed("text", ovalue, value);
}

void workbench_model_NoteFigure::textColor(const grt::StringRef &value) {
  grt::ValueRef ovalue(_textColor);
  _textColor = value;
  _data->set_text_color(_textColor);
  member_changed("textColor", ovalue, value);
}

void workbench_model_NoteFigure::font(const grt::StringRef &value) {
  grt::ValueRef ovalue(_font);
  _font = value;
  _data->set_font(*value);
  member_changed("font", ovalue, value);
}
