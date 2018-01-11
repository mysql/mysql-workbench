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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_menu.h"

using namespace System::ComponentModel;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Controls;
using namespace MySQL::Forms;

ref class MformsMenuStrip : public ContextMenuStrip {
public:
  mforms::Menu *backend;

  virtual void OnOpening(CancelEventArgs ^ args) override {
    (*(backend->signal_will_show()))();
  }

  //------------------------------------------------------------------------------------------------

  void ItemClick(Object ^ sender, System::EventArgs ^ args) {
    ToolStripItem ^ item = dynamic_cast<ToolStripItem ^>(sender);
    if (item != nullptr) {
      String ^ action = dynamic_cast<String ^>(item->Tag);
      if (action != nullptr)
        backend->handle_action(NativeToCppString(action));
    }
  }

  //------------------------------------------------------------------------------------------------
};

//----------------- MenuWrapper ---------------------------------------------------------------------

MenuWrapper::MenuWrapper(mforms::Menu *backend) : ObjectWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool MenuWrapper::create(mforms::Menu *backend) {
  MenuWrapper *wrapper = new MenuWrapper(backend);
  MformsMenuStrip ^ menuStrip = MenuWrapper::Create<MformsMenuStrip>(backend, wrapper);
  menuStrip->backend = backend;
  menuStrip->AutoSize = true;

  switch (base::Color::get_active_scheme()) {
    case base::ColorSchemeCustom:
    case base::ColorSchemeHighContrast:
    case base::ColorSchemeStandard:
    case base::ColorSchemeStandardWin7:
      menuStrip->Renderer = gcnew TransparentMenuStripRenderer();
      break;

    default:
      menuStrip->Renderer = gcnew Win8MenuStripRenderer();
      break;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

void MenuWrapper::remove_item(mforms::Menu *backend, int i) {
  ContextMenuStrip ^ menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(backend);
  menu->Items->RemoveAt(i);
}

//--------------------------------------------------------------------------------------------------

int MenuWrapper::add_item(mforms::Menu *backend, const std::string &caption, const std::string &action) {
  MformsMenuStrip ^ menu = MenuWrapper::GetManagedObject<MformsMenuStrip>(backend);
  ToolStripItem ^ item = menu->Items->Add(CppStringToNative(caption));
  item->Click += gcnew System::EventHandler(menu, &MformsMenuStrip::ItemClick);
  item->Tag = CppStringToNative(action);

  return menu->Items->Count - 1;
}

//--------------------------------------------------------------------------------------------------

int MenuWrapper::add_separator(mforms::Menu *backend) {
  ContextMenuStrip ^ menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(backend);
  menu->Items->Add(gcnew ToolStripSeparator());
  return menu->Items->Count - 1;
}

//--------------------------------------------------------------------------------------------------

int MenuWrapper::add_submenu(mforms::Menu *backend, const std::string &caption, mforms::Menu *submenu) {
  ContextMenuStrip ^ menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(backend);
  ContextMenuStrip ^ child_menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(submenu);

  ToolStripMenuItem ^ item = gcnew ToolStripMenuItem(CppStringToNative(caption));
  menu->Items->Add(item);
  item->DropDown = child_menu;

  return menu->Items->Count - 1;
}

//--------------------------------------------------------------------------------------------------

void MenuWrapper::set_item_enabled(mforms::Menu *backend, int i, bool flag) {
  ContextMenuStrip ^ menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(backend);
  menu->Items[i]->Enabled = flag;
}

//--------------------------------------------------------------------------------------------------

void MenuWrapper::popup_at(mforms::Menu *backend, mforms::Object *control, int x, int y) {
  // We need the the .NET control for which to show the context menu.
  Control ^ controller = nullptr;
  if (control != NULL)
    controller = MenuWrapper::GetControl(control);
  else {
    // If we did not get a control passed in then we take the one under the mouse
    // determined by the last message position. This should usually work as the context menu
    // is triggered by a right mouse button click.
    DWORD position = GetMessagePos();
    POINT point;
    point.x = (int)(short)LOWORD(position);
    point.y = (int)(short)HIWORD(position);
    HWND window = WindowFromPoint(point);
    controller = Control::FromHandle(IntPtr(window));
  }

  if (controller != nullptr) {
    ContextMenuStrip ^ menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(backend);
    menu->Show(controller, System::Drawing::Point(x, y));
  }
}

//--------------------------------------------------------------------------------------------------

void MySQL::Forms::MenuWrapper::clear(mforms::Menu *backend) {
  ContextMenuStrip ^ menu = MenuWrapper::GetManagedObject<ContextMenuStrip>(backend);
  menu->Items->Clear();
}

//--------------------------------------------------------------------------------------------------

void MenuWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_menu_impl.create = &MenuWrapper::create;
  f->_menu_impl.remove_item = &MenuWrapper::remove_item;
  f->_menu_impl.add_item = &MenuWrapper::add_item;
  f->_menu_impl.add_separator = &MenuWrapper::add_separator;
  f->_menu_impl.add_submenu = &MenuWrapper::add_submenu;
  f->_menu_impl.set_item_enabled = &MenuWrapper::set_item_enabled;
  f->_menu_impl.popup_at = &MenuWrapper::popup_at;
  f->_menu_impl.clear = &MenuWrapper::clear;
}

//--------------------------------------------------------------------------------------------------
