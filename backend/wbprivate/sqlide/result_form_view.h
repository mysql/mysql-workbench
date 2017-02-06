/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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
};

#endif