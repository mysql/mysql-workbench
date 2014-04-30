/*
 * Copyright (c) 2012, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "MySQLLexer.h"  // The generated lexer.
#include "MySQLParser.h" // The generated parser.

#include "base/log.h"
#include "base/string_utilities.h"

#include "mysql-parser.h"
#include "mysql-scanner.h"

DEFAULT_LOG_DOMAIN("MySQL parsing")

extern "C" {

  /**
   * Error report function which is set in the parser (see MySQL.g where this is done).
   * This function handles all 3 recognizer types (lexer, parser and tree parser).
   */
  void on_parse_error(struct ANTLR3_BASE_RECOGNIZER_struct *recognizer, pANTLR3_UINT8 *tokenNames)
  {
    std::ostringstream error;

	  pANTLR3_LEXER lexer;
	  pANTLR3_PARSER parser;
	  pANTLR3_TREE_PARSER tparser;
	  pANTLR3_INT_STREAM is;
	  pANTLR3_STRING ttext;
	  pANTLR3_STRING ftext;
	  pANTLR3_EXCEPTION ex;
	  pANTLR3_COMMON_TOKEN theToken = NULL;
	  pANTLR3_BASE_TREE theBaseTree;
	  pANTLR3_COMMON_TREE theCommonTree;

    // TODO: the generated message can be simplified, now that we also return token info.

	  // Retrieve some info for easy reading.
	  ex = recognizer->state->exception;
	  ttext = NULL;

	  // See if there is a 'filename' we can use.
	  if	(ex->streamName == NULL)
	  {
		  if	(((pANTLR3_COMMON_TOKEN)(ex->token))->type == ANTLR3_TOKEN_EOF)
			  error << "-end of input-(";
		  else
			  error << "-unknown source-(";
	  }
	  else
	  {
		  ftext = ex->streamName->to8(ex->streamName);
		  error << ftext->chars << "(";
	  }

	  // Next comes the line number.
	  error << recognizer->state->exception->line << ") ";
	  error << " : error " << recognizer->state->exception->type << " : " <<	(pANTLR3_UINT8)(recognizer->state->exception->message);

	  // How we determine the next piece is dependent on which thing raised the error.
	  switch	(recognizer->type)
	  {
      case ANTLR3_TYPE_LEXER:
      {
        lexer = (pANTLR3_LEXER)(recognizer->super);
        parser = NULL;
        tparser = NULL;

        error << " : lexer error " << ex->type << " : " <<	(pANTLR3_UINT8)(ex->message);
        error << ", at offset " << ex->charPositionInLine + 1;

        {
          ANTLR3_INT32	width;

          width	= ANTLR3_UINT32_CAST(( reinterpret_cast<pANTLR3_UINT8>(lexer->input->data) + (lexer->input->size(lexer->input) )) - reinterpret_cast<pANTLR3_UINT8>(ex->index));

          if	(width >= 1)
          {
            if	(isprint(ex->c))
              error << " near '" << (char)ex->c << "' :\n";
            else
              error << " near char(" << (ANTLR3_UINT8)(ex->c) << ") :\n";
          }
          else
          {
            error << "(end of input).\n\t Probably an unfinished wrapped expression (e.g. \"string\", (..) etc.\n";
            width = ANTLR3_UINT32_CAST((reinterpret_cast<pANTLR3_UINT8>(lexer->input->data) + (lexer->input->size(lexer->input))) - reinterpret_cast<pANTLR3_UINT8>(lexer->rec->state->tokenStartCharIndex));
          }
        }

        break;
      }

      case	ANTLR3_TYPE_PARSER:
        // Prepare the knowledge we know we have.
        parser = (pANTLR3_PARSER) (recognizer->super);
        tparser = NULL;
        is = parser->tstream->istream;
        theToken = (pANTLR3_COMMON_TOKEN)(recognizer->state->exception->token);
        ttext = theToken->toString(theToken);

        error << ", at offset " << recognizer->state->exception->charPositionInLine;
        if  (theToken != NULL)
        {
          if (theToken->type == ANTLR3_TOKEN_EOF)
            error << ", at <EOF>";
          else
            error << "\n    near " << (ttext == NULL ? (pANTLR3_UINT8)"<no text for the token>" : ttext->chars) << "\n    ";
        }
        break;

      case	ANTLR3_TYPE_TREE_PARSER:
        tparser		= (pANTLR3_TREE_PARSER) (recognizer->super);
        parser		= NULL;
        is			= tparser->ctnstream->tnstream->istream;
        theBaseTree	= (pANTLR3_BASE_TREE)(recognizer->state->exception->token);
        ttext		= theBaseTree->toStringTree(theBaseTree);

        if  (theBaseTree != NULL)
        {
          theCommonTree	= (pANTLR3_COMMON_TREE)	    theBaseTree->super;

          if	(theCommonTree != NULL)
            theToken	= (pANTLR3_COMMON_TOKEN)theBaseTree->getToken(theBaseTree);
          error << ", at offset " << theBaseTree->getCharPositionInLine(theBaseTree) << ", near " << ttext->chars;
        }
        break;

      default:
        error << "Internal error: unknown recognizer type appeared in error reporting.\n";
        return;
        break;
	  }

	  // Although this function should generally be provided by the implementation, this one
	  // should be as helpful as possible for grammar developers and serve as an example
	  // of what you can do with each exception type. In general, when you make up your
	  // 'real' handler, you should debug the routine with all possible errors you expect
	  // which will then let you be as specific as possible about all circumstances.
	  //
	  // Note that in the general case, errors thrown by tree parsers indicate a problem
	  // with the output of the parser or with the tree grammar itself. The job of the parser
	  // is to produce a perfect (in traversal terms) syntactically correct tree, so errors
	  // at that stage should really be semantic errors that your own code determines and handles
	  // in whatever way is appropriate.
	  switch (ex->type)
	  {
      case ANTLR3_UNWANTED_TOKEN_EXCEPTION:

        // Indicates that the recognizer was fed a token which seesm to be
        // spurious input. We can detect this when the token that follows
        // this unwanted token would normally be part of the syntactically
        // correct stream. Then we can see that the token we are looking at
        // is just something that should not be there and throw this exception.
        if (tokenNames == NULL)
          error << " : Extraneous input...";
        else
        {
          if	(ex->expecting == ANTLR3_TOKEN_EOF)
            error << " : Extraneous input - expected <EOF>\n";
          else
            error << " : Extraneous input - expected " << tokenNames[ex->expecting] <<  "...\n";
        }
        break;

      case ANTLR3_MISSING_TOKEN_EXCEPTION:

        // Indicates that the recognizer detected that the token we just
        // hit would be valid syntactically if preceeded by a particular
        // token. Perhaps a missing ';' at line end or a missing ',' in an
        // expression list, and such like.
        if	(tokenNames == NULL)
        {
          error << " : Missing token (" << ex->expecting << ")...\n";
        }
        else
        {
          if	(ex->expecting == ANTLR3_TOKEN_EOF)
            error << " : Missing <EOF>\n";
          else
            error << " : Missing " << tokenNames[ex->expecting] << " \n";
        }
        break;

      case	ANTLR3_RECOGNITION_EXCEPTION:

        // Indicates that the recognizer received a token
        // in the input that was not predicted. This is the basic exception type
        // from which all others are derived. So we assume it was a syntax error.
        // You may get this if there are not more tokens and more are needed
        // to complete a parse for instance.
        error << " : syntax error...\n";
        break;

      case    ANTLR3_MISMATCHED_TOKEN_EXCEPTION:

        // We were expecting to see one thing and got another. This is the
        // most common error if we coudl not detect a missing or unwanted token.
        // Here you can spend your efforts to
        // derive more useful error messages based on the expected
        // token set and the last token and so on. The error following
        // bitmaps do a good job of reducing the set that we were looking
        // for down to something small. Knowing what you are parsing may be
        // able to allow you to be even more specific about an error.
        if	(tokenNames == NULL)
          error << " : syntax error...\n";
        else
        {
          if	(ex->expecting == ANTLR3_TOKEN_EOF)
            error << " : expected <EOF>\n";
          else
            error << " : expected " << tokenNames[ex->expecting] << " ...\n";
        }
        break;

      case	ANTLR3_NO_VIABLE_ALT_EXCEPTION:

        // We could not pick any alt decision from the input given
        // so god knows what happened - however when you examine your grammar,
        // you should. It means that at the point where the current token occurred
        // that the DFA indicates nowhere to go from here.
        error << " : cannot match to any predicted input...\n";

        break;

      case	ANTLR3_MISMATCHED_SET_EXCEPTION:

		  {
			  ANTLR3_UINT32	  count;
			  ANTLR3_UINT32	  bit;
			  ANTLR3_UINT32	  size;
			  ANTLR3_UINT32	  numbits;
			  pANTLR3_BITSET	errBits;

			  // This means we were able to deal with one of a set of
			  // possible tokens at this point, but we did not see any
			  // member of that set.
			  error << " : unexpected input...\n  expected one of : ";

			  // What tokens could we have accepted at this point in the parse?
			  count   = 0;
			  errBits = antlr3BitsetLoad(ex->expectingSet);
			  numbits = errBits->numBits(errBits);
			  size    = errBits->size(errBits);

			  if  (size > 0)
			  {
				  // However many tokens we could have dealt with here, it is usually
				  // not useful to print ALL of the set here. I arbitrarily chose 8
				  // here, but you should do whatever makes sense for you of course.
				  // No token number 0, so look for bit 1 and on.
				  for	(bit = 1; bit < numbits && count < 8 && count < size; bit++)
				  {
					  // TODO: This doesn;t look right - should be asking if the bit is set!!
					  if  (tokenNames[bit])
					  {
						  error << (count > 0 ? ", " : "") << tokenNames[bit];
						  count++;
					  }
				  }
				  error << "\n";
			  }
			  else
			  {
				  error << "Actually dude, we didn't seem to be expecting anything here, or at least\n";
				  error << "I could not work out what I was expecting, like so many of us these days!\n";
			  }
		  }
        break;

      case	ANTLR3_EARLY_EXIT_EXCEPTION:

        // We entered a loop requiring a number of token sequences
        // but found a token that ended that sequence earlier than
        // we should have done.
        error << " : missing elements...\n";
        break;

      default:

        // We don't handle any other exceptions here, but you can
        // if you wish. If we get an exception that hits this point
        // then we are just going to report what we know about the
        // token.
        error << " : syntax not recognized...\n";
        break;
	  }

    int length = 0;
    if (theToken != NULL)
      length = (int)theToken->stop - (int)theToken->start + 1;

    MySQLParsingBase *our_recognizer = (MySQLParsingBase*)((RecognitionContext*)recognizer->state->userp)->payload;
    if (ex == NULL)
      our_recognizer->add_error(error.str(), 0, 0, 0, 0, 0);
    else
      our_recognizer->add_error(error.str(), ex->type, (theToken == NULL) ? 0 : theToken->type,
                                ex->line, ex->charPositionInLine, length);
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
  _origin = tree;

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
 * @return True if there was a next node, false otherwise. No change in the state is performed if
 *         there was no next node.
 */
bool MySQLRecognizerTreeWalker::next()
{
  pANTLR3_BASE_TREE node = get_next(_tree, true);
  if (node != NULL)
  {
    _tree = node;
    return true;
  }
  return false;
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

  pANTLR3_BASE_TREE current = _tree;
  for (size_t i = 0; i < _token_list.size(); i++)
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

  // If the loop didn't reach a token after the given position then we are
  // within or after the last token.
  if (current == _tree)
    _tree = _token_list[_token_list.size() - 1];

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next token with the given type. The parameter type can be any token id listed in
 * MySQLParser.h (search for "Symbolic definitions"). It can be either a simple lexical token
 * like DIV_OPERATOR or SELECT_SYMBOL, a more complex lexical token like IDENTIFIER or a parser token like
 * GROUP_BY or MISSING_ID. Tokens ending with _SYMBOL are text literals (e.g. "SELECT", "DIV" etc.)
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
  int _text_length;
  int _input_encoding;
  RecognitionContext _context;

  pANTLR3_INPUT_STREAM _input;
  pMySQLLexer _lexer;
  pANTLR3_COMMON_TOKEN_STREAM _tokens;
  pMySQLParser _parser;
  MySQLParser_query_return _ast;
};

MySQLRecognizer::MySQLRecognizer(const char *text, int length, bool is_utf8, long server_version,
                                 const std::string &sql_mode, const std::set<std::string> &charsets)
: MySQLParsingBase(charsets)
{
  d = new Private();
  d->_text = text;
  d->_text_length = length;
  d->_context.version = server_version;
  d->_context.payload = this;
  d->_context.sql_mode = parse_sql_mode(sql_mode);

  // If the text is not using utf-8 (which it should) then we interpret as 8bit encoding
  // (everything requiring only one byte per char as Latin1, ASCII and similar).
  d->_input_encoding = is_utf8 ? ANTLR3_ENC_UTF8 : ANTLR3_ENC_8BIT;
  parse();
}

//--------------------------------------------------------------------------------------------------

MySQLRecognizer::~MySQLRecognizer()
{
  d->_parser->free(d->_parser);
  d->_tokens ->free(d->_tokens);
  d->_lexer->free(d->_lexer);
  d->_input->close(d->_input);

  delete d;
}

//--------------------------------------------------------------------------------------------------

std::string MySQLRecognizer::dump_tree()
{
  log_debug2("Generating parse tree\n");

  return dump_tree(d->_ast.tree, "");
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
    result = base::strfmt("%s(line: %i, offset: %i, length: %li, index: %li, %s[%i])    %s\n",
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

void MySQLRecognizer::parse()
{
  log_debug2("Start parsing\n");

  d->_input = antlr3StringStreamNew((pANTLR3_UINT8)d->_text, d->_input_encoding, d->_text_length,
                                    (pANTLR3_UINT8)"mysql-script");
  d->_input->setUcaseLA(d->_input, ANTLR3_TRUE); // Make input case-insensitive. String literals must all be upper case in the grammar!

  d->_lexer = MySQLLexerNew(d->_input);
  d->_lexer->pLexer->rec->state->userp = &d->_context;
  d->_tokens = antlr3CommonTokenStreamSourceNew(ANTLR3_SIZE_HINT, TOKENSOURCE(d->_lexer));
  d->_parser = MySQLParserNew(d->_tokens);
  d->_parser->pParser->rec->state->userp = &d->_context;

  d->_ast = d->_parser->query(d->_parser);

  ANTLR3_UINT32 error_count = d->_parser->pParser->rec->getNumberOfSyntaxErrors(d->_parser->pParser->rec);
  if (error_count > 0)
    log_debug3("%i errors found\n", error_count);
  log_debug2("Parsing ended\n");
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a tree walker for the current AST.
 */
MySQLRecognizerTreeWalker MySQLRecognizer::tree_walker()
{
  return MySQLRecognizerTreeWalker(this, d->_ast.tree);
}

//--------------------------------------------------------------------------------------------------

unsigned MySQLRecognizer::sql_mode()
{
  return d->_context.sql_mode;
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
  return query_type(d->_ast.tree);
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
