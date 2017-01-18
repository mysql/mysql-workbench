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

#ifndef __REFRESH_UI_WR_H__
#define __REFRESH_UI_WR_H__

#include "grt/refresh_ui.h"
#include "DelegateWrapper.h"

namespace MySQL {
  namespace Grt {

    //! Class to save on typing and copy/paste of code. If you need to
    //! have a refresh feedback from BE to FE just derive from RefreshUI.
  public
    ref class RefreshUI {
      ::bec::RefreshUI *_inner;

    public:
      RefreshUI(::bec::RefreshUI *inner) : _inner(inner) {
      }

    public:
      void block_auto_refresh() {
        _inner->block_auto_refresh();
      }
      void unblock_auto_refresh() {
        _inner->unblock_auto_refresh();
      }

    public:
      typedef DelegateSlot0<void, void> RefreshSlot;
      void set_refresh_slot(RefreshSlot::ManagedDelegate ^ cb);

    private:
      RefreshSlot ^ _refresh_slot;

    public:
      typedef DelegateSlot1<void, void, int, int> PartialRefreshSlot;
      void set_partial_refresh_slot(PartialRefreshSlot::ManagedDelegate ^ cb);

    private:
      PartialRefreshSlot ^ _partial_refresh_slot;
    };
  }
}

#endif
