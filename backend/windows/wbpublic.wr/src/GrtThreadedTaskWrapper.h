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

#ifndef __GRT_THREADED_TASK_WR_H__
#define __GRT_THREADED_TASK_WR_H__

#include "grt/grt_threaded_task.h"

namespace MySQL {
  namespace Grt {

  public
    ref class GrtThreadedTaskWrapper {
    private:
      ::GrtThreadedTask *_inner;

    public:
      GrtThreadedTaskWrapper(::GrtThreadedTask *inner) : _inner(inner) {
      }

    private:
      ~GrtThreadedTaskWrapper();

    public:
      typedef DelegateSlot2<int, int, float, float, std::string, String ^> Progress_cb;
      typedef DelegateSlot0<int, int> Finish_cb;

      void progress_cb(Progress_cb::ManagedDelegate ^ cb);
      // void finish_cb(Finish_cb::ManagedDelegate ^cb);

    private:
      Progress_cb ^ _progress_cb;
      // Finish_cb ^_finish_cb;
    };

  }; // namespace Grt
};   // namespace MySQL

#endif // __GRT_THREADED_TASK_WR_H__
