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
