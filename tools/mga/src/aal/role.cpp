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

#include "role.h"

using namespace aal;

//----------------------------------------------------------------------------------------------------------------------

std::string aal::roleToString(const Role role) {
  static std::map<Role, std::string> roleMap = {
    { Role::Unknown, "Unknown" },
    { Role::Any, "Any" },
    { Role::Application, "Application" },
    { Role::Window, "Window" },
    { Role::Button, "Button" },
    { Role::RadioButton, "RadioButton" },
    { Role::RadioGroup, "RadioGroup" },
    { Role::CheckBox, "CheckBox" },
    { Role::ComboBox, "ComboBox" },
    { Role::Expander, "Expander" },
    { Role::Grid, "Grid" },
    { Role::TextBox, "TextBox" },
    { Role::TreeView, "TreeView" },
    { Role::Label, "Label" },
    { Role::Pane, "Pane" },
    { Role::Menu, "Menu" },
    { Role::MenuBar, "MenuBar" },
    { Role::MenuItem, "MenuItem" },
    { Role::Separator, "Separator" },
    { Role::SplitContainer, "SplitContainer" },
    { Role::Splitter, "Splitter" },
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
    { Role::ProgressIndicator, "ProgressIndicator" },
    { Role::BusyIndicator, "BusyIndicator" },
    { Role::ScrollBar, "ScrollBar" },
    { Role::ScrollThumb, "ScrollThumb" },
    { Role::HyperLink, "Hyperlink" },
  };

  if (roleMap.find(role) == roleMap.end())
    return "Unhandled role";
  return roleMap[role];
}

//----------------------------------------------------------------------------------------------------------------------

std::string aal::roleToFriendlyString(const Role role) {
  static std::map<Role, std::string> roleMap = {
    { Role::Unknown, "Unknown" },
    { Role::Any, "Any" },
    { Role::Application, "Application" },
    { Role::Window, "Window" },
    { Role::Button, "Button" },
    { Role::RadioButton, "Radio Button" },
    { Role::RadioGroup, "Radio Group" },
    { Role::CheckBox, "Check Box" },
    { Role::ComboBox, "Combo Box" },
    { Role::Expander, "Expander" },
    { Role::Grid, "Grid" },
    { Role::TextBox, "Text Box" },
    { Role::TreeView, "Tree View" },
    { Role::Label, "Label" },
    { Role::Pane, "Pane" },
    { Role::Menu, "Menu" },
    { Role::MenuBar, "Menu Bar" },
    { Role::MenuItem, "Menu Item" },
    { Role::Separator, "Separator" },
    { Role::SplitContainer, "Split Container" },
    { Role::Splitter, "Splitter" },
    { Role::GroupBox, "Group Box" },
    { Role::Image, "Image" },
    { Role::TabView, "Tab View" },
    { Role::TabPage, "Tab Page" },
    { Role::DatePicker, "Date Picker" },
    { Role::Row, "Row" },
    { Role::Column, "Column" },
    { Role::Cell, "Cell" },
    { Role::ScrollBox, "Scroll Box" },
    { Role::Slider, "Slider" },
    { Role::Stepper, "Stepper" },
    { Role::List, "List" },
    { Role::IconView, "Icon View" },
    { Role::ProgressIndicator, "Progress Indicator" },
    { Role::BusyIndicator, "Busy Indicator" },
    { Role::ScrollBar, "Scroll Bar" },
    { Role::ScrollThumb, "Scroll Thumb" },
    { Role::HyperLink, "Hyperlink" },
  };

  if (roleMap.find(role) == roleMap.end())
    return "Unhandled role";
  return roleMap[role];
}

//----------------------------------------------------------------------------------------------------------------------

std::string aal::roleToJSType(const Role role) {
  static std::map<Role, std::string> roleToJSTypeMap = {
    { Role::Unknown, "UIElement" },
    { Role::Any, "" },
    { Role::Application, "UIElement" },
    { Role::Window, "UIWindow" },
    { Role::Button, "UIButton" },
    { Role::RadioButton, "UIRadioButton" },
    { Role::RadioGroup, "UIRadioGroup" },
    { Role::CheckBox, "UICheckBox" },
    { Role::ComboBox, "UIComboBox" },
    { Role::Expander, "UIExpander" },
    { Role::Grid, "UIGrid" },
    { Role::TextBox, "UITextBox" },
    { Role::TreeView, "UITreeView" },
    { Role::Label, "UILabel" },
    { Role::Pane, "UIPane" },
    { Role::Menu, "UIMenu" },
    { Role::MenuBar, "UIMenuBar" },
    { Role::MenuItem, "UIMenuItem" },
    { Role::Separator, "UISeparator" },
    { Role::SplitContainer, "UISplitContainer" },
    { Role::Splitter, "UISplitter" },
    { Role::GroupBox, "UIGroupBox" },
    { Role::Image, "UIImage" },
    { Role::TabView, "UITabView" },
    { Role::TabPage, "UITabPage" },
    { Role::DatePicker, "UIDatePicker" },
    { Role::Row, "UIRow" },
    { Role::Column, "UIColumn" },
    { Role::Cell, "UICell" },
    { Role::ScrollBox, "UIScrollBox" },
    { Role::Slider, "UISlider" },
    { Role::Stepper, "UIStepper" },
    { Role::List, "UIList" },
    { Role::IconView, "UIIconView" },
    { Role::ProgressIndicator, "UIProgress" },
    { Role::BusyIndicator, "UIBusy" },
    { Role::ScrollBar, "UIScrollBar" },
    { Role::ScrollThumb, "UIScrollThumb" },
    { Role::HyperLink, "UIHyperLink" },
  };

  auto iterator = roleToJSTypeMap.find(role);
  if (iterator == roleToJSTypeMap.end())
    return "Unhandled role";
  return iterator->second;
}

//----------------------------------------------------------------------------------------------------------------------

// Properties applying to all roles.
std::vector<std::pair<std::string, std::vector<size_t>>> generalPropertiesList = {
   {{ "focused", { 1, 1 }}, { "canFocus", { 1 }}, { "enabled", { 1 }}, { "help", { 1 }}}
};

// Role specific properties (in addition to those applying to all UI elements.
// A general property can be suppressed by defining it here with empty counts.
std::map<Role, std::vector<std::pair<std::string, std::vector<size_t>>>> roleToPropertiesMap = {
  { Role::Unknown, {}},
  { Role::Any, {}},
  { Role::Application, {}},
  { Role::Window, {
    { "enabled", {}},
    { "title", { 1 }},
    { "screen", { 1 }},
    { "bounds", { 1, 1 }}
  }},
  { Role::Button, {}},
  { Role::RadioButton, {{ "checkState", { 1, 1 }}}},
  { Role::RadioGroup, {}},
  { Role::CheckBox, {{ "checkState", { 1, 1 }}}},
  { Role::ComboBox, {{ "editable", { 1 }}, { "text", { 1, 1 }}, { "expanded", { 1, 1 }}, { "selectedIndex", { 1, 1 }}}},
  { Role::Expander, {{ "canExpand", { 1 }}, { "expanded", { 1, 1 }}}},
  { Role::Grid, {{ "rows", { 1 }}, { "columns", { 1 }}}},
  { Role::TextBox, {
    { "horizontalScrollBar", { 1 }},
    { "verticalScrollBar", { 1 }},
    { "text", { 1, 1 }},
    { "readOnly", { 1 }},
    { "isSecure", { 1 }},
    { "caretPosition", { 1, 1 }},
    { "selectedText", { 1, 1 }},
    { "selectionRange", { 1, 1 }},
    { "characterCount", { 1 }},
  }},
  { Role::TreeView, {{ "rows", { 1 }}, { "columns", { 1 }}}},
  { Role::Label, {{ "text", { 1 }}}},
  { Role::Pane, {}},
  { Role::Menu, {{ "focused", {}}, { "canFocus", {}}}},
  { Role::MenuBar, {{ "focused", {}}, { "canFocus", {}}}},
  { Role::MenuItem, {{ "title", { 1 }}, { "focused", {}}, { "canFocus", {}}}},
  { Role::Separator, {}},
  { Role::SplitContainer, {{ "horizontal", { 1 }}}},
  { Role::Splitter, {{ "value", { 1, 1 }}, { "minValue", { 1 }}, { "maxValue", { 1 }}}},
  { Role::GroupBox, {{ "title", { 1 }}}},
  { Role::Image, {}},
  { Role::TabView, {{ "activeTab", { 1, 1 }}}},
  { Role::TabView, {{ "tabPages", { 1 }}}},
  { Role::TabPage, {{ "active", { 1, 1 }}, { "closeButton", { 1 }}}},
  { Role::DatePicker, {{ "date", { 1, 1 }}}},
  { Role::Row, {{ "canExpand", { 1 }}, { "expanded", { 1, 1 }}, { "selected", { 1, 1 }}, { "entries", { 1 }}}},
  { Role::Column, {{ "selected", { 1, 1 }}, { "header", { 1 }}, { "entries", { 1 }}}},
  { Role::Cell, {}},
  { Role::ScrollBox, {{ "horizontalScrollBar", { 1 }}, { "verticalScrollBar", { 1 }}}},
  { Role::Slider, {{ "value", { 1, 1 }}, { "minValue", { 1 }}, { "maxValue", { 1 }}, { "horizontal", { 1 }}}},
  { Role::Stepper, {{ "value", { 1, 1 }}, { "minValue", { 1 }}, { "maxValue", { 1 }}}},
  { Role::List, {{ "selectedIndexes", { 1, 1 }}}},
  { Role::IconView, {}},
  { Role::ProgressIndicator, {{ "value", { 1 }}, { "minValue", { 1 }}, { "maxValue", { 1 }}}},
  { Role::BusyIndicator, {}},
  { Role::ScrollBar, {{ "horizontal", { 1 }}, { "range", { 1 }}, { "position", { 1, 1 }}}},
  { Role::ScrollThumb, {}},
  { Role::HyperLink, {}},
};

// Ditto for functions.
std::set<std::string> generalFunctionsList = {
  "getBounds", "showContextMenu"
};

std::map<Role, std::vector<std::string>> roleToFunctionsMap = {
  { Role::Unknown, {}},
  { Role::Any, {}},
  { Role::Application, {}},
  { Role::Window, { "takeScreenShot" }},
  { Role::Button, { "press" }},
  { Role::RadioButton, { "press" }},
  { Role::RadioGroup, { }},
  { Role::CheckBox, { "press" }},
  { Role::ComboBox, {}},
  { Role::Expander, {}},
  { Role::Grid, {}},
  { Role::TextBox, { "insertText" }},
  { Role::TreeView, {}},
  { Role::Label, {}},
  { Role::Pane, {}},
  { Role::Menu, { "activate" }},
  { Role::MenuBar, { "activate" }},
  { Role::MenuItem, { "activate" }},
  { Role::Separator, {}},
  { Role::SplitContainer, {}},
  { Role::GroupBox, {}},
  { Role::Image, { "saveContent" }},
  { Role::TabView, {}},
  { Role::TabPage, {}},
  { Role::DatePicker, {}},
  { Role::Row, {}},
  { Role::Column, {}},
  { Role::Cell, {}},
  { Role::ScrollBox, { "scrollLeft", "scrollRight", "scrollUp", "scrollDown" }},
  { Role::Slider, { "increment", "decrement" }},
  { Role::Stepper, { "stepUp", "stepDown" }},
  { Role::List, {}},
  { Role::IconView, {}},
  { Role::ProgressIndicator, {}},
  { Role::BusyIndicator, {}},
  { Role::ScrollBar, {}},
  { Role::ScrollThumb, {}},
  { Role::HyperLink, {}},
};

//----------------------------------------------------------------------------------------------------------------------

