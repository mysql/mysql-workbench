
#include "stdafx.h"

#include <grts/structs.db.h>

#include <grtpp_util.h>


//================================================================================
// db_Trigger


void db_Trigger::init()
{

}

db_Trigger::~db_Trigger()
{
  
}


void db_Trigger::event(const grt::StringRef &value)
{
  grt::ValueRef ovalue(_event);

  if (_owner.is_valid() && _event != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("trigger");

  _event= value;
  member_changed("event", ovalue, value);
}

void db_Trigger::name(const grt::StringRef &value)
{
  grt::ValueRef ovalue(_name);
  
  if (_owner.is_valid() && _name != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("trigger");

  _name= value;
  member_changed("name", ovalue, value);
}

void db_Trigger::timing(const grt::StringRef &value)
{
  grt::ValueRef ovalue(_timing);

  if (_owner.is_valid() && _timing != value)
    (*db_TableRef::cast_from(_owner)->signal_refreshDisplay())("trigger");

  _timing= value;
  member_changed("timing", ovalue, value);
}




