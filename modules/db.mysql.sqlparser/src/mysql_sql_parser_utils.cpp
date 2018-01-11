/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <glib.h>
#include <boost/signals2.hpp>

#include <cctype>
#include <algorithm>

#include <locale>
#include "mysql_sql_parser_utils.h"
#include "myx_sql_tree_item.h"
#include "grts/structs.db.mysql.h"
//#include "util_functions.h"
#include "grtpp_util.h"

std::string unquot(std::string text, const std::string quot_sym) {
  if (!text.empty())
    if ((std::string::npos != quot_sym.find(text[0]) && (std::string::npos != quot_sym.find(*text.rbegin()))))
      text = text.substr(1, text.size() - 2);
  return text;
}

std::string quot(std::string &text, char quot_sym) {
  if (!text.empty()) {
    text.insert(0, 1, quot_sym);
    text.append(1, quot_sym);
  }
  return text;
}

inline bool rulename2typename(const SqlAstNode *item, std::string &type_name);
inline bool get_type_token_name(const SqlAstNode *item, std::string &type_token_name);
inline bool translate_type_synonym(std::string &type_name);

db_SimpleDatatypeRef map_datatype(const SqlAstNode *item, DictRef &datatype_cache) {
  std::string type_name;

  if (!rulename2typename(item, type_name) && !get_type_token_name(item, type_name))
    return db_SimpleDatatypeRef();
  translate_type_synonym(type_name);

  if (type_name.empty())
    return db_SimpleDatatypeRef();

  if (datatype_cache.has_key(type_name))
    return db_SimpleDatatypeRef::cast_from(datatype_cache.get(type_name));

  return db_SimpleDatatypeRef();
}

bool rulename2typename(const SqlAstNode *item, std::string &type_name) {
  static std::map<sql::symbol, std::string> subst_rules;

  class Subst_rules_initializer {
  public:
    Subst_rules_initializer() {
      const sql::symbol keys[] = {
        sql::_real_type, sql::_varchar, sql::_nchar, sql::_nvarchar,
      };

      const char *values[] = {
        "DOUBLE", "VARCHAR", "NCHAR", "NVARCHAR",
      };

      for (size_t n = 0; n < ARR_CAPACITY(values); ++n)
        subst_rules[keys[n]] = values[n];
    }
  };
  static Subst_rules_initializer subst_rules_initializer;

  for (std::map<sql::symbol, std::string>::const_iterator i = subst_rules.begin(), i_end = subst_rules.end();
       i != i_end; ++i) {
    if (item->subitem(i->first)) {
      type_name = i->second;
      return true;
    }
  }
  return false;
}

bool get_type_token_name(const SqlAstNode *item, std::string &type_token_name) {
  static std::map<sql::symbol, bool> type_token_names;

  class Type_token_names_initializer {
  public:
    Type_token_names_initializer() {
      const sql::symbol type_token_names_[] = {
        sql::_BINARY,   sql::_BIT_SYM,     sql::_BLOB_SYM,   sql::_BOOLEAN_SYM,  sql::_BOOL_SYM,   sql::_DATETIME,
        sql::_DATE_SYM, sql::_DECIMAL_SYM, sql::_ENUM,       sql::_FIXED_SYM,    sql::_FLOAT_SYM,  sql::_LONGBLOB,
        sql::_LONGTEXT, sql::_MEDIUMBLOB,  sql::_MEDIUMTEXT, sql::_NUMERIC_SYM,  sql::_SERIAL_SYM, sql::_SET,
        sql::_TEXT_SYM, sql::_TIMESTAMP,   sql::_TIME_SYM,   sql::_TINYBLOB,     sql::_TINYTEXT,   sql::_VARBINARY,
        sql::_YEAR_SYM, sql::_char,        sql::_int_type,   sql::_spatial_type,
      };

      for (size_t n = 0; n < ARR_CAPACITY(type_token_names_); ++n)
        type_token_names[type_token_names_[n]];
    }
  };
  static Type_token_names_initializer type_token_names_initializer;

  const SqlAstNode *type_token_item = item->subitem(0);
  if (type_token_item && (type_token_names.end() != type_token_names.find(type_token_item->name()))) {
    type_token_name = type_token_item->value();
    type_token_name = base::toupper(type_token_name);
    return true;
  }

  return false;
}

bool translate_type_synonym(std::string &type_name) {
  static const char *subst_rules[][2] = {
    {"INTEGER", "INT"},  {"DEC", "DECIMAL"},  {"NUMERIC", "DECIMAL"},
    {"FLOAT4", "FLOAT"}, {"BOOL", "BOOLEAN"}, {"CHARACTER", "CHAR"},
  };

  for (size_t n = 0; n < ARR_CAPACITY(subst_rules); ++n) {
    if (0 == type_name.compare(subst_rules[n][0])) {
      type_name = subst_rules[n][1];
      return true;
    }
  }
  return false;
}

char toupper_(char c) {
  return std::toupper(c);
}

void concatenate_items(const SqlAstNode *item, StringListRef &list, bool toupper) {
  if (item) {
    for (SqlAstNode::SubItemList::const_iterator it = item->subitems()->begin(); it != item->subitems()->end(); ++it) {
      if ((*it)->value_length()) {
        std::string value = (*it)->value();
        if (toupper) {
          std::string val = value;
          std::locale loc;
          std::transform(val.begin(), val.end(), val.begin(), &toupper_);
          list.insert(val);
        } else
          list.insert(value);
      }
    }
  }
}

std::string get_str_attr_from_subitem_(const SqlAstNode *item, sql::symbol name, ...) { // item, name1, name2, ...
  va_list args;
  va_start(args, name);
  item = item->subitem__(name, args);
  va_end(args);

  return item ? item->value() : "";
}

Cs_collation_setter cs_collation_setter(db_SchemaRef obj, db_CatalogRef container, bool explicit_cs) {
  return CS_COLLATION_SETTER(db_Schema, obj.content(), defaultCharacterSetName, defaultCollationName, db_Catalog,
                             container.content(), defaultCharacterSetName, defaultCollationName, explicit_cs);
}

Cs_collation_setter cs_collation_setter(db_mysql_TableRef obj, db_SchemaRef container, bool explicit_cs) {
  return CS_COLLATION_SETTER(db_mysql_Table, obj.content(), defaultCharacterSetName, defaultCollationName, db_Schema,
                             container.content(), defaultCharacterSetName, defaultCollationName, explicit_cs);
}

Cs_collation_setter cs_collation_setter(db_ColumnRef obj, db_mysql_TableRef container, bool explicit_cs) {
  return CS_COLLATION_SETTER(db_Column, obj.content(), characterSetName, collationName, db_mysql_Table,
                             container.content(), defaultCharacterSetName, defaultCollationName, explicit_cs);
}

// in general, this could also include stripping of leading/trailing comments
std::string strip_sql_statement(const std::string &text, bool confirmation) {
  if (!confirmation)
    return text;

  int start_idx = 0;
  for (std::string::const_iterator e = text.end(), it = text.begin(); it != e; it++, start_idx++) {
    char c = *it;
    if ((c != ' ') && (c != '\t') && (c != '\r') && (c != '\n'))
      break;
  }

  int npos = (int)text.length() - start_idx;
  for (std::string::const_reverse_iterator e = text.rend(), it = text.rbegin(); it != e; it++, npos--) {
    char c = *it;
    if ((c != ' ') && (c != '\t') && (c != '\r') && (c != '\n'))
      break;
  }
  return text.substr(start_idx, npos);
}

std::string cut_sql_statement(std::string text) {
  static const size_t MAX_TEXT_LENGTH = 255U;
  if (MAX_TEXT_LENGTH < text.size())
    text.replace(MAX_TEXT_LENGTH, std::string::npos, "...");
  return text;
}

std::string qualify_obj_name(std::string obj_name, std::string schema_name) {
  std::string qualified_obj_name;
  qualified_obj_name.append("`").append(schema_name).append("`.`").append(obj_name).append("`");
  return qualified_obj_name;
}

std::string shape_index_type(std::string index_type) {
  index_type = index_type.substr(0, index_type.find(' ')); // only first word is meaningful
  index_type = base::toupper(index_type);
  if (0 == index_type.compare("KEY"))
    index_type = "INDEX";
  return index_type;
}

std::string shape_index_kind(const std::string &index_kind) {
  return base::toupper(index_kind);
}
