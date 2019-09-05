/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "casmine.h"
#include "wb_test_helpers.h"

#include "grts/structs.db.query.h"
#include "sqlide/wb_context_sqlide.h"
#include "sqlide/wb_live_schema_tree.h"
#include "sqlide/wb_sql_editor_tree_controller.h"
#include "stub/stub_mforms.h"

#include "grt.h"

#include "cppdbc.h"
#include "wb_connection_helpers.h"

#include "cppconn/driver.h"
#include "cppconn/sqlstring.h"

#include "sqlide/wb_sql_editor_form.h"

using namespace grt;
using namespace wb;
using namespace sql;

$ModuleEnvironment() {};

// This class is a friend of the SqlEditorForm in TESTING mode. This way everything is available for testing.
class LocalEditorFormTester {
private:
  wb::LiveSchemaTree::NodeChildrenUpdaterSlot updaterSlot;
  wb::LiveSchemaTree::NewSchemaContentArrivedSlot schemaContentArrivedSlot;

public:
  LocalEditorFormTester() :
    _expectSchemaContentArrived(false),
    _expectUpdateNodeChildren(false),
    _mockValidateSchemaContent(false),
    _mockPropagateSchemaContent(false),
    _mockPropagateUpdateNodeChildren(false) {
    updaterSlot = std::bind(&LocalEditorFormTester::mock_update_node_children, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                            std::placeholders::_4, std::placeholders::_5);
    schemaContentArrivedSlot = std::bind(&LocalEditorFormTester::mock_schema_content_arrived, this, std::placeholders::_1, std::placeholders::_2,
                                         std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6);
  }

  void set_target(const SqlEditorForm::Ref &sql_editor) {
    _form = sql_editor;
  }

  void fetch_column_data(const std::string &schema_name, const std::string &obj_name, wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_column_data(schema_name, obj_name, object_type, updaterSlot);
  }

  void fetch_index_data(const std::string &schema_name, const std::string &obj_name, wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_index_data(schema_name, obj_name, object_type, updaterSlot);
  }

  void fetch_trigger_data(const std::string &schema_name, const std::string &obj_name, wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_trigger_data(schema_name, obj_name, object_type, updaterSlot);
  }

  void fetch_foreign_key_data(const std::string &schema_name, const std::string &obj_name, wb::LiveSchemaTree::ObjectType object_type) {
    _form->get_live_tree()->fetch_foreign_key_data(schema_name, obj_name, object_type, updaterSlot);
  }

  void fetch_schema_contents(const std::string &schema_name) {
    _form->get_live_tree()->fetch_schema_contents(schema_name, schemaContentArrivedSlot);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    perform_idle_tasks();
  }

  void perform_idle_tasks() {
    bec::GRTManager::get()->perform_idle_tasks();
  }

  std::vector<std::string> fetch_schema_list() {
    return _form->get_live_tree()->fetch_schema_list();
  }

  void load_schema_list() {
    return _form->get_live_tree()->tree_refresh();
  }

  void load_schema_data(const std::string &schema) {
    mforms::TreeNodeRef schema_node = _form->get_live_tree()->_schema_tree->get_node_for_object(schema, wb::LiveSchemaTree::Schema, "");
    _form->get_live_tree()->_schema_tree->expand_toggled(schema_node, true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    perform_idle_tasks();
  }

  void exec_sql(std::string &sql) {
    _form->exec_sql_returning_results(sql, false);
  }

  /* mock function that will simulate the schema list loading using this thread */
  void tree_refresh() {
    std::vector<std::string> sl = _form->get_live_tree()->fetch_schema_list();
    base::StringListPtr schema_list(new std::list<std::string>());
    schema_list->assign(sl.begin(), sl.end());
    _form->get_live_tree()->_schema_tree->update_schemata(schema_list);
  }

  void set_lst_model_view(mforms::TreeView *pmodel_view) {
    _form->get_live_tree()->_schema_tree->set_model_view(pmodel_view);
    _form->get_live_tree()->_schema_tree->enable_events(true);
  }

  bool mock_update_node_children(mforms::TreeNodeRef parent, base::StringListPtr children, wb::LiveSchemaTree::ObjectType type, bool sorted = false,
                                 bool just_append = false) {
    $expect(_expectUpdateNodeChildren).toBeTrue(_checkId + " : Unexpected call to update_node_children");
    _expectUpdateNodeChildren = false;

    if (_mockPropagateUpdateNodeChildren) {
      _form->get_live_tree()->_schema_tree->update_node_children(parent, children, type, sorted, just_append);
    }

    return false;
  }

  void mock_schema_content_arrived(const std::string &schema_name, base::StringListPtr tables, base::StringListPtr views,
    base::StringListPtr procedures, base::StringListPtr functions, bool just_append) {
    $expect(_expectSchemaContentArrived).toBeTrue(_checkId + " : Unexpected call to schema_content_arrived");
    _expectSchemaContentArrived = false;

    if (_mockValidateSchemaContent) {
      _mockValidateSchemaContent = false;
      $expect(tables->size()).toBe(_expectedTables.size(), _checkId + " : Unexpected number of tables received");

      std::list<std::string>::const_iterator index, end = tables->end();
      for (index = tables->begin(); index != end; index++) {
        $expect(std::find(_expectedTables.begin(), _expectedTables.end(), *index) != _expectedTables.end()).toBeTrue(_checkId + " : Unexpected table retrieved");
      }

      _expectedTables.clear();

      $expect(views->size()).toBe(_expectedViews.size(), _checkId + " : Unexpected number of views received");

      end = views->end();
      for (index = views->begin(); index != end; index++) {
        $expect(std::find(_expectedViews.begin(), _expectedViews.end(), *index) != _expectedViews.end()).toBeTrue(_checkId + " : Unexpected view retrieved");
      }

      _expectedViews.clear();

      $expect(procedures->size()).toBe(_expectedProcedures.size(), _checkId + " : Unexpected number of procedures received");

      end = procedures->end();
      for (index = procedures->begin(); index != end; index++) {
        $expect(std::find(_expectedProcedures.begin(), _expectedProcedures.end(), *index) != _expectedProcedures.end()).toBeTrue(_checkId + " : Unexpected procedure retrieved");
      }

      _expectedProcedures.clear();

      $expect(functions->size()).toBe(_expectedFunctions.size(), _checkId + " : Unexpected number of functions received");

      end = functions->end();
      for (index = functions->begin(); index != end; index++) {
        $expect(std::find(_expectedFunctions.begin(), _expectedFunctions.end(), *index) != _expectedFunctions.end()).toBeTrue(_checkId + " : Unexpected function retrieved");
      }

      _expectedFunctions.clear();
    }

    if (_mockPropagateSchemaContent) {
      //_form->get_live_tree()->_schema_tree->schema_contents_arrived(schema_name, tables, views, procedures, functions,
      //just_append);
    }
  }

  void clean_and_reset() {
    $expect(_expectSchemaContentArrived).toBeFalse(_checkId + " : Missing call to schema_content_arrived");

    _expectSchemaContentArrived = false;

    $expect(_expectUpdateNodeChildren).toBeFalse(_checkId + " : Missing call to update_node_children");
    _expectUpdateNodeChildren = false;
  }

 public:
  bool _expectSchemaContentArrived;
  bool _expectUpdateNodeChildren;
  bool _mockValidateSchemaContent;
  bool _mockPropagateSchemaContent;
  bool _mockPropagateUpdateNodeChildren;
  std::list<std::string> _expectedTables;
  std::list<std::string> _expectedViews;
  std::list<std::string> _expectedProcedures;
  std::list<std::string> _expectedFunctions;
  std::list<std::string> _expectedChildren;

  std::string _expectedSchemaName;
  std::string _expectedParentName;

  std::string _checkId;

 private:
  SqlEditorForm::Ref _form;
};

namespace {

$TestData {
  std::unique_ptr<WorkbenchTester> tester;
  WBContextSQLIDE *wbContextSqlide;
  sql::ConnectionWrapper connection;
  SqlEditorForm::Ref form;
  LocalEditorFormTester *formTester;
  mforms::TreeView *pmodelView;
};

$describe("SQL Editor Form") {
  $beforeAll([&]() {
    bec::GRTManager::get(); // Ensure the GRT instance exists.

    data->tester.reset(new WorkbenchTester());
    data->wbContextSqlide = new WBContextSQLIDE();
    data->formTester = new LocalEditorFormTester();

    data->tester->initializeRuntime();

    // init database connection
    db_mgmt_ConnectionRef connectionProperties(grt::Initialized);

    setupConnectionEnvironment(connectionProperties);
    db_mgmt_DriverRef driverProperties = db_mgmt_DriverRef::cast_from(grt::GRT::get()->get("/rdbms/drivers/0/"));
    connectionProperties->driver(driverProperties);

    sql::DriverManager *dm = sql::DriverManager::getDriverManager();

    data->connection = dm->getConnection(connectionProperties);

    db_mgmt_ConnectionRef myConnection(grt::Initialized);
    setupConnectionEnvironment(myConnection);
    myConnection->driver(driverProperties);

    data->form = SqlEditorForm::create(data->wbContextSqlide, myConnection);
    data->form->connect(std::shared_ptr<wb::SSHTunnel>());

    data->pmodelView =
      new mforms::TreeView(mforms::TreeNoColumns | mforms::TreeNoBorder | mforms::TreeSidebar | mforms::TreeNoHeader);

    data->formTester->set_target(data->form);
    data->formTester->set_lst_model_view(data->pmodelView);

    //"USE `wb_sql_editor_form_test`;"
    static const char *sql1 =
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

      " CREATE FUNCTION `dummy_function`() RETURNS int(1) DETERMINISTIC"
      " BEGIN"
      " RETURN 1;"
      " END;;"

      " CREATE FUNCTION `other_function`() RETURNS int(1) DETERMINISTIC"
      " BEGIN"
      " RETURN 1;"
      " END;;"

      " CREATE PROCEDURE `get_films`() READS SQL DATA"
      " BEGIN"
      " select * from film;"
      " END;;"

      " DELIMITER ;\n";

    std::string sql(sql1);
    data->formTester->exec_sql(sql);

    data->formTester->tree_refresh();
    data->formTester->load_schema_data("wb_sql_editor_form_test");

    // Stay for some time to finish the setup (it's done in a background thread).
    std::this_thread::sleep_for(std::chrono::seconds(1));
    data->formTester->perform_idle_tasks();
  });

  $afterAll([&]() {
    // cleanup
    std::string sql = "DROP DATABASE wb_sql_editor_form_test";
    data->formTester->exec_sql(sql);
    data->form->close();
    data->form.reset();

    delete data->formTester;
    delete data->wbContextSqlide;
  });

  $it("Testing fetch_schema_list.", [&]() {
    std::vector<std::string> schemaList = data->formTester->fetch_schema_list();

    $expect(schemaList.size() > 0).toBeTrue("TF001CHK001: Unexpected number of schemas retrieved");

    bool found = false;

    std::vector<std::string>::iterator index, end = schemaList.end();
    for (index = schemaList.begin(); !found && index != end; index++)
      found = (*index) == "wb_sql_editor_form_test";

    $expect(found).toBeTrue("TF001CHK002: wb_sql_editor_form_test not found on retrieved list");

  });

  $it("Loads the schema list.", [&]() {
    data->formTester->tree_refresh();

    // Sets the expectations..
    data->formTester->_expectSchemaContentArrived = true;
    data->formTester->_mockValidateSchemaContent = true;

    data->formTester->_expectedTables.push_back("language");
    data->formTester->_expectedTables.push_back("film");
    data->formTester->_expectedTables.push_back("film_text");
    data->formTester->_expectedTables.push_back("dummy_table");
    data->formTester->_expectedTables.push_back("complex_pk_table");
    data->formTester->_expectedTables.push_back("no_pk_table");
    data->formTester->_expectedTables.push_back("pk_table_unique_not_null");
    data->formTester->_expectedTables.push_back("no_pk_table_unique_not_null");

    data->formTester->_expectedViews.push_back("dummy_film_view");

    data->formTester->_expectedProcedures.push_back("get_films");

    data->formTester->_expectedFunctions.push_back("dummy_function");
    data->formTester->_expectedFunctions.push_back("other_function");

    data->formTester->_checkId = "TF002CHK001";

    // Loads the specific schema...
    data->formTester->fetch_schema_contents("wb_sql_editor_form_test");

    data->formTester->clean_and_reset();
  });

  $it("Testing for SqlEditorForm::fetch_column_data.", [&]() {
    mforms::TreeNodeRef tableNode;
    mforms::TreeNodeRef collectionNode;
    mforms::TreeNodeRef childNode;
    wb::LiveSchemaTree::TableData *pdata;
    wb::LiveSchemaTree::ColumnData *pchildData;

    // Loads the schema list into the tree...
    data->formTester->tree_refresh();

    // Loads a specific schema contents...
    data->formTester->load_schema_data("wb_sql_editor_form_test");

    // Loads the column data from the language table.
    data->formTester->_expectUpdateNodeChildren = true;
    data->formTester->_mockPropagateUpdateNodeChildren = true;
    data->formTester->_checkId = "TF003CHK001";
    data->formTester->fetch_column_data("wb_sql_editor_form_test", "language", wb::LiveSchemaTree::Table);

    tableNode = data->form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                                    wb::LiveSchemaTree::Table, "language");
    pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(tableNode->get_data());

    $expect(pdata->is_data_loaded(wb::LiveSchemaTree::COLUMN_DATA)).toBeTrue("TF003CHK002 : Columns were not loaded");

    // Gets the columns node...
    collectionNode = tableNode->get_child(0);

    // Now validates each column...
    childNode = collectionNode->get_child(0);
    pchildData = dynamic_cast<wb::LiveSchemaTree::ColumnData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("language_id", "TF003CHK002 : Unexpected column name");
    $expect(pchildData->is_pk).toBeTrue("TF003CHK002 : Unexpected primary key flag");
    $expect(pchildData->is_fk).toBeFalse("TF003CHK002 : Unexpected foreign key flag");

    childNode = collectionNode->get_child(1);
    pchildData = dynamic_cast<wb::LiveSchemaTree::ColumnData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("name", "TF003CHK003 : Unexpected column name");
    $expect(pchildData->is_pk).toBeFalse("TF003CHK003 : Unexpected primary key flag");
    $expect(pchildData->is_fk).toBeFalse("TF003CHK003 : Unexpected foreign key flag");

    childNode = collectionNode->get_child(2);
    pchildData = dynamic_cast<wb::LiveSchemaTree::ColumnData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("last_update", "TF003CHK004 : Unexpected column name");
    $expect(pchildData->is_pk).toBeFalse("TF003CHK004 : Unexpected primary key flag");
    $expect(pchildData->is_fk).toBeFalse("TF003CHK004 : Unexpected foreign key flag");
  });

  $it("Testing for SqlEditorForm::fetch_index_data.", [&]() {
    mforms::TreeNodeRef tableNode;
    mforms::TreeNodeRef collectionNode;
    mforms::TreeNodeRef childNode;
    wb::LiveSchemaTree::TableData *pdata;
    wb::LiveSchemaTree::IndexData *pchildData;

    // Loads the schema list into the tree...
    data->formTester->tree_refresh();

    // Loads a specific schema contents...
    data->formTester->_checkId = "TF004CHK001";
    data->formTester->load_schema_data("wb_sql_editor_form_test");

    // Loads the index data from the film table.
    data->formTester->_expectUpdateNodeChildren = true;
    data->formTester->_mockPropagateUpdateNodeChildren = true;
    data->formTester->_checkId = "TF004CHK002";
    data->formTester->fetch_index_data("wb_sql_editor_form_test", "film", wb::LiveSchemaTree::Table);

    tableNode = data->form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                                    wb::LiveSchemaTree::Table, "film");
    pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(tableNode->get_data());

    $expect(pdata->is_data_loaded(wb::LiveSchemaTree::INDEX_DATA)).toBeTrue("TF004CHK003 : Indexes were not loaded");

    // Gets the indexes node...
    collectionNode = tableNode->get_child(1);
    $expect(collectionNode->count()).toEqual(4, "TF004CHK004 : Unexpected nuber of indexes");

    // Now validates each index...
    childNode = collectionNode->get_child(0);
    pchildData = dynamic_cast<wb::LiveSchemaTree::IndexData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("PRIMARY", "TF004CHK005 : Unexpected index name");
    $expect(pchildData->unique).toBeTrue("TF004CHK005 : Unexpected non unique index found");
    $expect(pchildData->type).toEqual(6U, "TF004CHK005 : Unexpected index type");

    childNode = collectionNode->get_child(1);
    pchildData = dynamic_cast<wb::LiveSchemaTree::IndexData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("idx_title", "TF004CHK006 : Unexpected index name");
    $expect(pchildData->unique).toBeFalse("TF004CHK006 : Unexpected unique index found");
    $expect(pchildData->type).toEqual(6U, "TF004CHK006 : Unexpected index type");

    childNode = collectionNode->get_child(2);
    pchildData = dynamic_cast<wb::LiveSchemaTree::IndexData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("idx_fk_language_id", "TF004CHK007 : Unexpected index name");
    $expect(pchildData->unique).toBeFalse("TF004CHK007 : Unexpected non index found");
    $expect(pchildData->type).toEqual(6U, "TF004CHK007 : Unexpected index type");

    childNode = collectionNode->get_child(3);
    pchildData = dynamic_cast<wb::LiveSchemaTree::IndexData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("idx_fk_original_language_id", "TF004CHK008 : Unexpected index name");
    $expect(pchildData->unique).toBeFalse("TF004CHK008 : Unexpected non unique index found");
    $expect(pchildData->type).toEqual(6U, "TF004CHK008 : Unexpected index type");
  });

  $it("Testing for SqlEditorForm::fetch_trigger_data.", [&]() {
    mforms::TreeNodeRef tableNode;
    mforms::TreeNodeRef collectionNode;
    mforms::TreeNodeRef childNode;
    wb::LiveSchemaTree::TableData *pdata;
    wb::LiveSchemaTree::TriggerData *pchildData;

    // Loads the schema list into the tree...
    data->formTester->tree_refresh();

    // Loads a specific schema contents...
    data->formTester->_checkId = "TF005CHK001";
    data->formTester->load_schema_data("wb_sql_editor_form_test");

    // Loads the trigger data from the film table.
    data->formTester->_expectUpdateNodeChildren = true;
    data->formTester->_mockPropagateUpdateNodeChildren = true;
    data->formTester->_checkId = "TF005CHK002";
    data->formTester->fetch_trigger_data("wb_sql_editor_form_test", "film", wb::LiveSchemaTree::Table);

    tableNode = data->form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                                    wb::LiveSchemaTree::Table, "film");
    pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(tableNode->get_data());

    $expect(pdata->is_data_loaded(wb::LiveSchemaTree::TRIGGER_DATA)).toBeTrue("TF005CHK003 : Triggers were not loaded");

    // Gets the triggers node...
    collectionNode = tableNode->get_child(3);
    $expect(collectionNode->count()).toEqual(3, "TF005CHK004 : Unexpected nuber of triggers");

    // Now validates each index...
    childNode = collectionNode->get_child(0);
    pchildData = dynamic_cast<wb::LiveSchemaTree::TriggerData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("ins_film", "TF005CHK004 : Unexpected trigger name");
    $expect(pchildData->event_manipulation).toBe(11, "TF005CHK004 : Unexpected trigger event");// 11 is INSERT
    $expect(pchildData->timing).toBe(15, "TF005CHK004 : Unexpected trigger timing");// 15 is AFTER

    childNode = collectionNode->get_child(1);
    pchildData = dynamic_cast<wb::LiveSchemaTree::TriggerData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("upd_film", "TF005CHK005 : Unexpected trigger name");
    $expect(pchildData->event_manipulation).toBe(12, "TF005CHK005 : Unexpected trigger event");// 12 is UPDATE
    $expect(pchildData->timing).toBe(15, "TF005CHK005 : Unexpected trigger timing");// 15 is AFTER

    childNode = collectionNode->get_child(2);
    pchildData = dynamic_cast<wb::LiveSchemaTree::TriggerData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("del_film", "TF005CHK006 : Unexpected trigger name");
    $expect(pchildData->event_manipulation).toBe(13, "TF005CHK006 : Unexpected trigger event");// 13 is DELETE
    $expect(pchildData->timing).toBe(15, "TF005CHK006 : Unexpected trigger timing");// 15 is AFTER
  });

  $it("Testing for SqlEditorForm::fetch_foreign_key_data.", [&]() {
    mforms::TreeNodeRef tableNode;
    mforms::TreeNodeRef collectionNode;
    mforms::TreeNodeRef childNode;
    wb::LiveSchemaTree::TableData *pdata;
    wb::LiveSchemaTree::FKData *pchildData;

    // Loads the schema list into the tree...
    data->formTester->tree_refresh();

    // Loads a specific schema contents...
    data->formTester->_checkId = "TF006CHK001";
    data->formTester->load_schema_data("wb_sql_editor_form_test");

    // Loads the foreign key data from the film table.
    data->formTester->_expectUpdateNodeChildren = true;
    data->formTester->_mockPropagateUpdateNodeChildren = true;
    data->formTester->_checkId = "TF006CHK002";
    data->formTester->fetch_foreign_key_data("wb_sql_editor_form_test", "film", wb::LiveSchemaTree::Table);

    tableNode = data->form->get_live_tree()->get_schema_tree()->get_node_for_object("wb_sql_editor_form_test",
                                                                                    wb::LiveSchemaTree::Table, "film");
    pdata = dynamic_cast<wb::LiveSchemaTree::TableData *>(tableNode->get_data());

    $expect(pdata->is_data_loaded(wb::LiveSchemaTree::FK_DATA)).toBeTrue("TF006CHK003 : Foreign keys were not loaded");

    // Gets the foreign keys node...
    collectionNode = tableNode->get_child(2);
    $expect(collectionNode->count()).toEqual(2, "TF006CHK004 : Unexpected nuber of foreign keys");

    // Now validates each index...
    childNode = collectionNode->get_child(0);
    pchildData = dynamic_cast<wb::LiveSchemaTree::FKData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("fk_film_language", "TF006CHK004 : Unexpected foreign key name");
    $expect(pchildData->update_rule).toEqual(1U, "TF006CHK004 : Unexpected foreign key update rule");
    $expect(pchildData->delete_rule).toEqual(4U, "TF006CHK004 : Unexpected foreign key delete rule");
    $expect(pchildData->referenced_table).toBe("language", "TF006CHK004 : Unexpected foreign key delete rule");

    childNode = collectionNode->get_child(1);
    pchildData = dynamic_cast<wb::LiveSchemaTree::FKData *>(childNode->get_data());

    $expect(childNode->get_string(0)).toBe("fk_film_language_original", "TF006CHK005 : Unexpected foreign key name");
    $expect(pchildData->update_rule).toEqual(1U, "TF006CHK005 : Unexpected foreign key update rule");
    $expect(pchildData->delete_rule).toEqual(4U, "TF006CHK005 : Unexpected foreign key delete rule");
    $expect(pchildData->referenced_table).toBe("language", "TF006CHK005 : Unexpected foreign key delete rule");
  });
}

}
