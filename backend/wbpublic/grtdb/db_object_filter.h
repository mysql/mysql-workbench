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

#ifndef _DB_OBJECT_FILTER_BE_H_
#define _DB_OBJECT_FILTER_BE_H_

#include "grt/icon_manager.h"
#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"

#include "wbpublic_public_interface.h"

namespace bec {

  class GrtStringListModel;

  class WBPUBLICBACKEND_PUBLIC_FUNC DBObjectFilterBE {
  public:
    DBObjectFilterBE();
    virtual ~DBObjectFilterBE(){};

    virtual void set_object_type_name(const std::string &type_name);
    virtual const std::string &get_full_type_name() const;
    bec::IconId icon_id(bec::IconSize icon_size);

    void filter_model(GrtStringListModel *filter_model) {
      _filter_model = filter_model;
    }
    GrtStringListModel *filter_model() {
      return _filter_model;
    }
    void add_stored_filter_set(const std::string &name);
    void remove_stored_filter_set(int index);
    void load_stored_filter_set(int index);
    int stored_filter_set_index(const std::string &name);
    void load_stored_filter_set_list(std::list<std::string> &names);

  protected:
    std::string _grt_type_name;
    std::string _full_type_name;
    grt::DictRef _stored_filter_sets;
    std::string _stored_filter_sets_filepath;
    GrtStringListModel *_filter_model;
  };
};

#endif /* _DB_OBJECT_FILTER_BE_H_ */
