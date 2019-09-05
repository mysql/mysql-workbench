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

#include "grt.h"
#include "grtpp_util.h"
#include "base/log.h"
#include <glib.h>
#include <algorithm>

DEFAULT_LOG_DOMAIN(DOMAIN_GRT)

using namespace grt;

inline std::string get_prop(xmlNodePtr node, const char *name) {
  xmlChar *prop = xmlGetProp(node, (xmlChar *)name);
  std::string tmp = prop ? (char *)prop : "";
  xmlFree(prop);
  return tmp;
}

inline bool get_type_spec(xmlNodePtr node, TypeSpec &type, bool allow_void = false) {
  std::string s = get_prop(node, "type");

  if (allow_void && s == "void") {
    type.base.type = UnknownType;
    return true;
  }

  type.base.type = str_to_type(s);
  if (type.base.type == UnknownType) {
    logWarning("[XML parser] Unknown type '%s'.", s.c_str());
    return false;
  }

  // lists and dicts can be typed
  if ((type.base.type == ListType) || (type.base.type == DictType)) {
    std::string content_type = get_prop(node, "content-type");
    std::string content_object_class = get_prop(node, "content-struct-name");

    if (!content_type.empty()) {
      type.content.type = str_to_type(content_type);
      if (type.content.type == UnknownType) {
        logWarning("[XML parser] Unknown content-type '%s'.\n", content_type.c_str());
        return false;
      }
    }
    if (!content_object_class.empty())
      type.content.object_class = content_object_class;
  } else if (type.base.type == ObjectType) {
    std::string class_name = get_prop(node, "struct-name");

    if (!class_name.empty())
      type.base.object_class = class_name;
    else {
      logWarning("[XML parser] object member without struct-name.\n");
      return false;
    }
  }
  return true;
}

/** Generates the checksum of a GRT metaclass.
 *
 *   Calculates the checksum of all members+types of the checksum so that
 * changes made to them, could be caught by comparing checksums. Both
 * the names and types of members are checksummed. The order of members does
 * not affect the checksum.
 *
 * Note: crc32 from Public Domain code from www.varnish-cache.org
 *
 * @param metaclass the class to be checksummed
 *
 * @return 32bit CRC of the class.
 */
static unsigned int make_checksum(MetaClass *metaclass) {
  static unsigned int crc32bits[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832,
    0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a,
    0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
    0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab,
    0xb6662d3d, 0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4,
    0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074,
    0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525,
    0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
    0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76,
    0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6,
    0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7,
    0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7,
    0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
    0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c, 0xcabac28a, 0x53b39330,
    0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

  const unsigned char *p;
  unsigned int crc = ~0U;
  GSList *strings = NULL;
  GSList *node;

  const std::map<std::string, MetaClass::Member> &members(metaclass->get_members_partial());
  const std::map<std::string, MetaClass::Method> &methods(metaclass->get_methods_partial());

  // create a sorted list with the class members
  for (std::map<std::string, MetaClass::Member>::const_iterator iter = members.begin(); iter != members.end(); ++iter) {
    const MetaClass::Member &member(iter->second);

    strings = g_slist_insert_sorted(
      strings,
      g_strdup_printf("%s %i %s %i %s %i %i %i %i %i", member.name.c_str(), member.type.base.type,
                      member.type.base.object_class.c_str(), member.type.content.type,
                      member.type.content.object_class.c_str(), member.read_only ? 1 : 0, member.delegate_get ? 1 : 0,
                      member.delegate_set ? 1 : 0, member.private_ ? 1 : 0, member.calculated ? 1 : 0),
      (GCompareFunc)strcmp);
  }
  for (std::map<std::string, MetaClass::Method>::const_iterator iter = methods.begin(); iter != methods.end(); ++iter) {
    const MetaClass::Method &method(iter->second);
    char *tmp = g_strdup_printf("%s %i %s %i %s", method.name.c_str(), method.ret_type.base.type,
                                method.ret_type.base.object_class.c_str(), method.ret_type.content.type,
                                method.ret_type.content.object_class.c_str());
    strings = g_slist_insert_sorted(strings, tmp, (GCompareFunc)strcmp);

    for (ArgSpecList::const_iterator iter = method.arg_types.begin(); iter != method.arg_types.end(); ++iter) {
      char *tmp = g_strdup_printf("= %s %i %s %i %s", iter->name.c_str(), iter->type.base.type,
                                  iter->type.base.object_class.c_str(), iter->type.content.type,
                                  iter->type.content.object_class.c_str());

      strings = g_slist_insert_sorted(strings, tmp, (GCompareFunc)strcmp);
    }
  }

  strings = g_slist_prepend(strings, g_strdup_printf("%s : %s", metaclass->name().c_str(),
                                                     metaclass->parent() ? metaclass->parent()->name().c_str() : ""));

  // calculate crc of everything
  for (node = strings; node != NULL; node = g_slist_next(node)) {
    for (p = (const unsigned char *)node->data; *p != 0; p++)
      crc = (crc >> 8) ^ crc32bits[(crc ^ *p) & 0xff];

    g_free(node->data);
  }

  // free everything
  g_slist_free(strings);

  return (crc ^ ~0U);
}

bool MetaClass::foreach_validator(const ObjectRef &obj, const Validator::Tag &tag) {
  bool ret = true;
  const ValidatorList::size_type size = _validators.size();
  for (ValidatorList::size_type i = 0; i < size; ++i) {
    if (_validators[i]->validate(tag, obj))
      ret = false;
  }
  return ret;
}

bool MetaClass::has_member(const std::string &member) const {
  if (_members.find(member) == _members.end()) {
    if (_parent)
      return _parent->has_member(member);
    return false;
  }
  return true;
}

bool MetaClass::has_method(const std::string &method) const {
  if (_methods.find(method) == _methods.end()) {
    if (_parent)
      return _parent->has_method(method);
    return false;
  }
  return true;
}

bool MetaClass::is_a(const std::string &name) const {
  MetaClass *mc = grt::GRT::get()->get_metaclass(name);
  if (!mc)
    return false;

  return is_a(mc);
}

bool MetaClass::is_a(MetaClass *struc) const {
  const MetaClass *par = this;

  while (par && par != struc)
    par = par->_parent;

  return par == struc;
}

MetaClass::MetaClass() {
  _crc32 = 0;
  _parent = 0;
  _placeholder = false;
  _alloc = 0;
  _bound = false;

  _impl_data = false;
  _force_impl = false;
  _watch_lists = false;
  _watch_dicts = false;
}

MetaClass::~MetaClass() {
  for (MemberList::iterator iter = _members.begin(); iter != _members.end(); ++iter)
    delete iter->second.property;
}

MetaClass *MetaClass::create_base_class() {
  MetaClass *mc = new MetaClass;
  mc->_name = internal::Object::static_class_name();
  mc->_placeholder = false;
  mc->bind_allocator(0);
  return mc;
}

MetaClass *MetaClass::from_xml(const std::string &source, xmlNodePtr node) {
  std::string name = get_prop(node, "name");
  MetaClass *stru;

  if (!name.empty()) {
    stru = grt::GRT::get()->get_metaclass(name);
    if (stru) {
      if (!stru->_placeholder)
        throw std::runtime_error(
          std::string("Error loading struct from ").append(source).append(": duplicate struct name ").append(name));
      stru->_placeholder = false;
    } else {
      stru = new MetaClass;
      logDebug3("Creating metaclass %s, from source: %s\n", name.c_str(), source.c_str());
    }
  } else
    throw std::runtime_error("Invalid struct definition in " + source);

  stru->_source = source;
  stru->load_xml(node);

  return stru;
}

void MetaClass::load_attribute_list(xmlNodePtr node, const std::string &member) {
  if (node->properties) {
    xmlAttr *attr = node->properties;
    std::string prefix;

    prefix.append(member);
    if (!prefix.empty())
      prefix.append(":");

    while (attr) {
      if (attr->ns && xmlStrcmp(attr->ns->prefix, (xmlChar *)"attr") == 0) {
        xmlChar *prop = xmlGetNsProp(node, attr->name, attr->ns->href);
        _attributes[prefix + (char *)attr->name] = (char *)prop;
        xmlFree(prop);
      }
      attr = attr->next;
    }
  }
}

void MetaClass::load_xml(xmlNodePtr node) {
  std::string node_property = get_prop(node, "name");
  xmlNodePtr child_node;

  if (xmlStrcmp(node->name, (xmlChar *)"gstruct") != 0) {
    logWarning("[XML parser] Node '%s': 'gstruct' expected.\n", node->name);
    throw std::runtime_error("missing 'metaclass' loading grt xml");
  }

  if (node_property.empty()) {
    logWarning("[XML parser] Node '%s' does not have a name property.\n", node->name);
    throw std::runtime_error("missing 'name' loading grt xml");
  }

  _name = node_property;

  if (get_prop(node, "force-impl") == "1")
    _force_impl = true;

  if (get_prop(node, "watch-lists") == "1")
    _watch_lists = true;

  if (get_prop(node, "watch-dicts") == "1")
    _watch_dicts = true;

  if (get_prop(node, "impl-data") == "1")
    _impl_data = true;

  node_property = get_prop(node, "parent");
  if (node_property.empty())
    node_property = internal::Object::static_class_name();

  MetaClass *parent = grt::GRT::get()->get_metaclass(node_property);
  if (parent)
    _parent = parent;
  else {
    // if the parent is not loaded yet, create a placeholder object to be filled later
    MetaClass *tmp = new MetaClass;
    tmp->_name = node_property;
    tmp->_source = _source;
    tmp->_placeholder = true;
    _parent = tmp;
    grt::GRT::get()->add_metaclass(tmp);
    logDebug3("Creating metaclass placeholder %s", node_property.c_str());
  }

  load_attribute_list(node);

  // read in members and child_structs
  child_node = node->children;
  while (child_node) {
    // members
    if (xmlStrcmp(child_node->name, (xmlChar *)"members") == 0) {
      xmlNodePtr member_node = child_node->children;

      while (member_node) {
        if (xmlStrcmp(member_node->name, (xmlChar *)"member") == 0) {
          Member member;
          member.read_only = false;
          member.delegate_get = false;
          member.delegate_set = false;
          member.private_ = false;
          member.calculated = false;
          member.owned_object = false;
          member.overrides = false;
          member.null_content_allowed = true;

          member.property = 0;

          std::string type = get_prop(member_node, "type");

          member.name = get_prop(member_node, "name");
          member.default_value = get_prop(member_node, "default");

          if (!get_prop(member_node, "dontfollow").empty())
            logWarning("[XML parser] Node '%s' contains 'attr:dontfollow' prop which was replaced with 'owned'\n.",
                      _name.c_str());

          if (get_prop(member_node, "read-only") == "1")
            member.read_only = true;
          else
            member.read_only = false;
          if (get_prop(member_node, "private") == "1")
            member.private_ = true;
          else
            member.private_ = false;
          if (get_prop(member_node, "delegate-get") == "1")
            member.delegate_get = true;
          else
            member.delegate_get = false;
          if (get_prop(member_node, "delegate-set") == "1")
            member.delegate_set = true;
          else
            member.delegate_set = false;
          if (get_prop(member_node, "calculated") == "1")
            member.calculated = true;
          else
            member.calculated = false;
          if (get_prop(member_node, "owned") == "1")
            member.owned_object = true;
          else
            member.owned_object = false;
          if (get_prop(member_node, "overrides") != "" && get_prop(member_node, "overrides") != "0")
            member.overrides = true;
          else
            member.overrides = false;
          if (get_prop(member_node, "allow-null") == "1")
            member.null_content_allowed = true;
          else
            member.null_content_allowed = false;

          load_attribute_list(member_node, member.name);

          // don't override the whole member, only the attributes
          if (get_prop(member_node, "override-attributes-only") != "1") {
            if (!get_type_spec(member_node, member.type))
              logWarning("[XML parser] Node '%s'::'%s' contains invalid type specification.\n", _name.c_str(),
                        member.name.c_str());

            // do some validation
            if (!is_container_type(member.type.base.type) && member.owned_object)
              logWarning("[XML parser] Node '%s'::'%s' marked as 'owned', but is not an object.\n", _name.c_str(),
                        member.name.c_str());

            if (member.calculated && (!member.delegate_get || (!member.read_only && !member.delegate_set)))
              logWarning("[XML parser] Node '%s'::'%s' marked as 'calculated', but accessors are not delegated.\n",
                        _name.c_str(), member.name.c_str());

            if (member.calculated && member.private_)
              logWarning("[XML parser] Node '%s'::'%s' marked as 'private' and 'calculated', which is not allowed.\n",
                        _name.c_str(), member.name.c_str());

            if (member.calculated && member.owned_object)
              logWarning("[XML parser] Node '%s'::'%s' marked as 'owned' and 'calculated', which is not allowed.\n",
                        _name.c_str(), member.name.c_str());

            // can't replace lists/dicts members in objects
            if ((member.type.base.type == ListType) || (member.type.base.type == DictType))
              member.read_only = true;

            _members[member.name] = member;
          }
        } else if (xmlStrcmp(member_node->name, (xmlChar *)"method") == 0 ||
                   xmlStrcmp(member_node->name, (xmlChar *)"constructor") == 0) {
          Method method;

          method.constructor = xmlStrcmp(member_node->name, (xmlChar *)"constructor") == 0;
          method.abstract = false;

          method.name = get_prop(member_node, "name");

          if (get_prop(member_node, "constructor") == "1")
            method.constructor = true;
          if (get_prop(member_node, "abstract") == "1")
            method.abstract = true;

          if (method.constructor && method.abstract)
            logWarning("[XML parser] Node '%s'::'%s' cannot be both abstract and constructor.\n", _name.c_str(),
                      method.name.c_str());

          int return_node_count = 0;
          xmlNodePtr arg_node = member_node->children;
          while (arg_node) {
            if (xmlStrcmp(arg_node->name, (xmlChar *)"argument") == 0) {
              ArgSpec arg;

              arg.name = get_prop(arg_node, "name");
              if (!get_type_spec(arg_node, arg.type))
                logWarning("[XML parser] Node '%s'::'%s'::'%s' contains invalid argument type specification\n",
                          _name.c_str(), method.name.c_str(), arg.name.c_str());

              method.arg_types.push_back(arg);

              load_attribute_list(arg_node, method.name + ":" + arg.name);
            } else if (xmlStrcmp(arg_node->name, (xmlChar *)"return") == 0) {
              return_node_count++;

              if (method.constructor || !get_type_spec(arg_node, method.ret_type, true))
                logWarning("[XML parser] Node '%s'::'%s' contains invalid type specification.\n", _name.c_str(),
                          method.name.c_str());

              load_attribute_list(arg_node, method.name + ":return");
            }
            arg_node = arg_node->next;
          }
          if (return_node_count != 1 && !method.constructor)
            logWarning("[XML parser] Node '%s'::'%s' has %i return value specifications\n", _name.c_str(),
                      method.name.c_str(), return_node_count);

          load_attribute_list(member_node, method.name);

          _methods[method.name] = method;
        } else if (xmlStrcmp(member_node->name, (xmlChar *)"signal") == 0) {
          Signal sig;

          sig.name = get_prop(member_node, "name");

          xmlNodePtr arg_node = member_node->children;
          while (arg_node) {
            if (xmlStrcmp(arg_node->name, (xmlChar *)"argument") == 0) {
              SignalArg arg;
              std::string type = get_prop(arg_node, "type");

              if (type == "bool")
                arg.type = BoolSArg;
              else if (type == "int")
                arg.type = IntSArg;
              else if (type == "double")
                arg.type = DoubleSArg;
              else if (type == "string")
                arg.type = StringSArg;
              else if (type == "object") {
                arg.type = ObjectSArg;
                arg.object_class = get_prop(arg_node, "struct-name");
              } else
                logWarning("Signal '%s'::'%s' contains invalid argument type '%s'\n", _name.c_str(), sig.name.c_str(),
                          type.c_str());

              sig.arg_types.push_back(arg);
            }
            arg_node = arg_node->next;
          }

          _signals.push_back(sig);
        }
        member_node = member_node->next;
      }
    }

    child_node = child_node->next;
  }

  _crc32 = make_checksum(this);
}

bool MetaClass::is_bound() const {
  return _bound;
}

bool MetaClass::validate() {
  std::map<std::string, std::string> seen;
  bool ok = true;

  for (MemberList::const_iterator mem = _members.begin(); mem != _members.end(); ++mem) {
    const Member *member;

    // check if the member is overriding another one
    if (_parent && (member = _parent->get_member_info(mem->second.name))) {
      if (member->type.base.type != mem->second.type.base.type) {
        logWarning("Member %s::%s overrides a member with a different base type\n", _name.c_str(),
                  mem->second.name.c_str());
        ok = false;
      } else {
        // check if the overriding type is compatible
        switch (member->type.base.type) {
          case ListType:
          case DictType:
            if (member->type.content.type != mem->second.type.content.type) {
              logWarning("Member %s::%s overrides a member with a different content type\n", _name.c_str(),
                        mem->second.name.c_str());
              ok = false;
            }
            if (member->type.content.type == ObjectType) {
              MetaClass *member_content_class1;
              MetaClass *member_content_class2;

              if (!(member_content_class1 = grt::GRT::get()->get_metaclass(mem->second.type.content.object_class))) {
                logWarning("Member %s::%s has invalid content object class '%s'\n", _name.c_str(),
                          mem->second.name.c_str(), mem->second.type.content.object_class.c_str());
                ok = false;
              }

              member_content_class2 = grt::GRT::get()->get_metaclass(member->type.content.object_class);
              if (member_content_class1 && member_content_class2) {
                if (!member_content_class1->is_a(member_content_class2)) {
                  logWarning("Member %s::%s overrides a member with an incompatible content object class\n", _name.c_str(),
                            mem->second.name.c_str());
                  ok = false;
                }
              }
            }
            break;
          case ObjectType:
            if (member->type.content.object_class != mem->second.type.content.object_class) {
              logWarning("Member %s::%s overrides a member with a different class\n", _name.c_str(),
                        mem->second.name.c_str());
              ok = false;
            }
            break;
          default:
            break;
        }
      }
      if (ok) {
        // mark it as overrides
        _members[mem->first].overrides = true;
      }
    }

    if (seen.find(mem->second.name) != seen.end() && !mem->second.overrides) {
      logWarning("Member %s::%s is duplicate\n", _name.c_str(), mem->second.name.c_str());
      ok = false;
    }
    seen[mem->second.name] = _name;
  }

  return ok;
}

std::string MetaClass::get_attribute(const std::string &attr, bool search_parents) {
  MetaClass *root = this;

  std::unordered_map<std::string, std::string>::const_iterator iter;
  do {
    iter = root->_attributes.find(attr);
    if (iter != root->_attributes.end())
      return iter->second;
    if (root->_parent && search_parents)
      root = root->_parent;
    else
      return "";
  } while (root);

  return "";
}

std::string MetaClass::get_member_attribute(const std::string &member, const std::string &attr, bool search_parents) {
  MetaClass *root = this;
  const std::string search_string = member + ":" + attr;

  std::unordered_map<std::string, std::string>::const_iterator iter;
  do {
    iter = root->_attributes.find(search_string);
    if (iter != root->_attributes.end())
      return iter->second;
    if (root->_parent && search_parents)
      root = root->_parent;
    else
      return "";
  } while (root);

  return "";
}

bool MetaClass::is_abstract() const {
  if (_bound && !_alloc)
    return true;

  for (MethodList::const_iterator iter = _methods.begin(); iter != _methods.end(); ++iter) {
    if (iter->second.abstract)
      return true;
  }

  return false;
}

ObjectRef MetaClass::allocate() {
  if (is_abstract())
    throw std::runtime_error("cannot allocate an abstract class");

  if (!_bound)
    throw std::runtime_error("GRT class " + name() + " was not initialized/registered with the GRT instance");

  ObjectRef object = (*_alloc)();
  object->init();

  return object;
}

void MetaClass::bind_allocator(Allocator alloc) {
  _alloc = alloc;
  _bound = true;
}

void MetaClass::bind_member(const std::string &name, PropertyBase *prop) {
  std::map<std::string, Member>::iterator iter = _members.find(name);
  if (iter == _members.end())
    throw std::runtime_error("Attempt to bind invalid member " + name);

  iter->second.property = prop;
}

void MetaClass::bind_method(const std::string &name, Method::Function method) {
  std::map<std::string, Method>::iterator iter = _methods.find(name);
  if (iter == _methods.end())
    throw std::runtime_error("Attempt to bind invalid method " + name);

  iter->second.function = method;
}

void MetaClass::add_validator(Validator *v) {
  if (v && _validators.end() == std::find(_validators.begin(), _validators.end(), v))
    _validators.push_back(v);
}

void MetaClass::remove_validator(Validator *v) {
  throw std::logic_error("void MetaClass::del_validator(Validator* v) not implemented!");
}

void MetaClass::set_member_value(internal::Object *object, const std::string &name, const ValueRef &value) {
  set_member_internal(object, name, value, false);
}

void MetaClass::set_member_internal(internal::Object *object, const std::string &name, const ValueRef &value,
                                    bool force) {
  MetaClass *mc = this;
  MemberList::const_iterator mem, end;
  bool found = false;
  do {
    mem = mc->_members.find(name);
    end = mc->_members.end();

    if (mem != end)
      found = true;

    mc = mc->_parent;
  } while (mc && (mem == end || mem->second.overrides == true || !mem->second.property->has_setter()));

  if (mem == end) {
    if (found)
      throw grt::read_only_item(_name + "." + name);
    else
      throw bad_item(_name + "." + name);
  }

  if (mem->second.read_only && !force) {
    if (mem->second.type.base.type == ListType || mem->second.type.base.type == DictType)
      throw grt::read_only_item(_name + "." + name + " (which is a container)");
    throw grt::read_only_item(_name + "." + name);
  }
  mem->second.property->set(object, value);
}

ValueRef MetaClass::get_member_value(const internal::Object *object, const std::string &name) {
  MetaClass *mc = this;
  MemberList::const_iterator mem, end;
  do {
    mem = mc->_members.find(name);
    end = mc->_members.end();

    mc = mc->_parent;
  } while (mc && (mem == end || mem->second.overrides));

  if (mem == end || mem->second.property == NULL)
    throw bad_item(name);

  return mem->second.property->get(object);
}

ValueRef MetaClass::get_member_value(const internal::Object *object, const MetaClass::Member *member) {
  return member->property->get(object);
}

ValueRef MetaClass::call_method(internal::Object *object, const std::string &name, const BaseListRef &args) {
  MetaClass *mc = this;
  MethodList::const_iterator mem, end;
  do {
    mem = mc->_methods.find(name);
    end = mc->_methods.end();

    mc = mc->_parent;
  } while (mc && mem == end);

  if (mem == end)
    throw grt::bad_item(name);

  return (*mem->second.function)(object, args);
}

ValueRef MetaClass::call_method(internal::Object *object, const Method *method, const BaseListRef &args) {
  return (*method->function)(object, args);
}

const MetaClass::Member *MetaClass::get_member_info(const std::string &member) const {
  const MetaClass *mc = this;
  MemberList::const_iterator mem, end;
  do {
    mem = mc->_members.find(member);
    end = mc->_members.end();

    mc = mc->_parent;
  } while (mc && mem == end);

  if (mem == end)
    return 0;
  return &mem->second;
}

const MetaClass::Method *MetaClass::get_method_info(const std::string &method) const {
  const MetaClass *mc = this;
  MethodList::const_iterator mem, end;
  do {
    mem = mc->_methods.find(method);
    end = mc->_methods.end();

    mc = mc->_parent;
  } while (mc && mem == end);

  if (mem == end)
    return 0;
  return &mem->second;
}

TypeSpec MetaClass::get_member_type(const std::string &member) const {
  const Member *mem = get_member_info(member);
  if (!mem)
    throw bad_item(member);

  return mem->type;
}
