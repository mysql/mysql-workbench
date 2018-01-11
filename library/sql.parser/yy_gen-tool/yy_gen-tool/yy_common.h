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

#pragma once

#include "grammar_tree_item.h"

typedef Grammar_tree_item * YYSTYPE;
#define YYSTYPE_IS_DECLARED

#define YYERROR_VERBOSE

#include "parser.tab.hh"

extern FILE *yyin;
extern YYSTYPE yy_result;
extern int yy_lineno;

int yyinit(FILE *input_file);
int yyfreeres();
int yylex();
int yyparse();

void yyerror(const char *msg);
int error(const char *msg);
int warning(const char *msg);

int generate_grammar_file(const Grammar_tree_item* tree, const char *filename);
