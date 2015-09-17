/* 
 * Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.
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
  uint32_t token_ref;   // In case of a terminal the id of the token.
  std::string rule_ref; // In case of a non-terminal the name of the rule.

  GrammarNode()
  {
    is_terminal = true;
    is_required = true;
    multiple = false;
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

typedef std::vector<GrammarSequence> RuleAlternatives; // A list of alternatives for a given rule.

//--------------------------------------------------------------------------------------------------

// A shared data structure for a given grammar file + some additional parsing info.
static struct
{
  std::map<std::string, RuleAlternatives> rules; // The full grammar.
  std::map<std::string, uint32_t> token_map;     // Map token names to token ids.

  // Rules that must not be examined further when collecting candidates.
  std::set<std::string> special_rules;  // Rules with a special meaning (e.g. "table_ref").
  std::set<std::string> ignored_rules;  // Rules we don't provide completion with (e.g. "literal").
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

    special_rules.insert("user_variable");
    special_rules.insert("system_variable");

    ignored_rules.clear();
    ignored_rules.insert("literal");
    ignored_rules.insert("text_or_identifier");
    ignored_rules.insert("identifier");

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
    ignored_tokens.insert("SINGLE_QUOTED_TEXT");
    ignored_tokens.insert("DOUBLE_QUOTED_TEXT");
    ignored_tokens.insert("NCHAR_TEXT");

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

    log_debug("Parsing grammar...");

    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    pANTLR3_INPUT_STREAM input = antlr3StringStreamNew((pANTLR3_UINT8)text.c_str(), ANTLR3_ENC_UTF8,
                                                       (ANTLR3_UINT32)text.size(), (pANTLR3_UINT8)"");
    pANTLRv3Lexer lexer = ANTLRv3LexerNew(input);
    pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
    pANTLRv3Parser parser = ANTLRv3ParserNew(tokens);

    pANTLR3_BASE_TREE tree = parser->grammarDef(parser).tree;

    if (parser->pParser->rec->state->errorCount > 0)
      log_error("Found grammar errors. No code completion data available.");
    else
    {
      //std::string dump = dumpTree(tree, parser->pParser->rec->state, "");
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
      case GATED_SEMPRED_V3TOK:
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

      case SEMPRED_V3TOK:     // A normal semantic predicate.
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
              switch (child->getType(child))
              {
                case TOKEN_REF:
                {
                  node.is_terminal = true;
                  pANTLR3_STRING token_text = child->getText(child);
                  std::string name = (char*)token_text->chars;
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

        case TOKEN_REF:
        {
          node.is_terminal = true;
          pANTLR3_STRING token_text = child->getText(child);
          std::string name = (char*)token_text->chars;
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

    // One less child in the loop as the list is always ended by a EOB node.
    for (ANTLR3_UINT32 index = 0; index < block->getChildCount(block) - 1; index++)
    {
      pANTLR3_BASE_TREE alt = (pANTLR3_BASE_TREE)block->getChild(block, index);
      if (alt->getType(alt) == ALT_V3TOK) // There can be REWRITE nodes (which we don't need).
      {
        std::stringstream alt_name;
        alt_name << name << "_alt" << index;
        GrammarSequence sequence = traverse_alternative(alt, alt_name.str());
        alternatives.push_back(sequence);
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

  enum RunState { RunStateMatching, RunStateCollectionPending, RunStateCollectionDone } run_state;

  MySQLScanner *scanner;
  std::set<std::string> completion_candidates;

  size_t caret_line;
  size_t caret_offset;

  std::vector<TableReference> references; // As in FROM, UPDATE etc.

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
  bool collect_candiates(MySQLScanner *aScanner)
  {
    scanner = aScanner; // Has all the data necessary for scanning already.
    server_version = scanner->get_server_version();
    sql_mode = scanner->get_sql_mode_flags();

    run_state = RunStateMatching;
    completion_candidates.clear();

    if (scanner->token_channel() != 0)
      scanner->next(true);

    bool matched = match_rule("query");

    // Post processing some entries.
    if (completion_candidates.find("NOT2_SYMBOL") != completion_candidates.end())
    {
      // NOT2 is a NOT with special meaning in the operator preceedence chain.
      // For code completion it's the same as NOT.
      completion_candidates.erase("NOT2_SYMBOL");
      completion_candidates.insert("NOT_SYMBOL");
    }

    if (completion_candidates.find("column_ref") != completion_candidates.end())
      collectTableReferences();

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
          matched = node.token_ref == scanner->token_type();
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
        if (scanner->token_type() != ANTLR3_TOKEN_EOF && node.multiple)
        {
          do
          {
            if (node.is_terminal)
            {
              matched = node.token_ref == scanner->token_type();
              scanner->next(true);
            }
            else
              matched = matchRuleAndCollectTableRefs(node.rule_ref);
          } while (matched);

          if (scanner->token_type() == ANTLR3_TOKEN_EOF)
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
    walk_stack.push_back(rule);

    size_t marker = scanner->position();
    RuleAlternatives alts = rules_holder.rules[rule];
    for (std::vector<GrammarSequence>::const_iterator i = alts.begin(); i != alts.end(); ++i)
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
        walk_stack.pop_back();

        // If that was the table_alias rule then we can look back for which table (or subquery)
        // it was set and collect the values.
        if (rule == "table_alias")
        {
          // The current scanner position is after the alias, so we can seek back straight to the
          // needed values. Need to hard code grammar knowledge here, for this to work however.
          marker = scanner->position();

          TableReference reference;
          scanner->previous(true);
          reference.alias = scanner->token_text();
          scanner->previous(true);
          if (scanner->token_type() == AS_SYMBOL || scanner->token_type() == EQUAL_OPERATOR)
            scanner->previous(true); // Skip AS and = operators if they exist.

          if (scanner->token_type() == CLOSE_PAR_SYMBOL)
          {
            // There can be a partition list between table reference and alias, which we have
            // to skip too. Since there can only be an identifier list and we have valid syntax
            // to even arrive here, we can simply scan back to the open par and be done.
            do
            {
              scanner->previous(true);
            } while (scanner->token_type() != OPEN_PAR_SYMBOL);

            // Skip open par and PARTITION.
            scanner->previous(true);
            scanner->previous(true);
          }

          // Finally arrived at the table reference.
          reference.table = scanner->token_text();
          scanner->previous(true);
          if (scanner->token_type() == DOT_SYMBOL)
          {
            // Might have a schema part (or just a leading dot).
            scanner->previous(true);
            if (scanner->token_type() == IDENTIFIER)
              reference.schema = scanner->token_text();
          }


          references.push_back(reference);
          scanner->seek(marker);
        }
        return true;
      }

      scanner->seek(marker);
    }

    walk_stack.pop_back();
    return false;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Called if one of the candidates is a column_ref before a FROM keyword (i.e. a select item).
   * The function attempts to get table references together with aliases where possible.
   * Since it is not required that the sql code must be valid after the caret we scan only as far
   * as we can match the input to the grammar.
   */
  void collectTableReferences()
  {
    // First advance to the FROM keyword on the same level as we are (no subselects etc.).
    size_t level = 0;
    bool done = false;
    while (!done)
    {
      switch (scanner->token_type())
      {
        case FROM_SYMBOL:
          if (level > 0)
          {
            scanner->next(true);
            break;
          }
          // Fall through.

        case ANTLR3_TOKEN_EOF:
          done = true;
          break;

        case OPEN_PAR_SYMBOL:
          ++level;
          scanner->next(true);
          break;

        case CLOSE_PAR_SYMBOL:
          // We can act relaxed here, even if the number of opening and closing
          // parentheses doesn't match.
          // Additionally, it can happen we are called with any number of opening pars before
          // the caret position.
          if (level > 0)
            --level;
          scanner->next(true);
          break;

        default:
          scanner->next(true);
      }
    }

    if (scanner->token_type() != FROM_SYMBOL)
      return; // Something is wrong (but not critical), e.g. end of input .

    scanner->next(true);

    if (scanner->token_type() == DUAL_SYMBOL)
      return; // Nothing to do.

    walk_stack.clear();
    references.clear();
    matchRuleAndCollectTableRefs("join_table_list");
  }

  //------------------------------------------------------------------------------------------------
  
  bool is_token_end_after_caret()
  {
    // The first time we found that the caret is before the current token we store the
    // current position of the input stream to return to it after we collected all candidates.
    if (scanner->token_type() == ANTLR3_TOKEN_EOF)
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
        break;

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
          // Don't do this however, if we have an ignored token here.
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
          run_state = RunStateCollectionDone;
          return;
        }
      }
      else
      {
        collect_from_rule(node.rule_ref);
        if (!node.is_required)
          run_state = RunStateCollectionPending;
        else
          if (run_state != RunStateCollectionPending)
            run_state = RunStateCollectionDone;
        if (node.is_required && run_state == RunStateCollectionDone)
          return;
      }
    }
    run_state = RunStateCollectionPending; // Parent needs to continue.
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
      run_state = RunStateCollectionDone;
      return;
    }

    // If this is an ignored rule see if we are in a path that involves a special rule
    // and use this if found.
    if (rules_holder.ignored_rules.find(rule) != rules_holder.ignored_rules.end())
    {
      for (std::deque<std::string>::const_reverse_iterator i = walk_stack.rbegin(); i != walk_stack.rend(); ++i)
      {
        if (rules_holder.special_rules.find(*i) != rules_holder.special_rules.end())
        {
          completion_candidates.insert(*i);
          run_state = RunStateCollectionDone;
          break;
        }
      }
      return;
    }

    // Any other rule goes here.
    RunState combined_state = RunStateCollectionDone;
    RuleAlternatives alts = rules_holder.rules[rule];
    for (std::vector<GrammarSequence>::const_iterator i = alts.begin(); i != alts.end(); ++i)
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
      return node.token_ref == token_type;
    else
      return match_rule(node.rule_ref);
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
            collect_from_alternative(sequence, i + 1);

            /* This part looks suspicious as it inserts special rules even if they don't match.

            // If we just started collecting it might be we are in a special rule.
            // Check the end of the stack and if so push the rule name to the candidates.
            // Duplicates will be handled automatically.
            if (rules_holder.special_rules.find(walk_stack.back()) != rules_holder.special_rules.end())
              completion_candidates.insert(walk_stack.back());
             */
          }
          return false;
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

      if (matched)
      {
        // Load next token if the grammar node is a terminal node.
        // Otherwise the match() call will have advanced the input position already.
        if (node.is_terminal)
        {
          scanner->next(true);
          if (is_token_end_after_caret())
          {
            collect_from_alternative(sequence, i + 1);
            return false;
          }
        }

        // If the current grammar node can be matched multiple times try as often as you can.
        // This is the greedy approach and default in ANTLR. At the moment we don't support non-greedy matches
        // as we don't use them in MySQL parser rules.
        if (scanner->token_type() != ANTLR3_TOKEN_EOF && node.multiple)
        {
          while (match(node, scanner->token_type()))
          {
            if (run_state != RunStateMatching)
            {
              if (run_state == RunStateCollectionPending)
                collect_from_alternative(sequence, i + 1);
              return false;
            }

            if (node.is_terminal)
            {
              scanner->next(true);
              if (is_token_end_after_caret())
              {
                collect_from_alternative(sequence, i + 1);
                return false;
              }
            }
            if (scanner->token_type() == ANTLR3_TOKEN_EOF)
              break;
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
    if (run_state != RunStateMatching) // Sanity check - should never happen at this point.
      return false;

    if (is_token_end_after_caret())
    {
      collect_from_rule(rule);
      return false;
    }

    walk_stack.push_back(rule);

    // The first alternative that matches wins.
    RuleAlternatives alts = rules_holder.rules[rule];
    bool can_seek = false;

    size_t highest_token_index = 0;

    RunState result_state = run_state;
    for (size_t i = 0; i < alts.size(); ++i)
    {
      // First run a predicate check if this alt can be considered at all.
      GrammarSequence alt = alts[i];
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
      if (match_alternative(alt) || run_state != RunStateMatching)
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

    run_state = result_state;

    walk_stack.pop_back();
    return can_seek && result_state == RunStateMatching;
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
  
  // If we come here then we are outside any quoted text. Scan back for anything we consider
  // to be a word stopper (for now anything below '0', char code wise).
  while (head < run--)
  {
    if (*run < '0')
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
ObjectFlags determine_qualifier(MySQLScanner *scanner, std::string &qualifier)
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

  if (scanner->token_type() != DOT_SYMBOL && !scanner->is_identifier())
  {
    // We are at the end of an incomplete identifier spec. Jump back, so that the other tests succeed.
    scanner->previous(true);
  }

  // Go left until we find something not related to an id or find at most 1 dot.
  if (position > 0)
  {
    if (scanner->is_identifier() && scanner->look_around(-1, true) == DOT_SYMBOL)
      scanner->previous(true);
    if (scanner->token_type() == DOT_SYMBOL && scanner->MySQLRecognitionBase::is_identifier(scanner->look_around(-1, true)))
      scanner->previous(true);
  }

  // The scanner is now on the leading identifier or dot (if there's no leading id).
  qualifier = "";
  std::string temp;
  if (scanner->is_identifier())
  {
    temp = scanner->token_text();
    scanner->next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret position.
  if (scanner->token_type() != DOT_SYMBOL || position <= scanner->position())
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
ObjectFlags determine_schema_table_qualifier(MySQLScanner *scanner, std::string &schema, std::string &table)
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
    if (scanner->token_type() == DOT_SYMBOL && scanner->MySQLRecognitionBase::is_identifier(scanner->look_around(-1, true)))
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
    temp = scanner->token_text();
    scanner->next(true);
  }

  // Bail out if there is no more id parts or we are already behind the caret position.
  if (scanner->token_type() != DOT_SYMBOL || position <= scanner->position())
    return ObjectFlags(ShowSchemas | ShowTables | ShowColumns);

  scanner->next(true); // Skip dot.
  table = temp;
  schema = temp;
  if (scanner->is_identifier())
  {
    temp = scanner->token_text();
    scanner->next(true);

    if (scanner->token_type() != DOT_SYMBOL || position <= scanner->position())
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

void insert_schemas(AutoCompleteCache *cache, CompletionSet &set, const std::string &typed_part)
{
  std::vector<std::string> schemas = cache->get_matching_schema_names(typed_part);
  for (std::vector<std::string>::const_iterator schema = schemas.begin(); schema != schemas.end(); ++schema)
    set.insert(std::make_pair(AC_SCHEMA_IMAGE, *schema));
}

//--------------------------------------------------------------------------------------------------

void insert_tables(AutoCompleteCache *cache, CompletionSet &set, const std::string &schema,
  const std::string &typed_part)
{
  std::vector<std::string> tables = cache->get_matching_table_names(schema, typed_part);
  for (std::vector<std::string>::const_iterator table = tables.begin(); table != tables.end(); ++table)
    set.insert(std::make_pair(AC_TABLE_IMAGE, *table));
}

//--------------------------------------------------------------------------------------------------

void insert_views(AutoCompleteCache *cache, CompletionSet &set, const std::string &schema,
  const std::string &typed_part)
{
  std::vector<std::string> views = cache->get_matching_view_names(schema, typed_part);
  for (std::vector<std::string>::const_iterator view = views.begin(); view != views.end(); ++view)
    set.insert(std::make_pair(AC_VIEW_IMAGE, *view));
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
  CompletionSet user_var_entries;
  CompletionSet system_var_entries;
  CompletionSet keyword_entries;

  // To be done yet.
  CompletionSet collation_entries;
  CompletionSet charset_entries;
  CompletionSet user_entries;
  CompletionSet index_entries;
  CompletionSet event_entries;
  CompletionSet plugin_entries;

  _auto_completion_entries.clear();

  bool uppercase_keywords = make_keywords_uppercase();
  MySQLScanner *scanner = parser_context->create_scanner(statement);
  context.server_version = scanner->get_server_version();
  context.collect_candiates(scanner);

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
        if (ends_with(*j, "_SYMBOL"))
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
        if (*i == "udf_call")
        {
          log_debug3("Adding runtime function names to code completion list\n");

          std::vector<std::string> functions = base::split_by_set(keyword_map["Functions"], " \t\n");
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
          insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);
        }
        else if (*i == "procedure_ref")
        {
          log_debug3("Adding procedure names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);

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
            insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);

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
            insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);

          if (schema.empty())
            schema = _current_schema;
          if ((flags & ShowTables) != 0)
          {
            insert_tables(_auto_completion_cache, table_entries, schema, context.typed_part);
            insert_views(_auto_completion_cache, view_entries, schema, context.typed_part);
          }
        }
        else if (*i == "table_ref" || *i == "filter_table_ref" || *i == "table_ref_no_db")
        {
          log_debug3("Adding table + view names from cache\n");

          // Tables refs also allow view refs.
          std::string qualifier;
          ObjectFlags flags = determine_qualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = _current_schema;

            insert_tables(_auto_completion_cache, table_entries, qualifier, context.typed_part);
            insert_views(_auto_completion_cache, view_entries, qualifier, context.typed_part);
          }
        }
        else if (*i == "column_ref" || *i == "column_ref_with_wildcard" || *i == "table_wild")
        {
          log_debug3("Adding column names from cache\n");

          std::string schema, table;
          ObjectFlags flags = determine_schema_table_qualifier(scanner, schema, table);
          if ((flags & ShowSchemas) != 0)
            insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);

          if (schema.empty())
            schema = _current_schema;
          if ((flags & ShowTables) != 0)
          {
            insert_tables(_auto_completion_cache, table_entries, schema, context.typed_part);
            if (*i == "column_ref")
            {
              // Insert also views.
              insert_views(_auto_completion_cache, view_entries, schema, context.typed_part);

              // Insert also tables from our references list (which is a collection of table names
              // with aliases).
              for (size_t i = 0; i < context.references.size(); ++i)
              {
                TableReference reference = context.references[i];
                std::string used_schema = schema;
                if (reference.schema.empty())
                  used_schema = _current_schema;
                if (reference.schema == used_schema)
                  table_entries.insert(std::make_pair(AC_TABLE_IMAGE, reference.alias));
              }
            }
          }

          if ((flags & ShowColumns) != 0)
          {
            // If we only show columns then we can use the given schema.
            // Otherwise we don't have an explicit schema and use the default one.
            if (flags != ShowColumns)
              schema = _current_schema;
            std::vector<std::string> columns = _auto_completion_cache->get_matching_column_names(schema, table, context.typed_part);
            for (std::vector<std::string>::const_iterator column = columns.begin(); column != columns.end(); ++column)
              column_entries.insert(std::make_pair(AC_COLUMN_IMAGE, *column));

            if (*i == "column_ref")
            {
              // Insert also columns from matching tables from our references list.
              for (size_t i = 0; i < context.references.size(); ++i)
              {
                TableReference reference = context.references[i];
                if ((reference.schema.empty() || reference.schema == schema)
                    && (reference.alias == table))
                {
                  columns = _auto_completion_cache->get_matching_column_names(schema, reference.table, context.typed_part);
                  for (std::vector<std::string>::const_iterator column = columns.begin(); column != columns.end(); ++column)
                    column_entries.insert(std::make_pair(AC_COLUMN_IMAGE, *column));
                }
              }
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

          if ((flags & ShowFirst) != 0)
            insert_tables(_auto_completion_cache, schema_entries, _current_schema, context.typed_part);

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
            insert_schemas(_auto_completion_cache, schema_entries, context.typed_part);

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

  std::copy(keyword_entries.begin(), keyword_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(schema_entries.begin(), schema_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(table_entries.begin(), table_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(view_entries.begin(), view_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(function_entries.begin(), function_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(procedure_entries.begin(), procedure_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(trigger_entries.begin(), trigger_entries.end(), std::back_inserter(_auto_completion_entries));
  std::copy(column_entries.begin(), column_entries.end(), std::back_inserter(_auto_completion_entries));
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

  delete scanner;

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
