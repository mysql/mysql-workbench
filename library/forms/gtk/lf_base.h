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
#ifndef _LF_BASE_H_
#define _LF_BASE_H_

#include <string>
#include <mforms/base.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#include <gtkmm.h>
#pragma GCC diagnostic pop
#include "base/trackable.h"

namespace mforms {
  namespace gtk {

    class ObjectImpl : public sigc::trackable, public base::trackable {
    protected:
      ::mforms::Object *owner;

      ObjectImpl(::mforms::Object *object) : owner(object) {
        object->set_data(this, &free_object);
      }

      virtual ~ObjectImpl() {
        // TODO: Check if we need to free memory here?
      }

      static void free_object(void *obj) {
        delete (ObjectImpl *)obj;
      }
    };
  };
};

#endif
