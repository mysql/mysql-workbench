/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SELECTOPTIONWIZARD_H_
#define _SELECTOPTIONWIZARD_H_

#include "workbench/wb_backend_public_interface.h"
#include <string>
#include <vector>

#include "mforms/form.h"
#include "mforms/selector.h"
#include "mforms/box.h"
#include "mforms/label.h"
#include "mforms/button.h"

using namespace mforms;

class MYSQLWBBACKEND_PUBLIC_FUNC SelectOptionDialog : public mforms::Form {
public:
  SelectOptionDialog(const std::string &title, const std::string &description, std::vector<std::string> &options,
                     mforms::SelectorStyle style = SelectorCombobox);
  ~SelectOptionDialog();

  void set_validation_function(std::function<bool(std::string)> target) {
    validate = target;
  }
  std::string run();

protected:
  mforms::Box _top_vbox;
  mforms::Box _bottom_hbox;

  mforms::Label _description;
  mforms::Selector _option_box;
  mforms::Button _ok_button;
  mforms::Button _cancel_button;

  std::function<bool(std::string)> validate;
};

#endif /* _SELECTOPTIONWIZARD_H_ */
