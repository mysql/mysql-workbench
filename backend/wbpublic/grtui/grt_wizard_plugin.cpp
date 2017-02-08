/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt_wizard_plugin.h"

using namespace grtui;

WizardPlugin::WizardPlugin(grt::Module *module) : GUIPluginBase(module), WizardForm() {
  set_name("wizard");
}

bool WizardPlugin::run_wizard() {
  // TODO: shouldn't the result depend on the outcome of the wizard?
  run_modal();

  return true;
}
