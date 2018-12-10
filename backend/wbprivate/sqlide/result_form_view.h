/*
 * Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _RESULT_FORM_VIEW_H_
#define _RESULT_FORM_VIEW_H_

#include "mforms/appview.h"
#include "mforms/scrollpanel.h"
#include "mforms/table.h"
#include "mforms/toolbar.h"

class ResultFormView : public mforms::AppView {
public:
  class FieldView;

private:
  Recordset::Ptr _rset;

  mforms::ScrollPanel _spanel;
  mforms::Table _table;
  std::vector<FieldView *> _fields;
  mforms::ToolBar _tbar;
  mforms::ToolBarItem *_label_item;
  mforms::ToolBarItem *_geom_type_item;

  bool _editable;
  boost::signals2::connection _refresh_ui_connection;

  void navigate(mforms::ToolBarItem *item);
  void update_value(int column, const std::string &value);
  void open_field_editor(int column, const std::string &type);

  void geom_type_changed();

public:
  ResultFormView(bool editable);

  mforms::ToolBar *get_toolbar() {
    return &_tbar;
  }

  virtual ~ResultFormView();
  int display_record();
  int display_record(RowId row_id);
  std::string get_full_column_type(SqlEditorForm *editor, const std::string &schema, const std::string &table,
                                   const std::string &column);

  void init_for_resultset(Recordset::Ptr rset_ptr, SqlEditorForm *editor);
  void updateColors();
};

#endif
