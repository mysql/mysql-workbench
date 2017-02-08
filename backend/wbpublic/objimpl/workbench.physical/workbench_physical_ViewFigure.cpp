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

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_viewfigure_impl.h"

//================================================================================
// workbench_physical_ViewFigure

void workbench_physical_ViewFigure::init() {
  if (!_data)
    _data = new workbench_physical_ViewFigure::ImplData(this);
  model_Figure::set_data(_data);
}

void workbench_physical_ViewFigure::set_data(ImplData *data) {
}

workbench_physical_ViewFigure::~workbench_physical_ViewFigure() {
  delete _data;
}

void workbench_physical_ViewFigure::view(const db_ViewRef &value) {
  if (_view == value)
    return;
  if (_view.is_valid() && value.is_valid())
    throw std::runtime_error("Cannot change view field of figure after its set");

  if (_is_global && _view.is_valid())
    _view.unmark_global();
  if (_is_global && value.is_valid())
    value.mark_global();

  grt::ValueRef ovalue(_view);
  get_data()->set_view(value);
  member_changed("view", ovalue, value);
}
