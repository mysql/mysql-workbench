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
#ifndef _WORKBENCH_PHYSICAL_VIEWFIGURE_IMPL_H_
#define _WORKBENCH_PHYSICAL_VIEWFIGURE_IMPL_H_

#include "model_figure_impl.h"
#include "grts/structs.workbench.physical.h"

#include "view_figure.h"

/** View figure implementation.
 * Implements inner working of db view figures.
 *
 * Figure contents are updated when the following changes:
 *
 * <li>figure attributes (color, state etc)
 * <li>view name
 *
 * @ingroup Figures
 */
class WBPUBLICBACKEND_PUBLIC_FUNC workbench_physical_ViewFigure::ImplData : public model_Figure::ImplData {
  typedef model_Figure::ImplData super;

protected:
  wbfig::View *_figure;

  boost::signals2::connection _figure_conn;

  virtual bool realize();

  void view_member_changed(const std::string &name, const grt::ValueRef &ovalue);
  void member_changed(const std::string &name, const grt::ValueRef &ovalue);

  virtual void set_in_view(bool flag);

public:
  ImplData(workbench_physical_ViewFigure *owner);
  virtual ~ImplData(){};

  virtual mdc::CanvasItem *get_canvas_item() const {
    return _figure;
  }
  virtual bool is_realizable();

  virtual void unrealize();

  void set_view(const db_ViewRef &view);

private:
  workbench_physical_ViewFigure *self() {
    return (workbench_physical_ViewFigure *)_self;
  }
};

#endif
