/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _STUB_APP_H_
#define _STUB_APP_H_

#include "stub_base.h"

#include "mforms/app.h"

namespace mforms {
  namespace stub {

    class AppWrapper : public ObjectWrapper {
      static void set_status_text(App *app, const std::string &title) {
      }

      static std::string get_resource_path(App *app, const std::string &file);

      static base::Rect get_application_bounds(App *app) {
        return base::Rect();
      }

      static int enter_event_loop(App *app, float max_wait_time) {
        return 0;
      }
      static void exit_event_loop(App *app, int result) {
      }

    public:
      static void init(wb::WBOptions *theOptions);
    };
  };
};

#endif
