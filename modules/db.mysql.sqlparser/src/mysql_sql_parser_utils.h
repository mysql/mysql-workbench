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

#pragma once

#include "mysql_sql_parser_fe.h"
#include "grt.h"
#include "grts/structs.db.mysql.h"
#include "grtdb/charset_utils.h"

#include "base/string_utilities.h"

using namespace grt;
using namespace boost::placeholders;

template <typename T>
class Val_keeper {
public:
  Val_keeper(T *val_ptr) : _val_ptr(val_ptr), _val(*val_ptr) {
  }
  ~Val_keeper() {
    restore();
  }
  Val_keeper &operator=(const Val_keeper &val_keeper) {
    restore();
    _val_ptr = val_keeper._val_ptr;
    _val = val_keeper._val;
    val_keeper._val_ptr = NULL;
    val_keeper._val = T();
  }

private:
  void restore() {
    if (_val_ptr)
      *_val_ptr = _val;
  }

  T *_val_ptr;
  T _val;
};

std::string unquot(std::string &text, const std::string quot_sym = std::string("\"\'`"));
std::string quot(std::string &text, char quot_sym = '\'');

db_SimpleDatatypeRef map_datatype(const SqlAstNode *item, DictRef &datatype_cache);

void concatenate_items(const SqlAstNode *item, StringListRef &list, bool toupper);

std::string get_str_attr_from_subitem_(const SqlAstNode *item, sql::symbol name, ...);
#define get_str_attr_from_subitem(...) get_str_attr_from_subitem_(__VA_ARGS__, NULL)

class Cs_collation_setter {
public:
  typedef grt::StringRef (db_Schema::*str_mem_getter_fp)() const;
  typedef void(str_mem_setter_fp)(const grt::StringRef &);
  typedef boost::function<grt::StringRef()> str_mem_getter;
  typedef boost::function<void(const grt::StringRef &)> str_mem_setter;

private:
  str_mem_getter _charset_mem_getter;
  str_mem_setter _charset_mem_setter;
  str_mem_getter _collation_mem_getter;
  str_mem_setter _collation_mem_setter;
  str_mem_getter _parent_charset_mem_getter;
  str_mem_getter _parent_collation_mem_getter;
  bool _explicit_cs;
  void set_charset_name(std::string cs_name, bool force_explicit_cs = false) {
    if ((_explicit_cs || force_explicit_cs) && cs_name.empty())
      cs_name = base::tolower(*_parent_charset_mem_getter());
    _charset_mem_setter(cs_name);
  }
  void set_collation_name(std::string collation_name) {
    _collation_mem_setter(collation_name);
  }

public:
  Cs_collation_setter(str_mem_getter charset_mem_getter, str_mem_setter charset_mem_setter,
                      str_mem_getter collation_mem_getter, str_mem_setter collation_mem_setter,
                      str_mem_getter parent_charset_mem_getter, str_mem_getter parent_collation_mem_getter,
                      bool explicit_cs)
    : _charset_mem_getter(charset_mem_getter),
      _charset_mem_setter(charset_mem_setter),
      _collation_mem_getter(collation_mem_getter),
      _collation_mem_setter(collation_mem_setter),
      _parent_charset_mem_getter(parent_charset_mem_getter),
      _parent_collation_mem_getter(parent_collation_mem_getter),
      _explicit_cs(explicit_cs) {
  }
  void charset_name(std::string cs_name) {
    cs_name = base::tolower(cs_name);
    if (0 == cs_name.compare("DEFAULT"))
      cs_name = base::tolower(*_parent_charset_mem_getter());
    set_charset_name(cs_name);

    std::string collation_name = *_collation_mem_getter();
    if (!collation_name.empty()) {
      collation_name = base::tolower(collation_name);
      // clear collation if it's default collation or it belongs to another character set
      if ((collation_name == defaultCollationForCharset(cs_name)) || (cs_name != charsetForCollation(collation_name)))
        set_collation_name("");
    }
  }
  void collation_name(std::string collation_name) {
    if (!collation_name.empty()) {
      collation_name = base::tolower(collation_name);
      if (0 == collation_name.compare("DEFAULT"))
        collation_name = base::tolower(*_parent_collation_mem_getter());

      // clear collation if it's default collation
      std::string cs_name = charsetForCollation(collation_name);
      std::string cs_def_collation_name = defaultCollationForCharset(cs_name);
      if (cs_def_collation_name == collation_name)
        collation_name = "";

      if ((*_charset_mem_getter()).empty())
        set_charset_name(cs_name, true);
    }
    set_collation_name(collation_name);
  }
};

#define CS_COLLATION_SETTER(OBJ_CLASS, OBJ, OBJ_CS_MEMBER, OBJ_COLLATION_MEMBER, CONTAINER_CLASS, CONTAINER,          \
                            CONTAINER_CS_MEMBER, CONTRAINER_COLLATION_MEMBER, EXPLICIT_CS)                            \
  Cs_collation_setter(boost::bind(&OBJ_CLASS::OBJ_CS_MEMBER, &OBJ), boost::bind(&OBJ_CLASS::OBJ_CS_MEMBER, &OBJ, _1), \
                      boost::bind(&OBJ_CLASS::OBJ_COLLATION_MEMBER, &OBJ),                                            \
                      boost::bind(&OBJ_CLASS::OBJ_COLLATION_MEMBER, &OBJ, _1),                                        \
                      boost::bind(&CONTAINER_CLASS::CONTAINER_CS_MEMBER, &CONTAINER),                                 \
                      boost::bind(&CONTAINER_CLASS::CONTRAINER_COLLATION_MEMBER, &CONTAINER), EXPLICIT_CS)

Cs_collation_setter cs_collation_setter(db_SchemaRef obj, db_CatalogRef container, bool explicit_cs);
Cs_collation_setter cs_collation_setter(db_mysql_TableRef obj, db_SchemaRef container, bool explicit_cs);
Cs_collation_setter cs_collation_setter(db_ColumnRef obj, db_mysql_TableRef container, bool explicit_cs);

std::string strip_sql_statement(const std::string &text, bool confirmation);
std::string cut_sql_statement(std::string text);
std::string qualify_obj_name(std::string obj_name, std::string schema_name);
std::string shape_index_type(std::string index_type);
std::string shape_index_kind(const std::string &index_kind);

/*
  macro for convenient setting attribute value from string
  SET_STR
  SET_INT
*/
#define SET_PROLOG(val) const std::string val_(val);
#define SET_STR(attr, val) \
  {                        \
    SET_PROLOG(val)        \
    attr(val_);            \
  }
#define SET_INT(attr, val)    \
  {                           \
    SET_PROLOG(val)           \
    attr(atoi(val_.c_str())); \
  }

/*
  macro for convenient setting attribute value from parse tree item
  SET_STR_I
  SET_INT_I
  SET_SQL_I
*/
#define SET_I_PROLOG(item)          \
  const SqlAstNode *item_ = (item); \
  if (NULL != item_)
#define SET_STR_I(attr, item)     \
  {                               \
    SET_I_PROLOG(item)            \
    SET_STR(attr, item_->value()) \
  }
#define SET_INT_I(attr, item)     \
  {                               \
    SET_I_PROLOG(item)            \
    SET_INT(attr, item_->value()) \
  }
#define SET_SQL_I(attr, item)                      \
  {                                                \
    SET_I_PROLOG(item)                             \
    attr(item_->restore_sql_text(_sql_statement)); \
  }

/*
  macro for convenient setting attribute value from parse tree subitem
  SET_STR_SI
  SET_INT_SI
  SET_SQL_SI
*/
#define SET_SI_PROLOG(item, ...)                          \
  const SqlAstNode *item_ = (item)->subitem(__VA_ARGS__); \
  if (NULL != item_)
#define SET_STR_SI(attr, item, ...)  \
  {                                  \
    SET_SI_PROLOG(item, __VA_ARGS__) \
    SET_STR(attr, item_->value())    \
  }
#define SET_INT_SI(attr, item, ...)  \
  {                                  \
    SET_SI_PROLOG(item, __VA_ARGS__) \
    SET_INT(attr, item_->value())    \
  }
#define SET_SQL_SI(attr, item, ...)                \
  {                                                \
    SET_SI_PROLOG(item, __VA_ARGS__)               \
    attr(item_->restore_sql_text(_sql_statement)); \
  }
