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

#include "accessible.wr.h"
#include "UIAutomationClient.h"

using namespace aal;
namespace SWA = System::Windows::Automation;

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::accessibilitySetup() {
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::NativeToCppString(System::String ^ str) {
  if (str == nullptr || str->Length == 0)
    return "";

  array<unsigned char> ^ chars = System::Text::Encoding::UTF8->GetBytes(str);
  if (chars == nullptr || chars->Length == 0)
    return "";

  pin_ptr<unsigned char> char_ptr = &chars[0];
  std::string result((char *)char_ptr);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::getByPid(const int pid) {
  try {
    return AccessibleWr::Ref(new AccessibleWr(AccessibleNet::GetByPid(pid)));
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

int AccessibleWr::getRunningProcess(std::wstring const& fileName) {
  try {
    return AccessibleNet::GetRunningProcess(gcnew System::String(fileName.c_str()));
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<int> AccessibleWr::getRunningProcessByName(std::wstring const& name) {
  try {
    cli::array<int> ^list = AccessibleNet::GetRunningProcessByName(
      gcnew System::String(name.c_str()));
    std::vector<int> result(list->Length);
    if (list->Length > 0) {
      System::Runtime::InteropServices::Marshal::Copy(list, 0, System::IntPtr(&result[0]), list->Length);
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::AccessibleWr(AccessibleNet ^accessible)
  : _managedObject(accessible) {
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::~AccessibleWr() {
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a clone of this instance, that is, a new instance referring to the same AccessibleNet object
 * (which can be shared, while AccessibleWr cannot).
 * This is useful in cases where we need to return an otherwise managed instance in an enumeration (which uses
 * unique pointers).
 */
AccessibleWr::Ref AccessibleWr::clone() {
  return AccessibleWr::Ref(new AccessibleWr(_managedObject));
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::getHorizontalScrollBar() const {
   try {
     AccessibleNet ^element = _managedObject->HorizontalScrollBar;
     if (element != nullptr) {
       return std::unique_ptr<AccessibleWr>(new AccessibleWr(element));
     }
     return Ref();
   } catch (System::Exception ^e) {
     std::string message = NativeToCppString(e->ToString());
     throw std::runtime_error(message.c_str());
   }
 }

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::getVerticalScrollBar() const {
  try {
    AccessibleNet ^element = _managedObject->VerticalScrollBar;
    if (element != nullptr) {
      return std::unique_ptr<AccessibleWr>(new AccessibleWr(element));
    }
    return Ref();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

double AccessibleWr::getScrollPosition() const {
  try {
    return _managedObject->ScrollPosition;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setScrollPosition(double value) {
  try {
    _managedObject->ScrollPosition = value;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::AccessibleWr::menuShown() const {
  try {
    return  _managedObject->MenuShown;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::showMenu() const {
  try {
    _managedObject->ShowMenu();
  }
  catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getID() const {
  try {
    System::String ^id = _managedObject->ID;
    return NativeToCppString(id);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getName() const {
  try {
    System::String ^name = _managedObject->Name;
    return NativeToCppString(name);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getHelp() const {
  try {
    System::String ^name = _managedObject->HelpText;
    return NativeToCppString(name);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

Role AccessibleWr::getRole() const {
  try {
    Role role = Role::Unknown;
    SWA::ControlType ^type = _managedObject->ControlType;
    switch (type->Id) {
    case UIA_WindowControlTypeId: 
      role = Role::Window;
      break;
    case UIA_ButtonControlTypeId:
      role = Role::Button;
      break;
    case UIA_CalendarControlTypeId:
      break;
    case UIA_CheckBoxControlTypeId:
      role = Role::CheckBox;
      break;
    case UIA_ComboBoxControlTypeId: 
      role = Role::ComboBox;
      break;
    case UIA_TextControlTypeId:
      role = Role::Label;
      break;
    case UIA_EditControlTypeId:
    case UIA_DocumentControlTypeId:
      role = Role::TextBox;
      break;
    case UIA_GroupControlTypeId: 
      role = Role::GroupBox;
      break;
    case UIA_RadioButtonControlTypeId:
      role = Role::RadioButton;
      break;
    case UIA_TableControlTypeId:
    case UIA_DataGridControlTypeId:
        role = Role::Grid;
      break;
    case UIA_TreeControlTypeId:
      if (_managedObject->IsVirtualColumn)
        role = Role::Column;
      else
        role = Role::TreeView;
      break;
    case UIA_ToolBarControlTypeId:
      role = Role::MenuBar;
      break;
    case UIA_MenuControlTypeId:
    case UIA_MenuBarControlTypeId:
      role = Role::Menu;
      break;
    case UIA_MenuItemControlTypeId:
      role = Role::MenuItem;
      break;
    case UIA_ScrollBarControlTypeId:
      role = Role::ScrollBar;
      break;
    case UIA_ImageControlTypeId:
      role = Role::Image;
      break;
    case UIA_TabControlTypeId:
      role = Role::TabView;
      break;
    case UIA_ListControlTypeId:
      role = Role::List;
      break;
    case UIA_TabItemControlTypeId:
      role = Role::TabPage;
      break;
    case UIA_ProgressBarControlTypeId:
      role = Role::ProgressIndicator;
      break;
    case UIA_SliderControlTypeId:
      role = Role::Slider;
      break;
    case UIA_SpinnerControlTypeId: {
      role = Role::Stepper;
      break;
    }
    case UIA_SeparatorControlTypeId:
      role = Role::Separator;
      break;
    case UIA_PaneControlTypeId: {
      // Unfortunately, quite a number of controls are listed as panes (image, calendar, inner tabpage, to name a few).
      System::String ^className = _managedObject->ClassName;
      if (className->Equals("SysMonthCal32"))
        role = Role::DatePicker;
      else if (_managedObject->Parent->ControlType->Id == UIA_TableControlTypeId &&
        className->Equals("scrollbar", System::StringComparison::InvariantCultureIgnoreCase))
        role = Role::ScrollBar;
      else if (_managedObject->Scrollable)
        role = Role::ScrollBox;
      else
        role = Role::Pane;
      break;
    }
    case UIA_HeaderControlTypeId:
    case UIA_DataItemControlTypeId:
    case UIA_TreeItemControlTypeId:
      if (_managedObject->IsVirtualRow)
        role = Role::Row;
      else
        role = Role::Cell;
      break;

    case UIA_ThumbControlTypeId:
      role = Role::ScrollThumb;
      break;
    case UIA_CustomControlTypeId: {
      AccessibleNet^ parent = _managedObject->Parent;
      if (parent->ControlType->Id == UIA_TableControlTypeId || parent->ControlType->Id == UIA_DataGridControlTypeId) {
        if (_managedObject->IsVirtualColumn)
          role = Role::Column;
        else if (_managedObject->IsVirtualRow)
          role = Role::Row;
      }
      else if (_managedObject->IsCell) {
        role = Role::Cell;
      }
      break;
    }
    case UIA_ListItemControlTypeId:
      role = Role::Cell;
      break;

    case UIA_TitleBarControlTypeId:
    case UIA_HyperlinkControlTypeId: 
    case UIA_SemanticZoomControlTypeId:
    case UIA_SplitButtonControlTypeId:
    case UIA_StatusBarControlTypeId:
    case UIA_AppBarControlTypeId:
    case UIA_ToolTipControlTypeId:  
      role = Role::Unknown;
    }
    return role;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isInternal() const {
  try {
    return _managedObject->IsInternal;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::getParent() {
  try {
    return AccessibleWr::Ref(new AccessibleWr(_managedObject->Parent));
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isEnabled() const {
  try {
    return _managedObject->IsEnabled;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::AccessibleWr::isEditable() const {
  try {
    return _managedObject->IsEditable;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::AccessibleWr::isReadOnly() const {
  try {
    return _managedObject->IsReadOnly;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::AccessibleWr::isSecure() const {
  try {
    return _managedObject->IsSecure;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::AccessibleWr::isHorizontal() const {
  try {
    return _managedObject->IsHorizontal;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Rectangle AccessibleWr::getBounds(bool screenCoordinates) {
  try {
    System::Windows::Rect ^bounds = _managedObject->Bounds;
    if (!screenCoordinates) {
      auto parent = _managedObject->Parent;
      if (parent != nullptr) {
        auto parentBounds = parent->Bounds;
        bounds->X -= parentBounds.X;
        bounds->Y -= parentBounds.Y;
      }
    }
    return geometry::Rectangle((int)bounds->X, (int)bounds->Y, (int)bounds->Width, (int)bounds->Height);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::setBounds(geometry::Rectangle const& bounds) {
  try {
    System::Windows::Rect managedRect(bounds.position.x, bounds.position.y, bounds.size.width, bounds.size.height);
    _managedObject->Bounds = managedRect;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::show() {
  try {
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::bringToFront() {
  try {
    _managedObject->BringToFront();
  }
  catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}


//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getText() const {
  std::string text;
  try {
    System::String ^value = System::String::Empty;
    SWA::ControlType ^type = _managedObject->ControlType;
    switch (type->Id) {
    case UIA_ListItemControlTypeId:
    case UIA_TreeItemControlTypeId:
      value = _managedObject->Name;
      break;
    default:
      value = _managedObject->Text;
    }
    text = NativeToCppString(value);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
  return text;
}

//----------------------------------------------------------------------------------------------------------------------

std::string aal::AccessibleWr::getTitle() const {
  return getText();
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::setTitle(std::string const &text) {
  setText(text);
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::setSelectionText(const std::string & text) {
  try {
    System::String ^value = gcnew System::String(text.c_str());
    _managedObject->SelectedText = value;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string aal::AccessibleWr::getDescription() const {
  std::string text;
  try {
    System::String ^value = _managedObject->Description;
    return NativeToCppString(value);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
  return text;
}

//----------------------------------------------------------------------------------------------------------------------

size_t AccessibleWr::getCaretPosition() const {
  try {
    return _managedObject->CaretPosition;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::setCaretPosition(size_t position) {
  _managedObject->CaretPosition = (int)position;
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setText(std::string const& text) {
  try {
    System::String ^value = gcnew System::String(text.c_str());
    _managedObject->InsertText(-1, value);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::setText(size_t offset, std::string const & text) {
  try {
    System::String ^value = gcnew System::String(text.c_str());
    _managedObject->InsertText(static_cast<int>(offset), value);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getSelectionText() const {
  try {
    System::String ^value = _managedObject->SelectedText;
    return NativeToCppString(value);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

TextRange AccessibleWr::getSelectionRange() {
  try {
    size_t start = 0;
    size_t end = 0;
    _managedObject->GetSelectionRange(start, end);
    return TextRange(start, end);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setSelectionRange(TextRange range) {
  try {
    _managedObject->SetSelectionRange(range.start, range.end);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

size_t AccessibleWr::getCharacterCount() const {
  try {
    return _managedObject->getCharacterCount();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::set<size_t> aal::AccessibleWr::getSelectedIndexes() const {
  try {
    cli::array<unsigned __int64> ^list = _managedObject->SelectedIndexes;
    std::set<size_t> result;
    for (int i = 0; i < list->Length; ++i) {
      size_t val = list[i];
      result.insert(val);
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
  return std::set<size_t>();
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::setSelectedIndexes(std::set<size_t> const & indexes) {
  try {
    cli::array<unsigned __int64> ^list = gcnew cli::array<unsigned __int64>(static_cast<int>(indexes.size()));
    std::set<size_t>::iterator it = indexes.begin();
    for (int i = 0; it != indexes.end() && i < indexes.size(); ++it, i++) {
      list[i] = (unsigned __int64)(*it);
    }
    _managedObject->SelectedIndexes = list;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::mouseDown(const geometry::Point & pos, const MouseButton button) {
  try {
    System::Drawing::Point pt(pos.x, pos.y);
    _managedObject->MouseDown(pt, mapButtonEnum(button));
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::mouseUp(const geometry::Point & pos, const MouseButton button) {
  try {
    System::Drawing::Point pt(pos.x, pos.y);
    _managedObject->MouseUp(pt, mapButtonEnum(button));
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::mouseMove(const geometry::Point & pos) {
  try {
    System::Drawing::Point pt(pos.x, pos.y);
    _managedObject->MouseMove(pt, AccessibleNet::MouseMovePosition::Relative);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::mouseMoveTo(const geometry::Point & pos) {
  try {
    System::Drawing::Point pt(pos.x, pos.y);
    _managedObject->MouseMove(pt, AccessibleNet::MouseMovePosition::Absolute);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Point aal::AccessibleWr::getMousePosition() const {
  try {
    System::Drawing::Point pt = _managedObject->MousePosition();
    return geometry::Point(pt.X, pt.Y);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------
 
static std::vector< unsigned short> keyCodeMap = {
  0x00, 
  /* 0, 1, 2, 3, ...., 9*/
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
  VK_OEM_PLUS, VK_OEM_MINUS,
  /* A, B, C, D, .... Z*/
  0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D,
  0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A,

  VK_TAB, VK_BACK, VK_RETURN, VK_OEM_PERIOD, VK_OEM_COMMA, VK_OEM_COMMA, VK_OEM_2, VK_OEM_5, VK_OEM_4, VK_OEM_6,
  VK_DELETE, VK_UP, VK_ESCAPE, VK_DOWN, VK_LEFT, VK_RIGHT, VK_PRIOR, VK_NEXT, VK_END, VK_HOME, VK_SPACE,
  VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12
};

//----------------------------------------------------------------------------------------------------------------------

static unsigned short modifierToFlags(aal::Modifier modifier) {
  unsigned short result = 0;
  if (containsModifier(modifier, aal::Modifier::ShiftLeft) || containsModifier(modifier, aal::Modifier::ShiftRight))
    result |= VK_SHIFT;
  if (containsModifier(modifier, aal::Modifier::ControlLeft) || containsModifier(modifier, aal::Modifier::ControlRight))
    result |= VK_CONTROL;
  if (containsModifier(modifier, aal::Modifier::AltLeft) || containsModifier(modifier, aal::Modifier::AltRight))
    result |= VK_MENU;
  if (containsModifier(modifier, aal::Modifier::MetaLeft))
    result |= VK_LWIN;
  if (containsModifier(modifier, aal::Modifier::MetaRight))
    result |= VK_RWIN;
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::keyDown(const aal::Key k, aal::Modifier modifier) const {
  try {
    if (k == Key::NoKey)
      return;
    unsigned short vKey = keyCodeMap[static_cast<size_t>(k)];
    unsigned short modifiers = modifierToFlags(modifier);
    if (modifiers != 0)
      _managedObject->KeyDown(modifiers, false);
    _managedObject->KeyDown(vKey, false);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::keyUp(const aal::Key k, aal::Modifier modifier) const {
  try {
    if (k == Key::NoKey)
      return;
    unsigned short vKey = keyCodeMap[static_cast<size_t>(k)];
    unsigned short modifiers = modifierToFlags(modifier);
    _managedObject->KeyUp(vKey, false);
    if (modifiers != 0)
      _managedObject->KeyUp(modifiers, false);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::keyPress(const aal::Key k, aal::Modifier modifier) const {
  keyDown(k, modifier);
  keyUp(k, modifier);
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::typeString(std::string const& input) const {
  try {
    _managedObject->TypeString(gcnew System::String(input.c_str()));
  }
  catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::click() {
  try {
    _managedObject->Click();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::stepUp() {
  try {
    _managedObject->StepUp();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::stepDown() {
  try {
    _managedObject->StepDown();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::scrollLeft() {
  try {
    _managedObject->ScrollLeft();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::scrollRight() {
  try {
    _managedObject->ScrollRight();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::scrollUp() {
  try {
    _managedObject->ScrollUp();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::scrollDown() {
  try {
    _managedObject->ScrollDown();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::increment() {
  try {
    _managedObject->Increment();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}
//----------------------------------------------------------------------------------------------------------------------

void aal::AccessibleWr::decrement() {
  try {
    _managedObject->Decrement();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isExpandable() {
  try {
    return _managedObject->IsExpandable();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

  //----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isExpanded() {
  try {
    return _managedObject->IsExpanded();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setExpanded(bool value) {
  try {
    _managedObject->SetExpanded(value);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::RefList AccessibleWr::children() {
  try {
    cli::array<aal::AccessibleNet ^> ^list = _managedObject->Children();
    RefList result;
    for (int i = 0; i < list->Length; ++i) {
      result.emplace_back(new aal::AccessibleWr(list[i]));
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isRoot() {
  try {
    return _managedObject->IsRoot;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isValid() {
  try {
    return _managedObject && _managedObject->Valid;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::equals(AccessibleWr *other) {
  try {
    if (!other->_managedObject || !_managedObject)
      return false;
    return _managedObject->Equals(other->_managedObject);

  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::AccessibleWr::canFocus() const {
  try {
    return _managedObject->CanFocus;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isFocused() const {
  try {
    return _managedObject->IsFocused;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setFocused() {
  try {
    _managedObject->SetFocused();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setCheckState(CheckState state) {
  try {
    _managedObject->CheckState = mapCheckStateEnum(state);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

CheckState AccessibleWr::getCheckState() const {
  try {
    return mapCheckStateEnum(_managedObject->CheckState);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

double AccessibleWr::getValue() const {
  try {
    return _managedObject->Value;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

double AccessibleWr::getMaxValue() const {
  try {
    return _managedObject->MaxValue;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

double AccessibleWr::getMinValue() const {
  try {
    return _managedObject->MinValue;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setValue(const double value) {
  try {
    _managedObject->Value = value;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

double aal::AccessibleWr::getRange() const {
  try {
    return _managedObject->MaxValue - _managedObject->MinValue;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::activate() {
  try {
    _managedObject->ActivateControl();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setActiveTabPage(std::string const& title) {
  try {
    System::String ^value = gcnew System::String(title.c_str());
    _managedObject->ActiveTabPage = value;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getActiveTabPage() const {
  try {
    return NativeToCppString(_managedObject->ActiveTabPage);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isActiveTabPage() const {
  try {
    return _managedObject->IsActiveTabPage;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::RefList AccessibleWr::tabPages() const {
  try {
    cli::array<aal::AccessibleNet ^> ^list = _managedObject->TabPages();
    RefList result;
    for (int i = 0; i < list->Length; ++i) {
      result.emplace_back(new aal::AccessibleWr(list[i]));
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::fromPoint(geometry::Point point) {
  try {
    AccessibleNet ^element = _managedObject->FromPoint(System::Windows::Point(point.x, point.y));
    if (element != nullptr) {
      return std::unique_ptr<AccessibleWr>(new AccessibleWr(element));
    }
    return Ref();

  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::getContainingRow() const {
  try {
    AccessibleNet ^element = _managedObject->ContainingRow;
    if (element != nullptr) {
      return std::unique_ptr<AccessibleWr>(new AccessibleWr(element));
    }
    return Ref();

  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::RefList AccessibleWr::rows() const {
  try {
    cli::array<aal::AccessibleNet ^> ^list = _managedObject->Rows();
    RefList result;
    for (int i = 0; i < list->Length; ++i) {
      result.emplace_back(new aal::AccessibleWr(list[i]));
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::RefList AccessibleWr::rowEntries() const {
  try {
    cli::array<aal::AccessibleNet ^> ^list = _managedObject->RowEntries();
    RefList result;
    for (int i = 0; i < list->Length; ++i) {
      result.emplace_back(new aal::AccessibleWr(list[i]));
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::RefList AccessibleWr::columns() const {
  try {
    cli::array<aal::AccessibleNet ^> ^list = _managedObject->Columns();
    RefList result;
    for (int i = 0; i < list->Length; ++i) {
      result.emplace_back(new aal::AccessibleWr(list[i]));
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::Ref AccessibleWr::getHeader() const {
  try {
    AccessibleNet ^element = _managedObject->ColumnHeader();
    if (element != nullptr) {
      return std::unique_ptr<AccessibleWr>(new AccessibleWr(element));
    }
    return Ref();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleWr::RefList AccessibleWr::columnEntries() const {
  try {
    cli::array<aal::AccessibleNet ^> ^list = _managedObject->ColumnEntries();
    RefList result;
    for (int i = 0; i < list->Length; ++i) {
      result.emplace_back(new aal::AccessibleWr(list[i]));
    }
    return result;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  };
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::isSelected() const {
  try {
    return _managedObject->Selected;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setSelected(bool value) {
  try {
    _managedObject->Selected = value;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getPlatformRoleName() const {
  SWA::ControlType ^type = _managedObject->ControlType;
  return NativeToCppString(type->ProgrammaticName);
}

//----------------------------------------------------------------------------------------------------------------------

bool AccessibleWr::takeScreenShot(std::string const& path, bool onlyWindow, geometry::Rectangle rect) const {
  try {
    System::String ^value = gcnew System::String(path.c_str());
    System::Windows::Rect managedRect(rect.position.x, rect.position.y, rect.size.width, rect.size.height);
    _managedObject->TakeScreenShot(value, onlyWindow, managedRect);
    return true;
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleNet::MouseButton AccessibleWr::mapButtonEnum(const MouseButton button) {
  switch (button) {
  case MouseButton::Left:
    return AccessibleNet::MouseButton::Left;
  case MouseButton::Right:
    return AccessibleNet::MouseButton::Right;
  case MouseButton::Middle:
    return AccessibleNet::MouseButton::Middle;
  default:
    return AccessibleNet::MouseButton::Left;
  }
}

//----------------------------------------------------------------------------------------------------------------------

SWA::ToggleState AccessibleWr::mapCheckStateEnum(const CheckState state) const {
  switch (state) {
  case CheckState::Checked:
    return SWA::ToggleState::On;
  case CheckState::Unchecked:
    return SWA::ToggleState::Off;
  case CheckState::Indeterminate:
    return SWA::ToggleState::Indeterminate;
  default:
    return SWA::ToggleState::Indeterminate;
  }
}

//----------------------------------------------------------------------------------------------------------------------

CheckState AccessibleWr::mapCheckStateEnum(SWA::ToggleState state) const {
  switch (state) {
  case SWA::ToggleState::On:
    return CheckState::Checked;
  case SWA::ToggleState::Off:
    return CheckState::Unchecked;
  case SWA::ToggleState::Indeterminate:
    return CheckState::Indeterminate;
  default:
    return CheckState::Indeterminate;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::printNativeInfo() const {
  try {
    _managedObject->PrintNativeInfo();
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::highlight() const {
  try {
    _managedObject->Highlight(false);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::removeHighlight() const {
  try {
    _managedObject->Highlight(true);
  } catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string AccessibleWr::getClipboardText()
{
  try {
    System::String^ text = AccessibleNet::ClipboardText;
    return  NativeToCppString(text);
  }
  catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

void AccessibleWr::setClipboardText(const std::string& content)
{
  try {
    AccessibleNet::ClipboardText = gcnew System::String(content.c_str());
  }
  catch (System::Exception ^e) {
    std::string message = NativeToCppString(e->ToString());
    throw std::runtime_error(message.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

