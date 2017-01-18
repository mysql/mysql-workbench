/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/view.h>

namespace mforms {
  class TextEntry;

  enum TextEntryType {
    NormalEntry,   //!< a normal plain text entry
    PasswordEntry, //!< password entry, will hide typed characters
    SearchEntry,   //!< normal text entry, with a distinctive appearance for search
#ifdef __APPLE__
    SmallSearchEntry
#else
    SmallSearchEntry = SearchEntry
#endif
  };

  enum TextEntryAction {
    EntryActivate, //!< Enter key was pressed
    EntryEscape,   //!< Esc key was pressed
    EntryKeyUp,    //!< Up arrow key was pressed
    EntryKeyDown,  //!< Down arrow key was pressed
    EntryCKeyUp,   //!< Up arrow key was pressed with Control/Command held
    EntryCKeyDown  //!< Down arrow key was pressed with Control/Command held
  };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct TextEntryImplPtrs {
    bool (*create)(TextEntry *self, TextEntryType type);
    void (*set_text)(TextEntry *self, const std::string &text);
    void (*set_max_length)(TextEntry *self, int len);
    std::string (*get_text)(TextEntry *self);
    void (*set_read_only)(TextEntry *self, bool flag);
    void (*set_placeholder_text)(TextEntry *self, const std::string &text);
    void (*set_placeholder_color)(TextEntry *self, const std::string &color);
    void (*set_bordered)(TextEntry *self, bool flag);

    void (*cut)(TextEntry *self); // TODO Windows
    void (*copy)(TextEntry *self);
    void (*paste)(TextEntry *self);
    void (*select)(TextEntry *self, const base::Range &range);
    base::Range (*get_selection)(TextEntry *self);
  };
#endif
#endif

  /** Single line text edit control, with support for some special events. */
  class MFORMS_EXPORT TextEntry : public View {
  public:
    /** Constructor.
    */
    TextEntry(TextEntryType type = NormalEntry);

    /** Sets the max allowed length that the user can type */
    void set_max_length(int len);

    /** Sets the text in the entry */
    void set_value(const std::string &text);

    /** Gets the text in the entry */
    virtual std::string get_string_value();

    /** Sets whether contents of entry can be edited by the user */
    void set_read_only(bool flag);

    /** Sets the text to be shown as a placeholder, when there's no text in the entry */
    void set_placeholder_text(const std::string &text);

    /** Foreground color for the placeholder text. */
    void set_placeholder_color(const std::string &color);

    /** Sets whether to draw a border around the text box. Default is bordered. */
    void set_bordered(bool flag);

    void cut();
    void copy();
    void paste();
    void select(const base::Range &);
    base::Range get_selection();

#ifndef SWIG
    /** Signal emitted when the entry is edited

     In Python use add_changed_callback() */
    boost::signals2::signal<void()> *signal_changed() {
      return &_signal_changed;
    }

    /** Signal emitted when certain actions are performed:

     */
    boost::signals2::signal<void(TextEntryAction)> *signal_action() {
      return &_signal_action;
    }

  public:
    void callback();
    void action(TextEntryAction action);
#endif
  protected:
    TextEntryImplPtrs *_textentry_impl;

    boost::signals2::signal<void()> _signal_changed;
    boost::signals2::signal<void(TextEntryAction)> _signal_action;

    bool _updating;
  };
};
