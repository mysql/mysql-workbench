/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __AUTO_COMPLETABLE_H__
#define __AUTO_COMPLETABLE_H__

#include <string>
#include <glibmm/refptr.h>
#include "text_list_columns_model.h"

namespace Gtk {
  class Entry;
  class ListStore;
  class EntryCompletion;
}

//!
//! \addtogroup linuxutils Linux utils
//! @{
//!

//==============================================================================
//! AutoCompletion adds ability to have a history in the entry and to auto-complete
//! curretly typed text. Usage is simple: create AutoCompletable instance
//! and pass entry either to constructor or using method add_to_entry later.
//! Add each item which should appear in completion or list via add_completion_text
class AutoCompletable {
public:
  AutoCompletable(Gtk::Entry* entry = 0);
  void add_completion_text(const std::string& s);
  void add_to_entry(Gtk::Entry* entry);
  void set_popup_enabled(const bool enabled);
  void clear();

private:
  TextListColumnsModel _completion_columns;
  Glib::RefPtr<Gtk::ListStore> _completion_model;
  Glib::RefPtr<Gtk::EntryCompletion> _completion;
};

//!
//! @}
//!

#endif
