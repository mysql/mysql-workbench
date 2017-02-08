/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _DB_OBJECT_MASTER_FILTER_BE_H_
#define _DB_OBJECT_MASTER_FILTER_BE_H_

#include "grts/structs.db.h"
#include "grts/structs.db.mgmt.h"

#include "wbpublic_public_interface.h"

namespace bec {

  class DBObjectFilterBE;

  class WBPUBLICBACKEND_PUBLIC_FUNC DBObjectMasterFilterBE {
  public:
    DBObjectMasterFilterBE();

    void add_filter(DBObjectFilterBE *filter);
    void remove_all_filters();

    void add_stored_filter_set(const std::string &name, std::list<std::string> &names);
    void remove_stored_filter_set(int index);
    void load_stored_filter_set(int index, std::list<int> &indexes);
    void load_stored_filter_set_list(std::list<std::string> &names);

  protected:
    std::vector<DBObjectFilterBE *> _filters;
    grt::DictRef _stored_master_filter_sets;
    std::string _stored_master_filter_sets_filepath;
  };
};

#endif /* _DB_OBJECT_MASTER_FILTER_BE_H_ */
