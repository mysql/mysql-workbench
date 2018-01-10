/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __CHANGE_FACTORY_H
#define __CHANGE_FACTORY_H

namespace grt {
  class DiffChange;
  class DictRef;
  class BaseListRef;
  class MultiChange;
  struct ChangeSet;

  struct ChangeFactory {
    static std::shared_ptr<DiffChange> create_value_added_change(std::shared_ptr<DiffChange> parent,
                                                                 const ValueRef &source, const ValueRef &target,
                                                                 bool dupvalue = true);
    static std::shared_ptr<DiffChange> create_value_removed_change(std::shared_ptr<DiffChange> parent,
                                                                   const ValueRef &source, const ValueRef &target);

    static std::shared_ptr<DiffChange> create_object_attr_modified_change(std::shared_ptr<DiffChange> parent,
                                                                          const ObjectRef &source,
                                                                          const ObjectRef &target,
                                                                          const std::string &attr,
                                                                          std::shared_ptr<DiffChange> change);
    static std::shared_ptr<MultiChange> create_object_modified_change(std::shared_ptr<DiffChange> parent,
                                                                      const ObjectRef &source, const ObjectRef &target,
                                                                      ChangeSet &changes);

    static std::shared_ptr<MultiChange> create_dict_change(std::shared_ptr<DiffChange> parent, const DictRef &source,
                                                           const DictRef &target, ChangeSet &changes);

    static std::shared_ptr<DiffChange> create_dict_item_added_change(std::shared_ptr<DiffChange> parent,
                                                                     const DictRef &source, const DictRef &target,
                                                                     const std::string &key, ValueRef v,
                                                                     bool dupvalue = true);
    static std::shared_ptr<DiffChange> create_dict_item_modified_change(std::shared_ptr<DiffChange> parent,
                                                                        const DictRef &source, const DictRef &target,
                                                                        const std::string &key,
                                                                        std::shared_ptr<DiffChange> change);
    static std::shared_ptr<DiffChange> create_dict_item_removed_change(std::shared_ptr<DiffChange> parent,
                                                                       const DictRef &source, const DictRef &target,
                                                                       const std::string &key);

    static std::shared_ptr<DiffChange> create_simple_value_change(std::shared_ptr<DiffChange> parent,
                                                                  const ValueRef &source, const ValueRef &target);
  };
}

#endif
