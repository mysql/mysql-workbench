/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grtui/grt_wizard_plugin.h"
#include "grtui/wizard_view_text_page.h"

#include "db_mysql_diff_reporting.h"
#include "backend/db_plugin_be.h"
#include "base/string_utilities.h"
#include "mforms/treeview.h"

using namespace grtui;
using namespace mforms;
using namespace base;

#include "grtui/connection_page.h"
#include "frontend/multi_source_selector_page.h"
#include "frontend/fetch_schema_names_multi_page.h"
#include "frontend/fetch_schema_contents_multi_page.h"

class MultiSchemaSelectionPage : public WizardPage {
public:
  MultiSchemaSelectionPage(WizardForm *form, const char *name)
    : WizardPage(form, name), _body(true), _left(mforms::TreeFlatList), _right(mforms::TreeFlatList) {
    set_title("Select Schemas from Source and Target to be Compared");
    set_short_title("Select Schemas");

    add(&_body, true, true);

    _body.set_spacing(12);
    _body.set_homogeneous(true);
    _body.add(&_left, true, true);
    _body.add(&_right, true, true);

    _left.add_column(mforms::IconStringColumnType, "Source Schema", 300, false);
    _left.end_columns();
    _left.signal_changed()->connect(std::bind(&MultiSchemaSelectionPage::validate, this));

    _right.add_column(mforms::IconStringColumnType, "Target Schema", 300, false);
    _right.end_columns();
    _right.signal_changed()->connect(std::bind(&MultiSchemaSelectionPage::validate, this));
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      std::string icon = bec::IconManager::get_instance()->get_icon_path("db.Schema.16x16.png");
      grt::StringListRef schemata(grt::StringListRef::cast_from(values().get("schemata")));
      grt::StringListRef targetSchemata(grt::StringListRef::cast_from(values().get("targetSchemata")));

      _left.clear();
      for (grt::StringListRef::const_iterator i = schemata.begin(); i != schemata.end(); ++i) {
        mforms::TreeNodeRef node = _left.add_node();
        node->set_string(0, *i);
        node->set_icon_path(0, icon);
      }

      _right.clear();
      for (grt::StringListRef::const_iterator i = targetSchemata.begin(); i != targetSchemata.end(); ++i) {
        mforms::TreeNodeRef node = _right.add_node();
        node->set_string(0, *i);
        node->set_icon_path(0, icon);
      }
    }
  }

  virtual void leave(bool advancing) {
    if (advancing) {
      {
        grt::StringListRef slist(grt::Initialized);
        slist.insert(grt::StringRef(_left.get_selected_node()->get_string(0)));
        values().set("selectedOriginalSchemata", slist);
      }
      {
        grt::StringListRef slist(grt::Initialized);
        slist.insert(grt::StringRef(_right.get_selected_node()->get_string(0)));
        values().set("selectedSchemata", slist);
      }
    }
  }

  virtual bool allow_next() {
    return _left.get_selected_node() && _right.get_selected_node();
  }

protected:
  mforms::Box _body;
  mforms::TreeView _left;
  mforms::TreeView _right;
};

//--------------------------------------------------------------------------------

class ViewResultPage : public ViewTextPage {
public:
  ViewResultPage(WizardForm *form)
    : ViewTextPage(form, "viewdiff", (ViewTextPage::Buttons)(ViewTextPage::SaveButton | ViewTextPage::CopyButton),
                   "Text Files (*.txt)|*.txt") {
    set_short_title(_("Differences Report"));
    set_title(_("Differences Found in Catalog Comparison"));
    _text.set_language(mforms::LanguageNone);
  }

  void set_generate_text_slot(const std::function<std::string()> &slot) {
    _generate = slot;
  }

  virtual void enter(bool advancing) {
    if (advancing)
      _text.set_value(_generate());
  }

  virtual bool allow_cancel() {
    return false;
  }
  virtual bool next_closes_wizard() {
    return true;
  }

protected:
  std::function<std::string()> _generate;
};

//--------------------------------------------------------------------------------

class WbPluginDiffReport : public WizardPlugin {
public:
  WbPluginDiffReport(grt::Module *module) : WizardPlugin(module) {
    set_name("Diff Report Wizard");
    setInternalName("diff_report_wizard");
    add_page(mforms::manage(_source_page = new MultiSourceSelectPage(this, false)));
    _source_page->relayout();

    _left_db.grtm(true);
    _right_db.grtm(true);

    ConnectionPage *connect;
    // Pick source connection (optional)
    add_page(mforms::manage(
      connect = new ConnectionPage(this, "connect_source", "db.mysql.compareSchema:left_source_connection")));
    connect->set_db_connection(_left_db.db_conn());
    connect->set_title(std::string("Source Database: ").append(connect->get_title()));
    connect->set_short_title("Source Database");
    // Pick target connection (optional)
    add_page(mforms::manage(
      connect = new ConnectionPage(this, "connect_target", "db.mysql.compareSchema:right_source_connection")));
    connect->set_db_connection(_right_db.db_conn());
    connect->set_title(std::string("Target Database: ").append(connect->get_title()));
    connect->set_short_title("Target Database");

    // Fetch names from source and target if they're DBs, reveng script if they're files
    FetchSchemaNamesSourceTargetProgressPage *fetch_names_page;
    add_page(mforms::manage(fetch_names_page =
                              new FetchSchemaNamesSourceTargetProgressPage(this, _source_page, "fetch_names")));
    fetch_names_page->set_load_schemata_slot(
      _left_db.db_conn(), std::bind(&WbPluginDiffReport::load_schemata, this, &_left_db), _right_db.db_conn(),
      std::bind(&WbPluginDiffReport::load_schemata, this, &_right_db));
    fetch_names_page->set_model_catalog(_be.get_model_catalog());

    // Pick what to synchronize
    _schema_pick_page = new MultiSchemaSelectionPage(this, "pick_schemata");
    add_page(mforms::manage(_schema_pick_page));

    // Fetch contents from source and target, if they come from the database.. otherwise the schemas are already loaded
    // (optional)
    FetchSchemaContentsSourceTargetProgressPage *fetch_schema_page;
    add_page(mforms::manage(fetch_schema_page =
                              new FetchSchemaContentsSourceTargetProgressPage(this, _source_page, "fetch_schema")));
    fetch_schema_page->set_db_plugin(&_left_db, &_right_db);

    ViewResultPage *page;
    add_page(mforms::manage(page = new ViewResultPage(this)));
    page->set_generate_text_slot(std::bind(&WbPluginDiffReport::generate_report, this));

    set_title(_("Compare and Report Differences in Catalogs"));
  }

  std::vector<std::string> load_schemata(Db_plugin *db) {
    std::vector<std::string> names;
    db->load_schemata(names);
    return names;
  }

  std::string generate_report() {
    db_CatalogRef left_catalog, right_catalog;

    if (_source_page->get_left_source() == DataSourceSelector::ServerSource)
      left_catalog = _left_db.db_catalog();
    else if (_source_page->get_left_source() == DataSourceSelector::FileSource)
      left_catalog = db_CatalogRef::cast_from(values().get("left_file_catalog"));
    else if (_source_page->get_left_source() == DataSourceSelector::ModelSource)
      left_catalog = _be.get_model_catalog();

    if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
      right_catalog = _right_db.db_catalog();
    else if (_source_page->get_right_source() == DataSourceSelector::FileSource)
      right_catalog = db_CatalogRef::cast_from(values().get("right_file_catalog"));
    else if (_source_page->get_right_source() == DataSourceSelector::ModelSource)
      right_catalog = _be.get_model_catalog();

    std::string report;
    try {
      report = _be.generate_report(db_mysql_CatalogRef::cast_from(left_catalog),
                                   db_mysql_CatalogRef::cast_from(right_catalog));
    } catch (const std::exception &exc) {
      report = base::strfmt("Error generating report: %s", exc.what());
    }
    return report;
  }

  virtual WizardPage *get_next_page(WizardPage *current) {
    std::string curid = current->get_id();
    std::string nextid;

    if (curid == "source") {
      if (_source_page->get_left_source() == DataSourceSelector::ServerSource)
        nextid = "connect_source";
      else if (_source_page->get_right_source() == DataSourceSelector::ServerSource)
        nextid = "connect_target";
      else
        nextid = "fetch_names";
    }

    if (!nextid.empty())
      return get_page_with_id(nextid);
    else
      return WizardForm::get_next_page(current);
  }

protected:
  DbMySQLDiffReporting _be;
  Db_plugin _left_db;
  Db_plugin _right_db;
  MultiSourceSelectPage *_source_page;
  MultiSchemaSelectionPage *_schema_pick_page;

  std::vector<std::string> load_schemas(Db_plugin *db) {
    std::vector<std::string> names;
    db->load_schemata(names);
    return names;
  }
};

WizardPlugin *createWbPluginDiffReport(grt::Module *module) {
  return new WbPluginDiffReport(module);
}
