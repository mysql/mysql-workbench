//#include "wb_config.h"
#ifdef COMMERCIAL_CODE

#ifndef __VALIDATION_PANEL_H__
#define __VALIDATION_PANEL_H__

#include <gtkmm/box.h>
#include <gtkmm/treeview.h>
#include <gtkmm/label.h>
#include <gtkmm/image.h>
#include <glibmm/refptr.h>
#include "validation_manager.h"
#include "listmodel_wrapper.h"
#include "base/trackable.h"

namespace Gtk
{
class TreeView;
class HBox;
}

class ValidationPanel : public Gtk::VBox, public base::trackable
{
  public:
    ValidationPanel();

    void refresh(const bec::NodeId &node, int ocount);
    Gtk::HBox* notebook_label(Gtk::HBox* lbl = 0);
    
  private:
    void size_request_slot(Gtk::Allocation& req);
    
    bec::ValidationMessagesBE        _be;
    Gtk::TreeView                    _tv;
    Glib::RefPtr<ListModelWrapper>   _model;
    Gtk::HBox                       *_label_box;
    Gtk::Image                       _icon;
    Gtk::Label                       _label;
};

#endif
#endif
