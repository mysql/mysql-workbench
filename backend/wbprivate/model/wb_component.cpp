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

#include "wb_component.h"
#include "workbench/wb_context.h"
#include "model/wb_model_diagram_form.h"
#include "wb_layer_tree.h"
#include "mdc_algorithms.h"
#include <grtpp_undo_manager.h>
#include "base/string_utilities.h"

using namespace bec;
using namespace wb;
using namespace base;

WBComponent::WBComponent(WBContext *context) : _wb(context) {
}

//--------------------------------------------------------------------------------------------------

grt::ValueRef WBComponent::place_object(ModelDiagramForm *form, const Point &pos, const std::string &object_struct,
                                        const grt::DictRef &args) {
  model_DiagramRef view(form->get_model_diagram());
  std::string object_type, object_struct_name;

  Point offset = pos;
  model_LayerRef layer = view->rootLayer();

  // find the layer that contains the point
  for (grt::ListRef<model_Layer>::const_iterator l = view->layers().begin(); l != view->layers().end(); ++l) {
    Rect rect((*l)->left(), (*l)->top(), (*l)->width(), (*l)->height());
    if (mdc::bounds_contain_point(rect, pos.x, pos.y)) {
      layer = *l;
      offset.x = pos.x - rect.left();
      offset.y = pos.y - rect.top();
      break;
    }
  }

  form->get_view()->snap_to_grid(offset);

  model_FigureRef figure(grt::GRT::get()->create_object<model_Figure>(object_struct));

  if (!figure.is_valid())
    throw std::runtime_error("Could not create object of type " + object_struct);

  figure->owner(view);
  figure->layer(layer);
  figure->left(offset.x);
  figure->top(offset.y);

  if (figure->name() == "")
    figure->name(figure.get_metaclass()->get_attribute("caption"));

  if (args.is_valid()) {
    for (grt::DictRef::const_iterator iter = args.begin(); iter != args.end(); ++iter) {
      figure.set_member(iter->first, iter->second);
    }
  }

  if (!args.is_valid() || !args.has_key("color")) {
    if (!form->get_tool_argument(object_struct + ":Color").empty())
      figure->color(grt::StringRef(form->get_tool_argument(object_struct + ":Color")));
    else
      figure->color(_wb->get_wb_options().get_string(object_struct + ":Color", ""));
  }

  grt::AutoUndo undo;

  view->addFigure(figure);

  // ensure the figure fits in the layer and if not, resize the layer
  double w = *figure->width();
  double h = *figure->height();

  if (offset.x + w > *layer->width()) {
    layer->width(offset.x + w);
  }

  if (offset.y + h > *layer->height()) {
    layer->height(offset.y + h);
  }

  // auto-select figure
  view->unselectAll();
  view->selectObject(figure);

  undo.end(_("Place Figure"));

  // manually notify selection change
  _wb->request_refresh(RefreshSelection, "", (NativeHandle)form->get_frontend_data());

  return figure;
}

//--------------------------------------------------------------------------------------------------

std::string WBComponent::get_command_option_value(const std::string &option) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());

  if (form)
    return form->get_tool_argument(option);

  return "";
}

//--------------------------------------------------------------------------------------------------

void WBComponent::set_command_option_value(const std::string &option, const std::string &item) {
  ModelDiagramForm *form = dynamic_cast<ModelDiagramForm *>(_wb->get_active_main_form());

  if (form)
    form->set_tool_argument(option, item);
}
