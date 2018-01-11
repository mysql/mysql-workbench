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

#pragma once

#include "grt.h"

#include "tree_model.h"
#include "refresh_ui.h"

#define ValueInspectorBE_VERSION 2

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC ValueInspectorBE : public TreeModel, public RefreshUI {
  public:
    enum ValueInspectorColumns {
      Name,        // or group name
      Value,       // not valid for groups
      Description, // description
      IsReadonly,  // determines if given object member is editable
      EditMethod   // only for objects, defined with attr:editas in the struct xml
    };

    static ValueInspectorBE *create(const grt::ValueRef &value, bool grouped, bool process_editas_flag);

    static ValueInspectorBE *create(const std::vector<grt::ObjectRef> &objects);

    virtual bool add_item(NodeId &new_node) = 0;
    virtual bool delete_item(const NodeId &node) = 0;

    // virtual MYX_GRT_VALUE_TYPE get_field_type(const NodeId &node, ColumnId column)= 0;

    virtual grt::ValueRef get_grt_value(const NodeId &node, ColumnId column);

    virtual bool set_convert_field(const NodeId &node, ColumnId column, const std::string &value);
    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);
    virtual bool set_field(const NodeId &node, ColumnId column, double value);
    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);

    virtual IconId get_field_icon(const NodeId &node, ColumnId column, IconSize size);

  public: // Responder methods
  protected:
    ValueInspectorBE();

    virtual grt::Type get_canonical_type(const NodeId &node) = 0;

    virtual bool set_value(const NodeId &node, const grt::ValueRef &value) = 0;

    void monitor_object_changes(const grt::ObjectRef &obj);

  private:
    void changed_slot(const std::string &name, const grt::ValueRef &value);
    boost::signals2::scoped_connection _changed_conn;
  };
};
