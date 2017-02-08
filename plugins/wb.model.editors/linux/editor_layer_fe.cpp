#include "linux_utilities/plugin_editor_base.h"
#include "../backend/wb_editor_layer.h"
#include "gtk_helpers.h"
#include <gtkmm/entry.h>
#include <gtkmm/button.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/entry.h>
#include <gtkmm/table.h>
#include <gtkmm/builder.h>

class LayerEditor : public PluginEditorBase {
  LayerEditorBE *_be;

  virtual bec::BaseEditor *get_be() {
    return _be;
  }

public:
  LayerEditor(grt::Module *m, const grt::BaseListRef &args)
    : PluginEditorBase(m, args, "modules/data/editor_layer.glade"), _be(0) {
    switch_edited_object(args);
    set_border_width(8);

    Gtk::Table *table(0);
    xml()->get_widget("table1", table);
    table->reparent(*this);

    show_all();

    refresh_form_data();
  }

  virtual ~LayerEditor() {
    delete _be;
  }

  virtual void do_refresh_form_data() {
    Gtk::Entry *entry(0);
    xml()->get_widget("layer_name", entry);
    entry->set_text(_be->get_name());

    xml()->get_widget("layer_color", entry);
    entry->set_text(_be->get_color());

    Gtk::Button *button(0);
    xml()->get_widget("layer_color_btn", button);
    if (button) {
      Gtk::ColorButton *cbutton = static_cast<Gtk::ColorButton *>(button);
      cbutton->set_color(Gdk::Color(_be->get_color()));
      cbutton->signal_color_set().connect(sigc::mem_fun(this, &LayerEditor::color_set));
    }
  }

  virtual bool switch_edited_object(const grt::BaseListRef &args) {
    LayerEditorBE *old_be = _be;
    _be = new LayerEditorBE(workbench_physical_LayerRef::cast_from(args[0]));
    delete old_be;

    _be->set_refresh_ui_slot(std::bind(&LayerEditor::refresh_form_data, this));

    bind_entry_and_be_setter("layer_name", this, &LayerEditor::set_name);

    do_refresh_form_data();

    return true;
  }

  void set_name(const std::string &name) {
    _be->set_name(name);
    _signal_title_changed.emit(_be->get_title());
  }

private:
  void color_set() {
    Gtk::Button *button(0);
    xml()->get_widget("layer_color_btn", button);

    if (button) {
      const Gtk::ColorButton *cbutton = static_cast<Gtk::ColorButton *>(button);
      const Gdk::Color color(cbutton->get_color());
      char buffer[32];
      snprintf(buffer, sizeof(buffer) - 1, "#%02x%02x%02x", color.get_red() >> 8, color.get_green() >> 8,
               color.get_blue() >> 8);
      buffer[sizeof(buffer) - 1] = 0;

      _be->set_color(buffer);
    }
  }
};

extern "C" {
GUIPluginBase *createPhysicalLayerEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new LayerEditor(m, args));
}
};
