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

#ifndef __changeobjects_H
#define __changeobjects_H

#include <iostream>
#include <assert.h>
#include "diffchange.h"
#include "../grtpp_util.h"

namespace grt {

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC ObjectAttrModifiedChange : public DiffChange {
    std::string _attr;
    std::shared_ptr<DiffChange> subchange;

  public:
    ObjectAttrModifiedChange(const std::string& attr, std::shared_ptr<DiffChange> change)
      : DiffChange(ObjectAttrModified), _attr(attr), subchange(change) {
      subchange->set_parent(this);
    }

    const std::string& get_attr_name() const {
      return _attr;
    }
    const std::shared_ptr<DiffChange> get_subchange() const {
      return subchange;
    }

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name() << "::" << _attr << std::endl;
      subchange->dump_log(level + 1);
    }
  };

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC ValueAddedChange : public DiffChange {
    ValueRef _v;
    bool _free_value;

  public:
    const ValueRef get_value() const {
      return _v;
    }

    ValueAddedChange(ChangeType type, ValueRef v, bool dupvalue = true)
      : DiffChange(type), _v(dupvalue ? copy_value(v, true) : v), _free_value(dupvalue) {
    }

    virtual ~ValueAddedChange() {
      if (_free_value && _v.is_valid())
        _v.valueptr()->reset_references();
    }
  };

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC ValueRemovedChange : public DiffChange {
  public:
    ValueRemovedChange() : DiffChange(ValueRemoved) {
    }
  };

  class MYSQLGRT_PUBLIC DictItemModifiedChange : public DiffChange {
    std::string key;
    std::shared_ptr<DiffChange> subchange;

  public:
    DictItemModifiedChange(const std::string& i, std::shared_ptr<DiffChange> change)
      : DiffChange(DictItemModified), key(i), subchange(change) {
      subchange->set_parent(this);
    }

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name() << "::" << key << std::endl;
      subchange->dump_log(level + 1);
    }
  };

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC DictItemAddedChange : public DiffChange {
    ValueRef _v;
    std::string key;
    bool _free_values;

  public:
    DictItemAddedChange(const std::string& i, ValueRef v, bool dupvalue = true)
      : DiffChange(DictItemAdded), _v(dupvalue ? copy_value(v, true) : v), key(i), _free_values(dupvalue) {
    }

    virtual ~DictItemAddedChange() {
      if (_free_values && _v.is_valid())
        _v.valueptr()->reset_references();
    }

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name() << "::" << key << std::endl;
    }
  };

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC DictItemRemovedChange : public DiffChange {
    std::string key;

  public:
    DictItemRemovedChange(const std::string& i) : DiffChange(DictItemRemoved), key(i) {
    }

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name() << "::" << key << std::endl;
    }
  };

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC SimpleValueChange : public DiffChange {
    ValueRef _old;
    ValueRef _v;

  public:
    SimpleValueChange(ValueRef old, ValueRef v) : DiffChange(SimpleValue), _old(old), _v(v) {
    }

    ValueRef get_new_value() const {
      return _v;
    }
    ValueRef get_old_value() const {
      return _old;
    }

    virtual void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name();
      std::cout << " new:" << _v.debugDescription();
      std::cout << " old:" << _old.debugDescription() << std::endl;
    }
  };

  //////////////////////////////////////////////////////////////
}
#endif
