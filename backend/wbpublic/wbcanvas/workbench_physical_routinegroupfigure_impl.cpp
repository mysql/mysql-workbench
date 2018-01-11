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

#include "workbench_physical_routinegroupfigure_impl.h"
#include "workbench_physical_model_impl.h"

#include "workbench_physical_diagram_impl.h"
#include "model_layer_impl.h"

#include "base/string_utilities.h"

using namespace base;

workbench_physical_RoutineGroupFigure::ImplData::ImplData(workbench_physical_RoutineGroupFigure *self)
  : super(self), _figure(0) {
  _resizable = false;

  scoped_connect(self->signal_changed(),
                 std::bind(&ImplData::member_changed, this, std::placeholders::_1, std::placeholders::_2));
}

void workbench_physical_RoutineGroupFigure::ImplData::routinegroup_member_changed(const std::string &name,
                                                                                  const grt::ValueRef &ovalue) {
  if (name == "name") {
    self()->_name = self()->_routineGroup->name();

    if (_figure)
      _figure->set_title(*self()->_name, strfmt("%i routines", (int)self()->_routineGroup->routines().count()));
  }
}

void workbench_physical_RoutineGroupFigure::ImplData::contents_changed() {
  sync_routines();
}

void workbench_physical_RoutineGroupFigure::ImplData::set_in_view(bool flag) {
  if (!self()->owner().is_valid())
    throw std::logic_error("adding figure to view before setting owner");

  if (flag) {
    if (self()->_routineGroup.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->add_mapping(self()->_routineGroup, self());
  } else {
    if (self()->_routineGroup.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->remove_mapping(self()->_routineGroup);
  }

  model_Figure::ImplData::set_in_view(flag);
}

void workbench_physical_RoutineGroupFigure::ImplData::set_routine_group(const db_RoutineGroupRef &rgroup) {
  // Check if we had a valid rg before and revert the previous setup if so.
  if (self()->_routineGroup.is_valid()) {
    if (self()->_owner.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->remove_mapping(self()->_routineGroup);
  }

  self()->_routineGroup = rgroup;

  if (_figure_conn.connected())
    _figure_conn.disconnect();

  if (self()->routineGroup().is_valid()) {
    rgroup->signal_contentChanged()->connect(
      std::bind(&workbench_physical_RoutineGroupFigure::ImplData::contents_changed, this));

    if (self()->_owner.is_valid())
      workbench_physical_DiagramRef::cast_from(self()->_owner)->get_data()->add_mapping(rgroup, self());

    _figure_conn = self()->routineGroup()->signal_changed()->connect(
      std::bind(&ImplData::routinegroup_member_changed, this, std::placeholders::_1, std::placeholders::_2));
    self()->_name = self()->routineGroup()->name();
  }

  if (!_figure)
    try_realize();
  else if (_figure) {
    if (self()->routineGroup().is_valid()) {
      _figure->set_title(*self()->_name, strfmt("%i routines", (int)self()->_routineGroup->routines().count()));
    } else
      unrealize();
  }
}

void workbench_physical_RoutineGroupFigure::ImplData::member_changed(const std::string &name,
                                                                     const grt::ValueRef &ovalue) {
  if (name == "color" && self()->owner().is_valid() && self()->owner()->owner().is_valid() &&
      self()->owner()->owner()->get_data()->get_int_option("SynchronizeObjectColors", 0)) {
    if (*grt::StringRef::cast_from(ovalue) != "")
      self()->owner()->owner()->get_data()->update_object_color_in_all_diagrams(self()->_color, "routineGroup",
                                                                                self()->_routineGroup.id());

    super::member_changed(name, ovalue);
  }
}

bool workbench_physical_RoutineGroupFigure::ImplData::is_realizable() {
  if (!super::is_realizable())
    return false;

  if (self()->_routineGroup.is_valid())
    return true;

  return false;
}

void workbench_physical_RoutineGroupFigure::ImplData::unrealize() {
  workbench_physical_ModelRef model(workbench_physical_ModelRef::cast_from(self()->owner()->owner()));

  notify_will_unrealize();

  // remove tag badges
  std::list<meta_TagRef> tags(model->get_data()->get_tags_for_dbobject(self()->_routineGroup));
  for (std::list<meta_TagRef>::const_iterator end = tags.end(), tag = tags.begin(); tag != end; ++tag) {
    self()->owner()->get_data()->remove_tag_badge_from_figure(self(), *tag);
  }

  super::unrealize();

  delete _figure;
  _figure = 0;
}

bool workbench_physical_RoutineGroupFigure::ImplData::realize() {
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

    view->lock();

    _figure = new wbfig::RoutineGroup(view->get_current_layer(), self()->owner()->get_data(), self());

    agroup = self()->layer()->get_data()->get_area_group();

    view->get_current_layer()->add_item(_figure, agroup);

    _figure->set_color(Color::parse(*self()->_color));

    _figure->set_title(*self()->_name, strfmt("%i routines", (int)self()->_routineGroup->routines().count()));

    sync_routines();

    _figure->toggle(*self()->_expanded != 0);

    finish_realize();

    view->unlock();

    notify_realized();

    // add badges for each tag
    std::list<meta_TagRef> tags(model->get_data()->get_tags_for_dbobject(self()->_routineGroup));
    for (std::list<meta_TagRef>::const_iterator end = tags.end(), tag = tags.begin(); tag != end; ++tag) {
      self()->owner()->get_data()->add_tag_badge_to_figure(self(), *tag);
    }
  }
  return true;
}

void workbench_physical_RoutineGroupFigure::ImplData::sync_routines() {
  if (_figure) {
    wbfig::BaseFigure::ItemList::iterator iter = _figure->begin_routines_sync();

    grt::ListRef<db_Routine> routines(self()->_routineGroup->routines());
    int maxnlength = self()->owner()->owner()->get_data()->get_int_option(
      "workbench.physical.RoutineGroupFigure:MaxRoutineNameLength", 20);

    for (size_t c = routines.count(), i = 0; i < c; i++) {
      db_RoutineRef routine(routines.get(i));
      std::string text;

      text = *routine->name();

      if (g_utf8_strlen(text.data(), (gssize)text.size()) > maxnlength) {
        gchar *buf = (gchar *)g_malloc((gssize)text.size() + 1);
        g_utf8_strncpy(buf, text.data(), maxnlength);
        text = buf;
        g_free(buf);
        text.append("...");
      }

      iter = _figure->sync_next_routine(iter, routine.id(), text);
    }
    _figure->end_routines_sync(iter);

    _figure->set_title(*self()->_routineGroup->name(),
                       strfmt("%i routines", (int)self()->_routineGroup->routines().count()));
  }
}
