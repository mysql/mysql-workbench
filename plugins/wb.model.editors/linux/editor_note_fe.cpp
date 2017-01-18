#include "linux_utilities/plugin_editor_base.h"
#include "../backend/wb_editor_note.h"
#include <gtkmm/image.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/box.h>
#include <gtkmm/textview.h>
#include <gtkmm/builder.h>

class NoteEditor : public PluginEditorBase {
  NoteEditorBE _be;
  Glib::RefPtr<Gtk::Builder> _xml;

  virtual bec::BaseEditor *get_be() {
    return &_be;
  }

  void set_name(const std::string &name) {
    _be.set_name(name);
    _signal_title_changed.emit(_be.get_title());
  }

public:
  NoteEditor(grt::Module *m, const grt::BaseListRef &args)
    : PluginEditorBase(m, args), _be(workbench_model_NoteFigureRef::cast_from(args[0])) {
    set_border_width(8);

    _xml = Gtk::Builder::create_from_file(bec::GRTManager::get()->get_data_file_path("modules/data/editor_note.glade"));

    Gtk::Widget *widget;
    _xml->get_widget("base_table", widget);

    Gtk::Image *image;
    _xml->get_widget("image", image);

    // image->set(grtm->get_data_file_path(""));

    Gtk::Entry *entry;
    _xml->get_widget("name_entry", entry);

    add_entry_change_timer(entry, sigc::mem_fun(this, &NoteEditor::set_name));

    Gtk::TextView *tview;
    _xml->get_widget("text_view", tview);

    add_text_change_timer(tview, sigc::mem_fun(_be, &NoteEditorBE::set_text));

    widget->reparent(*this);

    show_all();

    refresh_form_data();
  }

  virtual void do_refresh_form_data() {
    Gtk::Entry *entry;
    _xml->get_widget("name_entry", entry);

    Gtk::TextView *tview;
    _xml->get_widget("text_view", tview);

    entry->set_text(_be.get_name());

    tview->get_buffer()->set_text(_be.get_text());
  }
};

extern "C" {
GUIPluginBase *createNoteEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new NoteEditor(m, args));
}
};
