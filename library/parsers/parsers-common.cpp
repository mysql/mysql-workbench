/*
 * Copyright (c) 2015, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <antlr3.h>
#include <regex>

#include "base/string_utilities.h"
#include "parsers-common.h"

extern "C" {

//--------------------------------------------------------------------------------------------------

/**
 * Tries to match the given text against the (regex) pattern.
 * Returns true if it matches, false otherwise.
 * Note: text can be utf-8 encoded, which only works reliably if pattern doesn't contain utf-8 chars.
 *       Additionally, the function is not very effective, as it creates a new regex class on each
 *       invocation, but should be ok for occasional execution as used while parsing.
 */
ANTLR3_BOOLEAN matches(pANTLR3_STRING text, const char *pattern) {
  return std::regex_match((const char *)text->chars, std::regex(pattern));
}
}

//------------------ IRecognizer ------------------------------------------------------------------

std::string IRecognizer::dumpTree(pANTLR3_UINT8 *tokenNames, pANTLR3_BASE_TREE tree, const std::string &indentation) {
  std::string result;

  ANTLR3_UINT32 char_pos = tree->getCharPositionInLine(tree);
  ANTLR3_UINT32 line = tree->getLine(tree);
  pANTLR3_STRING token_text = tree->getText(tree);

  pANTLR3_COMMON_TOKEN token = tree->getToken(tree);
  const char *utf8 = (const char *)token_text->chars;
  if (token != NULL) {
    ANTLR3_UINT32 token_type = token->getType(token);

    pANTLR3_UINT8 token_name;
    if (token_type == EOF)
      token_name = (pANTLR3_UINT8) "EOF";
    else
      token_name = tokenNames[token_type];

#ifdef ANTLR3_USE_64BIT
    result = base::strfmt("%s(line: %i, offset: %i, length: %" PRId64 ", index: %" PRId64 ", %s[%i])    %s\n",
                          indentation.c_str(), line, char_pos, token->stop - token->start + 1, token->index, token_name,
                          token_type, utf8);
#else
    result = base::strfmt("%s(line: %i, offset: %i, length: %i, index: %i, %s[%i])    %s\n", indentation.c_str(), line,
                          char_pos, token->stop - token->start + 1, token->index, token_name, token_type, utf8);
#endif

  } else {
    result = base::strfmt("%s(line: %i, offset: %i, nil)    %s\n", indentation.c_str(), line, char_pos, utf8);
  }

  for (ANTLR3_UINT32 index = 0; index < tree->getChildCount(tree); index++) {
    pANTLR3_BASE_TREE child = (pANTLR3_BASE_TREE)tree->getChild(tree, index);
    std::string child_text = dumpTree(tokenNames, child, indentation + "\t");
    result += child_text;
  }
  return result;
}

//----------------- MySQLTreeWalker ----------------------------------------------------------------

struct CompareTokenIndex {
  inline bool operator()(const pANTLR3_BASE_TREE left, const pANTLR3_BASE_TREE right) {
    pANTLR3_COMMON_TOKEN t1 = left->getToken(left);
    pANTLR3_COMMON_TOKEN t2 = right->getToken(right);
    return t1->index < t2->index;
  }
};

RecognizerTreeWalker::RecognizerTreeWalker(IRecognizer *recognizer, pANTLR3_BASE_TREE tree) {
  _recognizer = recognizer;
  _tree = tree;

  if (tokenType() == 0) // If there's a null root node skip over that.
    next();

  _origin = _tree;

  // Fill the list of tokens for quick lookup by type or position in the correct order.
  pANTLR3_BASE_TREE run = _tree;
  while (run != nullptr) {
    // Add only entries that carry useful information for position search.
    pANTLR3_COMMON_TOKEN token = run->getToken(run);
    if (token != nullptr && token->lineStart != nullptr) // Virtual tokens have no line information.
      _tokenList.push_back(run);
    run = getNext(run, true);
  }

  // Sort token list by token index, which puts them in appearance order.
  if (_tokenList.size() > 1)
    std::sort(_tokenList.begin(), _tokenList.end(), CompareTokenIndex());
}

//--------------------------------------------------------------------------------------------------

void RecognizerTreeWalker::printToken(pANTLR3_BASE_TREE tree) {
  pANTLR3_STRING tokenText = tree->getText(tree);
  printf("Token: %s\n", (tokenText == NULL) ? "nil" : (char *)tokenText->chars);
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
bool RecognizerTreeWalker::next(std::size_t count) {
  pANTLR3_BASE_TREE node = _tree;
  while (count > 0) {
    node = getNext(node, true);
    if (node == nullptr)
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
bool RecognizerTreeWalker::nextSibling() {
  pANTLR3_BASE_TREE node = getNext(_tree, false);
  if (node != nullptr) {
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
bool RecognizerTreeWalker::previous() {
  pANTLR3_BASE_TREE node = getPrevious(_tree, true);
  if (node == nullptr)
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
bool RecognizerTreeWalker::previousByIndex() {
  pANTLR3_BASE_TREE previous = getPreviousByIndex(_tree);
  if (previous == nullptr)
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
bool RecognizerTreeWalker::previousSibling() {
  pANTLR3_BASE_TREE node = getPrevious(_tree, false);
  if (node == nullptr)
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
bool RecognizerTreeWalker::up() {
  pANTLR3_BASE_TREE parent = _tree->getParent(_tree);
  if (parent == nullptr)
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
bool RecognizerTreeWalker::advanceToPosition(int line, int offset) {
  if (_tokenList.size() == 0)
    return false;

  size_t i = 0;
  for (; i < _tokenList.size(); i++) {
    pANTLR3_BASE_TREE run = _tokenList[i];
    ANTLR3_UINT32 tokenLine = run->getLine(run);
    if ((int)tokenLine >= line) {
      int tokenOffset = run->getCharPositionInLine(run);
      pANTLR3_COMMON_TOKEN token = run->getToken(run);
      int tokenLength = (int)(token->stop - token->start) + 1;
      if ((int)tokenLine == line && tokenOffset <= offset && offset < tokenOffset + tokenLength) {
        _tree = _tokenList[i];
        break;
      }

      if ((int)tokenLine > line || (int)tokenOffset > offset) {
        // We reached a token after the current offset. Take the previous one as result then.
        if (i == 0)
          return false;

        _tree = _tokenList[i - 1];
        break;
      }
    }
  }

  if (i == _tokenList.size())
    _tree = _tokenList[i - 1]; // Nothing found, take the last token instead.

  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next token with the given lexical type.
 *
 * @param type The token type to search.
 * @param recurse If false search only siblings, otherwise any node in any tree level.
 * @return True if such a node exists, false otherwise (no change performed then).
 */
bool RecognizerTreeWalker::advanceToType(uint32_t type, bool recurse) {
  pANTLR3_BASE_TREE run = _tree;
  while (true) {
    run = getNext(run, recurse);
    if (run == nullptr)
      return false;

    if (run->getType(run) == type) {
      _tree = run;
      return true;
    }
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Steps over a number of tokens and positions in the walker.
 * The tokens must all be siblings of the current token and are traversed in exactly the given order
 * without intermediate tokens. The current token must be start_token.
 *
 * Note: the list must be terminated by ANTLR3_TOKEN_INVALID (or 0).
 *
 * @return True if all the given tokens were found and there is another sibling after the last token
 *         in the list, false otherwise. If the token sequence could not be found or there is no more
 *         token the internal state is undefined.
 */
bool RecognizerTreeWalker::skipTokenSequence(uint32_t startToken, ...) {
  bool result = false;

  unsigned int token = startToken;
  va_list tokens;
  va_start(tokens, startToken);
  while (true) {
    if (tokenType() != token)
      break;

    if (!nextSibling())
      break;

    token = va_arg(tokens, unsigned int);
    if (token == ANTLR3_TOKEN_INVALID) {
      result = true;
      break;
    }
  }
  va_end(tokens);

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the nth next token if the current one is that given by @token.
 * Returns true if we skipped actually.
 */
bool RecognizerTreeWalker::skipIf(uint32_t token, size_t count) {
  if (tokenType() == token) {
    next(count);
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Advances to the next node after the current subtree. If the current node is not a subtree then
 * we simply skip to the next node after that.
 * Properly handles subtrees at the end of a child list where there is no next sibling.
 */
void RecognizerTreeWalker::skipSubtree() {
  if (isSubtree()) {
    if (nextSibling())
      return;

    do {
      up();
    } while (!nextSibling()); // Terminates reliably on the top level as there is always the EOF node.

    return;
  }

  next();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the type of the next sibling (if recursive is false) or the next token (including child
 * nodes) without changing the internal state.
 */
uint32_t RecognizerTreeWalker::lookAhead(bool recursive) {
  pANTLR3_BASE_TREE next = getNext(_tree, recursive);
  if (next == nullptr)
    return ANTLR3_TOKEN_INVALID;
  return next->getType(next);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the parent's token type if we are in a subtree.
 */
uint32_t RecognizerTreeWalker::parentType() {
  pANTLR3_BASE_TREE parent = _tree->getParent(_tree);
  if (parent == nullptr)
    return ANTLR3_TOKEN_INVALID;

  return parent->getType(parent);
}

//--------------------------------------------------------------------------------------------------

/**
 * Look back in the stream (physical order) what was before the current token, without
 * modifying the current position.
 */
unsigned int RecognizerTreeWalker::previousType() {
  pANTLR3_BASE_TREE previous = getPreviousByIndex(_tree);
  if (previous == nullptr)
    return ANTLR3_TOKEN_INVALID;

  return previous->getType(previous);
}

//--------------------------------------------------------------------------------------------------

/**
 * Resets the walker to be at the original location.
 */
void RecognizerTreeWalker::reset() {
  _tree = _origin;
  while (!_tokenStack.empty())
    _tokenStack.pop();
}

//--------------------------------------------------------------------------------------------------

/**
 * Store the current node on the stack, so we can easily come back when needed.
 */
void RecognizerTreeWalker::push() {
  _tokenStack.push(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns to the location at the top of the token stack (if any).
 */
bool RecognizerTreeWalker::pop() {
  if (_tokenStack.empty())
    return false;

  _tree = _tokenStack.top();
  _tokenStack.pop();
  return true;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes the current top of stack entry without restoring the internal state.
 * Does nothing if the stack is empty.
 */
void RecognizerTreeWalker::removeTos() {
  if (!_tokenStack.empty())
    _tokenStack.pop();
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is of the given type.
 */
bool RecognizerTreeWalker::is(unsigned int type) const {
  return _tree->getType(_tree) == type;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is empty, false otherwise.
 */
bool RecognizerTreeWalker::isNil() const {
  return _tree->isNilNode(_tree) == ANTLR3_TRUE;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token is the root of a subtree (i.e. has child nodes).
 */
bool RecognizerTreeWalker::isSubtree() const {
  return isSubtree(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token has no previous sibling.
 */
bool RecognizerTreeWalker::isFirstChild() const {
  return _tree->getChildIndex(_tree) == 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token has no previous sibling.
 */
bool RecognizerTreeWalker::isIdentifier() const {
  return _recognizer->isIdentifier(tokenType());
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns true if the current token has no previous sibling.
 */
bool RecognizerTreeWalker::isKeyword() const {
  return _recognizer->isKeyword(tokenType());
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the textual expression of the token. This will either return the actual text in the
 * parsed query (if it is a lexer symbol) or the textual expression of the constant name for abstract
 * tokens.
 */
std::string RecognizerTreeWalker::tokenText(bool keepQuotes) const {
  return _recognizer->tokenText(_tree, keepQuotes);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the type of the current token. Same as the type you can specify in advance_to().
 */
uint32_t RecognizerTreeWalker::tokenType() const {
  return _tree->getType(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the (one-base) line number of the token.
 */
uint32_t RecognizerTreeWalker::tokenLine() const {
  return _tree->getLine(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the (zero-based) offset of the token on its line.
 */
uint32_t RecognizerTreeWalker::tokenStart() const {
  return _tree->getCharPositionInLine(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the (zero-based) index of the current token within the input.
 */
int64_t RecognizerTreeWalker::tokenIndex() const {
  pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
  return token->index;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the offset of the token in its source string.
 */
size_t RecognizerTreeWalker::tokenOffset() const {
  pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
  return (size_t)(token->start - (ANTLR3_MARKER)_recognizer->lineStart());
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the length of the token in bytes.
 */
int64_t RecognizerTreeWalker::tokenLength() const {
  pANTLR3_COMMON_TOKEN token = _tree->getToken(_tree);
  if (token == nullptr)
    return 0;

  // Start and stop are actually pointers into the input stream.
  return (int)token->stop - (int)token->start + 1;
}

//--------------------------------------------------------------------------------------------------

std::string RecognizerTreeWalker::textForTree() const {
  return _recognizer->textForTree(_tree);
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the next node after the given one without changing any internal state or NULL if there's
 * no next node. The recurse flag determines if we can change tree levels or stay in the one we are in
 * currently.
 */
pANTLR3_BASE_TREE RecognizerTreeWalker::getNext(pANTLR3_BASE_TREE node, bool recurse) const {
  if (recurse) {
    // If there are child take the first one.
    if (node->getChildCount(node) > 0)
      return (pANTLR3_BASE_TREE)_tree->getChild(node, 0);
  }

  // No child nodes (or no recursion), so see if there is another sibling of this node or one of its parents.
  while (true) {
    pANTLR3_BASE_TREE parent = node->getParent(node);
    if (parent == nullptr)
      return nullptr;

    int index = parent->getChildIndex(node) + 1;
    if ((int)parent->getChildCount(parent) > index)
      return (pANTLR3_BASE_TREE)parent->getChild(parent, index);

    if (!recurse)
      return nullptr;

    // No sibling either - go up one level and try the next sibling of the parent.
    node = parent;
  }
  return nullptr;
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns the previous node before the given one without changing any internal state or NULL if there's
 * no previous node. The meaning of the recurse flag is the same as for get_next().
 */
pANTLR3_BASE_TREE RecognizerTreeWalker::getPrevious(pANTLR3_BASE_TREE node, bool recurse) const {
  pANTLR3_BASE_TREE parent = _tree->getParent(_tree);
  if (parent == nullptr)
    return NULL;

  int index = parent->getChildIndex(_tree) - 1;
  if (index < 0) {
    if (!recurse)
      return nullptr;
    return parent;
  }

  pANTLR3_BASE_TREE last_node = (pANTLR3_BASE_TREE)parent->getChild(parent, index);
  if (recurse) {
    while (last_node->getChildCount(last_node) > 0) {
      // Walk down the entire tree hierarchy to the last sub node of the previous sibling.
      index = last_node->getChildCount(last_node) - 1;
      last_node = (pANTLR3_BASE_TREE)last_node->getChild(last_node, index);
    }
  }

  return last_node;
}

//--------------------------------------------------------------------------------------------------

pANTLR3_BASE_TREE RecognizerTreeWalker::getPreviousByIndex(pANTLR3_BASE_TREE node) const {
  if (node == nullptr)
    return nullptr;

  auto iterator = lower_bound(_tokenList.begin(), _tokenList.end(), node, CompareTokenIndex());
  if (iterator == _tokenList.end())
    return nullptr;

  if (iterator == _tokenList.begin())
    return nullptr;

  return *(--iterator);
}

//--------------------------------------------------------------------------------------------------

bool RecognizerTreeWalker::isSubtree(struct ANTLR3_BASE_TREE_struct *tree) const {
  return tree->getChildCount(tree) > 0;
}

//--------------------------------------------------------------------------------------------------
