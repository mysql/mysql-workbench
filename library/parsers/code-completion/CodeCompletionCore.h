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

#include <unordered_set>
#include <string>
#include <vector>
#include <typeindex>

namespace antlr4 {
  class Parser;

  namespace dfa {
    class Vocabulary;
  }

  namespace atn {
    class ATN;
  }

  using TokenList = std::vector<size_t>;
  using RuleList = std::vector<size_t>;

  // A pair containing one of the preferred rule indexes and the rule index path where it has been collected.
  using RulePathPair = std::pair<size_t, TokenList>;

  // All the candidates which have been found. Tokens and rules are separated (both use a numeric value).
  struct CandidatesCollection { std::map<size_t, TokenList> tokens; std::set<RulePathPair> rules; };

  // The main class for doing the collection process.
  class CodeCompletionCore
  {
  public:
    // Debugging options. Print human readable ATN state and other info.
    bool showResult = false;                // Not dependent on showDebugOutput. Prints the collected rules + tokens to terminal.
    bool showDebugOutput = false;           // Enables printing ATN state info to terminal.
    bool debugOutputWithTransitions = true; // Only relevant when showDebugOutput is true. Enables transition printing for a state.
    bool showRuleStack = false;             // Also depends on showDebugOutput. Enables call stack printing for each rule recursion.

    // Tailoring of the result.
    std::unordered_set<size_t> ignoredTokens;  // Tokens which should not appear in the candidates set.
    std::unordered_set<size_t> preferredRules; // Rules which replace any candidate token they contain.
                                               // This allows to return descriptive rules (e.g. className, instead of ID/identifier).
    std::unordered_set<size_t> noSeparatorRequiredFor; // A list of tokens which don't need a separator (e.g. a whitespace) to other tokens.

    CodeCompletionCore(antlr4::Parser *parser);

    CandidatesCollection collectCandidates(std::pair<std::size_t, std::size_t> caret, std::size_t startRule = 0);

  private:
    antlr4::Parser *_parser;
    antlr4::atn::ATN const& _atn;

    antlr4::dfa::Vocabulary const& _vocabulary;
    std::vector<std::string> const& _ruleNames;

    std::vector<antlr4::Token *> _tokens;

    std::pair<size_t, size_t> _caret; // { column, row }
    size_t _caretTokenIndex; // Derived from the caret position.

    size_t _statesProcessed;
    CandidatesCollection _candidates; // The collected candidates (rules and tokens).

    // A record for a follow set along with the path at which this set was found.
    // If there is only a single symbol in the interval set then we also collect and store tokens which follow
    // this symbol directly in its rule (i.e. there is no intermediate rule transition). Only single label transitions
    // are considered. This is useful if you have a chain of tokens which can be suggested as a whole, because there is
    // a fixed sequence in the grammar.
    using FollowSetWithPath = struct { antlr4::misc::IntervalSet sets; TokenList following; RuleList path; };

    // A list of follow sets (for a given state number) + all of them combined for quick hit tests.
    // This data is static in nature (because the used ATN states are part of a static struct: the ATN).
    // Hence it can be shared between all C3 instances, however it dependes on the actual parser class (type).
    using FollowSetsHolder = struct { std::vector<FollowSetWithPath> sets; antlr4::misc::IntervalSet combined; };
    using FollowSetsPerState = std::unordered_map<size_t, FollowSetsHolder>;
    static std::unordered_map<std::type_index, FollowSetsPerState> _followSetsByATN;

    struct PipelineEntry {
      antlr4::atn::ATNState *state;
      size_t tokenIndex;
    };

    // ATN + input stream position info after a rule was processed.
    using RuleEndStatus = std::vector<PipelineEntry>;

    bool checkPredicate(antlr4::atn::PredicateTransition *transition) const;
    bool translateToRuleIndex(std::vector<size_t> const& ruleStack);
    void printRuleState(std::vector<size_t> const& stack) const;

    TokenList getFollowingTokens(antlr4::atn::Transition *transition) const;
    std::vector<FollowSetWithPath> determineFollowSets(antlr4::atn::ATNState *start, antlr4::atn::ATNState *stop) const;
    void collectFollowSets(antlr4::atn::ATNState *s, antlr4::atn::ATNState *stopState,
                           std::vector<FollowSetWithPath> &followSets, std::unordered_set<antlr4::atn::ATNState *> &seen,
                           std::vector<size_t> &ruleStack) const;

    RuleEndStatus processRule(antlr4::atn::ATNState *startState, size_t tokenIndex, std::vector<size_t> &callStack,
                              std::string indentation);
    void process(size_t startRule);

    std::string generateBaseDescription(antlr4::atn::ATNState *state) const;
    void printDescription(std::string const& currentIndent, antlr4::atn::ATNState *state, std::string
                          const& baseDescription, size_t tokenIndex) const;
  };
  
} // namespace antlr4
