/* Copyright (C) 2000-2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* sql_yacc.yy */

%{

#include "yy_common.h"

const Grammar_tree_item empty_item;

%}

%token IDENT
%token CHAR_LITERAL
%token PREC

%left EMPTY
%left IDENT CHAR_LITERAL PREC

%%

grammar:
    rule_list
    {
      yy_result= $$= $1;
    }
  ;

rule_list:
    rule
    {
      $$= new Grammar_tree_item();
      $$->add_item_as_last($1);
    }
  | rule_list rule
    {
      $$= $1;
      $$->add_item_as_last($2);
    }
  ;

rule:
    IDENT ':' alt_list ';'
    {
      $$= $3;
      $$->text($1->text());
      delete $1;
    }
  ;

alt_list:
    alt
    {
      $$= new Grammar_tree_item();
      $$->add_item_as_last($1);
    }
  | alt_list '|' alt
    {
      $$= $1;
      $$->add_item_as_last($3);
    }
  ;

alt:
    /* empty */ %prec EMPTY
    {
      $$= new Grammar_tree_item();
      $$->add_item_as_last(new Grammar_tree_item());
    }
  | alt_particle
    {
      $$= new Grammar_tree_item();
      $$->add_item_as_last($1);
    }
  | alt alt_particle
    {
      $$= $1;
      $$->add_item_as_last($2);
    }
  ;
  
alt_particle:
    IDENT
    {
      $$= $1;
    }
  | CHAR_LITERAL
    {
      $$= $1;
    }
  | PREC
    {
      $$= $1;
    }
  ;
