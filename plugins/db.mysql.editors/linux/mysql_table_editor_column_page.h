#ifndef __WB_COLUMN_PAGE_HANDLING_H__
#define __WB_COLUMN_PAGE_HANDLING_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>
#include <gtkmm/radiobutton.h>

class MySQLTableEditorBE;
class ListModelWrapper;
class DbMySQLTableEditor;
class AutoCompletable;

namespace Gtk {
  class ScrolledWindow;
  class ComboBox;
}

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorColumnPage : public sigc::trackable {
public:
  DbMySQLTableEditorColumnPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be, Glib::RefPtr<Gtk::Builder> xml);
  ~DbMySQLTableEditorColumnPage();

  void refresh();
  void partial_refresh(const int what);

  void switch_be(MySQLTableEditorBE* be);

private:
  grt::StringListRef get_types_for_table(const db_TableRef table); //!< T

  bool process_event(GdkEvent* event);
  void type_column_event(GdkEvent* e);
  void cursor_changed();

  void update_column_details(const ::bec::NodeId& node);

  void set_comment(const std::string& comment);
  void set_collation();
  void update_collation();
  void update_gc_storage_type();
  void set_gc_storage_type();

  void check_resize(Gtk::Allocation& r);
  bool do_on_visible(GdkEventVisibility*);

  bec::NodeId get_selected();

  void start_auto_edit();

private:
  void refill_completions();
  void refill_columns_tv();

  DbMySQLTableEditor* _owner;
  MySQLTableEditorBE* _be;
  Glib::RefPtr<Gtk::Builder> _xml;

  Glib::RefPtr<ListModelWrapper> _model;
  Gtk::TreeView* _tv;
  Gtk::ScrolledWindow* _tv_holder;

  Gtk::ComboBox* _collation_combo;

  Gtk::RadioButton* _radioStored;
  Gtk::RadioButton* _radioVirtual;

  gulong _edit_conn;
  GtkCellEditable* _ce;
  int _old_column_count;
  bool _auto_edit_pending;

  // Auto completion of types and related functions
  static std::shared_ptr<AutoCompletable> _types_completion;
  static std::shared_ptr<AutoCompletable> _names_completion;
  static std::shared_ptr<AutoCompletable> types_completion();
  static std::shared_ptr<AutoCompletable> names_completion();
  static void type_cell_editing_started(GtkCellRenderer* cr, GtkCellEditable* ce, gchar* path, gpointer udata);
  static void cell_editing_done(GtkCellEditable* ce, gpointer udata);
  bool _editing;
};

#endif
