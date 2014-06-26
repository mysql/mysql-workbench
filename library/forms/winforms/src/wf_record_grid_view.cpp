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


#include "wf_base.h"
#include "wf_view.h"
#include "wf_native.h"
#include "wf_record_grid_view.h"

using namespace MySQL::Forms;
using namespace MySQL::Base;

class ConcreteRecordGridView : public mforms::RecordGrid
{
  gcroot<IRecordsetView^> _view;

public:
  ConcreteRecordGridView(IRecordsetView ^rset)
  {
    _view = rset;
  }

  Control ^control() { return _view->control(); }

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
    _view->set_current_cell(row, column);
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

  return backend;
}


void RecordGridViewWrapper::init(CreateRecordGridDelegate ^creator)
{
  factory = creator;
  mforms::RecordGrid::register_factory(&RecordGridViewWrapper::create);
}

