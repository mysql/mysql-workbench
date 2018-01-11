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

#include "ConvUtils.h"
#include "GrtTemplates.h"

#include "RefreshUI.h"

namespace MySQL {
  namespace Grt {

    void RefreshUI::set_refresh_slot(RefreshSlot::ManagedDelegate ^ cb) {
      _refresh_slot = gcnew RefreshSlot(cb);
      _inner->set_refresh_ui_slot(_refresh_slot->get_slot());
    }

    void RefreshUI::set_partial_refresh_slot(PartialRefreshSlot::ManagedDelegate ^ cb) {
      _partial_refresh_slot = gcnew PartialRefreshSlot(cb);
      _inner->set_partial_refresh_ui_slot(_partial_refresh_slot->get_slot());
    }
  }
}
