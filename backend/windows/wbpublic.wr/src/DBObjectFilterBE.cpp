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

#include "GrtWrapper.h"
#include "GrtManager.h"
#include "GrtStringListModel.h"
#include "DBObjectFilterBE.h"
#include "GrtTemplates.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

      DBObjectFilterBE::DBObjectFilterBE() {
        inner = new bec::DBObjectFilterBE();
      }

      void DBObjectFilterBE::set_object_type_name(String ^ type_name) {
        get_unmanaged_object()->set_object_type_name(NativeToCppString(type_name));
      }

      String ^ DBObjectFilterBE::get_full_type_name() {
        return CppStringToNative(get_unmanaged_object()->get_full_type_name());
      }

      int DBObjectFilterBE::icon_id(MySQL::Grt::IconSize icon_size) {
        return (int)get_unmanaged_object()->icon_id((bec::IconSize)icon_size);
      }

      void DBObjectFilterBE::filter_model(GrtStringListModel ^ filter_model) {
        get_unmanaged_object()->filter_model(filter_model->get_unmanaged_object());
      }

      GrtStringListModel ^ DBObjectFilterBE::filter_model() {
        return gcnew GrtStringListModel(get_unmanaged_object()->filter_model());
      }

      void DBObjectFilterBE::add_stored_filter_set(String ^ name) {
        get_unmanaged_object()->add_stored_filter_set(NativeToCppString(name));
      }

      void DBObjectFilterBE::remove_stored_filter_set(int index) {
        get_unmanaged_object()->remove_stored_filter_set(index);
      }

      void DBObjectFilterBE::load_stored_filter_set(int index) {
        get_unmanaged_object()->load_stored_filter_set(index);
      }

      void DBObjectFilterBE::load_stored_filter_set_list(List<String ^> ^ % names) {
        std::list<std::string> names_ = NativeToCppStringList2(names);
        get_unmanaged_object()->load_stored_filter_set_list(names_);
        names = CppStringListToNative2(names_);
      }

    }; // namespace Db
  };   // namespace Grt
};     // namespace MySQL