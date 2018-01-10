/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_tabview.h"
#include "wf_panel.h"

using namespace System::Drawing;
using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Controls;
using namespace MySQL::Utilities;

//----------------- MformsStandardTabControl -------------------------------------------------------

ref class MformsStandardTabControl : TabControl {
public:
  virtual void OnSelected(TabControlEventArgs ^ args) override {
    __super ::OnSelected(args);

    mforms::TabView *tabview = TabViewWrapper::GetBackend<mforms::TabView>(this);
    if (tabview != NULL)
      (*tabview->signal_tab_changed())();
  }
};

//----------------- MformsFlatTabControl -----------------------------------------------------------

ref class MformsFlatTabControl : FlatTabControl {
public:
  virtual void OnTabShowMenu(TabMenuEventArgs ^ args) override {
    __super ::OnTabShowMenu(args);

    mforms::TabView *tabview = TabViewWrapper::GetBackend<mforms::TabView>(this);
    tabview->set_menu_tab(args->pageIndex);
    if (tabview->get_tab_menu()) {
      tabview->get_tab_menu()->popup_at(tabview, base::Point(args->location.X, args->location.Y));
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnSelected(TabControlEventArgs ^ args) override {
    __super ::OnSelected(args);

    mforms::TabView *tabview = TabViewWrapper::GetBackend<mforms::TabView>(this);
    if (tabview != NULL)
      (*tabview->signal_tab_changed())();
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnTabClosing(TabClosingEventArgs ^ args) override {
    __super ::OnTabClosing(args);

    mforms::TabView *tabview = TabViewWrapper::GetBackend<mforms::TabView>(this);
    if (tabview != NULL)
      args->canClose = tabview->can_close_tab(args->index);
  }

  //------------------------------------------------------------------------------------------------

  virtual void OnTabMoved(TabMovedEventArgs ^ args) override {
    __super ::OnTabMoved(args);
    mforms::TabView *tabview = TabViewWrapper::GetBackend<mforms::TabView>(this);
    if (tabview != NULL && args->MovedPage->Controls->Count > 0) {
      mforms::View *view = ViewWrapper::GetBackend<mforms::View>(args->MovedPage->Controls[0]);
      tabview->reordered(view, args->ToIndex);
    }
  }

  //------------------------------------------------------------------------------------------------
};

//--------------------------------------------------------------------------------------------------

/**
 * Helper method for basic setup of a flat tabview glued to the backend tabview by the given wrapper.
 */
FlatTabControl ^
  CreateFlatTabControl(mforms::TabView *backend, TabViewWrapper *wrapper) {
    MformsFlatTabControl ^ result = TabViewWrapper::Create<MformsFlatTabControl>(backend, wrapper);
    result->UpdateColors();

    result->CanCloseLastTab = false;
    result->HideWhenEmpty = false;
    result->ItemPadding = Padding(6, 0, 6, 0);
    result->ItemSize = Size(0, 21);
    result->Margin = Padding(6, 0, 6, 0);

    return result;
  }

  //----------------- TabViewWrapper -----------------------------------------------------------------

  TabViewWrapper::TabViewWrapper(mforms::TabView *backend, mforms::TabViewType type)
  : ViewWrapper(backend) {
  tabType = type;
  activeIndex = -1;
}

//--------------------------------------------------------------------------------------------------

bool TabViewWrapper::create(mforms::TabView *backend, mforms::TabViewType type) {
  TabViewWrapper *wrapper = new TabViewWrapper(backend, type);

  switch (type) {
    case mforms::TabViewTabless: {
      FlatTabControl ^ tabControl = CreateFlatTabControl(backend, wrapper);
      tabControl->Margin = Padding(0);
      tabControl->ShowFocusState = false;
      tabControl->TabStyle = FlatTabControl::TabStyleType::NoTabs;
      break;
    }

    case mforms::TabViewMainClosable: {
      FlatTabControl ^ tabControl = CreateFlatTabControl(backend, wrapper);
      tabControl->ContentPadding = Padding(0, 6, 0, 6);
      tabControl->ShowCloseButton = true;
      tabControl->ShowFocusState = false;
      tabControl->TabStyle = FlatTabControl::TabStyleType::TopTransparent;
      break;
    }

    case mforms::TabViewDocument:
    case mforms::TabViewDocumentClosable: {
      FlatTabControl ^ tabControl = CreateFlatTabControl(backend, wrapper);
      tabControl->Margin = Padding(0);
      tabControl->ShowCloseButton = type == mforms::TabViewDocumentClosable;
      tabControl->ShowFocusState = true;
      tabControl->TabStyle = FlatTabControl::TabStyleType::TopNormal;
      tabControl->ContentPadding = Padding(0, 4, 0, 0);

      break;
    }

    case mforms::TabViewPalette:
    case mforms::TabViewSelectorSecondary: {
      FlatTabControl ^ tabControl = CreateFlatTabControl(backend, wrapper);
      tabControl->Margin = Padding(0);
      tabControl->ItemSize = Size(0, 19);
      tabControl->ShowCloseButton = false;
      tabControl->ShowFocusState = false;
      tabControl->TabStyle = FlatTabControl::TabStyleType::BottomNormal;

      break;
    }

    case mforms::TabViewEditorBottom: {
      FlatTabControl ^ tabControl = CreateFlatTabControl(backend, wrapper);
      tabControl->Margin = Padding(0);
      tabControl->ItemSize = Size(0, 19);
      tabControl->ShowCloseButton = true;
      tabControl->ShowFocusState = false;
      tabControl->CanCloseLastTab = true;
      tabControl->TabStyle = FlatTabControl::TabStyleType::BottomNormal;
      break;
    }

    default: // mforms::TabViewSystemStandard
      TabViewWrapper::Create<MformsStandardTabControl>(backend, wrapper);
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

void TabViewWrapper::set_active_tab(mforms::TabView *backend, int index) {
  TabViewWrapper *wrapper = backend->get_data<TabViewWrapper>();
  wrapper->GetManagedObject<TabControl>()->SelectedIndex = index;
  (*backend->signal_tab_changed())();

  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

int TabViewWrapper::get_active_tab(mforms::TabView *backend) {
  TabViewWrapper *wrapper = backend->get_data<TabViewWrapper>();
  return wrapper->GetManagedObject<TabControl>()->SelectedIndex;
}

//--------------------------------------------------------------------------------------------------

int TabViewWrapper::add_page(mforms::TabView *backend, mforms::View *page, const std::string &caption,
                             bool hasCloseButton) {
  TabViewWrapper *wrapper = backend->get_data<TabViewWrapper>();
  int new_index = -1;

  ViewWrapper *view = page->get_data<ViewWrapper>();
  view->set_resize_mode(AutoResizeMode::ResizeNone);

  TabPage ^ tabPage = gcnew TabPage();

  Control ^ control = view->GetControl();
  control->Dock = DockStyle::Fill;
  tabPage->Controls->Add(control);
  tabPage->Text = CppStringToNative(caption);
  tabPage->BackColor = control->BackColor;

  TabControl ^ tabControl = wrapper->GetManagedObject<TabControl>();
  tabControl->TabPages->Add(tabPage);

  new_index = tabControl->TabPages->Count - 1;

  // Set the page as active page if this is the first page that was added.
  if (new_index == 0)
    set_active_tab(backend, new_index);

  backend->set_layout_dirty(true);

  return new_index;
}

//--------------------------------------------------------------------------------------------------

void TabViewWrapper::remove_page(mforms::TabView *backend, mforms::View *page) {
  TabViewWrapper *wrapper = backend->get_data<TabViewWrapper>();
  ViewWrapper *view = page->get_data<ViewWrapper>();

  Control ^ control = view->GetControl();

  TabControl ^ tabControl = wrapper->GetManagedObject<TabControl>();
  for each(TabPage ^ native_page in tabControl->TabPages) {
      if (native_page->Controls->Count > 0 && native_page->Controls[0] == control) {
        tabControl->TabPages->Remove(native_page);
        break;
      }
    }
  backend->set_layout_dirty(true);
}

//--------------------------------------------------------------------------------------------------

void TabViewWrapper::set_tab_title(mforms::TabView *backend, int tab, const std::string &caption) {
  TabControl ^ tabControl = TabViewWrapper::GetManagedObject<TabControl>(backend);
  if (tab >= 0 && tab < tabControl->TabPages->Count)
    tabControl->TabPages[tab]->Text = CppStringToNative(caption);
}

//--------------------------------------------------------------------------------------------------

void TabViewWrapper::set_aux_view(mforms::TabView *backend, mforms::View *aux) {
  FlatTabControl ^ tabControl = TabViewWrapper::GetManagedObject<FlatTabControl>(backend);

  if (tabControl != nullptr) {
    Control ^ control = TabViewWrapper::GetManagedObject<Control>(aux);
    if (control != nullptr) {
      tabControl->AuxControl = control;
      return;
    }
  }
#ifdef _DEBUG
  throw std::invalid_argument("invalid args to set_aux_view");
#endif
}

//--------------------------------------------------------------------------------------------------

void TabViewWrapper::set_allows_reordering(mforms::TabView *backend, bool flag) {
  FlatTabControl ^ tabControl = TabViewWrapper::GetManagedObject<FlatTabControl>(backend);
  if (tabControl != nullptr)
    tabControl->CanReorderTabs = flag;
}

//--------------------------------------------------------------------------------------------------

void TabViewWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_tabview_impl.create = &TabViewWrapper::create;
  f->_tabview_impl.get_active_tab = &TabViewWrapper::get_active_tab;
  f->_tabview_impl.set_active_tab = &TabViewWrapper::set_active_tab;
  f->_tabview_impl.add_page = &TabViewWrapper::add_page;
  f->_tabview_impl.remove_page = &TabViewWrapper::remove_page;
  f->_tabview_impl.set_tab_title = &TabViewWrapper::set_tab_title;
  f->_tabview_impl.set_aux_view = &TabViewWrapper::set_aux_view;
  f->_tabview_impl.set_allows_reordering = &TabViewWrapper::set_allows_reordering;
}

//--------------------------------------------------------------------------------------------------
