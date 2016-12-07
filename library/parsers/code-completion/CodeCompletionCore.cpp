/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "antlr4-runtime.h"

#include "CodeCompletionCore.h"

using namespace antlr4;
using namespace antlr4::atn;
using namespace antlr4::misc;

std::unordered_map<std::type_index, CodeCompletionCore::FollowSetsPerState> CodeCompletionCore::_followSetsByATN;

CodeCompletionCore::CodeCompletionCore(Parser *parser)
: _parser(parser), _atn(parser->getATN()), _vocabulary(parser->getVocabulary()), _ruleNames(parser->getRuleNames()) {

  BufferedTokenStream *tokenStream = dynamic_cast<BufferedTokenStream *>(parser->getTokenStream());
  if (tokenStream == nullptr)
    throw RuntimeException("Code completion currently only works with a buffered token stream");

  tokenStream->fill();
  for (auto token : tokenStream->getTokens())
    if (token->getChannel() == Token::DEFAULT_CHANNEL)
      _tokens.push_back(token);
}

CandidatesCollection CodeCompletionCore::collectCandidates(std::pair<size_t, size_t> caret, size_t startRule) {
  _caret = caret;
  _candidates.rules.clear();
  _candidates.tokens.clear();

  process(startRule);

  if (showResult) {
    std::cout << std::endl << std::endl << "Collected rules:" << std::endl;
    for (auto candidate : _candidates.rules) {
      std::cout << _ruleNames[candidate.first] << ", path: ";

      for (auto rule : candidate.second) {
        std::cout << _ruleNames[rule] << " ";
      }
      std::cout << std::endl;
    }

    std::set<std::string> sortedTokens;
    for (auto token : _candidates.tokens) {
      std::string value = _vocabulary.getDisplayName(token.first);
      for (size_t following : token.second)
        value += " " + _vocabulary.getDisplayName(following);
      sortedTokens.insert(value);
    }
    std::cout << std::endl << std::endl << "Collected tokens:" << std::endl;
    for (std::string symbol : sortedTokens) {
      std::cout << symbol << std::endl;
    }
    std::cout << std::endl << std::endl;
  }

  return _candidates;
}

/**
 * Check if the predicate associated with the given transition evaluates to true.
 */
bool CodeCompletionCore::checkPredicate(PredicateTransition *transition) const {
  return transition->getPredicate()->eval(_parser, nullptr);
};

/**
 * Walks the rule chain upwards starting at the given state to see if that matches any of the preferred rules.
 * If found, that rule is added to the collection candidates and true is returned.
 */
bool CodeCompletionCore::translateToRuleIndex(std::vector<size_t> const& ruleStack) {
  if (preferredRules.empty())
    return false;

  // Loop over the rule stack from highest to lowest rule level. This way we properly handle the higher rule
  // if it contains a lower one that is also a preferred rule.
  for (size_t i = 0; i < ruleStack.size(); ++ i)
    if (std::any_of(preferredRules.begin(), preferredRules.end(), [&ruleStack, i](size_t preferredRule) {
      return ruleStack[i] == preferredRule;
    })) {
      _candidates.rules.insert({ ruleStack[i], std::vector<size_t>(ruleStack.begin(), ruleStack.begin() + i) });
      if (showDebugOutput)
        std::cout << "=====> collected: " << _ruleNames[i] << std::endl;

      return true;
    }

  return false;
}

void CodeCompletionCore::printRuleState(std::vector<size_t> const& stack) const {
  if (stack.empty()) {
    std::cout << "<empty stack>" << std::endl;
    return;
  }

  for (size_t rule : stack)
    std::cout << _ruleNames[rule] << std::endl;
}

/**
 * This method follows the given transition and collects all symbols within the same rule that directly follow it
 * without intermediate transitions to other rules and only if there is a single symbol for a transition.
 */
std::vector<size_t> CodeCompletionCore::getFollowingTokens(Transition *transition) const {
  std::vector<size_t> result;

  std::vector<ATNState *> seen;
  std::vector<ATNState *> pipeline { transition->target };

  while (!pipeline.empty()) {
    ATNState *state = pipeline.back();
    pipeline.pop_back();

    for (Transition *transition : state->transitions) {
      if (transition->getSerializationType() == Transition::ATOM) {
        if (!transition->isEpsilon()) {
          std::vector<ssize_t> list = transition->label().toList();
          if (list.size() == 1 && ignoredTokens.count(list[0]) == 0) {
            result.push_back(list[0]);
            pipeline.push_back(transition->target);
          }
        } else {
          pipeline.push_back(transition->target);
        }
      }
    }
  }

  return result;
}

/**
 * Collects possible tokens which could be matched following the given ATN state. This is essentially the same
 * algorithm as used in the LL1Analyzer class, but here we consider predicates also and use no parser rule context.
 */
void CodeCompletionCore::collectFollowSets(ATNState *s, ATNState *stopState, std::vector<FollowSetWithPath> &followSets,
  std::unordered_set<ATNState *> &seen, std::vector<size_t> &ruleStack) const {

  if (seen.count(s) > 0)
    return;

  seen.insert(s);

  if (s == stopState) {
    followSets.push_back({ IntervalSet::of(Token::EPSILON), {}, ruleStack });
    return;
  }

  if (s->getStateType() == ATNState::RULE_STOP) {
    followSets.push_back({ IntervalSet::of(Token::EPSILON), {}, ruleStack });
    return;
  }

  for (Transition *transition : s->transitions) {
    if (transition->getSerializationType() == Transition::RULE) {
      RuleTransition *ruleTransition = static_cast<RuleTransition*>(transition);
      if (std::find(ruleStack.rbegin(), ruleStack.rend(), ruleTransition->target->ruleIndex) != ruleStack.rend()) {
        continue;
      }

      ruleStack.push_back(ruleTransition->target->ruleIndex);
      collectFollowSets(transition->target, stopState, followSets, seen, ruleStack);
      ruleStack.pop_back();

    } else if (transition->getSerializationType() == Transition::PREDICATE) {
      if (checkPredicate(static_cast<PredicateTransition*>(transition)))
        collectFollowSets(transition->target, stopState, followSets, seen, ruleStack);
    } else if (transition->isEpsilon()) {
      collectFollowSets(transition->target, stopState, followSets, seen, ruleStack);
    } else if (transition->getSerializationType() == Transition::WILDCARD) {
      followSets.push_back({ IntervalSet::of(Token::MIN_USER_TOKEN_TYPE, (ssize_t)_atn.maxTokenType), ruleStack });
    } else {
      misc::IntervalSet set = transition->label();
      if (!set.isEmpty()) {
        if (transition->getSerializationType() == Transition::NOT_SET) {
          set = set.complement(misc::IntervalSet::of(Token::MIN_USER_TOKEN_TYPE, (ssize_t)_atn.maxTokenType));
        }
        followSets.push_back({ set, getFollowingTokens(transition), ruleStack });
      }
    }
  }
}

/**
 * Entry point for the recursive follow set collection function.
 */
std::vector<CodeCompletionCore::FollowSetWithPath> CodeCompletionCore::determineFollowSets(ATNState *start, ATNState *stop) const {
  std::vector<FollowSetWithPath> result;

  std::unordered_set<ATNState *> seen;
  std::vector<size_t> ruleStack;
  collectFollowSets(start, stop, result, seen, ruleStack);

  return result;
}

/**
 * Walks the ATN for a single rule only. It returns the states that continue the walk in the calling rule.
 * The result can be empty in case we hit only non-epsilon transitions that didn't match the current input or if we
 * hit the caret position.
 */
CodeCompletionCore::RuleEndStatus CodeCompletionCore::processRule(ATNState *startState, size_t tokenIndex,
  std::vector<size_t> &callStack, std::string indentation) {

  // Start with rule specific handling before going into the ATN walk.

  // For rule start states we determine and cache the follow set, which gives us 3 advantages:
  // 1) We can quickly check if a symbol would be matched when we follow that rule. We can so check in advance
  //    and can save us all the intermediate steps if there is no match.
  // 2) We'll have all symbols that are collectable already together when we are at the caret when entering a rule.
  // 3) We get this lookup for free with any 2nd or further visit of the same rule, which often happens
  //    in non trivial grammars, especially with (recursive) expressions and of course when invoking code completion
  //    multiple times.
  FollowSetsPerState &setsPerState = _followSetsByATN[typeid(_parser)];
  if (setsPerState.count(startState->stateNumber) == 0) {
    ATNState *stop = _atn.ruleToStopState[startState->ruleIndex];
    setsPerState[startState->stateNumber].sets = determineFollowSets(startState, stop);

    // Sets are split by path to allow translating them to preferred rules. But for quick hit tests
    // it is also useful to have a set with all symbols combined.
    IntervalSet combined;
    for (auto &set : setsPerState[startState->stateNumber].sets)
      combined.addAll(set.sets);
    setsPerState[startState->stateNumber].combined = combined;
  }

  FollowSetsHolder &followSets = setsPerState[startState->stateNumber];
  callStack.push_back(startState->ruleIndex);
  size_t currentSymbol = _tokens[tokenIndex]->getType();

  if (tokenIndex >= _caretTokenIndex) { // At caret?
    if (preferredRules.count(startState->ruleIndex) > 0) {
      // No need to go deeper when collecting entries and we reach a rule that we want to collect anyway.
      translateToRuleIndex(callStack);
    } else {
      // Convert all follow sets to either single symbols or their associated preferred rule and add
      // the result to our candidates list.
      for (auto &set : followSets.sets) {
        std::vector<size_t> fullPath = callStack;
        fullPath.insert(fullPath.end(), set.path.begin(), set.path.end());
        if (!translateToRuleIndex(fullPath)) {
          for (ssize_t symbol : set.sets.toList())
            if (ignoredTokens.count(symbol) == 0) {
              if (showDebugOutput)
                std::cout << "=====> collected: " << _vocabulary.getDisplayName(symbol) << std::endl;
              if (_candidates.tokens.count(symbol) == 0)
                _candidates.tokens[symbol] = set.following; // Following is empty if there is more than one entry in the set.
              else {
                // More than one following list for the same symbol.
                if (_candidates.tokens[symbol] != set.following)
                  _candidates.tokens[symbol] = {};
              }
            }
        }
      }
    }

    callStack.pop_back();
    return {};

  } else {
    // Process the rule if we either could pass it without consuming anything (epsilon transition)
    // or if the current input symbol will be matched somewhere after this entry point.
    if (!followSets.combined.contains(Token::EPSILON) && !followSets.combined.contains(currentSymbol)) {
      callStack.pop_back();
      return {};
    }
  }

  RuleEndStatus result;
  bool isLeftRecursive = ((RuleStartState *)startState)->isLeftRecursiveRule;
  bool forceLoopEnd = false;

  // The current state execution pipeline contains all yet-to-be-processed ATN states in this rule.
  // For each such state we store the token index + a list of rules that lead to it.
  std::vector<PipelineEntry> statePipeline;
  PipelineEntry currentEntry;

  // Bootstrap the pipeline.
  statePipeline.push_back({ startState, tokenIndex });

  while (!statePipeline.empty()) {
    currentEntry = statePipeline.back();
    statePipeline.pop_back();

    ++_statesProcessed;

    currentSymbol = _tokens[currentEntry.tokenIndex]->getType();

    bool atCaret = currentEntry.tokenIndex >= _caretTokenIndex;
    if (showDebugOutput)
    {
      printDescription(indentation, currentEntry.state, generateBaseDescription(currentEntry.state), currentEntry.tokenIndex);
      if (showRuleStack)
        printRuleState(callStack);
    }

    switch (currentEntry.state->getStateType()) {
      case ATNState::RULE_START: // Happens only for the first state in this rule, not subrules.
        indentation += "  ";
        break;

      // Found the end of this rule. Determine the following states and return to the caller.
      case ATNState::RULE_STOP:
      {
        // Multiple paths can lead to the stop state. We only need to add the same outgoing transition again
        // when we arrive with different token input positions.

        // Find the transitions that lead us back to the correct next state (which must correspond to the
        // top of the rule stack, after we removed the current rule from it).
        size_t returnIndex = callStack[callStack.size() - 2];
        for (auto it = currentEntry.state->transitions.rbegin(); it != currentEntry.state->transitions.rend(); ++it)
          if ((*it)->target->ruleIndex == returnIndex) {
            // Don't add more than once.
            bool canAdd = true;
            for (auto state : result) {
              if (state.state == (*it)->target && state.tokenIndex == currentEntry.tokenIndex) {
                canAdd = false;
                break;
              }
            }
            if (canAdd)
              result.push_back({ (*it)->target, currentEntry.tokenIndex });
          }
        continue;
      }

      case ATNState::STAR_LOOP_ENTRY:
        // In left recursive rules we can end up doing the same processing twice for each level of invocation, which
        // quickly sums up to an unbearable amount (doubling the steps on each invocation). We can avoid this by
        // not following the transition to the star block start state from the star block entry state (but instead
        // go directly to the loop end state) if we are in a left recursive rule and arrived here from ourselve.
        //
        // This is a similar approach like the stack unrolling you can see in the parser (see pushNewRecursionContext
        // and unrollRecursionContexts).
        if (forceLoopEnd) {
          for (Transition *transition : currentEntry.state->transitions) {
            // Find the loop end and only continue with that.
            if (transition->target->getStateType() == ATNState::LOOP_END) {
              statePipeline.push_back({ transition->target, currentEntry.tokenIndex });
              break;
            }
          }
          continue;
        }
        break;

      default:
        break;
    }

    for (auto iterator = currentEntry.state->transitions.rbegin(); iterator != currentEntry.state->transitions.rend(); ++iterator) {
      Transition *transition = *iterator;
      if (transition->getSerializationType() == Transition::RULE) {
        auto endStatus = processRule(transition->target, currentEntry.tokenIndex, callStack, indentation);
        for (auto &status : endStatus)
          statePipeline.push_back(status);

        // See description above for this flag.
        if (isLeftRecursive && transition->target->ruleIndex == callStack.back())
          forceLoopEnd = true;

      } else if (transition->getSerializationType() == Transition::PREDICATE) {
        if (checkPredicate(static_cast<PredicateTransition *>(transition)))
          statePipeline.push_back({ transition->target, currentEntry.tokenIndex });
      } else if (transition->isEpsilon()) {
        statePipeline.push_back({ transition->target, currentEntry.tokenIndex });
      } else if (transition->getSerializationType() == Transition::WILDCARD) {
        if (atCaret) {
          if (!translateToRuleIndex(callStack)) {
            for (ssize_t token : misc::IntervalSet::of(Token::MIN_USER_TOKEN_TYPE, (ssize_t)_atn.maxTokenType).toList())
              if (ignoredTokens.count(token) == 0)
                _candidates.tokens[token] = {};
          }
        } else {
          statePipeline.push_back({ transition->target, currentEntry.tokenIndex + 1 });
        }
      } else {
        misc::IntervalSet set = transition->label();
        if (!set.isEmpty()) {
          if (transition->getSerializationType() == Transition::NOT_SET) {
            set = set.complement(misc::IntervalSet::of(Token::MIN_USER_TOKEN_TYPE, (ssize_t)_atn.maxTokenType));
          }
          if (atCaret) {
            if (!translateToRuleIndex(callStack)) {
              std::vector<ssize_t> list = set.toList();
              bool addFollowing = list.size() == 1;
              for (ssize_t symbol : list)
                if (ignoredTokens.count(symbol) == 0) {
                  if (showDebugOutput)
                    std::cout << "=====> collected: " << _vocabulary.getDisplayName(symbol) << std::endl;
                  if (addFollowing)
                    _candidates.tokens[symbol] = getFollowingTokens(transition);
                  else
                    _candidates.tokens[symbol] = {};
                }
            }
          } else {
            if (set.contains(currentSymbol)) {
              if (showDebugOutput)
                std::cout << "=====> consumed: " << _vocabulary.getDisplayName(currentSymbol) << std::endl;
              statePipeline.push_back({ transition->target, currentEntry.tokenIndex + 1 });
            }
          }
        }
      }
    }
  }

  callStack.pop_back();
  return result;
}

void CodeCompletionCore::process(size_t startRule) {
  _statesProcessed = 0;

  // Determine the first token in the input which ends after the given caret position. This is were we stop walking
  // and start collecting candidates.
  _caretTokenIndex = 0;
  while (_caretTokenIndex < _tokens.size()) {
    size_t start = _tokens[_caretTokenIndex]->getCharPositionInLine();
    size_t stop = start + _tokens[_caretTokenIndex]->getStopIndex() - _tokens[_caretTokenIndex]->getStartIndex();

    // Certain tokens (like identifiers) must be treated as if the char directly following them still belongs to that token
    // (e.g. a whitespace after a name), because visually the caret is placed between that token and the whitespace creating
    // the impression we are still at the identifier (and we should show candidates for this identifier position).
    // Other tokens (like operators) however don't include that position, hence the token range is one less for them.
    if (noSeparatorRequiredFor.count(_tokens[_caretTokenIndex]->getType()) == 0)
      ++stop;
    if ((_tokens[_caretTokenIndex]->getType() == Token::EOF) || (_tokens[_caretTokenIndex]->getLine() > _caret.second)
        || (_tokens[_caretTokenIndex]->getLine() == _caret.second && stop >= _caret.first))
      break;

    ++_caretTokenIndex;
  }

  std::vector<size_t> callStack;
  processRule(_atn.ruleToStartState[startRule], 0, callStack, "");

  if (showResult)
    std::cout << "States processed: " << _statesProcessed << std::endl;
}

std::string CodeCompletionCore::generateBaseDescription(ATNState *state) const {
  std::string stateValue = state->stateNumber == ATNState::INVALID_STATE_NUMBER ? "Invalid" : std::to_string(state->stateNumber);
  return "[" + stateValue + " " + ATNState::serializationNames[state->getStateType()] + "] in " + _ruleNames[state->ruleIndex];
}

void CodeCompletionCore::printDescription(std::string const& currentIndent, ATNState *state,
  std::string const& baseDescription, size_t tokenIndex) const {

  std::cout << currentIndent;

  std::string transitionDescription;
  if (debugOutputWithTransitions)
  {
    for (auto transition : state->transitions)
    {
      std::string labels;
      auto symbols = transition->label().toList();
      if (symbols.size() > 2)
      {
        // Only print start and end symbols to avoid large lists in debug output.
        labels = _vocabulary.getDisplayName(symbols.front()) + " .. " + _vocabulary.getDisplayName(symbols.back());
      }
      else
      {
        for (auto symbol : symbols)
        {
          if (!labels.empty())
            labels += ", ";
          labels += _vocabulary.getDisplayName(symbol);
        }
      }
      if (labels.empty())
        labels = "ε";
      transitionDescription += "\n" + currentIndent + "\t(" + labels + ") -> " + "[" + std::to_string(transition->target->stateNumber) + " " +
      ATNState::serializationNames[transition->target->getStateType()] + "] in " + _ruleNames[transition->target->ruleIndex];
    }
  }

  if (tokenIndex >= _caretTokenIndex)
    std::cout << "<<" << tokenIndex << ">> ";
  else
    std::cout << "<" << tokenIndex << "> ";
  std::cout << "Current state: " << baseDescription << transitionDescription << std::endl;
}
