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

#include "wb_editor_layer.h"
#include "base/string_utilities.h"

LayerEditorBE::LayerEditorBE(const workbench_physical_LayerRef &layer) : BaseEditor(layer), _layer(layer) {
}

bool LayerEditorBE::should_close_on_delete_of(const std::string &oid) {
  if (_layer.id() == oid || _layer->owner().id() == oid)
    return true;

  return false;
}

void LayerEditorBE::set_color(const std::string &color) {
  if (_layer->color() != color) {
    bec::AutoUndoEdit undo(this, _layer, "color");
    _layer->color(color);
    undo.end(_("Change Layer Color"));
  }
}

std::string LayerEditorBE::get_color() {
  return _layer->color();
}

void LayerEditorBE::set_name(const std::string &name) {
  if (_layer->name() != name) {
    bec::AutoUndoEdit undo(this, _layer, "name");
    _layer->name(name);
    undo.end(_("Change Layer Name"));
  }
}

std::string LayerEditorBE::get_name() {
  return _layer->name();
}

std::string LayerEditorBE::get_title() {
  return base::strfmt("%s - Layer", get_name().c_str());
}
