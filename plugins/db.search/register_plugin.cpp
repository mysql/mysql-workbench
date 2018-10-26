/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

#include "mforms/menubar.h"
#include "grt.h"
#include "grtpp_notifications.h"

#include "grts/structs.db.mgmt.h"
#include "grts/structs.workbench.h"
#include "DbSearchFilterPanel.h"
#include "DbSearchPanel.h"
#include "interfaces/plugin.h"

#include "base/log.h"
#include "base/string_utilities.h"

#include "objimpl/wrapper/mforms_ObjectReference_impl.h"

#define MODULE_VERSION "2.0.0"

#include <sstream>
#include <boost/assign/list_of.hpp>
#include <boost/lambda/bind.hpp>

class DBSearchView : public mforms::AppView, public grt::GRTObserver {
private:
  db_query_EditorRef _editor;
  mforms::Label _label;
  mforms::Label _description;
  DBSearchFilterPanel _filter_panel;
  DBSearchPanel _search_panel;
  mforms::TimeoutHandle _check_selection_timeout;
  mforms::TimeoutHandle _search_timeout;

  grt::ListRef<db_query_LiveDBObject> _tree_selection;
  time_t _last_selection_change;

  bool check_selection() {
    if (time(NULL) - _last_selection_change > 0) {
      _check_selection_timeout = 0;

      _tree_selection = _editor->schemaTreeSelection();
      _filter_panel.search_button()->set_enabled(_tree_selection.count() > 0);
      _last_selection_change = 0;
      return false;
    }
    return true;
  }

  virtual void handle_grt_notification(const std::string &name, grt::ObjectRef sender, grt::DictRef info) {
    if (name == "GRNLiveDBObjectSelectionDidChange") {
      _tree_selection = grt::ListRef<db_query_LiveDBObject>();
      if (info.get_int("selection-size") == 0) {
        _filter_panel.search_button()->set_enabled(false);
      } else {
        if (_last_selection_change == 0 && _check_selection_timeout == 0)
          _check_selection_timeout = mforms::Utilities::add_timeout(1, std::bind(&DBSearchView::check_selection, this));
        _last_selection_change = time(NULL);
      }
    }
  }

  grt::StringListRef get_filters_from_schema_tree_selection() {
    grt::StringListRef filters(grt::Initialized);
    std::set<std::string> selected_parents;

    if (!_tree_selection.is_valid())
      return filters;

    // first make a list of all top level schemas and tables that have sub-nodes selected
    for (size_t c = _tree_selection.count(), i = 0; i < c; i++) {
      db_query_LiveDBObjectRef obj = _tree_selection[i];

      if (obj->type() == "db.Column") {
        selected_parents.insert(obj->schemaName());
        selected_parents.insert(*obj->schemaName() + "." + *obj->owner()->name());
      } else if (obj->type() == "db.Table" || obj->type() == "db.View") {
        selected_parents.insert(obj->schemaName());
      }
    }

    // then add all objects that don't have a more specific selection added
    // (eg. if sakila.actor and sakila are selected, only add sakila.actor)
    for (size_t c = _tree_selection.count(), i = 0; i < c; i++) {
      db_query_LiveDBObjectRef obj = _tree_selection[i];

      if (obj->type() == "db.Schema" && selected_parents.find(obj->name()) == selected_parents.end())
        filters.insert(*obj->name() + ".%.%");
      else if (obj->type() == "db.Table" || obj->type() == "db.View")
        filters.insert(*obj->schemaName() + "." + *obj->name() + ".%");
      else if (obj->type() == "db.Column")
        filters.insert(*obj->schemaName() + "." + *obj->owner()->name() + "." + *obj->name());
    }

    return filters;
  }

  bool search_activate_from_timeout() {
    _search_timeout = 0;
    start_search();
    return false;
  }

  void search_activate(mforms::TextEntryAction action) {
    // we need to call this from timeout or idle, cause in gtk, we're blocking the application
    if (action == mforms::EntryActivate && _search_timeout == 0)
      _search_timeout =
        mforms::Utilities::add_timeout(0.1f, std::bind(&DBSearchView::search_activate_from_timeout, this));
  }

  void finished_search() {
    _filter_panel.set_searching(false);
    _search_panel._search_finished = true;
    mforms::App::get()->set_status_text("Searching finished");
  }

  void failed_search() {
    // we need to allow user to start search again
    _filter_panel.set_searching(false);
    _search_panel._search_finished = true;
    mforms::App::get()->set_status_text("Searching failed");
  }

  void start_search() {
    if (_search_panel.stop_search_if_working())
      return;
    // build filter list in format schema.table.column from the selection
    grt::StringListRef filters = get_filters_from_schema_tree_selection();
    if (filters.count() == 0) {
      mforms::Utilities::show_message(
        "Search Data", "Please select the tables or schemas to be searched from the schema tree in the sidebar.", "OK",
        "", "");
      return;
    }
    std::string search_keyword = _filter_panel.get_search_text();
    int limit_table = _filter_panel.get_limit_table();
    int limit_total = _filter_panel.get_limit_total();
    int search_type = _filter_panel.get_search_type();
    bool invert = _filter_panel.exclude();
    sql::DriverManager *dm = sql::DriverManager::getDriverManager();
    mforms::App::get()->set_status_text("Opening new connection...");
    sql::ConnectionWrapper wrapper;
    try {
      wrapper = dm->getConnection(_editor->connection());
    } catch (grt::user_cancelled &ucancel) {
      mforms::App::get()->set_status_text(ucancel.what());
      return;
    }
    mforms::App::get()->set_status_text("Searching...");

    bec::GRTManager::get()->set_app_option("db.search:SearchType", grt::IntegerRef(search_type));
    bec::GRTManager::get()->set_app_option("db.search:SearchLimit", grt::IntegerRef(limit_total));
    bec::GRTManager::get()->set_app_option("db.search:SearchLimitPerTable", grt::IntegerRef(limit_table));
    bec::GRTManager::get()->set_app_option("db.search:SearchInvert", grt::IntegerRef(invert));

    _filter_panel.set_searching(true);
    _search_panel.show(true);

    _search_panel.search(
      wrapper, search_keyword, filters, SearchMode(search_type), limit_total, limit_table, invert,
      _filter_panel.search_all_types() ? search_all_types : text_type, _filter_panel.search_all_types() ? "CHAR" : "",
      std::bind(&DBSearchView::finished_search, this), std::bind(&DBSearchView::failed_search, this));
  }

public:
  DBSearchView(db_query_EditorRef editor)
    : mforms::AppView(false, "Database Search", "dbsearch", false),
      _editor(editor),
      _search_panel(),
      _check_selection_timeout(0),
      _search_timeout(0),
      _last_selection_change(0) {
    set_padding(12);
    set_spacing(12);
    _label.set_text("Enter text to search in tables selected in the schema tree");
    _label.set_style(mforms::BoldStyle);
    add(&_label, false, true);

    _description.set_text(
      "A text search will be done on the selected tables using SELECT. Note that this can be very slow since it will "
      "search all columns from all tables.");
    _description.set_style(mforms::SmallHelpTextStyle);
    add(&_description, false, true);

    add(&_filter_panel, false, true);
    add(&_search_panel, true, true);

    _filter_panel.search_field()->signal_action()->connect(
      std::bind(&DBSearchView::search_activate, this, std::placeholders::_1));
    _filter_panel.search_button()->signal_clicked()->connect(std::bind(&DBSearchView::start_search, this));

    _search_panel.show(false);

    grt::GRTNotificationCenter::get()->add_grt_observer(this, "GRNLiveDBObjectSelectionDidChange", editor);

    _filter_panel.set_search_type((int)bec::GRTManager::get()->get_app_option_int("db.search:SearchType", 0));
    _filter_panel.set_limit_total(
      base::strfmt("%li", bec::GRTManager::get()->get_app_option_int("db.search:SearchLimit", 10000)));
    _filter_panel.set_limit_table(
      base::strfmt("%li", bec::GRTManager::get()->get_app_option_int("db.search:SearchLimitPerTable", 100)));
    _filter_panel.set_exclude(bec::GRTManager::get()->get_app_option_int("db.search:SearchInvert", 0) != 0);

    _tree_selection = _editor->schemaTreeSelection();
    _filter_panel.search_button()->set_enabled(_tree_selection.count() > 0);
  }

  virtual ~DBSearchView() {
    grt::GRTNotificationCenter::get()->remove_grt_observer(this, "GRNLiveDBObjectSelectionDidChange", _editor);
    if (_check_selection_timeout)
      mforms::Utilities::cancel_timeout(_check_selection_timeout);
    if (_search_timeout)
      mforms::Utilities::cancel_timeout(_search_timeout);
  }
};

class MySQLDBSearchModuleImpl : public grt::ModuleImplBase, public PluginInterfaceImpl {
public:
  MySQLDBSearchModuleImpl(grt::CPPModuleLoader *ldr) : grt::ModuleImplBase(ldr) {
  }

  DEFINE_INIT_MODULE(MODULE_VERSION, "Oracle and/or its affiliates", grt::ModuleImplBase,
                     DECLARE_MODULE_FUNCTION(MySQLDBSearchModuleImpl::getPluginInfo),
                     DECLARE_MODULE_FUNCTION(MySQLDBSearchModuleImpl::showSearchPanel), NULL);

  virtual grt::ListRef<app_Plugin> getPluginInfo() override {
    grt::ListRef<app_Plugin> plugins(true);
    {
      app_PluginRef plugin(grt::Initialized);

      plugin->moduleName("MySQLDBSearchModule");
      plugin->pluginType("standalone");
      plugin->moduleFunctionName("showSearchPanel");
      plugin->name("com.mysql.wb.menu.database.search");
      plugin->caption("DataSearch");
      plugin->groups().insert("database/Databaclearse");

      app_PluginObjectInputRef pdef(grt::Initialized);
      pdef->name("activeSQLEditor");
      pdef->objectStructName(db_query_Editor::static_class_name());
      plugin->inputValues().insert(pdef);

      plugins.insert(plugin);
    }
    return plugins;
  }

  int showSearchPanel(db_query_EditorRef editor) {
    mforms::DockingPoint *dpoint = dynamic_cast<mforms::DockingPoint *>(mforms_from_grt(editor->dockingPoint()));

    DBSearchView *v;

    dpoint->dock_view(v = mforms::manage(new DBSearchView(editor)), "", 0);
    dpoint->select_view(v);
    v->set_title("Search");

    return 0;
  }
};

GRT_MODULE_ENTRY_POINT(MySQLDBSearchModuleImpl);
