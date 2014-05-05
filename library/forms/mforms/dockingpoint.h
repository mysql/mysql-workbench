/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MFORMS_DOCKINGPOINT_H_
#define _MFORMS_DOCKINGPOINT_H_

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
  class MFORMS_EXPORT DockingPointDelegate
  {
    friend class DockingPoint;
  protected:
    DockingPoint *_dpoint;
  public:
    virtual ~DockingPointDelegate() {};
    
    virtual std::string get_type() = 0;
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
  class MFORMS_EXPORT DockingPoint : public Object
  {
  protected:
    DockingPoint() {}
  public:
#ifndef SWIG
    DockingPoint(DockingPointDelegate *delegate, bool delete_on_destroy);
#endif
  public:
    ~DockingPoint();

//    std::string get_name();
    std::string get_type();

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
    
    /** Undocks an AppView from the main window.
     */
    void undock_view(AppView *view);
    
    /** Closes the AppView as if the user clicks its close button */
    bool close_view(AppView *view);
    
    /** Sets the title of a previously docked AppView. 
     
     In Workbench this will set the label of the Tab where the AppView is docked.
     */
    void set_view_title(AppView *view, const std::string &title);
    
    /** Gets the size of the view. */
    std::pair<int, int> get_size();

    /** Gets the number of AppViews docked */
    virtual int view_count();

    /** Gets the AppView docket at the given index */
    virtual AppView *view_at_index(int index);

#ifndef SWIG
    DockingPointDelegate *get_delegate() { return _delegate; }

    boost::signals2::signal<void ()>* signal_view_switched() { return &_view_switched; }

    //XXX Linux, Windows: need to call this whenever a tab is switched (main tabs and connection tabs)
    void view_switched();
#endif
  protected:
    DockingPointDelegate *_delegate;
    boost::signals2::signal<void ()> _view_switched;

    bool _delete_delegate;
  };
};

#endif
