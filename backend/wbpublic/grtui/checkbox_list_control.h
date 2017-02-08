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

#pragma once

#include "mforms/scrollpanel.h"
#include "mforms/checkbox.h"
#include "mforms/box.h"

#include "grt.h"

#include "wbpublic_public_interface.h"

class WBPUBLICBACKEND_PUBLIC_FUNC StringCheckBoxList : public mforms::ScrollPanel {
  std::vector<mforms::CheckBox *> _items;
  mforms::Box _box;
  boost::signals2::signal<void()> _signal_changed;

  void toggled();

public:
  StringCheckBoxList();

  void set_strings(const std::vector<std::string> &strings);
  void set_strings(const grt::StringListRef &strings);

  std::vector<std::string> get_selection();
  bool has_selection();

  void set_selected(const std::string &name, bool flag);

  boost::signals2::signal<void()> *signal_changed() {
    return &_signal_changed;
  }
};
