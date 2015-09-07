/*
 * Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#ifndef HAVE_PRECOMPILED_HEADERS
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>

#include "antlr3.h"
#include <glib.h>
#endif

#include "base/common.h"
#include "base/log.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/threading.h"

#include "parsers-common.h"
#include "grammar-parser/ANTLRv3Lexer.h"
#include "grammar-parser/ANTLRv3Parser.h"
#include "mysql-scanner.h"
#include "MySQLLexer.h"

#include "mysql_object_names_cache.h"

#include "mysql-code-completion.h"

DEFAULT_LOG_DOMAIN("MySQL code completion");

//--------------------------------------------------------------------------------------------------

extern "C" pANTLR3_UINT8 MySQLParserTokenNames[]; // Defined in MySQLParser.

struct MySQLGrammarNode {
  bool isTerminal;
  bool isRequired;     // false for * and ? operators, otherwise true.
  bool multiple;        // true for + and * operators, otherwise false.
  uint32_t tokenRef;   // In case of a terminal the id of the token.
  std::string ruleRef; // In case of a non-terminal the name of the rule.

  MySQLGrammarNode()
  {
    isTerminal = true;
    isRequired = true;
    multiple = false;
    tokenRef = INVALID_TOKEN;
  }
};

// A sequence of grammar nodes (either terminal or non-terminal) in the order they appear in the grammar.
// Expressions in parentheses are extracted into an own rule with a private name.
// A sequence can have an optional predicate (min/max server version and/or sql modes active/not active).
struct MySQLGrammarSequence {
  // Version predicate. Values are inclusive (min <= x <= max).
  int minVersion;
  int maxVersion;

  // Bit mask of modes as defined in the lexer/parser.
  int activeSqlModes;   // SQL modes that must be active to match the predicate.
  int inactiveSqlModes; // SQL modes that must not be active. -1 for both if we don't care.

  std::vector<MySQLGrammarNode> nodes;

  MySQLGrammarSequence()
  {
    minVersion = MIN_SERVER_VERSION;
    maxVersion = MAX_SERVER_VERSION;
    activeSqlModes = -1;
    inactiveSqlModes = -1;
  };

};

typedef std::vector<MySQLGrammarSequence> MySQLRuleAlternatives; // A list of alternatives for a given rule.

//--------------------------------------------------------------------------------------------------

// A shared data structure for a given grammar file + some additional parsing info.
static struct
{
  std::map<std::string, MySQLRuleAlternatives> rules; // The full grammar.
  std::map<std::string, uint32_t> tokenMap;     // Map token names to token ids.

  // Rules that must not be examined further when collecting candidates.
  std::set<std::string> specialRules;  // Rules with a special meaning (e.g. "table_ref").
  std::set<std::string> ignoredRules;  // Rules we don't provide completion with (e.g. "literal").
  std::set<std::string> ignoredTokens; // Tokens we don't want to show up (e.g. operators).

  //------------------------------------------------------------------------------------------------

  // Parses the given grammar file (and its associated .tokens file which must be in the same folder)
  // and fills the rule and token_map structures.
  void parseFile(const std::string &name)
  {
    logDebug("Parsing grammar file: %s\n", name.c_str());

    specialRules.clear();
    specialRules.insert("schema_ref");

    specialRules.insert("table_ref");
    specialRules.insert("table_ref_with_wildcard");
    specialRules.insert("filter_table_ref");
    specialRules.insert("table_ref_no_db");

    specialRules.insert("column_ref");
    specialRules.insert("column_ref_with_wildcard");
    specialRules.insert("table_wild");

    specialRules.insert("function_ref"); // Pure stored function reference.
    specialRules.insert("stored_function_call"); // Stored function call definition.
    specialRules.insert("udf_call");
    specialRules.insert("runtime_function_call");
    specialRules.insert("trigger_ref");
    specialRules.insert("view_ref");
    specialRules.insert("procedure_ref");

    specialRules.insert("logfileGroup_ref");
    specialRules.insert("tablespace_ref");
    specialRules.insert("engine_ref");

    specialRules.insert("userVariable");
    specialRules.insert("systemVariable");

    ignoredRules.clear();
    ignoredRules.insert("literal");
    ignoredRules.insert("text_or_identifier");
    ignoredRules.insert("identifier");

    // We have to use strings for the ignored tokens, instead of their #defines because we would have
    // to include MySQLParser.h, which conflicts with ANTLRv3Parser.h.
    ignoredTokens.clear();
    ignoredTokens.insert("EQUAL_OPERATOR");
    ignoredTokens.insert("ASSIGN_OPERATOR");
    ignoredTokens.insert("NULL_SAFE_EQUAL_OPERATOR");
    ignoredTokens.insert("GREATER_OR_EQUAL_OPERATOR");
    ignoredTokens.insert("GREATER_THAN_OPERATOR");
    ignoredTokens.insert("LESS_OR_EQUAL_OPERATOR");
    ignoredTokens.insert("LESS_THAN_OPERATOR");
    ignoredTokens.insert("NOT_EQUAL_OPERATOR");
    ignoredTokens.insert("NOT_EQUAL2_OPERATOR");
    ignoredTokens.insert("PLUS_OPERATOR");
    ignoredTokens.insert("MINUS_OPERATOR");
    ignoredTokens.insert("MULT_OPERATOR");
    ignoredTokens.insert("DIV_OPERATOR");
    ignoredTokens.insert("MOD_OPERATOR");
    ignoredTokens.insert("LOGICAL_NOT_OPERATOR");
    ignoredTokens.insert("BITWISE_NOT_OPERATOR");
    ignoredTokens.insert("SHIFT_LEFT_OPERATOR");
    ignoredTokens.insert("SHIFT_RIGHT_OPERATOR");
    ignoredTokens.insert("LOGICAL_AND_OPERATOR");
    ignoredTokens.insert("BITWISE_AND_OPERATOR");
    ignoredTokens.insert("BITWISE_XOR_OPERATOR");
    ignoredTokens.insert("LOGICAL_OR_OPERATOR");
    ignoredTokens.insert("BITWISE_OR_OPERATOR");
    ignoredTokens.insert("DOT_SYMBOL");
    ignoredTokens.insert("COMMA_SYMBOL");
    ignoredTokens.insert("SEMICOLON_SYMBOL");
    ignoredTokens.insert("COLON_SYMBOL");
    ignoredTokens.insert("OPEN_PAR_SYMBOL");
    ignoredTokens.insert("CLOSE_PAR_SYMBOL");
    ignoredTokens.insert("OPEN_CURLY_SYMBOL");
    ignoredTokens.insert("CLOSE_CURLY_SYMBOL");
    ignoredTokens.insert("OPEN_BRACKET_SYMBOL");
    ignoredTokens.insert("CLOSE_BRACKET_SYMBOL");
    ignoredTokens.insert("UNDERLINE_SYMBOL");
    ignoredTokens.insert("AT_SIGN_SYMBOL");
    ignoredTokens.insert("AT_AT_SIGN_SYMBOL");
    ignoredTokens.insert("NULL2_SYMBOL");
    ignoredTokens.insert("PARAM_MARKER");
    ignoredTokens.insert("BACK_TICK");
    ignoredTokens.insert("SINGLE_QUOTE");
    ignoredTokens.insert("DOUBLE_QUOTE");
    ignoredTokens.insert("ESCAPE_OPERATOR");
    ignoredTokens.insert("CONCAT_PIPES_SYMBOL");
    ignoredTokens.insert("AT_TEXT_SUFFIX");
    ignoredTokens.insert("SINGLE_QUOTED_TEXT");
    ignoredTokens.insert("DOUBLE_QUOTED_TEXT");
    ignoredTokens.insert("NCHAR_TEXT");

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

        tokenMap[line.substr(0, p)] = atoi(line.substr(p + 1).c_str());
      }
    }
    else
      logError("Token file not found (%s)\n", tokenFileName.c_str());

    tokenMap["EOF"] = ANTLR3_TOKEN_EOF;

    // Now parse the grammar.
    std::ifstream stream(name.c_str(), std::ifstream::binary);
    if (!stream.is_open())
    {
      logError("Grammar file not found\n");
      return;
    }

    logDebug("Parsing grammar...");

    std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());

    pANTLR3_INPUT_STREAM input = antlr3StringStreamNew((pANTLR3_UINT8)text.c_str(), ANTLR3_ENC_UTF8,
      (ANTLR3_UINT32)text.size(), (pANTLR3_UINT8)"");
    pANTLRv3Lexer lexer = ANTLRv3LexerNew(input);
    pANTLR3_COMMON_TOKEN_STREAM tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(lexer));
    pANTLRv3Parser parser = ANTLRv3ParserNew(tokens);

    pANTLR3_BASE_TREE tree = parser->grammarDef(parser).tree;

    if (parser->pParser->rec->state->errorCount > 0)
      logError("Found grammar errors. No code completion data available.");
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
            traverseRule(child);
        }
        break;
      }
    }

    // Must manually clean up.
    parser->free(parser);
    tokens->free(tokens);
    lexer->free(lexer);
    input->close(input);
  }

private:
  void handleServerVersion(std::vector<std::string> parts, MySQLGrammarSequence &sequence)
  {
    bool includesEquality = parts[1].size() == 2;
    int version = atoi(parts[2].c_str());
    switch (parts[1][0])
    {
    case '<': // A max version.
      sequence.maxVersion = version;
      if (!includesEquality)
        --sequence.maxVersion;
      break;
    case '=': // An exact version.
      sequence.maxVersion = version;
      sequence.minVersion = version;
      break;
    case '>': // A min version.
      sequence.minVersion = version;
      if (!includesEquality)
        ++sequence.minVersion;
      break;

    default:
      throw std::runtime_error("Unhandled comparison operator in version number predicate (" + parts[1] + ")");
      break;
    }
  }

  //------------------------------------------------------------------------------------------------

  void parse_predicate(std::string predicate, MySQLGrammarSequence &sequence)
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
      if (base::hasPrefix(expression, "(") && base::hasSuffix(expression, ")"))
        expression = expression.substr(1, expression.size() - 2);
      std::vector<std::string> expression_parts = base::split(expression, " ");
      if ((expression_parts[0] == "SERVER_VERSION") && (expression_parts.size() == 3))
        handleServerVersion(expression_parts, sequence);

      expression = base::trim(parts[1]);
      if (base::hasPrefix(expression, "(") && base::hasSuffix(expression, ")"))
        expression = expression.substr(1, expression.size() - 2);
      expression_parts = base::split(expression, " ");
      if ((expression_parts[0] == "SERVER_VERSION") && (expression_parts.size() == 3))
        handleServerVersion(expression_parts, sequence);
    }
    else
    {
      // A single expression.
      parts = base::split(predicate, " ");
      if (parts.size() == 1)
      {
        if (base::hasPrefix(predicate, "SQL_MODE_ACTIVE("))
        {
          std::string mode = predicate.substr(16, predicate.size() - 17);
          if (mode_map.find(mode) != mode_map.end())
            sequence.activeSqlModes = mode_map[mode];
        }
        else if (base::hasPrefix(predicate, "!SQL_MODE_ACTIVE("))
        {
          std::string mode = predicate.substr(17, predicate.size() - 18);
          if (mode_map.find(mode) != mode_map.end())
            sequence.inactiveSqlModes = mode_map[mode];
        }
      }
      else
      {
        if ((parts[0] == "SERVER_VERSION") && (parts.size() == 3))
          handleServerVersion(parts, sequence);
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
  * Creates a node sequence that comprises an entire alternative.
  */
  MySQLGrammarSequence traverseAlternative(pANTLR3_BASE_TREE alt, const std::string name)
  {
    MySQLGrammarSequence sequence;

    uint32_t index = 0;

    // Check for special nodes first.
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)alt->getChild(alt, index);
    switch (child->getType(child))
    {
    case GATED_SEMPRED_V3TOK:
    {
      // See if we can extract version info or SQL mode condition from that.
      ++index;
      pANTLR3_STRING tokenText = child->getText(child);
      std::string predicate((char*)tokenText->chars);

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
      MySQLGrammarNode node;

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
        node.isRequired = (type != OPTIONAL_V3TOK) && (type != CLOSURE_V3TOK);
        node.multiple = (type == CLOSURE_V3TOK) || (type == POSITIVE_CLOSURE_V3TOK);

        child = (pANTLR3_BASE_TREE)child->getChild(child, 0);

        // See if this block only contains a single alt with a single child node.
        // If so optimize and make this single child node directly the current node.
        bool optimized = false;
        if (child->getChildCount(child) == 2) // 2 because there's always that EOB child node.
        {
          pANTLR3_BASE_TREE childAlt = (pANTLR3_BASE_TREE)child->getChild(child, 0);
          if (childAlt->getChildCount(childAlt) == 2) // 2 because there's always that EOA child node.
          {
            optimized = true;
            child = (pANTLR3_BASE_TREE)childAlt->getChild(childAlt, 0);
            switch (child->getType(child))
            {
            case TOKEN_REF:
            {
              node.isTerminal = true;
              pANTLR3_STRING tokenText = child->getText(child);
              std::string name = (char*)tokenText->chars;
              node.tokenRef = tokenMap[name];
              break;
            }

            case RULE_REF:
            {
              node.isTerminal = false;
              pANTLR3_STRING tokenText = child->getText(child);
              std::string name = (char*)tokenText->chars;
              node.ruleRef = name;
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
          std::stringstream blockName;
          blockName << name << "_block" << index;
          traverseBlock(child, blockName.str());

          node.isTerminal = false;
          node.ruleRef = blockName.str();
        }
        break;
      }

      case TOKEN_REF:
      {
        node.isTerminal = true;
        pANTLR3_STRING tokenText = child->getText(child);
        std::string name = (char*)tokenText->chars;
        node.tokenRef = tokenMap[name];
        break;
      }

      case RULE_REF:
      {
        node.isTerminal = false;
        pANTLR3_STRING tokenText = child->getText(child);
        std::string name = (char*)tokenText->chars;
        node.ruleRef = name;
        break;
      }

      case BLOCK_V3TOK:
      {
        std::stringstream blockName;
        blockName << name << "_block" << index;
        traverseBlock(child, blockName.str());

        node.isTerminal = false;
        node.ruleRef = blockName.str();
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

  void traverseBlock(pANTLR3_BASE_TREE block, const std::string name)
  {
    // A block is either a rule body or a part enclosed by parentheses.
    // A block consists of a number of alternatives which are stored as the content of that block
    // under the given name.

    MySQLRuleAlternatives alternatives;

    // One less child in the loop as the list is always ended by a EOB node.
    for (ANTLR3_UINT32 index = 0; index < block->getChildCount(block) - 1; index++)
    {
      pANTLR3_BASE_TREE alt = (pANTLR3_BASE_TREE)block->getChild(block, index);
      if (alt->getType(alt) == ALT_V3TOK) // There can be REWRITE nodes (which we don't need).
      {
        std::stringstream alt_name;
        alt_name << name << "_alt" << index;
        MySQLGrammarSequence sequence = traverseAlternative(alt, alt_name.str());
        alternatives.push_back(sequence);
      }
    }
    rules[name] = alternatives;
  }

  //------------------------------------------------------------------------------------------------

  void traverseRule(pANTLR3_BASE_TREE rule)
  {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)rule->getChild(rule, 0);
    pANTLR3_STRING tokenText = child->getText(child);
    std::string name((char*)tokenText->chars);

    // Parser rules start with a lower case letter.
    if (islower(name[0]))
    {
      child = (pANTLR3_BASE_TREE)rule->getChild(rule, 1);
      if (child->getType(child) == OPTIONS) // There might be an optional options block on the rule.
        child = (pANTLR3_BASE_TREE)rule->getChild(rule, 2);
      if (child->getType(child) == BLOCK_V3TOK)
        traverseBlock(child, name);
    }

    // There's another child (the always present EOR node) which we ignore.
  }

  //------------------------------------------------------------------------------------------------

} rulesHolder;

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
  std::string typedPart;

  long serverVersion;
  int sqlMode;

  std::deque<std::string> walkStack; // The rules as they are being matched or collected from.
                                     // It's a deque instead of a stack as we need to iterate over it.

  enum RunState { RunStateMatching, RunStateCollectionPending, RunStateCollectionDone } runState;

  std::shared_ptr<MySQLScanner> scanner;
  std::set<std::string> completionCandidates;

  size_t caretLine;
  size_t caretOffset;

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
  bool collectCandiates(std::shared_ptr<MySQLScanner> aScanner)
  {
    scanner = aScanner; // Has all the data necessary for scanning already.
    serverVersion = scanner->get_server_version();
    sqlMode = scanner->get_sql_mode_flags();

    runState = RunStateMatching;
    completionCandidates.clear();

    if (scanner->token_channel() != 0)
      scanner->next(true);

    bool matched = matchRule("query");

    // Post processing some entries.
    if (completionCandidates.find("NOT2_SYMBOL") != completionCandidates.end())
    {
      // NOT2 is a NOT with special meaning in the operator precedence chain.
      // For code completion it's the same as NOT.
      completionCandidates.erase("NOT2_SYMBOL");
      completionCandidates.insert("NOT_SYMBOL");
    }

    return matched;
  }

  //------------------------------------------------------------------------------------------------

private:
  bool isTokenEndAfterCaret()
  {
    // The first time we found that the caret is before the current token we store the
    // current position of the input stream to return to it after we collected all candidates.
    if (scanner->token_type() == ANTLR3_TOKEN_EOF)
      return true;

    assert(scanner->token_line() > 0);
    if (scanner->token_line() > caretLine)
      return true;

    if (scanner->token_line() < caretLine)
      return false;

    // This determination is a bit tricky as it depends on the type of the token.
    // For letters (like when typing a keyword) all positions directly attached to a letter must be
    // considered within the token (as we could extend it).
    // For example each vertical bar is a position within the token: |F|R|O|M|
    // Not so with tokens that can separate other tokens without the need of a whitespace (comma etc.).
    bool result;
    if (scanner->is_separator())
      result = scanner->token_end() > caretOffset;
    else
      result = scanner->token_end() >= caretOffset;

    return result;
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
  * Collects all tokens that can be reached in the sequence from the given start point. There can be more than one
  * if there are optional rules.
  * Returns true if the sequence between the starting point and the end consists only of optional tokens or there aren't
  * any at all.
  */
  void collectFromAlternative(const MySQLGrammarSequence &sequence, size_t start_index)
  {
    for (size_t i = start_index; i < sequence.nodes.size(); ++i)
    {
      MySQLGrammarNode node = sequence.nodes[i];
      if (node.isTerminal && node.tokenRef == ANTLR3_TOKEN_EOF)
        break;

      if (node.isTerminal)
      {
        // Insert only tokens we are interested in.
        std::string token_ref = (char*)MySQLParserTokenNames[node.tokenRef];
        bool ignored = rulesHolder.ignoredTokens.find(token_ref) != rulesHolder.ignoredTokens.end();
        bool exists = completionCandidates.find(token_ref) != completionCandidates.end();
        if (!ignored && !exists)
          completionCandidates.insert(token_ref);
        if (node.isRequired)
        {
          // Also collect following tokens into this candidate, until we find the end of the sequence
          // or a token that is either not required or can appear multiple times.
          // Don't do this however, if we have an ignored token here.
          std::string token_refs = token_ref;
          if (!ignored && !node.multiple)
          {
            while (++i < sequence.nodes.size())
            {
              MySQLGrammarNode node = sequence.nodes[i];
              if (!node.isTerminal || !node.isRequired || node.multiple)
                break;
              token_refs += std::string(" ") + (char *)MySQLParserTokenNames[node.tokenRef];
            }
            if (token_refs.size() > token_ref.size())
            {
              if (!exists)
                completionCandidates.erase(token_ref);
              completionCandidates.insert(token_refs);
            }
          }
          runState = RunStateCollectionDone;
          return;
        }
      }
      else
      {
        collectFromRule(node.ruleRef);
        if (!node.isRequired)
          runState = RunStateCollectionPending;
        else
          if (runState != RunStateCollectionPending)
            runState = RunStateCollectionDone;
        if (node.isRequired && runState == RunStateCollectionDone)
          return;
      }
    }
    runState = RunStateCollectionPending; // Parent needs to continue.
  }

  //----------------------------------------------------------------------------------------------------------------------

  /**
  * Collects possibly reachable tokens from all alternatives in the given rule.
  */
  void collectFromRule(const std::string rule)
  {
    // Don't go deeper if we have one of the special or ignored rules.
    if (rulesHolder.specialRules.find(rule) != rulesHolder.specialRules.end())
    {
      completionCandidates.insert(rule);
      runState = RunStateCollectionDone;
      return;
    }

    // If this is an ignored rule see if we are in a path that involves a special rule
    // and use this if found.
    if (rulesHolder.ignoredRules.find(rule) != rulesHolder.ignoredRules.end())
    {
      for (auto i = walkStack.rbegin(); i != walkStack.rend(); ++i)
      {
        if (rulesHolder.specialRules.find(*i) != rulesHolder.specialRules.end())
        {
          completionCandidates.insert(*i);
          runState = RunStateCollectionDone;
          break;
        }
      }
      return;
    }

    // Any other rule goes here.
    RunState combinedState = RunStateCollectionDone;
    MySQLRuleAlternatives alts = rulesHolder.rules[rule];
    for (auto i = alts.begin(); i != alts.end(); ++i)
    {
      // First run a predicate check if this alt can be considered at all.
      if ((i->minVersion > serverVersion) || (serverVersion > i->maxVersion))
        continue;

      if ((i->activeSqlModes > -1) && (i->activeSqlModes & sqlMode) != i->activeSqlModes)
        continue;

      if ((i->inactiveSqlModes > -1) && (i->inactiveSqlModes & sqlMode) != 0)
        continue;

      collectFromAlternative(*i, 0);
      if (runState == RunStateCollectionPending)
        combinedState = RunStateCollectionPending;
    }
    runState = combinedState;
  }

  //------------------------------------------------------------------------------------------------

  /**
  * Returns true if the given input token matches the given grammar node.
  * This may involve recursive rule matching.
  */
  bool match(const MySQLGrammarNode &node, uint32_t token_type)
  {
    if (node.isTerminal)
      return node.tokenRef == token_type;
    else
      return matchRule(node.ruleRef);
  }

  //----------------------------------------------------------------------------------------------------------------------

  bool matchAlternative(const MySQLGrammarSequence &sequence)
  {
    // An empty sequence per se matches anything without consuming input.
    if (sequence.nodes.empty())
      return true;

    size_t i = 0;
    while (true)
    {
      // Set to true if the current node allows multiple occurrences and was matched at least once.
      bool matchedLoop = false;

      // Skip any optional nodes if they don't match the current input.
      bool matched;
      MySQLGrammarNode node;
      do
      {
        node = sequence.nodes[i];
        matched = match(node, scanner->token_type());

        // If that match call caused the collection to start then don't continue with matching here.
        if (runState != RunStateMatching)
        {
          if (runState == RunStateCollectionPending)
          {
            collectFromAlternative(sequence, i + 1);

            // If we just started collecting it might be we are in a special rule.
            // Check the end of the stack and if so push the rule name to the candidates.
            // Duplicates will be handled automatically.
            if (rulesHolder.specialRules.find(walkStack.back()) != rulesHolder.specialRules.end())
              completionCandidates.insert(walkStack.back());
          }
          return false;
        }

        if (matched && node.multiple)
          matchedLoop = true;

        if (matched || node.isRequired)
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
        if (node.isTerminal)
        {
          scanner->next(true);
          if (isTokenEndAfterCaret())
          {
            collectFromAlternative(sequence, i + 1);
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
            if (runState != RunStateMatching)
            {
              if (runState == RunStateCollectionPending)
                collectFromAlternative(sequence, i + 1);
              return false;
            }

            if (node.isTerminal)
            {
              scanner->next(true);
              if (isTokenEndAfterCaret())
              {
                collectFromAlternative(sequence, i + 1);
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
        if (!matchedLoop)
          return false;
      }

      ++i;
      if (i == sequence.nodes.size())
        break;
    }

    return true;
  }

  //----------------------------------------------------------------------------------------------------------------------

  bool matchRule(const std::string &rule)
  {
    if (runState != RunStateMatching) // Sanity check - should never happen at this point.
      return false;

    if (isTokenEndAfterCaret())
    {
      collectFromRule(rule);
      return false;
    }

    walkStack.push_back(rule);

    // The first alternative that matches wins.
    MySQLRuleAlternatives alts = rulesHolder.rules[rule];
    bool canSeek = false;

    size_t highestTokenIndex = 0;

    RunState resultState = runState;
    for (size_t i = 0; i < alts.size(); ++i)
    {
      // First run a predicate check if this alt can be considered at all.
      MySQLGrammarSequence alt = alts[i];
      if ((alt.minVersion > serverVersion) || (serverVersion > alt.maxVersion))
        continue;

      if ((alt.activeSqlModes > -1) && (alt.activeSqlModes & sqlMode) != alt.activeSqlModes)
        continue;

      if ((alt.inactiveSqlModes > -1) && (alt.inactiveSqlModes & sqlMode) != 0)
        continue;

      // When attempting to match one alt out of a list pick the one with the longest match.
      // Reset the run state each time to have the base matching done first (in case a previous alt did collect).
      size_t marker = scanner->position();
      runState = RunStateMatching;
      if (matchAlternative(alt) || runState != RunStateMatching)
      {
        canSeek = true;
        if (scanner->position() > highestTokenIndex)
        {
          highestTokenIndex = scanner->position();
          resultState = runState;
        }
      }

      scanner->seek(marker);
    }

    if (canSeek)
      scanner->seek(highestTokenIndex); // Move to the end of the longest match.

    runState = resultState;

    walkStack.pop_back();
    return canSeek && resultState == RunStateMatching;
  }

  //------------------------------------------------------------------------------------------------

};

//--------------------------------------------------------------------------------------------------

void initializeMySQLCodeCompletion(const std::string &grammarPath)
{
  rulesHolder.parseFile(grammarPath);
}

//--------------------------------------------------------------------------------------------------

enum ObjectFlags {
  // For 3 part identifiers.
  ShowSchemas = 1 << 0,
  ShowTables = 1 << 1,
  ShowColumns = 1 << 2,

  // For 2 part identifiers.
  ShowFirst = 1 << 3,
  ShowSecond = 1 << 4,
};

/**
* Determines the qualifier used for a qualified identifier with up to 2 parts (id or id.id).
* Returns the found qualifier (if any) and a flag indicating what should be shown.
*/
ObjectFlags determineQualifier(std::shared_ptr<MySQLScanner> scanner, std::string &qualfier)
{
  ObjectFlags result = ObjectFlags(ShowFirst | ShowSecond);

  // If we are currently in whitespaces between an identifier and a dot (e.g.: "id   |  .") jump
  // forward to the dot. We can ignore the whitespaces entirely in this case.
  // No need to do an extra check if we are really in an identifier. We wouldn't be here if that
  // were not the case.
  if (scanner->token_channel() != 0)
    scanner->next(true);
  uint32_t token_type = scanner->token_type();
  if (token_type != ANTLR3_TOKEN_EOF && token_type != DOT_SYMBOL && !scanner->is_identifier())
    return result;

  switch (token_type)
  {
  case DOT_SYMBOL:
  {
    // Being on a dot is the same as being in the qualifier (as the dot is only one char).
    scanner->previous(true);
    if (scanner->is_identifier())
      qualfier = scanner->token_text();
    return result;
  }
  case ANTLR3_TOKEN_EOF:
  {
    if (scanner->look_around(-1, true) == DOT_SYMBOL)
    {
      scanner->previous(true);
      scanner->previous(true);
      if (scanner->is_identifier())
        qualfier = scanner->token_text();
      return ShowSecond;
    }
    return result;
  }

  default:
  {
    // First look left as this is more reliable (we need valid syntax until the caret to arrive here).
    std::string id = scanner->token_text();
    if (scanner->look_around(-1, true) == DOT_SYMBOL)
    {
      // We are in the second part of a fully qualified identifier.
      scanner->previous(true);
      if (scanner->isIdentifier(scanner->look_around(-1, true)))
      {
        scanner->previous(true);
        qualfier = scanner->token_text();
        return ShowSecond;
      }
    }
    else
    {
      // No qualifier or we are in the qualifier. Look right.
      if (scanner->look_around(2, true) == DOT_SYMBOL)
      {
        qualfier = id;
        return ShowFirst;
      }
    }
  }
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
* Enhanced variant of the previous function that determines schema and table qualifiers for
* column references (and table_wild in multi table delete, for that matter).
* Returns a set of flags that indicate what to show for that identifier, as well as schema and table
* if given.
* Because id.id can be schema.table or table.column we need a second schema to indicate the second variant.
*/
ObjectFlags determine_schema_table_qualifier(std::shared_ptr<MySQLScanner> scanner, std::string &schema,
  std::string &table, std::string &column_schema)
{
  ObjectFlags result = ObjectFlags(ShowSchemas | ShowTables | ShowColumns);

  if (scanner->token_channel() != 0)
    scanner->next(true);
  uint32_t token_type = scanner->token_type();
  if (token_type != ANTLR3_TOKEN_EOF && token_type != DOT_SYMBOL && !scanner->is_identifier())
    return result;

  // Mark the position and go as far left as possible.
  size_t position = scanner->position();
  if (position > 0)
  {
    // Move to the previous dot symbol if there's one.
    if ((scanner->is_identifier() || token_type == ANTLR3_TOKEN_EOF)
      && scanner->look_around(-1, true) == DOT_SYMBOL)
      scanner->previous(true);
    if (scanner->token_type() == DOT_SYMBOL && scanner->isIdentifier(scanner->look_around(-1, true)))
    {
      scanner->previous(true);

      // And once more.
      if (scanner->look_around(-1, true) == DOT_SYMBOL)
      {
        scanner->previous(true);
        if (scanner->isIdentifier(scanner->look_around(-1, true)))
          scanner->previous(true);
      }
    }
  }

  // The scanner is on the leading identifier or dot (if there's no leading id).
  schema = "";
  column_schema = "";
  table = "";
  if (scanner->is_identifier() && scanner->look_around(1, true) == DOT_SYMBOL)
  {
    schema = scanner->token_text(); // schema.table
    table = schema;                 // table.column
    scanner->next(true);
  }

  if (scanner->token_type() == DOT_SYMBOL)
  {
    size_t dot1_position = scanner->position();
    size_t dot2_position = 0;

    scanner->next(true);

    if (scanner->is_identifier())
    {
      std::string text = scanner->token_text();
      scanner->next(true);

      if (scanner->token_type() == DOT_SYMBOL)
      {
        dot2_position = scanner->position();
        schema = table;
        column_schema = table;
        table = text;
      }
    }

    // Now the flags.
    if (dot2_position == 0)
    {
      if (position <= dot1_position)
        result = ObjectFlags(ShowSchemas | ShowTables);
      else
        result = (table.empty()) ? ShowTables : ObjectFlags(ShowTables | ShowColumns);
    }
    else
    {
      if (position <= dot1_position)
        result = ShowSchemas;
      else
        if (position < dot2_position)
          result = ShowTables;
        else
          result = ShowColumns;
    }
  }

  return result;
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

void insertSchemas(MySQLObjectNamesCache *cache, CompletionSet &set, const std::string &typedPart)
{
  std::vector<std::string> schemas = cache->getMatchingSchemaNames(typedPart);
  for (auto &schema : schemas)
    set.insert({AC_SCHEMA_IMAGE, schema});
}

//--------------------------------------------------------------------------------------------------

void insertTables(MySQLObjectNamesCache *cache, CompletionSet &set, const std::string &schema,
  const std::string &typedPart)
{
  std::vector<std::string> tables = cache->getMatchingTableNames(schema, typedPart);
  for (auto &table : tables)
    set.insert({AC_TABLE_IMAGE, table});
}

//--------------------------------------------------------------------------------------------------

void insertViews(MySQLObjectNamesCache *cache, CompletionSet &set, const std::string &schema,
  const std::string &typedPart)
{
  std::vector<std::string> views = cache->getMatchingViewNames(schema, typedPart);
  for (auto &view : views)
    set.insert({AC_TABLE_IMAGE, view});
}

//--------------------------------------------------------------------------------------------------

std::vector<std::pair<int, std::string>> getCodeCompletionList(size_t caretLine, size_t caretOffset,
  const std::string &writtenPart, const std::string &defaultSchema, bool uppercaseKeywords,
  std::shared_ptr<MySQLScanner> scanner, const std::string &functionNames, MySQLObjectNamesCache *cache)
{
  logDebug("Invoking code completion\n");

  AutoCompletionContext context;

  context.caretLine = ++caretLine; // ANTLR parser is one-based.
  context.caretOffset = caretOffset; // A byte offset (text is always utf-8 encoded).

  // The scanner already contains the statement to examine. Hence no need to deal with that here.
  // But we need the letters the user typed until the current caret position.
  context.serverVersion = scanner->get_server_version();
  context.typedPart = writtenPart;

  // Remove escape characters from the typed part so we have the pure text.
  context.typedPart.erase(std::remove(context.typedPart.begin(), context.typedPart.end(), '\\'),
    context.typedPart.end());

  // A set for each object type. This will sort the groups alphabetically and avoids duplicates,
  // but allows to add them as groups to the final list.
  CompletionSet schemaEntries;
  CompletionSet tableEntries;
  CompletionSet columnEntries;
  CompletionSet viewEntries;
  CompletionSet functionEntries;
  CompletionSet udfEntries;
  CompletionSet runtimeFunctionEntries;
  CompletionSet procedureEntries;
  CompletionSet triggerEntries;
  CompletionSet engineEntries;
  CompletionSet logfileGroupEntries;
  CompletionSet tablespaceEntries;
  CompletionSet systemVarEntries;
  CompletionSet keywordEntries;

  // To be done yet.
  CompletionSet userVarEntries;
  CompletionSet collationEntries;
  CompletionSet charsetEntries;
  CompletionSet userEntries;
  CompletionSet indexEntries;
  CompletionSet eventEntries;
  CompletionSet pluginEntries;
  CompletionSet fkEntries;

  std::vector<std::pair<int, std::string>> result;

  context.collectCandiates(scanner);

  for (auto &candidate : context.completionCandidates)
  {
    scanner->reset();
    scanner->seek(context.caretLine, context.caretOffset);

    // There can be more than a single token in a candidate (e.g. for GROUP BY).
    // But if there are more than one we always have a keyword list.
    std::vector<std::string> entries = base::split(candidate, " ");
    if (entries.size() > 1 || base::hasSuffix(candidate, "_SYMBOL"))
    {
      std::string entry;
      for (auto j = entries.begin(); j != entries.end(); ++j)
      {
        if (base::hasSuffix(*j, "_SYMBOL"))
        {
          // A single keyword or something in a keyword sequence. Convert the entry to a readable form.
          std::string token = j->substr(0, j->size() - 7);
          if (token == "OPEN_PAR")
          {
            // Part of a runtime function call or special constructs like PROCEDURE ANALYSE ().
            // Make the entry a function call in the list if there's only one keyword involved.
            // Otherwise ignore the parentheses.
            if (entries.size() < 3)
            {
              entry = base::tolower(entry);
              entry += "()";

              // Parentheses are always at the end.
              runtimeFunctionEntries.insert({AC_FUNCTION_IMAGE, entry});
              entry = "";
              break;
            }
          }
          else
          {
            if (!entry.empty())
              entry += " ";

            if (uppercaseKeywords)
              entry += token;
            else
              entry += base::tolower(token);
          }
        }
        else if (base::hasSuffix(*j, "_OPERATOR"))
        {
          // Something that we accept in a keyword sequence, not standalone (very small set).
          if (*j == "EQUAL_OPERATOR")
            entry += " =";
        }
      }
      if (!entry.empty())
        keywordEntries.insert({AC_KEYWORD_IMAGE, entry});
    }
    else if (rulesHolder.specialRules.find(candidate) != rulesHolder.specialRules.end())
    {
      // Any of the special rules.
      if (cache != NULL)
      {
        if (candidate == "udf_call")
        {
          logDebug3("Adding runtime function names to code completion list\n");

          std::vector<std::string> functions = base::split_by_set(functionNames, " \t\n");
          for (auto &function : functions)
            runtimeFunctionEntries.insert({AC_FUNCTION_IMAGE, function + "()"});
        }
        else if (candidate == "engine_ref")
        {
          logDebug3("Adding engine names\n");

          std::vector<std::string> engines = cache->getMatchingEngines(context.typedPart);
          for (auto &engine : engines)
            engineEntries.insert({AC_ENGINE_IMAGE, engine});
        }
        else if (candidate == "schema_ref")
        {
          logDebug3("Adding schema names from cache\n");
          insertSchemas(cache, schemaEntries, context.typedPart);
        }
        else if (candidate == "procedure_ref")
        {
          logDebug3("Adding procedure names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determineQualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(cache, schemaEntries, context.typedPart);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = defaultSchema;

            std::vector<std::string> procedures = cache->getMatchingProcedureNames(qualifier, context.typedPart);
            for (auto &procedure : procedures)
              procedureEntries.insert({AC_ROUTINE_IMAGE, procedure});
          }
        }
        else if (candidate == "function_ref" || candidate == "stored_function_call")
        {
          logDebug3("Adding function names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determineQualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(cache, schemaEntries, context.typedPart);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = defaultSchema;

            std::vector<std::string> functions = cache->getMatchingFunctionNames(qualifier, context.typedPart);
            for (auto &function : functions)
              functionEntries.insert({AC_ROUTINE_IMAGE, function});
          }
        }
        else if (candidate == "table_ref_with_wildcard")
        {
          // A special form of table references (id.id.*) used only in multi-table delete.
          // Handling is similar as for column references (just that we have table/view objects instead of column refs).
          logDebug3("Adding table + view names from cache\n");

          std::string schema, table, column_schema;
          ObjectFlags flags = determine_schema_table_qualifier(scanner, schema, table, column_schema);
          if ((flags & ShowSchemas) != 0)
            insertSchemas(cache, schemaEntries, context.typedPart);

          if (schema.empty())
            schema = defaultSchema;
          if ((flags & ShowTables) != 0)
          {
            insertTables(cache, tableEntries, schema, context.typedPart);
            insertViews(cache, viewEntries, schema, context.typedPart);
          }
        }
        else if (candidate == "table_ref" || candidate == "filter_table_ref" || candidate == "table_ref_no_db")
        {
          logDebug3("Adding table + view names from cache\n");

          // Tables refs also allow view refs.
          std::string qualifier;
          ObjectFlags flags = determineQualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(cache, schemaEntries, context.typedPart);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = defaultSchema;

            insertTables(cache, tableEntries, qualifier, context.typedPart);
            insertViews(cache, viewEntries, qualifier, context.typedPart);
          }
        }
        else if (candidate == "column_ref" || candidate == "column_ref_with_wildcard" || candidate == "table_wild")
        {
          logDebug3("Adding column names from cache\n");

          std::string schema, table, column_schema;
          ObjectFlags flags = determine_schema_table_qualifier(scanner, schema, table, column_schema);
          if ((flags & ShowSchemas) != 0)
            insertSchemas(cache, schemaEntries, context.typedPart);

          if (schema.empty())
            schema = defaultSchema;
          if ((flags & ShowTables) != 0)
            insertTables(cache, tableEntries, schema, context.typedPart);

          if ((flags & ShowColumns) != 0)
          {
            if (column_schema.empty())
              column_schema = defaultSchema;
            std::vector<std::string> columns = cache->getMatchingColumnNames(column_schema, table, context.typedPart);
            for (auto &column : columns)
              columnEntries.insert({AC_COLUMN_IMAGE, column});
          }
        }
        else if (candidate == "trigger_ref")
        {
          // Trigger references only consist of a table name and the trigger name.
          // However we have to make sure to show only triggers from the current schema.
          logDebug3("Adding trigger names from cache\n");

          std::string qualifier;
          ObjectFlags flags = determineQualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertTables(cache, schemaEntries, defaultSchema, context.typedPart);

          if ((flags & ShowSecond) != 0)
          {
            std::vector<std::string> triggers = cache->getMatchingTriggerNames(defaultSchema, qualifier, context.typedPart);
            for (auto &trigger : triggers)
              triggerEntries.insert({AC_TRIGGER_IMAGE, trigger});
          }
        }
        else if (candidate == "view_ref")
        {
          logDebug3("Adding view names from cache\n");

          // View refs only (no table references), e.g. like in DROP VIEW ...
          std::string qualifier;
          ObjectFlags flags = determineQualifier(scanner, qualifier);

          if ((flags & ShowFirst) != 0)
            insertSchemas(cache, schemaEntries, context.typedPart);

          if ((flags & ShowSecond) != 0)
          {
            if (qualifier.empty())
              qualifier = defaultSchema;

            insertViews(cache, viewEntries, qualifier, context.typedPart);
          }
        }
        else if (candidate == "logfileGroup_ref")
        {
          logDebug3("Adding logfile group names from cache\n");

          std::vector<std::string> logfileGroups = cache->getMatchingLogfileGroups(context.typedPart);
          for (auto &logfileGroup : logfileGroups)
            logfileGroupEntries.insert({AC_LOGFILE_GROUP_IMAGE, logfileGroup});
        }
        else if (candidate == "tablespace_ref")
        {
          logDebug3("Adding tablespace names from cache\n");

          std::vector<std::string> tablespaces = cache->getMatchingTablespaces(context.typedPart);
          for (auto &tablespace : tablespaces)
            tablespaceEntries.insert({AC_TABLESPACE_IMAGE, tablespace});
        }
        else if (candidate == "userVariable")
        {
          logDebug3("Adding user variables\n");
          userVarEntries.insert({AC_USER_VAR_IMAGE, "<user variable>"});
        }
        else if (candidate == "systemVariable")
        {
          logDebug3("Adding system variables\n");

          std::vector<std::string> variables = cache->getMatchingVariables(context.typedPart);
          for (auto &variable : variables)
            systemVarEntries.insert({AC_SYSTEM_VAR_IMAGE, variable});
        }
      }
    }
    else
    {
      // Simply take over anything else. There should never been anything but keywords and special rules.
      // By adding the raw token/rule entry we can better find the bug, which must be the cause
      // for this addition.
      keywordEntries.insert({0, candidate});
    }
  }

  std::copy(keywordEntries.begin(), keywordEntries.end(), std::back_inserter(result));
  std::copy(schemaEntries.begin(), schemaEntries.end(), std::back_inserter(result));
  std::copy(tableEntries.begin(), tableEntries.end(), std::back_inserter(result));
  std::copy(viewEntries.begin(), viewEntries.end(), std::back_inserter(result));
  std::copy(functionEntries.begin(), functionEntries.end(), std::back_inserter(result));
  std::copy(procedureEntries.begin(), procedureEntries.end(), std::back_inserter(result));
  std::copy(triggerEntries.begin(), triggerEntries.end(), std::back_inserter(result));
  std::copy(columnEntries.begin(), columnEntries.end(), std::back_inserter(result));
  std::copy(indexEntries.begin(), indexEntries.end(), std::back_inserter(result));
  std::copy(eventEntries.begin(), eventEntries.end(), std::back_inserter(result));
  std::copy(userEntries.begin(), userEntries.end(), std::back_inserter(result));
  std::copy(engineEntries.begin(), engineEntries.end(), std::back_inserter(result));
  std::copy(pluginEntries.begin(), pluginEntries.end(), std::back_inserter(result));
  std::copy(logfileGroupEntries.begin(), logfileGroupEntries.end(), std::back_inserter(result));
  std::copy(tablespaceEntries.begin(), tablespaceEntries.end(), std::back_inserter(result));
  std::copy(charsetEntries.begin(), charsetEntries.end(), std::back_inserter(result));
  std::copy(collationEntries.begin(), collationEntries.end(), std::back_inserter(result));
  std::copy(userVarEntries.begin(), userVarEntries.end(), std::back_inserter(result));
  std::copy(runtimeFunctionEntries.begin(), runtimeFunctionEntries.end(), std::back_inserter(result));
  std::copy(systemVarEntries.begin(), systemVarEntries.end(), std::back_inserter(result));

  return result;
}

//--------------------------------------------------------------------------------------------------
