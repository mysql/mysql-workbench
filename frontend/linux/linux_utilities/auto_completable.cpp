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

#include "auto_completable.h"
#include <gtkmm/entry.h>
#include <gtkmm/liststore.h>
#include <gtkmm/entrycompletion.h>

//------------------------------------------------------------------------------
AutoCompletable::AutoCompletable(Gtk::Entry* entry)
  : _completion_model(Gtk::ListStore::create(_completion_columns)), _completion(Gtk::EntryCompletion::create()) {
  _completion->property_model() = _completion_model;
  _completion->set_text_column(0);
  _completion->set_inline_completion(true);
  if (entry)
    entry->set_completion(_completion);
}

//------------------------------------------------------------------------------
void AutoCompletable::add_completion_text(const std::string& s) {
  Gtk::TreeModel::iterator iter = _completion_model->append();
  Gtk::TreeModel::Row row = *iter;

  row[_completion_columns.item] = s;
}

//------------------------------------------------------------------------------
void AutoCompletable::add_to_entry(Gtk::Entry* entry) {
  entry->set_completion(_completion);
}

//------------------------------------------------------------------------------
void AutoCompletable::set_popup_enabled(const bool enabled) {
  _completion->set_popup_completion(enabled);
}

//------------------------------------------------------------------------------
void AutoCompletable::clear() {
  _completion_model->clear();
}
