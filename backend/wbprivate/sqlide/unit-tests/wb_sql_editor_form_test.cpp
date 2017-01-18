/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "grts/structs.db.query.h"
#include "sqlide/wb_context_sqlide.h"
#include "sqlide/wb_live_schema_tree.h"
#include "sqlide/wb_sql_editor_tree_controller.h"
#include "stub/stub_mforms.h"

#include "grt.h"

#include "cppdbc.h"
#include "wb_helpers.h"
#include "cppconn/driver.h"
#include "cppconn/sqlstring.h"

#include "sqlide/wb_sql_editor_form.h"

using namespace grt;
using namespace wb;
using namespace sql;

/* This class is a friend of the SqlEditorForm on DEBUG mode,
   this way everything is available for testing */
class EditorFormTester {
private:
  wb::LiveSchemaTree::NodeChildrenUpdaterSlot updater_slot;
  wb::LiveSchemaTree::NewSchemaContentArrivedSlot schema_content_arrived_slot;

public:
  EditorFormTester()
    : _expect_schema_content_arrived(false),
      _expect_update_node_children(false),
      _mock_validate_schema_content(false),
      _mock_propagate_schema_content(false),
      _mock_propagate_update_node_children(false)

  {
    updater_slot =
      std::bind(&EditorFormTester::mock_update_node_children, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4, std::placeholders::_5);
    schema_content_arrived_slot =
      std::bind(&EditorFormTester::mock_schema_content_arrived, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
  }
  void set_target(const SqlEditorForm::Ref &sql_editor) {
    _form = sql_editor;
  }

  void fetch_column_data(const std::string &schema_name, const std::string &obj_name,
                         wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_column_data(schema_name, obj_name, object_type, updater_slot);
  }

  void fetch_index_data(const std::string &schema_name, const std::string &obj_name,
                        wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_index_data(schema_name, obj_name, object_type, updater_slot);
  }

  void fetch_trigger_data(const std::string &schema_name, const std::string &obj_name,
                          wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_trigger_data(schema_name, obj_name, object_type, updater_slot);
  }

  void fetch_foreign_key_data(const std::string &schema_name, const std::string &obj_name,
                              wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_foreign_key_data(schema_name, obj_name, object_type, updater_slot);
  }

  void fetch_schema_contents(const std::string &schema_name) {
    _form->get_live_tree()->fetch_schema_contents(schema_name, schema_content_arrived_slot);
    g_usleep(1000000); // 1 second
    perform_idle_tasks();
  }

  void perform_idle_tasks() {
    bec::GRTManager::get()->perform_idle_tasks();
  }

  std::list<std::string> fetch_schema_list() {
    return _form->get_live_tree()->fetch_schema_list();
  }

  void load_schema_list() {
    return _form->get_live_tree()->tree_refresh();
  }

  void load_schema_data(const std::string &schema) {
    mforms::TreeNodeRef schema_node =
      _form->get_live_tree()->_schema_tree->get_node_for_object(schema, wb::LiveSchemaTree::Schema, "");
    _form->get_live_tree()->_schema_tree->expand_toggled(schema_node, true);
    g_usleep(1000000);
    perform_idle_tasks();
  }

  void exec_sql(std::string &sql) {
    _form->exec_sql_returning_results(sql, false);
  }

  /* mock function that will simulate the schema list loading using this thread */
  void tree_refresh() {
    std::list<std::string> sl = _form->get_live_tree()->fetch_schema_list();
    base::StringListPtr schema_list(new std::list<std::string>());
    schema_list->assign(sl.begin(), sl.end());
    _form->get_live_tree()->_schema_tree->update_schemata(schema_list);
  }

  void set_lst_model_view(mforms::TreeView *pmodel_view) {
    _form->get_live_tree()->_schema_tree->set_model_view(pmodel_view);
    _form->get_live_tree()->_schema_tree->enable_events(true);
  }

  bool mock_update_node_children(mforms::TreeNodeRef parent, base::StringListPtr children,
                                 wb::LiveSchemaTree::ObjectType type, bool sorted = false, bool just_append = false) {
    tut::ensure(_check_id + " : Unexpected call to update_node_children", _expect_update_node_children);
    _expect_update_node_children = false;

    if (_mock_propagate_update_node_children) {
      _form->get_live_tree()->_schema_tree->update_node_children(parent, children, type, sorted, just_append);
    }

    return false;
  }

  void mock_schema_content_arrived(const std::string &schema_name, base::StringListPtr tables,
                                   base::StringListPtr views, base::StringListPtr procedures,
                                   base::StringListPtr functions, bool just_append) {
    tut::ensure(_check_id + " : Unexpected call to schema_content_arrived", _expect_schema_content_arrived);
    _expect_schema_content_arrived = false;

    if (_mock_validate_schema_content) {
      _mock_validate_schema_content = false;

      tut::ensure_equals(_check_id + " : Unexpected number of tables received", tables->size(),
                         _expected_tables.size());

      std::list<std::string>::const_iterator index, end = tables->end();
      for (index = tables->begin(); index != end; index++)
        tut::ensure(_check_id + " : Unexpected table retrieved",
                    std::find(_expected_tables.begin(), _expected_tables.end(), *index) != _expected_tables.end());

      _expected_tables.clear();

      tut::ensure_equals(_check_id + " : Unexpected number of views received", views->size(), _expected_views.size());

      end = views->end();
      for (index = views->begin(); index != end; index++)
        tut::ensure(_check_id + " : Unexpected view retrieved",
                    std::find(_expected_views.begin(), _expected_views.end(), *index) != _expected_views.end());

      _expected_views.clear();

      tut::ensure_equals(_check_id + " : Unexpected number of procedures received", procedures->size(),
                         _expected_procedures.size());

      end = procedures->end();
      for (index = procedures->begin(); index != end; index++)
        tut::ensure(
          _check_id + " : Unexpected procedure retrieved",
          std::find(_expected_procedures.begin(), _expected_procedures.end(), *index) != _expected_procedures.end());

      _expected_procedures.clear();

      tut::ensure_equals(_check_id + " : Unexpected number of functions received", functions->size(),
                         _expected_functions.size());

      end = functions->end();
      for (index = functions->begin(); index != end; index++)
        tut::ensure(
          _check_id + " : Unexpected function retrieved",
          std::find(_expected_functions.begin(), _expected_functions.end(), *index) != _expected_functions.end());

      _expected_functions.clear();
    }

    if (_mock_propagate_schema_content) {
      //_form->get_live_tree()->_schema_tree->schema_contents_arrived(schema_name, tables, views, procedures, functions,
      //just_append);
    }
  }

  void clean_and_reset() {
    tut::ensure(_check_id + " : Missing call to schema_content_arrived", !_expect_schema_content_arrived);
    _expect_schema_content_arrived = false;

    tut::ensure(_check_id + " : Missing call to update_node_children", !_expect_update_node_children);
    _expect_update_node_children = false;
  }

public:
  bool _expect_schema_content_arrived;
  bool _expect_update_node_children;
  bool _mock_validate_schema_content;
  bool _mock_propagate_schema_content;
  bool _mock_propagate_update_node_children;
  std::list<std::string> _expected_tables;
  std::list<std::string> _expected_views;
  std::list<std::string> _expected_procedures;
  std::list<std::string> _expected_functions;
  std::list<std::string> _expected_children;

  std::string _expected_schema_name;
  std::string _expected_parent_name;

  std::string _check_id;

private:
  SqlEditorForm::Ref _form;
};

BEGIN_TEST_DATA_CLASS(wb_sql_editor_form_test)
public:
WBTester *tester;
WBContextSQLIDE *wb_context_sqlide;
sql::ConnectionWrapper connection;
SqlEditorForm::Ref form;
EditorFormTester *form_tester;
mforms::TreeView *pmodel_view;

void set_connection_properties(db_mgmt_ConnectionRef &connection) {
  grt::DictRef conn_params(true);
  conn_params.set("hostName", grt::StringRef(test_params->get_host_name()));
  conn_params.set("port", grt::IntegerRef(test_params->get_port()));
  conn_params.set("userName", grt::StringRef(test_params->get_user_name()));
  conn_params.set("password", grt::StringRef(test_params->get_password()));
  grt::replace_contents(connection->parameterValues(), conn_params);

  db_mgmt_DriverRef driverProperties = db_mgmt_DriverRef::cast_from(grt::GRT::get()->get("/rdbms/drivers/0/"));
  connection->driver(driverProperties);
}

sql::ConnectionWrapper create_connection_for_import() {
  // init database connection
  db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

  set_connection_properties(connectionProperties);

  sql::DriverManager *dm = sql::DriverManager::getDriverManager();

  return dm->getConnection(connectionProperties);
}

TEST_DATA_CONSTRUCTOR(wb_sql_editor_form_test) {
  bec::GRTManager::get(); // need to bcreated first
  tester = new WBTester;
  wb_context_sqlide = new WBContextSQLIDE();
  form_tester = new EditorFormTester();

  populate_grt(*tester);

  connection = create_connection_for_import();

  db_mgmt_ConnectionRef my_connection(grt::Initialized);
  set_connection_properties(my_connection);
  form = SqlEditorForm::create(wb_context_sqlide, my_connection);
  form->connect(std::shared_ptr<sql::TunnelConnection>());

  pmodel_view =
    new mforms::TreeView(mforms::TreeNoColumns | mforms::TreeNoBorder | mforms::TreeSidebar | mforms::TreeNoHeader);

  form_tester->set_target(form);
  form_tester->set_lst_model_view(pmodel_view);

  //"USE `wb_sql_editor_form_test`;"
  static const char *sql1 = //
    "DROP DATABASE IF EXISTS `wb_sql_editor_form_test`;"
    "CREATE DATABASE IF NOT EXISTS `wb_sql_editor_form_test` DEFAULT CHARSET=latin1 DEFAULT COLLATE = "
    "latin1_swedish_ci;"

    "USE `wb_sql_editor_form_test`;"

    "CREATE TABLE language ("
    "language_id TINYINT UNSIGNED NOT NULL AUTO_INCREMENT,"
    "name CHAR(20) NOT NULL,"
    "last_update TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
    "PRIMARY KEY (language_id)"
    ")ENGINE=InnoDB DEFAULT CHARSET=utf8;"

    "CREATE TABLE film ("
    "film_id SMALLINT UNSIGNED NOT NULL AUTO_INCREMENT,"
    "title VARCHAR(255) NOT NULL,"
    "description TEXT DEFAULT NULL,"
    "release_year YEAR DEFAULT NULL,"
    "language_id TINYINT UNSIGNED NOT NULL,"
    "original_language_id TINYINT UNSIGNED DEFAULT NULL,"
    "rental_duration TINYINT UNSIGNED NOT NULL DEFAULT 3,"
    "rental_rate DECIMAL(4,2) NOT NULL DEFAULT 4.99,"
    "length SMALLINT UNSIGNED DEFAULT NULL,"
    "replacement_cost DECIMAL(5,2) NOT NULL DEFAULT 19.99,"
    "rating ENUM('G','PG','PG-13','R','NC-17') DEFAULT 'G',"
    "special_features SET('Trailers','Commentaries','Deleted Scenes','Behind the Scenes') DEFAULT NULL,"
    "last_update TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
    "PRIMARY KEY  (film_id),"
    "KEY idx_title (title),"
    "KEY idx_fk_language_id (language_id),"
    "KEY idx_fk_original_language_id (original_language_id),"
    "CONSTRAINT fk_film_language FOREIGN KEY (language_id) REFERENCES language (language_id) ON DELETE RESTRICT ON "
    "UPDATE CASCADE,"
    "CONSTRAINT fk_film_language_original FOREIGN KEY (original_language_id) REFERENCES language (language_id) ON "
    "DELETE RESTRICT ON UPDATE CASCADE"
    ")ENGINE=InnoDB DEFAULT CHARSET=utf8;"

    "CREATE TABLE film_text ("
    "film_id SMALLINT NOT NULL,"
    "title VARCHAR(255) NOT NULL,"
    "description TEXT,"
    "PRIMARY KEY  (film_id),"
    "FULLTEXT KEY idx_title_description (title,description)"
    ")ENGINE=MyISAM DEFAULT CHARSET=utf8;"

    "CREATE TABLE dummy_table ("
    "dummy_table_id SMALLINT NOT NULL,"
    "name VARCHAR(255) NOT NULL,"
    "description TEXT,"
    "PRIMARY KEY  (dummy_table_id)"
    ")ENGINE=MyISAM DEFAULT CHARSET=utf8;"

    "CREATE TABLE complex_pk_table ("
    "dummy_first_id SMALLINT NOT NULL,"
    "dummy_second_id SMALLINT NOT NULL,"
    "title VARCHAR(255) NOT NULL,"
    "description TEXT,"
    "PRIMARY KEY  (dummy_first_id,dummy_second_id),"
    "FULLTEXT KEY idx_title_description (title,description)"
    ")ENGINE=MyISAM DEFAULT CHARSET=utf8;"

    "CREATE TABLE no_pk_table ("
    "dummy_id SMALLINT NOT NULL,"
    "title VARCHAR(255) NOT NULL,"
    "description TEXT,"
    "FULLTEXT KEY idx_title_description (title,description)"
    ")ENGINE=MyISAM DEFAULT CHARSET=utf8;"

    "CREATE TABLE pk_table_unique_not_null ("
    "dummy_pk SMALLINT NOT NULL,"
    "dummy_id SMALLINT NOT NULL,"
    "title VARCHAR(255) NOT NULL,"
    "description TEXT,"
    "FULLTEXT KEY idx_title_description (title,description),"
    "PRIMARY KEY  (dummy_pk),"
    "UNIQUE KEY dummy_id (dummy_id)"
    ")ENGINE=MyISAM DEFAULT CHARSET=utf8;"

    "CREATE TABLE no_pk_table_unique_not_null ("
    "dummy_id SMALLINT NOT NULL,"
    "title VARCHAR(255) NOT NULL,"
    "description TEXT,"
    "FULLTEXT KEY idx_title_description (title,description),"
    "UNIQUE KEY dummy_id (dummy_id)"
    ")ENGINE=MyISAM DEFAULT CHARSET=utf8;"

    "CREATE VIEW dummy_film_view"
    " AS"
    " SELECT film_id, title, description, release_year"
    " FROM film;"

    " DELIMITER ;;\n"
    " CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN"
    " INSERT INTO film_text (film_id, title, description)"
    " VALUES (new.film_id, new.title, new.description);"
    " END;;"

    " CREATE TRIGGER `upd_film` AFTER UPDATE ON `film` FOR EACH ROW BEGIN"
    " IF (old.title != new.title) or (old.description != new.description)"
    " THEN"
    " UPDATE film_text"
    " SET title=new.title,"
    " description=new.description,"
    " film_id=new.film_id"
    " WHERE film_id=old.film_id;"
    " END IF;"
    " END;;"

    " CREATE TRIGGER `del_film` AFTER DELETE ON `film` FOR EACH ROW BEGIN"
    " DELETE FROM film_text WHERE film_id = old.film_id;"
    " END;;"

    " CREATE FUNCTION `dummy_function`() RETURNS int(1)"
    " BEGIN"
    " RETURN 1;"
    " END;;"

    " CREATE FUNCTION `other_function`() RETURNS int(1)"
    " BEGIN"
    " RETURN 1;"
    " END;;"

    " CREATE PROCEDURE `get_films`()"
    " BEGIN"
    " select * from film;"
    " END;;"

    " DELIMITER ;\n";

  std::string sql(sql1);
  form_tester->exec_sql(sql);

  form_tester->tree_refresh();
  form_tester->load_schema_data("wb_sql_editor_form_test");

  // Stay for some time to finish the setup (it's done in a background thread).
  g_usleep(1000000);
  form_tester->perform_idle_tasks();
}

END_TEST_DATA_CLASS;

TEST_MODULE(wb_sql_editor_form_test, "sql editor form test");

// Testing fetch_schema_list.
TEST_FUNCTION(1) {
  std::list<std::string> schema_list = form_tester->fetch_schema_list();

  ensure("TF001CHK001: Unexpected number of schemas retrieved", schema_list.size() > 0);

  bool found = false;

  std::list<std::string>::iterator index, end = schema_list.end();
  for (index = schema_list.begin(); !found && index != end; index++)
    found = (*index) == "wb_sql_editor_form_test";

  ensure("TF001CHK002: wb_sql_editor_form_test not found on retrieved list", found);
}

// Testing fetch_schema_content.
TEST_FUNCTION(2) {
  // Loads the schema list
  form_tester->tree_refresh();

  // Sets the expectations..
  form_tester->_expect_schema_content_arrived = true;
  form_tester->_mock_validate_schema_content = true;

  form_tester->_expected_tables.push_back("language");
  form_tester->_expected_tables.push_back("film");
  form_tester->_expected_tables.push_back("film_text");
  form_tester->_expected_tables.push_back("dummy_table");
  form_tester->_expected_tables.push_back("complex_pk_table");
  form_tester->_expected_tables.push_back("no_pk_table");
  form_tester->_expected_tables.push_back("pk_table_unique_not_null");
  form_tester->_expected_tables.push_back("no_pk_table_unique_not_null");

  form_tester->_expected_views.push_back("dummy_film_view");

  form_tester->_expected_procedures.push_back("get_films");

  form_tester->_expected_functions.push_back("dummy_function");
  form_tester->_expected_functions.push_back("other_function");

  form_tester->_check_id = "TF002CHK001";

  // Loads the specific schema...
  form_tester->fetch_schema_contents("wb_sql_editor_form_test");

  form_tester->clean_and_reset();
}

// Testing for SqlEditorForm::fetch_column_data.
TEST_FUNCTION(3) {
  mforms::TreeNodeRef table_node;
  mforms::TreeNodeRef collection_node;
  mforms::TreeNodeRef child_node;
  wb::LiveSchemaTree::TableData *pdata;
  wb::LiveSchemaTree::ColumnData *pchild_data;

  // Loads the schema list into the tree...
  form_tester->tree_refresh();

  // Loads a specific schema contents...
  form_tester->load_schema_data("wb_sql_editor_form_test");

  // Loads the column data from the language table.
  form_tester->_expect_update_node_children = true;
  form_tester->_mock_propagate_update_node_children = true;
  form_tester->_check_id = "TF003CHK001";
  form_tester->fetch_column_data("wb_sql_editor_form_test", "language", wb::LiveSchemaTree::Table);

  table_node = form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                             wb::LiveSchemaTree::Table, "language");
  pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(table_node->get_data());

  ensure("TF003CHK002 : Columns were not loaded", pdata->is_data_loaded(wb::LiveSchemaTree::COLUMN_DATA));

  // Gets the columns node...
  collection_node = table_node->get_child(0);

  // Now validates each column...
  child_node = collection_node->get_child(0);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::ColumnData *>(child_node->get_data());

  ensure_equals("TF003CHK002 : Unexpected column name", child_node->get_string(0), "language_id");
  ensure("TF003CHK002 : Unexpected primary key flag", pchild_data->is_pk);
  ensure("TF003CHK002 : Unexpected foreign key flag", !pchild_data->is_fk);

  child_node = collection_node->get_child(1);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::ColumnData *>(child_node->get_data());

  ensure_equals("TF003CHK003 : Unexpected column name", child_node->get_string(0), "name");
  ensure("TF003CHK003 : Unexpected primary key flag", !pchild_data->is_pk);
  ensure("TF003CHK003 : Unexpected foreign key flag", !pchild_data->is_fk);

  child_node = collection_node->get_child(2);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::ColumnData *>(child_node->get_data());

  ensure_equals("TF003CHK004 : Unexpected column name", child_node->get_string(0), "last_update");
  ensure("TF003CHK004 : Unexpected primary key flag", !pchild_data->is_pk);
  ensure("TF003CHK004 : Unexpected foreign key flag", !pchild_data->is_fk);
}

// Testing for SqlEditorForm::fetch_index_data.
TEST_FUNCTION(4) {
  mforms::TreeNodeRef table_node;
  mforms::TreeNodeRef collection_node;
  mforms::TreeNodeRef child_node;
  wb::LiveSchemaTree::TableData *pdata;
  wb::LiveSchemaTree::IndexData *pchild_data;

  // Loads the schema list into the tree...
  form_tester->tree_refresh();

  // Loads a specific schema contents...
  form_tester->_check_id = "TF004CHK001";
  form_tester->load_schema_data("wb_sql_editor_form_test");

  // Loads the index data from the film table.
  form_tester->_expect_update_node_children = true;
  form_tester->_mock_propagate_update_node_children = true;
  form_tester->_check_id = "TF004CHK002";
  form_tester->fetch_index_data("wb_sql_editor_form_test", "film", wb::LiveSchemaTree::Table);

  table_node = form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                             wb::LiveSchemaTree::Table, "film");
  pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(table_node->get_data());

  ensure("TF004CHK003 : Indexes were not loaded", pdata->is_data_loaded(wb::LiveSchemaTree::INDEX_DATA));

  // Gets the indexes node...
  collection_node = table_node->get_child(1);
  ensure_equals("TF004CHK004 : Unexpected nuber of indexes", collection_node->count(), 4);

  // Now validates each index...
  child_node = collection_node->get_child(0);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::IndexData *>(child_node->get_data());

  ensure_equals("TF004CHK005 : Unexpected index name", child_node->get_string(0), "PRIMARY");
  ensure("TF004CHK005 : Unexpected non unique index found", pchild_data->unique);
  ensure_equals("TF004CHK005 : Unexpected index type", pchild_data->type, 6);

  child_node = collection_node->get_child(1);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::IndexData *>(child_node->get_data());

  ensure_equals("TF004CHK006 : Unexpected index name", child_node->get_string(0), "idx_title");
  ensure("TF004CHK006 : Unexpected unique index found", !pchild_data->unique);
  ensure_equals("TF004CHK006 : Unexpected index type", pchild_data->type, 6);

  child_node = collection_node->get_child(2);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::IndexData *>(child_node->get_data());

  ensure_equals("TF004CHK007 : Unexpected index name", child_node->get_string(0), "idx_fk_language_id");
  ensure("TF004CHK007 : Unexpected non index found", !pchild_data->unique);
  ensure_equals("TF004CHK007 : Unexpected index type", pchild_data->type, 6);

  child_node = collection_node->get_child(3);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::IndexData *>(child_node->get_data());

  ensure_equals("TF004CHK008 : Unexpected index name", child_node->get_string(0), "idx_fk_original_language_id");
  ensure("TF004CHK008 : Unexpected non unique index found", !pchild_data->unique);
  ensure_equals("TF004CHK008 : Unexpected index type", pchild_data->type, 6);
}

// Testing for SqlEditorForm::fetch_trigger_data.
TEST_FUNCTION(5) {
  mforms::TreeNodeRef table_node;
  mforms::TreeNodeRef collection_node;
  mforms::TreeNodeRef child_node;
  wb::LiveSchemaTree::TableData *pdata;
  wb::LiveSchemaTree::TriggerData *pchild_data;

  // Loads the schema list into the tree...
  form_tester->tree_refresh();

  // Loads a specific schema contents...
  form_tester->_check_id = "TF005CHK001";
  form_tester->load_schema_data("wb_sql_editor_form_test");

  // Loads the trigger data from the film table.
  form_tester->_expect_update_node_children = true;
  form_tester->_mock_propagate_update_node_children = true;
  form_tester->_check_id = "TF005CHK002";
  form_tester->fetch_trigger_data("wb_sql_editor_form_test", "film", wb::LiveSchemaTree::Table);

  table_node = form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                             wb::LiveSchemaTree::Table, "film");
  pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(table_node->get_data());

  ensure("TF005CHK003 : Triggers were not loaded", pdata->is_data_loaded(wb::LiveSchemaTree::TRIGGER_DATA));

  // Gets the triggers node...
  collection_node = table_node->get_child(3);
  ensure_equals("TF005CHK004 : Unexpected nuber of triggers", collection_node->count(), 3);

  // Now validates each index...
  child_node = collection_node->get_child(0);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::TriggerData *>(child_node->get_data());

  ensure_equals("TF005CHK004 : Unexpected trigger name", child_node->get_string(0), "ins_film");
  ensure_equals("TF005CHK004 : Unexpected trigger event", pchild_data->event_manipulation, 11); // 11 is INSERT
  ensure_equals("TF005CHK004 : Unexpected trigger timing", pchild_data->timing, 15);            // 15 is AFTER

  child_node = collection_node->get_child(1);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::TriggerData *>(child_node->get_data());

  ensure_equals("TF005CHK005 : Unexpected trigger name", child_node->get_string(0), "upd_film");
  ensure_equals("TF005CHK005 : Unexpected trigger event", pchild_data->event_manipulation, 12); // 12 is UPDATE
  ensure_equals("TF005CHK005 : Unexpected trigger timing", pchild_data->timing, 15);            // 15 is AFTER

  child_node = collection_node->get_child(2);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::TriggerData *>(child_node->get_data());

  ensure_equals("TF005CHK006 : Unexpected trigger name", child_node->get_string(0), "del_film");
  ensure_equals("TF005CHK006 : Unexpected trigger event", pchild_data->event_manipulation, 13); // 13 is DELETE
  ensure_equals("TF005CHK006 : Unexpected trigger timing", pchild_data->timing, 15);            // 15 is AFTER
}

// Testing for SqlEditorForm::fetch_foreign_key_data.
TEST_FUNCTION(6) {
  mforms::TreeNodeRef table_node;
  mforms::TreeNodeRef collection_node;
  mforms::TreeNodeRef child_node;
  wb::LiveSchemaTree::TableData *pdata;
  wb::LiveSchemaTree::FKData *pchild_data;

  // Loads the schema list into the tree...
  form_tester->tree_refresh();

  // Loads a specific schema contents...
  form_tester->_check_id = "TF006CHK001";
  form_tester->load_schema_data("wb_sql_editor_form_test");

  // Loads the foreign key data from the film table.
  form_tester->_expect_update_node_children = true;
  form_tester->_mock_propagate_update_node_children = true;
  form_tester->_check_id = "TF006CHK002";
  form_tester->fetch_foreign_key_data("wb_sql_editor_form_test", "film", wb::LiveSchemaTree::Table);

  table_node = form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                             wb::LiveSchemaTree::Table, "film");
  pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(table_node->get_data());

  ensure("TF006CHK003 : Foreign keys were not loaded", pdata->is_data_loaded(wb::LiveSchemaTree::FK_DATA));

  // Gets the foreign keys node...
  collection_node = table_node->get_child(2);
  ensure_equals("TF006CHK004 : Unexpected nuber of foreign keys", collection_node->count(), 2);

  // Now validates each index...
  child_node = collection_node->get_child(0);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::FKData *>(child_node->get_data());

  ensure_equals("TF006CHK004 : Unexpected foreign key name", child_node->get_string(0), "fk_film_language");
  ensure_equals("TF006CHK004 : Unexpected foreign key update rule", pchild_data->update_rule, 1);
  ensure_equals("TF006CHK004 : Unexpected foreign key delete rule", pchild_data->delete_rule, 4);
  ensure_equals("TF006CHK004 : Unexpected foreign key delete rule", pchild_data->referenced_table, "language");

  child_node = collection_node->get_child(1);
  pchild_data = dynamic_cast<wb::LiveSchemaTree::FKData *>(child_node->get_data());

  ensure_equals("TF006CHK005 : Unexpected foreign key name", child_node->get_string(0), "fk_film_language_original");
  ensure_equals("TF006CHK005 : Unexpected foreign key update rule", pchild_data->update_rule, 1);
  ensure_equals("TF006CHK005 : Unexpected foreign key delete rule", pchild_data->delete_rule, 4);
  ensure_equals("TF006CHK005 : Unexpected foreign key delete rule", pchild_data->referenced_table, "language");
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  // cleanup
  std::string sql = "DROP DATABASE wb_sql_editor_form_test";
  form_tester->exec_sql(sql);
  form->close();
  form.reset();

  delete form_tester;
  delete wb_context_sqlide;
  delete tester;
}

END_TESTS
