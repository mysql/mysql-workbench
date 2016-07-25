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
#include "db_ng_Ide.h"
#include <grtpp_util.h>

db_ng_Ide::ImplData::ImplData()
{
};


void db_ng_Ide::init()
{
//  if (!_data) _data= new db_ng_Ide::ImplData();
}

db_ng_Ide::~db_ng_Ide()
{
  delete _data;
}

void db_ng_Ide::set_data(ImplData *data)
{
  _data = data;
}

db_ng_SheetRef db_ng_Ide::activeNgSheet() const
{
  if (_data)
    return _data->activeNgSheet();

  return db_ng_SheetRef();
}



