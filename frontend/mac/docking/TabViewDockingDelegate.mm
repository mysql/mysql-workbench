/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#import "TabViewDockingDelegate.h"
#import "NSString_extras.h"

#include "mforms/appview.h"

TabViewDockingPointDelegate::TabViewDockingPointDelegate(NSTabView *tabView, const std::string &type)
  : _tabView(tabView), _type(type) {
}

mforms::AppView *TabViewDockingPointDelegate::appview_for_view(NSView *view) {
  if (_views.find(view) != _views.end())
    return _views[view];
  return NULL;
}

bool TabViewDockingPointDelegate::close_all() {
  for (NSTabViewItem *item in _tabView.tabViewItems) {
    mforms::AppView *view = appview_for_view(item.view);
    if (view != NULL && !view->on_close())
      return false;
  }

  for (NSTabViewItem *item in _tabView.tabViewItems) {
    mforms::AppView *view = appview_for_view(item.view);
    if (view != NULL)
      view->close();
  }
  return true;
}

void TabViewDockingPointDelegate::set_name(const std::string &name) {
  [_tabView setAccessibilityTitle: [NSString stringWithUTF8String:name.c_str()]];
}

void TabViewDockingPointDelegate::dock_view(mforms::AppView *view, const std::string &arg1, int arg2) {
  id v = view->get_data();
  NSTabViewItem *tabItem = [[NSTabViewItem alloc] initWithIdentifier: [NSString stringWithFormat: @"appview:%p", view]];
  tabItem.view = v;

  if (arg1 == "" || arg1 == "append")
    [_tabView addTabViewItem: tabItem];
  else if (arg1 == "prepend")
    [_tabView insertTabViewItem:tabItem atIndex: 0];

  if (view->release_on_add())
    view->set_release_on_add(false);
  else
    view->retain();
  _views[v] = view;

  if ([_tabView.delegate respondsToSelector: @selector(tabView:didSelectTabViewItem:)])
    [_tabView.delegate tabView:_tabView didSelectTabViewItem: tabItem];
}

bool TabViewDockingPointDelegate::select_view(mforms::AppView *view) {
  NSInteger i = [_tabView indexOfTabViewItemWithIdentifier: [NSString stringWithFormat: @"appview:%p", view]];
  if (i >= 0) {
    NSTabViewItem *item = [_tabView tabViewItemAtIndex: i];
    if (item) {
      [_tabView selectTabViewItem:item];
      return true;
    }
  }
  return false;
}

void TabViewDockingPointDelegate::undock_view(mforms::AppView *view) {
  NSInteger i = [_tabView indexOfTabViewItemWithIdentifier: [NSString stringWithFormat: @"appview:%p", view]];
  if (i != NSNotFound) {
    NSTabViewItem *item = [_tabView tabViewItemAtIndex: i];
    if (item) {
      [_tabView removeTabViewItem:item];
      _views.erase(view->get_data());
      view->release();
    }
  }
}

void TabViewDockingPointDelegate::set_view_title(mforms::AppView *view, const std::string &title) {
  NSInteger i = [_tabView indexOfTabViewItemWithIdentifier: [NSString stringWithFormat: @"appview:%p", view]];
  if (i >= 0) {
    NSTabViewItem *item = [_tabView tabViewItemAtIndex: i];
    if (item) {
      item.label = [NSString stringWithCPPString: title];
      [_tabView.superview setNeedsDisplay:YES];
    }
  }
}

std::pair<int, int> TabViewDockingPointDelegate::get_size() {
  NSRect frame = _tabView.contentRect;
  return std::make_pair(NSWidth(frame), NSHeight(frame));
}

mforms::AppView *TabViewDockingPointDelegate::selected_view() {
  id view = _tabView.selectedTabViewItem.view;
  if (view) {
    if (_views.find(view) != _views.end())
      return _views[view];
  }
  return NULL;
}

int TabViewDockingPointDelegate::view_count() {
  return (int)_tabView.numberOfTabViewItems;
}

mforms::AppView *TabViewDockingPointDelegate::view_at_index(int index) {
  id view = [_tabView tabViewItemAtIndex:index].view;

  if (_views.find(view) != _views.end())
    return _views[view];
  return NULL;
}
