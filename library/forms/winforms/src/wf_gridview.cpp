/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "wf_base.h"
#include "wf_view.h"
#include "wf_native.h"
#include "wf_menubar.h"
#include "wf_gridview.h"

using namespace MySQL;
using namespace MySQL::Controls;
using namespace MySQL::Forms;
using namespace MySQL::Base;

using namespace System::Drawing;

class ConcreteGridView : public mforms::GridView {
  gcroot<IRecordsetView ^> _view;
  bool _resizing;

public:
  ConcreteGridView(IRecordsetView ^ rset) {
    _resizing = false;
    _view = rset;
  }

  IRecordsetView ^ view() { return _view; }

    Control
    ^ control() { return _view->control(); }

    bool is_resizing() {
    return _resizing;
  }

  virtual int get_column_count() {
    return _view->get_column_count();
  }

  virtual int get_column_width(int column) {
    return _view->get_column_width(column);
  }

  virtual void set_column_width(int column, int width) {
    _view->set_column_width(column, width);
  }

  virtual void update_columns() {
    _view->update_columns();
  }

  virtual bool current_cell(size_t &row, int &column) {
    row = _view->current_cell_row();
    column = _view->current_cell_column();
    if (_view->current_cell_row() < 0)
      return false;
    return true;
  }

  virtual void set_current_cell(size_t row, int column) {
    _view->set_current_cell((int)row, column);
  }

  virtual void set_column_header_indicator(int column, mforms::ColumnHeaderIndicator order) {
    _view->set_column_header_indicator(column, (IRecordsetView::ColumnHeaderIndicator)order);
  }

  virtual void set_header_menu(mforms::ContextMenu *header_menu) {
    System::Windows::Forms::ContextMenuStrip ^ menu =
      MenuBarWrapper::GetManagedObject<System::Windows::Forms::ContextMenuStrip>(header_menu);
    if (Conversions::UseWin8Drawing())
      menu->Renderer = gcnew Win8MenuStripRenderer();
    else
      menu->Renderer = gcnew TransparentMenuStripRenderer();

    mforms::GridView::set_header_menu(header_menu);
  }
};

public
ref class MySQL::Forms::ColumnCallbackWrapper {
  ConcreteGridView *backend;

public:
  ColumnCallbackWrapper(ConcreteGridView *be) : backend(be) {
  }

  void resized(int column) {
    if (!backend->is_resizing())
      (*backend->signal_column_resized())(column);
  }

  System::Windows::Forms::ContextMenuStrip ^ header_right_click(int column) {
    backend->clicked_header_column(column);
    if (backend->header_menu() != NULL) {
      backend->header_menu()->will_show();

      return MenuBarWrapper::GetManagedObject<System::Windows::Forms::ContextMenuStrip>(backend->header_menu());
    }
    return nullptr;
  }
};

gcroot<CreateGridViewDelegate ^> GridViewWrapper::factory = nullptr;

GridViewWrapper::GridViewWrapper(mforms::GridView *backend) : NativeWrapper(backend) {
}

GridViewWrapper::~GridViewWrapper() {
}

mforms::GridView *GridViewWrapper::create(std::shared_ptr<Recordset> rset) {
  CreateGridViewDelegate ^ create = factory;
  ConcreteGridView *backend = new ConcreteGridView(create(IntPtr(&rset)));
  GridViewWrapper *wrapper = new GridViewWrapper(backend);
  NativeWrapper::ConnectParts(backend, wrapper, backend->control());

  wrapper->column_callback_delegate = gcnew ColumnCallbackWrapper(backend);
  backend->view()->set_column_resize_callback(
    gcnew IRecordsetView::ColumnResizeCallback(wrapper->column_callback_delegate, &ColumnCallbackWrapper::resized));
  backend->view()->set_column_header_right_click_callback(gcnew IRecordsetView::ColumnHeaderRightClickCallback(
    wrapper->column_callback_delegate, &ColumnCallbackWrapper::header_right_click));
  return backend;
}

void GridViewWrapper::init(CreateGridViewDelegate ^ creator) {
  factory = creator;
  mforms::GridView::register_factory(&GridViewWrapper::create);
}
