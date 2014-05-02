#include "stdafx.h"

#include <grts/structs.workbench.physical.h>

#include <grtpp_util.h>

#include "wbcanvas/workbench_physical_connection_impl.h"

//================================================================================
// workbench_physical_Connection


void workbench_physical_Connection::init()
{
  if (!_data) _data= new workbench_physical_Connection::ImplData(this);
  model_Connection::set_data(_data);
}

void workbench_physical_Connection::set_data(ImplData *data)
{
  throw std::logic_error("unexpected");
}

workbench_physical_Connection::~workbench_physical_Connection()
{
  delete _data;
}


void workbench_physical_Connection::foreignKey(const db_ForeignKeyRef &value)
{
  if (_foreignKey == value) return;
  if (_foreignKey.is_valid() && value.is_valid())
    throw std::runtime_error("Cannot change foreignKey field of connection after its set");

  if (_is_global && _foreignKey.is_valid()) _foreignKey.unmark_global();
  if (_is_global && value.is_valid()) value.mark_global();

  grt::ValueRef ovalue(_foreignKey);
  get_data()->set_foreign_key(value);
  member_changed("foreignKey", ovalue, value);
}




