//!
//! \addtogroup linuxui Linux UI
//! @{
//! 

#ifndef _DOCUMENTATION_BOX_H_
#define _DOCUMENTATION_BOX_H_

#include <gtkmm/box.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>

#include "workbench/wb_context_ui.h"


class DocumentationBox : public Gtk::VBox
{
  wb::WBContextUI *_wbui;
  Gtk::ComboBoxText _combo;
  Gtk::TextView _text;
  sigc::connection _timer;
  bec::UIForm *_selected_form;
  grt::ListRef<GrtObject> _object_list;
  bool _multiple_items;
  bool _initializing;

  void text_key_press(GdkEventKey *event);
  void text_button_press(GdkEventButton *event);
  void combo_changed();
  void text_changed();
  void commit();
  
public:
  DocumentationBox(wb::WBContextUI *wbui);
  ~DocumentationBox();
  
  void update_for_form(bec::UIForm *form);
};

#endif /* _DOCUMENTATION_BOX_H_ */

//!                                                                                                                                     
//! @}                                                                                                                                  
//!
