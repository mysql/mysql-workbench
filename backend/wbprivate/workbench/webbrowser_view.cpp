/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "webbrowser_view.h"

#include "grt.h"

#include "grts/structs.h"
#include "grts/structs.app.h"

#include "grt/editor_base.h"

#include "wb_context_ui.h"

#define X_WB_EXTRA_URI "x-wbextra"

using namespace wb;
using namespace mforms;

//----------------- WebBrowserView -----------------------------------------------------------------

WebBrowserView::WebBrowserView(WBContextUI* wbui) : AppView(false, "Browse", true), _wbui(wbui) {
  add(&_browser, true, true);
  _browser.set_link_click_handler(std::bind(&WebBrowserView::handle_url, this, std::placeholders::_1));
  UIForm::scoped_connect(_browser.signal_loaded(),
                         std::bind(&WebBrowserView::document_loaded, this, std::placeholders::_1));
}

//--------------------------------------------------------------------------------------------------

WebBrowserView::~WebBrowserView() {
}

//--------------------------------------------------------------------------------------------------

bool WebBrowserView::handle_url(const std::string& url) {
  if (g_str_has_prefix(url.c_str(), X_WB_EXTRA_URI)) {
    _wbui->start_plugin_net_install(url);

    return true;
  }

  // if (g_str_has_suffix(url.c_str(), ".mwbpluginz"))
  {
    // return true;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

void WebBrowserView::set_html(const std::string& code) {
  _browser.set_html(code);
}

//--------------------------------------------------------------------------------------------------

void WebBrowserView::navigate(const std::string& url) {
  _browser.navigate(url);
}

//--------------------------------------------------------------------------------------------------

bool WebBrowserView::on_close() {
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called by the front end wrapper when the web browser finished loading a page.
 * Since the user can navigate on its own in the browser, this will also allow us to track the current URL.
 */
void WebBrowserView::document_loaded(const std::string& actualUrl) {
  _current_url = actualUrl;
  set_title(_browser.get_document_title());
};

//--------------------------------------------------------------------------------------------------

std::string WebBrowserView::current_url() {
  return _current_url;
}

//--------------------------------------------------------------------------------------------------
