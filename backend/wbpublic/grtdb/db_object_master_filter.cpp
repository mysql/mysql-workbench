/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/grt_manager.h"
#include "db_object_master_filter.h"
#include "db_object_filter.h"

using namespace bec;

DBObjectMasterFilterBE::DBObjectMasterFilterBE() {
  // load stored filter sets
  grt::DictRef opt = grt::DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
  _stored_master_filter_sets_filepath.append(bec::GRTManager::get()->get_user_datadir())
    .append("/stored_master_filter_sets.xml");
  if (g_file_test(_stored_master_filter_sets_filepath.c_str(), G_FILE_TEST_EXISTS))
    _stored_master_filter_sets =
      grt::DictRef::cast_from(grt::GRT::get()->unserialize(_stored_master_filter_sets_filepath));
  if (!_stored_master_filter_sets.is_valid())
    _stored_master_filter_sets = grt::DictRef(true);
}

void DBObjectMasterFilterBE::add_filter(DBObjectFilterBE *filter) {
  _filters.push_back(filter);
}

void DBObjectMasterFilterBE::remove_all_filters() {
  _filters.clear();
}

void DBObjectMasterFilterBE::add_stored_filter_set(const std::string &name, std::list<std::string> &names) {
  if (_filters.empty())
    return;

  grt::DictRef stored_filter_sets(true);
  _stored_master_filter_sets.set(name, stored_filter_sets);

  {
    std::list<std::string>::iterator i = names.begin();
    std::list<std::string>::iterator i_end = names.end();
    std::vector<DBObjectFilterBE *>::iterator f = _filters.begin();
    std::vector<DBObjectFilterBE *>::iterator f_end = _filters.end();
    for (; f != f_end && i != i_end; ++f, ++i)
      stored_filter_sets.gset((*f)->get_full_type_name(), *i);
  }

  grt::GRT::get()->serialize(_stored_master_filter_sets, _stored_master_filter_sets_filepath);
}

void DBObjectMasterFilterBE::remove_stored_filter_set(int index) {
  /*QQQ
  std::string key;
  grt::DictRef filter_set_names;
  if (!_stored_master_filter_sets.get_by_index(index, key, filter_set_names))
    return;
  _stored_master_filter_sets.remove(key);

  grt::GRT::get()->serialize(_stored_master_filter_sets, _stored_master_filter_sets_filepath);
  */
  throw std::logic_error("needs update");
}

void DBObjectMasterFilterBE::load_stored_filter_set(int index, std::list<int> &indexes) {
  throw std::logic_error("needs update");
  /*QQQ
  if (_filters.empty())
    return;

  = _grtm->get_grt();

  std::string key;
  grt::DictRef filter_set_indexes(true);

  _stored_master_filter_sets.get_by_index(index, key, filter_set_indexes);

  for (std::vector<DBObjectFilterBE *>::iterator f= _filters.begin(), f_end= _filters.end(); f != f_end; ++f)
  {
    std::string name= filter_set_indexes.get_string((*f)->get_full_type_name());
    DBObjectFilterBE *filter= *f;
    int index= filter->stored_filter_set_index(name);
    filter->load_stored_filter_set(index);

    indexes.push_back(index);
  }
  */
}

void DBObjectMasterFilterBE::load_stored_filter_set_list(std::list<std::string> &names) {
  ///*QQQ
  std::string key;
  grt::DictRef stored_filter_sets;

  for (grt::DictRef::const_iterator it = _stored_master_filter_sets.begin(); it != _stored_master_filter_sets.end();
       it++) {
    names.push_back(it->second.toString());
  }

  names.push_back(std::string()); // empty value, denoting empty filter set
}
