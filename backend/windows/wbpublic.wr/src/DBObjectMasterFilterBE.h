/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __DB_OBJECT_MASTER_FILTER_BE_WRAPPER_H__
#define __DB_OBJECT_MASTER_FILTER_BE_WRAPPER_H__

#include "grtdb/db_object_master_filter.h"
#include "DBObjectFilterBE.h"

namespace MySQL {
  namespace Grt {
    namespace Db {

      ref class DBObjectMasterFilterBE;

    public
      ref class DBObjectMasterFilterBE {
      protected:
        ::bec::DBObjectMasterFilterBE *inner;

        DBObjectMasterFilterBE(::bec::DBObjectMasterFilterBE *inn) : inner(inn) {
        }

      public:
        DBObjectMasterFilterBE();

        ~DBObjectMasterFilterBE() {
          delete inner;
        }

        ::bec::DBObjectMasterFilterBE *get_unmanaged_object() {
          return static_cast<::bec::DBObjectMasterFilterBE *>(inner);
        }

        void add_filter(DBObjectFilterBE ^ filter);
        void remove_all_filters();

        void add_stored_filter_set(String ^ name, List<String ^> ^ stored_filter_set_names);
        void remove_stored_filter_set(int index);
        void load_stored_filter_set(int index, List<int> ^ % indexes);
        void load_stored_filter_set_list(List<String ^> ^ % names);
      };

    } // namespace Db
  }   // namespace Grt
} // namespace MySQL

#endif /* __DB_OBJECT_MASTER_FILTER_BE_WRAPPER_H__ */