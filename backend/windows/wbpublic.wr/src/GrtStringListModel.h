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

#ifndef __GRT_STRING_LIST_MODEL_H__
#define __GRT_STRING_LIST_MODEL_H__

#include "grt/grt_string_list_model.h"
#include "GrtWrapper.h"
#include "IconManager.h"
#include "ModelWrappers.h"
#include "GrtTemplates.h"

namespace MySQL {
  namespace Grt {

  public
    ref class GrtStringListModel : public MySQL::Grt::ListModelWrapper {
    public:
      enum class Columns { Name = ::bec::GrtStringListModel::Name };

      GrtStringListModel(::bec::GrtStringListModel *inn) : MySQL::Grt::ListModelWrapper(inn), free_inner(false) {
      }

      GrtStringListModel() : MySQL::Grt::ListModelWrapper(new ::bec::GrtStringListModel()), free_inner(true) {
      }

      ~GrtStringListModel() {
        if (free_inner)
          delete inner;
      }

      ::bec::GrtStringListModel *get_unmanaged_object() {
        return static_cast<::bec::GrtStringListModel *>(inner);
      }

      void add_item(String ^ item, int id) {
        get_unmanaged_object()->add_item(NativeToCppString(item), id);
      }

      void remove_items(List<int> ^ item_indexes) {
        get_unmanaged_object()->remove_items(IntListToCppVector2(item_indexes));
      }

      void remove_item(int index) {
        get_unmanaged_object()->remove_item(index);
      }

      void copy_items_to_val_masks_list(List<int> ^ item_indexes) {
        get_unmanaged_object()->copy_items_to_val_masks_list(IntListToCppVector2(item_indexes));
      }

      void items_val_mask(String ^ items_val_mask) {
        get_unmanaged_object()->items_val_mask(NativeToCppString(items_val_mask));
      }

      void invalidate() {
        get_unmanaged_object()->invalidate();
      }

      int total_items_count() {
        return (int)get_unmanaged_object()->total_items_count();
      }

      int active_items_count() {
        return (int)get_unmanaged_object()->active_items_count();
      }

    private:
      bool free_inner;
    };

  } // namespace Grt
} // namespace MySQL

#endif // __GRT_STRING_LIST_MODEL_H__
