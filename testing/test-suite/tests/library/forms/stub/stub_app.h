/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

      static std::string getExecutablePath(App *app, const std::string &file) {
        return "";
      }

      static float backingScaleFactor(App *app) {
        return 0.0;
      }

    public:
      static void init(wb::WBOptions *theOptions);
    };
  };
};

#endif
