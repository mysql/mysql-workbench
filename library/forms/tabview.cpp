/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "mforms/mforms.h"

using namespace mforms;

//--------------------------------------------------------------------------------------------------

TabView::TabView(TabViewType tabType)
{
  _tabview_impl= &ControlFactory::get_instance()->_tabview_impl;

  _tabview_impl->create(this, tabType);
}

//--------------------------------------------------------------------------------------------------

void TabView::set_active_tab(int index)
{
  _tabview_impl->set_active_tab(this, index);
}

//--------------------------------------------------------------------------------------------------

int TabView::get_active_tab()
{
  return _tabview_impl->get_active_tab(this);
}

//--------------------------------------------------------------------------------------------------

int TabView::add_page(View *page, const std::string& caption)
{
  cache_view(page);
  return _tabview_impl->add_page(this, page, caption);
}

//--------------------------------------------------------------------------------------------------

void TabView::remove_page(View *page)
{
  _tabview_impl->remove_page(this, page);
  remove_from_cache(page);
}

//--------------------------------------------------------------------------------------------------

int TabView::page_count()
{
  return get_subview_count();
}

//--------------------------------------------------------------------------------------------------

int TabView::get_page_index(View *page)
{
  return get_subview_index(page);
}

//--------------------------------------------------------------------------------------------------

View *TabView::get_page(int index)
{
  return get_subview_at_index(index);
}

//--------------------------------------------------------------------------------------------------

void TabView::set_tab_title(int page, const std::string& caption)
{
  _tabview_impl->set_tab_title(this, page, caption);
}

//--------------------------------------------------------------------------------------------------

bool TabView::can_close_tab(int index)
{
  if (!_signal_tab_closing.empty())
    return *_signal_tab_closing(index);
  return true;
}

//--------------------------------------------------------------------------------------------------
