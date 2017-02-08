/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __GRT_VALUE_INSPECTOR_H__
#define __GRT_VALUE_INSPECTOR_H__

#include "grt/grt_value_inspector.h"
#include "ModelWrappers.h"

namespace MySQL {
  namespace Grt {

  public
    ref class GrtValueInspector : TreeModelWrapper {
      bool free_inner;

    public:
      enum class Columns {
        Name = ::bec::ValueInspectorBE::Name,
        Value = ::bec::ValueInspectorBE::Value,
        Description = ::bec::ValueInspectorBE::Description,
        IsReadonly = ::bec::ValueInspectorBE::IsReadonly,
        EditMethod = ::bec::ValueInspectorBE::EditMethod
      };

      static GrtValueInspector ^
        Create(GRT ^ grt, GrtValue ^ value, bool grouped, bool process_editas_flag) {
          return gcnew GrtValueInspector(
            ::bec::ValueInspectorBE::create(value->get_unmanaged_object(), grouped, process_editas_flag), true);
        }

        GrtValueInspector(::bec::ValueInspectorBE *inn, bool free)
        : TreeModelWrapper(inn),
        free_inner(free) {
      }

      ~GrtValueInspector() {
        if (free_inner)
          delete inner;
      }

      inline ::bec::ValueInspectorBE *get_unmanaged_object() {
        return static_cast<::bec::ValueInspectorBE *>(inner);
      }

      virtual bool add_item([Out] NodeIdWrapper ^ % new_node) {
        ::bec::NodeId node;
        bool retval = get_unmanaged_object()->add_item(node);
        new_node = gcnew NodeIdWrapper(new ::bec::NodeId(node));
        return retval;
      }

      virtual bool delete_item(NodeIdWrapper ^ node) {
        return get_unmanaged_object()->delete_item(*node->get_unmanaged_object());
      }

      // virtual _clr_MYX_GRT_VALUE_TYPE get_value_type(NodeId^ node)
      //  { return
      //  static_cast<_clr_MYX_GRT_VALUE_TYPE>(get_unmanaged_object()->get_value_type(*node->get_unmanaged_object())); }
      virtual GrtValueType get_field_type(NodeIdWrapper ^ node, GrtValueInspector::Columns column) {
        return static_cast<GrtValueType>(
          get_unmanaged_object()->get_field_type(*node->get_unmanaged_object(), static_cast<int>(column)));
      }

      //  protected:
      //    virtual bool set_value(const NodeId &node, const grt::GenericValue &value)
    };

  } // namespace Grt
} // namespace MySQL

#endif // __GRT_VALUE_TREE_H__