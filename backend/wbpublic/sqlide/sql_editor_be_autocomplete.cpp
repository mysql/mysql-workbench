/*
 * Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include <boost/assign/std/vector.hpp> // for 'operator += ..'

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <deque>
#include <assert.h>

#include "sql_editor_be.h"
#include "grt/grt_manager.h"

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"

#include "mforms/code_editor.h"

#include "autocomplete_object_name_cache.h"
#include "mysql-scanner.h"

#include "grammar-parser/ANTLRv3Lexer.h"
#include "grammar-parser/ANTLRv3Parser.h"
#include "MySQLLexer.h"

DEFAULT_LOG_DOMAIN("Code Completion");

using namespace boost::assign;

using namespace bec;
using namespace grt;
using namespace base;
using namespace parser;

//--------------------------------------------------------------------------------------------------

struct GrammarNode {
  bool is_terminal;
  bool is_required;     // false for * and ? operators, otherwise true.
  bool multiple;        // true for + and * operators, otherwise false.
  bool any;             // Set for . as grammar node (which matches any lexer token).
  uint32_t token_ref;   // In case of a terminal the id of the token.
  std::string rule_ref; // In case of a non-terminal the name of the rule.

  GrammarNode()
  {
    is_terminal = true;
    is_required = true;
    multiple = false;
    any = false;
    token_ref = INVALID_TOKEN;
  }
};

// A sequence of grammar nodes (either terminal or non-terminal) in the order they appear in the grammar.
// Expressions in parentheses are extracted into an own rule with a private name.
// A sequence can have an optional predicate (min/max server version and/or sql modes active/not active).
struct GrammarSequence {
  // Version predicate. Values are inclusive (min <= x <= max).
  int min_version;
  int max_version;

  // Bit mask of modes as defined in the lexer/parser.
  int active_sql_modes;   // SQL modes that must be active to match the predicate.
  int inactive_sql_modes; // SQL modes that must not be active. -1 for both if we don't care.

  std::vector<GrammarNode> nodes;

  GrammarSequence()
  {
    min_version = MIN_SERVER_VERSION;
    max_version = MAX_SERVER_VERSION;
    active_sql_modes = -1;
    inactive_sql_modes = -1;
  };
  
};

// A list of alternatives for a given rule.
struct RuleAlternatives {
  bool optimized;

  // We either have a list of grammar sequences or (in the optimized case) a set of tokens
  // which form alternatives in a given block.
  std::vector<GrammarSequence> sequence;
  std::set<ANTLR3_UINT32> set;

  RuleAlternatives()
  {
    optimized = true;
  }
};

//--------------------------------------------------------------------------------------------------

// A shared data structure for a given grammar file + some additional parsing info.
static struct
{
  std::map<std::string, RuleAlternatives> rules; // The full grammar.
  std::map<std::string, uint32_t> token_map;     // Map token names to token ids.

  // Rules that must not be examined further when collecting candidates.
  std::set<std::string> special_rules;  // Rules with a special meaning (e.g. "table_ref").
  std::set<std::string> ignored_rules;  // Rules we don't provide completion with (e.g. "label").
  std::set<std::string> ignored_tokens; // Tokens we don't want to show up (e.g. operators).

  //------------------------------------------------------------------------------------------------

  // Parses the given grammar file (and its associated .tokens file which must be in the same folder)
  // and fills the rule and token_map structures.
  void parse_file(const std::string &name)
  {
    log_debug("Parsing grammar file: %s\n", name.c_str());

    special_rules.clear();
    special_rules.insert("schema_ref");

    special_rules.insert("table_ref");
    special_rules.insert("table_ref_with_wildcard");
    special_rules.insert("filter_table_ref");
    special_rules.insert("table_ref_no_db");

    special_rules.insert("column_ref");
    special_rules.insert("column_internal_ref");
    special_rules.insert("column_ref_with_wildcard");
    special_rules.insert("table_wild");

    special_rules.insert("function_ref"); // Pure stored function reference.
    special_rules.insert("stored_function_call"); // Stored function call definition.
    special_rules.insert("udf_call");
    special_rules.insert("runtime_function_call");
    special_rules.insert("trigger_ref");
    special_rules.insert("view_ref");
    special_rules.insert("procedure_ref");
    special_rules.insert("logfile_group_ref");
    special_rules.insert("tablespace_ref");
    special_rules.insert("engine_ref");
    special_rules.insert("collation_name");
    special_rules.insert("charset_name");
    special_rules.insert("event_ref");

    special_rules.insert("user_variable");
    special_rules.insert("system_variable");

    ignored_rules.clear();
    ignored_rules.insert("label"); // Includes certain keywords which would show up.
    ignored_rules.insert("label_identifier"); // ditto
    ignored_rules.insert("text_or_identifier");
    ignored_rules.insert("identifier");
    ignored_rules.insert("pure_identifier");
    ignored_rules.insert("string_literal");
    ignored_rules.insert("qualified_identifier");
    ignored_rules.insert("dot_identifier");
    ignored_rules.insert("num_literal");
    ignored_rules.insert("ulong_number");
    ignored_rules.insert("real_ulong_number");
    ignored_rules.insert("ulonglong_number");
    ignored_rules.insert("real_ulonglong_number");

    // We have to use strings for the ignored tokens, instead of their #defines because we would have
    // to include MySQLParser.h, which conflicts with ANTLRv3Parser.h.
    ignored_tokens.clear();
    ignored_tokens.insert("EQUAL_OPERATOR");
    ignored_tokens.insert("ASSIGN_OPERATOR");
    ignored_tokens.insert("NULL_SAFE_EQUAL_OPERATOR");
    ignored_tokens.insert("GREATER_OR_EQUAL_OPERATOR");
    ignored_tokens.insert("GREATER_THAN_OPERATOR");
    ignored_tokens.insert("LESS_OR_EQUAL_OPERATOR");
    ignored_tokens.insert("LESS_THAN_OPERATOR");
    ignored_tokens.insert("NOT_EQUAL_OPERATOR");
    ignored_tokens.insert("NOT_EQUAL2_OPERATOR");
    ignored_tokens.insert("PLUS_OPERATOR");
    ignored_tokens.insert("MINUS_OPERATOR");
    ignored_tokens.insert("MULT_OPERATOR");
    ignored_tokens.insert("DIV_OPERATOR");
    ignored_tokens.insert("MOD_OPERATOR");
    ignored_tokens.insert("LOGICAL_NOT_OPERATOR");
    ignored_tokens.insert("BITWISE_NOT_OPERATOR");
    ignored_tokens.insert("SHIFT_LEFT_OPERATOR");
    ignored_tokens.insert("SHIFT_RIGHT_OPERATOR");
    ignored_tokens.insert("LOGICAL_AND_OPERATOR");
    ignored_tokens.insert("BITWISE_AND_OPERATOR");
    ignored_tokens.insert("BITWISE_XOR_OPERATOR");
    ignored_tokens.insert("LOGICAL_OR_OPERATOR");
    ignored_tokens.insert("BITWISE_OR_OPERATOR");
    ignored_tokens.insert("DOT_SYMBOL");
    ignored_tokens.insert("COMMA_SYMBOL");
    ignored_tokens.insert("SEMICOLON_SYMBOL");
    ignored_tokens.insert("COLON_SYMBOL");
    ignored_tokens.insert("OPEN_PAR_SYMBOL");
    ignored_tokens.insert("CLOSE_PAR_SYMBOL");
    ignored_tokens.insert("OPEN_CURLY_SYMBOL");
    ignored_tokens.insert("CLOSE_CURLY_SYMBOL");
    ignored_tokens.insert("OPEN_BRACKET_SYMBOL");
    ignored_tokens.insert("CLOSE_BRACKET_SYMBOL");
    ignored_tokens.insert("UNDERLINE_SYMBOL");
    ignored_tokens.insert("AT_SIGN_SYMBOL");
    ignored_tokens.insert("AT_AT_SIGN_SYMBOL");
    ignored_tokens.insert("NULL2_SYMBOL");
    ignored_tokens.insert("PARAM_MARKER");
    ignored_tokens.insert("BACK_TICK");
    ignored_tokens.insert("SINGLE_QUOTE");
    ignored_tokens.insert("DOUBLE_QUOTE");
    ignored_tokens.insert("ESCAPE_OPERATOR");
    ignored_tokens.insert("CONCAT_PIPES_SYMBOL");
    ignored_tokens.insert("AT_TEXT_SUFFIX");
    ignored_tokens.insert("BACK_TICK_QUOTED_ID");
    ignored_tokens.insert("SINGLE_QUOTED_TEXT");
    ignored_tokens.insert("DOUBLE_QUOTED_TEXT");
    ignored_tokens.insert("NCHAR_TEXT");
    ignored_tokens.insert("UNDERSCORE_CHARSET");
    ignored_tokens.insert("IDENTIFIER");
    ignored_tokens.insert("INT_NUMBER");
    ignored_tokens.insert("LONG_NUMBER");
    ignored_tokens.insert("ULONGLONG_NUMBER");
    ignored_tokens.insert("DECIMAL_NUMBER");
    ignored_tokens.insert("BIN_NUMBER");
    ignored_tokens.insert("HEX_NUMBER");
    ignored_tokens.insert("DOT_IDENTIFIER");

    // Load token map first.
    std::string tokenFileName = base::strip_extension(name) + ".tokens";
    std::ifstream tokenFile(tokenFileName.c_str());
    if (tokenFile.is_open())
    {
      while (!tokenFile.eof())
      {
        std::string line;
        std::getline(tokenFile, line);
        std::string::size_type p = line.find('=');

        token_map[line.substr(0, p)] = atoi(line.substr(p + 1).c_str());
      }
    }
    else
      log_error("Token file not found (%s)\n", tokenFileName.c_str());

    token_map["EOF"] = ANTLR3_TOKEN_EOF;

    // Now parse the grammar.
    std::ifstream stream(name.c_str(), std::ifstream::binary);
    if (!stream.is_open())
    {
      log_error("Grammar file not found\n");
      return;
    }

    log_debug("Parsing grammar...\n");

    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    pANTLR3_INPUT_STREAM input = antlr3StringStreamNew((pANTLR3_UINT8)text.c_str(), ANTLR3_ENC_UTF8,
                                                       (ANTLR3_UINT32)text.size(), (pANTLR3_UINT8)"");
    pANTLRv3Lexer lexer = ANTLRv3LexerNew(input);
    pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
    pANTLRv3Parser parser = ANTLRv3ParserNew(tokens);

    pANTLR3_BASE_TREE tree = parser->grammarDef(parser).tree;

    if (parser->pParser->rec->state->errorCount > 0)
      log_error("Found grammar errors. No code completion data available.\n");
    else
    {
      //std::string dump = MySQLRecognitionBase::dumpTree(parser->pParser->rec->state->tokenNames, tree);
      //std::cout << dump;

      // Walk the AST and put all the rules into our data structures.
      // We can handle here only combined and pure parser grammars (the lexer rules are ignored in a combined grammar).
      switch (tree->getType(tree))
      {
      case COMBINED_GRAMMAR_V3TOK:
      case PARSER_GRAMMAR_V3TOK:
        // Advanced to the first rule. The first node is the grammar node. Everything else is in child nodes of this.
        for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++)
        {
          pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
          if (child->getType(child) == RULE_V3TOK)
            traverse_rule(child);
        }
        break;
      }
    }
    
    // Must manually clean up.
    parser->free(parser);
    tokens ->free(tokens);
    lexer->free(lexer);
    input->close(input);
  }
  
private:
  void handle_server_version(std::vector<std::string> parts, GrammarSequence &sequence)
  {
    bool includes_equality = parts[1].size() == 2;
    int version = atoi(parts[2].c_str());
    switch (parts[1][0])
    {
      case '<': // A max version.
        sequence.max_version = version;
        if (!includes_equality)
          --sequence.max_version;
        break;
      case '=': // An exact version.
        sequence.max_version = version;
        sequence.min_version = version;
        break;
      case '>': // A min version.
        sequence.min_version = version;
        if (!includes_equality)
          ++sequence.min_version;
        break;

      default:
        throw std::runtime_error("Unhandled comparison operator in version number predicate (" + parts[1] + ")");
        break;
    }
  }

  //------------------------------------------------------------------------------------------------

  void parse_predicate(std::string predicate, GrammarSequence &sequence)
  {
    // Parsable predicates have one of these forms:
    // - "SERVER_VERSION >= 50100"
    // - "(SERVER_VERSION >= 50105) && (SERVER_VERSION < 50500)"
    // - "SQL_MODE_ACTIVE(SQL_MODE_ANSI_QUOTES)"
    // - "!SQL_MODE_ACTIVE(SQL_MODE_ANSI_QUOTES)"
    //
    // We don't do full expression parsing here. Only what is given above.
    static std::map<std::string, int> mode_map;
    if (mode_map.empty())
    {
      mode_map["SQL_MODE_ANSI_QUOTES"] = 1;
      mode_map["SQL_MODE_HIGH_NOT_PRECEDENCE"] = 2;
      mode_map["SQL_MODE_PIPES_AS_CONCAT"] = 4;
      mode_map["SQL_MODE_IGNORE_SPACE"] = 8;
      mode_map["SQL_MODE_NO_BACKSLASH_ESCAPES"] = 16;
    }

    predicate = base::trim(predicate);
    std::vector<std::string> parts = base::split(predicate, "&&");
    if (parts.size() == 2)
    {
      // Min and max values for server versions.
      std::string expression = base::trim(parts[0]);
      if (base::starts_with(expression, "(") && base::ends_with(expression, ")"))
        expression = expression.substr(1, expression.size() - 2);
      std::vector<std::string> expression_parts = base::split(expression, " ");
      if ((expression_parts[0] == "SERVER_VERSION") && (expression_parts.size() == 3))
        handle_server_version(expression_parts, sequence);

      expression = base::trim(parts[1]);
      if (base::starts_with(expression, "(") && base::ends_with(expression, ")"))
        expression = expression.substr(1, expression.size() - 2);
      expression_parts = base::split(expression, " ");
      if ((expression_parts[0] == "SERVER_VERSION") && (expression_parts.size() == 3))
        handle_server_version(expression_parts, sequence);
    }
    else
    {
      // A single expression.
      parts = base::split(predicate, " ");
      if (parts.size() == 1)
      {
        if (base::starts_with(predicate, "SQL_MODE_ACTIVE("))
        {
          std::string mode = predicate.substr(16, predicate.size() - 17);
          if (mode_map.find(mode) != mode_map.end())
            sequence.active_sql_modes = mode_map[mode];
        }
        else if (base::starts_with(predicate, "!SQL_MODE_ACTIVE("))
        {
          std::string mode = predicate.substr(17, predicate.size() - 18);
          if (mode_map.find(mode) != mode_map.end())
            sequence.inactive_sql_modes = mode_map[mode];
        }
      }
      else
      {
        if ((parts[0] == "SERVER_VERSION") && (parts.size() == 3))
          handle_server_version(parts, sequence);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Creates a node sequence that comprises an entire alternative.
   */
  GrammarSequence traverse_alternative(pANTLR3_BASE_TREE alt, const std::string name)
  {
    GrammarSequence sequence;

    uint32_t index = 0;

    // Check for special nodes first.
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)alt->getChild(alt, index);
    switch (child->getType(child))
    {
      case GATED_SEMPRED_V3TOK: // A gated semantic predicate.
      case SEMPRED_V3TOK:       // A normal semantic predicate.
      {
        // See if we can extract version info or SQL mode condition from that.
        ++index;
        pANTLR3_STRING token_text = child->getText(child);
        std::string predicate((char*)token_text->chars);

        // A predicate has the form "{... text ... }?".
        predicate = predicate.substr(1, predicate.size() - 3);
        parse_predicate(predicate, sequence);
        break;
      }

      case SYN_SEMPRED_V3TOK: // A syntactic predicate converted to a semantic predicate.
                              // Not needed for our work, so we can ignore it.
        ++index;
        break;

      case EPSILON_V3TOK: // An empty alternative.
        return sequence;
    }

    // One less child node as the alt is always ended by an EOA node.
    for (; index < alt->getChildCount(alt) - 1; ++index)
    {
      child = (pANTLR3_BASE_TREE)alt->getChild(alt, index);
      GrammarNode node;

      uint32_t type = child->getType(child);

      // Ignore ROOT/BANG nodes (they are just tree construction markup).
      if (type == ROOT_V3TOK || type == BANG_V3TOK)
      {
        // Just one child.
        child = (pANTLR3_BASE_TREE)child->getChild(child, 0);
        type = child->getType(child);
      }

      switch (type)
      {
        case OPTIONAL_V3TOK:
        case CLOSURE_V3TOK:
        case POSITIVE_CLOSURE_V3TOK:
        {
          node.is_required = (type != OPTIONAL_V3TOK) && (type != CLOSURE_V3TOK);
          node.multiple = (type == CLOSURE_V3TOK) || (type == POSITIVE_CLOSURE_V3TOK);

          child = (pANTLR3_BASE_TREE)child->getChild(child, 0);

          // See if this block only contains a single alt with a single child node.
          // If so optimize and make this single child node directly the current node.
          bool optimized = false;
          if (child->getChildCount(child) == 2) // 2 because there's always that EOB child node.
          {
            pANTLR3_BASE_TREE child_alt = (pANTLR3_BASE_TREE)child->getChild(child, 0);
            if (child_alt->getChildCount(child_alt) == 2) // 2 because there's always that EOA child node.
            {
              optimized = true;
              child = (pANTLR3_BASE_TREE)child_alt->getChild(child_alt, 0);
              ANTLR3_UINT32 childType = child->getType(child);
              switch (childType)
              {
                case CHAR_LITERAL:
                case STRING_LITERAL:
                case TOKEN_REF:
                {
                  node.is_terminal = true;
                  pANTLR3_STRING token_text = child->getText(child);
                  std::string name = (char*)token_text->chars;
                  if (childType == CHAR_LITERAL || childType == STRING_LITERAL)
                    name = base::unquote(name);
                  node.token_ref = token_map[name];
                  break;
                }

                case RULE_REF:
                {
                  node.is_terminal = false;
                  pANTLR3_STRING token_text = child->getText(child);
                  std::string name = (char*)token_text->chars;
                  node.rule_ref = name;
                  break;
                }

                default:
                {
                  std::stringstream message;
                  message << "Unhandled type: " << type << " in alternative: " << name;
                  throw std::runtime_error(message.str());
                }
              }
            }
          }

          if (!optimized)
          {
            std::stringstream block_name;
            block_name << name << "_block" << index;
            traverse_block(child, block_name.str());

            node.is_terminal = false;
            node.rule_ref = block_name.str();
          }
          break;
        }

        case CHAR_LITERAL:
        case STRING_LITERAL:
        case TOKEN_REF:
        {
          node.is_terminal = true;
          pANTLR3_STRING token_text = child->getText(child);
          std::string name = (char*)token_text->chars;
          if (type == CHAR_LITERAL || type == STRING_LITERAL)
            name = base::unquote(name);
          node.token_ref = token_map[name];
          break;
        }

        case RULE_REF:
        {
          node.is_terminal = false;
          pANTLR3_STRING token_text = child->getText(child);
          std::string name = (char*)token_text->chars;
          node.rule_ref = name;
          break;
        }

        case BLOCK_V3TOK:
        {
          std::stringstream block_name;
          block_name << name << "_block" << index;
          traverse_block(child, block_name.str());

          node.is_terminal = false;
          node.rule_ref = block_name.str();
          break;
        }

        case DOT_SYM: // Match any token, except EOF.
          node.is_terminal = true;
          node.any = true;
          node.token_ref = DOT_SYMBOL; // Just a dummy (one of the ignore tokens), so it doesn't appear in the list.
          break;

        case LABEL_ASSIGN_V3TOK:
        {
          // A variable assignment, instead of a token or rule reference.
          // The reference is the second part of the assignment.
          pANTLR3_BASE_TREE token = (pANTLR3_BASE_TREE)child->getChild(child, 1);
          node.is_terminal = true;

          switch (token->getType(token))
          {
            case DOT_SYM:
              node.any = true;
              node.token_ref = DOT_SYMBOL;
              break;

            case CHAR_LITERAL:
            case STRING_LITERAL:
            case TOKEN_REF:
            {
              std::string tokenText = (char*)token->getText(token)->chars;
              if (type == CHAR_LITERAL || type == STRING_LITERAL)
                tokenText = base::unquote(tokenText);
              node.token_ref = token_map[tokenText];
              break;
            }

            default:
              std::stringstream message;
              message << "Unhandled type: " << type << " in label assignment: " << name;
              throw std::runtime_error(message.str());
          }

          break;
        }

        case SEMPRED_V3TOK:
          // A validating semantic predicate - ignore.
          // Might be necessary to handle one day, when we use such a predicate to
          // control parts with dynamic conditions.
          continue;

        default:
        {
          std::stringstream message;
          message << "Unhandled type: " << type << " in alternative: " << name;
          throw std::runtime_error(message.str());
        }
      }
      
      sequence.nodes.push_back(node);
    }
    
    return sequence;
  }
  
  //------------------------------------------------------------------------------------------------
  
  void traverse_block(pANTLR3_BASE_TREE block, const std::string name)
  {
    // A block is either a rule body or a part enclosed by parentheses.
    // A block consists of a number of alternatives which are stored as the content of that block
    // under the given name.

    RuleAlternatives alternatives;

    // Check if we can create an optimized alternatives variant which simply uses a set, so we can
    // test a match with a single operation.
    // To make this work the block must consist solely of single terminal token alternatives without
    // any predicate.
    for (ANTLR3_UINT32 index = 0; index < block->getChildCount(block) - 1; ++index)
    {
      pANTLR3_BASE_TREE alt = (pANTLR3_BASE_TREE)block->getChild(block, index);

      // 2 nodes at most: the single terminal + EOA. Gated semantic predicates are child nodes of that
      // alt node too, so they automatically get checked here too.
      if (alt->getType(alt) == ALT_V3TOK && alt->getChildCount(alt) > 2)
      {
        alternatives.optimized = false;
        break;
      }

      // Check also the type of the first node. We only accept terminals (no rule ref or closures).
      pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)alt->getChild(alt, 0);
      if (child->getType(child) != TOKEN_REF)
      {
        alternatives.optimized = false;
        break;
      }
    }

    if (alternatives.optimized)
    {
      for (ANTLR3_UINT32 index = 0; index < block->getChildCount(block) - 1; ++index)
      {
        pANTLR3_BASE_TREE alt = (pANTLR3_BASE_TREE)block->getChild(block, index);
        if (alt->getType(alt) == ALT_V3TOK)
        {
          pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)alt->getChild(alt, 0);
          pANTLR3_STRING token_text = child->getText(child);
          alternatives.set.insert(token_map[(char*)token_text->chars]);
        }
      }

    }
    else
    {
      // One less child in the loop as the list is always ended by a EOB node.
      for (ANTLR3_UINT32 index = 0; index < block->getChildCount(block) - 1; index++)
      {
        pANTLR3_BASE_TREE alt = (pANTLR3_BASE_TREE)block->getChild(block, index);
        if (alt->getType(alt) == ALT_V3TOK) // There can be REWRITE nodes (which we don't need).
        {
          std::stringstream alt_name;
          alt_name << name << "_alt" << index;
          GrammarSequence sequence = traverse_alternative(alt, alt_name.str());
          alternatives.sequence.push_back(sequence);
        }
      }
    }
    rules[name] = alternatives;
  }

  //------------------------------------------------------------------------------------------------

  void traverse_rule(pANTLR3_BASE_TREE rule)
  {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)rule->getChild(rule, 0);
    pANTLR3_STRING token_text = child->getText(child);
    std::string name((char*)token_text->chars);

    // Parser rules start with a lower case letter.
    if (islower(name[0]))
    {
      child = (pANTLR3_BASE_TREE)rule->getChild(rule, 1);
      if (child->getType(child) == OPTIONS) // There might be an optional options block on the rule.
        child = (pANTLR3_BASE_TREE)rule->getChild(rule, 2);
      if (child->getType(child) == BLOCK_V3TOK)
        traverse_block(child, name);
    }
    
    // There's another child (the always present EORu node) which we ignore.
  }
  
  //------------------------------------------------------------------------------------------------

} rules_holder;

//--------------------------------------------------------------------------------------------------

struct TableReference
{
  std::string schema;
  std::string table;
  std::string alias;
};

// Context structure for code completion results and token info.
struct AutoCompletionContext
{
  std::string typed_part;

  long server_version;
  int sql_mode;

  char **token_names;
  std::deque<std::string> walk_stack; // The rules as they are being matched or collected from.
                                      // It's a deque instead of a stack as we need to iterate over it.

  enum RunState { RunStateMatching, RunStateCollectionPending } run_state;

  boost::shared_ptr<MySQLScanner> scanner;
  std::set<std::string> completion_candidates;

  size_t caret_line;
  size_t caret_offset;

  // A hierarchical view of all table references in the code, updated constantly during the match process.
  // Organized as stack to be able to easily remove sets of references when changing nesting level.
  std::deque< std::vector<TableReference> > referencesStack;

  // A flat list of possible references. Kinda snapshot of the references stack at the point when collection
  // begins (the stack is cleaned up while bubbling up, after the collection process).
  // Additionally, it gets also all references after the caret.
  std::vector<TableReference> references;

  //------------------------------------------------------------------------------------------------

  /**
   * Uses the given scanner (with set input) to collect a set of possible completion candidates
   * at the given line + offset.
   *
   * @returns true if the input could fully be matched (happens usually only if the given caret
   *          is after the text and can be used to test if the algorithm parses queries fully).
   *
   * Actual candidates are stored in the completion_candidates member set.
   *
   */
  bool collectCandidates(boost::shared_ptr<MySQLScanner> aScanner)
  {
    scanner = aScanner; // Has all the data necessary for scanning already.
    server_version = scanner->get_server_version();
    sql_mode = scanner->get_sql_mode_flags();

    run_state = RunStateMatching;

    if (scanner->token_channel() != 0)
      scanner->next(true);

    referencesStack.push_back(std::vector<TableReference>()); // For the root level of table references.
    bool matched = match_rule("query");

    // Post processing some entries.
    if (completion_candidates.count("NOT2_SYMBOL") > 0)
    {
      // NOT2 is a NOT with special meaning in the operator preceedence chain.
      // For code completion it's the same as NOT.
      completion_candidates.erase("NOT2_SYMBOL");
      completion_candidates.insert("NOT_SYMBOL");
    }

    // Add synonyms.
    if (completion_candidates.count("CHAR_SYMBOL") > 0)
      completion_candidates.insert("CHARACTER_SYMBOL");
    if (completion_candidates.count("NOW_SYMBOL") > 0)
    {
      completion_candidates.insert("CURRENT_TIMESTAMP_SYMBOL");
      completion_candidates.insert("LOCALTIME_SYMBOL");
      completion_candidates.insert("LOCALTIMESTAMP_SYMBOL");
    }
    if (completion_candidates.count("DAY_SYMBOL") > 0)
      completion_candidates.insert("DAYOFMONTH_SYMBOL");
    if (completion_candidates.count("DECIMAL_SYMBOL") > 0)
      completion_candidates.insert("DEC_SYMBOL");
    if (completion_candidates.count("DISTINCT_SYMBOL") > 0)
      completion_candidates.insert("DISTINCTROW_SYMBOL");
    if (completion_candidates.count("COLUMNS_SYMBOL") > 0)
      completion_candidates.insert("FIELDS_SYMBOL");
    if (completion_candidates.count("FLOAT_SYMBOL") > 0)
      completion_candidates.insert("FLOAT4_SYMBOL");
    if (completion_candidates.count("DOUBLE_SYMBOL") > 0)
      completion_candidates.insert("FLOAT8_SYMBOL");
    if (completion_candidates.count("INT_SYMBOL") > 0)
    {
      completion_candidates.insert("INTEGER_SYMBOL");
      completion_candidates.insert("INT4_SYMBOL");
    }
    if (completion_candidates.count("RELAY_THREAD_SYMBOL") > 0)
      completion_candidates.insert("IO_THREAD_SYMBOL");
    if (completion_candidates.count("SUBSTRING_SYMBOL") > 0)
      completion_candidates.insert("MID_SYMBOL");
    if (completion_candidates.count("MEDIUMINT_SYMBOL") > 0)
      completion_candidates.insert("MIDDLEINT_SYMBOL");
    if (completion_candidates.count("NDBCLUSTER_SYMBOL") > 0)
      completion_candidates.insert("NDB_SYMBOL");
    if (completion_candidates.count("REGEXP_SYMBOL") > 0)
      completion_candidates.insert("RLIKE_SYMBOL");
    if (completion_candidates.count("DATABASE_SYMBOL") > 0)
      completion_candidates.insert("SCHEMA_SYMBOL");
    if (completion_candidates.count("DATABASES_SYMBOL") > 0)
      completion_candidates.insert("SCHEMAS_SYMBOL");
    if (completion_candidates.count("USER_SYMBOL") > 0)
      completion_candidates.insert("SESSION_USER_SYMBOL");
    if (completion_candidates.count("STD_SYMBOL") > 0)
    {
      completion_candidates.insert("STDDEV_SYMBOL");
      completion_candidates.insert("STDDEV_POP_SYMBOL");
    }
    if (completion_candidates.count("SUBSTRING_SYMBOL") > 0)
      completion_candidates.insert("SUBSTR_SYMBOL");
    if (completion_candidates.count("VARCHAR_SYMBOL") > 0)
      completion_candidates.insert("VARCHARACTER_SYMBOL");
    if (completion_candidates.count("VARIANCE_SYMBOL") > 0)
      completion_candidates.insert("VAR_POP_SYMBOL");

    if (completion_candidates.count("TINYINT_SYMBOL") > 0)
      completion_candidates.insert("INT1_SYMBOL");
    if (completion_candidates.count("SMALLINT_SYMBOL") > 0)
      completion_candidates.insert("INT2_SYMBOL");
    if (completion_candidates.count("MEDIUMINT_SYMBOL") > 0)
      completion_candidates.insert("INT3_SYMBOL");
    if (completion_candidates.count("BIGINT_SYMBOL") > 0)
      completion_candidates.insert("INT8_SYMBOL");
    if (completion_candidates.count("FRAC_SECOND_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_FRAC_SECOND_SYMBOL");
    if (completion_candidates.count("SECOND_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_SECOND_SYMBOL");
    if (completion_candidates.count("MINUTE_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_MINUTE_SYMBOL");
    if (completion_candidates.count("HOUR_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_HOUR_SYMBOL");
    if (completion_candidates.count("DAY_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_DAY_SYMBOL");
    if (completion_candidates.count("WEEK_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_WEEK_SYMBOL");
    if (completion_candidates.count("MONTH_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_MONTH_SYMBOL");
    if (completion_candidates.count("QUARTER_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_QUARTER_SYMBOL");
    if (completion_candidates.count("YEAR_SYMBOL") > 0)
      completion_candidates.insert("SQL_TSI_YEAR_SYMBOL");

    // If a column reference is required then we have to continue scanning the query for table references.
    if (completion_candidates.count("column_ref") > 0)
    {
      collectRemainingTableReferences();
      takeReferencesSnapshot(); // Move references from stack to the ref map.
    }

    return matched;
  }

  //------------------------------------------------------------------------------------------------
  
private:
  /**
   * Matches the entire sequence if the input allows that and returns true if that was the case,
   * otherwise false.
   * Collects table references as we come along them.
   */
  bool matchAltAndCollectTableRefs(const GrammarSequence &sequence)
  {
    // An empty sequence per se matches anything without consuming input.
    if (sequence.nodes.empty())
      return true;

    size_t i = 0;
    while (true)
    {
      // Set to true if the current node allows multiple occurrences and was matched at least once.
      bool matched_loop = false;

      // Skip any optional nodes if they don't match the current input.
      bool matched;
      GrammarNode node;
      do
      {
        node = sequence.nodes[i];
        if (node.is_terminal)
          matched = scanner->is(node.token_ref) || (node.any && !scanner->is(ANTLR3_TOKEN_EOF));
        else
          matched = matchRuleAndCollectTableRefs(node.rule_ref);

        if (matched && node.multiple)
          matched_loop = true;

        if (matched || node.is_required)
          break;

        // Did not match an optional part. That's ok, skip this then.
        ++i;
        if (i == sequence.nodes.size()) // Done with the sequence?
          return true;

      } while (true);

      if (matched)
      {
        // Load next token if the grammar node is a terminal node.
        // Otherwise the match-rule-call will have advanced the input position already.
        if (node.is_terminal)
          scanner->next(true);

        // If the current grammar node can be matched multiple times try as often as you can.
        // This is the greedy approach and default in ANTLR. At the moment we don't support non-greedy matches
        // as we don't use them in MySQL parser rules.
        if (!scanner->is(ANTLR3_TOKEN_EOF) && node.multiple)
        {
          do
          {
            if (node.is_terminal)
            {
              matched = scanner->is(node.token_ref) || (node.any && !scanner->is(ANTLR3_TOKEN_EOF));
              scanner->next(true);
            }
            else
              matched = matchRuleAndCollectTableRefs(node.rule_ref);
          } while (matched);

          if (scanner->is(ANTLR3_TOKEN_EOF))
            break;
        }
      }
      else
      {
        // No match, but could be end of a grammar node loop.
        if (!matched_loop)
          return false;
      }
      
      ++i;
      if (i == sequence.nodes.size())
        break;
    }

    return true;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Similar as the main rule matching function below, but simpler and specific for table references.
   * We try to match only one of the alts in the given rule (the first wins) and collect
   * table references on the way.
   */
  bool matchRuleAndCollectTableRefs(const std::string &rule)
  {
    RuleAlternatives alts = rules_holder.rules[rule];

    if (alts.optimized)
    {
      if (alts.set.count(scanner->token_type()) > 0)
      {
        scanner->next();
        return true;
      }
    }
    else
    {
      size_t marker = scanner->position();
      for (std::vector<GrammarSequence>::const_iterator i = alts.sequence.begin(); i != alts.sequence.end(); ++i)
      {
        // First run predicate checks if this alt can be considered at all.
        if ((i->min_version > server_version) || (server_version > i->max_version))
          continue;

        if ((i->active_sql_modes > -1) && (i->active_sql_modes & sql_mode) != i->active_sql_modes)
          continue;

        if ((i->inactive_sql_modes > -1) && (i->inactive_sql_modes & sql_mode) != 0)
          continue;

        if (matchAltAndCollectTableRefs(*i))
        {
          if (rule == "table_ref")
          {
            TableReference reference;
            size_t position = scanner->position();

            // At this point we are at the end of the table_ref token, so we need to scan back for the table
            // and forward for the alias.
            // Keep in mind we must have a valid table identifier here, as we just matched it in the above matchAltAndCollectTableRefs call.
            scanner->previous();
            reference.table = base::unquote(scanner->token_text());
            scanner->previous();
            if (scanner->is(DOT_SYMBOL))
            {
              scanner->previous();
              reference.schema = base::unquote(scanner->token_text());
            }

            // Now scan to the right (which might be errornous) for the alias.
            scanner->seek(position);
            if (scanner->skipIf(PARTITION_SYMBOL) && scanner->is(OPEN_PAR_SYMBOL))
            {
              // All partition info is between a pair of parentheses.
              do
              {
                scanner->next();
              } while (!scanner->is(CLOSE_PAR_SYMBOL) && !scanner->is(ANTLR3_TOKEN_EOF));
              scanner->skipIf(CLOSE_PAR_SYMBOL);
            }

            if (scanner->is(AS_SYMBOL) || scanner->is(EQUAL_OPERATOR))
              scanner->next();
            if (scanner->is_identifier())
              reference.alias = base::unquote(scanner->token_text());

            referencesStack.front().push_back(reference);

            scanner->seek(position);
          }

          return true;
        }
        
        scanner->seek(marker);
      }
    }

    return false;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Called if one of the candidates is a column_ref.
   * The function attempts to get table references together with aliases where possible.
   * Because inner queries can use table references from outer queries we can simply scan for any FROM clause
   * provided we don't go deeper. This way the query doesn't need to be error free, just the FROM clauses must.
   */
  void collectRemainingTableReferences()
  {
    // First advance to the FROM keyword on the same level as the caret is (no subselects etc.).
    // With certain syntax errors this can lead to a wrong FROM clause (e.g. if parentheses don't match).
    // But that is acceptable.

    // Reset the scanner to the caret position and continue from there. We have already collected all
    // table references before that position during normal matching.
    scanner->reset();
    scanner->seek(caret_line, caret_offset);

    size_t level = 0;
    while (true)
    {
      switch (scanner->token_type())
      {
        case FROM_SYMBOL:
          scanner->next(true);
          if (level == 0 && !scanner->is(DUAL_SYMBOL))
            matchRuleAndCollectTableRefs("join_table_list");
          break;

        case ANTLR3_TOKEN_EOF:
          return;

        case OPEN_PAR_SYMBOL:
          ++level;
          scanner->next(true);
          break;

        case CLOSE_PAR_SYMBOL:
          // No problem having more closing pars as we have opening ones. Could be there is a syntax error
          // or we just started and the caret position is between some parentheses.
          if (level > 0)
            --level;
          scanner->next(true);
          break;

        default:
          scanner->next(true);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Copies the current references stack into the references map.
   */
  void takeReferencesSnapshot()
  {
    // Don't clear the references map here. Can happen we have to take multiple snapshots.
    // We automatically remove duplicates by using a map.
    for (size_t level = 0; level < referencesStack.size(); ++level)
    {
      for (size_t entry = 0; entry < referencesStack[level].size(); ++entry)
        references.push_back(referencesStack[level][entry]);
    }

  }

  //------------------------------------------------------------------------------------------------

  bool is_token_end_after_caret()
  {
    if (scanner->is(ANTLR3_TOKEN_EOF))
      return true;

    assert(scanner->token_line() > 0);
    if (scanner->token_line() > caret_line)
      return true;

    if (scanner->token_line() < caret_line)
      return false;

    // This determination is a bit tricky as it depends on the type of the token.
    // For letters (like when typing a keyword) all positions directly attached to a letter must be
    // considered within the token (as we could extend it).
    // For example each vertical bar is a position within the token: |F|R|O|M|
    // Not so with tokens that can separate other tokens without the need of a whitespace (comma etc.).
    bool result;
    if (scanner->is_separator())
      result = scanner->token_end() > caret_offset;
    else
      result = scanner->token_end() >= caret_offset;

    return result;
  }

  //----------------------------------------------------------------------------------------------------------------------
  
  /**
   * Collects all tokens that can be reached in the sequence from the given start point. There can be more than one
   * if there are optional rules.
   * Returns true if the sequence between the starting point and the end consists only of optional tokens or there aren't
   * any at all.
   */
  void collect_from_alternative(const GrammarSequence &sequence, size_t start_index)
  {
    for (size_t i = start_index; i < sequence.nodes.size(); ++i)
    {
      GrammarNode node = sequence.nodes[i];
      if (node.is_terminal && node.token_ref == ANTLR3_TOKEN_EOF)
      {
        run_state = RunStateMatching;
        break;
      }

      if (node.is_terminal)
      {
        // Insert only tokens we are interested in.
        std::string token_ref = token_names[node.token_ref];
        bool ignored = rules_holder.ignored_tokens.find(token_ref) != rules_holder.ignored_tokens.end();
        bool exists = completion_candidates.find(token_ref) != completion_candidates.end();
        if (!ignored && !exists)
          completion_candidates.insert(token_ref);
        if (node.is_required)
        {
          // Also collect following tokens into this candidate, until we find the end of the sequence
          // or a token that is either not required or can appear multiple times.
          std::string token_refs = token_ref;
          if (!ignored && !node.multiple)
          {
            while (++i < sequence.nodes.size())
            {
              GrammarNode node = sequence.nodes[i];
              if (!node.is_terminal || !node.is_required || node.multiple)
                break;
              token_refs += std::string(" ") + token_names[node.token_ref];
            }

            if (token_refs.size() > token_ref.size())
            {
              if (!exists)
                completion_candidates.erase(token_ref);
              completion_candidates.insert(token_refs);
            }
          }

          // If we found a required token then we are done with this alternative.
          // That doesn't mean that we cannot start another collection run somewhere else. Just not in this alt anymore
          // (and those rules that include this alt).
          run_state = RunStateMatching;
          return;
        }
      }
      else
      {
        collect_from_rule(node.rule_ref);
        if (node.is_required && run_state != RunStateCollectionPending)
          return;
      }
    }

    // If we reach this point then we have found only optional parts, so the parent must continue collecting.
    run_state = RunStateCollectionPending;
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
   * Collects possibly reachable tokens from all alternatives in the given rule.
   */
  void collect_from_rule(const std::string rule)
  {
    // Don't go deeper if we have one of the special or ignored rules.
    if (rules_holder.special_rules.find(rule) != rules_holder.special_rules.end())
    {
      completion_candidates.insert(rule);
      run_state = RunStateMatching;
      return;
    }

    // Don't collect anything from an ignored rule.
    if (rules_holder.ignored_rules.count(rule) > 0)
    {
      run_state = RunStateMatching;
      return;
    }

    // Any other rule goes here.
    RunState combined_state = RunStateMatching;
    RuleAlternatives alts = rules_holder.rules[rule];
    if (alts.optimized)
    {
      // Insert only tokens we are interested in.
      for (std::set<ANTLR3_UINT32>::const_iterator i = alts.set.begin(); i != alts.set.end(); ++i)
      {
        std::string token_ref = token_names[*i];
        bool ignored = rules_holder.ignored_tokens.find(token_ref) != rules_holder.ignored_tokens.end();
        if (!ignored)
          completion_candidates.insert(token_ref);
      }

      run_state = RunStateMatching;
      return;
    }
    else
    {
      for (std::vector<GrammarSequence>::const_iterator i = alts.sequence.begin(); i != alts.sequence.end(); ++i)
      {
        // First run a predicate check if this alt can be considered at all.
        if ((i->min_version > server_version) || (server_version > i->max_version))
          continue;

        if ((i->active_sql_modes > -1) && (i->active_sql_modes & sql_mode) != i->active_sql_modes)
          continue;

        if ((i->inactive_sql_modes > -1) && (i->inactive_sql_modes & sql_mode) != 0)
          continue;

        collect_from_alternative(*i, 0);
        if (run_state == RunStateCollectionPending)
          combined_state = RunStateCollectionPending;
      }
    }
    run_state = combined_state;
  }
  
  //------------------------------------------------------------------------------------------------
  
  /**
   * Returns true if the given input token matches the given grammar node.
   * This may involve recursive rule matching.
   */
  bool match(const GrammarNode &node, uint32_t token_type)
  {
    if (node.is_terminal)
      return (node.token_ref == token_type) || (node.any && !scanner->is(ANTLR3_TOKEN_EOF));
    else
      return match_rule(node.rule_ref);
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
   * Returns true if the given index is at the end of the sequence or at a point where only optional parts follow.
   */
  bool hasMatchedAllMandatoryTokens(const GrammarSequence &sequence, size_t index)
  {
    if (index + 1 == sequence.nodes.size())
      return true;
    for (size_t i = index + 1; i < sequence.nodes.size(); ++i)
      if (sequence.nodes[i].is_required)
        return false;

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------------

  bool match_alternative(const GrammarSequence &sequence)
  {
    // An empty sequence per se matches anything without consuming input.
    if (sequence.nodes.empty())
      return true;

    size_t i = 0;
    while (true)
    {
      // Set to true if the current node allows multiple occurrences and was matched at least once.
      bool matched_loop = false;

      // Skip any optional nodes if they don't match the current input.
      bool matched;
      GrammarNode node;
      do
      {
        node = sequence.nodes[i];
        matched = match(node, scanner->token_type());

        // If that match call caused the collection to start then don't continue with matching here.
        if (run_state != RunStateMatching)
        {
          if (run_state == RunStateCollectionPending)
          {
            // We start collecting at the current node if it allows multiple matches (to include candidates from the
            // current rule). However this can prematurely stop the collection, since it might contain mandatory nodes.
            // But since we matched it already at least once we also have to include tokens directly following it.
            // Hence two calls for collect_from_alternative. The second call might include again already added candidates
            // but duplicates are sorted out automatically.
            if (node.multiple)
              collect_from_alternative(sequence, i);
            collect_from_alternative(sequence, i + 1);
          }
          return matched && hasMatchedAllMandatoryTokens(sequence, i);  // Return true only if we fully matched the sequence.
        }

        if (matched && node.multiple)
          matched_loop = true;

        if (matched || node.is_required)
          break;

        // Did not match an optional part. That's ok, skip this then.
        ++i;
        if (i == sequence.nodes.size()) // Done with the sequence?
          return true;

      } while (true);

      // Important note:
      // We still have an unsolved problem here, which has to do with ignored rules that are part of special rules
      // (e.g. a qualified identifier in an object reference).
      // Normally we walk up the match stack to see if we can include the special rule in such a case. However, if that
      // ignored rule ends with an optional part we cannot say currently if the current caret position is to be considered
      // still as part of that ignored rule or must be seen as part of the following one:
      // "qualifier. |" vs "identifier |" (with | being the caret).
      // In the first case we have to include the special rule, while in the second case we must not.
      //
      // However this is a very special case and we solve this currently by testing for the DOT symbol, but this
      // solution is not universal.
      if (matched)
      {
        // Load next token if the grammar node is a terminal node.
        // Otherwise the match() call will have advanced the input position already.
        if (node.is_terminal)
        {
          ANTLR3_UINT32 lastToken = scanner->token_type();
          scanner->next(true);
          if (is_token_end_after_caret())
          {
            takeReferencesSnapshot();

            // XXX: hack, need a better way to find out when we have to include the special rule from the stack.
            //      Using a fixed token look-back might not be valid for all languages.
            if (lastToken == DOT_SYMBOL)
            {
              for (std::deque<std::string>::const_iterator iterator = walk_stack.begin(); iterator != walk_stack.end(); ++iterator)
              {
                if (rules_holder.special_rules.find(*iterator) != rules_holder.special_rules.end())
                {
                  completion_candidates.insert(*iterator);
                  run_state = RunStateMatching;
                  return hasMatchedAllMandatoryTokens(sequence, i);
                }
              }
            }

            collect_from_alternative(sequence, node.multiple ? i : i + 1);

            return hasMatchedAllMandatoryTokens(sequence, i);
          }
        }
        else
        {
          // Similar here for non-terminals.
          if (is_token_end_after_caret())
          {
            takeReferencesSnapshot();
            collect_from_alternative(sequence, node.multiple ? i : i + 1);

            return hasMatchedAllMandatoryTokens(sequence, i);
          }
        }

        // If the current grammar node can be matched multiple times try as often as you can.
        // This is the greedy approach and default in ANTLR. At the moment we don't support non-greedy matches
        // as we don't use them in MySQL parser rules.
        if (!scanner->is(ANTLR3_TOKEN_EOF) && node.multiple)
        {
          while (true)
          {
            matched = match(node, scanner->token_type());

            // If we get a pending collection state here then it means the match() call caused a candidate collection
            // to start and reached the end of the node which contains at least one path that allows to match
            // more tokens after itself.
            // So, we have to continue collecting candidates after the current node.
            if (run_state == RunStateCollectionPending)
            {
              collect_from_alternative(sequence, i); // No check needed for multiple occurences (always the case here).
              collect_from_alternative(sequence, i + 1); // Same double collection as above.

              // If this collection run reached an end it means we are done here.
              // Otherwise we might still need more candidates to collect because this node or its subnodes are all
              // optional too.
              if (run_state != RunStateCollectionPending)
                return hasMatchedAllMandatoryTokens(sequence, i);

              if (!matched)
                break;
            }
            else
            {
              if (!matched)
                break;

              if (node.is_terminal)
              {
                scanner->next(true);
                if (is_token_end_after_caret())
                {
                  takeReferencesSnapshot();
                  collect_from_alternative(sequence, i + 1);
                  return hasMatchedAllMandatoryTokens(sequence, i);
                }
              }

              if (scanner->is(ANTLR3_TOKEN_EOF))
                break;
            }
          }
        }
      }
      else
      {
        // No match, but could be end of a grammar node loop.
        if (!matched_loop)
          return false;
      }

      ++i;
      if (i == sequence.nodes.size())
        break;
    }

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------------

  bool match_rule(const std::string &rule)
  {
    if (rule == "subquery")
      referencesStack.push_front(std::vector<TableReference>()); // Starting a new level.

    if (rule == "join_table_list" || rule == "table_ref")
    {
      // Collect table references as we come along them.
      size_t lastPosition = scanner->position();
      matchRuleAndCollectTableRefs(rule);
      scanner->seek(lastPosition);
    }

    if (run_state != RunStateMatching) // Sanity check - should never happen at this point.
      return false;

    if (is_token_end_after_caret())
    {
      collect_from_rule(rule);
      return false;
    }

    walk_stack.push_front(rule);

    size_t highest_token_index = 0;
    RunState result_state = run_state;
    bool matchedAtLeastOnce = false;

    // The longest match wins.
    RuleAlternatives alts = rules_holder.rules[rule];
    if (alts.optimized)
    {
      // In the optimized case we have neither predicates nor sequences.
      // We match a single terminal only, out of a set of alternative terminals.
      if (alts.set.count(scanner->token_type()) > 0)
      {
        matchedAtLeastOnce = true;
        scanner->next(true);
        if (is_token_end_after_caret())
          result_state = RunStateCollectionPending;
      }
    }
    else
    {
      bool can_seek = false;

      for (size_t i = 0; i < alts.sequence.size(); ++i)
      {
        // First run a predicate check if this alt can be considered at all.
        GrammarSequence alt = alts.sequence[i];
        if ((alt.min_version > server_version) || (server_version > alt.max_version))
          continue;

        if ((alt.active_sql_modes > -1) && (alt.active_sql_modes & sql_mode) != alt.active_sql_modes)
          continue;

        if ((alt.inactive_sql_modes > -1) && (alt.inactive_sql_modes & sql_mode) != 0)
          continue;

        // When attempting to match one alt out of a list pick the one with the longest match.
        // Reset the run state each time to have the base matching done first (in case a previous alt did collect).
        size_t marker = scanner->position();
        run_state = RunStateMatching;
        bool matched = match_alternative(alt);
        if (matched)
          matchedAtLeastOnce = true;
        if (matched || run_state != RunStateMatching)
        {
          can_seek = true;
          if (scanner->position() > highest_token_index)
          {
            highest_token_index = scanner->position();
            result_state = run_state;
          }
        }

        scanner->seek(marker);
      }

      if (can_seek)
        scanner->seek(highest_token_index); // Move to the end of the longest match.
    }

    run_state = result_state;
    walk_stack.pop_front();

    if (rule == "subquery")
      referencesStack.pop_front(); // Subquery ended, no need for the nested references anymore.

    return matchedAtLeastOnce;
  }

  //------------------------------------------------------------------------------------------------

};

//--------------------------------------------------------------------------------------------------

void MySQLEditor::setup_auto_completion()
{
  _code_editor->auto_completion_max_size(80, 15);

  static std::vector<std::pair<int, std::string> > ac_images;
  if (ac_images.empty())
    ac_images +=
      std::make_pair(AC_KEYWORD_IMAGE, "ac_keyword.png"),
      std::make_pair(AC_SCHEMA_IMAGE, "ac_schema.png"),
      std::make_pair(AC_TABLE_IMAGE, "ac_table.png"),
      std::make_pair(AC_ROUTINE_IMAGE, "ac_routine.png"),
      std::make_pair(AC_FUNCTION_IMAGE, "ac_function.png"),
      std::make_pair(AC_VIEW_IMAGE, "ac_view.png"),
      std::make_pair(AC_COLUMN_IMAGE, "ac_column.png"),
      std::make_pair(AC_OPERATOR_IMAGE, "ac_operator.png"),
      std::make_pair(AC_ENGINE_IMAGE, "ac_engine.png"),
      std::make_pair(AC_TRIGGER_IMAGE, "ac_trigger.png"),
      std::make_pair(AC_LOGFILE_GROUP_IMAGE, "ac_logfilegroup.png"),
      std::make_pair(AC_USER_VAR_IMAGE, "ac_uservar.png"),
      std::make_pair(AC_SYSTEM_VAR_IMAGE, "ac_sysvar.png"),
      std::make_pair(AC_TABLESPACE_IMAGE, "ac_tablespace.png"),
      std::make_pair(AC_EVENT_IMAGE, "ac_event.png"),
      std::make_pair(AC_INDEX_IMAGE, "ac_index.png"),
      std::make_pair(AC_USER_IMAGE, "ac_user.png"),
      std::make_pair(AC_CHARSET_IMAGE, "ac_charset.png"),
      std::make_pair(AC_COLLATION_IMAGE, "ac_collation.png");

  _code_editor->auto_completion_register_images(ac_images);
  _code_editor->auto_completion_stops("\t,.*;) "); // Will close ac even if we are in an identifier.
  _code_editor->auto_completion_fillups("");

  // Set up the shared grammar data if this is the first editor.
  if (rules_holder.rules.empty())
  {
    std::string grammar_path = make_path(grtm()->get_basedir(), "data/MySQL.g");
    rules_holder.parse_file(grammar_path);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Updates the auto completion list by filtering the determined entries by the text the user
 * already typed. If auto completion is not yet active it becomes active here.
 * Returns the list sent to the editor for unit tests to validate them.
 */
std::vector<std::pair<int, std::string> > MySQLEditor::update_auto_completion(const std::string &typed_part)
{
  log_debug2("Updating auto completion popup in editor\n");

  // Remove all entries that don't start with the typed text before showing the list.
  if (!typed_part.empty())
  {
    gchar *prefix = g_utf8_casefold(typed_part.c_str(), -1);
    
    std::vector<std::pair<int, std::string> > filtered_entries;
    for (std::vector<std::pair<int, std::string> >::const_iterator iterator = _auto_completion_entries.begin();
      iterator != _auto_completion_entries.end(); ++iterator)
    {
      gchar *entry = g_utf8_casefold(iterator->second.c_str(), -1);
      if (g_str_has_prefix(entry, prefix))
        filtered_entries.push_back(*iterator);
      g_free(entry);
    }
    
    switch (filtered_entries.size())
    {
    case 0:
      log_debug2("Nothing to autocomplete - hiding popup if it was active\n");
      _code_editor->auto_completion_cancel();
      break;
    case 1:
      // See if that single entry matches the typed part. If so we don't need to show ac either.
      if (base::same_string(filtered_entries[0].second, prefix, false)) // Exact (but case insensitive) match, not just string parts.
      {
        log_debug2("The only match is the same as the written input - hiding popup if it was active\n");
        _code_editor->auto_completion_cancel();
        break;
      }
      // Fall through.
    default:
      log_debug2("Showing auto completion popup\n");
      _code_editor->auto_completion_show(typed_part.size(), filtered_entries);
      break;
    }

    g_free(prefix);

    return filtered_entries;
  }
  else
  {
    if (_auto_completion_entries.size() > 0)
    {
      log_debug2("Showing auto completion popup\n");
      _code_editor->auto_completion_show(0, _auto_completion_entries);
    }
    else
    {
      log_debug2("Nothing to autocomplete - hiding popup if it was active\n");
      _code_editor->auto_completion_cancel();
    }
  }

  return _auto_completion_entries;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the text in the editor starting at the given position backwards until the line start.
 * If there's a back tick or double quote char then text until this quote char is returned. If there's
 * no quoting char but a space or dot char then everything up to (but not including) this is returned.
 */
std::string MySQLEditor::get_written_part(size_t position)
{
  ssize_t line = _code_editor->line_from_position(position);
  ssize_t start, stop;
  _code_editor->get_range_of_line(line, start, stop);
  std::string text = _code_editor->get_text_in_range(start, position);
  if (text.empty())
    return "";
  
  const char *head = text.c_str();
  const char *run = head;

  while (*run != '\0')
  {
    if (*run == '\'' || *run == '"' || *run == '`')
    {
      // Entering a quoted text.
      head = run + 1;
      char quote_char = *run;
      while (true)
      {
        run = g_utf8_next_char(run);
        if (*run == quote_char || *run == '\0')
          break;
        
        // If there's an escape char skip it and the next char too (if we didn't reach the end).
        if (*run == '\\')
        {
          run++;
          if (*run != '\0')
            run = g_utf8_next_char(run);
        }
      }
      if (*run == '\0') // Unfinished quoted text. Return everything.
        return head;
      head = run + 1; // Skip over this quoted text and start over.
    }
    run++;
  }
  
  // If we come here then we are outside of any quoted text. Scan back for anything we consider to be a word stopper.
  while (head < run--)
  {
    gunichar *converted = g_utf8_to_ucs4_fast(run, 1, NULL);
    bool isStopper = !(g_unichar_isalnum(*converted) || *run == '_');
    g_free(converted);
    if (isStopper)
      return run + 1;
  }
  return head;
}

//--------------------------------------------------------------------------------------------------

enum ObjectFlags {
  // For 3 part identifiers.
  ShowSchemas = 1 << 0,
  ShowTables  = 1 << 1,
  ShowColumns = 1 << 2,

  // For 2 part identifiers.
  ShowFirst  = 1 << 3,
  ShowSecond = 1 << 4,
};

/**
 * Determines the qualifier used for a qualified identifier with up to 2 parts (id or id.id).
 * Returns the found qualifier (if any) and a flag indicating what should be shown.
 *
 * Note: it is essential to understand that we do the determination only up to the caret
 *       (or the token following it, solely for getting a terminator). Since we cannot know the user's
 *       intention, we never look forward.
 */
ObjectFlags determine_qualifier(boost::shared_ptr<MySQLScanner> scanner, std::string &qualifier)
{
  // Five possible positions here:
  //   - In the first id (including directly before the first or directly after the last char).
  //   - In space between first id an a dot.
  //   - On a dot (visually directly before the dot).
  //   - In space after the dot, that includes the position directly after the dot.
  //   - In the second id.
  // All parts are optional (though not at the same time). The on-dot position is considered the same
  // as in first id as it visually belongs to the first id.

  size_t position = scanner->position();
  
  if (scanner->token_channel() != 0)
    scanner->next(true); // First skip to the next non-hidden token.

  if (!scanner->is(DOT_SYMBOL) && !scanner->is_identifier())
  {
    // We are at the end of an incomplete identifier spec. Jump back, so that the other tests succeed.
    scanner->previous(true);
  }

  // Go left until we find something not related to an id or find at most 1 dot.
  if (position > 0)
  {
    if (scanner->is_identifier() && scanner->look_around(-1, true) == DOT_SYMBOL)
      scanner->previous(true);
    if (scanner->is(DOT_SYMBOL) && scanner->MySQLRecognitionBase::is_identifier(scanner->look_around(-1, true)))
      scanner->previous(true);
  }

  // The scanner is now on the leading identifier or dot (if there's no leading id).
  qualifier = "";
  std::string temp;
  if (scanner->is_identifier())
  {
    temp = base::unquote(scanner->token_text());
    scanner->next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret position.
  if (!scanner->is(DOT_SYMBOL) || position <= scanner->position())
    return ObjectFlags(ShowFirst | ShowSecond);

  qualifier = temp;

  return ShowSecond;
}

//--------------------------------------------------------------------------------------------------

/**
 * Enhanced variant of the previous function that determines schema and table qualifiers for
 * column references (and table_wild in multi table delete, for that matter).
 * Returns a set of flags that indicate what to show for that identifier, as well as schema and table
 * if given.
 * The returned schema can be either for a schema.table situation (which requires to show tables)
 * or a schema.table.column situation. Which one is determined by whether showing columns alone or not.
 */
ObjectFlags determine_schema_table_qualifier(boost::shared_ptr<MySQLScanner> scanner, std::string &schema, std::string &table)
{
  size_t position = scanner->position();
  if (scanner->token_channel() != 0)
    scanner->next(true);

  uint32_t token_type = scanner->token_type();
  if (token_type != DOT_SYMBOL && !scanner->is_identifier())
  {
    // Just like in the simpler function. If we have found no identifier or dot then we are at the
    // end of an incomplete definition. Simply seek back to the previous non-hidden token.
    scanner->previous(true);
  }

  // Go left until we find something not related to an id or at most 2 dots.
  if (position > 0)
  {
    if (scanner->is_identifier() && scanner->look_around(-1, true) == DOT_SYMBOL)
      scanner->previous(true);
    if (scanner->is(DOT_SYMBOL) && scanner->MySQLRecognitionBase::is_identifier(scanner->look_around(-1, true)))
    {
      scanner->previous(true);

      // And once more.
      if (scanner->look_around(-1, true) == DOT_SYMBOL)
      {
        scanner->previous(true);
        if (scanner->MySQLRecognitionBase::is_identifier(scanner->look_around(-1, true)))
          scanner->previous(true);
      }
    }
  }

  // The scanner is now on the leading identifier or dot (if there's no leading id).
  schema = "";
  table = "";

  std::string temp;
  if (scanner->is_identifier())
  {
    temp = base::unquote(scanner->token_text());
    scanner->next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret position.
  if (!scanner->is(DOT_SYMBOL) || position <= scanner->position())
    return ObjectFlags(ShowSchemas | ShowTables | ShowColumns);

  scanner->next(true); // Skip dot.
  table = temp;
  schema = temp;
  if (scanner->is_identifier())
  {
    temp = base::unquote(scanner->token_text());
    scanner->next(true);

    if (!scanner->is(DOT_SYMBOL) || position <= scanner->position())
      return ObjectFlags(ShowTables | ShowColumns); // Schema only valid for tables. Columns must use default schema.

    table = temp;
    return ShowColumns;
  }
  return ObjectFlags(ShowTables | ShowColumns); // Schema only valid for tables. Columns must use default schema.
}

//--------------------------------------------------------------------------------------------------

struct CompareAcEntries
{
  bool operator() (const std::pair<int, std::string> &lhs, const std::pair<int, std::string> &rhs) const
  {
    return base::string_compare(lhs.second, rhs.second, false) < 0;
  }
};

typedef std::set<std::pair<int, std::string>, CompareAcEntries> CompletionSet;

//--------------------------------------------------------------------------------------------------

static void insertSchemas(AutoCompleteCache *cache, CompletionSet &set, const std::string &typed_part)
{
  std::vector<std::string> schemas = cache->get_matching_schema_names(typed_part);
  for (std::vector<std::string>::const_iterator schema = schemas.begin(); schema != schemas.end(); ++schema)
    set.insert(std::make_pair(AC_SCHEMA_IMAGE, *schema));
}

//--------------------------------------------------------------------------------------------------

static void insertTables(AutoCompleteCache *cache, CompletionSet &set, const std::set<std::string> &schemas,
  const std::string &typed_part)
{
  for (std::set<std::string>::const_iterator iterator = schemas.begin(); iterator != schemas.end(); ++iterator)
  {
    std::vector<std::string> tables = cache->get_matching_table_names(*iterator, typed_part);
    for (std::vector<std::string>::const_iterator table = tables.begin(); table != tables.end(); ++table)
      set.insert(std::make_pair(AC_TABLE_IMAGE, *table));
  }
}

//--------------------------------------------------------------------------------------------------

static void insertViews(AutoCompleteCache *cache, CompletionSet &set, const std::set<std::string> &schemas,
  const std::string &typed_part)
{
  for (std::set<std::string>::const_iterator iterator = schemas.begin(); iterator != schemas.end(); ++iterator)
  {
    std::vector<std::string> views = cache->get_matching_view_names(*iterator, typed_part);
    for (std::vector<std::string>::const_iterator view = views.begin(); view != views.end(); ++view)
      set.insert(std::make_pair(AC_VIEW_IMAGE, *view));
  }
}

//--------------------------------------------------------------------------------------------------

static void insertColumns(AutoCompleteCache *cache, CompletionSet &set, const std::set<std::string> &schemas,
  const std::set<std::string> &tables, const std::string &typedPart)
{
  for (std::set<std::string>::const_iterator schemaIterator = schemas.begin(); schemaIterator != schemas.end(); ++schemaIterator)
  {
    for (std::set<std::string>::const_iterator tableIterator = tables.begin(); tableIterator != tables.end(); ++tableIterator)
    {
      std::vector<std::string> columns = cache->get_matching_column_names(*schemaIterator, *tableIterator, typedPart);
      for (std::vector<std::string>::const_iterator column = columns.begin(); column != columns.end(); ++column)
        set.insert(std::make_pair(AC_COLUMN_IMAGE, *column));
    }
  }
}

//--------------------------------------------------------------------------------------------------

void MySQLEditor::show_auto_completion(bool auto_choose_single, ParserContext::Ref parser_context)
{
  if (!code_completion_enabled())
    return;

  log_debug("Invoking code completion\n");

  _code_editor->auto_completion_options(true, auto_choose_single, false, true, false);

  AutoCompletionContext context;
  context.token_names = parser_context->get_token_name_list();

  // Get the statement and its absolute position.
  size_t caret_token_index = _code_editor->get_caret_pos();
  context.caret_line = _code_editor->line_from_position(caret_token_index);
  ssize_t line_start, line_end;
  _code_editor->get_range_of_line(context.caret_line, line_start, line_end);
  context.caret_line++; // ANTLR parser is one-based.
  size_t offset = caret_token_index - line_start; // This is a byte offset.

  size_t min;
  size_t max;
  std::string statement;
  bool fixed_caret_pos = false;
  if (get_current_statement_range(min, max, true))
  {
    // If the caret is in the whitespaces before the query we would get a wrong line number
    // (because the statement splitter doesn't include these whitespaces in the determined ranges).
    // We set the caret pos to the first position in the query, which has the same effect for
    // code completion (we don't generate error line numbers).
    uint32_t code_start_line = (uint32_t)_code_editor->line_from_position(min);
    if (code_start_line > context.caret_line)
    {
      context.caret_line = 1;
      context.caret_offset = 0;
      fixed_caret_pos = true;
    }
    else
      context.caret_line -= code_start_line;

    statement = _code_editor->get_text_in_range(min, max);
  }
  else
  {
    // No query, means we have nothing typed yet in the current query (at least nothing valuable).
    context.caret_line = 1;
    context.caret_offset = 0;
    fixed_caret_pos = true;
  }

  // Convert current caret position into a position of the single statement. ANTLR uses one-based line numbers.
  // The byte-based offset in the line must be converted to a character offset.
  if (!fixed_caret_pos)
  {
    std::string line_text = _code_editor->get_text_in_range(line_start, line_end);
    context.caret_offset = g_utf8_pointer_to_offset(line_text.c_str(), line_text.c_str() + offset);
  }

  // Determine the word letters written up to the current caret position. If the caret is in the white
  // space behind a token then nothing has been typed.
  context.typed_part = get_written_part(caret_token_index);

  // Remove the escape character from the typed part so we have the pure text.
  context.typed_part.erase(std::remove(context.typed_part.begin(), context.typed_part.end(), '\\'),
    context.typed_part.end());

  // A set for each object type. This will sort the groups alphabetically and avoids duplicates,
  // but allows to add them as groups to the final list.
  CompletionSet schema_entries;
  CompletionSet table_entries;
  CompletionSet column_entries;
  CompletionSet view_entries;
  CompletionSet function_entries;
  CompletionSet udf_entries;
  CompletionSet runtime_function_entries;
  CompletionSet procedure_entries;
  CompletionSet trigger_entries;
  CompletionSet engine_entries;
  CompletionSet logfile_group_entries;
  CompletionSet tablespace_entries;
  CompletionSet system_var_entries;
  CompletionSet keyword_entries;
  CompletionSet collation_entries;
  CompletionSet charset_entries;
  CompletionSet event_entries;

  // Handled but needs meat yet.
  CompletionSet user_var_entries;

  // To be done yet.
  CompletionSet user_entries;
  CompletionSet index_entries;
  CompletionSet plugin_entries;

  _auto_completion_entries.clear();

  bool uppercase_keywords = make_keywords_uppercase();
  boost::shared_ptr<MySQLScanner> scanner = parser_context->createScanner(statement);
  context.server_version = scanner->get_server_version();
  context.collectCandidates(scanner);

  MySQLQueryType queryType = QtUnknown;
  {
    boost::shared_ptr<MySQLQueryIdentifier> queryIdentifier = parser_context->createQueryIdentifier();
    queryType = queryIdentifier->getQueryType(statement.c_str(), statement.size(), true);
  }

  // No sorting on the entries takes place. We group by type.
  for (std::set<std::string>::const_iterator i = context.completion_candidates.begin(); i != context.completion_candidates.end(); ++i)
  {
    scanner->reset();
    scanner->seek(context.caret_line, context.caret_offset);

    // There can be more than a single token in a candidate (e.g. for GROUP BY).
    // But if there are more than one we always have a keyword list.
    std::vector<std::string> entries = base::split( *i, " ");
    if (entries.size() > 1 || ends_with(*i, "_SYMBOL"))
    {
      std::string entry;
      for (std::vector<std::string>::const_iterator j = entries.begin(); j != entries.end(); ++j)
      {
        if (base::ends_with(*j, "_SYMBOL"))
        {
          // A single keyword or something in a keyword sequence. Convert the entry to a readable form.
          std::string token = j->substr(0, j->size() - 7);
          if (token == "OPEN_PAR")
          {
            // Part of a runtime function call or special constructs like PROCEDURE ANALYSE ().
            // Make the entry a function call in the list if there's only one keyword involved.
            // Otherrwise ignore the parentheses.
            if (entries.size() < 3)
            {
              entry = base::tolower(entry);
              entry += "()";

              // Parentheses are always at the end.
              runtime_function_entries.insert(std::make_pair(AC_FUNCTION_IMAGE, entry));
              entry = "";
              break;
            }
          }
          else if (token == "JSON_SEPARATOR")
          {
            keyword_entries.insert(std::make_pair(AC_OPERATOR_IMAGE, "->"));
          }
          else
          {
            if (!entry.empty())
              entry += " ";

            if (uppercase_keywords)
              entry += token;
            else
              entry += base::tolower(token);
          }
        }
        else if (ends_with(*j, "_OPERATOR"))
        {
          // Something that we accept in a keyword sequence, not standalone (very small set).
          if (*j == "EQUAL_OPERATOR")
            entry += " =";
        }
      }
      if (!entry.empty())
        keyword_entries.insert(std::make_pair(AC_KEYWORD_IMAGE, entry));
    }
    else if (rules_holder.special_rules.find(*i) != rules_holder.special_rules.end())
    {
      // Any of the special rules.
      if (_editor_config != NULL && _auto_completion_cache != NULL)
      {
        std::map<std::string, std::string> keyword_map = _editor_config->get_keywords();
        if (*i == "runtime_function_call")
        {
          log_debug3("Adding runtime function names\n");

          std::vector<std::string> functions = base::split_by_set(keyword_map["Functions"], " \t\n");
          for (std::vector<std::string>::const_iterator function = functions.begin(); function != functions.end(); ++function)
            runtime_function_entries.insert(std::make_pair(AC_FUNCTION_IMAGE, *function + "()"));
        }
        else if (*i == "udf_call")
        {
          log_debug3("Adding user defined function names from cache\n");

          std::vector<std::string> functions = _auto_completion_cache->get_matching_udf_names(context.typed_part);
          for (std::vector<std::string>::const_iterator function = functions.begin(); function != functions.end(); ++function)
            runtime_function_entries.insert(std::make_pair(AC_FUNCTION_IMAGE, *function + "()"));
        }
        else if (*i == "engine_ref")
        {
          log_debug3("Adding engine names\n");

          std::vector<std::string> engines = _auto_completion_cache->get_matching_engines(context.typed_part);
          for (std::vector<std::string>::const_iterator engine = engines.begin(); engine != engines.end(); ++engine)
            engine_entries.insert(std::make_pair(AC_ENGINE_IMAGE, *engine));
        }
        else if (*i == "schema_ref")
        {
          log_debug3("Adding schema names from cache\n");
          insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);
        }
        else if (*i == "procedure_ref")
        {
          log_debug3("Adding procedure names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = _current_schema;

            std::vector<std::string> procedures = _auto_completion_cache->get_matching_procedure_names(qualifier, context.typed_part);
            for (std::vector<std::string>::const_iterator procedure = procedures.begin(); procedure != procedures.end(); ++procedure)
              procedure_entries.insert(std::make_pair(AC_ROUTINE_IMAGE, *procedure));
          }
        }
        else if (*i == "function_ref" || *i == "stored_function_call")
        {
          log_debug3("Adding function names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = _current_schema;

            std::vector<std::string> functions = _auto_completion_cache->get_matching_function_names(qualifier, context.typed_part);
            for (std::vector<std::string>::const_iterator function = functions.begin(); function != functions.end(); ++function)
              function_entries.insert(std::make_pair(AC_ROUTINE_IMAGE, *function));
          }
        }
        else if (*i == "table_ref_with_wildcard")
        {
          // A special form of table references (id.id.*) used only in multi-table delete.
          // Handling is similar as for column references (just that we have table/view objects instead of column refs).
          log_debug3("Adding table + view names from cache\n");

          std::string schema, table;
          ObjectFlags flags = determine_schema_table_qualifier(scanner, schema, table);
          if ((flags & ShowSchemas) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          std::set<std::string> schemas;
          schemas.insert(schema.empty() ? _current_schema : schema);
          if ((flags & ShowTables) != 0)
          {
            insertTables(_auto_completion_cache, table_entries, schemas, context.typed_part);
            insertViews(_auto_completion_cache, view_entries, schemas, context.typed_part);
          }
        }
        else if (*i == "table_ref" || *i == "filter_table_ref" || *i == "table_ref_no_db")
        {
          log_debug3("Adding table + view names from cache\n");

          // Tables refs also allow view refs.
          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            std::set<std::string> schemas;
            schemas.insert(qualifier.empty() ? _current_schema : qualifier);

            insertTables(_auto_completion_cache, table_entries, schemas, context.typed_part);
            insertViews(_auto_completion_cache, view_entries, schemas, context.typed_part);
          }
        }
        else if (*i == "column_ref" || *i == "column_internal_ref" || *i == "column_ref_with_wildcard" || *i == "table_wild")
        {
          log_debug3("Adding column names from cache\n");

          // Try limiting what to show to the smallest set possible.
          // If we have table references show columns only from them.
          // Show columns from the default schema only if there are no references.
          std::string schema, table;
          ObjectFlags flags = determine_schema_table_qualifier(scanner, schema, table);
          if ((flags & ShowSchemas) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          // If a schema is given then list only tables + columns from that schema.
          // If no schema is given but we have table references use the schemas from them.
          // Otherwise use the default schema.
          // TODO: case sensitivity.
          std::set<std::string> schemas;

          if (!schema.empty())
            schemas.insert(schema);
          else
            if (!context.references.empty())
            {
              for (size_t i = 0; i < context.references.size(); ++ i)
              {
                if (!context.references[i].schema.empty())
                  schemas.insert(context.references[i].schema);
              }
            }

          if (schemas.empty())
            schemas.insert(_current_schema);

          if ((flags & ShowTables) != 0)
          {
            insertTables(_auto_completion_cache, table_entries, schemas, context.typed_part);
            if (*i == "column_ref")
            {
              // Insert also views.
              insertViews(_auto_completion_cache, view_entries, schemas, context.typed_part);

              // Insert also tables from our references list.
              for (std::vector<TableReference>::const_iterator iterator = context.references.begin();
                   iterator != context.references.end(); ++iterator)
              {
                // If no schema was specified then allow also tables without a given schema. Otherwise
                // the reference's schema must match any of the specified schemas (which include those from the ref list).
                if ((schema.empty() && iterator->schema.empty()) || (schemas.count(iterator->schema) > 0))
                  table_entries.insert(std::make_pair(AC_TABLE_IMAGE, iterator->alias.empty() ? iterator->table : iterator->alias));
              }
            }
          }

          if ((flags & ShowColumns) != 0)
          {
            if (schema == table) // Schema and table are equal if it's not clear if we see a schema or table qualfier.
              schemas.insert(_current_schema);
            
            // For the columns we use a similar approach like for the schemas.
            // If a table is given, list only columns from this (use the set of schemas from above).
            // If not and we have table references then show columns from them.
            // Otherwise show no columns.
            std::set<std::string> tables;
            if (!table.empty())
            {
              tables.insert(table);

              // Could be an alias.
              for (size_t i = 0; i < context.references.size(); ++ i)
                if (base::same_string(table, context.references[i].alias))
                {
                  tables.insert(context.references[i].table);
                  break;
                }
            }
            else
              if (!context.references.empty() && *i == "column_ref")
              {
                for (size_t i = 0; i < context.references.size(); ++ i)
                  tables.insert(context.references[i].table);
              }

            if (!tables.empty())
            {
              insertColumns(_auto_completion_cache, column_entries, schemas, tables, context.typed_part);
            }

            // Special deal here: triggers. Show columns for the "new" and "old" qualifiers too.
            // Use the first reference in the list, which is the table to which this trigger belongs (there can be more
            // if the trigger body references other tables).
            if (queryType == QtCreateTrigger && !context.references.empty() && (base::same_string(table, "old") || base::same_string(table, "new")))
            {
              tables.clear();
              tables.insert(context.references[0].table);
              insertColumns(_auto_completion_cache, column_entries, schemas, tables, context.typed_part);
            }
          }
        }
        else if (*i == "trigger_ref")
        {
          // Trigger references only consist of a table name and the trigger name.
          // However we have to make sure to show only triggers from the current schema.
          log_debug3("Adding trigger names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          std::set<std::string> schemas;
          schemas.insert(_current_schema);

          if ((flags & ShowFirst) != 0)
            insertTables(_auto_completion_cache, schema_entries, schemas, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            std::vector<std::string> triggers = _auto_completion_cache->get_matching_trigger_names(_current_schema, qualifier, context.typed_part);
            for (std::vector<std::string>::const_iterator trigger = triggers.begin(); trigger != triggers.end(); ++trigger)
              trigger_entries.insert(std::make_pair(AC_TRIGGER_IMAGE, *trigger));
          }
        }
        else if (*i == "view_ref")
        {
          log_debug3("Adding view names from cache\n");

          // View refs only (no table refers), e.g. like in DROP VIEW ...
          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = _current_schema;

            std::vector<std::string> views = _auto_completion_cache->get_matching_view_names(qualifier, context.typed_part);
            for (std::vector<std::string>::const_iterator view = views.begin(); view != views.end(); ++view)
              view_entries.insert(std::make_pair(AC_VIEW_IMAGE, *view));
          }
        }
        else if (*i == "logfile_group_ref")
        {
          log_debug3("Adding logfile group names from cache\n");

          std::vector<std::string> logfile_groups = _auto_completion_cache->get_matching_logfile_groups(context.typed_part);
          for (std::vector<std::string>::const_iterator logfile_group = logfile_groups.begin(); logfile_group != logfile_groups.end(); ++logfile_group)
            logfile_group_entries.insert(std::make_pair(AC_LOGFILE_GROUP_IMAGE, *logfile_group));
        }
        else if (*i == "tablespace_ref")
        {
          log_debug3("Adding tablespace names from cache\n");

          std::vector<std::string> tablespaces = _auto_completion_cache->get_matching_tablespaces(context.typed_part);
          for (std::vector<std::string>::const_iterator tablespace = tablespaces.begin(); tablespace != tablespaces.end(); ++tablespace)
            tablespace_entries.insert(std::make_pair(AC_TABLESPACE_IMAGE, *tablespace));
        }
        else if (*i == "user_variable")
        {
          log_debug3("Adding user variables\n");
          user_var_entries.insert(std::make_pair(AC_USER_VAR_IMAGE, "<user variable>"));
        }
        else if (*i == "system_variable")
        {
          log_debug3("Adding system variables\n");

          std::vector<std::string> variables = _auto_completion_cache->get_matching_variables(context.typed_part);
          for (std::vector<std::string>::const_iterator variable = variables.begin(); variable != variables.end(); ++variable)
            system_var_entries.insert(std::make_pair(AC_SYSTEM_VAR_IMAGE, *variable));
        }
        else if (*i == "charset_name")
        {
          log_debug3("Adding charsets\n");

          std::vector<std::string> charsets = _auto_completion_cache->get_matching_charsets(context.typed_part);
          for (std::vector<std::string>::const_iterator charset = charsets.begin(); charset != charsets.end(); ++charset)
            charset_entries.insert(std::make_pair(AC_CHARSET_IMAGE, *charset));
        }
        else if (*i == "collation_name")
        {
          log_debug3("Adding collations\n");

          std::vector<std::string> collations = _auto_completion_cache->get_matching_collations(context.typed_part);
          for (std::vector<std::string>::const_iterator collation = collations.begin(); collation != collations.end(); ++collation)
            collation_entries.insert(std::make_pair(AC_COLLATION_IMAGE, *collation));
        }
        else if (*i == "event_ref")
        {
          log_debug3("Adding events\n");

          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(_auto_completion_cache, schema_entries, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = _current_schema;

            std::vector<std::string> events = _auto_completion_cache->get_matching_events(qualifier, context.typed_part);
            for (std::vector<std::string>::const_iterator event = events.begin(); event != events.end(); ++event)
              view_entries.insert(std::make_pair(AC_EVENT_IMAGE, *event));
          }
        }
        else
          keyword_entries.insert(std::make_pair(0, *i));
      }
    }
    else
    {
      // Simply take over anything else. There should never been anything but keywords and special rules.
      // By adding the raw token/rule entry we can better find the bug, which must be the cause
      // for this addition.
      keyword_entries.insert(std::make_pair(0, *i));
    }
  }

  // Insert the groups "inside out", that is, most likely ones first + most inner first (columns before tables etc).
  std::copy(keyword_entries.begin(), keyword_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(column_entries.begin(), column_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(table_entries.begin(), table_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(view_entries.begin(), view_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(schema_entries.begin(), schema_entries.end(), std::back_inserter(_auto_completion_entries));

  // Everything else is significantly less used.
  // TODO: make this configurable.
  // TODO: show an optimized (small) list of candidates on first invocation, a full list on every following.
  std::copy(function_entries.begin(), function_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(procedure_entries.begin(), procedure_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(trigger_entries.begin(), trigger_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(index_entries.begin(), index_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(event_entries.begin(), event_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(user_entries.begin(), user_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(engine_entries.begin(), engine_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(plugin_entries.begin(), plugin_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(logfile_group_entries.begin(), logfile_group_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(tablespace_entries.begin(), tablespace_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(charset_entries.begin(), charset_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(collation_entries.begin(), collation_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(user_var_entries.begin(), user_var_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(runtime_function_entries.begin(), runtime_function_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(system_var_entries.begin(), system_var_entries.end(), std::back_inserter(_auto_completion_entries));

  update_auto_completion(context.typed_part);
}

//--------------------------------------------------------------------------------------------------

/**
 * The auto completion cache is connection dependent so it must be set by the owner of the editor
 * if there is a connection at all. Ownership of the cache remains with the owner of the editor.
 */
void MySQLEditor::set_auto_completion_cache(AutoCompleteCache *cache)
{
  log_debug2("Auto completion cache set to: %p\n", cache);

  _auto_completion_cache = cache;
}

//--------------------------------------------------------------------------------------------------
