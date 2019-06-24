/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __changelistobjects_H
#define __changelistobjects_H

#include <iostream>
#include <assert.h>

#include "diffchange.h"
#include "grtlistdiff.h"

#include "grtpp_util.h"

namespace grt {

  struct pless_struct : public std::function<bool (ValueRef, ValueRef)> { // functor for operator<
    bool operator()(const ValueRef &_Left, const ValueRef &_Right) const;
  };

  //////////////////////////////////////////////////////////////
  // @class
  //////////////////////////////////////////////////////////////
  //
  // @brief
  //
  //////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////

  class MYSQLGRT_PUBLIC ListItemChange : public DiffChange {
    size_t _index;

  public:
    size_t get_index() const {
      return _index;
    };
    ListItemChange(ChangeType atype, size_t index) : DiffChange(atype), _index(index){};
  };

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC ListItemModifiedChange : public ListItemChange {
    std::shared_ptr<DiffChange> subchange;
    ValueRef _old_value;
    ValueRef _new_value;

  public:
    ListItemModifiedChange(const ValueRef old_value, const ValueRef new_value, std::shared_ptr<DiffChange> change,
                           size_t index)
      : ListItemChange(ListItemModified, index), subchange(change), _old_value(old_value), _new_value(new_value) {
      subchange->set_parent(this);
    }

    virtual ValueRef get_old_value() const {
      return _old_value;
    };
    virtual ValueRef get_new_value() const {
      return _new_value;
    };

    const std::shared_ptr<DiffChange> get_subchange() const {
      return subchange;
    }

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name() << std::endl;
      //_subchange->dump_log(level+1);
      subchange->dump_log(level + 1);
    }
  };

  std::shared_ptr<ListItemModifiedChange> create_item_modified_change(const ValueRef &source, const ValueRef &target,
                                                                      const Omf *omf, const size_t index);

  //////////////////////////////////////////////////////////////
  class MYSQLGRT_PUBLIC ListItemAddedChange : public ListItemChange {
    ValueRef _value;
    ValueRef _prev_value;

  public:
    ListItemAddedChange(const ValueRef value, const ValueRef prev_value, size_t index)
      : ListItemChange(ListItemAdded, index), _value(value), _prev_value(prev_value) {
    }

    virtual ValueRef get_value() const {
      return _value;
    };

    // Used in column's AFTER statemen
    grt::ValueRef get_prev_item() const {
      return _prev_value;
    };

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      if (ObjectRef::can_wrap(_value) && ObjectRef::cast_from(_value).has_member("name"))
        std::cout << " name:" << ObjectRef::cast_from(_value).get_string_member("name").c_str();
      std::cout << std::endl;
    }
  };

  //////////////////////////////////////////////////////////////

  class MYSQLGRT_PUBLIC ListItemRemovedChange : public ListItemChange {
    ValueRef _value;

  public:
    ListItemRemovedChange(const ValueRef value, size_t index) : ListItemChange(ListItemRemoved, index), _value(value) {
    }

    virtual ValueRef get_value() const {
      return _value;
    };

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      if (ObjectRef::can_wrap(_value) && ObjectRef::cast_from(_value).has_member("name"))
        std::cout << " name:" << ObjectRef::cast_from(_value).get_string_member("name").c_str() << std::endl;
    }
  };

  //////////////////////////////////////////////////////////////

  class MYSQLGRT_PUBLIC ListItemOrderChange : public ListItemChange {
    std::shared_ptr<ListItemModifiedChange> _subchange;
    grt::ChangeSet cs;
    ValueRef _old_value;
    ValueRef _new_value;
    ValueRef _prev_value;

  public:
    ListItemOrderChange(const ValueRef &source, const ValueRef &target, const Omf *omf, const ValueRef prev_value,
                        size_t index)
      : ListItemChange(ListItemOrderChanged, index), _old_value(source), _new_value(target), _prev_value(prev_value) {
      _subchange = create_item_modified_change(source, target, omf, index);
      if (_subchange)
        _subchange->set_parent(this);
      cs.append(_subchange);
    }

    virtual ValueRef get_old_value() const {
      return _old_value;
    };
    virtual ValueRef get_new_value() const {
      return _new_value;
    };
    std::shared_ptr<ListItemModifiedChange> get_subchange() const {
      return _subchange;
    };

    // Used in column's AFTER statemen
    grt::ValueRef get_prev_item() const {
      return _prev_value;
    };

    virtual const grt::ChangeSet *subchanges() const {
      return &cs;
    }

    void dump_log(int level) const {
      std::cout << std::string(level, ' ');
      std::cout << get_type_name() << std::endl;
      if (_subchange)
        _subchange->dump_log(level + 1);
    }
  };
}
#endif
