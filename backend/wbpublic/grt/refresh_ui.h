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
#ifndef __REFRESH_UI_H__
#define __REFRESH_UI_H__

#include "wbpublic_public_interface.h"
#include <functional>

namespace bec {

  //! Class to save on typing and copy/paste of code. If you need to
  //! have a refresh feedback from BE to FE just derive from RefreshUI.
  class WBPUBLICBACKEND_PUBLIC_FUNC RefreshUI {
  public:
    RefreshUI() {
      unblock_auto_refresh();
    }
    virtual ~RefreshUI() {
    }

    typedef std::function<void()> RefreshSlot;
    typedef std::function<void(const int)> PartialRefreshSlot;

    void set_refresh_ui_slot(const RefreshSlot &slot);

    // Refreshes a part of the editor. This is preferred over a full UI refresh and should hence
    // be used most of the time.
    void set_partial_refresh_ui_slot(const PartialRefreshSlot &slot);

    void do_partial_ui_refresh(const int what);
    void do_ui_refresh();

    struct Blocker {
      Blocker(RefreshUI &o) : _obj(&o) {
        _obj->block_auto_refresh();
      }
      ~Blocker() {
        _obj->unblock_auto_refresh();
      }

      RefreshUI *_obj;
    };

    void block_auto_refresh() {
      _partial_refresh_blocked = true;
    }
    void unblock_auto_refresh() {
      _partial_refresh_blocked = false;
    }

  private:
    bool _partial_refresh_blocked;
    RefreshSlot _refresh_ui;
    PartialRefreshSlot _partial_refresh_ui;
  };
}

#endif
