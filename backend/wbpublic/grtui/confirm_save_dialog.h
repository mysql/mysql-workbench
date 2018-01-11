/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _CONFIRM_SAVE_DIALOG_H_
#define _CONFIRM_SAVE_DIALOG_H_

#include "wbpublic_public_interface.h"

// TODO: for mforms classes: use pointers, forward declare the classes and move the includes to the cpp file.
#include <mforms/form.h>
#include <mforms/box.h>
#include <mforms/label.h>
#include <mforms/scrollpanel.h>
#include <mforms/button.h>

class WBPUBLICBACKEND_PUBLIC_FUNC ConfirmSaveDialog : public mforms::Form {
public:
  enum Result { ReviewChanges, Cancel, DiscardChanges };

private:
  mforms::Box _box;
  mforms::Label _caption;
  mforms::ScrollPanel _scroller;
  mforms::Box _checkboxes;
  mforms::Button _review_button;
  mforms::Button _cancel_button;
  std::string _last_group;
  int _item_count;
  Result _result;

  void discard_clicked();

public:
  ConfirmSaveDialog(mforms::Form *owner, const std::string &window_title, const std::string &caption);

  void add_item(const std::string &group, const std::string &name);
  void add_item(const std::string &name);

  int change_count() {
    return _item_count;
  }
  Result run();
};

#endif
