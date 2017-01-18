
#include "sqlide_form.h"
#include "workbench/wb_context_ui.h"
#include "workbench/wb_context.h"
#include "linux_utilities/plugin_editor_base.h"
#include "sqlide_main.h"

class ToolbarManager;

static void close_plugin(PluginEditorBase *editor, wb::WBContext *wb) {
  wb->close_gui_plugin(dynamic_cast<GUIPluginBase *>(editor));
}

static FormViewBase *create_db_sql_editor_view(std::shared_ptr<bec::UIForm> form, wb::WBContext *wb) {
  SqlEditorForm::Ref editor_be = SqlEditorForm::Ref(std::dynamic_pointer_cast<SqlEditorForm>(form));

  DbSqlEditorView *view = Gtk::manage(DbSqlEditorView::create(editor_be));

  view->set_close_editor_callback(sigc::bind(sigc::ptr_fun(close_plugin), wb));

  view->init();
  return view;
}

void setup_sqlide(std::string &name, sigc::slot<FormViewBase *, std::shared_ptr<bec::UIForm> > &create_function) {
  name = WB_MAIN_VIEW_DB_QUERY;

  create_function = sigc::bind(sigc::ptr_fun(create_db_sql_editor_view), wb::WBContextUI::get()->get_wb());
}
