//!
//! \addtogroup linuxui Linux UI
//! @{
//! 

#ifndef _DIAGRAM_SIZE_FORM_H_
#define _DIAGRAM_SIZE_FORM_H_

#include <gtkmm/dialog.h>
#include <gtkmm/builder.h>
#include "base/trackable.h"

namespace wb
{
  class WBContextUI;
  class DiagramOptionsBE;
};

namespace mdc
{
  class GtkCanvas;
};


class DiagramSizeForm : public Gtk::Dialog, public base::trackable
{
  Glib::RefPtr<Gtk::Builder> _xml;
  mdc::GtkCanvas *_canvas;
  wb::DiagramOptionsBE *_be;

  void realize_be(wb::WBContextUI *wbui);
  void init(wb::WBContextUI *wb);
  void spin_changed();
  void changed();
  void ok_clicked();
public:
  DiagramSizeForm(GtkDialog *gobj, Glib::RefPtr<Gtk::Builder> xml);
  virtual ~DiagramSizeForm();

  static DiagramSizeForm *create(wb::WBContextUI *wb);
};


#endif /* _DIAGRAM_SIZE_FORM_H_ */

//!                                                                                                                                     
//! @}                                                                                                                                  
//!
