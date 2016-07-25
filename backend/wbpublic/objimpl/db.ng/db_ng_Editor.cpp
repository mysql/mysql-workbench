/*
* Copyright (c) 2015 Oracle and/or its affiliates. All rights reserved.
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

#include <grts/structs.db.ng.h>
#include "db_ng_Editor.h"
#include <grtpp_util.h>

db_ng_Editor::ImplData::ImplData()
{

}

void db_ng_Editor::init()
{
//  if (!_data) _data= new db_ng_Editor::ImplData();
}

db_ng_Editor::~db_ng_Editor()
{
  //this is deleted by it's owner
 // delete _data;
}


void db_ng_Editor::set_data(ImplData *data)
{
  _data = data;
}

db_mgmt_ConnectionRef db_ng_Editor::connection() const
{
  if (_data)
    return _data->getConnection();
  return db_mgmt_ConnectionRef();
}

grt::StringRef db_ng_Editor::script() const
{
  return grt::StringRef();
}
