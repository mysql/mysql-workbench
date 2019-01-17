/*
 * Copyright (c) 2017, 2019 Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.db.mgmt.h"

#include "db_mgmt_SSHFile.h"
#include "grtpp_util.h"

db_mgmt_SSHFile::ImplData::ImplData() {

}

//------------------------------------------------------------------------------------------------

db_mgmt_SSHFile::ImplData::~ImplData() {

}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHFile::init() {
//  if (!_data)
//    _data = new db_mgmt_SSHFile::ImplData();
}

//------------------------------------------------------------------------------------------------

db_mgmt_SSHFile::~db_mgmt_SSHFile() {
  delete _data;
}

//------------------------------------------------------------------------------------------------

void db_mgmt_SSHFile::set_data(ImplData *data) {
  _data = data;
}

//------------------------------------------------------------------------------------------------

grt::StringRef db_mgmt_SSHFile::getPath() {
  if (_data)
    return _data->getPath();
  return "";
}

//------------------------------------------------------------------------------------------------

grt::StringRef db_mgmt_SSHFile::read(ssize_t length) {
  if (_data)
    return _data->read(length);
  return "";
}

//------------------------------------------------------------------------------------------------

grt::StringRef db_mgmt_SSHFile::readline() {
  if (_data)
    return _data->readline();
  return "";
}

//------------------------------------------------------------------------------------------------

grt::IntegerRef db_mgmt_SSHFile::seek(ssize_t offset) {
  if (_data)
    return _data->seek(offset);
  return -1;
}

//------------------------------------------------------------------------------------------------

grt::IntegerRef db_mgmt_SSHFile::tell() {
  if (_data)
    return _data->tell();
  return -1;
}

//------------------------------------------------------------------------------------------------
