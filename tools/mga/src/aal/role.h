/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "aalcommon.h"

namespace aal {

  // This enum is exported to JS, so changes here are automatically propagated.
  // However, the TS typings must be updated if you change this list (beside the implementation here).
  enum class Role {
    Unknown = 0,
    Any,
    Application,
    Window,
    Button,
    RadioButton,
    RadioGroup,
    CheckBox,
    ComboBox,
    Expander, // A disclosure triangle or similar to expand/collapse an area.
    Grid,
    TextBox,
    TreeView,
    Label,
    Pane, // A plain box with no decoration.
    Menu,
    MenuBar, // Also known as Toolbar.
    MenuItem,
    Separator,
    SplitContainer,
    Splitter, // The draggable area between 2 split areas.
    GroupBox,
    Image,
    TabView,
    TabPage,
    DatePicker,
    Row,    // A row in a treeview, icon or grid view.
    Column, // A column in a multi column treeview or grid view.
    Cell,   // A cell in a row.
    ScrollBox,
    Slider,
    Stepper, // Also known as up/down or spinner control.
    List,    // An element consisting of equally structured items, often in custom layouts.
    IconView, // A control with icons + text in a grid like manner (list view on Windows).
    ProgressIndicator, // A determinate progress indicator.
    BusyIndicator,     // A indeterminate progress indicator.
    ScrollBar,
    ScrollThumb,
    HyperLink,
    
    Sentinel // The last entry, for iteration.
  };

  ACCESSIBILITY_PUBLIC std::string roleToString(const Role role);
  ACCESSIBILITY_PUBLIC std::string roleToFriendlyString(const Role role);
  ACCESSIBILITY_PUBLIC std::string roleToJSType(const Role role);
}
