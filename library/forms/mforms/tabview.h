/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _MFORMS_TABVIEW_H_
#define _MFORMS_TABVIEW_H_

#include <mforms/view.h>

namespace mforms
{
  class TabView;

  enum TabViewType
  {
    TabViewSystemStandard = 0, //!< Normal tab views with tabs on top in the style used by the system.
    TabViewTabless = 1,        //!< System style tab view without tabs. Switching tabs can then be
                               //   performed programmatically only. 
    TabViewMainClosable,       //!< WB main style tab view (top hanging tabs on Win), closable tabs
    TabViewDocument,           //!< WB style for tabbed documents (top standing tabs, sql editors 
                               //   and such). unclosable tabs
    TabViewDocumentClosable,   //!< WB style for tabbed documents (top standing tabs, sql editors 
                               //   and such). closable tabs
    TabViewPalette,            //!< WB style tab view (bottom hanging tabs on Win), unclosable tabs
    TabViewSelectorSecondary,  //!< Sidebar palette selector style, unclosable tabs.
  };
  
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT TabViewImplPtrs
  {
    bool (*create)(TabView*,TabViewType);
    void (*set_active_tab)(TabView*,int);
    void (*set_tab_title)(TabView*,int,const std::string&);
    int (*get_active_tab)(TabView*);
    int (*add_page)(TabView*,View*,const std::string&);
    void (*remove_page)(TabView*,View*);
  };
#endif
#endif
  
  /** A Notebook/Tabbed view. */
  class MFORMS_EXPORT TabView : public View
  {
    TabViewImplPtrs *_tabview_impl;

    boost::signals2::signal<void ()> _signal_tab_changed;
    boost::signals2::signal<bool (int)> _signal_tab_closing;
    boost::signals2::signal<void (int)> _signal_tab_closed;

  public:
    /** Constructor.
     
     @param type - Type of the tabView. See @ref TabViewType */
    TabView(TabViewType tabType = TabViewSystemStandard);

    /** Sets the currently selected/active tab */
    void set_active_tab(int index);
    /** Get currently selected tab */
    int get_active_tab();
    /** Add a new page to the tab view, with its tab caption. */
    int add_page(View *page, const std::string& caption);
    /** Remove a tab page by its content. */
    void remove_page(View *page);

    /** Sets the caption of the tab page at the given index. */
    void set_tab_title(int page, const std::string& caption);

    /** Returns the index of the tab page or -1 if its not found */
    int get_page_index(View *page);    
    /** Returns the page object at the given page index */
    View *get_page(int index);

    /** Returns true if the tab with the given index can be closed. */
    bool can_close_tab(int index);

#ifndef SWIG
    /** Signal emitted when the tab is switched by user.
     
     In Python use add_tab_changed_callback()
     */
    boost::signals2::signal<void ()>* signal_tab_changed() { return &_signal_tab_changed; }
    
    /** Callback called when a tab is about to close. Returning false will prevent the closure. */
    boost::signals2::signal<bool (int)>* signal_tab_closing() { return &_signal_tab_closing; }

    /** Callback called when a tab has been closed. */
    boost::signals2::signal<void (int)>* signal_tab_closed() { return &_signal_tab_closed; }
#endif
  };
};


#endif
