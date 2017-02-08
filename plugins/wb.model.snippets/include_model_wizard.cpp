/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "include_model_wizard.h"
#include "grtui/grt_wizard_plugin.h"

class SelectObjectsPage : public grtui::WizardPage {
public:
  SelectObjectsPage(grtui::WizardForm *form) : WizardPage(form, "select_objects"){};
};

class SelectTargetSchemaPage : public grtui::WizardPage {
public:
  SelectTargetSchemaPage(grtui::WizardForm *form) : WizardPage(form, "select_schema"){};
};

class FixNamesPage : public grtui::WizardPage {
public:
  FixNamesPage(grtui::WizardForm *form) : WizardPage(form, "fix_names"){};
};

class IncludeModelWizard : public grtui::WizardPlugin {
public:
  IncludeModelWizard(grt::Module *module) : WizardPlugin(module) {
    set_name("model_wizard");
    add_page(mforms::manage(_select_objects_page = new SelectObjectsPage(this)));
    add_page(mforms::manage(_select_schema_page = new SelectTargetSchemaPage(this)));
    add_page(mforms::manage(_fix_names_page = new FixNamesPage(this)));
  }

protected:
  SelectObjectsPage *_select_objects_page;
  SelectTargetSchemaPage *_select_schema_page;
  FixNamesPage *_fix_names_page;
};
