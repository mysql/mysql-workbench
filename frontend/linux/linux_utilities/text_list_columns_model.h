#ifndef __TEXT_LIST_COLUMNS_MODEL_H__
#define __TEXT_LIST_COLUMNS_MODEL_H__

//!
//! \addtogroup linuxutils Linux utils
//! @{
//!

#include <gtkmm/treemodelcolumn.h>

class TextListColumnsModel : public Gtk::TreeModelColumnRecord {
public:
  TextListColumnsModel() {
    add(item);
  }
  Gtk::TreeModelColumn<std::string> item;
};

//!
//! @}
//!

#endif
