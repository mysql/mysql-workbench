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

#include "GrtTemplates.h"
#include "DelegateWrapper.h"

#include "GrtThreadedTaskWrapper.h"

namespace MySQL {
  namespace Grt {

    using namespace System;
    using namespace System::Collections::Generic;

    GrtThreadedTaskWrapper::~GrtThreadedTaskWrapper() {
      if (!_inner)
        return;
      _inner->disconnect_callbacks();
      delete _progress_cb;
      // delete _finish_cb;
      _inner = NULL;
    }

    void GrtThreadedTaskWrapper::progress_cb(Progress_cb::ManagedDelegate ^ deleg) {
      _progress_cb = gcnew Progress_cb(deleg);
      _inner->progress_cb(_progress_cb->get_slot());
    }

    /*
    void GrtThreadedTaskWrapper::finish_cb(Finish_cb::ManagedDelegate ^deleg)
    {
      _finish_cb= gcnew Finish_cb(deleg);
      _inner->finish_cb(_finish_cb->get_slot());
    }*/

  }; // namespace Grt
};   // namespace MySQL
