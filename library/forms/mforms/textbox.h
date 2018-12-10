/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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
#include <mforms/utilities.h>

namespace mforms {
  enum ScrollBars {
    NoScrollBar = 0,                                            //!< Don't show any scroll bar.
    HorizontalScrollBar = (1 << 0),                             //!< Show only the horizontal scroll bar.
    VerticalScrollBar = (1 << 1),                               //!< Show only the vertical scroll bar.
    BothScrollBars = (HorizontalScrollBar | VerticalScrollBar), //!< Show both scroll bars.
    SmallScrollBars = (1 << 2)                                  //!< A smaller version of the scrollbars.
  };

  class TextBox;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct TextBoxImplPtrs {
    bool (*create)(TextBox *self, ScrollBars scroll_bars);
    void (*set_bordered)(TextBox *self, bool flag);
    void (*set_read_only)(TextBox *self, bool flag);
    void (*set_text)(TextBox *self, const std::string &text);
    std::string (*get_text)(TextBox *self);
    void (*append_text)(TextBox *self, const std::string &text, bool scroll_to_end);
    void (*set_padding)(TextBox *self, int pad); // TODO: not supported on Windows, should be removed.
    void (*set_monospaced)(TextBox *self, bool flag);
    void (*get_selected_range)(TextBox *self, int &start, int &end);
    void (*clear)(TextBox *self);
  };
#endif
#endif

  /** Multiline text editing control */
  class MFORMS_EXPORT TextBox : public View {
#ifdef SWIG
// starting with SWIG v3.0.3, this is no longer necessary (https://github.com/swig/swig/pull/201)
// workaround for problem with unicode strings and default args in swig
#if SWIG_VERSION < 0x030003 || SWIG_VERSION >= 0x030008
    %rename(append_text_and_scroll) append_text(const std::string &text, bool scroll_to_end);
    %rename(append_text) append_text(const std::string &text, bool scroll_to_end);
#endif
#endif
  public:
    TextBox(ScrollBars scroll_bars);

    /** Sets whether to draw a border around the text box */
    void set_bordered(bool flag);
    /** Sets whether text should be displayed in a monospaced font */
    void set_monospaced(bool flag);

    /** Sets whether the content of the text box is editable by the user */
    void set_read_only(bool flag);
    /** Sets the text contents of the control */
    void set_value(const std::string &text);
    /** Gets the text contents of the control */
    virtual std::string get_string_value();

    /** Gets range of selected text */
    void get_selected_range(int &start, int &end);

    /** Appends text to the end of the text box.

     If scroll_to_end is true, it will also scroll to make the end of the text box visible. */
    void append_text(const std::string &text, bool scroll_to_end = false);

    /** Appends text to the end of the text box, in the given character set encoding.

     If scroll_to_end is true, it will also scroll to make the end of the text box visible. */
    void append_text_with_encoding(const std::string &text, const std::string &encoding, bool scroll_to_end);

    /** Sets padding */
    void set_padding(int pad);

    /** Clear contents */
    void clear();

#ifndef SWIG
    /** Signal emitted when control's text changes (e.g. by typing, pasting etc.). */
    boost::signals2::signal<void()> *signal_changed() {
      return &_signal_changed;
    };

    /** Signal emitted when a key was pressed. The key code determines if it was a normal character or
     *  a special key. If a normal character was entered then the modifier keys have already been taken
     *  into account (e.g. for upper/lower casing etc.). The given text is (as always) utf-8 encoded.
     */
    boost::signals2::signal<bool(KeyCode, ModifierKey, const std::string &)> *key_event_signal() {
      return &_key_event_signal;
    };
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  public:
    void callback();
    bool key_event(KeyCode code, ModifierKey modifiers, const std::string &text);
#endif
#endif
  protected:
    TextBoxImplPtrs *_textbox_impl;
    bool _updating;

    boost::signals2::signal<void()> _signal_changed;
    boost::signals2::signal<bool(KeyCode, ModifierKey, const std::string &)> _key_event_signal;
  };
};
