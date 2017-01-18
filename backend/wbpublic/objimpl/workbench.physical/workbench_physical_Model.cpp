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

#include "grts/structs.workbench.physical.h"
#include "grts/structs.workbench.h"

#include "grtpp_util.h"
#include "grtpp_undo_manager.h"

#include "wbcanvas/workbench_physical_model_impl.h"
#include "wbcanvas/model_diagram_impl.h"
#include "base/string_utilities.h"

using namespace base;

//================================================================================
// workbench_physical_Model

void workbench_physical_Model::init() {
  if (!_data)
    _data = new workbench_physical_Model::ImplData(this);
  model_Model::set_data(_data);
}

void workbench_physical_Model::set_data(ImplData *data) {
}

workbench_physical_Model::~workbench_physical_Model() {
  delete _data;
}

model_DiagramRef workbench_physical_Model::addNewDiagram(ssize_t defer_realize) {
  grt::AutoUndo undo(!is_global());

  model_DiagramRef view;
  std::string name;

  name = grt::get_name_suggestion_for_list_object(_diagrams, "EER Diagram", false);

  // calculate default size of view
  Size size = model_Diagram::ImplData::get_size_for_page(get_data()->get_page_settings());

  size.width *= 2; // 2 pages

  view = workbench_physical_DiagramRef(grt::Initialized);
  view->owner(this);
  view->name(name);
  view->width(size.width);
  view->height(size.height);
  view->zoom(1);

  _diagrams.insert(view);

  if (!defer_realize)
    view->get_data()->realize();

  undo.end(base::strfmt(_("New Diagram '%s'"), name.c_str()));

  return view;
}
