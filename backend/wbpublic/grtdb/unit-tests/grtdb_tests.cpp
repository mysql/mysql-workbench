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

#include "grt.h"

#include "grtdb/editor_table.h"
#include "grtdb/db_object_helpers.h"
#include "wb_helpers.h"

#include <boost/assign/list_of.hpp>
using namespace boost::assign;

using namespace grt;
using namespace bec;
using namespace std;

BEGIN_TEST_DATA_CLASS(grtdb_tests)
public:
WBTester *tester;

TEST_DATA_CONSTRUCTOR(grtdb_tests) {
  tester = new WBTester();
  populate_grt(*tester);
}
END_TEST_DATA_CLASS

TEST_MODULE(grtdb_tests, "DB stuff tests");

TEST_FUNCTION(2) {
  tester->create_new_document();
}

TEST_FUNCTION(5) {
  // test primary key

  db_mysql_TableRef table(grt::Initialized);
  table->name("tbl");

  db_mysql_ColumnRef column(grt::Initialized);

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

TEST_FUNCTION(10) {
  db_mysql_TableRef table(grt::Initialized);

  ensure_equals("index content type", table->indices().content_class_name(), "db.mysql.Index");
}

// Helper macros for column base types parser tests.
#define ensure_parse_ok(str) ensure(str, column->setParseType(str, tester->get_rdbms()->simpleDatatypes()) != 0);
#define ensure_parse_fail(str) ensure(str, column->setParseType(str, tester->get_rdbms()->simpleDatatypes()) == 0);

TEST_FUNCTION(15) {
  // Test some generally wrong cases. ml: testing invalid cases is just nonsense. Should be removed.
  db_ColumnRef column(grt::Initialized);

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
void check_type_cardinalities(db_SimpleDatatypeRef type, db_ColumnRef column, int precision, int scale) {
  if (type->numericPrecision() != EMPTY_TYPE_PRECISION) {
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
  } else {
    // If there's no numeric precision then check for character or octet cardinalities.
    if (type->characterMaximumLength() != EMPTY_TYPE_MAXIMUM_LENGTH ||
        type->characterOctetLength() != EMPTY_TYPE_OCTET_LENGTH)
      ensure_equals("Comparing char or octet length", *column->length(), precision);
    else
      ensure("Unexpected precision parameter found", *column->length() == EMPTY_COLUMN_LENGTH);
  }
}

/**
 *	 Data type parsing tests based on our rdbms info xml. Does additional checks, e.g. for cardinality,
 *	 but does not consider all possible data type definitions, to do a full parser test.
 */
TEST_FUNCTION(20) {
  // Go through all our defined datatypes and construct a column definition.
  // Then see if they all parse successfully.
  db_SchemaRef schema(grt::Initialized);

  db_CatalogRef catalog = tester->get_catalog();
  schema->owner(catalog);

  db_mysql_TableRef table(grt::Initialized);
  table->owner(schema);
  table->name("table");

  db_mysql_ColumnRef column(grt::Initialized);
  column->owner(table);
  column->name("testee");
  table->columns().insert(column);

  std::string expected_enum_parameters = "('blah', 'foo', 'bar', 0b11100011011, 0x1234ABCDE)";
  ListRef<db_SimpleDatatype> types = tester->get_rdbms()->simpleDatatypes();
  for (std::size_t i = 0; i < tester->get_rdbms()->simpleDatatypes().count(); i++) {
    // Try all parameter combinations.
    string no_params = types[i]->name();
    string single_num_param = no_params + "(777)";
    string double_num_params = no_params + "(111, 5)";
    string param_list = no_params + "('blah', 'foo'  ,       'bar'\n, \n0b11100011011,\n\n\n 0x1234ABCDE)";
    string invalid_list = no_params + "(1, a, 'bb')";

    // Depending on the server version a data type is defined for we need to set the
    // correct version or parsing will fail where it should succeed.
    std::string validity = types[i]->validity();
    ensure("Invalid data type validity", validity.empty() || validity.size() > 2);

    if (validity.empty())
      validity = "<5.7.9"; // Default is latest GA server at this time.

    std::size_t offset = 1;
    if (validity[1] == '=')
      ++offset;

    GrtVersionRef version = bec::parse_version(validity.substr(offset));

    // Convert the version so that we get one that matches the validity.
    switch (validity[0]) {
      case '<':
        if (version->buildNumber() > 0)
          version->buildNumber(version->buildNumber() - 1);
        else {
          if (version->buildNumber() > -1)
            version->buildNumber(99);
          if (version->releaseNumber() > 0)
            version->releaseNumber(version->releaseNumber() - 1);
          else {
            version->releaseNumber(99);
            if (version->minorNumber() > 0)
              version->minorNumber(version->minorNumber() - 1);
            else {
              version->minorNumber(99);
              version->majorNumber(version->majorNumber() - 1); // There's always a valid major number.
            }
          }
        }
        break;
      case '>':
        if (version->buildNumber() > 0)
          version->buildNumber(version->buildNumber() + 1);
        else {
          if (version->releaseNumber() > 0)
            version->releaseNumber(version->releaseNumber() + 1);
          else if (version->minorNumber() > -1)
            version->minorNumber(version->minorNumber() + 1);
          else
            version->majorNumber(version->majorNumber() + 1);
        }
        break;
    }
    catalog->version(version);

    // The parameter format type tells us which combination is valid.
    switch (types[i]->parameterFormatType()) {
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
        column->setParseType(param_list, tester->get_rdbms()->simpleDatatypes());

        // The following tests just check if the parameter list is properly stored.
        // No type checking takes place for now.
        grt::StringRef explicitParam = column->datatypeExplicitParams();
        ensure_equals("Parameter list not properly stored", *explicitParam, expected_enum_parameters);
        break;
    }

    // This always must fail regardless of the actual type.
    // As currently no enum and set parsing is done we don't check invalid parameter lists for them.
    // TODO: Remove test for a specific parameter format once this has changed.
    if (types[i]->parameterFormatType() != 10)
      ensure_parse_fail(invalid_list);
  }
}

/**
 *	 Another data type test, but with focus on all possible input and its proper handling,
 *	 even for corner cases.
 *	 Based on the MySQL grammar we construct here all possible input combinations.
 */

// Valid id string for unquoted identifiers.
static std::string special_id = "\xE2\x86\xB2\xE2\x86\xB3"; // ↲↳

// This is the node of which a sequence consists.
struct GrammarNode {
  bool is_terminal;
  bool is_required;  // false for * and ? operators, otherwise true.
  bool multiple;     // true for + and * operators, otherwise false.
  std::string value; // Either the text of a terminal or the name of a non-terminal.

  GrammarNode(bool _terminal, bool _required, bool _multiple, std::string _value)
    : is_terminal(_terminal), is_required(_required), multiple(_multiple), value(_value){};
};

// A sequence of grammar nodes (either terminal or non-terminal) in the order they appear in the grammar.
// Expressions in parentheses are extracted into an own rule with a private name.
// A sequence can have an optional predicate (min/max server version).
struct GrammarSequence {
  int min_version;
  int max_version;
  std::vector<GrammarNode> nodes;

  GrammarSequence(std::vector<GrammarNode> _nodes, int _min_version = MIN_SERVER_VERSION,
                  int _max_version = MAX_SERVER_VERSION)
    : min_version(_min_version), max_version(_max_version), nodes(_nodes){};
};

// A list of alternatives for a given rule.
typedef std::vector<GrammarSequence> RuleAlternatives;

static std::map<std::string, RuleAlternatives> rules = map_list_of<std::string, RuleAlternatives>
  // First the root rule. Everything starts with this.
  ("data_type",
   list_of<GrammarSequence>(list_of<GrammarNode>(false, true, false, "integer_type")(
                              false, false, false, "field_length")(false, false, false, "field_options")
                              .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(false, true, false, "real_literal")(false, false, false, "precision")(false, false, false,
                                                                                                "field_options")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "FLOAT")(false, false, false, "float_options")(false, false, false,
                                                                                            "field_options")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "BIT")(false, false, false, "field_length")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "BOOL").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "BOOLEAN").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "CHAR")(false, false, false, "field_length")(false, false, false,
                                                                                          "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(false, true, false, "nchar_literal")(false, false, false, "field_length")(true, false, false,
                                                                                                    "BINARY")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, " BINARY")(false, false, false, "field_length")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(false, true, false, "varchar_literal")(false, true, false, "field_length")(
       false, false, false, "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(false, true, false, "nvarchar_literal")(false, true, false, "field_length")(true, false,
                                                                                                      false, "BINARY")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "VARBINARY")(false, true, false, "field_length")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "YEAR")(false, false, false, "field_length")(false, false, false,
                                                                                          "field_options")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "DATE").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "TIME")(false, false, false, "type_datetime_precision")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "TIMESTAMP")(false, false, false, "type_datetime_precision")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "DATETIME")(false, false, false, "type_datetime_precision")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "TINYBLOB").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "BLOB")(false, false, false, "field_length")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "MEDIUMBLOB").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "LONGBLOB").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "LONG")(true, true, false, "VARBINARY")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "LONG")(false, false, false, "varchar_literal")(false, false, false,
                                                                                             "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "TINYTEXT")(false, false, false, "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "TEXT")(false, false, false, "field_length")(false, false, false,
                                                                                          "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "MEDIUMTEXT")(false, false, false, "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "LONGTEXT")(false, false, false, "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "DECIMAL")(false, false, false, "float_options")(false, false, false,
                                                                                              "field_options")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "NUMERIC")(false, false, false, "float_options")(false, false, false,
                                                                                              "field_options")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "FIXED")(false, false, false, "float_options")(false, false, false,
                                                                                            "field_options")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "ENUM")(false, true, false, "string_list")(false, false, false,
                                                                                        "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "SET")(false, true, false, "string_list")(false, false, false,
                                                                                       "string_binary")
       .convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "SERIAL").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(false, true, false, "spatial_type").convert_to_container<std::vector<GrammarNode> >()))

  // Rules referenced from the main rule or sub rules.
  ("integer_type",
   list_of<GrammarSequence>(
     list_of<GrammarNode>(true, true, false, "INTEGER").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "INT").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "INT1").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "INT2").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "INT3").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "INT4").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "INT8").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "TINYINT").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "SMALLINT").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "MEDIUMINT").convert_to_container<std::vector<GrammarNode> >())(
     list_of<GrammarNode>(true, true, false, "BIGINT").convert_to_container<std::vector<GrammarNode> >())
     .convert_to_container<std::vector<GrammarSequence> >())

    ("field_length", list_of<GrammarSequence>(
                       list_of<GrammarNode>(true, true, false, "(")(true, true, false, "6")(true, true, false, ")")
                         .convert_to_container<std::vector<GrammarNode> >())
                       .convert_to_container<std::vector<GrammarSequence> >())

      ("field_options", list_of<GrammarSequence>(list_of<GrammarNode>(false, true, true, "field_options_alt1")
                                                   .convert_to_container<std::vector<GrammarNode> >())
                          .convert_to_container<std::vector<GrammarSequence> >())

        ("field_options_alt1",
         list_of<GrammarSequence>(
           list_of<GrammarNode>(true, true, false, "SIGNED").convert_to_container<std::vector<GrammarNode> >())(
           list_of<GrammarNode>(true, true, false, "UNSIGNED").convert_to_container<std::vector<GrammarNode> >())(
           list_of<GrammarNode>(true, true, false, "ZEROFILL").convert_to_container<std::vector<GrammarNode> >())
           .convert_to_container<std::vector<GrammarSequence> >())

          ("real_literal",
           list_of<GrammarSequence>(
             list_of<GrammarNode>(true, true, false, "REAL").convert_to_container<std::vector<GrammarNode> >())(
             list_of<GrammarNode>(true, true, false, "DOUBLE")(true, false, false, "PRECISION")
               .convert_to_container<std::vector<GrammarNode> >())
             .convert_to_container<std::vector<GrammarSequence> >())

            ("precision",
             list_of<GrammarSequence>(list_of<GrammarNode>(true, true, false, "(")(true, true, false, "12")(
                                        true, true, false, ",")(true, true, false, "5")(true, true, false, ")")
                                        .convert_to_container<std::vector<GrammarNode> >())
               .convert_to_container<std::vector<GrammarSequence> >())

              ("float_options", list_of<GrammarSequence>(list_of<GrammarNode>(false, true, false, "float_options_alt1")
                                                           .convert_to_container<std::vector<GrammarNode> >())
                                  .convert_to_container<std::vector<GrammarSequence> >())

                ("float_options_alt1",
                 list_of<GrammarSequence>(list_of<GrammarNode>(true, true, false, "(")(true, true, false, "12")(
                                            false, false, false, "float_options_alt1_seq1")(true, true, false, ")")
                                            .convert_to_container<std::vector<GrammarNode> >())
                   .convert_to_container<std::vector<GrammarSequence> >())

                  ("float_options_alt1_seq1",
                   list_of<GrammarSequence>(list_of<GrammarNode>(true, true, false, ",")(true, true, false, "6")
                                              .convert_to_container<std::vector<GrammarNode> >())
                     .convert_to_container<std::vector<GrammarSequence> >())

                    ("string_binary",
                     list_of<GrammarSequence>(list_of<GrammarNode>(false, true, false, "ascii")
                                                .convert_to_container<std::vector<GrammarNode> >())(
                       list_of<GrammarNode>(false, true, false, "unicode")
                         .convert_to_container<std::vector<GrammarNode> >())(
                       list_of<GrammarNode>(true, true, false, "BYTE")
                         .convert_to_container<std::vector<GrammarNode> >())(
                       list_of<GrammarNode>(false, true, false, "charset")(false, true, false, "charset_name")(
                         true, true, false, "BINARY")
                         .convert_to_container<std::vector<GrammarNode> >())(
                       list_of<GrammarNode>(true, true, false, "BINARY")(false, false, false, "string_binary_seq1")
                         .convert_to_container<std::vector<GrammarNode> >())
                       .convert_to_container<std::vector<GrammarSequence> >())

                      ("string_binary_seq1",
                       list_of<GrammarSequence>(
                         list_of<GrammarNode>(false, true, false, "charset")(false, true, false, "charset_name")
                           .convert_to_container<std::vector<GrammarNode> >())
                         .convert_to_container<std::vector<GrammarSequence> >())

                        ("ascii",
                         list_of<GrammarSequence>(
                           list_of<GrammarNode>(true, true, false, "ASCII")(true, false, false, "BINARY")
                             .convert_to_container<std::vector<GrammarNode> >())(
                           list_of<GrammarNode>(true, true, false, "BINARY")(true, true, false, "ASCII"), 50500)
                           .convert_to_container<std::vector<GrammarSequence> >())

                          ("unicode",
                           list_of<GrammarSequence>(
                             list_of<GrammarNode>(true, true, false, "UNICODE")(true, false, false, "BINARY")
                               .convert_to_container<std::vector<GrammarNode> >())(
                             list_of<GrammarNode>(true, true, false, "BINARY")(true, true, false, "UNICODE"), 50500)
                             .convert_to_container<std::vector<GrammarSequence> >())

                            ("charset", list_of<GrammarSequence>(
                                          list_of<GrammarNode>(true, true, false, "CHAR")(true, true, false, "SET")
                                            .convert_to_container<std::vector<GrammarNode> >())(
                                          list_of<GrammarNode>(true, true, false, "CHARSET")
                                            .convert_to_container<std::vector<GrammarNode> >())
                                          .convert_to_container<std::vector<GrammarSequence> >())

                              ("charset_name",
                               list_of<GrammarSequence>(list_of<GrammarNode>(true, true, false, "'utf8'")
                                                          .convert_to_container<std::vector<GrammarNode> >())(
                                 list_of<GrammarNode>(true, true, false, "utf8")
                                   .convert_to_container<std::vector<GrammarNode> >())(
                                 list_of<GrammarNode>(true, true, false, "BINARY")
                                   .convert_to_container<std::vector<GrammarNode> >())
                                 .convert_to_container<std::vector<GrammarSequence> >())

                                ("text_or_identifier",
                                 list_of<GrammarSequence>(list_of<GrammarNode>(false, true, false, "string_literal")
                                                            .convert_to_container<std::vector<GrammarNode> >())(
                                   list_of<GrammarNode>(false, true, false, "identifier")
                                     .convert_to_container<std::vector<GrammarNode> >())
                                   .convert_to_container<std::vector<GrammarSequence> >())

                                  ("string_literal",
                                   list_of<GrammarSequence>(
                                     list_of<GrammarNode>(true, true, false, "n'text'")
                                       .convert_to_container<std::vector<GrammarNode> >()) // NCHAR_TEXT
                                   (list_of<GrammarNode>(true, false, false, "_utf8")(false, true, true,
                                                                                      "string_literal_seq1")
                                      .convert_to_container<std::vector<GrammarNode> >())
                                     .convert_to_container<std::vector<GrammarSequence> >())

                                    ("string_literal_seq1",
                                     list_of<GrammarSequence>(
                                       list_of<GrammarNode>(true, true, false, "'text'")
                                         .convert_to_container<std::vector<GrammarNode> >()) // SINGLE_QUOTED_TEXT
                                     (list_of<GrammarNode>(true, true, false, "\"text\"")
                                        .convert_to_container<std::vector<GrammarNode> >())
                                       .convert_to_container<std::vector<GrammarSequence> >() // DOUBLE_QUOTED_TEXT
                                     )

                                      ("identifier",
                                       list_of<GrammarSequence>(
                                         list_of<GrammarNode>(true, true, false, special_id)
                                           .convert_to_container<std::vector<GrammarNode> >()) // IDENTIFIER
                                       (list_of<GrammarNode>(true, true, false, "`identifier`")
                                          .convert_to_container<std::vector<GrammarNode> >()) // BACK_TICK_QUOTED_ID
                                       (list_of<GrammarNode>(true, true, false, "host")
                                          .convert_to_container<std::vector<GrammarNode> >())
                                         .convert_to_container<std::vector<GrammarSequence> >() // (certain) keywords
                                       )

                                        ("nchar_literal",
                                         list_of<GrammarSequence>(list_of<GrammarNode>(true, true, false, "NCHAR")
                                                                    .convert_to_container<std::vector<GrammarNode> >())(
                                           list_of<GrammarNode>(true, true, false, "NATIONAL\tCHAR")
                                             .convert_to_container<std::vector<GrammarNode> >())
                                           .convert_to_container<std::vector<GrammarSequence> >())

                                          ("varchar_literal", list_of<GrammarSequence>(
                                                                list_of<GrammarNode>(true, true, false, "CHAR")(
                                                                  true, true, false, "VARYING")
                                                                  .convert_to_container<std::vector<GrammarNode> >())(
                                                                list_of<GrammarNode>(true, true, false, "VARCHAR")
                                                                  .convert_to_container<std::vector<GrammarNode> >())
                                                                .convert_to_container<std::vector<GrammarSequence> >())

                                            ("nvarchar_literal",
                                             list_of<GrammarSequence>(
                                               list_of<GrammarNode>(true, true, false, "NATIONAL CHAR")(
                                                 true, true, false, "VARYING")
                                                 .convert_to_container<std::vector<GrammarNode> >())(
                                               list_of<GrammarNode>(true, true, false, "NVARCHAR")
                                                 .convert_to_container<std::vector<GrammarNode> >())(
                                               list_of<GrammarNode>(true, true, false, "NCHAR")(true, true, false,
                                                                                                "VARCHAR")
                                                 .convert_to_container<std::vector<GrammarNode> >())(
                                               list_of<GrammarNode>(true, true, false, "NATIONAL")(
                                                 true, true, false, "CHAR")(true, true, false, "VARYING")
                                                 .convert_to_container<std::vector<GrammarNode> >())(
                                               list_of<GrammarNode>(true, true, false, "NCHAR")(true, true, false,
                                                                                                "VARYING")
                                                 .convert_to_container<std::vector<GrammarNode> >())
                                               .convert_to_container<std::vector<GrammarSequence> >())

                                              ("type_datetime_precision",
                                               list_of<GrammarSequence>(
                                                 list_of<GrammarNode>(true, true, false, "(")(true, true, false, "6")(
                                                   true, true, false, ")"),
                                                 50600)
                                                 .convert_to_container<std::vector<GrammarSequence> >())

                                                ("string_list",
                                                 list_of<GrammarSequence>(
                                                   list_of<GrammarNode>(true, true, false, "(")(false, true,
                                                                                                false, "text_string")(
                                                     false, false, true, "string_list_seq1")(true, true, false, ")")
                                                     .convert_to_container<std::vector<GrammarNode> >())
                                                   .convert_to_container<std::vector<GrammarSequence> >())

                                                  ("string_list_seq1",
                                                   list_of<GrammarSequence>(
                                                     list_of<GrammarNode>(true, true, false, ",")(false, true, false,
                                                                                                  "text_string")
                                                       .convert_to_container<std::vector<GrammarNode> >())
                                                     .convert_to_container<std::vector<GrammarSequence> >())

                                                    ("text_string",
                                                     list_of<GrammarSequence>(
                                                       list_of<GrammarNode>(true, true, false, "'text'")
                                                         .convert_to_container<
                                                           std::vector<GrammarNode> >()) // SINGLE_QUOTED_TEXT
                                                     (list_of<GrammarNode>(true, true, false, "0x12345AABBCCDDEEFF")
                                                        .convert_to_container<std::vector<GrammarNode> >()) // HEXNUMBER
                                                     (list_of<GrammarNode>(true, true, false, "0b1000111101001011")
                                                        .convert_to_container<std::vector<GrammarNode> >())
                                                       .convert_to_container<
                                                         std::vector<GrammarSequence> >() // BITNUMBER
                                                     )

                                                      ("spatial_type",
                                                       list_of<GrammarSequence>(
                                                         list_of<GrammarNode>(true, true, false, "GEOMETRY")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "GEOMETRYCOLLECTION")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "POINT")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "MULTIPOINT")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "LINESTRING")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "MULTILINESTRING")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "POLYGON")
                                                           .convert_to_container<std::vector<GrammarNode> >())(
                                                         list_of<GrammarNode>(true, true, false, "MULTIPOLYGON")
                                                           .convert_to_container<std::vector<GrammarNode> >())
                                                         .convert_to_container<std::vector<GrammarSequence> >())

  ;

//--------------------------------------------------------------------------------------------------

std::vector<std::string> get_variations_for_rule(std::string rule_name);

std::vector<std::string> get_variations_for_sequence(const GrammarSequence &sequence) {
  std::vector<std::string> result;
  result.push_back(""); // Start with an empty entry to get the code rolling.

  // For each entry add its variations to each of the existing entries in the result.
  // If it is an optional entry duplicate existing entries and append the values to the duplicates
  // so we have one set with and one without the value.
  // For entries with multiple appearance add more duplicates with different repeat counts.
  for (std::vector<tut::GrammarNode>::const_iterator iterator = sequence.nodes.begin();
       iterator != sequence.nodes.end(); ++iterator) {
    std::vector<std::string> variations;
    if (iterator->is_terminal)
      variations.push_back(iterator->value); // Only one variation.
    else
      variations = get_variations_for_rule(iterator->value); // Potentially many variations.

    std::vector<std::string> intermediate;
    if (!iterator->is_required)
      intermediate.insert(intermediate.begin(), result.begin(), result.end());

    for (std::vector<std::string>::iterator result_iterator = result.begin(); result_iterator != result.end();
         ++result_iterator) {
      // Add each variation to each result we have so far already. This is the default occurrence.
      for (std::vector<std::string>::iterator variation_iterator = variations.begin();
           variation_iterator != variations.end(); ++variation_iterator) {
        if (result_iterator->empty())
          intermediate.push_back(*variation_iterator);
        else
          intermediate.push_back(*result_iterator + " " + *variation_iterator);
      }

      if (iterator->multiple) {
        // If there can be multiple occurrences create a cross product of all alternatives,
        // so we have at least 2 values in all possible combinations.
        for (std::vector<std::string>::iterator outer_iterator = variations.begin(); outer_iterator != variations.end();
             ++outer_iterator) {
          for (std::vector<std::string>::iterator inner_iterator = variations.begin();
               inner_iterator != variations.end(); ++inner_iterator) {
            if (result_iterator->empty())
              intermediate.push_back(*outer_iterator + " " + *inner_iterator);
            else
              intermediate.push_back(*result_iterator + " " + *outer_iterator + " " + *inner_iterator);
          }
        }
      }
    }

    // Finally the intermediate entries become now our result entries and each might get
    // one or more additional entries.
    result = intermediate;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

std::vector<std::string> get_variations_for_rule(std::string rule_name) {
  std::vector<std::string> result;

  std::map<std::string, tut::RuleAlternatives>::iterator rule = rules.find(rule_name);
  if (rule == rules.end()) {
    fail("Rule: " + rule_name + " not found");
    return result;
  }

  for (tut::RuleAlternatives::const_iterator iterator = rule->second.begin(); iterator != rule->second.end();
       ++iterator) {
    std::vector<std::string> values = get_variations_for_sequence(*iterator);
    result.insert(result.end(), values.begin(), values.end());
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

TEST_FUNCTION(22) {
  // First generate all possible combinations.
  std::vector<std::string> definitions = get_variations_for_rule("data_type");

  grt::ListRef<db_UserDatatype> user_types;
  grt::ListRef<db_SimpleDatatype> type_list = tester->get_catalog()->simpleDatatypes();

  // The latest version at the point of writing this, to include all possible variations.
  GrtVersionRef version(grt::Initialized);
  version->majorNumber(5);
  version->minorNumber(7);
  version->releaseNumber(4);
  version->buildNumber(-1);

  for (std::vector<std::string>::iterator iterator = definitions.begin(); iterator != definitions.end(); ++iterator) {
    db_SimpleDatatypeRef simple_type;
    db_UserDatatypeRef user_type;
    int precision;
    int scale;
    int length;
    std::string explicit_params;

    std::string sql = *iterator;
    ensure("Data type parsing failed for: \"" + sql + "\"",
           parse_type_definition(sql, version, type_list, user_types, type_list, simple_type, user_type, precision,
                                 scale, length, explicit_params));
  }
}

TEST_FUNCTION(25) {
  // bug: make sure that mysql tables with a composite key have the auto_increment column
  // 1st in the index

  db_mysql_TableRef table(grt::Initialized);
  db_mysql_ColumnRef col1(grt::Initialized);
  db_mysql_ColumnRef col2(grt::Initialized);

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
TEST_FUNCTION(26) {
  ensure_equals("split trunc part", bec::TableHelper::get_sync_comment("hello world", 5), "hello");
  ensure_equals("split notrunc part", bec::TableHelper::get_sync_comment("hello world", 15), "hello world");

  ensure("split trunc part", bec::TableHelper::get_sync_comment("hell\xE2\x82\xAC world", 5).size() <= 5);

  ensure_equals("hell\xE2\x82\xAC world", bec::TableHelper::get_sync_comment("hell\xE2\x82\xAC world", 5), "hell");

  ensure_equals("split trunc part", bec::TableHelper::get_sync_comment("hello\n\nworld", 15), "hello\n\nworld");

  ensure_equals("split trunc part, paragraph break", bec::TableHelper::get_sync_comment("hello\n\nworld long text", 15),
                "hello");
}

// test full comment text generation (with quoting etc)
TEST_FUNCTION(27) {
  ensure_equals("comment", bec::TableHelper::generate_comment_text("hello world", 5),
                "'hello' /* comment truncated */ /* world*/");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("hello world", 15), "'hello world'");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("hello\nworld", 12), "'hello\\nworld'");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("hello\n\nworld", 10),
                "'hello' /* comment truncated */ /*\nworld*/");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("hello wo'rld", 5),
                "'hello' /* comment truncated */ /* wo'rld*/");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("hell' world", 5),
                "'hell\\'' /* comment truncated */ /* world*/");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("h'llo world", 5),
                "'h\\'llo' /* comment truncated */ /* world*/");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("h'llo /* a */", 5),
                "'h\\'llo' /* comment truncated */ /* /* a *\\/*/");

  ensure_equals("comment", bec::TableHelper::generate_comment_text("h'llo/* a */", 5),
                "'h\\'llo' /* comment truncated */ /*/* a *\\/*/");
}

TEST_FUNCTION(30) {
  // test version checking utils
  ensure("5.5.0 supported", bec::is_supported_mysql_version("5.5.0"));
  ensure("5.6.5 supported", bec::is_supported_mysql_version("5.6.5"));
  ensure("3.14.15 not supported", !bec::is_supported_mysql_version("3.14.15"));
  ensure("5.5 supported", bec::is_supported_mysql_version("5.5"));
  ensure("6.6.6 not supported", !bec::is_supported_mysql_version("6.6.6"));

  ensure("5.5.5 vs 5.7.4", bec::is_supported_mysql_version_at_least(5, 7, 4, 5, 5, 5));
  ensure("5.10.5 vs 5.7.4", !bec::is_supported_mysql_version_at_least(5, 7, 4, 5, 10, 5));
  ensure("5.5.5 vs 5.5.4", !bec::is_supported_mysql_version_at_least(5, 5, 4, 5, 5, 5));
  ensure("5.5.5 vs 6.6.6", !bec::is_supported_mysql_version_at_least(5, 5, 5, 6, 6, 6));
}

// Due to the tut nature, this must be executed as a last test always,
// we can't have this inside of the d-tor.
TEST_FUNCTION(99) {
  delete tester;
}

END_TESTS
