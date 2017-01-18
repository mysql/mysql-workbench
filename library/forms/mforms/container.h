/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "mforms/view.h"

namespace mforms {

  /** Intermediate class used to collectively check for containers and later maybe implement
   *  special container functionality.
   *  Not represented in platform code.
   */
  class MFORMS_EXPORT Container : public View {
  public:
    Container() {
    }

    virtual void set_padding(int left, int top, int right, int bottom);
    void set_padding(int padding);

    virtual void set_back_image(const std::string& path, Alignment alignment) {
      View::set_back_image(path, alignment);
    }
  };
};
