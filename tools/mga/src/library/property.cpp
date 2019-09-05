/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "property.h"

//----------------------------------------------------------------------------------------------------------------------

std::string mga::propertyToString(const mga::Property property) {
  static std::map<Property, std::string> propertyMap = {
    { Property::Text, "text" },
    { Property::Title, "title" },
    { Property::Description, "description" },
    { Property::Enabled, "enabled" },
    { Property::CanExpand, "canExpan" },
    { Property::Expanded, "expanded" },
    { Property::CanFocus, "canFocus" },
    { Property::Focused, "focused" },
    { Property::CheckState, "checkState" },
    { Property::Value, "value" },
    { Property::MaxValue, "maxValue" },
    { Property::MinValue, "minValue" },
    { Property::ActiveTab, "activeTab" },
    { Property:: Active, "active" },
    { Property::Editable, "editable" },
    { Property::SelectedIndexes, "selectedIndexes" },
    { Property::ReadOnly, "readOnly" },
    { Property::IsSecure, "isSecure" },
    { Property::CaretPosition, "caretPosition" },
    { Property::SelectedText, "selectedText" },
    { Property::SelectionRange, "selectedRange" },
    { Property::CharacterCount, "characterCount" },
    { Property::Horizontal, "horizontal" },
    { Property::Date, "date" },
    { Property::SelectedIndex, "selectedIndex" },
    { Property::Selected, "selected" },
    { Property::Help, "help" },
    { Property::Range, "range" },
    { Property::Position, "position" },
    { Property::Shown, "shown" },
    { Property::Bounds, "bounds" },
    { Property::Scroll, "scroll" },
    { Property::Increment, "increment" },
    { Property::Decrement, "decrement" },
    { Property::StepUp, "stepUp" },
    { Property::StepDown, "stepDown" },

    { Property::Rows, "rows" },
    { Property::Columns, "columns" },
    { Property::Pages, "pages" },
    { Property::CloseButton, "closeButton" },
    { Property::Entries, "entries" },
    { Property::Header, "header" },
    { Property::Scrollbar, "scrollbar" },
    { Property::Screen, "screen" },

    { Property::Press, "press" },
    { Property::Activate, "activate" },
    { Property::Show, "show" },
    { Property::ShowContextMenu, "showContextMenu" },
    { Property::TakeScreenShot, "takeScreenShot" },
    { Property::GetBounds, "getBounds" },
  };

  assert(propertyMap.size() == static_cast<size_t>(Property::PropertyCount));

  auto iterator = propertyMap.find(property);
  if (iterator == propertyMap.end())
    throw std::runtime_error("Invalid property enum: " + std::to_string(static_cast<size_t>(property)) + "");

  return iterator->second;
}

//----------------------------------------------------------------------------------------------------------------------

mga::Property mga::propertyFromString(const std::string text) {
  static std::map<std::string, Property> propertyMap = {
    { "text", Property::Text },
    { "title", Property::Title },
    { "description", Property::Description },
    { "enabled", Property::Enabled },
    { "canExpand", Property::CanExpand },
    { "expanded", Property::Expanded },
    { "canFocus", Property::CanFocus },
    { "focused", Property::Focused },
    { "checkState", Property::CheckState },
    { "value", Property::Value },
    { "maxValue", Property::MaxValue },
    { "minValue", Property::MinValue },
    { "activeTab", Property::ActiveTab },
    { "active", Property::Active },
    { "editable", Property::Editable },
    { "selectedIndexes", Property::SelectedIndexes },
    { "readOnly", Property::ReadOnly },
    { "isSecure", Property::IsSecure },
    { "caretPosition", Property::CaretPosition },
    { "selectedText", Property::SelectedText },
    { "selectionRange", Property::SelectionRange },
    { "characterCount", Property::CharacterCount },
    { "horizontal", Property::Horizontal },
    { "date", Property::Date },
    { "selectedIndex", Property::SelectedIndex },
    { "selected", Property::Selected },
    { "help", Property::Help },
    { "range", Property::Range },
    { "position", Property::Position },
    { "shown", Property::Shown },
    { "bounds", Property::Bounds },
    { "scroll", Property::Scroll },
    { "increment", Property::Increment },
    { "decrement", Property::Decrement },
    { "stepUp", Property::StepUp },
    { "stepDown", Property::StepDown },

    { "rows", Property::Rows },
    { "columns", Property::Columns },
    { "pages", Property::Pages },
    { "closeButton", Property::CloseButton },
    { "entries", Property::Entries },
    { "header", Property::Header },
    { "scrollbar", Property::Scrollbar },
    { "screen", Property::Screen },

    { "press", Property::Press },
    { "activate", Property::Activate },
    { "show", Property::Show },
    { "showContextMenu", Property::ShowContextMenu },
    { "takeScreenShot", Property::TakeScreenShot },
    { "getBounds", Property::GetBounds },
  };

  assert(propertyMap.size() == static_cast<size_t>(Property::PropertyCount));

  auto iterator = propertyMap.find(text);
  if (iterator == propertyMap.end())
    throw std::runtime_error("Invalid property/action: \"" + text + "\"");

  return iterator->second;
}

//----------------------------------------------------------------------------------------------------------------------
