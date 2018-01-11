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
