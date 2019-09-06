/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_mforms.h"

#include "mforms/hypertext.h"

#include "../lf_view.h"
#include <glib/gprintf.h>
#include <glib-object.h>
#include <glib.h>
#include "base/string_utilities.h"
#include <stdio.h>

namespace mforms {

  namespace gtk {

    //==============================================================================
    //
    //==============================================================================
    class HyperTextImpl : public ViewImpl {
    public:
      HyperTextImpl(HyperText *self);

      static bool create(HyperText *ht);
      static void set_markup_text(HyperText *ht, const std::string &text);
      static void set_background_color(HyperText *ht, const std::string &color);

      static void init();

    protected:
      virtual Gtk::Widget *get_outer() const {
        return &_win;
      }

    private:
      mutable Gtk::ScrolledWindow _win;
      Gtk::TextView _text;
      gulong _conn;
    };

    //------------------------------------------------------------------------------
    HyperTextImpl::HyperTextImpl(HyperText *self) : ViewImpl(self), _conn(0) {
      _win.add(_text);
      _win.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
      _text.show();
      _text.set_editable(false);
      _text.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    }

    //------------------------------------------------------------------------------
    bool HyperTextImpl::create(HyperText *ht) {
      return new HyperTextImpl(ht);
    }

    static std::string strip_html(const std::string &input) {
      std::string s(input);
      std::string ret;

      if (s.length()) {
        const size_t slen = s.length();
        bool copy = true;

        ret.reserve(slen);

        for (size_t i = 0; i < slen; ++i) {
          if (s[i] == '<') {
            copy = false;
            std::string::size_type p = s.find('>', i);
            if (p != std::string::npos) {
              std::string tag = s.substr(i, p - i + 1);
              if (tag == "<br>" || tag == "<br/>" || tag == "</div>") {
                ret.push_back('\n');
                i += tag.size() - 1;
              } else if (tag == "<tr>" || tag.find("<tr ") == 0) {
                ret.push_back('\n');
                i += tag.size() - 1;
              } else if (tag == "<td>" || tag.find("<td ") == 0) {
                ret.push_back('\t');
                i += tag.size() - 1;
                copy = true;
              }
            }
          } else if (s[i] == '>')
            copy = true;
          else if (s[i] == '&') {
            copy = false;
            std::string::size_type p = s.find(';', i);
            if (p != std::string::npos) {
              std::string tag = s.substr(i, p - i + 1);
              if (tag == "&lt;") {
                ret.push_back('<');
                i += tag.size() + 1;
              } else if (tag == "&gt;") {
                ret.push_back('>');
                i += tag.size() + 1;
              } else if (tag == "&amp;") {
                ret.push_back('&');
                i += tag.size() + 1;
              }
            }
          } else if (copy)
            ret.push_back(s[i]);
        }
      }
      return ret;
    }

    //------------------------------------------------------------------------------
    void HyperTextImpl::set_markup_text(HyperText *self, const std::string &text) {
      HyperTextImpl *impl = self->get_data<HyperTextImpl>();
      if (impl) {
        impl->_text.get_buffer()->set_text(strip_html(text));
      }
    }

    //------------------------------------------------------------------------------
    void HyperText_init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_hypertext_impl.create = &HyperTextImpl::create;
      f->_hypertext_impl.set_markup_text = &HyperTextImpl::set_markup_text;
    }

  } // gtk

} // mforms
