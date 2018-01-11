/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _SCHEMA_MATCHING_PAGE_H_
#define _SCHEMA_MATCHING_PAGE_H_

#include "grtui/grt_wizard_form.h"
#include "mforms/label.h"
#include "mforms/treeview.h"
#include "mforms/imagebox.h"
#include "mforms/menubar.h"
#include "mforms/checkbox.h"
#include "grts/structs.db.h"

class SchemaMatchingPage : public grtui::WizardPage {
  class OverridePanel;

public:
  SchemaMatchingPage(grtui::WizardForm *form, const char *name = "selectSchemata",
                     const std::string &left_name = "Model", const std::string &right_name = "Source",
                     bool unselect_by_default = false);

  void cell_edited(mforms::TreeNodeRef node, int column, const std::string &value);

  virtual bool allow_next();

  virtual void leave(bool advancing);
  virtual void enter(bool advancing);

  std::map<std::string, std::string> get_mapping();

private:
  void selection_changed();
  void action_clicked();

private:
  mforms::Box _header;
  mforms::ImageBox _image;
  mforms::Label _label;
  mforms::TreeView _tree;
  OverridePanel *_override;

  bool _unselect_by_default;
  mforms::ContextMenu _menu;
  mforms::Button _action_button;
  mforms::Label _explain_label;
  mforms::Label _missing_label;
};

#endif /* _SCHEMA_SELECTION_PAGE_H_ */
