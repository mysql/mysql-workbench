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

#include <sstream>
#include <algorithm>
#include <string>
#include <stack>
#include <set>

#include <antlr3.h>
#include <glib.h>

#include "MySQLLexer.h"  // The generated lexer.
#include "MySQLParser.h" // The generated parser.

#include "base/log.h"
#include "base/string_utilities.h"

#include "mysql-parser.h"
#include "mysql-scanner.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

//--------------------------------------------------------------------------------------------------

std::string get_token_name(pANTLR3_UINT8 *tokenNames, ANTLR3_UINT32 token)
{
  // Transform a selection of tokens to nice strings. All others just take the token name.
  switch (token)
  {
  case ANTLR3_TOKEN_EOF:
    return "end of statement";
  case 1:
  case 2:
  case 3:
  case 4:
    return "<invalid token>";
  case OPEN_PAR_SYMBOL:
    return "opening parenthesis";
  case CLOSE_PAR_SYMBOL:
    return "closing parenthesis";
  case OPEN_CURLY_SYMBOL:
    return "opening curly brace";
  case CLOSE_CURLY_SYMBOL:
    return "closing curly brace";
  case NULL2_SYMBOL:
    return "null escape sequence";
  case PARAM_MARKER:
    return "parameter placeholder";

  default:
    std::string result = base::tolower((char *)tokenNames[token]);
    std::string::size_type position = result.find("_symbol");
    if (position != std::string::npos)
      result = result.substr(0, position);

    base::replace(result, "_", " ");
    return result;
  }
}

//--------------------------------------------------------------------------------------------------

bool handle_lexer_error(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_EXCEPTION exception,
  ANTLR3_MARKER &start, ANTLR3_MARKER &length, std::string &message)
{
  pANTLR3_LEXER lexer = (pANTLR3_LEXER)(recognizer->super);
  start = recognizer->state->tokenStartCharIndex;

  // For debugging use the native error message that contains the grammar line.
  // std::string native = (pANTLR3_UINT8)(exception->message);
  length = ANTLR3_UINT32_CAST(((pANTLR3_UINT8)(lexer->input->data) + (lexer->input->size(lexer->input))) - (pANTLR3_UINT8)(exception->index));

  if (length <= 0)
  {
    message = "unexpected end of input (unfinished string or quoted identifier)";
    length = ANTLR3_UINT32_CAST(((pANTLR3_UINT8)(lexer->input->data) + (lexer->input->size(lexer->input))) - (pANTLR3_UINT8)(lexer->rec->state->tokenStartCharIndex));
  }
  else
  {
    switch (exception->type)
    {
    case ANTLR3_RECOGNITION_EXCEPTION:
      // Invalid character. Can currently never appear because any input from the Unicode BMP
      // is acceptable input for our parser. We cannot parse input beyond the BMP.
      message += "'";
      if (isprint(exception->c))
        message += (char)exception->c;
      else
        message += (ANTLR3_UINT8)(exception->c);

      message += "' is not valid input";
      break;
    }
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

bool handle_parser_error(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_EXCEPTION exception,
  pANTLR3_UINT8 *tokenNames, ANTLR3_MARKER &start, ANTLR3_MARKER &length, std::string &message)
{
  std::ostringstream error;

  pANTLR3_PARSER parser = (pANTLR3_PARSER) (recognizer->super);
  pANTLR3_COMMON_TOKEN error_token = (pANTLR3_COMMON_TOKEN)(exception->token);

  std::string token_text = (char *)error_token->getText(error_token)->chars;
  if (token_text[0] != '"' && token_text[0] != '\'' && token_text[0] != '`')
    token_text = "'" + token_text + "'";

  std::string token_name;

  bool eoi = error_token->type == ANTLR3_TOKEN_EOF;
  if (eoi)
  {
    // We are at the end of the input. Seek back one token to have a meaningful error indicator.
    // If we cannot get a previous token then issue a generic eoi error.
    pANTLR3_COMMON_TOKEN previous_token = parser->tstream->_LT(parser->tstream, -1);
    if (previous_token != NULL)
      error_token = previous_token;
  }
  else
    token_name = get_token_name(tokenNames, error_token->type);

  start = error_token->start;
  switch (exception->type)
  {
  case ANTLR3_RECOGNITION_EXCEPTION:
    // Unpredicted input.
    error << token_text << " (" << token_name << ") is not valid input at this position";
    break;

  case ANTLR3_MISMATCHED_TOKEN_EXCEPTION:
    // We were expecting to see one thing and got another. This is the
    // most common error if we could not detect a missing or unwanted token.
    if (exception->expecting == ANTLR3_TOKEN_EOF)
      error << "expected end of statement but found " << token_text << " (" << token_name << ")";
    else
      error << "expected '" << tokenNames[exception->expecting] << "' but found " << token_text << " (" << token_name << ")";
    break;

  case ANTLR3_NO_VIABLE_ALT_EXCEPTION:
    // No alternative to choose from here.
    if (eoi)
      error << "unexpected end of input";
    else
      error << "unexpected " << token_text << " (" << token_name << ")";

    break;

  case ANTLR3_MISMATCHED_SET_EXCEPTION:
    {
      // One out of a set of tokens was expected but hasn't been found.
      pANTLR3_BITSET errBits = antlr3BitsetLoad(exception->expectingSet);
      ANTLR3_UINT32 numbits = errBits->numBits(errBits);
      ANTLR3_UINT32 size = errBits->size(errBits);

      if (size == 0)
      {
        // No information about expected tokens available.
        error << "unexpected " << token_text << " (" << token_name << ")";
      }
      else
      {
        // I'd expect only a few set members here, but this case is hard to test. So
        // just to be sure not to show a huge list of expected tokens we limit the number here.
        // TODO: find a query that triggers this error branch.
        error << "wrong input, expected one of: ";
        for (ANTLR3_UINT32 bit = 1; bit < numbits && bit <= 20 && bit < size; ++bit)
        {
          if  (errBits->isMember(errBits, bit))
            error << (bit > 1 ? ", " : "") << get_token_name(tokenNames, bit);
        }
      }
    }
    break;

  case ANTLR3_EARLY_EXIT_EXCEPTION:
    // We entered a loop requiring a number of token sequences but found a token that ended that
    // sequence earlier than we should have done.
    // Unfortunately, there's no expecting set for this exception which would have made the message
    // very useful.
    error << "missing sub clause or other elements before " << token_text << " (" << token_name << ")";
    break;

  case ANTLR3_FAILED_PREDICATE_EXCEPTION:
    // Appears when a gated semantic predicate is used in the grammar, but not for predicting an alternative,
    // e.g. ... some_rule {condition}? => some_rule.
    // If the condition does not match we get a failed predicate error. So it's more like a grammar error
    // unless this is by intention (which is never the case in the MySQL grammar where predicates are only used
    // to guide the parser).
    error << "failed predicate";
    break;

  case ANTLR3_MISMATCHED_TREE_NODE_EXCEPTION:
    // This is very likely a tree parser error and hence not relevant here (no info in ANTLR docs).
    error << "unexpected parser error type (" << exception->type << "), please file a bug report!";
    break;

  case ANTLR3_REWRITE_EARLY_EXCEPTION:
    // ANTLR docs say: No elements within a (...)+ in a rewrite rule
    // so this seems to be an error only raised if there was a grammar bug -> internal error.
    error << "internal parser error type (" << exception->type << "), please file a bug report!";
    break;

  case ANTLR3_UNWANTED_TOKEN_EXCEPTION:
    // Indicates that the recognizer was fed a token which seems to be spurious input. We can detect
    // this when the token that follows this unwanted token would normally be part of the
    // syntactically correct stream.
    if	(exception->expecting == ANTLR3_TOKEN_EOF)
      error << "extraneous input found - expected end of input";
    else
      error << "extraneous input found - expected '" << get_token_name(tokenNames, exception->expecting) << "'";
    break;

  case ANTLR3_MISSING_TOKEN_EXCEPTION:
    {
      // Indicates that the recognizer detected that the token we just
      // hit would be valid syntactically if preceded by a particular
      // token. Perhaps a missing ';' at line end or a missing ',' in an
      // expression list, and such like.
      if (tokenNames == NULL)
        error << "missing token " << exception->expecting; // Will very likely never occur.
      else
      {
        if (exception->expecting == ANTLR3_TOKEN_EOF)
          // Will probably not occur since ANTLR3_UNWANTED_TOKEN_EXCEPTION will kick in instead.
          error << "expected end of input";
        else
          error << "missing '" << get_token_name(tokenNames, exception->expecting) << "'";
      }

      // The error token for a missing token does not contain much information to display.
      // If we reach this error case then the token after the missing token has been consumed already
      // (which is how the parser found out about the missing one), so by going back to that token
      // we can get good start and length information (showing so the error at this following token instead).
      error_token = parser->tstream->_LT(parser->tstream, -1);
      if (error_token == NULL)
        return false;

      start = error_token->start;
      break;
    }

  default:
    error << "unexpected parser error type (" << exception->type << "), please file a bug report!";
    break;

  }

  if (length == 0)
  {
    if (error_token != NULL)
      length = (int)error_token->stop - (int)error_token->start + 1;
    else
      length = 1;
  }

  message = error.str();
  return true;
}

//-------------------------------------------------------------------------------------------------- 

extern "C" {

  /**
   * Error report function which is set in the parser (see MySQL.g where this is done).
   */
  void on_parse_error(pANTLR3_BASE_RECOGNIZER recognizer, pANTLR3_UINT8 *tokenNames)
  {
    pANTLR3_EXCEPTION exception = recognizer->state->exception;

    // Only the take the current exception into account. There's a linked list of all exceptions we could walk
    // but that only contains what we've seen anyway.
    if (exception != NULL)
    {
      // Token position and length for error marker.
      ANTLR3_MARKER length = 0;
      ANTLR3_MARKER start = 0;
      std::string message;
      switch (recognizer->type)
      {
      case ANTLR3_TYPE_LEXER:
        if (!handle_lexer_error(recognizer, exception, start, length, message))
          return;
        break;

      case ANTLR3_TYPE_PARSER:
        if (!handle_parser_error(recognizer, exception, tokenNames, start, length, message))
          return;
        break;
      }

      pANTLR3_COMMON_TOKEN error_token = (pANTLR3_COMMON_TOKEN)(exception->token);
      MySQLRecognitionBase *our_recognizer = (MySQLRecognitionBase*)((RecognitionContext*)recognizer->state->userp)->payload;
      our_recognizer->add_error("Syntax error: " + message,
        (error_token == NULL) ? 0 : error_token->type, start,
        exception->line, exception->charPositionInLine, length);
    }
  }

} // extern "C"

//----------------- MySQLTreeWalker ----------------------------------------------------------------

struct compare_token_index
{
  inline bool operator() (const pANTLR3_BASE_TREE left, const pANTLR3_BASE_TREE right)
  {
    pANTLR3_COMMON_TOKEN t1 = left->getToken(left);
    pANTLR3_COMMON_TOKEN t2 = right->getToken(right);
    return t1->index < t2->index;
  }
};

MySQLRecognizerTreeWalker::MySQLRecognizerTreeWalker(MySQLRecognizer *recognizer, pANTLR3_BASE_TREE tree)
{
  _recognizer = recognizer;
  _tree = tree;
  if (token_type() == 0) // If there's a null root node skip over that.
    next();

  _origin = _tree;

  // Fill the list of tokens for quick lookup by type or position in the correct order.
  pANTLR3_BASE_TREE run = _tree;
  while (run != NULL)
  {
    // Add only entries that carry useful information for position search.
    pANTLR3_COMMON_TOKEN token = run->getToken(run);
    if (token != NULL && token->lineStart != NULL) // Virtual tokens have no line information.
      _token_list.push_back(run);
    run = get_next(run, true);
  }

  // Sort token list by token index, which puts them in appearance order.
  if (_token_list.size() > 1)
    std::sort(_token_list.begin(), _token_list.end(), compare_token_index());
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the next node after the given one without changing any internal state or NULL if there's
 * no next node. The recurse flag determines if we can change tree levels or stay in the one we are in
 * currently.
 */
pANTLR3_BASE_TREE MySQLRecognizerTreeWalker::get_next(pANTLR3_BASE_TREE node, bool recurse)
{
  if (recurse)
  {
    // If there are child take the first one.
    if (node->getChildCount(node) > 0)
      return (pANTLR3_BASE_TREE)_tree->getChild(node, 0);
  }

  // No child nodes (or no recursion), so see if there is another sibling of this node or one of its parents.
  while (true)
  {
    pANTLR3_BASE_TREE parent = node->getParent(node);
    if (parent == NULL)
      return NULL;

    int index = parent->getChildIndex(node) + 1;
    if ((int)parent->getChildCount(parent) > index)
      return (pANTLR3_BASE_TREE)parent->getChild(parent, index);

    if (!recurse)
      return NULL;

    // No sibling either - go up one level and try the next sibling of the parent.
    node = parent;
  }
  return NULL;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the previous node before the given one without changing any internal state or NULL if there's
 * no previous node. The meaning of the recurse flag is the same as for get_next().
 */
pANTLR3_BASE_TREE MySQLRecognizerTreeWalker::get_previous(pANTLR3_BASE_TREE node, bool recurse)
{
  pANTLR3_BASE_TREE parent = _tree->getParent(_tree);
  if (parent == NULL)
    return NULL;

  int index = parent->getChildIndex(_tree) - 1;
  if (index < 0)
  {
    if (!recurse)
      return NULL;
    return parent;
  }

  pANTLR3_BASE_TREE last_node = (pANTLR3_BASE_TREE)parent->getChild(parent, index);
  if (recurse)
  {
    while (last_node->getChildCount(last_node) > 0)
    {
      // Walk down the entire tree hierarchy to the last sub node of the previous sibling.
      index = last_node->getChildCount(last_node) - 1;
      last_node = (pANTLR3_BASE_TREE)last_node->getChild(last_node, index);
    }
  }

  return last_node;
}

//--------------------------------------------------------------------------------------------------

pANTLR3_BASE_TREE MySQLRecognizerTreeWalker::get_previous_by_index(pANTLR3_BASE_TREE node)
{
  if (node == NULL)
    return NULL;

  std::vector<pANTLR3_BASE_TREE>::const_iterator iterator =
   lower_bound(_token_list.begin(), _token_list.end(), node, compare_token_index());
  if (iterator == _token_list.end())
    return NULL;

  if (iterator == _token_list.begin())
    return NULL;

  return *(--iterator);
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognizerTreeWalker::print_token(pANTLR3_BASE_TREE tree)
{
  pANTLR3_STRING token_text = tree->getText(tree);
  printf("Token: %s\n", (token_text == NULL) ? "nil" : (char*)token_text->chars);
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next token. If this is a subtree the next token is the first child node,
 * otherwise the next sibling node is used. If there is no sibling node then the next sibling of
 * the parent is used, if there's one, and so on.
 *
 * @param count Number of steps. Default is 1.
 * @return True if there was count next nodes, false otherwise. If false then no state change is performed.
 */
bool MySQLRecognizerTreeWalker::next(size_t count)
{
  pANTLR3_BASE_TREE node = _tree;
  while (count > 0)
  {
    node = get_next(node, true);
    if (node == NULL)
      return false;

    --count;
  }

  _tree = node;
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next sibling token, ignoring any child nodes (if there are any).
 *
 * @return True if there was a next sibling node, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::next_sibling()
{
  pANTLR3_BASE_TREE node = get_next(_tree, false);
  if (node != NULL)
  {
    _tree = node;
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns to the previous token. If this is the first child in a sub tree then the parent node
 * is used, otherwise the previous sibling. If the previous sibling has children then the last child
 * of it is used instead. If this also has children the last grand child is used and so on.
 *
 * @return True if there was a previous node, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::previous()
{
  pANTLR3_BASE_TREE node = get_previous(_tree, true);
  if (node == NULL)
    return false;

  _tree = node;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns to the previous token in index order, which might be different to the tree order.
 * The index order is the order by which tokens appear in the text. Index ordering is not continuous
 * as some tokens are hidden.
 *
 * @return True if there is a previous node, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::previous_by_index()
{
  pANTLR3_BASE_TREE previous = get_previous_by_index(_tree);
  if (previous == NULL)
    return false;

  _tree = previous;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns to the previous sibling token, ignoring any child nodes of that sibling. If the current
 * node is the first child in a subtree then the function fails.
 *
 * @return True if there was a previous node, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::previous_sibling()
{
  pANTLR3_BASE_TREE node = get_previous(_tree, false);
  if (node == NULL)
    return false;

  _tree = node;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns to the parent token if the current one is a child.
 *
 * @return True if there was a parent node, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::up()
{
  pANTLR3_BASE_TREE parent = _tree->getParent(_tree);
  if (parent == NULL)
    return false;

  _tree = parent;

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the token that covers the given line and char offset. The line number is one-based
 * while the character offset is zero-based. This search only considers real tokens.
 *
 * Note: if the given position is between two tokens then the first one is used.
 *
 * @return True if such a node exists, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::advance_to_position(int line, int offset)
{
  if (_token_list.size() == 0)
    return false;

  size_t i = 0;
  for (; i < _token_list.size(); i++)
  {
    pANTLR3_BASE_TREE run = _token_list[i];
    ANTLR3_UINT32 token_line = run->getLine(run);
    if ((int)token_line >= line)
    {
      int token_offset = run->getCharPositionInLine(run);
      pANTLR3_COMMON_TOKEN token = run->getToken(run);
      int token_length = (int)(token->stop - token->start) + 1;
      if ((int)token_line == line && token_offset <= offset && offset < token_offset + token_length)
      {
        _tree = _token_list[i];
        break;
      }

      if ((int)token_line > line || (int)token_offset > offset)
      {
        // We reached a token after the current offset. Take the previous one as result then.
        if (i == 0)
          return false;

        _tree = _token_list[i - 1];
        break;
      }
    }
  }

  if (i == _token_list.size())
    _tree = _token_list[i - 1]; // Nothing found, take the last token instead.

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next token with the given type. The parameter type can be any token id listed in
 * MySQLParser.h (search for "Symbolic definitions"). It can be either a simple lexical token
 * like DIV_OPERATOR or SELECT_SYMBOL, a more complex lexical token like IDENTIFIER or a parser token like
 * GROUP_BY or MISSING_ID.
 *
 * @param type The token type to search.
 * @param recurse If false search only siblings, otherwise any node in any tree level.
 * @return True if such a node exists, false otherwise (no change performed then).
 */
bool MySQLRecognizerTreeWalker::advance_to_type(unsigned int type, bool recurse)
{
  pANTLR3_BASE_TREE run = _tree;
  while (true)
  {
    run = get_next(run, recurse);
    if (run == NULL)
      return false;

    if (run->getType(run) == type)
    {
      _tree = run;
      return true;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Steps back to the start of the current subquery (or the top level query if we are not in a subquery).
 * On exit the current walker position is on the first query token.
 */
void MySQLRecognizerTreeWalker::go_to_subquery_start()
{
  do
  {
    switch (token_type())
    {
      case ANALYZE_SYMBOL:
      case ALTER_SYMBOL:
      case BACKUP_SYMBOL:
      case BINLOG_SYMBOL:
      case CACHE_SYMBOL:
      case CALL_SYMBOL:
      case CHANGE_SYMBOL:
      case CHECK_SYMBOL:
      case CHECKSUM_SYMBOL:
      case COMMIT_SYMBOL:
      case CREATE_SYMBOL:
      case DEALLOCATE_SYMBOL:
      case DELETE_SYMBOL:
      case DESC_SYMBOL:
      case DESCRIBE_SYMBOL:
      case DO_SYMBOL:
      case DROP_SYMBOL:
      case EXECUTE_SYMBOL:
      case EXPLAIN_SYMBOL:
      case FLUSH_SYMBOL:
      case GRANT_SYMBOL:
      case HANDLER_SYMBOL:
      case HELP_SYMBOL:
      case INSERT_SYMBOL:
      case INSTALL_SYMBOL:
      case KILL_SYMBOL:
      case LOAD_SYMBOL:
      case LOCK_SYMBOL:
      case OPTIMIZE_SYMBOL:
      case PARTITION_SYMBOL:
      case PREPARE_SYMBOL:
      case PURGE_SYMBOL:
      case RELEASE_SYMBOL:
      case REMOVE_SYMBOL:
      case RENAME_SYMBOL:
      case REPAIR_SYMBOL:
      case REPLACE_SYMBOL:
      case RESET_SYMBOL:
      case RESTORE_SYMBOL:
      case REVOKE_SYMBOL:
      case ROLLBACK_SYMBOL:
      case SAVEPOINT_SYMBOL:
      case SELECT_SYMBOL:
      case SET_SYMBOL:
      case SHOW_SYMBOL:
      case START_SYMBOL:
      case STOP_SYMBOL:
      case TRUNCATE_SYMBOL:
      case UNINSTALL_SYMBOL:
      case UNLOCK_SYMBOL:
      case UPDATE_SYMBOL:
      case USE_SYMBOL:
      case XA_SYMBOL:
        return;

      default:
        if (!up())
        {
          // Advance to first child.
          next();
          return;
        }
        break;
    }
  } while (true);
}

//--------------------------------------------------------------------------------------------------

/**
 * Steps over a number of tokens and positions the walker at the first token after the last one.
 * The tokens must all be siblings of the current token and are traversed in exactly the given order
 * without intermediate tokens. The current token must be start_token.
 *
 * Note: the list must be terminated by ANTLR3_TOKEN_INVALID (or 0).
 *
 * @return True if all the given tokens were found and there is another sibling after the last token
 *         in the list, false otherwise. If the token sequence could not be found or there is no more
 *         token the internal state is undefined.
 */
bool MySQLRecognizerTreeWalker::skip_token_sequence(unsigned int start_token, ...)
{
  bool result = false;

  unsigned int token = start_token;
  va_list tokens;
  va_start(tokens, start_token);
  while (true)
  {
    if (token_type() != token)
      break;

    if (!next_sibling())
      break;

    token = va_arg(tokens, unsigned int);
    if (token == ANTLR3_TOKEN_INVALID)
    {
      result = true;
      break;
    }
  }
  va_end(tokens);

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advance to the nth next token if the current one is that given by @token.
 */
void MySQLRecognizerTreeWalker::skip_if(unsigned int token, size_t count)
{
  if (token_type() == token)
    next(count);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the type of the next sibling (if recursive is false) or the next token (including child
 * nodes) without changing the internal state.
 */
unsigned int MySQLRecognizerTreeWalker::look_ahead(bool recursive)
{
  pANTLR3_BASE_TREE next = get_next(_tree, recursive);
  if (next == NULL)
    return ANTLR3_TOKEN_INVALID;
  return next->getType(next);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the parent's token type if we are in a subtree.
 */
unsigned int MySQLRecognizerTreeWalker::parent_type()
{
  pANTLR3_BASE_TREE parent = _tree->getParent(_tree);
  if (parent == NULL)
    return ANTLR3_TOKEN_INVALID;

  return parent->getType(parent);
}

//--------------------------------------------------------------------------------------------------

/**
 * Look back in the stream (physical order) what was before the current token, without
 * modifying the current position.
 */
unsigned int MySQLRecognizerTreeWalker::previous_type()
{
  pANTLR3_BASE_TREE previous = get_previous_by_index(_tree);
  if (previous == NULL)
    return ANTLR3_TOKEN_INVALID;

  return previous->getType(previous);
}

//--------------------------------------------------------------------------------------------------

/**
 * Resets the walker to be at the original location.
 */
void MySQLRecognizerTreeWalker::reset()
{
  _tree = _origin;
  while (!_token_stack.empty())
    _token_stack.pop();
}

//--------------------------------------------------------------------------------------------------

/**
 * Store the current node on the stack, so we can easily come back when needed.
 */
void MySQLRecognizerTreeWalker::push()
{
  _token_stack.push(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns to the location at the top of the token stack (if any).
 */
bool MySQLRecognizerTreeWalker::pop()
{
  if (_token_stack.empty())
    return false;

  _tree = _token_stack.top();
  _token_stack.pop();
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the current top of stack entry without restoring the internal state.
 * Does nothing if the stack is empty.
 */
void MySQLRecognizerTreeWalker::remove_tos()
{
  if (!_token_stack.empty())
    _token_stack.pop();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is empty, false otherwise.
 */
bool MySQLRecognizerTreeWalker::is_nil()
{
  return _tree->isNilNode(_tree) == ANTLR3_TRUE;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is the root of a subtree (i.e. has child nodes).
 */
bool MySQLRecognizerTreeWalker::is_subtree()
{
  return _recognizer->is_subtree(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is an identifier.
 */
bool MySQLRecognizerTreeWalker::is_identifier()
{
  return _recognizer->is_identifier(_tree->getType(_tree));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is an identifier.
 */
bool MySQLRecognizerTreeWalker::is_keyword()
{
  return _recognizer->is_keyword(_tree->getType(_tree));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is a relation specifier (operator, math symbol etc).
 */
bool MySQLRecognizerTreeWalker::is_relation()
{
  return _recognizer->is_relation(_tree->getType(_tree));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token has no previous sibling.
 */
bool MySQLRecognizerTreeWalker::is_first_child()
{
  return _tree->getChildIndex(_tree) == 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is any of the number types we recognize
 * (i.e. integer, float, hex, bit).
 */
bool MySQLRecognizerTreeWalker::is_number()
{
  return _recognizer->is_number(_tree->getType(_tree));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is an operator/punctuation character.
 * No space is needed between them and the following token.
 */
bool MySQLRecognizerTreeWalker::is_operator()
{
  return _recognizer->is_operator(_tree->getType(_tree));
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the textual expression of the token. This will either return the actual text in the
 * parsed query (if it is a lexer symbol) or the textual expression of the constant name for abstract
 * tokens.
 */
std::string MySQLRecognizerTreeWalker::token_text()
{
  return _recognizer->token_text(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the type of the current token. Same as the type you can specify in advance_to().
 */
unsigned int MySQLRecognizerTreeWalker::token_type()
{
  return _tree->getType(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the (one-base) line number of the token.
 */
unsigned int MySQLRecognizerTreeWalker::token_line()
{
  return _tree->getLine(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the (zero-based) offset of the token on its line.
 */
unsigned int MySQLRecognizerTreeWalker::token_start()
{
  return _tree->getCharPositionInLine(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
* Returns the (zero-based) index of the current token within the input.
*/
ANTLR3_MARKER MySQLRecognizerTreeWalker::token_index()
{
  pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
  return token->index;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the offset of the token in its source string.
 */
size_t MySQLRecognizerTreeWalker::token_offset()
{
  pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
  return (size_t)(token->start - (ANTLR3_MARKER)_recognizer->text());
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the length of the token in bytes.
 */
int MySQLRecognizerTreeWalker::token_length()
{
  pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
  if (token == NULL)
    return 0;

  // Start and stop are actually pointers into the input stream.
  return (int) token->stop - (int) token->start + 1;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizerTreeWalker::text_for_tree()
{
  return _recognizer->text_for_tree(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the query type of the (sub)query the waker is in currently.
 */
MySQLQueryType MySQLRecognizerTreeWalker::get_current_query_type()
{
  // Walk up the parent chain until we find either a major keyword or the top of the tree.
  push();
  bool done = false;
  do
  {
    switch (token_type())
    {
      case ANALYZE_SYMBOL:
      case ALTER_SYMBOL:
      case BACKUP_SYMBOL:
        //case BEGIN_SYMBOL:
      case BINLOG_SYMBOL:
      case CACHE_SYMBOL:
      case CALL_SYMBOL:
      case CHANGE_SYMBOL:
      case CHECK_SYMBOL:
      case CHECKSUM_SYMBOL:
      case COMMIT_SYMBOL:
      case CREATE_SYMBOL:
      case DEALLOCATE_SYMBOL:
      case DELETE_SYMBOL:
      case DESC_SYMBOL:
      case DESCRIBE_SYMBOL:
      case DO_SYMBOL:
      case DROP_SYMBOL:
      case EXECUTE_SYMBOL:
      case EXPLAIN_SYMBOL:
      case FLUSH_SYMBOL:
      case GRANT_SYMBOL:
      case HANDLER_SYMBOL:
      case HELP_SYMBOL:
      case INSERT_SYMBOL:
      case INSTALL_SYMBOL:
      case KILL_SYMBOL:
      case LOAD_SYMBOL:
      case LOCK_SYMBOL:
      case OPTIMIZE_SYMBOL:
      case PARTITION_SYMBOL:
      case PREPARE_SYMBOL:
      case PURGE_SYMBOL:
      case RELEASE_SYMBOL:
      case REMOVE_SYMBOL:
      case RENAME_SYMBOL:
      case REPAIR_SYMBOL:
      case REPLACE_SYMBOL:
      case RESET_SYMBOL:
      case RESTORE_SYMBOL:
      case REVOKE_SYMBOL:
      case ROLLBACK_SYMBOL:
      case SAVEPOINT_SYMBOL:
      case SELECT_SYMBOL:
      case SET_SYMBOL:
      case SHOW_SYMBOL:
      case START_SYMBOL:
      case STOP_SYMBOL:
      case TRUNCATE_SYMBOL:
      case UNINSTALL_SYMBOL:
      case UNLOCK_SYMBOL:
      case UPDATE_SYMBOL:
      case USE_SYMBOL:
      case XA_SYMBOL:
        if (is_subtree())
          done = true;
        else
          if (!up())
            done = true;
        break;
      default:
        if (!up())
          done = true;
        break;
    }
  } while (!done);

  MySQLQueryType result = _recognizer->query_type(_tree);

  pop();

  return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Returns the query type of the top most query (just for convenience).
 */
MySQLQueryType MySQLRecognizerTreeWalker::get_main_query_type()
{
  return _recognizer->query_type();
}

//----------------- MySQLRecognizer ----------------------------------------------------------------

class MySQLRecognizer::Private
{
public:
  const char *_text;
  size_t _text_length;
  int _input_encoding;
  RecognitionContext _context;

  pANTLR3_INPUT_STREAM _input;
  pMySQLLexer _lexer;
  pANTLR3_COMMON_TOKEN_STREAM _tokens;
  pMySQLParser _parser;
  pANTLR3_BASE_TREE _ast;
};

MySQLRecognizer::MySQLRecognizer(long server_version, const std::string &sql_mode, const std::set<std::string> &charsets)
  : MySQLRecognitionBase(charsets)
{
  d = new Private();
  d->_context.version = server_version;
  d->_context.payload = this;
  set_sql_mode(sql_mode);

  d->_input = NULL;
  d->_lexer = NULL;
  d->_tokens = NULL;
  d->_parser = NULL;
}

//--------------------------------------------------------------------------------------------------

MySQLRecognizer::~MySQLRecognizer()
{
  if (d->_parser != NULL)
    d->_parser->free(d->_parser);
  if (d->_tokens != NULL)
    d->_tokens ->free(d->_tokens);
  if (d->_lexer != NULL)
    d->_lexer->free(d->_lexer);
  if (d->_input != NULL)
    d->_input->close(d->_input);

  delete d;
}

//--------------------------------------------------------------------------------------------------

/**
 * Starts parsing with new input but keeps everything else in place. This is expected to be more
 * efficient than creating a new parser over and over again for many statements (e.g. for error checking).
 * 
 * @param text The text to parse.
 * @param length The length of the text.
 * @param is_utf8 True if text is utf-8 encoded. If false we assume ASCII encoding.
 * @param parse_unit used to restrict parsing to a particular query type. 
 *                   Note: only a few types are supported, everything else is just parsed as a query.
 */
void MySQLRecognizer::parse(const char *text, size_t length, bool is_utf8, MySQLParseUnit parse_unit)
{
  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  // TODO: handle the (bad) case that the input encoding changes between parse runs.
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;

  d->_text = text;
  d->_text_length = length;

  // Logging adds significant time to parsing which especially shows with large scripts
  // (thousands of rather small queries to error check). And it adds not much benefit, so leave it off.
  //log_debug3("Start parsing\n");

  reset();

  if (d->_input == NULL)
  {
    // Input and depending structures are only created once. If there's no input stream yet we need the full setup.
    d->_input = antlr3StringStreamNew((pANTLR3_UINT8)d->_text, d->_input_encoding, (ANTLR3_UINT32)d->_text_length, (pANTLR3_UINT8)"");
    d->_input->setUcaseLA(d->_input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!
    d->_lexer = MySQLLexerNew(d->_input);
    d->_lexer->pLexer->rec->state->userp = &d->_context;

    d->_tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(d->_lexer));

    d->_parser = MySQLParserNew(d->_tokens);
    d->_parser->pParser->rec->state->userp = &d->_context;
  }
  else
  {
    d->_input->reuse(d->_input, (pANTLR3_UINT8)d->_text, (ANTLR3_UINT32)d->_text_length, (pANTLR3_UINT8)"");
    d->_tokens->reset(d->_tokens);
    d->_lexer->reset(d->_lexer);
    d->_parser->reset(d->_parser);

    // Manually free adaptor and vector pool members. The parser reset() misses them and we cannot
    // add this code to the parser (as it is generated). Without that these members grow endlessly.
    d->_parser->vectors->close(d->_parser->vectors);
    d->_parser->vectors = antlr3VectorFactoryNew(0);

    d->_parser->adaptor->free(d->_parser->adaptor);
    d->_parser->adaptor = ANTLR3_TREE_ADAPTORNew(d->_tokens->tstream->tokenSource->strFactory);
  }

  switch (parse_unit)
  {
  case PuCreateTrigger:
    d->_ast = d->_parser->create_trigger(d->_parser).tree;
    break;
  case PuCreateView:
    d->_ast = d->_parser->create_view(d->_parser).tree;
    break;
  case PuCreateRoutine:
    d->_ast = d->_parser->create_routine(d->_parser).tree;
    break;
  case PuCreateEvent:
    d->_ast = d->_parser->create_event(d->_parser).tree;
    break;
  case PuGrant:
    d->_ast = d->_parser->parse_grant(d->_parser).tree;
    break;
  case PuDataType:
    d->_ast = d->_parser->data_type_definition(d->_parser).tree;
    break;
  default:
    d->_ast = d->_parser->query(d->_parser).tree;
    break;
  }
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizer::dump_tree()
{
  log_debug2("Generating parse tree\n");

  return dump_tree(d->_ast, "");
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizer::dump_tree(pANTLR3_BASE_TREE tree, const std::string &indentation)
{
  std::string result;

  pANTLR3_RECOGNIZER_SHARED_STATE state = d->_parser->pParser->rec->state;
  ANTLR3_UINT32 char_pos = tree->getCharPositionInLine(tree);
  ANTLR3_UINT32 line = tree->getLine(tree);
  pANTLR3_STRING token_text = tree->getText(tree);

  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
  const char* utf8 = (const char*)token_text->chars;
  if (token != NULL)
  {
    ANTLR3_UINT32 token_type = token->getType(token);

    pANTLR3_UINT8 token_name;
    if (token_type == EOF)
      token_name = (pANTLR3_UINT8)"EOF";
    else
      token_name = state->tokenNames[token_type];

#ifdef  ANTLR3_USE_64BIT
    result = base::strfmt("%s(line: %i, offset: %i, length: %lli, index: %lli, %s[%i])    %s\n",
                          indentation.c_str(), line, char_pos, token->stop - token->start + 1, token->index, token_name,
                          token_type, utf8);
#else
    result = base::strfmt("%s(line: %i, offset: %i, length: %i, index: %i, %s[%i])    %s\n",
                          indentation.c_str(), line, char_pos, token->stop - token->start + 1, token->index, token_name,
                          token_type, utf8);
#endif

  }
  else
  {
    result = base::strfmt("%s(line: %i, offset: %i, nil)    %s\n", indentation.c_str(), line, char_pos, utf8);
  }

  for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++)
  {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
    std::string child_text = dump_tree(child, indentation + "\t");
    result += child_text;
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

const char* MySQLRecognizer::text()
{
  return d->_text;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a tree walker for the current AST.
 */
MySQLRecognizerTreeWalker MySQLRecognizer::tree_walker()
{
  return MySQLRecognizerTreeWalker(this, d->_ast);
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognizer::set_sql_mode(const std::string &new_mode)
{
  MySQLRecognitionBase::set_sql_mode(new_mode);
  d->_context.sql_mode = sql_mode(); // Parsed SQL mode.
}

//--------------------------------------------------------------------------------------------------

void MySQLRecognizer::set_server_version(long new_version)
{
  d->_context.version = new_version;
}

//--------------------------------------------------------------------------------------------------

long MySQLRecognizer::server_version()
{
  return d->_context.version;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the text of the given node. The result depends on the type of the node. If it represents
 * a quoted text entity then two consecutive quote chars are replaced by a single one and if
 * escape sequence parsing is not switched off (as per sql mode) then such sequences are handled too.
 */
std::string MySQLRecognizer::token_text(pANTLR3_BASE_TREE node)
{
  pANTLR3_STRING text = node->getText(node);
  if (text == NULL)
    return "";

  std::string chars;
  pANTLR3_COMMON_TOKEN token = node->getToken(node);
  ANTLR3_UINT32 type = (token != NULL) ? token->type : ANTLR3_TOKEN_INVALID;

  if (type == STRING_TOKEN)
  {
    // STRING is the grouping subtree for multiple consecutive string literals, which
    // must be concatenated.
    for (ANTLR3_UINT32 index = 0; index < node->getChildCount(node); index++)
    {
      pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)node->getChild(node, index);
      chars += token_text(child);
    }

    return chars;
  }

  chars = (const char*)text->chars;
  std::string quote_char;
  switch (type)
  {
    case BACK_TICK_QUOTED_ID:
      quote_char = "`";
      break;
    case SINGLE_QUOTED_TEXT:
      quote_char = "'";
      break;
    case DOUBLE_QUOTED_TEXT:
      quote_char = "\"";
      break;
    default:
      return chars;
  }

  std::string double_quotes = quote_char + quote_char;

  if ((d->_context.sql_mode & SQL_MODE_NO_BACKSLASH_ESCAPES) == 0)
    chars = base::unescape_sql_string(chars, quote_char[0]);
  else
    if (token->user1 > 0)
    {
      // The field user1 is set by the parser to the number of quote char pairs it found.
      // So we can use it to shortcut our handling here.
      base::replace(chars, double_quotes, quote_char);
    }

  return chars.substr(1, chars.size() - 2);
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines the type of the query the recognizer is set to (and has parsed).
 * We don't check the validity of a query here, but only scan as much info as needed to
 * disambiguate the queries.
 */

MySQLQueryType MySQLRecognizer::query_type()
{
  return query_type(d->_ast);
}

//--------------------------------------------------------------------------------------------------

MySQLQueryType MySQLRecognizer::query_type(pANTLR3_BASE_TREE node)
{
  MySQLRecognizerTreeWalker walker(this, node);
  if (node->getToken(node) == NULL) // If we are at the root node advance to the first real node.
    walker.next();

  switch (walker.token_type())
  {
  case ALTER_SYMBOL:
    if (!walker.next())
      return QtAmbiguous;

    switch (walker.token_type())
    {
    case DATABASE_SYMBOL:
      return QtAlterDatabase;

    case LOGFILE_SYMBOL:
      return QtAlterLogFileGroup;

    case FUNCTION_SYMBOL:
      return QtAlterFunction;

    case PROCEDURE_SYMBOL:
      return QtAlterProcedure;

    case SERVER_SYMBOL:
      return QtAlterServer;

    case TABLE_SYMBOL:
    case ONLINE_SYMBOL:  // Optional part of ALTER TABLE.
    case OFFLINE_SYMBOL: // ditto
    case IGNORE_SYMBOL:
      return QtAlterTable;

    case TABLESPACE_SYMBOL:
      return QtAlterTableSpace;

    case EVENT_SYMBOL:
      return QtAlterEvent;

    case VIEW_SYMBOL:
      return QtAlterView;

    case DEFINER_SYMBOL: // Can be both event or view.
      if (!walker.next_sibling()) // DEFINER has an own subtree so we can just jump over the details.
        return QtAmbiguous;

      switch (walker.token_type())
      {
      case EVENT_SYMBOL:
        return QtAlterEvent;

      case SQL_SYMBOL:
      case VIEW_SYMBOL:
        return QtAlterView;
      }
      break;

    case ALGORITHM_SYMBOL: // Optional part of CREATE VIEW.
      return QtAlterView;

    case USER_SYMBOL:
      return QtAlterUser;
    }
    break;

  case CREATE_SYMBOL:
    if (!walker.next())
      return QtAmbiguous;

    switch (walker.token_type())
    {
    case TEMPORARY_SYMBOL: // Optional part of CREATE TABLE.
    case TABLE_SYMBOL:
      return QtCreateTable;

    case ONLINE_SYMBOL:
    case OFFLINE_SYMBOL:
    case INDEX_SYMBOL:
      return QtCreateIndex;

    case DATABASE_SYMBOL:
      return QtCreateDatabase;

    case DEFINER_SYMBOL: // Can be event, view, procedure, function, UDF, trigger.
      {
        if (!walker.next_sibling())
          return QtAmbiguous;

        switch (walker.token_type())
        {
        case EVENT_SYMBOL:
          return QtCreateEvent;

        case VIEW_SYMBOL:
        case SQL_SYMBOL:
          return QtAlterView;

        case PROCEDURE_SYMBOL:
          return QtCreateProcedure;

        case FUNCTION_SYMBOL:
          {
            if (!walker.next() || !walker.is_identifier())
              return QtAmbiguous;

            if (!walker.next())
              return QtAmbiguous;

            if (walker.token_type() == RETURNS_SYMBOL)
              return QtCreateUdf;
            return QtCreateFunction;
          }

        case AGGREGATE_SYMBOL:
          return QtCreateUdf;

        case TRIGGER_SYMBOL:
          return QtCreateTrigger;
        }
      }

    case OR_SYMBOL:        // CREATE OR REPLACE ... VIEW
    case ALGORITHM_SYMBOL: // CREATE ALGORITHM ... VIEW
      return QtCreateView;

    case LOGFILE_SYMBOL:
      return QtCreateLogFileGroup;

    case SERVER_SYMBOL:
      return QtCreateServer;

    case TABLESPACE_SYMBOL:
      return QtCreateTableSpace;

    case USER_SYMBOL:
      return QtCreateUser;
    }
    break;

  case DROP_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      switch (walker.token_type())
      {
      case DATABASE_SYMBOL:
        return QtDropDatabase;

      case EVENT_SYMBOL:
        return QtDropEvent;

      case PROCEDURE_SYMBOL:
        return QtDropProcedure;

      case FUNCTION_SYMBOL:
        return QtDropFunction;

      case ONLINE_SYMBOL:
      case OFFLINE_SYMBOL:
      case INDEX_SYMBOL:
        return QtDropIndex;

      case LOGFILE_SYMBOL:
        return QtDropLogfileGroup;

      case SERVER_SYMBOL:
        return QtDropServer;

      case TEMPORARY_SYMBOL:
      case TABLE_SYMBOL:
      case TABLES_SYMBOL:
        return QtDropTable;

      case TABLESPACE_SYMBOL:
        return QtDropTablespace;

      case TRIGGER_SYMBOL:
        return QtDropTrigger;

      case VIEW_SYMBOL:
        return QtDropView;

      case PREPARE_SYMBOL:
        return QtDeallocate;

      case USER_SYMBOL:
        return QtDropUser;
      }
    }

  case TRUNCATE_SYMBOL:
    return QtTruncateTable;

  case CALL_SYMBOL:
    return QtCall;

  case DELETE_SYMBOL:
    return QtDelete;

  case DO_SYMBOL:
    return QtDo;

  case HANDLER_SYMBOL:
    return QtHandler;

  case INSERT_SYMBOL:
    return QtInsert;

  case LOAD_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      switch (walker.token_type())
      {
      case DATA_SYMBOL:
        {
          if (!walker.next())
            return QtAmbiguous;

          if (walker.token_type() == FROM_SYMBOL)
            return QtLoadDataMaster;
          return QtLoadData;
        }
      case XML_SYMBOL:
        return QtLoadXML;

      case TABLE_SYMBOL:
        return QtLoadTableMaster;

      case INDEX_SYMBOL:
        return QtLoadIndex;
      }
    }

  case REPLACE_SYMBOL:
      return QtReplace;

  case SELECT_SYMBOL:
    return QtSelect;

  case UPDATE_SYMBOL:
    return QtUpdate;

  case OPEN_PAR_SYMBOL: // Either (((select ..))) or (partition...)
    {
      while (walker.token_type() == OPEN_PAR_SYMBOL)
      {
        if (!walker.next())
          return QtAmbiguous;
      }
      if (walker.token_type() == SELECT_SYMBOL)
        return QtSelect;
      return QtPartition;
    }

  case PARTITION_SYMBOL:
  case PARTITIONS_SYMBOL:
    return QtPartition;

  case START_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      if (walker.token_type() == TRANSACTION_SYMBOL)
        return QtStartTransaction;
      return QtStartSlave;
    }

  case BEGIN_SYMBOL: // Begin directly at the start of the query must be a transaction start.
    return QtBeginWork;

  case COMMIT_SYMBOL:
    return QtCommit;

  case ROLLBACK_SYMBOL:
    {
      // We assume a transaction statement here unless we exactly know it's about a savepoint.
      if (!walker.next())
        return QtRollbackWork;
      if (walker.token_type() == WORK_SYMBOL)
      {
        if (!walker.next())
          return QtRollbackWork;
      }

      if (walker.token_type() == TO_SYMBOL)
        return QtRollbackSavepoint;
      return QtRollbackWork;
    }

  case SET_SYMBOL:
    {
      if (!walker.next())
        return QtSet;

      switch (walker.token_type())
      {
      case PASSWORD_SYMBOL:
        return QtSetPassword;

      case GLOBAL_SYMBOL:
      case LOCAL_SYMBOL:
      case SESSION_SYMBOL:
        if (!walker.next())
          return QtSet;
        break;

      case IDENTIFIER:
        if (base::tolower(walker.token_text()) == "autocommit")
          return QtSetAutoCommit;
        break;
      }

      if (walker.token_type() == TRANSACTION_SYMBOL)
        return QtSetTransaction;
      return QtSet;
    }

  case SAVEPOINT_SYMBOL:
    return QtSavepoint;

  case RELEASE_SYMBOL: // Release at the start of the query, obviously.
    return QtReleaseSavepoint;

  case LOCK_SYMBOL:
    return QtLock;

  case UNLOCK_SYMBOL:
    return QtUnlock;

  case XA_SYMBOL:
    return QtXA;

  case PURGE_SYMBOL:
    return QtPurge;

  case CHANGE_SYMBOL:
    return QtChangeMaster;

  case RESET_SYMBOL:
    {
      if (!walker.next())
        return QtReset;

      switch (walker.token_type())
      {
      case SERVER_SYMBOL:
        return QtResetMaster;
      case SLAVE_SYMBOL:
        return QtResetSlave;
      default:
        return QtReset;
      }
    }

  case STOP_SYMBOL:
    return QtStopSlave;

  case PREPARE_SYMBOL:
    return QtPrepare;

  case EXECUTE_SYMBOL:
    return QtExecute;

  case DEALLOCATE_SYMBOL:
    return QtDeallocate;

  case GRANT_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      if (walker.token_type() == PROXY_SYMBOL)
        return QtGrantProxy;
      return QtGrant;
    }

  case RENAME_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      if (walker.token_type() == USER_SYMBOL)
        return QtRenameUser;
      return QtRenameTable;
    }

  case REVOKE_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      if (walker.token_type() == PROXY_SYMBOL)
        return QtRevokeProxy;
      return QtRevoke;
    }

  case ANALYZE_SYMBOL:
    return QtAnalyzeTable;

  case CHECK_SYMBOL:
    return QtCheckTable;

  case CHECKSUM_SYMBOL:
    return QtChecksumTable;

  case OPTIMIZE_SYMBOL:
    return QtOptimizeTable;

  case REPAIR_SYMBOL:
    return QtRepairTable;

  case BACKUP_SYMBOL:
    return QtBackUpTable;

  case RESTORE_SYMBOL:
    return QtRestoreTable;

  case INSTALL_SYMBOL:
    return QtInstallPlugin;

  case UNINSTALL_SYMBOL:
    return QtUninstallPlugin;

  case SHOW_SYMBOL:
    {
      if (!walker.next())
        return QtShow;

      if (walker.token_type() == FULL_SYMBOL)
      {
        // Not all SHOW cases allow an optional FULL keyword, but this is not about checking for
        // a valid query but to find the most likely type.
        if (!walker.next())
          return QtShow;
      }

      switch (walker.token_type())
      {
      case GLOBAL_SYMBOL:
      case LOCK_SYMBOL:
      case SESSION_SYMBOL:
        {
          if (!walker.next())
            return QtShow;

          if (walker.token_type() == STATUS_SYMBOL)
            return QtShowStatus;
          return QtShowVariables;
        }

      case AUTHORS_SYMBOL:
        return QtShowAuthors;

      case BINARY_SYMBOL:
        return QtShowBinaryLogs;

      case BINLOG_SYMBOL:
        return QtShowBinlogEvents;

      case RELAYLOG_SYMBOL:
        return QtShowRelaylogEvents;

      case CHAR_SYMBOL:
        return QtShowCharset;

      case COLLATION_SYMBOL:
        return QtShowCollation;

      case COLUMNS_SYMBOL:
        return QtShowColumns;

      case CONTRIBUTORS_SYMBOL:
        return QtShowContributors;

      case COUNT_SYMBOL:
        {
          if (!walker.next() || walker.token_type() != OPEN_PAR_SYMBOL)
            return QtShow;
          if (!walker.next() || walker.token_type() != MULT_OPERATOR)
            return QtShow;
          if (!walker.next() || walker.token_type() != CLOSE_PAR_SYMBOL)
            return QtShow;

          if (!walker.next())
            return QtShow;

          switch (walker.token_type())
          {
          case WARNINGS_SYMBOL:
            return QtShowWarnings;

          case ERRORS_SYMBOL:
            return QtShowErrors;
          }

          return QtShow;
        }

      case CREATE_SYMBOL:
        {
          if (!walker.next())
            return QtShow;

          switch (walker.token_type())
          {
          case DATABASE_SYMBOL:
            return QtShowCreateDatabase;

          case EVENT_SYMBOL:
            return QtShowCreateEvent;

          case FUNCTION_SYMBOL:
            return QtShowCreateFunction;

          case PROCEDURE_SYMBOL:
            return QtShowCreateProcedure;

          case TABLE_SYMBOL:
            return QtShowCreateTable;

          case TRIGGER_SYMBOL:
            return QtShowCreateTrigger;

          case VIEW_SYMBOL:
            return QtShowCreateView;
          }

          return QtShow;
        }

      case DATABASES_SYMBOL:
        return QtShowDatabases;

      case ENGINE_SYMBOL:
        return QtShowEngineStatus;

      case STORAGE_SYMBOL:
      case ENGINES_SYMBOL:
        return QtShowStorageEngines;

      case ERRORS_SYMBOL:
        return QtShowErrors;

      case EVENTS_SYMBOL:
        return QtShowEvents;

      case FUNCTION_SYMBOL:
        {
          if (!walker.next())
            return QtAmbiguous;

          if (walker.token_type() == CODE_SYMBOL)
            return QtShowFunctionCode;
          return QtShowFunctionStatus;
        }

      case GRANT_SYMBOL:
        return QtShowGrants;

      case INDEX_SYMBOL:
      case INDEXES_SYMBOL:
      case KEY_SYMBOL:
        return QtShowIndexes;

      case INNODB_SYMBOL:
        return QtShowInnoDBStatus;

      case MASTER_SYMBOL:
        return QtShowMasterStatus;

      case OPEN_SYMBOL:
        return QtShowOpenTables;

      case PLUGIN_SYMBOL:
      case PLUGINS_SYMBOL:
        return QtShowPlugins;

      case PROCEDURE_SYMBOL:
        {
          if (!walker.next())
            return QtShow;

          if (walker.token_type() == STATUS_SYMBOL)
            return QtShowProcedureStatus;
          return QtShowProcedureCode;
        }

      case PRIVILEGES_SYMBOL:
        return QtShowPrivileges;

      case PROCESSLIST_SYMBOL:
        return QtShowProcessList;

      case PROFILE_SYMBOL:
        return QtShowProfile;

      case PROFILES_SYMBOL:
        return QtShowProfiles;

      case SLAVE_SYMBOL:
        {
          if (!walker.next())
            return QtAmbiguous;

          if (walker.token_type() == HOSTS_SYMBOL)
            return QtShowSlaveHosts;
          return QtShowSlaveStatus;
        }

      case STATUS_SYMBOL:
        return QtShowStatus;

      case VARIABLES_SYMBOL:
        return QtShowVariables;

      case TABLE_SYMBOL:
        return QtShowTableStatus;

      case TABLES_SYMBOL:
        return QtShowTables;

      case TRIGGERS_SYMBOL:
        return QtShowTriggers;

      case WARNINGS_SYMBOL:
        return QtShowWarnings;
      }

      return QtShow;
    }

  case CACHE_SYMBOL:
    return QtCacheIndex;

  case FLUSH_SYMBOL:
    return QtFlush;

  case KILL_SYMBOL:
    return QtKill;


  case DESCRIBE_SYMBOL: // EXPLAIN is converted to DESCRIBE in the lexer.
  case DESC_SYMBOL:
    {
      if (!walker.next())
        return QtAmbiguous;

      if (walker.is_identifier() || walker.token_type() == DOT_SYMBOL)
        return QtExplainTable;

      // EXTENDED is a bit special as it can be both, a table identifier or the keyword.
      if (walker.token_type() == EXTENDED_SYMBOL)
      {
        if (!walker.next())
          return QtExplainTable;

        switch (walker.token_type())
        {
        case DELETE_SYMBOL:
        case INSERT_SYMBOL:
        case REPLACE_SYMBOL:
        case UPDATE_SYMBOL:
          return QtExplainStatement;
        default:
          return QtExplainTable;
        }
      }
      return QtExplainStatement;
    }

  case HELP_SYMBOL:
    return QtHelp;

  case USE_SYMBOL:
    return QtUse;
  }
  
  return QtUnknown;
}

//--------------------------------------------------------------------------------------------------

/**
 * If the given node is a subtree this function collects the original text for all tokens in this tree,
 * including all tokens on the hidden channel (whitespaces).
 */
std::string MySQLRecognizer::text_for_tree(pANTLR3_BASE_TREE node)
{
  if (node->getChildCount(node) == 0)
    return "";

  std::string result;

  pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)node->getChild(node, 0);
  pANTLR3_COMMON_TOKEN token = child->getToken(child);
  ANTLR3_MARKER start = token->start;

  child = (pANTLR3_BASE_TREE)node->getChild(node, node->getChildCount(node) - 1);
  token = child->getToken(child);
  ANTLR3_MARKER stop = token->stop;
  return std::string((char*)start, stop - start + 1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the information for the token at the given index in the input stream. This includes
 * every possible token, including those on a hidden channel (e.g. comments and whitespaces).
 * Before calling this function the parser must have parsed the input to have the values available.
 * The result's type member can be used to find out if token information is not yet available or
 * the given index is out of the available range (ANTLR3_TOKEN_INVALID).
 */
MySQLToken MySQLRecognizer::token_at_index(ANTLR3_MARKER index)
{
  MySQLToken result;

  pANTLR3_COMMON_TOKEN token = d->_tokens->tstream->get(d->_tokens->tstream, (ANTLR3_UINT32)index);
  if (token != NULL)
  {
    result.type = token->type;
    result.line = token->line;
    result.position = token->charPosition;
    result.index = token->index;
    result.channel = token->channel;
    result.line_start = (char*)token->lineStart;
    result.start = reinterpret_cast<char*>(token->start);
    result.stop = reinterpret_cast<char*>(token->stop);

    // If necessary the following part can be optimized to not always create a copy of the input.
    pANTLR3_STRING text = token->getText(token);
    result.text = (const char*)text->chars;
  }

  return result;
}

//--------------------------------------------------------------------------------------------------
