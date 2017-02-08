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
#ifndef _STUB_TABLE_H_
#define _STUB_TABLE_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class TableWrapper : public ViewWrapper {
    protected:
      TableWrapper(mforms::Table *self) : ViewWrapper(self) {
      }

      static bool create(mforms::Table *self) {
        return true;
      }

      static void set_row_count(Table *self, int count) {
      }

      static void set_col_count(Table *self, int count) {
      }

      static void add(Table *self, View *child, int left, int right, int top, int bottom, int flags) {
      }

      static void remove(Table *self, View *child) {
      }

      static void set_row_spacing(Table *self, int space) {
      }

      static void set_col_spacing(Table *self, int space) {
      }

      static void set_homogeneous(Table *self, bool flag) {
      }

      static void set_back_color(Table *self, const std::string &) {
      }

    public:
      static void init() {
        mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

        f->_table_impl.create = &TableWrapper::create;
        f->_table_impl.set_row_count = &TableWrapper::set_row_count;
        f->_table_impl.set_column_count = &TableWrapper::set_col_count;
        f->_table_impl.add = &TableWrapper::add;
        f->_table_impl.remove = &TableWrapper::remove;
        f->_table_impl.set_row_spacing = &TableWrapper::set_row_spacing;
        f->_table_impl.set_column_spacing = &TableWrapper::set_col_spacing;
        f->_table_impl.set_homogeneous = &TableWrapper::set_homogeneous;
      }

      virtual ~TableWrapper() {
      }
    };
  }
}

#endif /* _STUB_TABLE_H_ */
