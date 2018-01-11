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

#include "yy_common.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <locale>

extern bool html_gen_mode;
bool html_gen_mode= false;

int yy_lineno;
YYSTYPE yy_result;

void yyrestart (FILE *input_file);

int yyinit(FILE *input_file)
{
  ::yy_result= NULL;
  ::yy_lineno= 0;
  ::yyin= input_file;
  return 0;
}

int yyfreeres()
{
  fclose(::yyin);
  yyrestart(::yyin);
  return 0;
}

void yyerror(const char *msg)
{
  error(msg);
}

int error(const char *msg)
{
  std::cerr << "Error (line: " << ::yy_lineno << "): " << msg << std::endl;
  return 0;
}

int warning(const char *msg)
{
  std::cerr << "Warning (line: " << ::yy_lineno << "): " << msg << std::endl;
  return 0;
}

int generate_grammar_file(const Grammar_tree_item* tree, const char *filename)
{
/*
0 - root
|_1 - rule (has name)
  |_2 - alternative
    |_3 - alternative particle (has name)
*/

  typedef const Grammar_tree_item * Item;
  typedef const Grammar_tree_item::Item_list * Items;
  typedef Grammar_tree_item::Item_list::const_iterator Items_iterator;
  typedef std::map <std::string, Item> Rules;
  typedef std::map <std::string, Item>::const_iterator Rules_iterator;

#define ITERATE_SUBITEMS(ptr, level) \
  Item item ## level= ptr; \
  Items items ## level= item ## level->items(); \
  for (Items_iterator i ## level = items ## level->begin(); i ## level != items ## level->end(); ++i ## level) \

#define NEW_LOC \
  ENDL << std::setw(10) << ' '

  // determine end of line sequence (in html generation mode it additionaly has <br> tag)
  std::string ENDL;
  {
    std::ostringstream oss;
    if (html_gen_mode)
      oss << "<br>";
    oss << std::endl;
    ENDL= oss.str();
  }

  // index rules by names
  std::map <std::string, Item> rules;
  {
    ITERATE_SUBITEMS(tree, 0) // rules
    {
      std::string name= (*i0)->text();
      if (!name.empty())
        rules[name]= *i0;
    }
  }

  std::ofstream os(filename); // grammar file

  if (html_gen_mode)
  {
    // html prolog
    os << "<html><body>" << std::endl;
  }
  else
  {
    // generate header with symbol constants (must be included by grammar file)

    std::set<std::string> symbols;
    ITERATE_SUBITEMS(tree, 0)
    {
      symbols.insert((*i0)->text());
      ITERATE_SUBITEMS(*i0, 1)
      {
        ITERATE_SUBITEMS(*i1, 2)
        {
          symbols.insert((*i2)->text_unquoted());
        }
      }
    }
    symbols.erase("");
    symbols.erase("%prec");

    std::string header_fn= std::string(filename) + ".h";
    std::string impl_fn= std::string(filename) + ".cpp";

    std::ofstream os(header_fn.c_str());

    const char *INCLUDE_GUARDER_NAME= "_SQL_PARSER_SYMBOLS_H_";

    os << "#ifndef " << INCLUDE_GUARDER_NAME << std::endl
      << "#define " << INCLUDE_GUARDER_NAME << std::endl << std::endl
      << "namespace sql" << std::endl << "{" << std::endl << std::endl;

    // constants
    os << "enum symbol" << std::endl << "{" << std::endl << "_," << std::endl;
    for (std::set<std::string>::const_iterator i= symbols.begin(), i_end= symbols.end(); i != i_end; ++i)
      os << "_" << *i << "," << std::endl;
    os << "};" << std::endl << std::endl;

    os << "extern const char *symbol_names[];" << std::endl << std::endl
      << "}" << std::endl << std::endl << "#endif // " << INCLUDE_GUARDER_NAME << std::endl;

    os.close();
    os.open(impl_fn.c_str());

    os << "#include \"sql_parser_symbols.h\"" << std::endl
      << "namespace sql" << std::endl << "{" << std::endl << std::endl;

    // names
    os << "const char *symbol_names[]=" << std::endl << "{" << std::endl << "\"\"," << std::endl;
    for (std::set<std::string>::const_iterator i= symbols.begin(), i_end= symbols.end(); i != i_end; ++i)
      os << "\"" << *i << "\"," << std::endl;
    os << "};" << std::endl << std::endl;

    os << "} // namespace sql_parser_symbols" << std::endl;
  }

  // level-0
  ITERATE_SUBITEMS(tree, 0) // rules
  {
    Item item= (*i0);
    Items items= item->items();

    if (html_gen_mode) // add id for reference
      os << "<a name=\"" << item->text() << "\" id=\"" << item->text() << "\">";
    os << item->text() << ':';
    if (html_gen_mode)
      os << "</a>";
    os << ENDL;

    // check if only one alt particle at max for the rule (that should be true for 2 levels in depth)
    bool one_alt_particle_at_max= true;
    {
      for (Items_iterator i= items->begin(); i != items->end(); ++i)
      {
        if ((*i)->has_more_then_one_meaningful_child())
          one_alt_particle_at_max= false;
        else
        {
          // check two levels in depth
          for (Items_iterator i2= (*i)->items()->begin(); i2 != (*i)->items()->end(); ++i2)
          {
            const Grammar_tree_item *rule= NULL;
            {
              Rules_iterator i3= rules.find((*i2)->text());
              if (rules.end() != i3)
                rule= i3->second;
            }
            if (NULL != rule)
            {
              for (Items_iterator i3= rule->items()->begin(); i3 != rule->items()->end(); ++i3)
              {
                if ((*i3)->has_more_then_one_meaningful_child())
                {
                  one_alt_particle_at_max= false;
                  break;
                }
              }
              if (!one_alt_particle_at_max)
                break;
            }
          }
        }

        if (!one_alt_particle_at_max)
          break;
      }
    }

    ITERATE_SUBITEMS(*i0, 1) // rule->alternatives
    {
      os << std::setw(5) << ((items1->begin() == i1) ? ' ' : '|');
    
      char recursion_alt_flag= 0; // check if alt is recursive; 1-first alt particle is recursive, 2-last
      bool empty_alternative= !(*i1)->non_prec_child_exists(); // check if current alt is empty

      {
        // flush alt particle names & determine some characteristics of alt
        ITERATE_SUBITEMS(*i1, 2) // alternative->alt particles
        {
          Item item= (*i2);

          // check for recursion
          if (item1->text() == item->text())
          {
            if (recursion_alt_flag != 0)
              // ignore recursion optimization when more then 2 rec. entrances were met
              recursion_alt_flag= 3;
            else if (items2->front() == item)
              recursion_alt_flag= 1;
            else if (item2->is_last_non_prec_child(item))
              recursion_alt_flag= 2;
            else
              // ignore recursion optimization when rec. entrance is not in first/last position
              recursion_alt_flag= 3;
          }

          // flush name
          if (empty_alternative && (items2->front() == item))
            os << " /* empty */";
          if (!item->empty())
          {
            bool href_needed= html_gen_mode && (rules.end() != rules.find(item->text()));
            os << ' ';
            if (href_needed)
              os << "<a href=\"#" << item->text() << "\">";
            os << item->text();
            if (href_needed)
              os << "</a>";
          }
        }
        os << ENDL;
      }

      if (!html_gen_mode)// flush alt semantic
      {
        os << std::setw(7) << '{';
        
        if (empty_alternative)
          os << NEW_LOC << "$$= NULL;";
        else
        {
          os << ENDL << std::setw(8) << " " << "if (mysql_parser::SqlAstStatics::is_ast_generation_enabled)";
          os << ENDL << std::setw(9) << "{";

          if (one_alt_particle_at_max)
          {
            os << NEW_LOC << "$$= mysql_parser::set_ast_node_name($1, sql::_" << item1->text_unquoted() << ");";
          }
          else
          {
            switch (recursion_alt_flag)
            {
              case 1:
              {
                os << NEW_LOC << "$$= mysql_parser::reuse_ast_node($1, sql::_" << item1->text() << ");";
                break;
              }
              default:
              {
                os << NEW_LOC << "$$= mysql_parser::new_ast_node(sql::_" << item1->text() << ");";
                break;
              }
            }

            size_t n= 0;
            bool last_particle_was_prec_directive= false;

            ITERATE_SUBITEMS(*i1, 2) // alt particles
            {
              Item item= (*i2);
              
              // skip %prec directives
              if (item->is_prec_directive())
              {
                last_particle_was_prec_directive= true;
                continue;
              }
              else if (last_particle_was_prec_directive)
              {
                last_particle_was_prec_directive= false;
                continue;
              }

              ++n;

              if ((1 == recursion_alt_flag) && (items2->front() == item))
              {
                // skip processing of first recursive element, it was done earlier
              }
              else if ((2 == recursion_alt_flag) && (item2->is_last_non_prec_child(item)))
              {
                // process last recursive element in specific way
                os << NEW_LOC << "mysql_parser::merge_ast_child_nodes($$, $" << n << ");";
              }
              else
              {
                os << NEW_LOC << "mysql_parser::add_ast_child_node($$, ";
                if (item->is_terminal())
                  os << "mysql_parser::set_ast_node_name($" << n << ", sql::_" << item->text_unquoted() << ")";
                else
                  os << "$" << n;
                os << ");";
              }
            }
          }

          os << ENDL << std::setw(9) << "}";
        }

        os << ENDL << std::setw(7) << '}' << ENDL;
      }
    }
    os << std::setw(5) << ';' << ENDL << ENDL;
  }

  // html epilog
  if (html_gen_mode)
    os << "</body></html>" << std::endl;

  return 0;

#undef ITERATE_SUBITEMS
#undef NEW_LOC
}
