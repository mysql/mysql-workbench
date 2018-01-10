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

#pragma once

#include "grt.h"

#include <set>

#include "base/any.h"
#include "base/string_utilities.h"

#define GRTLIST_FOREACH(type, list, iter) \
  for (grt::ListRef<type>::const_iterator iter##end = list.end(), iter = list.begin(); iter != iter##end; ++iter)

#define GRTLIST_REVERSE_FOREACH(type, list, iter)                                                                   \
  for (grt::ListRef<type>::const_reverse_iterator iter##end = list.rend(), iter = list.rbegin(); iter != iter##end; \
       ++iter)

namespace grt {
  std::map<std::string, base::any> MYSQLGRT_PUBLIC convert(const grt::DictRef dict);

  std::string MYSQLGRT_PUBLIC type_to_str(Type type);
  Type MYSQLGRT_PUBLIC str_to_type(const std::string &str);

  std::string MYSQLGRT_PUBLIC fmt_simple_type_spec(const SimpleTypeSpec &type);
  std::string MYSQLGRT_PUBLIC fmt_type_spec(const TypeSpec &type);
  std::string MYSQLGRT_PUBLIC fmt_arg_spec_list(const ArgSpecList &args);

  ValueRef MYSQLGRT_PUBLIC get_value_by_path(const ValueRef &root, const std::string &path);
  bool MYSQLGRT_PUBLIC set_value_by_path(const ValueRef &value, const std::string &path, const ValueRef &new_value);

  inline bool is_container_type(Type type) {
    if (type == ListType || type == DictType || type == ObjectType)
      return true;
    return false;
  }

  inline bool is_simple_type(Type type) {
    if (type == IntegerType || type == DoubleType || type == StringType)
      return true;
    return false;
  }

  std::string MYSQLGRT_PUBLIC get_guid();

  inline std::string path_base(const std::string &path) {
    std::string::size_type p = path.rfind('/');
    if (p != std::string::npos)
      return path.substr(0, p);
    return "";
  }

  inline std::string path_last(const std::string &path) {
    std::string::size_type p = path.rfind('/');
    if (p != std::string::npos)
      return path.substr(p);
    return "";
  }

  template <class O>
  inline Ref<O> find_named_object_in_list(const ListRef<O> &list, const std::string &value, bool case_sensitive = true,
                                          const std::string &name = "name") {
    for (size_t i = 0; i < list.count(); i++) {
      Ref<O> tmp = list[i];

      if (tmp.is_valid() && base::same_string(tmp->get_string_member(name), value, case_sensitive))
        return tmp;
    }
    return Ref<O>();
  }

  template <class O>
  inline Ref<O> find_object_in_list(const ListRef<O> &list, const std::string &id) {
    size_t i, c = list.count();
    for (i = 0; i < c; i++) {
      Ref<O> value = list[i];

      if (value.is_valid() && value->id() == id)
        return value;
    }
    return Ref<O>();
  }

  template <class O>
  inline size_t find_object_index_in_list(ListRef<O> list, const std::string &id) {
    size_t i, c = list.count();
    for (i = 0; i < c; i++) {
      Ref<O> value = list.get(i);

      if (value.is_valid() && value.id() == id)
        return i;
    }
    return -1;
  }

  template <typename TPredicate>
  std::string get_name_suggestion(TPredicate duplicate_found_pred, const std::string &prefix, const bool serial) {
    char buffer[30] = "";
    int x = 1;
    std::string name;

    if (serial)
      g_snprintf(buffer, sizeof(buffer), "%i", x);
    name = prefix + buffer;
    while (duplicate_found_pred(name)) {
      g_snprintf(buffer, sizeof(buffer), "%i", x++);
      name = prefix + buffer;
    }
    return name;
  }

  MYSQLGRT_PUBLIC std::string get_name_suggestion_for_list_object(const BaseListRef &objlist, const std::string &prefix,
                                                                  bool serial = true);

  MYSQLGRT_PUBLIC ObjectRef find_child_object(const DictRef &dict, const std::string &id, bool recursive = true);
  MYSQLGRT_PUBLIC ObjectRef find_child_object(const BaseListRef &list, const std::string &id, bool recursive = true);
  MYSQLGRT_PUBLIC ObjectRef find_child_object(const ObjectRef &object, const std::string &id, bool recursive = true);

  MYSQLGRT_PUBLIC void update_ids(ObjectRef object,
                                  const std::set<std::string> &skip_members = std::set<std::string>());
  // the following merge functions are not recursive
  MYSQLGRT_PUBLIC void append_contents(BaseListRef target, BaseListRef source);
  MYSQLGRT_PUBLIC void replace_contents(BaseListRef target, BaseListRef source);
  MYSQLGRT_PUBLIC void merge_contents_by_name(ObjectListRef target, ObjectListRef source, bool replace_matching);
  MYSQLGRT_PUBLIC void merge_contents_by_id(ObjectListRef target, ObjectListRef source, bool replace_matching);

  MYSQLGRT_PUBLIC void replace_contents(DictRef target, DictRef source);
  MYSQLGRT_PUBLIC void merge_contents(DictRef target, DictRef source, bool overwrite);
  MYSQLGRT_PUBLIC void merge_contents(ObjectRef target, ObjectRef source);

  MYSQLGRT_PUBLIC bool compare_list_contents(const ObjectListRef &list1, const ObjectListRef &list2);

  MYSQLGRT_PUBLIC std::string join_string_list(const StringListRef &list, const std::string &separator);

  MYSQLGRT_PUBLIC void remove_list_items_matching(ObjectListRef list,
                                                  const std::function<bool(grt::ObjectRef)> &matcher);

  // XXX don't use this for objects, use CopyContext::copy() instead
  MYSQLGRT_PUBLIC ValueRef copy_value(ValueRef value, bool deep);

  struct MYSQLGRT_PUBLIC CopyContext {
    std::map<std::string, ValueRef> object_copies;
    std::list<ObjectRef> copies;

    CopyContext() {
    }

    ObjectRef copy(const ObjectRef &object, std::set<std::string> skip_members = std::set<std::string>());
    ObjectRef shallow_copy(const ObjectRef &object);
    void finish() {
      update_references();
    }
    void update_references();

    ValueRef copy_for_object(ValueRef object);

  private:
    ObjectRef duplicate_object(ObjectRef object, std::set<std::string> skip_members, bool dontfollow);
    void copy_list(BaseListRef &list, const BaseListRef &source, bool dontfollow);
    void copy_dict(DictRef &dict, const DictRef &source, bool dontfollow);
  };

  template <typename OType>
  OType copy_object(const OType &object, std::set<std::string> skip_members = std::set<std::string>()) {
    CopyContext copier;
    OType copy;

    copy = OType::cast_from(copier.copy(object, skip_members));
    copier.finish();

    return copy;
  }

  template <typename OType>
  OType shallow_copy_object(const OType &object) {
    CopyContext copier;
    OType copy;

    copy = OType::cast_from(copier.shallow_copy(object));
    // this is a shallow copy so changing the references in it's sub-values would mean modifying
    // their originals
    // copier.finish();

    return copy;
  }

  MYSQLGRT_PUBLIC void dump_value(const grt::ValueRef &value);

  // temporary code
  MYSQLGRT_PUBLIC bool init_python_support(const std::string &python_module_path);
  MYSQLGRT_PUBLIC void add_python_module_dir(const std::string &python_module_path);

  // diffing

  class DiffChange;
  typedef std::function<bool(ValueRef, ValueRef, std::string)> TSlotNormalizerSlot;

  struct MYSQLGRT_PUBLIC Omf {
    TSlotNormalizerSlot normalizer;
    bool case_sensitive;
    bool skip_routine_definer;
    // some structure members needs to be skipped only when diffing model vs db but not when diffing model
    //_dontdiff_mask will hold mask to allow selective bypass of ceratin fields
    // 1 always diff, 2 diff only vs db, 4 diff only vs live object
    unsigned int dontdiff_mask;
    Omf() : case_sensitive(true), skip_routine_definer(false), dontdiff_mask(1){};
    virtual ~Omf(){};
    virtual bool less(const ValueRef &, const ValueRef &) const = 0;
    virtual bool equal(const ValueRef &, const ValueRef &) const = 0;
  };

  struct default_omf : public Omf {
    bool peq(const ValueRef &l, const ValueRef &r) const {
      if ((l.type() == r.type() && l.type() == ObjectType) && ObjectRef::can_wrap(l) && ObjectRef::can_wrap(r)) {
        ObjectRef left = ObjectRef::cast_from(l);
        ObjectRef right = ObjectRef::cast_from(r);
        if (left->has_member("name"))
          return left->get_string_member("name") == right->get_string_member("name");
      }
      return l == r;
    }

    bool pless(const ValueRef &l, const ValueRef &r) const {
      if ((l.type() == r.type() && l.type() == ObjectType) && ObjectRef::can_wrap(l) && ObjectRef::can_wrap(r)) {
        ObjectRef left = ObjectRef::cast_from(l);
        ObjectRef right = ObjectRef::cast_from(r);
        if (left->has_member("name"))
          return left->get_string_member("name") < right->get_string_member("name");
      }
      return l < r;
    }

    virtual bool less(const ValueRef &l, const ValueRef &r) const {
      return pless(l, r);
    };
    virtual bool equal(const ValueRef &l, const ValueRef &r) const {
      return peq(l, r);
    };
  };

  MYSQLGRT_PUBLIC
  std::shared_ptr<DiffChange> diff_make(const ValueRef &source, const ValueRef &target, const Omf *omf,
                                        bool dont_clone_values = false);
};
