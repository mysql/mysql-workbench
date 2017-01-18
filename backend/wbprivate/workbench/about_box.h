/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _ABOUT_BOX_H_
#define _ABOUT_BOX_H_

#include "wb_backend_public_interface.h"

#include "mforms/popup.h"

namespace wb {
  /**
   * A simple about box popup.
   */
  class MYSQLWBBACKEND_PUBLIC_FUNC AboutBox : public mforms::Popup {
  private:
    cairo_surface_t *_back_image;
    float _scale_factor;
    std::string _edition;

  protected:
    void repaint(cairo_t *cr, int x, int y, int w, int h);
    bool mouse_up(mforms::MouseButton button, int x, int y);

    static void closed();

  public:
    AboutBox(const std::string &edition);
    ~AboutBox();

    static void show_about(const std::string &edition);
  };
}

#endif // _ABOUT_BOX_H_
