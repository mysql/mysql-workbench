

#include "linux_utilities/plugin_editor_base.h"
#include "../backend/mysql_schema_editor.h"
#include "grtdb/db_object_helpers.h"
#include "linux_utilities/image_cache.h"
#include <gtkmm/image.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/textview.h>
#include <gtkmm/liststore.h>
#include <gtkmm/box.h>
//#include <gtk/gtkversion.h>

class SchemaEditor : public PluginEditorBase {
  MySQLSchemaEditorBE *_be;
  std::string _old_name;

  virtual bec::BaseEditor *get_be() {
    return _be;
  }

public:
  virtual ~SchemaEditor() {
    delete _be;
    _be = 0;
  }

  SchemaEditor(grt::Module *m, const grt::BaseListRef &args)
    : PluginEditorBase(m, args, "modules/data/editor_schema.glade"),
      _be(new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]))) {
    xml()->get_widget("mysql_schema_editor_notebook", _editor_notebook);

    Gtk::Image *image;
    xml()->get_widget("image", image);
    image->set(ImageCache::get_instance()->image_from_filename("db.Schema.editor.48x48.png", false));

    bind_entry_and_be_setter("name_entry", this, &SchemaEditor::set_name);
    if (_be->is_editing_live_object() && _be->get_schema()->oldName() != "") {
      Gtk::Entry *entry;
      xml()->get_widget("name_entry", entry);
      entry->set_sensitive(false);
    }

    Gtk::Button *btn;
    xml()->get_widget("refactor_btn", btn);
    btn->set_sensitive(_be->refactor_possible());
    btn->signal_clicked().connect(sigc::mem_fun(this, &SchemaEditor::refactor_schema));

    Gtk::ComboBox *combo;
    xml()->get_widget("collation_combo", combo);
    Glib::RefPtr<Gtk::ListStore> store(
      Glib::RefPtr<Gtk::ListStore>::cast_dynamic(xml()->get_object("collation_store")));
    setup_combo_for_string_list(combo);
    fill_combo_from_string_list(combo, _be->get_charset_collation_list());
    add_option_combo_change_handler(combo, "CHARACTER SET - COLLATE",
                                    sigc::mem_fun(this, &SchemaEditor::set_schema_option_by_name));

    Gtk::TextView *tview;
    xml()->get_widget("text_view", tview);
    add_text_change_timer(tview, sigc::mem_fun(this, &SchemaEditor::set_comment));

    //! widget->reparent(*this);
    add(*_editor_notebook);
    _editor_notebook->show();

    show_all();

    refresh_form_data();
  }

  void set_name(const std::string &name) {
    if (_be) {
      _be->set_name(name);
      Gtk::Button *btn;
      xml()->get_widget("refactor_btn", btn);
      btn->set_sensitive(_be->refactor_possible());
    }
  }

  void refactor_schema() {
    if (_be) {
      _be->refactor_catalog();
      Gtk::Button *btn;
      xml()->get_widget("refactor_btn", btn);
      btn->set_sensitive(_be->refactor_possible());
    }
  }

  void set_comment(const std::string &text) {
    if (_be)
      _be->set_comment(text);
  }

  void set_schema_option_by_name(const std::string &name, const std::string &value) {
    if (_be)
      _be->set_schema_option_by_name(name, value);
  }

  virtual void do_refresh_form_data() {
    Gtk::Entry *entry;
    xml()->get_widget("name_entry", entry);

    Gtk::TextView *tview;
    xml()->get_widget("text_view", tview);

    Gtk::ComboBox *combo;
    xml()->get_widget("collation_combo", combo);

    Gtk::Button *btn;
    xml()->get_widget("refactor_btn", btn);

    if (_be) {
      set_selected_combo_item(combo, _be->get_schema_option_by_name("CHARACTER SET - COLLATE"));

      _old_name = _be->get_name();
      entry->set_text(_old_name);

      tview->get_buffer()->set_text(_be->get_comment());

      bool is_editing_live_obj = is_editing_live_object();
      tview->set_sensitive(!is_editing_live_obj);
      Gtk::Label *tlabel;
      xml()->get_widget("label5", tlabel);
      tlabel->set_sensitive(!is_editing_live_obj);
      btn->set_sensitive(_be->refactor_possible());
    }
  }

  virtual bool switch_edited_object(const grt::BaseListRef &args);
};

//------------------------------------------------------------------------------
bool SchemaEditor::switch_edited_object(const grt::BaseListRef &args) {
  MySQLSchemaEditorBE *old_be = _be;
  _be = new MySQLSchemaEditorBE(db_mysql_SchemaRef::cast_from(args[0]));

  if (_be) {
    do_refresh_form_data();

    delete old_be;
    old_be = 0;
  } else
    _be = old_be;

  return true;
}

extern "C" {
GUIPluginBase *createDbMysqlSchemaEditor(grt::Module *m, const grt::BaseListRef &args) {
  return Gtk::manage(new SchemaEditor(m, args));
}
};
