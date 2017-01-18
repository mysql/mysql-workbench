/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.ui.h>

#include <grtpp_util.h>
#include "ui_ObjectEditor_impl.h"

//================================================================================
// ui_ObjectEditor

void ui_ObjectEditor::init() {
  if (!_data)
    _data = new ui_ObjectEditor::ImplData(this);
}

ui_ObjectEditor::~ui_ObjectEditor() {
  delete _data;
}

void ui_ObjectEditor::set_data(ImplData *data) {
  _data = data;
}
