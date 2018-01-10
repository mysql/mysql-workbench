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

#pragma once

#include <map>

#include "mforms/base.h"
#include "base/geometry.h"
#include "base/drawing.h"

#include <boost/signals2.hpp>

namespace mforms {

  class AppView;
  class DockingPoint;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  class MFORMS_EXPORT DockingPointDelegate {
    friend class DockingPoint;

  protected:
    DockingPoint *_dpoint;

  public:
    virtual ~DockingPointDelegate(){};

    virtual std::string get_type() = 0;
    virtual void set_name(const std::string &name) = 0;
    virtual void dock_view(AppView *view, const std::string &arg1, int arg2) = 0;
    virtual bool select_view(AppView *view) = 0;
    virtual void undock_view(AppView *view) = 0;
    virtual void set_view_title(AppView *view, const std::string &title) = 0;
    virtual std::pair<int, int> get_size() = 0;

    virtual AppView *selected_view() = 0;
    virtual int view_count() = 0;
    virtual AppView *view_at_index(int index) = 0;
  };
#endif
#endif

  /** Proxy class for interfacing with host application windows.

   Provides some utility functions to perform certain actions in windows
   of the host application.
   */
  class MFORMS_EXPORT DockingPoint : public Object {
    friend class AppView; // for set_view_title().

  protected:
    DockingPoint() : _delegate(nullptr), _delete_delegate(false) {
    }

  public:
#ifndef SWIG
    DockingPoint(DockingPointDelegate *delegate, bool delete_on_destroy);
#endif
  public:
    ~DockingPoint();

    std::string get_type();

    /** Set component name, used mainly for a11y
     */
    void set_name(const std::string &name);

    /** Docks an AppView into a view belonging to the application, at the requested position.

     @param view - an AppView object to be docked in the target
     @param arg1 - target specific string argument
     @param arg2 - target specific int argument

     In Workbench, view will be added into one of the tabs in the application window.
     */
    void dock_view(AppView *view, const std::string &arg1 = "", int arg2 = 0);

    /** Selects a docked view.

     @return false if a view with the identifier could not be found. identifier
     must be set in the AppView before its docked.
     */
    bool select_view(AppView *view);

    /** Returns the currently selected view

        Note that a return value of NULL may just mean that the selected tab is not an AppView.
     */
    AppView *selected_view();

    /** Undocks an AppView from the main window. */
    void undock_view(AppView *view);

    /* Closes the view at the given index if it exists and is an appview. */
    void close_view_at_index(int index);

    /** Checks if all docked views can be closed and if so closes them. Returns true if all of them could be closed. */
    bool close_all_views();

    /** Gets the size of the view. */
    std::pair<int, int> get_size();

    /** Gets the number of AppViews docked */
    virtual int view_count();

    /** Gets the AppView docket at the given index */
    virtual AppView *view_at_index(int index);

#ifndef SWIG
    DockingPointDelegate *get_delegate() {
      return _delegate;
    }

    boost::signals2::signal<void()> *signal_view_switched() {
      return &_view_switched;
    }

    boost::signals2::signal<void(AppView *)> *signal_view_undocked() {
      return &_view_undocked;
    }

    void view_switched();
#endif
  protected:
    DockingPointDelegate *_delegate;
    boost::signals2::signal<void()> _view_switched;
    boost::signals2::signal<void(AppView *)> _view_undocked;

    bool _delete_delegate;

    void set_view_title(AppView *view, const std::string &title);
  };
};
