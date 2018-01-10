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

#include "workbench_physical_viewfigure_impl.h"

#include "model_layer_impl.h"
#include "workbench_physical_model_impl.h"
#include "workbench_physical_diagram_impl.h"

using namespace base;

workbench_physical_ViewFigure::ImplData::ImplData(workbench_physical_ViewFigure *owner) : super(owner), _figure(0) {
  _resizable = false;
}

void workbench_physical_ViewFigure::ImplData::view_member_changed(const std::string &name,
                                                                  const grt::ValueRef &ovalue) {
  if (name == "name") {
    self()->_name = self()->view()->name();

    if (_figure)
      _figure->set_title(*self()->_name);
  }
}

void workbench_physical_ViewFigure::ImplData::set_in_view(bool flag) {
  if (!self()->owner().is_valid())
    throw std::logic_error("adding figure to diagram before setting owner");

  if (flag) {
    if (self()->_view.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->add_mapping(self()->_view, self());
  } else {
    if (self()->_view.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->remove_mapping(self()->_view);
  }

  model_Figure::ImplData::set_in_view(flag);
}

void workbench_physical_ViewFigure::ImplData::set_view(const db_ViewRef &view) {
  // Check if we had a valid view before and revert the previous setup if so.
  if (self()->_view.is_valid()) {
    if (self()->_owner.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->remove_mapping(self()->_view);
  }

  self()->_view = view;

  if (_figure_conn.connected())
    _figure_conn.disconnect();

  if (self()->view().is_valid()) {
    if (self()->_owner.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->add_mapping(view, self());

    _figure_conn = self()->view()->signal_changed()->connect(
      std::bind(&ImplData::view_member_changed, this, std::placeholders::_1, std::placeholders::_2));
    self()->_name = self()->view()->name();
  }

  if (!_figure)
    try_realize();
  else if (_figure) {
    if (self()->view().is_valid())
      _figure->set_title(*self()->view()->name());
    else
      unrealize();
  }
}

void workbench_physical_ViewFigure::ImplData::member_changed(const std::string &name, const grt::ValueRef &ovalue) {
  /*  dont rename view from figure
else if (name == "name")
{
  if (grt::StringRef::cast_from(value) == self()->_view->name())
  {
    //if (__view.is_valid())
    //  __view.name(__name);
  }
}*/
  if (name == "color" && self()->owner().is_valid() && self()->owner()->owner().is_valid() &&
      self()->owner()->owner()->get_data()->get_int_option("SynchronizeObjectColors", 0)) {
    if (*grt::StringRef::cast_from(ovalue) != "")
      self()->owner()->owner()->get_data()->update_object_color_in_all_diagrams(self()->_color, "view",
                                                                                self()->_view.id());

    super::member_changed(name, ovalue);
  }
}

bool workbench_physical_ViewFigure::ImplData::is_realizable() {
  if (!super::is_realizable())
    return false;

  if (self()->_view.is_valid())
    return true;

  return false;
}

void workbench_physical_ViewFigure::ImplData::unrealize() {
  workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(self()->owner()->owner()));

  notify_will_unrealize();

  // remove tag badges
  std::list<meta_TagRef> tags(model->get_data()->get_tags_for_dbobject(self()->_view));
  for (std::list<meta_TagRef>::const_iterator end = tags.end(), tag = tags.begin(); tag != end; ++tag) {
    self()->owner()->get_data()->remove_tag_badge_from_figure(self(), *tag);
  }

  super::unrealize();

  delete _figure;
  _figure = 0;
}

bool workbench_physical_ViewFigure::ImplData::realize() {
  if (_figure)
    return true;
  if (!is_realizable())
    return false;

  if (!is_main_thread()) {
    run_later(std::bind(&ImplData::realize, this));
    return true;
  }

  if (!_figure) {
    mdc::CanvasView *view = self()->owner()->get_data()->get_canvas_view();
    workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(self()->owner()->owner()));
    mdc::AreaGroup *agroup;

    if (!self()->_view.is_valid())
      throw std::logic_error("Realizing table figure without table object");

    view->lock();

    _figure = new wbfig::View(view->get_current_layer(), self()->owner()->get_data(), self());

    agroup = self()->layer()->get_data()->get_area_group();

    view->get_current_layer()->add_item(_figure, agroup);

    _figure->set_color(Color::parse(*self()->_color));

    _figure->set_title(*self()->_view->name());

    finish_realize();

    view->unlock();

    notify_realized();

    // add badges for each tag
    std::list<meta_TagRef> tags(model->get_data()->get_tags_for_dbobject(self()->_view));
    for (std::list<meta_TagRef>::const_iterator end = tags.end(), tag = tags.begin(); tag != end; ++tag) {
      self()->owner()->get_data()->add_tag_badge_to_figure(self(), *tag);
    }
  }
  return true;
}
