/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <memory>
#include "grtlistdiff.h"
#include "grt.h"
#include "changefactory.h"
#include "changelistobjects.h"
#include "grtdiff.h"
#include "base/log.h"

#include <memory>
#include <algorithm>

namespace grt {
  // typedef ListDifference<ValueRef, internal::List::raw_iterator, internal::List::raw_iterator> GrtListDifference;

  // TODO: check if this is till used
  bool pless_struct::operator()(const ValueRef &_Left, const ValueRef &_Right) const { // apply operator< to operands
    return _Left < _Right;
  }

  struct omf_eq {
    const Omf *omf;
    omf_eq(const Omf *omf) : omf(omf){};
    bool operator()(const ValueRef &l, const ValueRef &r) {
      return omf->equal(l, r);
    }
  };

  struct omf_lt {
    const Omf *omf;
    omf_lt(const Omf *omf) : omf(omf){};

    bool operator()(const ValueRef &l, const ValueRef &r) {
      return omf->less(l, r);
    }
  };

  struct OmfEqPred : public std::function<bool (ValueRef, ValueRef)> { // functor for operator<
    const Omf *_omf;
    OmfEqPred(const Omf *omf) : _omf(omf) {
    }
    bool operator()(const ValueRef &_Left, const ValueRef &_Right) const {
      return _omf->equal(_Left, _Right);
    }
  };

  /**
   * Find Longest Increasing Subsequence (LIS)
   *
   * This is used to find sequence elements that needs to be moved
   * in order to sort sequence with minimal amount of elment moves
   * LIS represents maximum amount of elements that are already on
   * in the correct positions (one relative to each other) rest of
   * elements needs to be moved
   *
   * Note that elements in output are in reversed ored. You may use std::reverse(res.begin(), res.end());
   * if you really need to reverse them othervise using .rbegin() and .rend() should be enough
   *
   * @param src[in] source sequence all elements must be unique
   * @param res[out] container to be filled with elements belonging to LIS must have push_back and resize
   */
  template <typename InputContainerType, typename OutputContainerType>
  void reversed_LIS(const InputContainerType &src, OutputContainerType &res) {
    typedef typename InputContainerType::value_type value_type;
    typedef std::map<value_type, size_t> TTailsMap;
    std::vector<size_t> sequence_history(src.size(), std::string::npos); // traces back LIS including each element
    // longest_sequence_tails.size() is LIS size, last element represents last
    TTailsMap longest_sequence_tails; // holds <element, index> that may become LIS endings, but not sequnce itself
    for (size_t i = 0; i < src.size(); ++i) {
      // It_added_value will hold iterator for newly added value
      typename TTailsMap::iterator It_added_value =
        longest_sequence_tails.insert(typename TTailsMap::value_type(src[i], i)).first;
      if (It_added_value == longest_sequence_tails.begin())
        sequence_history[i] = std::string::npos; // minmal element starts new sequence, mark it
      else {
        // yet another sequence member so, mark previous(by value not by index!) item as part of sequence
        sequence_history[i] = (--It_added_value)->second;
        ++It_added_value;
      }
      // current element is less than next one, but has the same sized LIS
      // thus it may be included in any LIS instead next one and possibly
      // some other LISes so it replaces bigger one, or in case if new
      // element has maximum value it becomes "king of the hill" representing
      // end of longest sequence that was found so far
      if (++It_added_value != longest_sequence_tails.end())
        longest_sequence_tails.erase(It_added_value);
    }

    if (longest_sequence_tails.empty()) // empty input
      return;
    // this is the last element of LIS
    typename TTailsMap::iterator It = longest_sequence_tails.end();
    --It;
    size_t j = It->second;
    res.reserve(longest_sequence_tails.size()); // avoid unecessary reallocations
    // trace back LIS thru history array
    do {
      res.push_back(src[j]);
    } while ((j = sequence_history[j]) != std::string::npos);
    // history where iterated in backward direction reverse it
    // no need toreverse it here, using reverse iteratros would be much faster
    //  std::reverse(res.begin(), res.end());
  }

  bool diffPred(const std::shared_ptr<ListItemChange> &a, const std::shared_ptr<ListItemChange> &b) {
    if (a->get_change_type() == grt::ListItemRemoved)
      if (b->get_change_type() == grt::ListItemRemoved)
        return a->get_index() > b->get_index(); // removals should be processed right to left to keep still existing
                                                // items at the same indexes
      else
        return false;
    else if (b->get_change_type() == grt::ListItemRemoved)
      return true;
    else
      return a->get_index() < b->get_index();
  }

  std::shared_ptr<MultiChange> GrtListDiff::diff(const BaseListRef &source, const BaseListRef &target, const Omf *omf) {
    typedef std::vector<size_t> TIndexContainer;
    default_omf def_omf;
    std::vector<std::shared_ptr<ListItemChange> > changes;
    const Omf *comparer = omf ? omf : &def_omf;
    ValueRef prev_value;
    // This is indexes of source's elements that exist in both target and source
    // in order of element appearance in target
    // We need to swap indexes(and eventually elements) so that source's elements order
    // will become the same as target's
    TIndexContainer source_indexes;  // new indexes for already existing elements
    TIndexContainer ordered_indexes; // ordered indexes list for set_difference
    for (size_t target_idx = 0; target_idx < target.count();
         ++target_idx) { // look for something that exists in target but not in source, it should be added
      const ValueRef v = target.get(target_idx);
      internal::List::raw_const_iterator It_Dup = find_if(
        target.content().raw_begin(), target.content().raw_begin() + target_idx, std::bind(OmfEqPred(comparer), 
          std::placeholders::_1, v));
      if (It_Dup != target.content().raw_begin() + target_idx)
        continue;
      internal::List::raw_const_iterator It =
        find_if(source.content().raw_begin(), source.content().raw_end(), std::bind(OmfEqPred(comparer), 
          std::placeholders::_1,v));
      if (It == source.content().raw_end())
        changes.push_back(std::shared_ptr<ListItemChange>(new ListItemAddedChange(v, prev_value, target_idx)));
      else // item exists in both target and source, save indexes
        source_indexes.push_back(source.get_index(*It));
      prev_value = v;
    };

    for (size_t source_idx = 0; source_idx < source.count();
         ++source_idx) { // look for something that exists in source but not in target, it should be removed
      const ValueRef v = source.get(source_idx);

      // This shouldn't happend actually, since lists are expected to be unique
      // But in case of caseless compare we may have non-unique lists
      // so just skip it
      internal::List::raw_const_iterator It_Dup = find_if(
        source.content().raw_begin(), source.content().raw_begin() + source_idx, std::bind(OmfEqPred(comparer),
          std::placeholders::_1, v));
      if (It_Dup != source.content().raw_begin() + source_idx)
        continue;

      internal::List::raw_const_iterator It =
        find_if(target.content().raw_begin(), target.content().raw_end(), std::bind(OmfEqPred(comparer), 
          std::placeholders::_1, v));
      if (It == target.content().raw_end()) {
#ifdef DEBUG_DIFF
        logInfo("Removing %s from list\n", grt::ObjectRef::cast_from(v)->get_string_member("name").c_str());
        if (grt::ObjectRef::cast_from(v)->get_string_member("name") == "fk_tblClientApp_base_tblClient_base1_idx")
          dump_value(target);
#endif
        changes.push_back(std::shared_ptr<ListItemChange>(new ListItemRemovedChange(v, source_idx)));
      } else
        ordered_indexes.push_back(source_idx);
    };

    //  return changes.empty()? NULL : new MultiChange(ListModified, changes);// No ListItemOrderChange

    TIndexContainer stable_elements;
    reversed_LIS(source_indexes, stable_elements);
    TIndexContainer moved_elements(source_indexes.size() - stable_elements.size());
    std::set_difference(ordered_indexes.begin(), ordered_indexes.end(), stable_elements.rbegin(),
                        stable_elements.rend(), moved_elements.begin());
    for (TIndexContainer::iterator It = moved_elements.begin(); It != moved_elements.end(); ++It) {
      internal::List::raw_const_iterator It_target = find_if(target.content().raw_begin(), target.content().raw_end(),
                std::bind(OmfEqPred(comparer), std::placeholders::_1, source.get(*It)));
      prev_value = It_target == target.content().raw_begin() ? ValueRef() : *(It_target - 1);
      std::shared_ptr<ListItemOrderChange> orderchange(
        new ListItemOrderChange(source.get(*It), *It_target, omf, prev_value, target.get_index(*It_target)));
      //    if (!orderchange->subchanges()->empty())
      changes.push_back(orderchange);
    }

    for (TIndexContainer::iterator It = stable_elements.begin(); It != stable_elements.end(); ++It) {
      internal::List::raw_const_iterator It_target = find_if(target.content().raw_begin(), target.content().raw_end(),
                std::bind(OmfEqPred(comparer), std::placeholders::_1, source.get(*It)));
      if (It_target != target.content().raw_end()) {
        std::shared_ptr<ListItemChange> change =
          create_item_modified_change(source.get(*It), *It_target, omf, target.get_index(*It_target));
        if (change)
          changes.push_back(change);
      }
    }
    ChangeSet retval;
    std::sort(changes.begin(), changes.end(), diffPred);
    for (std::vector<std::shared_ptr<ListItemChange> >::const_iterator It = changes.begin(); It != changes.end(); ++It)
      retval.append(*It);
    return retval.empty() ? std::shared_ptr<MultiChange>()
                          : std::shared_ptr<MultiChange>(new MultiChange(ListModified, retval));
  }

  ////////////////////////////////////////////////////////////////////////////
  std::shared_ptr<ListItemModifiedChange> create_item_modified_change(const ValueRef &source, const ValueRef &target,
                                                                      const Omf *omf, const size_t index) {
    std::shared_ptr<DiffChange> subchange = GrtDiff(omf).diff(source, target, omf);
    if (!subchange)
      return std::shared_ptr<ListItemModifiedChange>();
    //    diff_make(source, target, omf, sqlDefinitionCmp);
    return std::shared_ptr<ListItemModifiedChange>(new ListItemModifiedChange(source, target, subchange, index));
  }
}
