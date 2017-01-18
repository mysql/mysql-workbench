/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

  virtual std::string get_type() {
    return NativeToCppString(_managed_delegate->get_type(_represented_object));
  }

  virtual void dock_view(mforms::AppView *view, const std::string &arg1, int arg2) {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    if (view->release_on_add())
      view->set_release_on_add(false);
    else
      view->retain();
    _managed_delegate->dock_view(_represented_object, wrapper->GetHost(), CppStringToNative(arg1), arg2);
  }

  virtual bool select_view(mforms::AppView *view) {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    return _managed_delegate->select_view(_represented_object, wrapper->GetHost());
  }

  virtual void undock_view(mforms::AppView *view) {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    _managed_delegate->undock_view(_represented_object, wrapper->GetHost());
    view->release();
  }

  virtual void set_view_title(mforms::AppView *view, const std::string &title) {
    AppViewWrapper *wrapper = view->get_data<AppViewWrapper>();
    _managed_delegate->set_view_title(_represented_object, wrapper->GetHost(), CppStringToNative(title));
  }

  virtual std::pair<int, int> get_size() {
    Drawing::Size size = _managed_delegate->get_size(_represented_object);
    return std::make_pair(size.Width, size.Height);
  }

  virtual int view_count() {
    return _managed_delegate->view_count();
  }

  virtual mforms::AppView *view_at_index(int i) {
    AppViewDockContent ^ appview = _managed_delegate->view_at_index(i);
    if (appview == nullptr)
      return NULL;
    return appview->GetBackend();
  }

  virtual mforms::AppView *selected_view() {
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
