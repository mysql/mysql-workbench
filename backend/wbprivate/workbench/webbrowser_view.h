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
