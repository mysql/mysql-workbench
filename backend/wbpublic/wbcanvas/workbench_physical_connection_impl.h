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

#ifndef _WORKBENCH_PHYSICAL_CONNECTION_IMPL_H_
#define _WORKBENCH_PHYSICAL_CONNECTION_IMPL_H_

#include "model_connection_impl.h"
#include "grts/structs.workbench.physical.h"

namespace wbfig {
  class FigureItem;
};

class WBPUBLICBACKEND_PUBLIC_FUNC workbench_physical_Connection::ImplData : public model_Connection::ImplData {
  typedef model_Connection::ImplData super;

protected:
  boost::signals2::scoped_connection _realize_conn;

  boost::signals2::scoped_connection _fk_member_changed_conn;
  boost::signals2::scoped_connection _fk_changed_conn;
  boost::signals2::scoped_connection _table_changed_conn;

  bool _highlighting;

  void fk_changed(const db_ForeignKeyRef &fk);
  void member_changed(const std::string &name, const grt::ValueRef &ovalue);

  virtual bool realize();
  virtual void unrealize();

  void update_line_ends();
  void layout_changed();
  void table_changed(const std::string &detail);

  virtual mdc::CanvasItem *get_start_canvas_item();
  virtual mdc::CanvasItem *get_end_canvas_item();

  virtual void caption_bounds_changed(const base::Rect &obounds, mdc::TextFigure *figure);

  void fk_member_changed(const std::string &member, const grt::ValueRef &ovalue);

  void object_realized(const model_ObjectRef &object);

  void update_connected_tables();

public:
  ImplData(workbench_physical_Connection *self);
  virtual ~ImplData();

  virtual void highlight(const base::Color *color = 0);
  virtual void unhighlight();

  virtual void set_in_view(bool flag);

  void set_foreign_key(const db_ForeignKeyRef &fk);

private:
  workbench_physical_Connection *self() const {
    return (workbench_physical_Connection *)_self;
  }
};

#endif
