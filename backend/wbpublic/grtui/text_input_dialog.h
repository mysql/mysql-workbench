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
#ifndef _TEXT_INPUT_DIALOG_H_
#define _TEXT_INPUT_DIALOG_H_

#include "wbpublic_public_interface.h"

#include "mforms/form.h"
#include "mforms/table.h"
#include "mforms/label.h"
#include "mforms/textentry.h"
#include "mforms/box.h"
#include "mforms/button.h"

namespace grtui {
  class WBPUBLICBACKEND_PUBLIC_FUNC TextInputDialog : public mforms::Form {
  public:
    TextInputDialog(mforms::Form *owner);

    void set_description(const std::string &text);
    void set_caption(const std::string &text);

    void set_value(const std::string &text);
    std::string get_value();

    bool run();

  protected:
    mforms::Table _table;
    mforms::Label _description;
    mforms::Label _caption;
    mforms::TextEntry _input;
    mforms::Box _button_box;
    mforms::Button _ok_button;
    mforms::Button _cancel_button;
  };
};

#endif /* _TEXT_INPUT_DIALOG_H_ */
