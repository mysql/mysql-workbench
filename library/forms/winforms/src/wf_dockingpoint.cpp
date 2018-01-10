/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include "wf_box.h"
#include "wf_appview.h"
#include "wf_dockingpoint.h"

using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

class MySQL::Forms::DockingPointDelegateWrapper : public mforms::DockingPointDelegate {
private:
  gcroot<Object ^> _represented_object;
  gcroot<ManagedDockDelegate ^> _managed_delegate;

public:
  DockingPointDelegateWrapper(ManagedDockDelegate ^ owner, Object ^ represented_object) {
    _managed_delegate = owner;
    _represented_object = represented_object;
  }

  virtual std::string get_type() override {
    return NativeToCppString(_managed_delegate->get_type(_represented_object));
  }

  virtual void set_name(const std::string &name) override {
  }

  virtual void dock_view(mforms::AppView *view, const std::string &arg1, int arg2) override {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    if (view->release_on_add())
      view->set_release_on_add(false);
    else
      view->retain();
    _managed_delegate->dock_view(_represented_object, wrapper->GetHost(), CppStringToNative(arg1), arg2);
  }

  virtual bool select_view(mforms::AppView *view) override {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    return _managed_delegate->select_view(_represented_object, wrapper->GetHost());
  }

  virtual void undock_view(mforms::AppView *view) override {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    _managed_delegate->undock_view(_represented_object, wrapper->GetHost());
    view->release();
  }

  virtual void set_view_title(mforms::AppView *view, const std::string &title) override {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    _managed_delegate->set_view_title(_represented_object, wrapper->GetHost(), CppStringToNative(title));
  }

  virtual std::pair<int, int> get_size() override {
    Drawing::Size size = _managed_delegate->get_size(_represented_object);
    return std::make_pair(size.Width, size.Height);
  }

  virtual int view_count() override {
    return _managed_delegate->view_count();
  }

  virtual mforms::AppView *view_at_index(int i) override {
    AppViewDockContent ^ appview = _managed_delegate->view_at_index(i);
    if (appview == nullptr)
      return NULL;
    return appview->GetBackend();
  }

  virtual mforms::AppView *selected_view() override {
    AppViewDockContent ^ appview = _managed_delegate->selected_view();
    if (appview == nullptr)
      return NULL;
    return appview->GetBackend();
  }
};

//----------------- ManagedDockDelegate ------------------------------------------------------------

ManagedDockDelegate::ManagedDockDelegate(Object ^ represented_object) {
  wrapper = new DockingPointDelegateWrapper(this, represented_object);
}

//--------------------------------------------------------------------------------------------------

ManagedDockDelegate::~ManagedDockDelegate() {
  delete wrapper;
}

//--------------------------------------------------------------------------------------------------

mforms::DockingPointDelegate *ManagedDockDelegate::get_unmanaged_delegate() {
  return wrapper;
}

//--------------------------------------------------------------------------------------------------
