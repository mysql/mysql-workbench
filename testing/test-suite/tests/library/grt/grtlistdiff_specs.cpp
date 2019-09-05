/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "diff/grtdiff.h"
#include "grt.h"
#include "diff/diffchange.h"
#include "diff/changeobjects.h"
#include "diff/changelistobjects.h"
#include "grtdb/diff_dbobjectmatch.h"

#include "casmine.h"
#include "wb_test_helpers.h"
#include "grt_test_helpers.h"

using namespace grt;

namespace {

$ModuleEnvironment() {};

template <typename TValueIter, typename TContainer>
void list_from_container(TValueIter It, TValueIter ItEnd, TContainer& target) {
  for (; It != ItEnd; It++)
    target.insert(*It);
}

void apply_change(BaseListRef source, const grt::ListItemAddedChange* change) {
  size_t index = change->get_prev_item().is_valid() ? source.get_index(change->get_prev_item()) + 1 : 0;
  source.ginsert(change->get_value(), index);
}

void apply_change(BaseListRef source, const grt::ListItemRemovedChange* change) {
  source.gremove_value(change->get_value());
}

void apply_change(BaseListRef source, const grt::ListItemModifiedChange* change) {
}

void apply_change(BaseListRef source, const grt::ListItemOrderChange* change) {
  source.gremove_value(change->get_old_value());
  size_t index = change->get_prev_item().is_valid() ? source.get_index(change->get_prev_item()) + 1 : 0;
  source.ginsert(change->get_new_value(), index);
}

void apply_change(BaseListRef source, const grt::MultiChange* change) {
  const grt::ChangeSet* change_list = change->subchanges();
  for (grt::ChangeSet::const_iterator e2 = change_list->end(), jt = change_list->begin(); jt != e2; jt++) {
    const grt::DiffChange* subchange = jt->get();
    //    print_array(source);
    switch (subchange->get_change_type()) {
      case grt::ListItemAdded:
        apply_change(source, static_cast<const grt::ListItemAddedChange*>(subchange));
        break;
      case grt::ListItemRemoved:
        apply_change(source, static_cast<const grt::ListItemRemovedChange*>(subchange));
        break;
      case grt::ListItemModified:
        apply_change(source, static_cast<const grt::ListItemModifiedChange*>(subchange));
        break;
      case grt::ListItemOrderChanged:
        apply_change(source, static_cast<const grt::ListItemOrderChange*>(subchange));
        break;
      default:
        break;
    }
  }
  //  print_array(source);
}

template <typename TValue>
void resolve_change_type(TValue source, DiffChange* change) {
  switch (change->get_change_type()) {
    // case SimpleValue:
    case grt::ValueAdded:
      //    dynamic_cast<grt::ValueAddedChange*>(change);
      break;
    case grt::DictItemAdded:
      break;
    case grt::ListItemAdded:
      //    dynamic_cast<grt::ListItemAddedChange*>(change);
      break;

    case grt::ValueRemoved:
    case grt::ListItemRemoved:
    case grt::DictItemRemoved:
    case grt::ObjectModified:
    case grt::ObjectAttrModified:
      break;
    case grt::ListModified:
      apply_change(source, dynamic_cast<grt::MultiChange*>(change));
    case grt::ListItemModified:
    case grt::ListItemOrderChanged:
    case grt::DictModified:
    case grt::DictItemModified:
      break;

    default:
      break;
  }
}

void apply_change_to_object(ValueRef source, DiffChange* change) {
  if (!change) // No changes detected
    return;
  if (BaseListRef::can_wrap(source))
    resolve_change_type(BaseListRef::cast_from(source), change);
}
std::vector<std::vector<int> > test_src;
std::vector<std::vector<int> > test_dst;

template <typename TTestData>
void test_diff(TTestData src, TTestData dest) {
  IntegerListRef source(grt::Initialized);
  IntegerListRef target(grt::Initialized);
  list_from_container(src->begin(), src->end(), source);
  list_from_container(dest->begin(), dest->end(), target);

  default_omf omf;
  grt::NormalizedComparer normalizer;
  normalizer.init_omf(&omf);
  std::shared_ptr<DiffChange> change = diff_make(source, target, &omf);
  apply_change_to_object(source, change.get());
  casmine::deepCompareGrtValues("test_diff fail", ValueRef(source), ValueRef(target));
}

$describe("GRT list diff") {
  $it("Int values test", []() {
    { // No changes
      const int s[] = {0, 1, 2, 3, 4, 5};
      const int t[] = {0, 1, 2, 3, 4, 5};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    { // Add/remove
      const int s[] = {0, 1, 2};
      const int t[] = {0, 1};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    { // Move
      const int s[] = {0, 1, 2, 3, 4, 5};
      const int t[] = {0, 1, 5, 3, 4, 2};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    { // Move + Add/Remove
      const int s[] = {0, 1, 2, 3, 4, 5, 6};
      const int t[] = {0, 1, 5, 3, 4, 2};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    { // Replace all
      const int s[] = {0, 1, 2, 3, 4, 5};
      const int t[] = {6, 7, 8, 9, 10, 11};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    { // Move to first/last pos
      const int s[] = {1, 2, 3, 4, 5, 0};
      const int t[] = {0, 1, 2, 3, 4, 5};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    {
      const int s[] = {0, 1, 2, 3, 4, 5};
      const int t[] = {5, 4, 3, 2, 1, 0};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    {
      const int s[] = {10, 11, 0, 1, 2, 3};
      const int t[] = {2, 5, 1, 6, 3};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

    {
      const int s[] = {0, 1, 2, 3};
      const int t[] = {5, 1, 6, 3, 2};
      test_src.push_back(std::vector<int>(s, s + UPPER_BOUND(s)));
      test_dst.push_back(std::vector<int>(t, t + UPPER_BOUND(t)));
    }

  #if 0 // Use to run only one data set
    size_t test_idx = 1;
    test_diff(&test_src[test_idx],&test_dst[test_idx],test_grt);
    test_diff(&test_dst[test_idx],&test_src[test_idx],test_grt);
    return;
  #endif

    std::vector<std::vector<int> >::const_iterator It2 = test_dst.begin();
    for (std::vector<std::vector<int> >::const_iterator It1 = test_src.begin();
        It1 != test_src.end() && It2 != test_dst.end(); ++It1, ++It2) {
      test_diff(It1, It2);
      test_diff(It2, It1);
    }
  });

  $it("Double values test", []() {
    const double s[] = {.0, 1., .2, .3};
    const double t[] = {5., .1, .6, .3, .2};

    DoubleListRef source(grt::Initialized);
    DoubleListRef target(grt::Initialized);

    list_from_container(s, s + UPPER_BOUND(s), source);
    list_from_container(t, t + UPPER_BOUND(t), target);

    default_omf omf;
    grt::NormalizedComparer normalizer;
    normalizer.init_omf(&omf);
    std::shared_ptr<DiffChange> change = diff_make(source, target, &omf);
    apply_change_to_object(source, change.get());

    casmine::deepCompareGrtValues("Differnet grt values", ValueRef(source), ValueRef(target));
  });

}
}
