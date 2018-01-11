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

#ifndef myx_lex_helpers_h
#define myx_lex_helpers_h

#include <iostream>
#include <fstream>

namespace mysql_parser
{

extern const void *tree;

struct Lex_args
{
  void *arg1;
  void *arg2;
};
extern struct Lex_args lex_args;

extern std::istream* lex_input_stream;

extern int yylex(void **yylval);
extern void yyerror(const char *msg);
//extern int yywrap();
//extern void yy_custom_input(char *buf, int* result, int max_size);
//extern int yy_token_match(int token, const char *value);
//int yy_unknown_token(const char *value);
//extern FILE *yyin;
//extern HANDLE h_file;

} // namespace mysql_parser

extern int yyparse();

#endif // myx_lex_helpers_h
