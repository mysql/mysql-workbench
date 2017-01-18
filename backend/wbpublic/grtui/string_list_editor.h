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

#include "wbpublic_public_interface.h"
#include "grt.h"

#include "mforms/form.h"
#include "mforms/box.h"
#include "mforms/treeview.h"
#include "mforms/button.h"

namespace grtui {

  class WBPUBLICBACKEND_PUBLIC_FUNC StringListEditor : public mforms::Form {
  protected:
    mforms::Box _vbox;
    mforms::TreeView _tree;
    mforms::Box _button_box;
    mforms::Button _ok_button;
    mforms::Button _cancel_button;

    mforms::Button _add_button;
    mforms::Button _del_button;

    virtual void add();
    virtual void del();

  public:
    StringListEditor(mforms::Form *owner = 0, const bool reorderable = false);

    bool run();

    void set_string_list(const std::vector<std::string> &strings);
    void set_grt_string_list(const grt::StringListRef &strings);

    std::vector<std::string> get_string_list();
    grt::StringListRef get_grt_string_list();
  };
};
