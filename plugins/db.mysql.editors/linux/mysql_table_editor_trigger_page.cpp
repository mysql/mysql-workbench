
#include "mysql_table_editor_fe.h"
#include "grtdb/db_object_helpers.h"
#include "mysql_table_editor_trigger_page.h"
#include "mforms/../gtk/lf_view.h"

//------------------------------------------------------------------------------
DbMySQLTableEditorTriggerPage::DbMySQLTableEditorTriggerPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be,
                                                             Glib::RefPtr<Gtk::Builder> xml)
  : _be(be), _xml(xml) {
  switch_be(be);
  // Gtk::Paned *paned(0);
  //_xml->get("trigger_paned", &paned);
}

//------------------------------------------------------------------------------
DbMySQLTableEditorTriggerPage::~DbMySQLTableEditorTriggerPage() {
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorTriggerPage::switch_be(MySQLTableEditorBE* be) {
  Gtk::Box* trigger_code_win;
  _xml->get_widget("trigger_code_holder", trigger_code_win);

  //  trigger_code_win->remove_all();

  _be = be;
  trigger_code_win->pack_start(*mforms::widget_for_view(be->get_trigger_panel()), true, true);
  trigger_code_win->show_all();
}

//------------------------------------------------------------------------------
void DbMySQLTableEditorTriggerPage::refresh() {
  if (_be)
    _be->load_trigger_sql();
}
