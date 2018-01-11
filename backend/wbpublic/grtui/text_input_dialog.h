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
