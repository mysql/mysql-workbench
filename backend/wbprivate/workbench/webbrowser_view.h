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

#ifndef _WB_WEBBROWSER_H_
#define _WB_WEBBROWSER_H_

#include "wb_backend_public_interface.h"

#include "mforms/appview.h"
#include "mforms/webbrowser.h"

namespace wb {
  class WBContextUI;
  /**
   * This class implements the web browser interface in MySQL Workbench.
   */
  class MYSQLWBBACKEND_PUBLIC_FUNC WebBrowserView : public mforms::AppView {
  private:
    WBContextUI* _wbui;
    mforms::WebBrowser _browser;
    void document_loaded(const std::string& actualUrl);
    bool handle_url(const std::string& url);
    std::string _current_url;

  public:
    WebBrowserView(WBContextUI* wbui);
    ~WebBrowserView();

    void set_html(const std::string& code);
    void navigate(const std::string& code);
    virtual bool on_close();
    std::string current_url();
  };
}

#endif // #ifndef _WB_WEBBROWSER_H_
