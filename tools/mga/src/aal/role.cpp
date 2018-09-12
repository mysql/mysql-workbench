/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "role.h"

//----------------------------------------------------------------------------------------------------------------------

std::string aal::roleToString(const Role role) {
  static std::map<Role, std::string> roleMap = {
    { Role::Unknown, "Unknown" },
    { Role::Any, "Any" },
    { Role::Application, "Application" },
    { Role::Window, "Window" },
    { Role::Button, "Button" },
    { Role::RadioButton, "RadioButton" },
    { Role::CheckBox, "CheckBox" },
    { Role::ComboBox, "ComboBox" },
    { Role::Expander, "Expander" },
    { Role::Grid, "Grid" },
    { Role::TextBox, "Textbox" },
    { Role::TreeView, "TreeView" },
    { Role::Label, "Label" },
    { Role::Pane, "Pane" },
    { Role::Menu, "Menu" },
    { Role::MenuBar, "MenuBar" },
    { Role::MenuItem, "MenuItem" },
    { Role::Separator, "Separator" },
    { Role::SplitContainer, "SplitContainer" },
    { Role::GroupBox, "GroupBox" },
    { Role::Image, "Image" },
    { Role::TabView, "TabView" },
    { Role::TabPage, "TabPage" },
    { Role::DatePicker, "DatePicker" },
    { Role::Row, "Row" },
    { Role::Column, "Column" },
    { Role::Cell, "Cell" },
    { Role::ScrollBox, "ScrollBox" },
    { Role::Slider, "Slider" },
    { Role::Stepper, "Stepper" },
    { Role::List, "List" },
    { Role::IconView, "IconView" },

    { Role::BusyIndicator, "BusyIndicator" },
    { Role::ProgressIndicator, "ProgressIndicator" },

    { Role::ScrollBar, "ScrollBar" },
    { Role::ScrollThumb, "ScrollThumb" },
  };

  if (roleMap.find(role) == roleMap.end())
    return "Unhandled role";
  return roleMap[role];
}

//----------------------------------------------------------------------------------------------------------------------
