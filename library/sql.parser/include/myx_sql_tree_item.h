/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef MYX_SQL_TREE_ITEM
#define MYX_SQL_TREE_ITEM

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4251 )
#endif

#include "mysql_sql_parser_public_interface.h"
#include "myx_sql_parser_public_interface.h"
#include "sql_parser_symbols.h"
#include <list>
#include <set>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <memory>


namespace mysql_parser
{

extern void * new_ast_node(sql::symbol name);
extern void * reuse_ast_node(void *node_, sql::symbol name);
extern void * set_ast_node_name(void *node_, sql::symbol name);
extern void add_ast_child_node(void *parent_node_, void *child_node_);
extern void merge_ast_child_nodes(void *dest_node_, void *src_node_);

} // namespace mysql_parser


namespace mysql_parser
{

#define ARR_CAPACITY(arr) (sizeof(arr)/sizeof(arr[0]))

bool are_cstrings_eq(const char *str1, const char *str2, bool case_sensitive);
bool are_strings_eq(const std::string &str1, const std::string &str2, bool case_sensitive);

bool are_cstrings_eq_ci(const char *str1, const char *str2);
bool are_strings_eq_ci(const std::string &str1, const std::string &str2);

const char* find_cstr_in_array_ci(const char *arr[], size_t arr_size, const char *str);
const char* find_str_in_array_ci(const char *arr[], size_t arr_size, const std::string &str);


#ifdef __cplusplus

class SqlAstNode;
class SqlAstTerminalNode;
class SqlAstNonTerminalNode;
class SqlAstStatics
{
private:
  static std::list<SqlAstNode *> _ast_nodes; // flat list of all allocated ast nodes
public:
  static SqlAstNode * add_ast_node(SqlAstNode *ast_node)
  {
    _ast_nodes.push_back(ast_node);
    return ast_node;
  }
  static void cleanup_ast_nodes();

private:
  static const SqlAstNode *_tree;
public:
  static const SqlAstNode * tree() { return _tree; }
  static void tree(const SqlAstNode *tree);

public:
  static bool is_ast_generation_enabled;
  
  static std::shared_ptr<SqlAstTerminalNode> first_terminal_node();
  static std::shared_ptr<SqlAstTerminalNode> last_terminal_node();
  
  static void first_terminal_node(std::shared_ptr<SqlAstTerminalNode> value);
  static void last_terminal_node(std::shared_ptr<SqlAstTerminalNode> value);
private:
  static const char *_sql_statement;
public:
  static const char * sql_statement() { return _sql_statement; }
  static void sql_statement(const char *val) { _sql_statement= val; }
};

// pattern composite
class MYSQL_SQL_PARSER_PUBLIC_FUNC SqlAstNode
{
public:
  typedef std::list<SqlAstNode *> SubItemList;

private:
  sql::symbol _name;      // _name is sql::symbol
  std::shared_ptr<std::string> _value;
  int _value_length;      // _value_length is in bytes
  int _stmt_lineno;
  int _stmt_boffset;
  int _stmt_eoffset;
  SubItemList* _subitems;

  const SqlAstNode * left_most_subitem() const;
  const SqlAstNode * right_most_subitem() const;
  const SqlAstNode * subitem_by_name(sql::symbol name, const SqlAstNode *start_item) const;
  const SqlAstNode * subitem_by_name(sql::symbol name, size_t position= 0) const;
  const SqlAstNode * rsubitem_by_name(sql::symbol name, size_t position= 0) const; // reverse

  inline const SqlAstNode * subseq__(const SqlAstNode *start_subitem, sql::symbol name, va_list args) const;
  inline const SqlAstNode * find_subseq__(const SqlAstNode *start_subitem, sql::symbol name, va_list args) const;

  const SqlAstNode * check_words(sql::symbol words[], size_t words_count, const SqlAstNode *start_item= NULL) const;
  const SqlAstNode * find_words(sql::symbol words[], size_t words_count, const SqlAstNode *start_item= NULL) const;

  char * subitems_as_string(const char *delim= " ") const;

  void restore_sql_text(int &boffset, int &eoffset, const SqlAstNode *first_subitem, const SqlAstNode *last_subitem) const;

public:
  SqlAstNode(sql::symbol name, const char *value, int value_length, int stmt_lineno, int stmt_boffset, int stmt_eoffset, SubItemList *items);
  virtual ~SqlAstNode();

  void set_name(sql::symbol name) { _name= name; }
  bool name_equals(sql::symbol to) const { return _name == to; }
  sql::symbol name() const { return _name; }
  std::string value() const;
  int value_length() const { return _value_length; }
  int stmt_lineno() const;
  int stmt_boffset() const;
  int stmt_eoffset() const;

  SubItemList *subitems() const { return _subitems; }

#define subitem(...) subitem_(__VA_ARGS__, NULL)
  const SqlAstNode * subitem_(int position, ...) const;
  const SqlAstNode * subitem_(sql::symbol name, ...) const; // name1, name2, ...
  const SqlAstNode * subitem__(sql::symbol name, va_list args) const;

  const SqlAstNode * subitem_by_path(sql::symbol path[]) const;

#define subseq(...) subseq_(__VA_ARGS__, NULL)
  const SqlAstNode * subseq_(const SqlAstNode *start_subitem, sql::symbol name, ...) const;
  const SqlAstNode * subseq_(sql::symbol name, ...) const;

#define find_subseq(...) find_subseq_(__VA_ARGS__, NULL)
  const SqlAstNode * find_subseq_(const SqlAstNode *start_subitem, sql::symbol name, ...) const;
  const SqlAstNode * find_subseq_(sql::symbol name, ...) const;

  const SqlAstNode * search_by_names(sql::symbol names[], size_t path_count) const;
  const SqlAstNode * search_by_paths(sql::symbol * paths[], size_t path_count) const;

  std::string restore_sql_text(const std::string &sql_statement, const SqlAstNode *first_subitem= NULL, const SqlAstNode *last_subitem= NULL) const;
  void build_sql(std::string &sql) const; // warning - produces incorrect sql now
};


class SqlAstTerminalNode : public SqlAstNode 
{
  SubItemList _empty_list;
public:
  SqlAstTerminalNode() : SqlAstNode(sql::_, NULL, 0, -1, -1, -1, &_empty_list) {} 
  SqlAstTerminalNode(const char *value, int value_length, int stmt_lineno, int stmt_boffset, int stmt_eoffset)
    : SqlAstNode(sql::_, value, value_length, stmt_lineno, stmt_boffset, stmt_eoffset, &_empty_list) {} 
};


class SqlAstNonTerminalNode : public SqlAstNode 
{
  SubItemList _subitems;
public:
  SqlAstNonTerminalNode(sql::symbol name)
    : SqlAstNode(name, NULL, 0, -1, -1, -1, &_subitems) {} 
  virtual ~SqlAstNonTerminalNode();
};


MYX_PUBLIC_FUNC std::ostream& operator << (std::ostream&, const SqlAstNode&);


#endif // __cplusplus

} // namespace mysql_parser

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif // MYX_SQL_TREE_ITEM

