/*
 * Copyright (c) 2010, 2017, Oracle and/or its affiliates. All rights reserved.
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

WebBrowser::WebBrowser() {
  _webbrowser_impl = &ControlFactory::get_instance()->_webbrowser_impl;

  _webbrowser_impl->create(this);
}

//--------------------------------------------------------------------------------------------------

void WebBrowser::set_html(const std::string& code) {
  _webbrowser_impl->set_html(this, code);
}

//--------------------------------------------------------------------------------------------------

void WebBrowser::navigate(const std::string& url) {
  _webbrowser_impl->navigate(this, url);
}

//--------------------------------------------------------------------------------------------------
void WebBrowser::set_link_click_handler(const std::function<bool(const std::string&)>& slot) {
  _handle_link_click = slot;
}

//--------------------------------------------------------------------------------------------------

bool WebBrowser::on_link_clicked(const std::string& uri) {
  if (_handle_link_click)
    return _handle_link_click(uri);
  return false;
}

//--------------------------------------------------------------------------------------------------

std::string WebBrowser::get_document_title() {
  return _webbrowser_impl->get_document_title(this);
}

//--------------------------------------------------------------------------------------------------

void WebBrowser::document_loaded(const std::string& actualUrl) {
  _document_loaded(actualUrl);
}

//--------------------------------------------------------------------------------------------------
