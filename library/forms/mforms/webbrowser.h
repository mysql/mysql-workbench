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

#pragma once

#include <mforms/view.h>

namespace mforms {

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  class WebBrowser;

  struct MFORMS_EXPORT WebBrowserImplPtrs {
    bool (*create)(WebBrowser *);
    void (*set_html)(WebBrowser *, const std::string &);
    void (*navigate)(WebBrowser *, const std::string &);
    std::string (*get_document_title)(WebBrowser *);
  };
#endif
#endif

  /** HTML browser.

   This is implemented using the native Browser widget (IE in Windows and WebKit in Mac)
   Currently not supported in Linux */
  class MFORMS_EXPORT WebBrowser : public View {
  public:
    WebBrowser();

    /** Sets an HTML file to be displayed. */
    void set_html(const std::string &path);

    /** Opens the given URL in the browser */
    void navigate(const std::string &url);

    /** Gets the title of the displayed document. */
    std::string get_document_title();

    /** Sets a handle for link clicks */
    void set_link_click_handler(const std::function<bool(const std::string &)> &slot);

#ifndef SWIG
    /** Signal emitted when the document finishes loading

     In Python use add_loaded_callback()
     */
    boost::signals2::signal<void(const std::string &)> *signal_loaded() {
      return &_document_loaded;
    }
#ifndef DOXYGEN_SHOULD_SKIP_THIS
    virtual void document_loaded(const std::string &actualUrl);

    // called when a URL is clicked. A return value of true means the link was
    // handled and the URL should not be opened by the browser
    bool on_link_clicked(const std::string &uri);
#endif
#endif
  protected:
    WebBrowserImplPtrs *_webbrowser_impl;
    boost::signals2::signal<void(const std::string &)> _document_loaded;
    std::function<bool(const std::string &)> _handle_link_click;
  };
};
