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

#include <mforms/view.h>

namespace mforms {
  class TabView;
  class ContextMenu;

  enum TabViewType {
    TabViewSystemStandard = 0, //!< Normal tab views with tabs on top in the style used by the system.
    TabViewTabless = 1,        //!< System style tab view without tabs. Switching tabs can then be
                               //   performed programmatically only.
    TabViewMainClosable,       //!< WB main style tab view (top hanging tabs on Win), closable tabs
    TabViewDocument,           //!< WB style for tabbed documents (top standing tabs, sql editors
                               //   and such). unclosable tabs
    TabViewDocumentClosable,   //!< WB style for tabbed documents (top standing tabs, sql editors
                               //   and such). closable tabs
    TabViewPalette,           //!< WB style tab view (bottom hanging tabs on Win), unclosable tabs
    TabViewSelectorSecondary, //!< Sidebar palette selector style, unclosable tabs.
    TabViewEditorBottom,      //!< Bottom facing, closable tabs to be used for docking editors
    TabViewEditorBottomPinnable //!< same as earlier, but with pins
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT TabViewImplPtrs {
    bool (*create)(TabView *, TabViewType);
    void (*set_active_tab)(TabView *, int);
    void (*set_tab_title)(TabView *, int, const std::string &);
    int (*get_active_tab)(TabView *);
    int (*add_page)(TabView *, View *, const std::string &, bool);
    void (*remove_page)(TabView *, View *);
    void (*set_aux_view)(TabView *, View *);
    void (*set_allows_reordering)(TabView *, bool);
  };
#endif
#endif

  /** A Notebook/Tabbed view. */
  class MFORMS_EXPORT TabView : public View {
    TabViewImplPtrs *_tabview_impl;
    TabViewType _type;
    View *_aux_view;
    int _menu_tab;
    ContextMenu *_tab_menu;

    boost::signals2::signal<void()> _signal_tab_changed;
    boost::signals2::signal<void(View *, int, int)> _signal_tab_reordered;
    boost::signals2::signal<bool(int)> _signal_tab_closing;
    boost::signals2::signal<void(View *, int)> _signal_tab_closed;
    boost::signals2::signal<void(int, bool)> _signal_tab_pin_changed;

  public:
    /** Constructor.

     @param type - Type of the tabView. See @ref TabViewType */
    TabView(TabViewType tabType = TabViewSystemStandard);
    virtual ~TabView();

    TabViewType get_type() const {
      return _type;
    }

    /** Sets the currently selected/active tab */
    void set_active_tab(int index);
    /** Get currently selected tab */
    int get_active_tab();
    /** Add a new page to the tab view, with its tab caption. */
    int add_page(View *page, const std::string &caption, bool hasCloseButton = true);
    /** Remove a tab page by its content. */
    void remove_page(View *page);

    /** Sets the caption of the tab page at the given index. */
    void set_tab_title(int page, const std::string &caption);

    /** Returns the index of the tab page or -1 if its not found */
    int get_page_index(View *page);
    /** Returns the page object at the given page index */
    View *get_page(int index);

    /** Number of tabs in the control */
    int page_count();

    /** Returns true if the tab with the given index can be closed. */
    bool can_close_tab(int index);

    /** Whether the tabs can be reordered by the user by dragging (supported by select tabview types) */
    void set_allows_reordering(bool flag);

    /** Sets a menu to be shown when right clicking on a tab (supported by select tabview types) */
    void set_tab_menu(ContextMenu *menu);
    ContextMenu *get_tab_menu() {
      return _tab_menu;
    }

    /** Returns the index of the tab for which the context menu is being shown */
    int get_menu_tab();

    void set_aux_view(View *view);
    View *get_aux_view() {
      return _aux_view;
    }

#ifndef SWIG
    void set_menu_tab(int tab);

    void reordered(View *view, int index);

    void pin_changed(int tab, bool pinned);

    /** Signal emitted when the tab is switched by user.

     In Python use add_tab_changed_callback()
     */
    boost::signals2::signal<void()> *signal_tab_changed() {
      return &_signal_tab_changed;
    }

    boost::signals2::signal<void(View *, int, int)> *signal_tab_reordered() {
      return &_signal_tab_reordered;
    }

    /** Callback called when a tab is about to close. Returning false will prevent the closure. */
    boost::signals2::signal<bool(int)> *signal_tab_closing() {
      return &_signal_tab_closing;
    }

    boost::signals2::signal<void(View *, int)> *signal_tab_closed() {
      return &_signal_tab_closed;
    }

    boost::signals2::signal<void(int, bool)> *signal_tab_pin_changed() {
      return &_signal_tab_pin_changed;
    }

    std::function<bool(int)> is_pinned;
#endif
  };
};
