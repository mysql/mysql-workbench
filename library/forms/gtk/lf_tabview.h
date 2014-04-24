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
#ifndef _LF_TABVIEW_H_
#define _LF_TABVIEW_H_

#include "lf_view.h"
#include "mforms/tabview.h"

namespace mforms
{
namespace gtk
{

class TabViewImpl : public ViewImpl
{
  Gtk::Notebook *_nb;
  virtual Gtk::Widget *get_outer() const { return _nb; }
 protected:
  TabViewImpl(::mforms::TabView *self, ::mforms::TabViewType tabType)
    : ViewImpl(self)
  {
    _nb = new Gtk::Notebook();
    if (tabType == mforms::TabViewTabless)
    {
      _nb->set_show_tabs(false);
      _nb->set_show_border(false);
    }
    _nb->set_scrollable(true);
    _nb->signal_switch_page().connect(sigc::mem_fun(this, &TabViewImpl::tab_changed));
    _nb->show();
  }

  virtual ~TabViewImpl()
  {
    delete _nb;
  }

  void tab_changed(GtkNotebookPage*, guint)
  {
    TabView* tv = dynamic_cast< TabView* >(owner);
    if (tv && !tv->is_destroying())
      (*tv->signal_tab_changed())();
  }

  static bool create(::mforms::TabView *self, mforms::TabViewType tabtype)
  {
    return new TabViewImpl(self, tabtype) != 0;
  }

  static void set_active_tab(::mforms::TabView *self, int index)
  {
    TabViewImpl* cb= self->get_data<TabViewImpl>();

    if ( cb)
    {
      cb->_nb->set_current_page(index);
    }
  }

  static int get_active_tab(::mforms::TabView *self)
  {
    TabViewImpl* cb= self->get_data<TabViewImpl>();

    return cb ? cb->_nb->get_current_page() : -1;
  }

  static int add_page(::mforms::TabView *self, ::mforms::View *page, const std::string& caption)
  {
    int page_index_after_insert = -1;
    TabViewImpl* cb= self->get_data<TabViewImpl>();

    if ( cb)
    {
      ViewImpl *widget_wrapper = page->get_data<ViewImpl>();
      if ( widget_wrapper )
      {
        Gtk::Label *label= Gtk::manage(new Gtk::Label(caption));
        widget_wrapper->get_outer()->set_data("TabViewLabel", label);
        page_index_after_insert = cb->_nb->append_page(*widget_wrapper->get_outer(), *label);
        label->show();
        widget_wrapper->get_outer()->show();
      }
    }
    
    return page_index_after_insert;
  }

  static void remove_page(::mforms::TabView *self, ::mforms::View *page)
  {
    TabViewImpl* cb= self->get_data<TabViewImpl>();

    if ( cb)
    {
      ViewImpl *widget_wrapper = page->get_data<ViewImpl>();
      if ( widget_wrapper )
      {
        cb->_nb->remove_page(*widget_wrapper->get_outer());
      }
    }
  }

  static void set_tab_title(::mforms::TabView* self,int tab,const std::string& title)
  {
    TabViewImpl* cb= self->get_data<TabViewImpl>();

    if (cb)
    {
      Gtk::Widget *page= cb->_nb->get_nth_page(tab);
      if (page)
      {
        Gtk::Label *label = reinterpret_cast<Gtk::Label*>(page->get_data("TabViewLabel"));
        if (label)
        {
          label->set_text(title);
        }
      }
    }
  }

 public:
  static void init()
  {
    ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

    f->_tabview_impl.create         = &TabViewImpl::create;
    f->_tabview_impl.get_active_tab = &TabViewImpl::get_active_tab;
    f->_tabview_impl.set_active_tab = &TabViewImpl::set_active_tab;
    f->_tabview_impl.add_page       = &TabViewImpl::add_page;
    f->_tabview_impl.remove_page    = &TabViewImpl::remove_page;
    f->_tabview_impl.set_tab_title  = &TabViewImpl::set_tab_title;
  }
};

};
};

#endif
