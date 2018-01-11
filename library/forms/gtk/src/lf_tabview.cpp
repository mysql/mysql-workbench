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

#include "../lf_tabview.h"
#include "../src/active_label.h"

class MyActiveLabel : public ActiveLabel {
  mforms::TabView *_owner;
  mforms::View *_page;

public:
  MyActiveLabel(mforms::TabView *owner, mforms::View *page, const std::string &title, const sigc::slot<void> &close_cb)
    : ActiveLabel(title, close_cb), _owner(owner), _page(page) {
    signal_button_press_event().connect(sigc::mem_fun(this, &MyActiveLabel::button_press_slot));
  }

  bool button_press_slot(GdkEventButton *evb) {
    if (evb->button == 3) {
      _owner->set_menu_tab(_owner->get_page_index(_page));
      if (_owner->get_tab_menu()) {
        _owner->get_tab_menu()->will_show();
        _owner->get_tab_menu()->popup_at(NULL, base::Point(evb->x, evb->y));
      }
    }
    return false;
  }
};

mforms::gtk::TabViewImpl::TabViewImpl(::mforms::TabView *self, ::mforms::TabViewType tabType)
  : ViewImpl(self), _reorderable(false) {
  _nb = new Gtk::Notebook();
  switch (tabType) {
    case mforms::TabViewSystemStandard:
      break;
    case mforms::TabViewTabless:
      _nb->set_show_tabs(false);
      _nb->set_show_border(false);
      break;
    case mforms::TabViewPalette:
    case mforms::TabViewEditorBottom:
      _nb->set_tab_pos(Gtk::POS_BOTTOM);
      break;
    default: // XXX
      break;
  }
  _nb->set_scrollable(true);
  _nb->signal_switch_page().connect(sigc::mem_fun(this, &TabViewImpl::tab_changed));
  _nb->signal_page_reordered().connect(sigc::mem_fun(this, &TabViewImpl::tab_reordered));
  _nb->show();
  setup();
}

mforms::gtk::TabViewImpl::~TabViewImpl() {
  delete _nb;
}

void mforms::gtk::TabViewImpl::tab_changed(Gtk::Widget *, guint) {
  TabView *tv = dynamic_cast<TabView *>(owner);
  if (tv && !tv->is_destroying())
    (*tv->signal_tab_changed())();
}

void mforms::gtk::TabViewImpl::tab_reordered(Gtk::Widget *page, guint to) {
  TabView *tv = dynamic_cast<TabView *>(owner);
  mforms::View *view = view_for_widget(page);
  if (!view)
    throw std::logic_error("view_for_widget returned NULL");

  if (tv)
    tv->reordered(view, to);
}

void mforms::gtk::TabViewImpl::close_tab_clicked(mforms::View *page) {
  TabView *tv = dynamic_cast<TabView *>(owner);
  int i = tv->get_page_index(page);
  page->retain();
  if (tv->can_close_tab(i)) {
    if (tv->get_page_index(page) >= 0)
      tv->remove_page(page);
  }
  page->release();
}

bool mforms::gtk::TabViewImpl::create(::mforms::TabView *self, mforms::TabViewType tabtype) {
  return new TabViewImpl(self, tabtype) != 0;
}

void mforms::gtk::TabViewImpl::set_active_tab(::mforms::TabView *self, int index) {
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  if (cb) {
    cb->_nb->set_current_page(index);
  }
}

int mforms::gtk::TabViewImpl::get_active_tab(::mforms::TabView *self) {
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  return cb ? cb->_nb->get_current_page() : -1;
}

int mforms::gtk::TabViewImpl::add_page(::mforms::TabView *self, ::mforms::View *page, const std::string &caption,
                                       bool hasCloseButton) {
  int page_index_after_insert = -1;
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  if (cb) {
    ViewImpl *widget_wrapper = page->get_data<ViewImpl>();
    if (widget_wrapper) {
      widget_wrapper->get_outer()->set_data("mforms::View", page);

      Gtk::Widget *label;
      switch (self->get_type()) {
        case mforms::TabViewMainClosable:
	/* fall-thru */
        case mforms::TabViewEditorBottom:
	/* fall-thru */
        case mforms::TabViewDocumentClosable:
          if (hasCloseButton) {
            label = Gtk::manage(new MyActiveLabel(
              self, page, caption, sigc::bind(sigc::mem_fun(cb, &TabViewImpl::close_tab_clicked), page)));
            break;
          }
        /* fall-thru */
        default:
          label = Gtk::manage(new Gtk::Label(caption));
	  break;
      }
      widget_wrapper->get_outer()->show();
      page_index_after_insert = cb->_nb->append_page(*widget_wrapper->get_outer(), *label);
      widget_wrapper->get_outer()->set_data("TabViewLabel", label);
      if (!hasCloseButton)
        label->get_style_context()->add_class("noClose");

      if (cb->_reorderable)
        cb->_nb->set_tab_reorderable(*widget_wrapper->get_outer(), true);
    }
  }

  return page_index_after_insert;
}

void mforms::gtk::TabViewImpl::remove_page(::mforms::TabView *self, ::mforms::View *page) {
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  if (cb) {
    ViewImpl *widget_wrapper = page->get_data<ViewImpl>();
    if (widget_wrapper) {
      cb->_nb->remove_page(*widget_wrapper->get_outer());
    }
  }
}

void mforms::gtk::TabViewImpl::set_tab_title(::mforms::TabView *self, int tab, const std::string &title) {
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  if (cb) {
    Gtk::Widget *page = cb->_nb->get_nth_page(tab);
    if (page) {
      Gtk::Widget *w = reinterpret_cast<Gtk::Widget *>(page->get_data("TabViewLabel"));
      if (Gtk::Label *label = dynamic_cast<Gtk::Label *>(w))
        label->set_text(title);
      else if (ActiveLabel *label = dynamic_cast<ActiveLabel *>(w))
        label->set_text(title);
    }
  }
}

void mforms::gtk::TabViewImpl::set_aux_view(mforms::TabView *self, mforms::View *view) {
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  if (cb) {
    Gtk::Widget *widget = view->get_data<ViewImpl>()->get_outer();
#if GTK_CHECK_VERSION(2, 20, 0)
    // gtkmm2 shipped in el6 doesn't support this
    //      cb->_nb->set_action_widget(widget, Gtk::PACK_END);
    gtk_notebook_set_action_widget(cb->_nb->gobj(), widget->gobj(), GTK_PACK_END);
#else
#error "Gtk+ version 2.20 or newer is required"
#endif
  }
}

void mforms::gtk::TabViewImpl::set_allows_reordering(mforms::TabView *self, bool flag) {
  TabViewImpl *cb = self->get_data<TabViewImpl>();

  if (cb) {
    cb->_reorderable = flag;
    for (int c = cb->_nb->get_n_pages(), i = 0; i < c; i++) {
      cb->_nb->set_tab_reorderable(*cb->_nb->get_nth_page(i), flag);
    }
  }
}
void mforms::gtk::TabViewImpl::init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_tabview_impl.create = &TabViewImpl::create;
  f->_tabview_impl.get_active_tab = &TabViewImpl::get_active_tab;
  f->_tabview_impl.set_active_tab = &TabViewImpl::set_active_tab;
  f->_tabview_impl.add_page = &TabViewImpl::add_page;
  f->_tabview_impl.remove_page = &TabViewImpl::remove_page;
  f->_tabview_impl.set_tab_title = &TabViewImpl::set_tab_title;
  f->_tabview_impl.set_aux_view = &TabViewImpl::set_aux_view;
  f->_tabview_impl.set_allows_reordering = &TabViewImpl::set_allows_reordering;
}
