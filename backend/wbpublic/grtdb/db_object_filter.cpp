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

#include "db_object_filter.h"
#include "grt/grt_string_list_model.h"
#include "grt/grt_manager.h"

using namespace bec;

DBObjectFilterBE::DBObjectFilterBE() : _filter_model(NULL) {
}

void DBObjectFilterBE::set_object_type_name(const std::string &type_name) {
  _grt_type_name = type_name;

  if (type_name.empty()) {
    _full_type_name = type_name;
    return;
  }

  grt::MetaClass *meta = grt::GRT::get()->get_metaclass(type_name);
  if (!meta)
    throw grt::bad_class(type_name);

  grt::ObjectRef obj = meta->allocate();
  _full_type_name = meta->get_attribute("caption");

  // load stored filter sets
  grt::DictRef opt = grt::DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));
  _stored_filter_sets_filepath.append(bec::GRTManager::get()->get_user_datadir())
    .append("/stored_filter_sets.")
    .append(_full_type_name)
    .append(".xml");
  if (g_file_test(_stored_filter_sets_filepath.c_str(), G_FILE_TEST_EXISTS))
    _stored_filter_sets = grt::DictRef::cast_from(grt::GRT::get()->unserialize(_stored_filter_sets_filepath));
  if (!_stored_filter_sets.is_valid())
    _stored_filter_sets = grt::DictRef(true);
}

const std::string &DBObjectFilterBE::get_full_type_name() const {
  return _full_type_name;
}

bec::IconId DBObjectFilterBE::icon_id(bec::IconSize icon_size) {
  if (!_grt_type_name.empty()) {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(_grt_type_name);
    if (!meta)
      throw grt::bad_class(_grt_type_name);
    return bec::IconManager::get_instance()->get_icon_id(meta, icon_size, "many");
  }
  return 0;
}

void DBObjectFilterBE::add_stored_filter_set(const std::string &name) {
  if (!_filter_model)
    return;

  grt::StringListRef masks(grt::Initialized);
  _stored_filter_sets.set(name, masks);

  std::vector<std::string> items = _filter_model->items();
  for (std::vector<std::string>::iterator i = items.begin(); i != items.end(); ++i)
    masks.insert(*i);

  grt::GRT::get()->serialize(_stored_filter_sets, _stored_filter_sets_filepath);
}

void DBObjectFilterBE::remove_stored_filter_set(int index) {
  if (index < 0 || index >= (int)_stored_filter_sets.count())
    return;

  grt::DictRef::const_iterator item = _stored_filter_sets.begin();
  while (item != _stored_filter_sets.end() && index > 0) {
    ++item; --index;
  }

  if (item != _stored_filter_sets.end())
    _stored_filter_sets.remove(item->first);

  grt::GRT::get()->serialize(_stored_filter_sets, _stored_filter_sets_filepath);
}

void DBObjectFilterBE::load_stored_filter_set(int index) {
  if (!_filter_model)
    return;

  grt::StringListRef masks;

  grt::DictRef::const_iterator item = _stored_filter_sets.begin();
  while (item != _stored_filter_sets.end() && index > 0) {
    ++item; --index;
  }

  if (item != _stored_filter_sets.end()) {
    masks = grt::StringListRef::cast_from(item->second);

    std::list<std::string> items;
    for (size_t n = 0, count = masks.count(); n < count; ++n)
      items.push_back(*masks.get(n));
    _filter_model->reset(items);
  }
}

int DBObjectFilterBE::stored_filter_set_index(const std::string &name) {
  if (!_filter_model)
    return -1;

  int n = 0;

  for (grt::DictRef::const_iterator item = _stored_filter_sets.begin(); item != _stored_filter_sets.end();
       ++item, n++) {
    if (item->first == name)
      return n;
  }

  return (int)_stored_filter_sets.count();
}

void DBObjectFilterBE::load_stored_filter_set_list(std::list<std::string> &names) {
  grt::StringListRef masks;

  for (grt::DictRef::const_iterator item = _stored_filter_sets.begin(); item != _stored_filter_sets.end(); ++item) {
    names.push_back(item->first);
  }

  names.push_back(std::string()); // empty value, denoting empty filter set
}
