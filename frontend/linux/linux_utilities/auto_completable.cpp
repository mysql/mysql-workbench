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
