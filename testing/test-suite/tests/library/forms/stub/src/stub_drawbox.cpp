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

#include "../stub_drawbox.h"

namespace mforms {
  namespace stub {

    DrawBoxWrapper::DrawBoxWrapper(mforms::DrawBox *self) : ViewWrapper(self) {
    }

    void DrawBoxWrapper::set_size(int width, int height) {
    }

    bool DrawBoxWrapper::create(mforms::DrawBox *self) {
      return true;
    }

    void DrawBoxWrapper::set_needs_repaint(mforms::DrawBox *self) {
    }

    void DrawBoxWrapper::setNeedsRepaintArea(DrawBox *, int x, int y, int w, int h) {

    }

    void DrawBoxWrapper::add(mforms::DrawBox *, mforms::View *, mforms::Alignment alignment) {
    }

    void DrawBoxWrapper::remove(mforms::DrawBox *, mforms::View *) {
    }

    void DrawBoxWrapper::move(mforms::DrawBox *, mforms::View *, int x, int y) {
    }

    void DrawBoxWrapper::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_drawbox_impl.create = &DrawBoxWrapper::create;
      f->_drawbox_impl.set_needs_repaint = &DrawBoxWrapper::set_needs_repaint;
      f->_drawbox_impl.set_needs_repaint_area = &DrawBoxWrapper::setNeedsRepaintArea;
      f->_drawbox_impl.add = &DrawBoxWrapper::add;
      f->_drawbox_impl.remove = &DrawBoxWrapper::remove;
      f->_drawbox_impl.move = &DrawBoxWrapper::move;
    }
  };
};
