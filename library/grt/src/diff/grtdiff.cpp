/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <assert.h>
#include <algorithm>

#include "base/util_functions.h"
#include "base/log.h"

#include "grtdiff.h"
#include "diffchange.h"
#include "changefactory.h"
#include "grtlistdiff.h"
#include "grtpp_util.h"

#include "grts/structs.h"

// DEFAULT_LOG_DOMAIN("Diff module") currently unused

namespace grt {

  time_t timestamp() {
#if defined(_MSC_VER)
    return GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif
  }

  //#define LOG_DIFF_TIME
  std::shared_ptr<DiffChange> diff_make(const ValueRef &source, const ValueRef &target, const Omf *omf,
                                        bool dont_clone_values) {
#ifdef LOG_DIFF_TIME
    time_t start = timestamp();
#endif
    std::shared_ptr<DiffChange> result = GrtDiff(omf, dont_clone_values).diff(source, target, omf);
#ifdef LOG_DIFF_TIME
    logDebug2("Diff took %li ticks\n", timestamp() - start);
    printf("Diff took %li ticks\n", timestamp() - start);
#endif
    return result;
  }

  bool is_any(const ValueRef &v) {
    return !v.is_valid() || v.type() == AnyType;
  }

  inline bool XOR(bool a, bool b) {
    return a ^ b;
  }

  bool are_compatible(const ValueRef &source, const ValueRef &target, Type *cmptype) {
    Type st = source.type();
    Type tt = target.type();

    if (cmptype)
      *cmptype = (st == tt || tt == AnyType) ? st : tt;

    return ((st == tt) && !is_any(source)) || XOR(is_any(source), is_any(target));
  }

  bool are_compatible_lists(const BaseListRef &source, const BaseListRef &target, Type *cmptype) {
    Type stl = is_any(source) ? AnyType : source.content_type();
    Type ttl = is_any(target) ? AnyType : target.content_type();

    Type type = (stl == ttl || ttl == AnyType) ? stl : ttl;
    if (cmptype)
      *cmptype = type;

    return ((stl == ttl) && !is_any(source)) ||
           (XOR(is_any(source), is_any(target)) && (is_simple_type(type) || type == ObjectType));
  }

  std::shared_ptr<DiffChange> GrtDiff::diff(const ValueRef &source, const ValueRef &target, const Omf *omf) {
    return on_value(std::shared_ptr<DiffChange>(), source, target);
  }

  std::shared_ptr<DiffChange> GrtDiff::on_value(std::shared_ptr<DiffChange> parent, const ValueRef &source,
                                                const ValueRef &target) {
    Type type;
    if (!are_compatible(source, target, &type))
      return on_uncompatible(parent, source, target);

    if (is_any(source))
      return ChangeFactory::create_value_added_change(parent, source, target, !_dont_clone_values);

    if (is_any(target))
      return ChangeFactory::create_value_removed_change(parent, source, target);

    // TODO use omf here to determine if we should replace value

    switch (type) {
      case IntegerType:
      case DoubleType:
      case StringType:
        return ChangeFactory::create_simple_value_change(parent, source, target);
        break;
      case ListType:
        return on_list(parent, BaseListRef::cast_from(source), BaseListRef::cast_from(target));
        break;
      case DictType:
        return on_dict(parent, DictRef::cast_from(source), DictRef::cast_from(target));
        break;
      case ObjectType:
        return on_object(parent, ObjectRef::cast_from(source), ObjectRef::cast_from(target));
        break;
      default:
        break;
    }
    assert(0);
    return std::shared_ptr<DiffChange>();
  }

  std::shared_ptr<DiffChange> GrtDiff::on_object(std::shared_ptr<DiffChange> parent, const ObjectRef &source,
                                                 const ObjectRef &target) {
    ChangeSet changes;
    MetaClass *meta = source.get_metaclass();

    if (meta->has_member("isStub")) {
      ValueRef v1 = source.get_member("isStub");
      ValueRef v2 = target.get_member("isStub");
      if ((1 == IntegerRef::cast_from(v1)) || (1 == IntegerRef::cast_from(v2)))
        return std::shared_ptr<DiffChange>();
    }
    if (meta->has_member("modelOnly")) {
      ValueRef v1 = source.get_member("modelOnly");
      ValueRef v2 = target.get_member("modelOnly");
      if ((1 == IntegerRef::cast_from(v1)) || (1 == IntegerRef::cast_from(v2)))
        return std::shared_ptr<DiffChange>();
    }

    // Compare all members of the objects with each other, looking for any differences
    do {
      for (MetaClass::MemberList::const_iterator iter = meta->get_members_partial().begin();
           iter != meta->get_members_partial().end(); ++iter) {
        if (iter->second.overrides)
          continue;

        std::string name = iter->second.name;
        std::string attr = meta->get_member_attribute(name, "dontdiff");
        int dontdiff = attr.size() && (base::atoi<int>(attr, 0) & omf->dontdiff_mask);

        if (dontdiff)
          continue;

        ValueRef v1 = source.get_member(name);
        ValueRef v2 = target.get_member(name);

        if (!v1.is_valid() && !v2.is_valid())
          continue;

        // don't bother with the rest if the values are simple and identical
        if (v1.type() == v2.type() && is_simple_type(v1.type()) && v1 == v2)
          continue;

        // skip empty containers
        if (v1.type() == grt::ListType)
          if (grt::BaseListRef::cast_from(v1).count() == 0 && grt::BaseListRef::cast_from(v2).count() == 0)
            continue;
        if (v1.type() == grt::DictType)
          if (grt::DictRef::cast_from(v1).count() == 0 && grt::DictRef::cast_from(v2).count() == 0)
            continue;

        if (omf->skip_routine_definer &&
            (source.class_name() == "db.mysql.Routine" || source.class_name() == "db.Routine") &&
            (name == "sqlDefinition" || name == "definer"))
          continue;

        if (omf->normalizer && omf->normalizer(source, target, name))
          continue;

        //        if (name == "sqlDefinition") continue;
        // v2 is our model
        std::shared_ptr<DiffChange> change;
        const bool dontfollow =
          !iter->second.owned_object && (name != "flags") && (name != "columns" || meta->is_a("db.Index"));
        if (dontfollow && GrtObjectRef::can_wrap(v1) && GrtObjectRef::can_wrap(v2)) {
          if (omf->normalizer && omf->normalizer(GrtObjectRef::cast_from(v1), GrtObjectRef::cast_from(v2), "name"))
            continue;
          StringRef n1(v1.is_valid() ? GrtObjectRef::cast_from(v1)->name() : "");
          StringRef n2(v2.is_valid() ? GrtObjectRef::cast_from(v2)->name() : "");
          if (n1 == n2)
            continue;
        }

#if 0
#error "don't use log_calls* for debug output! This is output is not meant to end up in the log."
      // Debug code below, modify it to suit your needs to see what is the exact difference being detected
      // between 2 objects
      if (source.class_name() == "db.mysql.Routine")
      {
        if (changes.empty())
          log_info("\nObject: %s <%s>  [%s]\n", source.get_string_member("name").c_str(), source.class_name().c_str(), source.id().c_str());

        std::string s1 = v1.description();
        std::string s2 = v2.description();
        if (s1 == s2)
          log_info("field %s came as different, but looks the same?\n", name.c_str());
        else
        {
          if (false)
          {
            int first_diff = strspn(s1.data(), s2.data());
            log_info("Changed field %s.'%s': [%i] %s -> %s\n", source.get_string_member("name").c_str(), name.c_str(), first_diff,
                     (s1.substr(std::max(first_diff-20, 0), 40)+"...").c_str(), (s2.substr(std::max(first_diff-20, 0), 40)+"...").c_str());
          }
          else
            log_info("Changed field %s.'%s':\nOBJECT 1: %s\n\nOBJECT 2: %s\n\n", source.get_string_member("name").c_str(), name.c_str(),
                     s1.c_str(), s2.c_str());

        }
      }
#endif

        change = ChangeFactory::create_object_attr_modified_change(
          parent, source, target, name, dontfollow ? ChangeFactory::create_simple_value_change(parent, v1, v2)
                                                   : on_value(std::shared_ptr<DiffChange>(), v1, v2));

        changes.append(change);
      }

      meta = meta->parent();
    } while (meta != 0);
    return ChangeFactory::create_object_modified_change(parent, source, target, changes);
  }

  std::shared_ptr<DiffChange> GrtDiff::on_list(std::shared_ptr<DiffChange> parent, const BaseListRef &source,
                                               const BaseListRef &target) {
    Type type;

    if (!are_compatible_lists(source, target, &type))
      return on_uncompatible(parent, source, target);

    return GrtListDiff::diff(source, target, omf);
  }

  std::shared_ptr<DiffChange> GrtDiff::on_dict(std::shared_ptr<DiffChange> parent, const DictRef &source,
                                               const DictRef &target) {
    ChangeSet changes;

    for (internal::Dict::const_iterator iter = source.begin(); iter != source.end(); ++iter) {
      std::string key = iter->first;
      ValueRef source_item(iter->second);

      if (!target.has_key(key))
        changes.append(ChangeFactory::create_dict_item_removed_change(parent, source, target, key));
      else
        changes.append(ChangeFactory::create_dict_item_modified_change(
          parent, source, target, key, on_value(std::shared_ptr<DiffChange>(), source_item, target.get(key))));
    }

    for (internal::Dict::const_iterator iter = target.begin(); iter != target.end(); ++iter) {
      std::string key = iter->first;
      ValueRef target_item(iter->second);

      if (!source.has_key(key))
        changes.append(ChangeFactory::create_dict_item_added_change(parent, source, target, key, target_item));
    }

    return ChangeFactory::create_dict_change(parent, source, target, changes);
  }

  std::shared_ptr<DiffChange> GrtDiff::on_uncompatible(std::shared_ptr<DiffChange> parent, const ValueRef &source,
                                                       const ValueRef &target) {
    return ChangeFactory::create_value_added_change(parent, source, target);
  }
}
