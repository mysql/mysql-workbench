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
#ifndef _WORKBENCH_PHYSICAL_ROUTINEGROUPFIGURE_IMPL_H_
#define _WORKBENCH_PHYSICAL_ROUTINEGROUPFIGURE_IMPL_H_

#include "model_figure_impl.h"
#include "grts/structs.workbench.physical.h"

#include "figure_common.h"
#include "routine_group_figure.h"

/** Routine Group figure implementation.
 * Implements inner working of routine group figures.
 *
 * Figure contents are updated when the following changes:
 *
 * <li>figure attributes (color, state etc)
 * <li>group name
 * <li>routine list (added, removed)
 * <li>routine name changes
 *
 * @ingroup Figures
 */
class WBPUBLICBACKEND_PUBLIC_FUNC workbench_physical_RoutineGroupFigure::ImplData : public model_Figure::ImplData {
  typedef model_Figure::ImplData super;

protected:
  wbfig::RoutineGroup *_figure;
  boost::signals2::scoped_connection _figure_conn;

  virtual bool realize();

  void sync_routines();

  void contents_changed();

  void member_changed(const std::string &member, const grt::ValueRef &ovalue);
  void routinegroup_member_changed(const std::string &member, const grt::ValueRef &ovalue);

  virtual void set_in_view(bool flag);

public:
  ImplData(workbench_physical_RoutineGroupFigure *self);
  virtual ~ImplData(){};

  void set_routine_group(const db_RoutineGroupRef &rgroup);

  virtual mdc::CanvasItem *get_canvas_item() const {
    return _figure;
  }
  virtual bool is_realizable();

  virtual void unrealize();

private:
  workbench_physical_RoutineGroupFigure *self() {
    return (workbench_physical_RoutineGroupFigure *)_self;
  }
};

#endif
