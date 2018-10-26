/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/ui_form.h"

#include "mforms/box.h"
#include "mforms/app.h"

namespace bec {
  class UIForm;
};

namespace mforms {
  class AppView;
  class DockingPoint;
  class ToolBar;
  class MenuBar;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct AppViewImplPtrs {
    // I *need* an own create function, even if AppView is derived from Box.
    // Otherwise I cannot tell appart which managed wrapper to create.
    bool (*create)(AppView *self, bool horizontal);
  };
#endif
#endif

  /** A view that is dockable into the host application window.

   Provides some functionality specific to views that are embedded in a
   window not owned by mforms. This class is a subclass of Box, so it can
   be used as a container for multiple subviews as well.
   */
  class MFORMS_EXPORT AppView : public Box, public bec::UIForm {
  private:
#ifdef _MSC_VER
    AppViewImplPtrs *_app_view_impl;
#endif
    std::function<bool()> _on_close_slot;
    std::string _context_name;
    std::string _identifier;
    std::string _title;
    mforms::MenuBar *_menubar;
    mforms::ToolBar *_toolbar;
    bool _is_main;

    // for docked views
    mforms::DockingPoint *_dpoint;

  public:
#ifdef _MSC_VER
    /** Constructor.

     @param horiz - whether subviews are to be laid out horizontally instead of vertically
     @param context_name - name for Workbench internal context. Use a unique name.
     @param is_main - pass true
     */
    AppView(bool horiz, const std::string &accessibilityName, const std::string &context_name, bool is_main);
#else
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    AppView(bool horiz, const std::string &accessibilityName, const std::string &context_name, bool is_main);
#endif
#endif
    virtual ~AppView();

    /** Sets the title of this view, when docked. Alias for App::set_view_title()
     */
    void set_title(const std::string &title);

    virtual std::string get_title();

    /** Sets the unique identifier for this view.

     The identifier cannot be changed once it is docked. */
    void set_identifier(const std::string &identifier) {
      _identifier = identifier;
    }

    /** Gets the previously unique identifier for this view. */
    std::string identifier() const {
      return _identifier;
    }

    virtual void close();

  public:
#ifndef SWIG
    /** Sets the callback to be called when the view is closed in the host window.

     This is called when eg. the user closes the tab view. Return true from the
     callback if the view should be undocked and false to prevent that.
     */
    void set_on_close(const std::function<bool()> &slot) {
      _on_close_slot = slot;
    }
#endif

    mforms::MenuBar *get_menubar() {
      return _menubar;
    }
    void set_menubar(mforms::MenuBar *menu);

    mforms::ToolBar *get_toolbar() {
      return _toolbar;
    }
    void set_toolbar(mforms::ToolBar *toolbar);

    /** Internal use */
    virtual bool on_close();

  public:
    virtual bool is_main_form() {
      return _is_main;
    }
    virtual std::string get_form_context_name() const {
      return _context_name;
    }

#ifndef SWIG
    void set_containing_docking_point(mforms::DockingPoint *dpoint);
    mforms::DockingPoint *containing_docking_point() {
      return _dpoint;
    }
#endif
  };
};
