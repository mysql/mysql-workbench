
#ifndef _MODEL_PANEL_H_
#define _MODEL_PANEL_H_

#include <map>
#include <gtkmm/paned.h>
#include <gtkmm/notebook.h>
#include <gtkmm/box.h>
#include <gtkmm/frame.h>
#include <gtkmm/builder.h>
#include "linux_utilities/form_view_base.h"

#include "workbench/wb_overview.h"

class OverviewPanel;
class DocumentationBox;
#ifdef COMMERCIAL_CODE
class ValidationPanel;
#endif

namespace mforms {
  class TreeView;
};

class ModelPanel : public Gtk::Box, public FormViewBase {
public:
  static ModelPanel *create(wb::OverviewBE *overview);
  ~ModelPanel();

  virtual bool on_close();
  virtual void on_activate();

  virtual Gtk::Widget *get_panel() {
    return this;
  }
  virtual bec::UIForm *get_form() const;

  virtual void reset_layout() {
    _editor_paned->set_position(_editor_paned->get_height() - 300);
  }
  OverviewPanel *get_overview() {
    return _overview;
  }

  void selection_changed();

  virtual void find_text(const std::string &text);
  virtual void restore_sidebar_layout();

private:
  OverviewPanel *_overview;
  Gtk::Paned *_editor_paned;
  Gtk::Widget *_sidebar;
  Gtk::Frame *_secondary_sidebar;
  bec::NodeId _last_found_node;

  mforms::TreeView *_history_tree;
  mforms::TreeView *_usertypes_box;
  DocumentationBox *_documentation_box;
#ifdef COMMERCIAL_CODE
  ValidationPanel *_validation_panel;
#endif
  Glib::RefPtr<Gtk::Builder> _builder;
  bool _pending_rebuild_overview;

  friend class Gtk::Builder;
  ModelPanel(GtkBox *cobject, const Glib::RefPtr<Gtk::Builder> &xml);
  void post_construct(wb::OverviewBE *overview);

  void resize_overview();
  bool do_resize_overview();

  sigc::connection _sig_restore_layout;
  sigc::connection _sig_resize_overview;
};

#endif /* _MODEL_PANEL_H_ */
