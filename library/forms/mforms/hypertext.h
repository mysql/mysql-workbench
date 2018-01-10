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

#pragma once

#include "mforms/view.h"

namespace mforms {

  class HyperText;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct HyperTextImplPtrs {
    bool (*create)(HyperText *ht);
    void (*set_markup_text)(HyperText *ht, const std::string &text);
  };
#endif
#endif

  class MFORMS_EXPORT HyperText : public View {
    HyperTextImplPtrs *_hypertext_impl;

    // std::map<std::string, std::string> _tag_styles;

    boost::signals2::signal<void(const std::string &)> _url_click_signal;

  public:
    HyperText();

    void set_markup_text(const std::string &text);
    // background color is implemented in mforms::View.
    // void add_style(const std::string &tag, const std::string &span_equiv);

    virtual void set_padding(int left, int top, int right, int bottom);

#ifndef SWIG
    boost::signals2::signal<void(const std::string &)> *signal_link_click() {
      return &_url_click_signal;
    }
#endif
  public:
    void handle_url_click(const std::string &url);
  };
};
