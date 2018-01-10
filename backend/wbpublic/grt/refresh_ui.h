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
