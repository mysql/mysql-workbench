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

#include "base/log.h"
#include "grt/grt_manager.h"

#include "model_model_impl.h"

#include "model_diagram_impl.h"
#include "model_connection_impl.h"
#include "model_figure_impl.h"
#include "model_layer_impl.h"

#include "grts/structs.workbench.h"
#include "grtpp_undo_manager.h"

DEFAULT_LOG_DOMAIN(DOMAIN_CANVAS_BE)

model_Model::ImplData::ImplData(model_Model *owner) : _owner(owner) {
  _reset_pending = false;
  _options_signal_installed = false;
  _delegate = NULL;

  scoped_connect(_owner->signal_dict_changed(), std::bind(&ImplData::option_changed, this, std::placeholders::_1,
                                                          std::placeholders::_2, std::placeholders::_3));

  scoped_connect(_owner->signal_list_changed(), std::bind(&ImplData::list_changed, this, std::placeholders::_1,
                                                          std::placeholders::_2, std::placeholders::_3));
}

void model_Model::ImplData::list_changed(grt::internal::OwnedList *list, bool added, const grt::ValueRef &value) {
  if (list == _owner->_diagrams.valueptr()) {
    if (added) {
      if (grt::GRT::get()->get_undo_manager()->is_redoing()) {
        model_DiagramRef::cast_from(value)->get_data()->realize();
      }
    } else
      remove_diagram(model_DiagramRef::cast_from(value));
  }
}

void model_Model::ImplData::option_changed(grt::internal::OwnedDict *dict, bool added, const std::string &option) {
  if (!_options_changed_signal.empty())
    _options_changed_signal(option);
  if (!_reset_pending && (base::hasSuffix(option, "Font") || option == "workbench.physical.Connection:ShowCaptions" ||
                          option == "workbench.physical.Diagram:DrawLineCrossings")) {
    _reset_pending = true;
    //    run_later(std::bind(&model_Model::ImplData::reset_figures, this));
    run_later(std::bind(&model_Model::ImplData::reset_layers, this));
    run_later(std::bind(&model_Model::ImplData::reset_connections, this));
  }
}

void model_Model::ImplData::remove_diagram(const model_DiagramRef &view) {
  view->get_data()->unrealize();

  size_t index = _owner->_diagrams.get_index(view);
  if (index != grt::BaseListRef::npos)
    _owner->_diagrams.remove(index);
}

void model_Model::ImplData::unrealize() {
  const grt::ListRef<model_Diagram> &views(_owner->_diagrams);

  for (size_t c = views.count(), i = 0; i < c; i++) {
    views[i]->get_data()->unrealize();
  }
}

bool model_Model::ImplData::realize() {
  model_Diagram::ImplData *view;

  if (!_options_signal_installed) {
    _options_signal_installed = true;
    GrtObjectRef object(_owner);
    while (object.is_valid() && !object.is_instance<app_Application>())
      object = object->owner();

    if (object.is_valid())
      scoped_connect(app_ApplicationRef::cast_from(object)->options()->signal_dict_changed(),
                     std::bind(&ImplData::option_changed, this, std::placeholders::_1, std::placeholders::_2,
                               std::placeholders::_3));
  }

  // do not use __diagrams because it will get the wrong instance
  grt::ListRef<model_Diagram> views(_owner->_diagrams);

  for (size_t c = views.count(), i = 0; i < c; i++) {
    view = views[i]->get_data();
    if (view)
      view->get_canvas_view(); // Will realize the canvas view if not yet done.
  }

  return true;
}

void model_Model::ImplData::reset_connections() {
  _reset_pending = false;
  grt::ListRef<model_Diagram> views(_owner->_diagrams);

  for (size_t c = views.count(), i = 0; i < c; i++) {
    model_DiagramRef view(views[i]);
    grt::ListRef<model_Connection> conns(view->connections());

    for (size_t cc = conns.count(), ci = 0; ci < cc; ++ci) {
      model_Connection::ImplData *obj = conns.get(ci)->get_data();
      if (obj && obj->get_canvas_item()) {
        obj->unrealize();
        obj->realize();
      }
    }
  }
}

void model_Model::ImplData::reset_figures() {
  _reset_pending = false;
  grt::ListRef<model_Diagram> views(_owner->_diagrams);

  for (size_t c = views.count(), i = 0; i < c; i++) {
    model_DiagramRef view(views[i]);
    grt::ListRef<model_Figure> figs(view->figures());

    for (size_t cc = figs.count(), ci = 0; ci < cc; ++ci) {
      model_Figure::ImplData *obj = figs.get(ci)->get_data();
      if (obj && obj->get_canvas_item()) {
        obj->unrealize();
        obj->realize();
      }
    }
  }
}

void model_Model::ImplData::reset_layers() {
  _reset_pending = false;
  grt::ListRef<model_Diagram> views(_owner->_diagrams);

  for (size_t c = views.count(), i = 0; i < c; i++) {
    model_DiagramRef view(views[i]);
    grt::ListRef<model_Layer> layers(view->layers());

    for (size_t cc = layers.count(), ci = 0; ci < cc; ++ci) {
      model_Layer::ImplData *obj = layers.get(ci)->get_data();
      if (obj && obj->get_canvas_item()) {
        obj->unrealize();
        obj->realize();
      }
    }
  }
}

app_PageSettingsRef model_Model::ImplData::get_page_settings() {
  GrtObjectRef object(_owner);

  while (object.is_valid() && !object.is_instance<workbench_Document>())
    object = object->owner();

  if (object.is_valid())
    return workbench_DocumentRef::cast_from(object)->pageSettings();

  return app_PageSettingsRef();
}

grt::DictRef model_Model::ImplData::get_app_options_dict() {
  GrtObjectRef object(_owner);

  while (object.is_valid() && !object.is_instance<app_Application>())
    object = object->owner();

  if (object.is_valid())
    return app_ApplicationRef::cast_from(object)->options()->options();

  return grt::DictRef();
}

std::string model_Model::ImplData::get_string_option(const std::string &name, const std::string &defvalue) {
  return _owner->_options.get_string(name, get_app_options_dict().get_string(name, defvalue));
}

int model_Model::ImplData::get_int_option(const std::string &name, int defvalue) {
  // TODO: we may want to have ssize_t instead of int returned.
  return (int)_owner->_options.get_int(name, get_app_options_dict().get_int(name, defvalue));
}

std::string model_Model::ImplData::common_color_for_db_object(const grt::ObjectRef &object, const std::string &member) {
  for (size_t c = _owner->diagrams().count(), i = 0; i < c; i++) {
    grt::ListRef<model_Figure> figures(_owner->diagrams()[i]->figures());
    for (size_t vc = figures.count(), vi = 0; vi < vc; vi++) {
      model_FigureRef figure(figures[vi]);

      if (figure.has_member(member) && figure.get_member(member) == object)
        return figure->color();
    }
  }
  return "";
}

void model_Model::ImplData::update_object_color_in_all_diagrams(const std::string &color,
                                                                const std::string &object_member,
                                                                const std::string &object_id) {
  // change color of all objects in all diagrams that point to the same object
  for (size_t vc = _owner->diagrams().count(), vi = 0; vi < vc; vi++) {
    grt::ListRef<model_Figure> figures(_owner->diagrams()[vi]->figures());

    for (grt::ListRef<model_Figure>::const_iterator fig = figures.begin(); fig != figures.end(); ++fig) {
      if ((*fig)->has_member(object_member)) {
        if (!(*fig)->get_member(object_member).is_valid()) {
          logWarning("Corrupted model: figure %s is invalid\n", (*fig)->name().c_str());
        } else if (grt::ObjectRef::cast_from((*fig)->get_member(object_member)).id() == object_id &&
                   strcmp((*fig)->color().c_str(), color.c_str()) != 0) {
          (*fig)->color(color);
        }
      }
    }
  }
}
