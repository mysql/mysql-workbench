/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grtui/wizard_progress_page.h"

class FetchSchemaNamesSourceTargetProgressPage : public WizardProgressPage {
public:
  FetchSchemaNamesSourceTargetProgressPage(WizardForm *form, MultiSourceSelectPage *source_page,
                                           const char *name = "fetchNames")
    : WizardProgressPage(form, name, true),
      _source_page(source_page),
      _source_dbconn(0),
      _target_dbconn(0),
      _finished(0) {
    set_title(_("Retrieve Source and Target Schema Names"));
    set_short_title(_("Get Source and Target"));

    set_status_text("");
  }

  void set_load_schemata_slot(DbConnection *sdbc, const std::function<std::vector<std::string>()> &sslot,
                              DbConnection *tdbc, const std::function<std::vector<std::string>()> &tslot) {
    _source_dbconn = sdbc;
    _target_dbconn = tdbc;
    _load_source_schemata = sslot;
    _load_target_schemata = tslot;
  }

  void set_model_catalog(db_CatalogRef catalog) {
    _model_catalog = catalog;
  }

protected:
  bool perform_connect(bool source) {
    DbConnection *dbc = source ? _source_dbconn : _target_dbconn;
    db_mgmt_ConnectionRef conn = dbc->get_connection();

    execute_grt_task(std::bind(&FetchSchemaNamesSourceTargetProgressPage::do_connect, this, dbc), false);

    return true;
  }

  grt::ValueRef do_connect(DbConnection *dbc) {
    if (!dbc)
      throw std::logic_error("must call set_db_connection() 1st");
    dbc->test_connection();

    return grt::ValueRef();
  }

  bool perform_fetch(bool source) {
    execute_grt_task(std::bind(&FetchSchemaNamesSourceTargetProgressPage::do_fetch, this, source), false);
    return true;
  }

  static bool collate(const std::string &a, const std::string &b) {
    return g_utf8_collate(a.c_str(), b.c_str()) < 0;
  }

  grt::ValueRef do_fetch(bool source) {
    std::vector<std::string> schema_names = source ? _load_source_schemata() : _load_target_schemata();

    // order the schema names alphabetically
    std::sort(schema_names.begin(), schema_names.end(),
      std::bind(&FetchSchemaNamesSourceTargetProgressPage::collate, std::placeholders::_1, std::placeholders::_2));

    grt::StringListRef list(grt::Initialized);
    for (std::vector<std::string>::const_iterator iter = schema_names.begin(); iter != schema_names.end(); ++iter)
      list.insert(*iter);

    if (source)
      values().set("schemata", list);
    else
      values().set("targetSchemata", list);

    _finished++;

    return grt::ValueRef();
  }

  bool perform_script_fetch(bool source) {
    std::string path = values().get_string(source ? "left_source_file" : "right_source_file");
    db_CatalogRef catalog = parse_catalog_from_file(path);

    grt::StringListRef schemata(grt::Initialized);
    for (size_t i = 0; i < catalog->schemata().count(); i++)
      schemata.insert(catalog->schemata()[i]->name());

    if (source) {
      values().set("left_file_catalog", catalog);
      values().set("schemata", schemata);
    } else {
      values().set("right_file_catalog", catalog);
      values().set("targetSchemata", schemata);
    }

    _finished++;

    return true;
  }

  db_CatalogRef parse_catalog_from_file(const std::string &filename) {
    workbench_physical_ModelRef pm = workbench_physical_ModelRef::cast_from(_model_catalog->owner());

    db_mysql_CatalogRef cat(grt::Initialized);
    cat->version(pm->rdbms()->version());
    grt::replace_contents(cat->simpleDatatypes(), pm->rdbms()->simpleDatatypes());

    cat->name("default");
    cat->oldName("default");

    GError *file_error = NULL;
    char *sql_input_script = NULL;
    gsize sql_input_script_length = 0;

    if (!g_file_get_contents(filename.c_str(), &sql_input_script, &sql_input_script_length, &file_error)) {
      std::string file_error_msg("Error reading input file: ");
      file_error_msg.append(file_error->message);
      throw std::runtime_error(file_error_msg);
    }

    SqlFacade::Ref sql_parser = SqlFacade::instance_for_rdbms(pm->rdbms());
    sql_parser->parseSqlScriptString(cat, sql_input_script);
    g_free(sql_input_script);

    return cat;
  }

  bool perform_model_fetch(bool source) {
    {
      db_CatalogRef catalog(_model_catalog);
      grt::StringListRef names(grt::Initialized);
      for (size_t i = 0; i < catalog->schemata().count(); i++)
        names.insert(catalog->schemata()[i]->name());
      values().set(source ? "schemata" : "targetSchemata", names);
    }
    _finished++;
    return true;
  }

  virtual void enter(bool advancing) {
    if (advancing) {
      clear_tasks();
      switch (_source_page->get_left_source()) {
        case DataSourceSelector::ServerSource:
          add_async_task(_("Connect to Source DBMS"),
                         std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_connect, this, true),
                         _("Connecting to Source DBMS..."));

          add_async_task(_("Retrieve Schema List from Source Database"),
                         std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_fetch, this, true),
                         _("Retrieving schema list from source database..."));
          break;
        case DataSourceSelector::FileSource:
          add_task(_("Retrieve database objects from source file"),
                   std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_script_fetch, this, true),
                   _("Retrieving objects from selected source file..."));
          break;
        case DataSourceSelector::ModelSource:
          add_task(_("Load schemas from source model"),
                   std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_model_fetch, this, true),
                   _("Loading schemas from source model..."));
          break;
      }
      switch (_source_page->get_right_source()) {
        case DataSourceSelector::ServerSource:
          add_async_task(_("Connect to Target DBMS"),
                         std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_connect, this, false),
                         _("Connecting to Target DBMS..."));

          add_async_task(_("Retrieve Schema List from Target Database"),
                         std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_fetch, this, false),
                         _("Retrieving schema list from target database..."));
          break;
        case DataSourceSelector::FileSource:
          add_task(_("Retrieve database objects from target file"),
                   std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_script_fetch, this, false),
                   _("Retrieving objects from selected target file..."));
          break;
        case DataSourceSelector::ModelSource:
          add_task(_("Load schemas from target model"),
                   std::bind(&FetchSchemaNamesSourceTargetProgressPage::perform_model_fetch, this, false),
                   _("Loading schemas from target model..."));
          break;
      }
      end_adding_tasks(_("Execution Completed Successfully"));

      _finished = 0;
      reset_tasks();
    }

    WizardProgressPage::enter(advancing);
  }

  virtual bool allow_next() {
    return _finished == 2;
  }

private:
  MultiSourceSelectPage *_source_page;

  db_CatalogRef _model_catalog;

  DbConnection *_source_dbconn;
  DbConnection *_target_dbconn;
  std::function<std::vector<std::string>()> _load_source_schemata;
  std::function<std::vector<std::string>()> _load_target_schemata;

  int _finished;
};
