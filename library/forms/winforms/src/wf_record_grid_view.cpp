/*
* Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "wf_base.h"
#include "wf_view.h"
#include "wf_native.h"
#include "wf_menubar.h"
#include "wf_record_grid_view.h"

using namespace MySQL;
using namespace MySQL::Controls;
using namespace MySQL::Forms;
using namespace MySQL::Base;


using namespace System::Drawing;

class ConcreteRecordGridView : public mforms::RecordGrid
{
  gcroot<IRecordsetView^> _view;
  bool _resizing;

public:
  ConcreteRecordGridView(IRecordsetView ^rset)
  {
    _resizing = false;
    _view = rset;
  }

  IRecordsetView ^view() { return _view; }

  Control ^control() { return _view->control(); }

  bool is_resizing() { return _resizing;  }

  virtual int get_column_count()
  {
    return _view->get_column_count();
  }

  virtual int get_column_width(int column)
  {
    return _view->get_column_width(column);
  }

  virtual void set_column_width(int column, int width)
  {
    _view->set_column_width(column, width);
  }

  virtual bool current_cell(size_t &row, int &column)
  {
    row = _view->current_cell_row();
    column = _view->current_cell_column();
    if (_view->current_cell_row() < 0)
      return false;
    return true;
  }

  virtual void set_current_cell(size_t row, int column)
  {
    _view->set_current_cell((int)row, column);
  }

  virtual void set_font(const std::string &font)
  {
    std::string family;
    float size;
    bool bold;
    bool italic;
    base::parse_font_description(font, family, size, bold, italic);

    FontStyle style = FontStyle::Regular;
    if (bold)
      style = (FontStyle)(style | FontStyle::Bold);
    if (italic)
      style = (FontStyle)(style | FontStyle::Italic);

    _view->set_font(MySQL::CppStringToNative(family), size, style);
  }

  virtual void set_column_header_indicator(int column, mforms::ColumnHeaderIndicator order)
  {
    _view->set_column_header_indicator(column, (IRecordsetView::ColumnHeaderIndicator)order);
  }

  virtual void set_header_menu(mforms::ContextMenu *header_menu)
  {
    System::Windows::Forms::ContextMenuStrip ^menu = MenuBarWrapper::GetManagedObject<System::Windows::Forms::ContextMenuStrip>(header_menu);
    if (Conversions::UseWin8Drawing())
      menu->Renderer = gcnew Win8MenuStripRenderer();
    else
      menu->Renderer = gcnew TransparentMenuStripRenderer();

    mforms::RecordGrid::set_header_menu(header_menu);
  }
};


public ref class MySQL::Forms::ColumnCallbackWrapper
{
  ConcreteRecordGridView *backend;
public:
  ColumnCallbackWrapper(ConcreteRecordGridView *be)
    : backend(be)
  {
  }

  void resized(int column)
  {
    if (!backend->is_resizing())
      (*backend->signal_column_resized())(column);
  }

  System::Windows::Forms::ContextMenuStrip ^header_right_click(int column)
  {
    backend->clicked_header_column(column);
    backend->header_menu()->will_show();
    
    return MenuBarWrapper::GetManagedObject<System::Windows::Forms::ContextMenuStrip>(backend->header_menu());
  }
};

gcroot<CreateRecordGridDelegate^> RecordGridViewWrapper::factory = nullptr;

RecordGridViewWrapper::RecordGridViewWrapper(mforms::RecordGrid *backend)
: NativeWrapper(backend)
{
}


RecordGridViewWrapper::~RecordGridViewWrapper()
{
}

mforms::RecordGrid *RecordGridViewWrapper::create(boost::shared_ptr<Recordset> rset)
{
  CreateRecordGridDelegate ^create = factory;
  ConcreteRecordGridView *backend = new ConcreteRecordGridView(create(IntPtr(&rset)));
  RecordGridViewWrapper *wrapper = new RecordGridViewWrapper(backend);
  NativeWrapper::ConnectParts(backend, wrapper, backend->control());

  wrapper->column_callback_delegate = gcnew ColumnCallbackWrapper(backend);
  backend->view()->set_column_resize_callback(gcnew IRecordsetView::ColumnResizeCallback(wrapper->column_callback_delegate, &ColumnCallbackWrapper::resized));
  backend->view()->set_column_header_right_click_callback(gcnew IRecordsetView::ColumnHeaderRightClickCallback(wrapper->column_callback_delegate, &ColumnCallbackWrapper::header_right_click));
  return backend;
}


void RecordGridViewWrapper::init(CreateRecordGridDelegate ^creator)
{
  factory = creator;
  mforms::RecordGrid::register_factory(&RecordGridViewWrapper::create);
}

