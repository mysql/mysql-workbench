//!
//! \addtogroup linuxui Linux UI
//! @{
//!

#ifndef _MODEL_VIEW_PANEL_H_
#define _MODEL_VIEW_PANEL_H_

#include "linux_utilities/form_view_base.h"
#include <gtkmm/paned.h>
#include <gtkmm/box.h>
#include <gtkmm/notebook.h>
#include <gtkmm/builder.h>
#include "model/wb_model_diagram_form.h"
#include "gtk/mdc_gtk_canvas_view.h"
#include "gtk/mdc_gtk_canvas_scroller.h"
#include "mdc_canvas_view_x11.h"
#include "gtk/lf_view.h"

namespace Gtk {
  class Entry;
};

namespace wb {
  class WBContextUI;
};

class CatalogTree;
class PropertiesTree;
#ifdef COMMERCIAL_CODE
class ValidationPanel;
#endif
class DocumentationBox;
class NavigatorBox;
class InfoBox;

class ModelDiagramPanel : public Gtk::Paned, public FormViewBase {
  class InlineEditor : public wb::InlineEditContext {
    ModelDiagramPanel *_owner;
    Gtk::Entry *_edit_field;
    bool _editing;

    virtual void begin_editing(int x, int y, int width, int height, const std::string &text);
    virtual void end_editing();

    virtual void set_font_size(float size);
    virtual void set_multiline(bool flag);

  public:
    InlineEditor(ModelDiagramPanel *owner);
  };

  friend class InlineEditor;

  Gtk::Box _top_box;

  Gtk::Box *_tools_toolbar;

  Gtk::Box *_vbox;
  Gtk::Box *_diagram_hbox;

  wb::ModelDiagramForm *_be;
  mdc::GtkCanvasScroller _scroller;
  mdc::GtkCanvas *_canvas;
  Glib::RefPtr<Gdk::Cursor> _cursor;
  InlineEditor _inline_editor;
  Gtk::Paned *_editor_paned;
  Gtk::Paned *_sidebar;
  Gtk::Paned *_side_model_pane2;

  NavigatorBox *_navigator_box;
  mforms::TreeView *_catalog_tree;
  mforms::TreeView *_usertypes_list;
  mforms::TreeView *_history_list;
  DocumentationBox *_documentation_box;
  PropertiesTree *_properties_tree;
#ifdef COMMERCIAL_CODE
  ValidationPanel *_validation_panel;
#endif
  Glib::RefPtr<Gtk::Builder> _xml;

  bool drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);

  void drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                          const Gtk::SelectionData &selection_data, guint, guint time);

  bool drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);

  void view_realized();

  void post_construct();

  sigc::connection _sig_restore_sidebar;

public:
  static ModelDiagramPanel *create();

  ModelDiagramPanel(GtkPaned *paned, const Glib::RefPtr<Gtk::Builder> &xml);

  ~ModelDiagramPanel();

  void init(const std::string &view_id);
  virtual bec::UIForm *get_form() const {
    return _be;
  }
  virtual Gtk::Widget *get_panel() {
    return &_top_box;
  }
  virtual bool on_close();
  virtual void on_activate();

  wb::ModelDiagramForm *get_diagram_form() const {
    return _be;
  }

  mdc::CanvasView *get_canvas() const {
    return _canvas->get_canvas();
  }

  void update_tool_cursor();

  void setup_navigator();
  void refresh_catalog(bool hard);
  void refresh_zoom();

  void selection_changed();

  virtual void find_text(const std::string &text);

  virtual void reset_layout() {
    _editor_paned->set_position(_editor_paned->get_height() - 300);
  }
};

#endif /* _MODEL_VIEW_PANEL_H_ */

//!
//! @}
//!
