#ifndef _FORM_VIEW_BASE_H_
#define _FORM_VIEW_BASE_H_

#include <gtkmm/notebook.h>
#include <gtkmm/widget.h>
#include <gtkmm/paned.h>
#include "base/ui_form.h"

namespace bec {
  class GRTManager;
};

namespace mforms {
  class ToolBar;
};

class PluginEditorBase;

class FormViewBase {
protected:
  sigc::signal<void, std::string> _title_changed;
  Gtk::Notebook *_editor_note;

  mforms::ToolBar *_toolbar;
  Gtk::Paned *_sidebar1_pane;
  Gtk::Paned *_sidebar2_pane;
  std::string _panel_savename;

  FormViewBase(const std::string &savename)
    : _editor_note(0), _toolbar(0), _sidebar1_pane(0), _sidebar2_pane(0), _panel_savename(savename) {
  }
  virtual ~FormViewBase(){};

public:
  sigc::signal<void, std::string> signal_title_changed() {
    return _title_changed;
  }

  std::string get_title() {
    return get_form()->get_title();
  }
  virtual Gtk::Widget *get_panel() = 0;

  virtual bec::UIForm *get_form() const = 0;

  virtual bool on_close() {
    return true;
  }
  virtual void on_activate() {
  }

  virtual void toggle_sidebar(bool show);
  virtual void toggle_secondary_sidebar(bool show);

  virtual void reset_layout() {
  }
  // close the selected tab and return true or false if no tab is active
  virtual bool close_focused_tab();

  virtual void find_text(const std::string &text) {
  }

  virtual void dispose() {
  }

  virtual bool perform_command(const std::string &cmd);

protected:
  sigc::slot<void, PluginEditorBase *> _close_editor;

  virtual void plugin_tab_added(PluginEditorBase *plugin){};

public:
  bool close_plugin_tab(PluginEditorBase *editor);

  void set_close_editor_callback(const sigc::slot<void, PluginEditorBase *> &handler);

  void add_plugin_tab(PluginEditorBase *plugin);
  void remove_plugin_tab(PluginEditorBase *plugin);
  bool close_editors_for_object(const std::string &id);

  PluginEditorBase *get_focused_plugin_tab();

  void sidebar_resized(bool primary);
  virtual void restore_sidebar_layout();
};

#endif
