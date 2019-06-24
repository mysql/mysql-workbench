/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grtpp_module_python.h"

#include "grtpp_util.h"
#include "base/string_utilities.h"
#include "base/log.h"

#include <glib.h>
#include <functional>

DEFAULT_LOG_DOMAIN("grt")

using namespace grt;

#ifdef HAVE_CONFIG_H
#include "../../../config.h"
#endif

#if defined(_MSC_VER)
#include <objbase.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#else
// Linux specific
#include <uuid/uuid.h>
#endif

std::string grt::get_guid() {
/* GUIDs must be no more than 50 chars */

#if defined(_MSC_VER)
  GUID guid;
  WCHAR guid_wstr[50];
  char guid_str[200];

  int len;

  CoCreateGuid(&guid);
  len = StringFromGUID2(guid, (LPOLESTR)guid_wstr, 50);

  // Covert GUID from WideChar to utf8
  WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)guid_wstr, len, (LPSTR)guid_str, 200, NULL, NULL);

  return guid_str;
#elif defined(__APPLE__)
  CFUUIDRef uid;
  CFStringRef str;
  char data[40];

  uid = CFUUIDCreate(NULL);
  str = CFUUIDCreateString(NULL, uid);

#ifdef ENABLE_DEBUG
  assert((int)sizeof(data) > CFStringGetLength(str));
#endif
  // data= (char*)g_malloc(sizeof(gchar)*(CFStringGetLength(str)+1));

  CFStringGetCString(str, data, sizeof(data), kCFStringEncodingUTF8);

  CFRelease(uid);
  CFRelease(str);

  std::string ret = data;
  // g_free(data);

  return ret;
#else
  {
    uuid_t gid;
    char buffer[40];
    uuid_generate_time(gid);
    uuid_unparse(gid, buffer);
    return buffer;
  }
#endif
}

std::string grt::fmt_simple_type_spec(const SimpleTypeSpec &type) {
  switch (type.type) {
    case IntegerType:
      return "ssize_t";
    case DoubleType:
      return "double";
    case StringType:
      return "string";
    case ListType:
      return "list";
    case DictType:
      return "dict";
    case ObjectType:
      return type.object_class;
    default:
      return "??? invalid ???";
  }
}

std::string grt::fmt_type_spec(const TypeSpec &type) {
  switch (type.base.type) {
    case IntegerType:
      return "ssize_t";
    case DoubleType:
      return "double";
    case StringType:
      return "string";
    case ListType:
      switch (type.content.type) {
        case IntegerType:
          return "list<int>";
        case DoubleType:
          return "list<double>";
        case StringType:
          return "list<string>";
        case ListType:
          return "???? invalid ???";
        case DictType:
          return "list<dict>";
        case ObjectType:
          return "list<" + type.content.object_class + ">";
        default:
          return "??? invalid ???";
      }
    case DictType:
      return "dict";
    case ObjectType:
      if (type.base.object_class.empty())
        return "object";
      else
        return type.base.object_class;
    default:
      return "??? invalid ???";
  }
}

std::string grt::fmt_arg_spec_list(const ArgSpecList &arglist) {
  std::string args;

  for (ArgSpecList::const_iterator arg = arglist.begin(); arg != arglist.end(); ++arg) {
    if (!args.empty())
      args.append(", ");

    args.append(fmt_type_spec(arg->type));
    if (!arg->name.empty())
      args.append(" ").append(arg->name);
  }
  return args;
}

/**
 ****************************************************************************
 * @brief Traverses a dict or object and return the requested value
 *
 *   This will treat the given value as a tree, where each node is named by
 * the key in its parent.
 *   For dictionaries or objects, the path component is used as the key name.
 * For lists, the given component is used to match the name attribute of each
 * list item or if its a number, it's used as the index in the list.
 *
 * @param value a dict or object type value to be traversed which nested dicts
 * @param path a / separated list of key names to the value to be returned.
 *
 * @return A pointer to the value if it's found or NULL otherwise.
 *****************************************************************************/
ValueRef grt::get_value_by_path(const ValueRef &root, const std::string &path) {
  std::string prefix, suffix;
  ValueRef value = root;
  bool ok = true;

  if (path == "/")
    return value;

  suffix = path.substr(1); // skip the /
  // fix possible typos
  base::replaceStringInplace(suffix, "//", "/");

  while (!suffix.empty() && value.is_valid()) {
    prefix = base::pop_path_front(suffix);

    if (value.type() == DictType) {
      value = DictRef::cast_from(value).get(prefix);
      if (!value.is_valid()) {
        ok = false;
        break;
      }
    } else if (value.type() == ListType) {
      BaseListRef list(BaseListRef::cast_from(value));
      long lindex;

      if (sscanf(prefix.c_str(), "%li", &lindex) == 1 && lindex >= 0 && lindex < (long)list.count())
        value = list.get(lindex);
      else {
        ok = false;
        break;
      }
    } else if (value.type() == ObjectType) {
      value = ObjectRef::cast_from(value)->get_member(prefix);
      if (!value.is_valid()) {
        ok = false;
        break;
      }
    } else {
      ok = false;
      value.clear();
      break;
    }
  }

  if (!ok)
    logError("Invalid path element '%s' in path: %s\n", prefix.c_str(), path.c_str());
  return value;
}

/**
 ****************************************************************************
 * @brief Traverses a dict or object and changes the value in the path
 *
 *   This will treat the given value as a tree, where each node is named by
 * the key in its parent.
 *   For dics and objects, the path component is used as the key name. For lists,
 * the given component is used to match the name attribute of each list item.
 *
 * @param value a value to be traversed which nested values
 * @param path a / separated list of key names to the value to be changed.
 * @param new_value the value to assign in the dict
 *
 * @return true on success
 * @return false if some component of the path doesn't exist or the
 * referenced object is not a dictionary or list.
 *
 *****************************************************************************/
bool grt::set_value_by_path(const ValueRef &value, const std::string &path, const ValueRef &new_value) {
  std::string front, last;

  if (path == "/")
    return false;

  if (path.find('/') == std::string::npos)
    return false;

  last = path;
  if (*last.rbegin() == '/') // strip ending / if there is one
    last = last.substr(0, last.length() - 1);

  size_t last_slash = last.rfind('/');
  if (last_slash == std::string::npos)
    front = last;
  else if (last_slash == 0)
    front = "/";
  else
    front = last.substr(0, last_slash);
  last = last.substr(last.rfind('/') + 1);

  ValueRef container(get_value_by_path(value, front));
  if (container.is_valid()) {
    if (container.type() == DictType)
      DictRef::cast_from(container).set(last, new_value);
    else if (container.type() == ObjectType)
      ObjectRef::cast_from(container).set_member(last, new_value);
    else if (container.type() == ListType) {
      BaseListRef list(BaseListRef::cast_from(container));

      int i; // Unlikely we will ever need an index beyond 32bit.
      if (sscanf(last.c_str(), "%i", &i) != 1 || i >= (int)list.count())
        return false;

      list.gset(i, new_value);
    } else
      return false;

    return true;
  }

  return false;
}

static ObjectRef find_child_object(const BaseListRef &list, const std::string &id, bool recursive,
                                   std::set<internal::Value *> &visited);
static ObjectRef find_child_object(const DictRef &dict, const std::string &id, bool recursive,
                                   std::set<internal::Value *> &visited);
static ObjectRef find_child_object(const ObjectRef &object, const std::string &id, bool recursive,
                                   std::set<internal::Value *> &visited);

static ObjectRef find_child_object(const BaseListRef &list, const std::string &id, bool recursive,
                                   std::set<internal::Value *> &visited) {
  if (!list.is_valid())
    throw std::invalid_argument("list is invalid");

  if (visited.find(list.valueptr()) != visited.end())
    return ObjectRef();
  visited.insert(list.valueptr());

  size_t c = list.count(), i;
  ObjectRef found;

  for (i = 0; i < c && !found.is_valid(); i++) {
    ValueRef value = list.get(i);

    if (value.type() == ObjectType) {
      ObjectRef ovalue(ObjectRef::cast_from(value));
      if (ovalue.id() == id)
        return ovalue;

      if (recursive)
        found = find_child_object(ovalue, id, recursive, visited);
    } else if (value.type() == ListType && recursive)
      found = find_child_object(BaseListRef::cast_from(value), id, recursive, visited);
    else if (value.type() == DictType && recursive)
      found = find_child_object(DictRef::cast_from(value), id, recursive, visited);
  }

  return found;
}

static ObjectRef find_child_object(const DictRef &dict, const std::string &id, bool recursive,
                                   std::set<internal::Value *> &visited) {
  if (!dict.is_valid())
    throw std::invalid_argument("dict is invalid");

  if (visited.find(dict.valueptr()) != visited.end())
    return ObjectRef();
  visited.insert(dict.valueptr());

  ObjectRef found;

  DictRef::const_iterator iter;
  for (iter = dict.begin(); iter != dict.end() && !found.is_valid(); ++iter) {
    ValueRef value = iter->second;
    std::string key = iter->first;

    if (value.type() == ObjectType) {
      ObjectRef ovalue(ObjectRef::cast_from(value));
      if (ovalue.id() == id)
        return ovalue;

      if (recursive)
        found = find_child_object(ovalue, id, recursive, visited);
    } else if (value.type() == ListType && recursive)
      found = find_child_object(BaseListRef::cast_from(value), id, recursive, visited);
    else if (value.type() == DictType && recursive)
      found = find_child_object(DictRef::cast_from(value), id, recursive, visited);
  }

  return found;
}

static ObjectRef find_child_object(const ObjectRef &object, const std::string &id, bool recursive,
                                   std::set<internal::Value *> &visited) {
  if (!object.is_valid())
    throw std::invalid_argument("object is invalid");

  if (visited.find(object.valueptr()) != visited.end())
    return ObjectRef();
  visited.insert(object.valueptr());

  ObjectRef found;

  if (object.id() == id)
    return object;

  MetaClass *mclass = object->get_metaclass();

  do {
    for (MetaClass::MemberList::const_iterator member = mclass->get_members_partial().begin();
         member != mclass->get_members_partial().end(); ++member) {
      if (member->second.overrides)
        continue;

      std::string name = member->second.name;
      ValueRef value = object->get_member(name);

      // ignore owner
      if (name == "owner")
        continue;

      switch (value.type()) {
        case ListType: {
          BaseListRef list(BaseListRef::cast_from(value));

          if (recursive && !is_simple_type(list.content_type())) {
            found = find_child_object(list, id, recursive, visited);
            if (found.is_valid())
              return found;
          }
        } break;
        case DictType: {
          DictRef dict(DictRef::cast_from(value));

          if (recursive && !is_simple_type(dict.content_type())) {
            found = find_child_object(dict, id, recursive, visited);
            if (found.is_valid())
              return found;
          }
        } break;
        case ObjectType:
          if (ObjectRef::cast_from(value).id() == id)
            return ObjectRef::cast_from(value);

          if (recursive) {
            found = find_child_object(ObjectRef::cast_from(value), id, recursive, visited);
            if (found.is_valid())
              return found;
          }
          break;
        default:
          break;
      }
    }

    mclass = mclass->parent();
  } while (mclass != 0);

  return ObjectRef();
}

ObjectRef grt::find_child_object(const DictRef &dict, const std::string &id, bool recursive) {
  std::set<internal::Value *> visited;

  return ::find_child_object(dict, id, recursive, visited);
}

ObjectRef grt::find_child_object(const BaseListRef &list, const std::string &id, bool recursive) {
  std::set<internal::Value *> visited;

  return ::find_child_object(list, id, recursive, visited);
}

ObjectRef grt::find_child_object(const ObjectRef &object, const std::string &id, bool recursive) {
  std::set<internal::Value *> visited;

  return ::find_child_object(object, id, recursive, visited);
}

class search_in_list_pred : public std::function<bool (std::string)> {
private:
  ObjectListRef _list;

public:
  search_in_list_pred(const ObjectListRef &list) : _list(list){};
  result_type operator()(std::string arg) const {
    return find_named_object_in_list(_list, arg).is_valid();
  }
};

std::string grt::get_name_suggestion_for_list_object(const BaseListRef &baselist, const std::string &prefix,
                                                     bool serial) {
  return get_name_suggestion(search_in_list_pred(ObjectListRef::cast_from(baselist)), prefix, serial);
}

void CopyContext::copy_list(BaseListRef &list, const BaseListRef &source, bool dontfollow) {
  for (size_t c = source.count(), i = 0; i < c; i++) {
    grt::ValueRef value(source.get(i));

    if (!value.is_valid() || is_simple_type(value.type()))
      list.ginsert(value);
    else if (value.type() == ListType) {
      if (dontfollow)
        list.ginsert(value);
      else {
        BaseListRef clist(true);
        copy_list(clist, BaseListRef::cast_from(value), dontfollow);
        list.ginsert(clist);
      }
    } else if (value.type() == DictType) {
      if (dontfollow)
        list.ginsert(value);
      else {
        DictRef cdict(true);
        copy_dict(cdict, DictRef::cast_from(value), dontfollow);
        list.ginsert(cdict);
      }
    } else if (value.type() == ObjectType) {
      if (dontfollow)
        list.ginsert(value);
      else
        list.ginsert(copy(ObjectRef::cast_from(value)));
    }
  }
}

void CopyContext::copy_dict(DictRef &dict, const DictRef &source, bool dontfollow) {
  for (DictRef::const_iterator iter = source.begin(); iter != source.end(); ++iter) {
    std::string key = iter->first;
    grt::ValueRef value = iter->second;

    if (!value.is_valid() || is_simple_type(value.type()))
      dict.set(key, value);
    else if (value.type() == ListType) {
      if (dontfollow)
        dict.set(key, value);
      else {
        BaseListRef clist(true);
        copy_list(clist, BaseListRef::cast_from(value), dontfollow);
        dict.set(key, clist);
      }
    } else if (value.type() == DictType) {
      if (dontfollow)
        dict.set(key, value);
      else {
        DictRef cdict(true);
        copy_dict(cdict, DictRef::cast_from(value), dontfollow);
        dict.set(key, cdict);
      }
    } else if (value.type() == ObjectType) {
      if (dontfollow)
        dict.set(key, value);
      else
        dict.set(key, copy(ObjectRef::cast_from(value)));
    }
  }
}

static void fixup_object_copied_references(ObjectRef copy, std::map<std::string, ValueRef> &object_copies) {
  MetaClass *metac(copy.get_metaclass());

  do {
    for (MetaClass::MemberList::const_iterator mem = metac->get_members_partial().begin();
         mem != metac->get_members_partial().end(); ++mem) {
      if (mem->second.overrides)
        continue;

      std::string k = mem->second.name;
      grt::ValueRef v = copy->get_member(k);

      if (!v.is_valid())
        continue;

      Type type = mem->second.type.base.type;

      bool dontfollow = !mem->second.owned_object;

      if (type == ListType) {
        BaseListRef list(BaseListRef::cast_from(v));

        for (size_t c = list.count(), i = 0; i < c; i++) {
          ValueRef value(list.get(i));

          if (value.type() != ObjectType)
            continue;

          if (dontfollow) {
            ObjectRef obj(ObjectRef::cast_from(value));
            if (object_copies.find(obj.id()) != object_copies.end()) {
              list.gset(i, object_copies[obj.id()]);
            }
          } else
            fixup_object_copied_references(ObjectRef::cast_from(value), object_copies);
        }
      } else if (type == DictType) {
        DictRef dict(DictRef::cast_from(v));
        for (DictRef::const_iterator iterator = dict.begin(); iterator != dict.end(); iterator++) {
          std::string k = iterator->first;
          ValueRef value = iterator->second;
          if (value.type() != ObjectType)
            continue;

          if (dontfollow) {
            ObjectRef obj(ObjectRef::cast_from(value));
            if (object_copies.find(obj.id()) != object_copies.end())
              dict[k] = object_copies[obj.id()];
          } else
            fixup_object_copied_references(ObjectRef::cast_from(value), object_copies);
        }
      } else if (type == ObjectType) {
        // if a dontfollow member is being copied too, it should be updated later to
        // point to the new copy
        if (dontfollow) {
          ObjectRef obj(ObjectRef::cast_from(v));
          if (object_copies.find(obj.id()) != object_copies.end())
            copy.set_member(k, object_copies[obj.id()]);
        } else
          fixup_object_copied_references(ObjectRef::cast_from(v), object_copies);
      }
    }

    metac = metac->parent();
  } while (metac != 0);
}

ObjectRef CopyContext::duplicate_object(ObjectRef object, std::set<std::string> skip_members, bool _dontfollow) {
  if (object.is_valid()) {
    MetaClass *metac(object.get_metaclass());
    ObjectRef copy = metac->allocate();

    // save a mapping from the original value to its copy
    object_copies[object.id()] = copy;

    do {
      for (MetaClass::MemberList::const_iterator mem = metac->get_members_partial().begin();
           mem != metac->get_members_partial().end(); ++mem) {
        std::string k = mem->second.name;
        grt::ValueRef v = object.get_member(k);

        if (skip_members.find(k) != skip_members.end() || mem->second.overrides)
          continue;

        if (mem->second.calculated) // don't copy calculated members
          continue;

        Type type = mem->second.type.base.type;

        bool dontfollow = _dontfollow || !mem->second.owned_object;

        if (is_simple_type(type)) {
          copy.set_member(k, v);
        } else if (type == ListType) {
          BaseListRef clist(BaseListRef::cast_from(copy.get_member(k)));
          BaseListRef olist(BaseListRef::cast_from(v));

          copy_list(clist, olist, dontfollow);
        } else if (type == DictType) {
          DictRef dict(DictRef::cast_from(copy.get_member(k)));
          copy_dict(dict, DictRef::cast_from(v), dontfollow);
        } else if (type == ObjectType) {
          // if a dontfollow member is being copied too, it should be updated later to
          // point to the new copy
          if (dontfollow) {
            ObjectRef obj(ObjectRef::cast_from(v));
            if (obj.is_valid() && object_copies.find(obj.id()) != object_copies.end())
              copy.set_member(k, object_copies[obj.id()]);
            else
              copy.set_member(k, v);
          } else {
            if (k == "owner")
              throw; // consistency check
            ObjectRef vcopy(duplicate_object(ObjectRef::cast_from(v), std::set<std::string>(), false));
            copy.set_member(k, vcopy);
          }
        }
      }

      metac = metac->parent();
    } while (metac != 0);

    return copy;
  }
  return ObjectRef();
}

void grt::update_ids(ObjectRef object, const std::set<std::string> &skip_members) {
  if (!object.is_valid())
    return;
  MetaClass *metac(object.get_metaclass());

  do {
    for (MetaClass::MemberList::const_iterator mem = metac->get_members_partial().begin();
         mem != metac->get_members_partial().end(); ++mem) {
      std::string k = mem->second.name;
      grt::ValueRef v = object.get_member(k);

      if (skip_members.find(k) != skip_members.end() || mem->second.overrides)
        continue;

      if (mem->second.calculated)
        continue;

      Type type = mem->second.type.base.type;

      if (!mem->second.owned_object)
        continue;

      if (type == ListType) {
        BaseListRef olist(BaseListRef::cast_from(v));
        for (size_t c = olist.count(), i = 0; i < c; i++) {
          if (ObjectRef::can_wrap(olist[i]))
            update_ids(ObjectRef::cast_from(olist[i]), skip_members);
        }
      } else if (type == DictType) {
        DictRef::cast_from(v);
      } else if (type == ObjectType) {
        update_ids(ObjectRef::cast_from(v), skip_members);
      }
    }
    metac = metac->parent();
  } while (metac != 0);
  object->__set_id(get_guid());
}

void grt::append_contents(BaseListRef target, BaseListRef source) {
  if (source.is_valid()) {
    for (size_t c = source.count(), i = 0; i < c; i++)
      target.ginsert(source[i]);
  }
}

void grt::replace_contents(BaseListRef target, BaseListRef source) {
  for (size_t c = target.count(), i = 0; i < c; i++)
    target.remove(0);

  for (size_t c = source.count(), i = 0; i < c; i++)
    target.ginsert(source[i]);
}

void grt::merge_contents_by_name(ObjectListRef target, ObjectListRef source, bool replace_matching) {
  std::map<std::string, int> known_names;

  for (size_t c = target.count(), i = 0; i < c; i++)
    known_names[StringRef::cast_from(target[i].get_member("name"))] = (int)i;

  for (size_t c = source.count(), i = 0; i < c; i++) {
    ObjectRef value(source[i]);
    std::string name = StringRef::cast_from(value.get_member("name"));
    if (known_names.find(name) != known_names.end()) {
      if (replace_matching)
        target.set(known_names[name], value);
    } else
      target.insert(value);
  }
}

void grt::merge_contents_by_id(ObjectListRef target, ObjectListRef source, bool replace_matching) {
  std::map<std::string, size_t> index_of_known_ids;

  for (size_t c = target.count(), i = 0; i < c; i++)
    index_of_known_ids[target[i].id()] = i;

  for (size_t c = source.count(), i = 0; i < c; i++) {
    ObjectRef value(source[i]);
    if (index_of_known_ids.find(value.id()) != index_of_known_ids.end()) {
      if (replace_matching)
        target.set(index_of_known_ids[value.id()], value);
    } else
      target.insert(value);
  }
}

void grt::merge_contents(DictRef target, DictRef source, bool overwrite) {
  DictRef::const_iterator iter;
  for (iter = source.begin(); iter != source.end(); ++iter) {
    std::string k = iter->first;
    ValueRef v = iter->second;

    if (!overwrite && target.has_key(k))
      continue;

    target.set(k, v);
  }
}

void grt::replace_contents(DictRef target, DictRef source) {
  DictRef::const_iterator iter, current;

  iter = target.begin();
  while (iter != target.end()) {
    current = iter;
    ++iter;
    target.remove(current->first);
  }

  for (iter = source.begin(); iter != source.end(); ++iter) {
    target.set(iter->first, iter->second);
  }
}

void grt::merge_contents(ObjectRef target, ObjectRef source) {
  MetaClass *metac = source->get_metaclass();

  do {
    for (MetaClass::MemberList::const_iterator mem = metac->get_members_partial().begin();
         mem != metac->get_members_partial().end(); ++mem) {
      if (mem->second.overrides || mem->second.read_only)
        continue;

      std::string k = mem->second.name;
      ValueRef v = source->get_member(k);

      target.set_member(k, v);
    }
    metac = metac->parent();
  } while (metac != 0);
}

std::string grt::join_string_list(const StringListRef &list, const std::string &separator) {
  std::string result;
  for (StringListRef::const_iterator i = list.begin(); i != list.end(); ++i) {
    if (i != list.begin())
      result.append(separator);
    result.append(*i);
  }
  return result;
}

ObjectRef CopyContext::copy(const ObjectRef &object, std::set<std::string> skip_members) {
  ObjectRef copy = duplicate_object(object, skip_members, false);
  if (copy.is_valid())
    copies.push_back(copy);

  return copy;
}

ObjectRef CopyContext::shallow_copy(const ObjectRef &object) {
  ObjectRef copy = duplicate_object(object, std::set<std::string>(), true);
  if (copy.is_valid())
    copies.push_back(copy);

  return copy;
}

void CopyContext::update_references() {
  // go through everything that was copied, replacing copied object references
  // if needed
  for (std::list<ObjectRef>::iterator iter = copies.begin(); iter != copies.end(); ++iter)
    fixup_object_copied_references(*iter, object_copies);
}

ValueRef CopyContext::copy_for_object(ValueRef object) {
  ObjectRef obj(ObjectRef::cast_from(object));
  if (object_copies.find(obj.id()) != object_copies.end())
    return object_copies[obj.id()];
  return ValueRef();
}

static ValueRef copy_value(ValueRef value, bool deep, internal::Object *owner) {
  switch (value.type()) {
    case AnyType:
      break;

    case IntegerType:
    case DoubleType:
    case StringType:
      // no need to duplicate simple values as they're immutable
      return value;

    case ListType: {
      BaseListRef orig(BaseListRef::cast_from(value));
      BaseListRef list(orig.content_type(), orig.content_class_name());

      // Copy items
      if (deep)
        for (internal::List::raw_const_iterator iter = orig.content().raw_begin(); iter != orig.content().raw_end();
             ++iter)
          list.ginsert(copy_value(*iter, deep, 0));
      else
        for (internal::List::raw_const_iterator iter = orig.content().raw_begin(); iter != orig.content().raw_end();
             ++iter)
          list.ginsert(*iter);

      return list;
    }
    case DictType: {
      DictRef orig(DictRef::cast_from(value));
      DictRef dict(orig.content_type(), orig.content_class_name());

      // Copy items
      if (deep)
        for (internal::Dict::const_iterator iter = orig.content().begin(); iter != orig.content().end(); ++iter)
          dict.set(iter->first, copy_value(iter->second, deep, 0));
      else
        for (internal::Dict::const_iterator iter = orig.content().begin(); iter != orig.content().end(); ++iter)
          dict.set(iter->first, iter->second);

      return dict;
    }
    case ObjectType: {
      ObjectRef obj(ObjectRef::cast_from(value));

      return copy_object(obj);
    }
  }

  return ValueRef();
}

ValueRef grt::copy_value(ValueRef value, bool deep) {
  return ::copy_value(value, deep, 0);
}

bool grt::compare_list_contents(const ObjectListRef &l1, const ObjectListRef &l2) {
  bool l1_valid = l1.is_valid();
  bool l2_valid = l2.is_valid();
  if (!l1_valid || !l2_valid)
    return (l1_valid == l2_valid);
  if (l1.count() != l2.count())
    return false;
  for (size_t n = 0, count = l1.count(); n < count; ++n) {
    grt::ObjectRef o1 = l1.get(n);
    grt::ObjectRef o2 = l2.get(n);
    if (o1.is_valid() != o2.is_valid())
      return false;
    if (o1.is_valid() && (o1.id() != o2.id()))
      return false;
  }
  return true;
}

void grt::remove_list_items_matching(ObjectListRef list, const std::function<bool(grt::ObjectRef)> &matcher) {
  for (size_t i = list.count(); i >= 1; --i) {
    if (matcher(list[i - 1]))
      list.remove(i - 1);
  }
}

// temporary code, should be replaced with dynamic loading once langauge support is pluginized

bool grt::init_python_support(const std::string &module_path) {
  PythonModuleLoader *loader = new PythonModuleLoader(module_path);
  if (!module_path.empty()) {
    loader->get_python_context()->add_module_path(module_path, true);
#ifdef _MSC_VER
    loader->get_python_context()->add_module_path(module_path + "/python/site-packages", true);
#endif
  }
  grt::GRT::get()->add_module_loader(loader);
  return true;
}

void grt::add_python_module_dir(const std::string &python_module_path) {
  PythonModuleLoader *loader = dynamic_cast<PythonModuleLoader *>(grt::GRT::get()->get_module_loader("python"));
  if (loader && !python_module_path.empty())
    loader->get_python_context()->add_module_path(python_module_path, true);
}

static void dump_value(const grt::ValueRef &value, int level, bool skip_spacing = false);

static bool dump_member(grt::ObjectRef object, const grt::MetaClass::Member *member, int level) {
  if (!object.get_member(member->name).is_valid())
    printf("%*s%s = NULL", level, "  ", member->name.c_str());
  else if (member->type.base.type == grt::ObjectType && !member->owned_object)
    printf("%*s%s = <<%s>>", level, "  ", member->name.c_str(),
           grt::ObjectRef::cast_from(object.get_member(member->name))->get_string_member("name").c_str());
  else {
    printf("%*s%s = ", level, "  ", member->name.c_str());
    ::dump_value(object.get_member(member->name), level + 1, true);
  }
  printf(";\n");
  return true;
}

static void dump_value(const grt::ValueRef &value, int level, bool skip_spacing) {
  int s = skip_spacing ? 0 : 1;
  switch (value.type()) {
    case grt::ListType: {
      grt::BaseListRef list(grt::BaseListRef::cast_from(value));
      printf("%*s%s", level * s, "  ", "[\n");
      for (size_t i = 0; i < list.count(); i++) {
        if (i > 0)
          printf(",\n");
        ::dump_value(list[i], level + 1);
      }
      printf("%*s%s", level, "  ", "]");
      break;
    }

    case grt::DictType: {
      grt::DictRef dict(grt::DictRef::cast_from(value));
      printf("%*s%s", level, "  ", "  {\n");
      for (grt::DictRef::const_iterator iter = dict.begin(); iter != dict.end(); ++iter) {
        if (iter != dict.begin())
          printf(",\n");
        printf("%*s%s: ", level, "  ", iter->first.c_str());
        ::dump_value(iter->second, level + 1, true);
      }
      printf("%*s%s", level, "  ", "}");
      break;
    }

    case grt::ObjectType: {
      grt::ObjectRef object(grt::ObjectRef::cast_from(value));
      grt::MetaClass *mc = object.get_metaclass();
      printf("%*s%s", level, "  ", "  {\n");
      mc->foreach_member(std::bind(::dump_member, object, std::placeholders::_1, level + 1));
      printf("%*s%s", level, "  ", "}");
      break;
    }

    default:
      printf("%*s%s", level, "  ", value.debugDescription().c_str());
      break;
  }
}

void grt::dump_value(const grt::ValueRef &value) {
  ::dump_value(value, 0);
  printf("\n");
}
