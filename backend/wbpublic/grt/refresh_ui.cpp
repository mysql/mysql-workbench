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

#include "refresh_ui.h"

using namespace bec;

//--------------------------------------------------------------------------------------------------

void RefreshUI::set_refresh_ui_slot(const RefreshSlot &slot) {
  _refresh_ui = slot;
}

//--------------------------------------------------------------------------------------------------

void bec::RefreshUI::set_partial_refresh_ui_slot(const PartialRefreshSlot &slot) {
  _partial_refresh_ui = slot;
}

//--------------------------------------------------------------------------------------------------

void bec::RefreshUI::do_partial_ui_refresh(const int what) {
  if (!_partial_refresh_blocked && _partial_refresh_ui)
    _partial_refresh_ui(what);
}

//--------------------------------------------------------------------------------------------------

void bec::RefreshUI::do_ui_refresh() {
  if (_refresh_ui)
    _refresh_ui();
}

//--------------------------------------------------------------------------------------------------
