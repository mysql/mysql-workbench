#include "stdafx.h"

#include <grts/structs.db.h>

#include <grtpp_util.h>


//================================================================================
// db_Index


void db_Index::init()
{

}

db_Index::~db_Index()
{
  
}


void db_Index::name(const grt::StringRef &value)
{
  grt::ValueRef ovalue(_name);

  if (_owner.is_valid() && _name != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("index");

  _name= value;
  member_changed("name", ovalue, value);
}




