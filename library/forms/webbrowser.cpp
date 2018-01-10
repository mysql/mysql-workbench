/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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
