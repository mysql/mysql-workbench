/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <mysql_connection.h>
#include <cppconn/metadata.h>

#include "grts/structs.h"
#include "grts/structs.db.mysql.h"
#include "base/log.h"
#include "grt.h"
#include "db_helpers.h"

#include "diff/diffchange.h"
#include "diff_dbobjectmatch.h"
#include <boost/assign/list_of.hpp>
#include <algorithm>
#include <functional>

#include "grtsqlparser/mysql_parser_services.h"

using namespace grt;

std::string get_qualified_schema_object_name(GrtNamedObjectRef object, const bool case_sensitive) {
  std::string s("`");
  s.append(object->owner()->name().c_str()).append("`.`").append(object->name().c_str()).append("`");
  return case_sensitive ? s : base::toupper(s);
}

std::string get_qualified_schema_object_old_name(GrtNamedObjectRef object, const bool case_sensitive) {
  const char* parent_name = NULL;
  if (db_mysql_SchemaRef::can_wrap(object->owner())) {
    parent_name = db_mysql_SchemaRef::cast_from(object->owner())->name().c_str();
  } else if (GrtNamedObjectRef::can_wrap(object->owner())) {
    GrtNamedObjectRef pr = GrtNamedObjectRef::cast_from(object->owner());

    parent_name = pr->oldName().empty() ? pr->name().c_str() : pr->oldName().c_str();
  } else {
    parent_name = object->owner()->name().c_str();
  }

  std::string s("`");
  s.append(parent_name).append("`.`").append(object->oldName().c_str()).append("`");

  return case_sensitive ? s : base::toupper(s);
}

/////////////////////////////////////////////////////////////////////////////////////////////
// Alter OMF
/////////////////////////////////////////////////////////////////////////////////////////////

bool grt::DbObjectMatchAlterOmf::less(const grt::ValueRef& l, const grt::ValueRef& r) const {
  if (l.type() == r.type() && l.type() == ObjectType) {
    if (db_IndexColumnRef::can_wrap(l) && db_IndexColumnRef::can_wrap(r)) {
      db_IndexColumnRef lc = db_IndexColumnRef::cast_from(l);
      db_IndexColumnRef rc = db_IndexColumnRef::cast_from(r);
      return less(lc->referencedColumn(), rc->referencedColumn());
    } else if (db_mysql_SchemaRef::can_wrap(l) && db_mysql_SchemaRef::can_wrap(r)) {
      return strcmp(db_mysql_SchemaRef::cast_from(l)->name().c_str(),
                    db_mysql_SchemaRef::cast_from(r)->name().c_str()) == 0;
    } else if (GrtNamedObjectRef::can_wrap(l) && GrtNamedObjectRef::can_wrap(r)) {
      GrtNamedObjectRef left = GrtNamedObjectRef::cast_from(l);
      GrtNamedObjectRef right = GrtNamedObjectRef::cast_from(r);

      if (left.is_valid() && right.is_valid()) {
        std::string l_str, r_str;

        if (strlen(left->oldName().c_str()) > 0)
          l_str = get_qualified_schema_object_old_name(left, case_sensitive);
        else
          l_str = get_qualified_schema_object_name(left, case_sensitive);

        if (strlen(right->oldName().c_str()) > 0)
          r_str = get_qualified_schema_object_old_name(right, case_sensitive);
        else
          r_str = get_qualified_schema_object_name(right, case_sensitive);

        int result = l_str.compare(r_str);

#ifdef _DB_OBJECT_MATCH_DEBUG_
        std::cout << std::endl
                  << "DbObjectMatchAlterOmf::less(" << l_str << ", " << r_str << ") = " << result << std::endl;
#endif
        return result < 0;
      }
    } else if (GrtObjectRef::can_wrap(l) && GrtObjectRef::can_wrap(r)) {
      GrtObjectRef left = GrtObjectRef::cast_from(l);
      GrtObjectRef right = GrtObjectRef::cast_from(r);

      if (left.is_valid() && right.is_valid()) {
        return (strcmp(left->name().c_str(), right->name().c_str()) < 0);
      }
    } else if (ObjectRef::can_wrap(l) && ObjectRef::can_wrap(r)) {
      ObjectRef left = ObjectRef::cast_from(l);
      ObjectRef right = ObjectRef::cast_from(r);
      if ((left.class_name() == right.class_name()) && left.has_member("oldName")) {
        const char* l_str = NULL;
        const char* r_str = NULL;

        if (strlen(left.get_string_member("oldName").c_str()) > 0)
          l_str = left.get_string_member("oldName").c_str();
        else
          l_str = left.get_string_member("name").c_str();

        if (strlen(right.get_string_member("oldName").c_str()) > 0)
          r_str = right.get_string_member("oldName").c_str();
        else
          r_str = right.get_string_member("name").c_str();

        return strcmp(l_str, r_str) < 0;
      }
    }
  }
  return std::less<grt::ValueRef>()(l, r);
}

bool grt::DbObjectMatchAlterOmf::equal(const ValueRef& l, const ValueRef& r) const {
  if (l.type() == r.type() && l.type() == ObjectType) {
    if (db_IndexColumnRef::can_wrap(l) && db_IndexColumnRef::can_wrap(r)) {
      db_IndexColumnRef lc = db_IndexColumnRef::cast_from(l);
      db_IndexColumnRef rc = db_IndexColumnRef::cast_from(r);
      return equal(lc->referencedColumn(), rc->referencedColumn());
    } else if (db_mysql_SchemaRef::can_wrap(l) && db_mysql_SchemaRef::can_wrap(r)) {
      return strcmp(db_mysql_SchemaRef::cast_from(l)->name().c_str(),
                    db_mysql_SchemaRef::cast_from(r)->name().c_str()) == 0;
    } else if (GrtNamedObjectRef::can_wrap(l) && GrtNamedObjectRef::can_wrap(r)) {
      GrtNamedObjectRef left = GrtNamedObjectRef::cast_from(l);
      GrtNamedObjectRef right = GrtNamedObjectRef::cast_from(r);

      if (left.is_valid() && right.is_valid()) {
        std::string l_str, r_str;

        if (strlen(left->oldName().c_str()) > 0)
          l_str = get_qualified_schema_object_old_name(left, case_sensitive);
        else
          l_str = get_qualified_schema_object_name(left, case_sensitive);

        if (strlen(right->oldName().c_str()) > 0)
          r_str = get_qualified_schema_object_old_name(right, case_sensitive);
        else
          r_str = get_qualified_schema_object_name(right, case_sensitive);

        int result = l_str.compare(r_str);

#ifdef _DB_OBJECT_MATCH_DEBUG_
        std::cout << std::endl
                  << "DbObjectMatchAlterOmf::equal(" << l_str << ", " << r_str << ") = " << result << std::endl;
#endif
        return result == 0;
      }
    } else if (GrtObjectRef::can_wrap(l) && GrtObjectRef::can_wrap(r)) {
      GrtObjectRef left = GrtObjectRef::cast_from(l);
      GrtObjectRef right = GrtObjectRef::cast_from(r);

      if (left.is_valid() && right.is_valid()) {
        return (strcmp(left->name().c_str(), right->name().c_str()) == 0);
      }
    } else if (ObjectRef::can_wrap(l) && ObjectRef::can_wrap(r)) {
      ObjectRef left = ObjectRef::cast_from(l);
      ObjectRef right = ObjectRef::cast_from(r);
      if (left.is_valid() && right.is_valid() && (left.class_name() == right.class_name()) &&
          left.has_member("oldName")) {
        ObjectRef left = ObjectRef::cast_from(l);
        ObjectRef right = ObjectRef::cast_from(r);

        const char* l_str = NULL;
        const char* r_str = NULL;

        if (strlen(left.get_string_member("oldName").c_str()) > 0)
          l_str = left.get_string_member("oldName").c_str();
        else
          l_str = left.get_string_member("name").c_str();

        if (strlen(right.get_string_member("oldName").c_str()) > 0)
          r_str = right.get_string_member("oldName").c_str();
        else
          r_str = right.get_string_member("name").c_str();

        return strcmp(l_str, r_str) == 0;
      }
    }
  }

  return std::equal_to<grt::ValueRef>()(l, r);
}

//--------------------------------------------------------------------------------------------------

bool sqlCompare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  // views are compared by sqlDefinition
  if (!db_ViewRef::can_wrap(obj1)) {
    std::string sql1 = ObjectRef::cast_from(obj1).get_string_member(name);
    std::string sql2 = ObjectRef::cast_from(obj2).get_string_member(name);
    SqlFacade* parser = SqlFacade::instance_for_rdbms_name("Mysql");

    if (!parser)
      return false;
    std::string schema1 = db_TriggerRef::can_wrap(obj1) ? GrtObjectRef::cast_from(obj1)->owner()->owner()->name()
                                                        : GrtObjectRef::cast_from(obj1)->owner()->name();
    std::string schema2 = db_TriggerRef::can_wrap(obj2) ? GrtObjectRef::cast_from(obj2)->owner()->owner()->name()
                                                        : GrtObjectRef::cast_from(obj2)->owner()->name();
    sql1 = parser->normalizeSqlStatement(sql1, schema1);
    sql2 = parser->normalizeSqlStatement(sql2, schema2);
    return sql1 == sql2;
  } else
    return true; // consider it as always matching
}

//--------------------------------------------------------------------------------------------------

bool caseless_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name,
                      const std::string& default_name) {
  std::string str1 = base::toupper(ObjectRef::cast_from(obj1).get_string_member(name));
  std::string str2 = base::toupper(ObjectRef::cast_from(obj2).get_string_member(name));
  if (str1 == default_name)
    str1 = "";
  if (str2 == default_name)
    str2 = "";
  return str1 == str2;
}

bool caseless_compare_arr(const ValueRef obj1, const ValueRef obj2, const std::string& name,
                          const std::vector<std::string>& default_names) {
  std::string str1 = base::toupper(ObjectRef::cast_from(obj1).get_string_member(name));
  std::string str2 = base::toupper(ObjectRef::cast_from(obj2).get_string_member(name));
  if (std::find(default_names.begin(), default_names.end(), str1) != default_names.end())
    str1 = "";
  if (std::find(default_names.begin(), default_names.end(), str2) != default_names.end())
    str2 = "";
  return str1 == str2;
}

bool charset_collation_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  std::string sql1 = ObjectRef::cast_from(obj1).get_string_member(name);
  std::string sql2 = ObjectRef::cast_from(obj2).get_string_member(name);

  if (db_ColumnRef::can_wrap(obj1)) {
    db_ColumnRef col1(db_ColumnRef::cast_from(obj1));
    db_ColumnRef col2(db_ColumnRef::cast_from(obj2));
    std::string table_fieldname(name == "characterSetName" ? "defaultCharacterSetName" : "defaultCollationName");

    // if ASCII flag is set, the server will treat the column as having charset latin1
    if (sql1.empty() && col1->flags().get_index("ASCII") != grt::BaseListRef::npos)
      sql1 = "latin1";
    if (sql2.empty() && col2->flags().get_index("ASCII") != grt::BaseListRef::npos)
      sql2 = "latin1";

    // Those two need to be here cause if sql1 or sql2 is empty then we should take the value from the owner.
    // It's needed when we have collation different than default in the table, and default column collation.
    if (sql1.empty())
      sql1 = col1->owner().get_string_member(table_fieldname); // not sure if this is supposed to be col1
    if (sql2.empty())
      sql2 = col2->owner().get_string_member(table_fieldname);

    if (name == "collationName") {
      if (sql1.empty()) {
        sql1 = col1->characterSetName();
        if (sql1.empty())
          sql1 = col1->owner().get_string_member("defaultCharacterSetName");
      }

      // if BINARY flag is set, the table's collation is replaced with the _bin variant of its charset
      if (col1->flags().get_index("BINARY") != grt::BaseListRef::npos) {
        size_t p = sql1.find('_');
        if (p != std::string::npos)
          sql1 = sql1.substr(0, p) + "_bin";
      }

      if (sql2.empty()) {
        sql2 = col2->characterSetName();
        if (sql2.empty())
          sql2 = col2->owner().get_string_member("defaultCharacterSetName");
      }

      if (col2->flags().get_index("BINARY") != grt::BaseListRef::npos) {
        size_t p = sql2.find('_');
        if (p != std::string::npos)
          sql2 = sql2.substr(0, p) + "_bin";
      }
    }

    if ((sql1 == sql2) || sql1.empty() || sql2.empty())
      return true;
  } else if (db_TableRef::can_wrap(obj1)) {
    if (name == "defaultCollationName") {
      if (sql1.empty()) {
        db_TableRef table1(db_TableRef::cast_from(obj1));
        ValueRef defCharset = table1->get_member("defaultCharacterSetName");
        if (StringRef::can_wrap(defCharset)) {
          sql1 = bec::get_default_collation_for_charset(table1, StringRef::cast_from(defCharset));
        }
      }
      if (sql2.empty()) {
        db_TableRef table2(db_TableRef::cast_from(obj2));
        ValueRef defCharset = table2->get_member("defaultCharacterSetName");
        if (StringRef::can_wrap(defCharset)) {
          sql2 = bec::get_default_collation_for_charset(table2, StringRef::cast_from(defCharset));
        }
      }
    }
  } else if (db_SchemaRef::can_wrap(obj1)) {
    if (name == "defaultCollationName") {
      if (sql1.empty()) {
        db_SchemaRef schema1(db_SchemaRef::cast_from(obj1));
        sql1 = bec::get_default_collation_for_charset(schema1, schema1->defaultCharacterSetName());
      }
      if (sql2.empty()) {
        db_SchemaRef schema2(db_SchemaRef::cast_from(obj2));
        sql2 = bec::get_default_collation_for_charset(schema2, schema2->defaultCharacterSetName());
      }
    }
  } else {
    if (sql1.empty() || sql2.empty())
      return true;
  }
  return sql1 == sql2;
}

bool fk_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  // Here we do not compare the ability for engines to support foreign keys but
  // check if at both engines does not. This can be used to optimize further FK handling
  // (no need to check all the FKs then).
  grt::StringRef ename = db_mysql_TableRef::cast_from(obj1)->tableEngine();
  db_mysql_StorageEngineRef engine = bec::TableHelper::get_engine_by_name(ename);

  //          if (!engine.is_valid() || !engine->supportsForeignKeys())
  //            return true;

  ename = db_mysql_TableRef::cast_from(obj2)->tableEngine();
  db_mysql_StorageEngineRef engine2 = bec::TableHelper::get_engine_by_name(ename);

  if ((engine.is_valid() && !engine->supportsForeignKeys()) && (engine2.is_valid() && !engine2->supportsForeignKeys()))
    return true;

  return false;
}

bool formatted_type_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  std::string sql1 = ObjectRef::cast_from(obj1).get_string_member(name);
  std::string sql2 = ObjectRef::cast_from(obj2).get_string_member(name);
  SqlFacade* parser = SqlFacade::instance_for_rdbms_name("Mysql");

  if (!parser)
    return false;

  sql1 = parser->removeInterTokenSpaces(sql1);
  sql2 = parser->removeInterTokenSpaces(sql2);
  //        if (sql1 != sql2)
  //        std::cout<<"============"<<sql1<<std::endl<<std::endl<<sql2<<"==========================="<<std::endl;
  return sql1 == sql2;
}

bool sql_definition_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  if (db_ViewRef::can_wrap(obj1)) {
    // for views, we can't directly compare the model definition with the server one, because they will almost never
    // match (the server modifies the view code)
    // so, we keep a snapshot of the model and the last known server version of the code and compare against these
    // instead
    // the snapshots are updated after the sync/reveng/fwdeng is done
    db_ViewRef view1(db_ViewRef::cast_from(obj1));
    db_ViewRef view2(db_ViewRef::cast_from(obj2));

    if (!(*view2->oldModelSqlDefinition()).empty() && (*view1->oldModelSqlDefinition()).empty()) {
      // if view2 has the oldModelSqlDefinition field set and view1 doesn't, the model vs server
      // assumption may be inverted
      db_ViewRef tmp(view1);
      view1 = view2;
      view2 = tmp;
    }
    if (view1->sqlDefinition() == view1->oldModelSqlDefinition() &&
        view1->oldServerSqlDefinition() == view2->sqlDefinition())
      return true;

    return false;
  } else {
    if (!db_DatabaseDdlObjectRef::can_wrap(obj1))
      return false;
    db_DatabaseDdlObjectRef dbobj1 = db_DatabaseDdlObjectRef::cast_from(obj1);
    db_DatabaseDdlObjectRef dbobj2 = db_DatabaseDdlObjectRef::cast_from(obj2);
    size_t alg1 = dbobj1.has_member("algorithm") ? dbobj1.get_integer_member("algorithm") : 0;
    size_t alg2 = dbobj2.has_member("algorithm") ? dbobj2.get_integer_member("algorithm") : 0;
    return sqlCompare(obj1, obj2, "sqlDefinition") && (alg1 == alg2) &&
           (caseless_compare(obj1, obj2, "definer", "ROOT`@`LOCALHOST"));
  }
}

//--------------------------------------------------------------------------------------------------

bool ignore_index_col_name(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  if (ObjectRef::cast_from(obj1).is_instance("db.IndexColumn") &&
      ObjectRef::cast_from(obj2).is_instance("db.IndexColumn") &&
      StringRef::can_wrap(ObjectRef::cast_from(obj1).get_member(name)) &&
      StringRef::can_wrap(ObjectRef::cast_from(obj2).get_member(name)))
    return true;
  return false;
}

std::string trim_zeros(const std::string& str) {
  if (str.empty())
    return str;
  size_t pos = str.find_first_not_of("0");
  if (pos == std::string::npos) // there is only zeroes so return "0"
    return std::string("0");
  // Check for 0.000000
  if ((str[pos] == '.') && (str.find_first_not_of("0", pos + 1) == std::string::npos))
    return std::string("0");
  if (pos == 0) // no leading zeroes return unaltered string
    return str;
  return str.substr(pos);
}

std::string fixDefalutString(const std::string& str) {
  if (str.empty())
    return str;
  if (str == std::string("0000-00-00 00:00:00"))
    return std::string("");
  // don't know wtf is this for, but it causes bug #68341
  //  if (strcasecmp(str.c_str(),"CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP") == 0) return
  //  std::string("CURRENT_TIMESTAMP");
  if (str == std::string("NOW()"))
    return std::string("CURRENT_TIMESTAMP");
  if (str == std::string("CURRENT_TIMESTAMP()"))
    return std::string("CURRENT_TIMESTAMP");
  //  if (str == std::string("CURRENT_TIMESTAMP")) return std::string("CURRENT_TIMESTAMP");
  if (str == std::string("LOCALTIME()"))
    return std::string("CURRENT_TIMESTAMP");
  if (str == std::string("LOCALTIME"))
    return std::string("CURRENT_TIMESTAMP");
  if (str == std::string("LOCALTIMESTAMP"))
    return std::string("CURRENT_TIMESTAMP");
  if (str == std::string("LOCALTIMESTAMP()"))
    return std::string("CURRENT_TIMESTAMP");
  if (str == std::string("TRUE"))
    return std::string("1");
  if (str == std::string("FALSE"))
    return std::string("");
  //  if (strcasecmp(str.c_str(),"NULL") == 0) return std::string("");
  return trim_zeros(str);
};
// if(name1 == "defaultValue")
bool default_value_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  std::string s1 = ObjectRef::cast_from(obj1).get_string_member(name);
  std::string s2 = ObjectRef::cast_from(obj2).get_string_member(name);
  s1.erase(std::remove_if(s1.begin(), s1.end(), std::bind(std::equal_to<std::string::value_type>(), 
    std::placeholders::_1, '\'')), s1.end());
  s2.erase(std::remove_if(s2.begin(), s2.end(), std::bind(std::equal_to<std::string::value_type>(),
    std::placeholders::_1 , '\'')), s2.end());
  s1 = fixDefalutString(s1);
  s2 = fixDefalutString(s2);
  return s1 == s2;
}

bool name_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  // db_ColumnRef name and old name should be always compared with case!
  if (db_ColumnRef::can_wrap(obj1))
    return false;
  std::string str1 = ObjectRef::cast_from(obj1).get_string_member(name);
  std::string str2 = ObjectRef::cast_from(obj2).get_string_member(name);

  // toupper is very expensive operation attempting to avoid that
  if (str1 == str2) // Strings are equal even without toupper
    return true;
  if (str1.length() != str2.length()) // strings have different length thus aren't equal
    return false;
  // Finally perform caseless comparison
  str1 = base::toupper(str1);
  str2 = base::toupper(str2);
  return str1 == str2;
}

bool ref_table_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  std::string str1 = db_mysql_ForeignKeyRef::cast_from(obj1)->referencedTable().is_valid()
                       ? base::toupper(db_mysql_ForeignKeyRef::cast_from(obj1)->referencedTable()->name())
                       : "";
  std::string str2 = db_mysql_ForeignKeyRef::cast_from(obj2)->referencedTable().is_valid()
                       ? base::toupper(db_mysql_ForeignKeyRef::cast_from(obj2)->referencedTable()->name())
                       : "";
  return str1 == str2;
}

grt::NormalizedComparer::NormalizedComparer(const grt::DictRef options) {
  if (options.is_valid()) {
    _case_sensitive = options.get_int("CaseSensitive") != 0;
    _skip_routine_definer = options.get_int("SkipRoutineDefiner") != 0;
    _maxTableCommentLength = (int)options.get_int("maxTableCommentLength");
    _maxIndexCommentLength = (int)options.get_int("maxIndexCommentLength");
    _maxColumnCommentLength = (int)options.get_int("maxColumnCommentLength");
    load_rules();

  } else {
    _skip_routine_definer = false;
    _case_sensitive = true;
    _maxTableCommentLength = 60;
    _maxIndexCommentLength = 0;
    _maxColumnCommentLength = 255;
  }

  load_rules();
};

bool grt::NormalizedComparer::comment_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) const {
  std::string str1 = ObjectRef::cast_from(obj1).get_string_member(name);
  std::string str2 = ObjectRef::cast_from(obj2).get_string_member(name);
  int comment_len = 60;
  if (ObjectRef::cast_from(obj1).is_instance("db.IndexColumn"))
    comment_len = _maxIndexCommentLength;
  else if (ObjectRef::cast_from(obj1).is_instance("db.Table"))
    comment_len = _maxTableCommentLength;
  else if (ObjectRef::cast_from(obj1).is_instance("db.Column"))
    comment_len = _maxColumnCommentLength;

  str1 = bec::TableHelper::get_sync_comment(str1, comment_len);
  str2 = bec::TableHelper::get_sync_comment(str2, comment_len);
  if (db_mysql_SchemaRef::can_wrap(obj1))
    return true;
  return str1 == str2;
}

bool supports_autoincement(const db_ColumnRef& column) {
  db_SimpleDatatypeRef columnType;

  // Determine actually used column type first.
  if (column->userType().is_valid() && column->userType()->actualType().is_valid())
    columnType = column->userType()->actualType();
  else if (column->simpleType().is_valid() && column->simpleType()->group().is_valid())
    columnType = column->simpleType();

  return (columnType.is_valid() && columnType->group().is_valid() &&
          !strcmp(columnType->group()->name().c_str(), "numeric"));
}
bool autoincrement_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  if (db_ColumnRef::can_wrap(obj1)) {
    db_ColumnRef col1 = db_ColumnRef::cast_from(obj1);
    db_ColumnRef col2 = db_ColumnRef::cast_from(obj2);
    return !supports_autoincement(col1) || !supports_autoincement(col2);
  }
  return false;
}

bool default_int_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  ssize_t i1 = ObjectRef::cast_from(obj1).get_integer_member(name);
  ssize_t i2 = ObjectRef::cast_from(obj2).get_integer_member(name);
  return (i1 == -1) || (i2 == -1);
}

bool returnDatatype_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  if (!db_mysql_RoutineRef::can_wrap(obj1) || !db_mysql_RoutineRef::can_wrap(obj2))
    return false;
  db_mysql_RoutineRef r1 = db_mysql_RoutineRef::cast_from(obj1);
  db_mysql_RoutineRef r2 = db_mysql_RoutineRef::cast_from(obj2);
  grt::ListRef<db_UserDatatype> user_types1;
  grt::ListRef<db_SimpleDatatype> default_type_list1;

  db_SchemaRef schema1 = db_SchemaRef::cast_from(r1->owner());
  db_CatalogRef catalog1;
  if (schema1.is_valid() && schema1->owner().is_valid()) {
    catalog1 = db_CatalogRef::cast_from(schema1->owner());
    user_types1 = catalog1->userDatatypes();
    default_type_list1 = catalog1->simpleDatatypes();
  }

  grt::ListRef<db_SimpleDatatype> types1;
  if (catalog1.is_valid() && catalog1->owner() /*phys.model*/.is_valid())
    types1 = db_mgmt_RdbmsRef::cast_from(catalog1->owner()->get_member("rdbms"))->simpleDatatypes();
  db_UserDatatypeRef userType1;
  db_SimpleDatatypeRef simpleType1;
  int precision1 = bec::EMPTY_COLUMN_PRECISION;
  int scale1 = bec::EMPTY_COLUMN_SCALE;
  int length1 = bec::EMPTY_COLUMN_LENGTH;
  std::string datatypeExplicitParams1;
  std::string type1 = r1->returnDatatype();

  GrtVersionRef version1;
  if (catalog1.is_valid())
    version1 = catalog1->version();
  parsers::MySQLParserServices *services = parsers::MySQLParserServices::get();
  if (!services->parseTypeDefinition(type1, version1, types1, user_types1, default_type_list1,
      simpleType1, userType1, precision1, scale1, length1, datatypeExplicitParams1))
      return false;

  grt::ListRef<db_UserDatatype> user_types2;
  grt::ListRef<db_SimpleDatatype> default_type_list2;

  db_SchemaRef schema2 = db_SchemaRef::cast_from(r2->owner());
  db_CatalogRef catalog2;
  if (schema2.is_valid() && schema2->owner().is_valid()) {
    catalog2 = db_CatalogRef::cast_from(schema2->owner());
    user_types2 = catalog2->userDatatypes();
    default_type_list2 = catalog2->simpleDatatypes();
  }

  grt::ListRef<db_SimpleDatatype> types2;
  if (catalog2.is_valid() && catalog2->owner() /*phys.model*/.is_valid())
    types2 = db_mgmt_RdbmsRef::cast_from(catalog2->owner()->get_member("rdbms"))->simpleDatatypes();
  db_UserDatatypeRef userType2;
  db_SimpleDatatypeRef simpleType2;
  int precision2 = bec::EMPTY_COLUMN_PRECISION;
  int scale2 = bec::EMPTY_COLUMN_SCALE;
  int length2 = bec::EMPTY_COLUMN_LENGTH;
  std::string datatypeExplicitParams2;
  std::string type2 = r1->returnDatatype();

  GrtVersionRef version2;
  if (catalog2.is_valid())
    version2 = catalog1->version();
  if (!services->parseTypeDefinition(type2, version2, types2, user_types2, default_type_list2,
      simpleType2, userType2, precision2, scale2, length2, datatypeExplicitParams2))
      return false;

  return (simpleType1 == simpleType2) && (userType1 == userType2) && (precision1 == precision2) && (scale1 == scale2) &&
         (length1 = length2) && (datatypeExplicitParams1 == datatypeExplicitParams2);
}

bool datatypeExplicitParams_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  db_ColumnRef col1 = db_ColumnRef::cast_from(obj1);
  db_ColumnRef col2 = db_ColumnRef::cast_from(obj2);

  if (col1->simpleType().is_valid() && col2->simpleType().is_valid() &&
      ((col1->simpleType()->name() == "ENUM" && col2->simpleType()->name() == "ENUM") ||
       (col1->simpleType()->name() == "SET" && col2->simpleType()->name() == "SET"))) {
    std::string value1 = col1->get_string_member(name);
    std::string value2 = col2->get_string_member(name);
    if (value1 == value2)
      return true;
    if (value1.empty() || value2.empty())
      return false;

    if (value1[0] == '(' && value1[value1.size() - 1] == ')' && value2[0] == '(' && value2[value2.size() - 1] == ')') {
      // if the values don't match it could be because there's a space after each comma
      std::vector<std::string> tokens1 = base::split_token_list(value1.substr(1, value1.size() - 2), ',');
      std::vector<std::string> tokens2 = base::split_token_list(value2.substr(1, value2.size() - 2), ',');
      if (tokens1.size() == tokens2.size()) {
        for (size_t c = tokens1.size(), i = 0; i < c; i++)
          if (tokens1[i] != tokens2[i])
            return false;
        return true;
      }
    }
    return false;
  }
  return false;
}

inline std::vector<std::string> from_stringlist(const grt::StringListRef& slist) {
  std::vector<std::string> s;
  for (size_t i = 0; i < slist.count(); i++)
    s.push_back(*slist[i]);
  return s;
}

static bool has_item(std::vector<std::string>& l, const std::string& s) {
  return std::find(l.begin(), l.end(), s) != l.end();
}

static void remove_item(std::vector<std::string>& l, const std::string& s) {
  std::vector<std::string>::iterator it = std::find(l.begin(), l.end(), s);
  if (it != l.end())
    l.erase(it);
}

static bool column_flags_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  if (db_ColumnRef::can_wrap(obj1)) {
    db_ColumnRef col1 = db_ColumnRef::cast_from(obj1);
    db_ColumnRef col2 = db_ColumnRef::cast_from(obj2);
    std::vector<std::string> flags1;
    std::vector<std::string> flags2;

    if (col1->simpleType().is_valid())
      flags1 = from_stringlist(col1->flags());
    else if (col1->userType().is_valid())
      flags1 = base::split(col1->userType()->flags(), ",");

    if (col2->simpleType().is_valid())
      flags2 = from_stringlist(col2->flags());
    else if (col2->userType().is_valid())
      flags2 = base::split(col2->userType()->flags(), ",");

    // if ZEROFILL is specified, UNSIGNED will tag along
    if (has_item(flags1, "ZEROFILL") && !has_item(flags1, "UNSIGNED"))
      flags1.push_back("UNSIGNED");
    if (has_item(flags2, "ZEROFILL") && !has_item(flags2, "UNSIGNED"))
      flags2.push_back("UNSIGNED");

    // ASCII and BINARY flags are compared in the characterSet comparer, so remove them from here
    remove_item(flags1, "ASCII");
    remove_item(flags2, "ASCII");
    remove_item(flags1, "BINARY");
    remove_item(flags2, "BINARY");

    if (flags1.size() != flags2.size())
      return false;

    std::sort(flags1.begin(), flags1.end());
    std::sort(flags2.begin(), flags2.end());
    for (std::vector<std::string>::const_iterator i = flags1.begin(), j = flags2.begin(); i != flags1.end(); ++i, ++j) {
      if (*i != *j)
        return false;
    }
    return true;
  }
  return false;
}

static bool table_name_list_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) {
  db_TableRef table1(db_TableRef::cast_from(obj1));
  db_TableRef table2(db_TableRef::cast_from(obj2));

  if (bec::TableHelper::normalize_table_name_list(table1->owner()->name(), table1->get_string_member(name)) ==
      bec::TableHelper::normalize_table_name_list(table2->owner()->name(), table2->get_string_member(name)))
    return true;
  return false;
}

void grt::NormalizedComparer::load_rules() {
  static const std::vector<std::string> rules_defaults = {"RESTRICT"};
  rules.clear();
  rules["owner"].push_back(std::bind([]() { return true; }));

  if (!_case_sensitive) {
    rules["name"].push_back(
      std::bind(&name_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    rules["oldName"].push_back(
      std::bind(&name_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    rules["referencedTable"].push_back(
      std::bind(&ref_table_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  }

  // For IndexColumn names should be ignored
  rules["name"].push_back(
    std::bind(&ignore_index_col_name, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  rules["rowFormat"].push_back(
    std::bind(&caseless_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, "DEFAULT"));
  rules["packKeys"].push_back(
    std::bind(&caseless_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, "DEFAULT"));
  rules["tableEngine"].push_back(
    std::bind(&caseless_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ""));
  rules["returnDatatype"].push_back(
    std::bind(&caseless_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, ""));
  rules["deleteRule"].push_back(std::bind(&caseless_compare_arr, std::placeholders::_1, std::placeholders::_2,
                                          std::placeholders::_3, rules_defaults));
  rules["updateRule"].push_back(std::bind(&caseless_compare_arr, std::placeholders::_1, std::placeholders::_2,
                                          std::placeholders::_3, rules_defaults));
  rules["mergeUnion"].push_back(
    std::bind(&table_name_list_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["comment"].push_back(std::bind(&grt::NormalizedComparer::comment_compare, this, std::placeholders::_1,
                                       std::placeholders::_2, std::placeholders::_3));
  rules["collationName"].push_back(
    std::bind(&charset_collation_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["defaultCollationName"].push_back(
    std::bind(&charset_collation_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["characterSetName"].push_back(
    std::bind(&charset_collation_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["defaultCharacterSetName"].push_back(
    std::bind(&charset_collation_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["foreignKeys"].push_back(
    std::bind(&fk_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["formattedRawType"].push_back(
    std::bind(&formatted_type_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["formattedType"].push_back(
    std::bind(&formatted_type_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["sqlDefinition"].push_back(
    std::bind(&sql_definition_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["precision"].push_back(
    std::bind(&default_int_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["length"].push_back(
    std::bind(&default_int_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["definer"].push_back(std::bind(&caseless_compare, std::placeholders::_1, std::placeholders::_2,
                                       std::placeholders::_3, "ROOT`@`LOCALHOST"));
  rules["defaultValue"].push_back(
    std::bind(&default_value_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["autoIncrement"].push_back(
    std::bind(&autoincrement_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["returnDatatype"].push_back(
    std::bind(&returnDatatype_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
  rules["datatypeExplicitParams"].push_back(
    std::bind(&datatypeExplicitParams_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

  rules["flags"].push_back(
    std::bind(&column_flags_compare, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
};

bool grt::NormalizedComparer::normalizedComparison(const ValueRef obj1, const ValueRef obj2, const std::string name) {
  std::list<comparison_rule>& rul_list = rules[name];
  for (std::list<comparison_rule>::iterator It = rul_list.begin(); It != rul_list.end(); ++It)
    if ((*It)(obj1, obj2, name))
      return true;
  return false;
};

void grt::NormalizedComparer::init_omf(Omf* omf) {
  omf->case_sensitive = _case_sensitive;
  omf->skip_routine_definer = _skip_routine_definer;
  omf->normalizer = std::bind(&NormalizedComparer::normalizedComparison, this, std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3);
};

// TODO: This shouldn't be here but rather in DBPlugin, but QE doesn't use that
void grt::NormalizedComparer::load_db_options(sql::DatabaseMetaData* dbc_meta) {
  _case_sensitive = dbc_meta->storesMixedCaseIdentifiers();
  const unsigned int major = dbc_meta->getDatabaseMajorVersion();
  const unsigned int minor = dbc_meta->getDatabaseMinorVersion();
  const unsigned int revision = dbc_meta->getDatabasePatchVersion();
  if (bec::is_supported_mysql_version_at_least(major, minor, revision, 5, 5, 5)) {
    _maxTableCommentLength = 2048;
    _maxIndexCommentLength = 1024;
    _maxColumnCommentLength = 1024;
  } else {
    _maxTableCommentLength = 60;
    _maxIndexCommentLength = 0;
    _maxColumnCommentLength = 255;
  }
  load_rules();
};

grt::DictRef grt::NormalizedComparer::get_options_dict() const {
  grt::DictRef result(true);
  result.set("CaseSensitive", grt::IntegerRef(_case_sensitive));
  result.set("SkipRoutineDefiner", grt::IntegerRef(_skip_routine_definer));
  result.set("maxTableCommentLength", grt::IntegerRef(_maxTableCommentLength));
  result.set("maxIndexCommentLength", grt::IntegerRef(_maxIndexCommentLength));
  result.set("maxColumnCommentLength", grt::IntegerRef(_maxColumnCommentLength));
  return result;
};
