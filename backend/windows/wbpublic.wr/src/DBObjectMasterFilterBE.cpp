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

#include "GrtWrapper.h"
#include "GrtManager.h"
#include "GrtStringListModel.h"
#include "DBObjectMasterFilterBE.h"
#include "GrtTemplates.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

      DBObjectMasterFilterBE::DBObjectMasterFilterBE() {
        inner = new bec::DBObjectMasterFilterBE();
      }

      void DBObjectMasterFilterBE::add_filter(DBObjectFilterBE ^ filter) {
        get_unmanaged_object()->add_filter(filter->get_unmanaged_object());
      }

      void DBObjectMasterFilterBE::remove_all_filters() {
        get_unmanaged_object()->remove_all_filters();
      }

      void DBObjectMasterFilterBE::add_stored_filter_set(String ^ name, List<String ^> ^ stored_filter_set_names) {
        std::list<std::string> stored_filter_set_names_ = NativeToCppStringList2(stored_filter_set_names);
        get_unmanaged_object()->add_stored_filter_set(NativeToCppString(name), stored_filter_set_names_);
      }

      void DBObjectMasterFilterBE::remove_stored_filter_set(int index) {
        get_unmanaged_object()->remove_stored_filter_set(index);
      }

      void DBObjectMasterFilterBE::load_stored_filter_set(int index, List<int> ^ % indexes) {
        std::list<int> indexes_ = NativeListToCppList<int, int>(indexes);
        get_unmanaged_object()->load_stored_filter_set(index, indexes_);
        indexes = CppListToNativeList<int, int>(indexes_);
      }

      void DBObjectMasterFilterBE::load_stored_filter_set_list(List<String ^> ^ % names) {
        std::list<std::string> names_ = NativeToCppStringList2(names);
        get_unmanaged_object()->load_stored_filter_set_list(names_);
        names = CppStringListToNative2(names_);
      }

    }; // namespace Db
  };   // namespace Grt
};     // namespace MySQL