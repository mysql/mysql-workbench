/* 
 * Copyright (c) 2011, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "tut_stdafx.h"
#include "grtpp.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"

using namespace grt;
using namespace bec;
using namespace std;

BEGIN_TEST_DATA_CLASS(grtdb_tests)
public:
  WBTester tester;
  GRTManager *grtm;

TEST_DATA_CONSTRUCTOR(grtdb_tests)
{
  grtm = tester.wb->get_grt_manager();
  populate_grt(tester.grt, tester);
}
END_TEST_DATA_CLASS

TEST_MODULE(grtdb_tests, "DB stuff tests");

TEST_FUNCTION(1)
{
  tester.create_new_document();
}

TEST_FUNCTION(5)
{
  // test primary key

  db_mysql_TableRef table(grtm->get_grt());
  table->name("tbl");

  db_mysql_ColumnRef column(grtm->get_grt());

  column->name("col");
  column->owner(table);

  table->columns().ginsert(column);

  table->addPrimaryKeyColumn(column);

  ensure("PK created", table->primaryKey().is_valid());
  ensure_equals("PK index created", table->indices().count(), 1U);

  ensure_equals("PK correct", table->primaryKey()->columns().count(), 1U);
  ensure("PK correct", table->primaryKey()->columns().get(0)->referencedColumn() == column);
  

  table->removePrimaryKeyColumn(column);

  ensure("PK removed", !table->primaryKey().is_valid());
  ensure_equals("PK index removed", table->indices().count(), 0U);
}


TEST_FUNCTION(10)
{
  db_mysql_TableRef table(grtm->get_grt());
  
  ensure_equals("index content type", table->indices().content_class_name(), "db.mysql.Index");
}


// Helper macros for column base types parser tests.
#define ensure_parse_ok(str) ensure(str, column->setParseType(str, tester.get_rdbms()->simpleDatatypes()) != 0);
#define ensure_parse_fail(str) ensure(str, !column->setParseType(str, tester.get_rdbms()->simpleDatatypes()));

TEST_FUNCTION(15)
{
  // Test some generally wrong cases. ml: testing invalid cases is just nonsense. Should be removed.
  db_ColumnRef column(grtm->get_grt());

  ensure_parse_fail("");
  ensure_parse_fail("()");
  ensure_parse_fail("(0)");
  ensure_parse_fail("INT(");
  ensure_parse_fail("INT)");
  ensure_parse_fail("INT()");
  ensure_parse_fail("INT(()");
  ensure_parse_fail("INT())");
  ensure_parse_fail("INT(xyz)");

  ensure_parse_fail("junk");
  ensure_parse_fail("junk(0)");
  ensure_parse_fail("junk(0,0)");
  ensure_parse_fail("junk('a','b', 'c')");
}

/**
 * Checks the values for precision and scale, as well as character and octet length against the given
 * values depending on the actual type.
 */
void check_type_cardinalities(db_SimpleDatatypeRef type, db_ColumnRef column, int precision, int scale)
{
  if (type->numericPrecision() != EMPTY_TYPE_PRECISION)
  {
    // Precision is optional, so both values must be equal: either both are set to EMTPY_TYPE_PRECISION 
    // or both have the same precision value.
    ensure_equals("Comparing precisions", *column->precision(), precision);

    // Scale can only be given if we also have a precision.
    if (type->numericScale() != EMPTY_TYPE_SCALE)
      // Scale is optional, so both values must be equal: either both are set to EMTPY_TYPE_SCALE 
      // or both have the same scale value.
      ensure_equals("Comparing scales", *column->scale(), scale);
    else
      ensure("Unexpected scale parameter found", *column->scale() == EMPTY_COLUMN_SCALE);
  }
  else
  {
    // If there's no numeric precision then check for character or octet cardinalities.
    if (type->characterMaximumLength() != EMPTY_TYPE_MAXIMUM_LENGTH
      || type->characterOctetLength() != EMPTY_TYPE_OCTET_LENGTH)
      ensure_equals("Comparing char or octet length", *column->length(), precision);
    else
      ensure("Unexpected precision parameter found", *column->length() == EMPTY_COLUMN_LENGTH);
  }
}

TEST_FUNCTION(20)
{
  // Go through all our defined datatypes and construct a column definition.
  // Then see if they all parse successfully.
  db_SchemaRef schema(grtm->get_grt());

  db_CatalogRef catalog = tester.get_catalog();
  schema->owner(catalog);

  db_mysql_TableRef table(grtm->get_grt());
  table->owner(schema);
  table->name("table");

  db_mysql_ColumnRef column(grtm->get_grt());
  column->owner(table);
  column->name("testee");
  table->columns().insert(column);

  ListRef<db_SimpleDatatype> types= tester.get_rdbms()->simpleDatatypes();
  for (size_t i= 0; i < tester.get_rdbms()->simpleDatatypes().count(); i++)
  {
    // Try all parameter combinations.
    string no_params= types[i]->name();
    string single_num_param= no_params + "(777)";
    string double_num_params= no_params + "(111, 5)";
    string enum_parameters= "('blah', 'foo', 'bar', 'gah')";
    string param_list= no_params + enum_parameters;
    string invalid_list= no_params + "(1, a, 'bb')";

    // Depending on the server version a data type is defined for we need to set the
    // correct version or parsing will fail where it should succeed.
    GrtVersionRef version(grtm->get_grt());
    version->majorNumber(5);
    std::string validity = types[i]->validity();
    if (validity == "<5.6")
      version->minorNumber(5);
    else
      if (validity == ">=5.6")
        version->minorNumber(6);
      else
        version->minorNumber(7);
    version->releaseNumber(-1);
    version->buildNumber(-1);
    catalog->version(version);

    // The parameter format type tells us which combination is valid.
    switch (types[i]->parameterFormatType())
    {
      case 0: // no params
        ensure_parse_ok(no_params);
        check_type_cardinalities(types[i], column, EMPTY_COLUMN_PRECISION, EMPTY_COLUMN_SCALE);
        ensure_parse_fail(single_num_param);
        ensure_parse_fail(double_num_params);
        ensure_parse_fail(param_list);
        break;
      case 1: // (n)
        ensure_parse_fail(no_params);
        ensure_parse_ok(single_num_param);
        check_type_cardinalities(types[i], column, 777, EMPTY_COLUMN_SCALE);
        ensure_parse_fail(double_num_params);
        ensure_parse_fail(param_list);
        break;
      case 2: // [(n)]
        ensure_parse_ok(no_params);
        check_type_cardinalities(types[i], column, EMPTY_COLUMN_PRECISION, EMPTY_COLUMN_SCALE);
        ensure_parse_ok(single_num_param);
        check_type_cardinalities(types[i], column, 777, EMPTY_COLUMN_SCALE);
        ensure_parse_fail(double_num_params);
        ensure_parse_fail(param_list);
        break;
      case 3: // (m, n)
        ensure_parse_fail(no_params);
        ensure_parse_fail(single_num_param);
        ensure_parse_ok(double_num_params);
        check_type_cardinalities(types[i], column, 111, 5);
        ensure_parse_fail(param_list);
        break;
      case 4: // (m[,n])
        ensure_parse_fail(no_params);
        ensure_parse_ok(single_num_param);
        check_type_cardinalities(types[i], column, 777, EMPTY_COLUMN_SCALE);
        ensure_parse_ok(double_num_params);
        check_type_cardinalities(types[i], column, 111, 5);
        ensure_parse_fail(param_list);
        break;
      case 5: // [(m,n)]
        ensure_parse_ok(no_params);
        check_type_cardinalities(types[i], column, EMPTY_COLUMN_PRECISION, EMPTY_COLUMN_SCALE);
        ensure_parse_fail(single_num_param);
        ensure_parse_ok(double_num_params);
        check_type_cardinalities(types[i], column, 111, 5);
        ensure_parse_fail(param_list);
        break;
      case 6: // [(m[,n])]
        ensure_parse_ok(no_params);
        check_type_cardinalities(types[i], column, EMPTY_COLUMN_PRECISION, EMPTY_COLUMN_SCALE);
        ensure_parse_ok(single_num_param);
        check_type_cardinalities(types[i], column, 777, EMPTY_COLUMN_SCALE);
        ensure_parse_ok(double_num_params);
        check_type_cardinalities(types[i], column, 111, 5);
        ensure_parse_fail(param_list);
        break;
      case 10: // ('a','b','c' ...)
        ensure_parse_fail(no_params);
        column->setParseType(param_list, tester.get_rdbms()->simpleDatatypes());

        // The following tests just check if the parameter list is properly stored.
        // No type checking takes place for now.
        grt::StringRef explicitParam= column->datatypeExplicitParams();
        ensure_equals("Parameter list not properly stored", *explicitParam, enum_parameters);
        break;
    }

    // This always must fail regardless of the actual type.
    // As currently no enum and set parsing is done we don't check invalid parameter lists for them.
    // TODO: Remove test for a specific parameter format once this has changed.
    if (types[i]->parameterFormatType() != 10)
      ensure_parse_fail(invalid_list);
  }
}


TEST_FUNCTION(25)
{
  // bug: make sure that mysql tables with a composite key have the auto_increment column
  // 1st in the index

  db_mysql_TableRef table(grtm->get_grt());
  db_mysql_ColumnRef col1(grtm->get_grt());
  db_mysql_ColumnRef col2(grtm->get_grt());

  table->name("table");

  col1->owner(table);
  col1->name("col1");
  table->columns().insert(col1);

  col2->owner(table);
  col2->name("col2");
  col2->autoIncrement(1);
  table->columns().insert(col2);

  table->addPrimaryKeyColumn(col1);
  table->addPrimaryKeyColumn(col2);

  ensure_equals("1st col in index is col2", *table->primaryKey()->columns().get(0)->referencedColumn()->name(), "col2");
}


// test comment splitter functions
TEST_FUNCTION(26)
{
  ensure_equals("split trunc part", 
    bec::TableHelper::get_sync_comment("hello world", 5), "hello");
  ensure_equals("split notrunc part", 
    bec::TableHelper::get_sync_comment("hello world", 15), "hello world");

   ensure("split trunc part", 
    bec::TableHelper::get_sync_comment("hell\xE2\x82\xAC world", 5).size() <= 5);

   ensure_equals("hell\xE2\x82\xAC world", 
    bec::TableHelper::get_sync_comment("hell\xE2\x82\xAC world", 5), "hell");
   
   ensure_equals("split trunc part", 
    bec::TableHelper::get_sync_comment("hello\nworld", 15), "hello");
}


// test full comment text generation (with quoting etc)
TEST_FUNCTION(27)
{
  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("hello world", 5), 
    "'hello' /* comment truncated */ /* world*/");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("hello world", 15), 
    "'hello world'");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("hello\nworld", 15), 
    "'hello' /* comment truncated */ /*world*/");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("hello wo'rld", 5), 
    "'hello' /* comment truncated */ /* wo'rld*/");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("hell' world", 5), 
    "'hell\\'' /* comment truncated */ /* world*/");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("h'llo world", 5), 
    "'h\\'llo' /* comment truncated */ /* world*/");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("h'llo /* a */", 5), 
    "'h\\'llo' /* comment truncated */ /* /* a *\\/*/");

  ensure_equals("comment", 
    bec::TableHelper::generate_comment_text("h'llo/* a */", 5), 
    "'h\\'llo' /* comment truncated */ /*/* a *\\/*/");
}


END_TESTS


