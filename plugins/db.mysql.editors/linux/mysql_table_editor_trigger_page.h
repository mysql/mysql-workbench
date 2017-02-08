#ifndef __WB_MYSQL_TABLE_EDITOR_TRIGGER_PAGE_H__
#define __WB_MYSQL_TABLE_EDITOR_TRIGGER_PAGE_H__

#include "grt/tree_model.h"
#include <gtkmm/builder.h>

class MySQLTableEditorBE;
class ListModelWrapper;
class DbMySQLTableEditor;

//==============================================================================
//
//==============================================================================
class DbMySQLTableEditorTriggerPage {
public:
  DbMySQLTableEditorTriggerPage(DbMySQLTableEditor* owner, MySQLTableEditorBE* be, Glib::RefPtr<Gtk::Builder> xml);
  ~DbMySQLTableEditorTriggerPage();

  void refresh();

  void switch_be(MySQLTableEditorBE* be);

private:
  MySQLTableEditorBE* _be;
  Glib::RefPtr<Gtk::Builder> _xml;
};

#endif
