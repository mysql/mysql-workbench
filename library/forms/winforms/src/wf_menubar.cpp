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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_menubar.h"

using namespace System::ComponentModel;
using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;
using namespace MySQL::Controls;

ref class MformsContextMenuStrip : ContextMenuStrip {
protected:
  virtual void OnOpening(CancelEventArgs ^ e) override {
    mforms::ContextMenu *menu = MenuBarWrapper::GetBackend<mforms::ContextMenu>(this);
    if (menu != NULL)
      menu->will_show();
  }
};

//----------------- MenuItemEventTarget ------------------------------------------------------------

ref class MenuItemEventTarget {
public:
  void DropDownOpened(Object ^ sender, System::EventArgs ^ e) {
    ToolStripMenuItem ^ native_item = dynamic_cast<ToolStripMenuItem ^>(sender);
    if (native_item != nullptr) {
      mforms::MenuItem *item = MenuBarWrapper::GetBackend<mforms::MenuItem>(native_item);
      if (item != NULL) {
        mforms::MenuBase *menu = item->get_top_menu();
        if (dynamic_cast<mforms::MenuBar *>(menu) != NULL)
          dynamic_cast<mforms::MenuBar *>(menu)->will_show_submenu_from(item);
        else if (dynamic_cast<mforms::ContextMenu *>(menu) != NULL)
          dynamic_cast<mforms::ContextMenu *>(menu)->will_show_submenu_from(item);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void MenuItemClick(Object ^ sender, System::EventArgs ^ e) {
    mforms::MenuItem *item = MenuBarWrapper::GetBackend<mforms::MenuItem>(static_cast<ToolStripItem ^>(sender));
    if (item != NULL)
      item->callback();
  }
};

//--------------------------------------------------------------------------------------------------

class MenuItemWrapper : public ObjectWrapper {
private:
  gcroot<ToolStripItem ^> item;
  gcroot<MenuItemEventTarget ^> eventTarget;

protected:
  virtual System::Object ^ InternalGetNativeObject() { return item; }

    public : MenuItemWrapper(mforms::MenuItem *backend, const std::string &title, const mforms::MenuItemType type)
    : ObjectWrapper(backend) {
    eventTarget = gcnew MenuItemEventTarget();
    switch (type) {
      case mforms::CheckedMenuItem: {
        item = MenuItemWrapper::Create<ToolStripMenuItem>(backend, this);
        item->Text = CppStringToNative(title);
        item->Click += gcnew System::EventHandler(eventTarget, &MenuItemEventTarget::MenuItemClick);

        // native_item->CheckOnClick = true; Not supported on the other platforms.
      } break;

      case mforms::SeparatorMenuItem: {
        item = MenuItemWrapper::Create<ToolStripSeparator>(backend, this);
        item->AutoSize = false;
        item->Height = 3;
        item->Margin = Padding(32, 0, 2, 0);
      } break;

      default: {
        item = MenuItemWrapper::Create<ToolStripMenuItem>(backend, this);
        item->Text = CppStringToNative(title);
        item->Click += gcnew System::EventHandler(eventTarget, &MenuItemEventTarget::MenuItemClick);
      } break;
    }
  }

  void RegisterDropDown() {
    ToolStripItem ^ stripItem = item;
    ToolStripMenuItem ^ menu = dynamic_cast<ToolStripMenuItem ^>(stripItem);
    menu->DropDownOpening += gcnew System::EventHandler(eventTarget, &MenuItemEventTarget::DropDownOpened);
  }

  void UnregisterDropDown() {
    ToolStripItem ^ stripItem = item;
    ToolStripMenuItem ^ menu = dynamic_cast<ToolStripMenuItem ^>(stripItem);
    menu->DropDownOpening -= gcnew System::EventHandler(eventTarget, &MenuItemEventTarget::DropDownOpened);
  }
};

//----------------- MenuBarWrapper --------------------------------------------------------------------

MenuBarWrapper::MenuBarWrapper(mforms::MenuBase *backend) : ObjectWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool MenuBarWrapper::create_menu_bar(mforms::MenuBar *backend) {
  MenuBarWrapper *wrapper = new MenuBarWrapper(backend);
  MenuStrip ^ menu = Create<MenuStrip>(backend, wrapper);

  menu->AllowDrop = false;
  menu->AllowItemReorder = false;
  menu->AllowMerge = false;
  menu->AutoSize = true;
  menu->CanOverflow = false;
  menu->Dock = DockStyle::Top;
  menu->DefaultDropDownDirection = ToolStripDropDownDirection::BelowRight;
  menu->LayoutStyle = ToolStripLayoutStyle::HorizontalStackWithOverflow;
  menu->Name = "MenuStrip";
  menu->Padding = Padding(0, 2, 0, 2);
  menu->RenderMode = ToolStripRenderMode::System;
  menu->ShowItemToolTips = false;
  menu->Stretch = true;
  menu->TabIndex = 0;
  menu->TabStop = false;

  switch (base::Color::get_active_scheme()) {
    case base::ColorSchemeCustom:
    case base::ColorSchemeHighContrast:
    case base::ColorSchemeStandard:
    case base::ColorSchemeStandardWin7: {
      TransparentMenuStripRenderer ^ renderer = gcnew TransparentMenuStripRenderer();
      menu->Renderer = renderer;
      break;
    }
    default: {
      Win8MenuStripRenderer ^ renderer = gcnew Win8MenuStripRenderer();
      menu->Renderer = renderer;
      break;
    }
  }
  menu->Font = menu->DefaultFont;

  return true;
}

//--------------------------------------------------------------------------------------------------

bool MenuBarWrapper::create_context_menu(mforms::ContextMenu *backend) {
  MenuBarWrapper *wrapper = new MenuBarWrapper(backend);
  MformsContextMenuStrip ^ strip = Create<MformsContextMenuStrip>(backend, wrapper);

  strip->AllowDrop = false;
  strip->AllowItemReorder = false;
  strip->AllowMerge = false;
  strip->AutoSize = true;
  strip->CanOverflow = false;
  strip->Dock = DockStyle::Top;
  strip->DefaultDropDownDirection = ToolStripDropDownDirection::Right;
  strip->LayoutStyle = ToolStripLayoutStyle::HorizontalStackWithOverflow;
  strip->Name = "ContextMenuStrip";
  strip->Padding = Padding(0, 2, 0, 2);
  strip->RenderMode = ToolStripRenderMode::System;
  strip->ShowItemToolTips = false;
  strip->Stretch = true;
  strip->TabIndex = 0;
  strip->TabStop = false;

  switch (base::Color::get_active_scheme()) {
    case base::ColorSchemeCustom:
    case base::ColorSchemeHighContrast:
    case base::ColorSchemeStandard:
    case base::ColorSchemeStandardWin7: {
      TransparentMenuStripRenderer ^ renderer = gcnew TransparentMenuStripRenderer();
      strip->Renderer = renderer;
      break;
    }
    default: {
      Win8MenuStripRenderer ^ renderer = gcnew Win8MenuStripRenderer();
      strip->Renderer = renderer;
      break;
    }
  }
  strip->Font = strip->DefaultFont;

  return true;
}

//--------------------------------------------------------------------------------------------------

bool MenuBarWrapper::create_menu_item(mforms::MenuItem *item, const std::string &title,
                                      const mforms::MenuItemType type) {
  // MenuItemWrapper will itself create the connections to the backend and its native object.
  MenuItemWrapper *wrapper = new MenuItemWrapper(item, title, type);
  return true;
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::set_title(mforms::MenuItem *item, const std::string &title) {
  ToolStripItem ^ object = GetManagedObject<ToolStripItem>(item);
  object->Text = CppStringToNative(title);
}

//--------------------------------------------------------------------------------------------------

std::string MenuBarWrapper::get_title(mforms::MenuItem *item) {
  ToolStripItem ^ object = GetManagedObject<ToolStripItem>(item);
  return NativeToCppString(object->Text);
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::set_name(mforms::MenuItem *item, const std::string &name) {
  ToolStripItem ^ object = GetManagedObject<ToolStripItem>(item);
  object->AccessibleName = CppStringToNative(name);
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::set_shortcut(mforms::MenuItem *item, const std::string &value) {
  ToolStripMenuItem ^ object = GetManagedObject<ToolStripMenuItem>(item);
  object->ShortcutKeys = MenuManager::convertShortcut(CppStringToNative(value));
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::set_enabled(mforms::MenuBase *item, bool state) {
  ToolStripItem ^ object = GetManagedObject<ToolStripItem>(item);
  object->Enabled = state;
}

//--------------------------------------------------------------------------------------------------

bool MenuBarWrapper::get_enabled(mforms::MenuBase *item) {
  ToolStripItem ^ object = GetManagedObject<ToolStripItem>(item);
  return object->Enabled;
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::set_checked(mforms::MenuItem *item, bool state) {
  ToolStripMenuItem ^ object = GetManagedObject<ToolStripMenuItem>(item);
  object->Checked = state;
}

//--------------------------------------------------------------------------------------------------

bool MenuBarWrapper::get_checked(mforms::MenuItem *item) {
  ToolStripMenuItem ^ object = GetManagedObject<ToolStripMenuItem>(item);
  return object->Checked;
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::insert_item(mforms::MenuBase *menu, int index, mforms::MenuItem *item) {
  if (dynamic_cast<mforms::MenuBar *>(menu) != NULL || dynamic_cast<mforms::ContextMenu *>(menu) != NULL) {
    ToolStrip ^ native_menu = MenuBarWrapper::GetManagedObject<ToolStrip>(menu);
    ToolStripItem ^ native_item = MenuBarWrapper::GetManagedObject<ToolStripItem>(item);

    if (native_menu->Items->Count == index)
      native_menu->Items->Add(native_item);
    else
      native_menu->Items->Insert(index, native_item);
  } else {
    ToolStripMenuItem ^ native_parent = MenuBarWrapper::GetManagedObject<ToolStripMenuItem>(menu);
    ToolStripItem ^ native_item = MenuBarWrapper::GetManagedObject<ToolStripItem>(item);

    if (native_parent->DropDownItems->Count == index)
      native_parent->DropDownItems->Add(native_item);
    else
      native_parent->DropDownItems->Insert(index, native_item);

    // Register drop down delegate if this is the first item that was added.
    if (native_parent->DropDownItems->Count == 1) {
      MenuItemWrapper *wrapper = menu->get_data<MenuItemWrapper>();
      wrapper->RegisterDropDown();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::remove_item(mforms::MenuBase *menu, mforms::MenuItem *item) {
  MenuItemWrapper *item_wrapper = (item == NULL) ? NULL : item->get_data<MenuItemWrapper>();
  if (dynamic_cast<mforms::MenuBar *>(menu) != NULL || dynamic_cast<mforms::ContextMenu *>(menu) != NULL) {
    // This is the top menu bar or context menu.
    ToolStrip ^ native_menu = MenuBarWrapper::GetManagedObject<ToolStrip>(menu);

    if (item != NULL) {
      ToolStripMenuItem ^ native_item = MenuBarWrapper::GetManagedObject<ToolStripMenuItem>(menu);
      native_menu->Items->Remove(native_item);
    } else
      native_menu->Items->Clear();
  } else {
    // A menu item with drop down items.
    ToolStripMenuItem ^ native_menu = MenuBarWrapper::GetManagedObject<ToolStripMenuItem>(menu);

    if (item != NULL) {
      ToolStripMenuItem ^ native_item = MenuBarWrapper::GetManagedObject<ToolStripMenuItem>(menu);
      native_menu->DropDownItems->Remove(native_item);
    } else
      native_menu->DropDownItems->Clear();

    // Remove drop down delegate if there are no drop down items anymore.
    if (native_menu->DropDownItems->Count == 0) {
      MenuItemWrapper *wrapper = menu->get_data<MenuItemWrapper>();
      wrapper->UnregisterDropDown();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::popup_at(mforms::ContextMenu *menu, mforms::View *owner, base::Point location) {
  MformsContextMenuStrip ^ native_menu = MenuBarWrapper::GetManagedObject<MformsContextMenuStrip>(menu);
  if (native_menu != nullptr)
    native_menu->Show((int)location.x, (int)location.y);
}

//--------------------------------------------------------------------------------------------------

void MenuBarWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_menu_item_impl.create_menu_bar = &MenuBarWrapper::create_menu_bar;
  f->_menu_item_impl.create_context_menu = &MenuBarWrapper::create_context_menu;
  f->_menu_item_impl.create_menu_item = &MenuBarWrapper::create_menu_item;
  f->_menu_item_impl.set_title = &MenuBarWrapper::set_title;
  f->_menu_item_impl.get_title = &MenuBarWrapper::get_title;
  f->_menu_item_impl.set_name = &MenuBarWrapper::set_name;
  f->_menu_item_impl.set_shortcut = &MenuBarWrapper::set_shortcut;
  f->_menu_item_impl.set_enabled = &MenuBarWrapper::set_enabled;
  f->_menu_item_impl.get_enabled = &MenuBarWrapper::get_enabled;
  f->_menu_item_impl.set_checked = &MenuBarWrapper::set_checked;
  f->_menu_item_impl.get_checked = &MenuBarWrapper::get_checked;
  f->_menu_item_impl.insert_item = &MenuBarWrapper::insert_item;
  f->_menu_item_impl.remove_item = &MenuBarWrapper::remove_item;
  f->_menu_item_impl.popup_at = &MenuBarWrapper::popup_at;
}

//--------------------------------------------------------------------------------------------------
