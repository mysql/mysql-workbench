/*
* Copyright (c) 2004, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_MYSQL_IMPORT_DBD4_H_
#define _WB_MYSQL_IMPORT_DBD4_H_


#include "wb_mysql_import_public_interface.h"
#include "grtpp.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.model.h"
#include "grts/structs.workbench.model.h"
#include "grts/structs.workbench.physical.h"


using namespace grt;


class Wb_mysql_import_DBD4
{
private:
  class Import_exception;
  enum Import_result { pr_undefined= 0, pr_processed };
  class Neutral_state_keeper;
  struct Simple_type_flag;
  typedef std::map<int, db_mysql_SchemaRef> Schemata;
  typedef std::map<int, db_mysql_TableRef> Tables;
  typedef std::map<int, db_mysql_ColumnRef> Columns;
  typedef std::map<int, std::string> SimpleDatatypes;
  typedef std::map<std::string, int> SimpleDatatypesRevInd;
  typedef std::map<int, std::list<Simple_type_flag> > SimpleDatatypesFlags;
  typedef std::map<int, workbench_physical_TableFigureRef> TableFigures;

public:
  Wb_mysql_import_DBD4();
  int import_DBD4(workbench_physical_ModelRef model, const char *file_name, grt::DictRef options);

private:
  void set_fkey_references();

  db_mysql_SchemaRef ensure_schema_created(int index, const char *name);
  void remove_unused_schemata();

  grt::GRT *_grt;
  db_mysql_CatalogRef _catalog;
  SimpleDatatypes _datatypes;
  SimpleDatatypesFlags _datatypes_flags;
  SimpleDatatypesRevInd _datatypes_revind;
  Schemata _schemata;
  Tables _tables;
  Columns _columns;
  TableFigures _table_figures;
  ListRef<db_mysql_Schema> _created_schemata;
  bool _gen_fk_names_when_empty; // generate unique fk name when name is not given

  struct Simple_type_flag
  {
    std::string name;
    int default_val;
  };

  class Neutral_state_keeper
  {
  public:
    Neutral_state_keeper(Wb_mysql_import_DBD4 *import_dbd4)
      : _import_dbd4(import_dbd4)
    {}

    ~Neutral_state_keeper()
    {
      _import_dbd4->_catalog= db_mysql_CatalogRef();
      _import_dbd4->_grt= NULL;
      _import_dbd4->_table_figures.clear();
      _import_dbd4->_columns.clear();
      _import_dbd4->_tables.clear();
      _import_dbd4->_created_schemata= ListRef<db_mysql_Schema>();
      _import_dbd4->_schemata.clear();
      _import_dbd4->_datatypes_revind.clear();
      _import_dbd4->_datatypes.clear();
      _import_dbd4->_datatypes_flags.clear();
      _import_dbd4->_gen_fk_names_when_empty= true;
    }

  private:
    Wb_mysql_import_DBD4 *_import_dbd4;
  };
  friend class Neutral_state_keeper;

  class Import_exception : public std::exception
  {
  public:
    Import_exception(const std::string& msg_text) : _msg_text(msg_text), _flag(2) {};
    Import_exception(const char * msg_text) : _msg_text(msg_text), _flag(2) {};
    virtual ~Import_exception() THROW() {}
    const char *what() const THROW() { return _msg_text.c_str(); }
    int flag() const { return _flag; }
    void flag(int val) { _flag= val; }
  private:
    std::string _msg_text;
    int _flag;
  };
};


#endif // _WB_MYSQL_IMPORT_DBD4_H_
