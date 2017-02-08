/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/mforms.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

TabView::TabView(TabViewType tabType) : _type(tabType), _aux_view(NULL), _menu_tab(0), _tab_menu(NULL) {
  _tabview_impl = &ControlFactory::get_instance()->_tabview_impl;

  _tabview_impl->create(this, tabType);
}

//--------------------------------------------------------------------------------------------------

TabView::~TabView() {
  if (_aux_view)
    _aux_view->release();
  _aux_view = NULL;
}

//--------------------------------------------------------------------------------------------------

void TabView::set_active_tab(int index) {
  _tabview_impl->set_active_tab(this, index);
}

//--------------------------------------------------------------------------------------------------

int TabView::get_active_tab() {
  return _tabview_impl->get_active_tab(this);
}

//--------------------------------------------------------------------------------------------------

int TabView::add_page(View *page, const std::string &caption, bool hasCloseButton) {
  cache_view(page);
  return _tabview_impl->add_page(this, page, caption, hasCloseButton);
}

//--------------------------------------------------------------------------------------------------

void TabView::remove_page(View *page) {
  page->retain();
  int i = get_page_index(page);
  _tabview_impl->remove_page(this, page);
  remove_from_cache(page);
  _signal_tab_closed(page, i);
  page->release();
}

//--------------------------------------------------------------------------------------------------

void TabView::reordered(View *view, int index) {
  int old_index = get_subview_index(view);
  reorder_cache(view, index);
  _signal_tab_reordered(view, old_index, index);
}

//--------------------------------------------------------------------------------------------------

void TabView::pin_changed(int tab, bool pinned) {
  _signal_tab_pin_changed(tab, pinned);
}

//--------------------------------------------------------------------------------------------------

int TabView::page_count() {
  return get_subview_count();
}

//--------------------------------------------------------------------------------------------------

int TabView::get_page_index(View *page) {
  return get_subview_index(page);
}

//--------------------------------------------------------------------------------------------------

View *TabView::get_page(int index) {
  return get_subview_at_index(index);
}

//--------------------------------------------------------------------------------------------------

void TabView::set_tab_title(int page, const std::string &caption) {
  _tabview_impl->set_tab_title(this, page, caption);
}

//--------------------------------------------------------------------------------------------------

bool TabView::can_close_tab(int index) {
  if (!_signal_tab_closing.empty())
    return *_signal_tab_closing(index);
  return true;
}

//--------------------------------------------------------------------------------------------------

void TabView::set_aux_view(View *view) {
  if (_aux_view)
    _aux_view->release();
  _aux_view = view;
  if (_aux_view)
    _aux_view->retain();
  _tabview_impl->set_aux_view(this, view);
}

//--------------------------------------------------------------------------------------------------

void TabView::set_allows_reordering(bool flag) {
  _tabview_impl->set_allows_reordering(this, flag);
}

//--------------------------------------------------------------------------------------------------

void TabView::set_tab_menu(ContextMenu *menu) {
  _tab_menu = menu;
}

//--------------------------------------------------------------------------------------------------

void TabView::set_menu_tab(int tab) {
  _menu_tab = tab;
}

//--------------------------------------------------------------------------------------------------

int TabView::get_menu_tab() {
  return _menu_tab;
}
