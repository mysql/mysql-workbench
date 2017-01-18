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
