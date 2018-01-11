/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

//! This is a generic algorithm for incrementally building any kind of list
//! with the least ammount of changes.
//! It was written primarily for updating menus and toolbars without having
//! to recreate them from scratch, but can also be used for other similar tasks.
//! The algorithm works in 2 stages, one to find out the differences and the
//! other to apply the differences.

//! Pre-conditions:
//! - no duplicate items must be possible
//! - there must be a 1:1 mapping between items from the 2 lists

#ifndef _INCREMENTAL_LIST_UPDATER_H_
#define _INCREMENTAL_LIST_UPDATER_H_

namespace bec {

  template <class DestIterator, class DestRef, class SourceIterator>
  class IncrementalListUpdater {
  public:
    typedef DestIterator dest_iterator;
    typedef DestRef dest_ref;
    typedef SourceIterator source_iterator;

    virtual ~IncrementalListUpdater() {
    }

    virtual dest_iterator get_dest_iterator() = 0;
    virtual source_iterator get_source_iterator() = 0;

    virtual dest_iterator increment_dest(dest_iterator &iter) = 0;
    virtual source_iterator increment_source(source_iterator &iter) = 0;

    virtual bool has_more_dest(dest_iterator iter) = 0;
    virtual bool has_more_source(source_iterator iter) = 0;

    virtual bool items_match(dest_iterator diter, source_iterator siter) = 0;

    virtual dest_ref get_dest(dest_iterator iter) = 0;

    virtual void update(dest_ref dest_item, source_iterator source_item) = 0;

    // begin adding items to the begginning of the dest list
    virtual dest_iterator begin_adding() = 0;
    virtual dest_iterator add(dest_iterator &iter, source_iterator source_item) = 0;
    virtual dest_iterator add(dest_iterator &iter, dest_ref item) = 0;
    // end adding items to the dest item, stuff after the last item added must be removed
    virtual void end_adding(dest_iterator iter) = 0;

    virtual void execute() {
      // find location of items in the source list in the dest list
      for (source_iterator src_item = get_source_iterator(); has_more_source(src_item); increment_source(src_item)) {
        //        bool found= false;

        for (dest_iterator item = get_dest_iterator(); has_more_dest(item); increment_dest(item)) {
          if (items_match(item, src_item)) {
            source_mapping[src_item] = get_dest(item);
            //            found= true;
            break;
          }
        }
      }

      dest_iterator iter = begin_adding();
      // go through source items and reorder or insert new items
      for (source_iterator src_item = get_source_iterator(); has_more_source(src_item); increment_source(src_item)) {
        if (dest_exists(src_item)) {
          dest_ref dest = get_dest_for_source(src_item);
          update(dest, src_item);
          iter = add(iter, dest);
        } else {
          iter = add(iter, src_item);
        }
      }
      end_adding(iter);
    }

  protected:
    std::map<source_iterator, dest_ref> source_mapping;

    bool dest_exists(source_iterator item) {
      return source_mapping.find(item) != source_mapping.end();
    }

    dest_ref get_dest_for_source(source_iterator item) {
      return source_mapping[item];
    }
  };
};

#endif
