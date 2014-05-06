/* 
 * Copyright (c) 2011, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grtpp.h"
#include "../../plugins/db.mysql.editors/backend/mysql_table_editor.h"
#include "synthetic_mysql_model.h"
#include "wb_helpers.h"


using namespace grt;
using namespace bec;
using namespace tut;


BEGIN_TEST_DATA_CLASS(mysql_table_editor)
public:
  WBTester wbt;
  GRTManager *grtm;
  GRT *grt;
  db_mgmt_RdbmsRef rdbms;
  void reset_wbt_document();

TEST_DATA_CONSTRUCTOR(mysql_table_editor)
{
  wbt.flush_until(0.5);
  grt= wbt.wb->get_grt();
  grtm= wbt.wb->get_grt_manager();
  wbt.create_new_document();
}

END_TEST_DATA_CLASS


TEST_MODULE(mysql_table_editor, "mysql_table_editor");


void Test_object_base<mysql_table_editor>::reset_wbt_document()
{
  wbt.renew_document();
  rdbms= wbt.wb->get_document()->physicalModels().get(0)->rdbms();
}


TEST_FUNCTION(1)
{
  reset_wbt_document();
  ensure("db_mgmt_RdbmsRef initialization", rdbms.is_valid());
}


TEST_FUNCTION(4) 
{
const char* corrupted_trigger_sql= 
"--\n"
"-- Triggers for loading film_text from film\n"
"--\n"
"DELIMITER //\n"
"CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN\n"
"    INSERT INTO film_text (film_id, title, description)\n"
"        VALUES (new.film_id, new.title, new.description);\n"
"  END//\n"
"CREATE TRIGGER `upd_film` AFTER !!_UPDATE_!! ON `film` FOR EACH ROW BEGIN\n"
"    IF (old.title != new.title) or (old.description != new.description)\n"
"    THEN\n"
"        UPDATE film_text\n"
"            SET title=new.title,\n"
"                description=new.description,\n"
"                film_id=new.film_id\n"
"        WHERE film_id=old.film_id;\n"
"    END IF;\n"
"  END//\n"
"CRTE TRIGGER `del_film1` AFTER DELETE ON `film` FOR EACH ROW BEGIN\n"
"    DELETE FROM film_text WHERE film_id = old.film_id;\n"
"  END//\n"
"DELIMITER ;\n";

  reset_wbt_document();
  SynteticMySQLModel model(&wbt);

  model.schema->name("test_schema");
  model.table->name("film");
  MySQLTableEditorBE t(grtm, model.table, model.model->rdbms());

  t.parse_triggers_sql(grt, corrupted_trigger_sql);

  assure_equal(model.table->triggers().count(), 3U);
  std::string names[]= {"ins_film", "SYNTAX_ERROR_1", "SYNTAX_ERROR_2"};

  for (size_t i= 0, size= model.table->triggers().count(); i < size; i++)
  {
    std::string name= model.table->triggers().get(i)->name();
    assure_equal(name, names[i]);
  }
}

TEST_FUNCTION(13)
{
  // check if adding columns/indices/foreign keys by setting name of placeholder item works

  reset_wbt_document();
  SynteticMySQLModel model(&wbt);

  db_mysql_TableRef table= model.table;
  table->name("table");
  table->columns().remove_all();
  table->indices().remove_all();
  table->foreignKeys().remove_all();

  MySQLTableEditorBE editor(grtm, table, rdbms);

  ensure_equals("add column", table->columns().count(), 0U);
  ((bec::TableColumnsListBE*)editor.get_columns())->set_field(0, 0, "newcol");
  ((bec::TableColumnsListBE*)editor.get_columns())->set_field(0, 1, "int(11)");
  ensure_equals("add column", table->columns().count(), 1U);


  editor.get_indexes()->select_index(0);
  ensure_equals("add index", table->indices().count(), 0U);
  editor.get_indexes()->set_field(0, 0, "index");
  ensure_equals("add index", table->indices().count(), 1U);

  editor.get_fks()->select_fk(0);
  ensure_equals("add fk", table->foreignKeys().count(), 0U);
  editor.get_fks()->set_field(0, 0, "newfk");
  ensure_equals("add fk", table->foreignKeys().count(), 1U);
}

const char* table_sql= 
"use test_schema;\n"
"--\n"
"-- Triggers for loading film_text from film\n"
"--\n"
"DELIMITER //\n"
"CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN\n"
"    INSERT INTO film_text (film_id, title, description)\n"
"        VALUES (new.film_id, new.title, new.description);\n"
"  END//\n"
"CREATE TRIGGER `upd_film` AFTER UPDATE ON `film` FOR EACH ROW BEGIN\n"
"    IF (old.title != new.title) or (old.description != new.description)\n"
"    THEN\n"
"        UPDATE film_text\n"
"            SET title=new.title,\n"
"                description=new.description,\n"
"                film_id=new.film_id\n"
"        WHERE film_id=old.film_id;\n"
"    END IF;\n"
"  END//\n"
"CREATE TRIGGER `del_film` AFTER DELETE ON `film` FOR EACH ROW BEGIN\n"
"    DELETE FROM film_text WHERE film_id = old.film_id;\n"
"  END//\n"
"DELIMITER ;\n";

TEST_FUNCTION(20) 
{
  reset_wbt_document();
  SynteticMySQLModel model(&wbt);

  model.schema->name("test_schema");
  model.table->name("film");
  MySQLTableEditorBE t(grtm, model.table, model.model->rdbms());

  t.parse_triggers_sql(grt, table_sql);
  for (size_t i= 0, size= model.table->triggers().count(); i < size; i++)
  {
    db_TriggerRef t= model.table->triggers().get(i);
    std::string name= t->name();
  }
  assure_equal(model.table->triggers().count(), 3U);
}

TEST_FUNCTION(21) 
{
const char* corrupted_trigger_sql= 
"--\n"
"-- Triggers for loading film_text from film\n"
"--\n"
"DELIMITER //\n"
"CREATE TRIGGER `ins_film` AFTER INSERT ON `film` FOR EACH ROW BEGIN\n"
"    INSERT INTO film_text (film_id, title, description)\n"
"        VALUES (new.film_id, new.title, new.description);\n"
"  END//\n"
"CREATE TRIGGER `upd_film` AFTER !!_UPDATE_!! ON `film` FOR EACH ROW BEGIN\n"
"    IF (old.title != new.title) or (old.description != new.description)\n"
"    THEN\n"
"        UPDATE film_text\n"
"            SET title=new.title,\n"
"                description=new.description,\n"
"                film_id=new.film_id\n"
"        WHERE film_id=old.film_id;\n"
"    END IF;\n"
"  END//\n"
"CREATE TRIGGER `del_film` AFTER DELETE ON `film` FOR EACH ROW BEGIN\n"
"    DELETE FROM film_text WHERE film_id = old.film_id;\n"
"  END//\n"
"DELIMITER ;\n";

  reset_wbt_document();
  SynteticMySQLModel model(&wbt);

  model.schema->name("test_schema");
  model.table->name("film");
  MySQLTableEditorBE t(grtm, model.table, model.model->rdbms());

  t.parse_triggers_sql(grt, corrupted_trigger_sql);

  for (size_t i= 0, size= model.table->triggers().count(); i < size; i++)
  {
    db_TriggerRef t= model.table->triggers().get(i);
    std::string name= t->name();
  }
  assure_equal(model.table->triggers().count(), 3U);
  std::string names[]= {"ins_film", "SYNTAX_ERROR_1", "del_film"};

  for (size_t i= 0, size= model.table->triggers().count(); i < size; i++)
  {
    std::string name= model.table->triggers().get(i)->name();
    assure_equal(name, names[i]);
  }
}

END_TESTS
