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

#pragma once

namespace MySQL {
  namespace Forms {

  public
    class WebBrowserWrapper : public ViewWrapper {
    protected:
      WebBrowserWrapper(mforms::View *backend);

      static bool create(mforms::WebBrowser *backend);
      static void set_html(mforms::WebBrowser *backend, const std::string &code);
      static void navigate(mforms::WebBrowser *backend, const std::string &url);
      static std::string get_document_title(mforms::WebBrowser *backend);

    public:
      static void init();
    };
  };
};
