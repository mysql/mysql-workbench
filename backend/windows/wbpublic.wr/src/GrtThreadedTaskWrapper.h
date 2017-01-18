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
