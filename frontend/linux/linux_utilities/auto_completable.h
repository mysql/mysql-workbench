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
