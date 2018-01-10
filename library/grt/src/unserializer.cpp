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

#include "unserializer.h"

#include "grtpp_util.h"

#include "base/string_utilities.h"
#include "base/log.h"
#include "base/xml_functions.h"

DEFAULT_LOG_DOMAIN(DOMAIN_GRT)

using namespace grt;
using namespace grt::internal;

internal::Unserializer::Unserializer(bool check_crc) : _check_serialized_crc(check_crc) {
}

ValueRef internal::Unserializer::find_cached(const std::string &id) {
  std::map<std::string, ValueRef>::const_iterator iter;
  if ((iter = _cache.find(id)) == _cache.end())
    return ValueRef();

  return iter->second;
}

ValueRef internal::Unserializer::load_from_xml(const std::string &path, std::string *doctype, std::string *docversion) {
  xmlDocPtr doc = base::xml::loadXMLDoc(path);

  ValueRef value = unserialize_xmldoc(doc, path);

  if (doctype && docversion)
    base::xml::getXMLDocMetainfo(doc, *doctype, *docversion);

  xmlFreeDoc(doc);

  return value;
}

ValueRef internal::Unserializer::unserialize_xmldoc(xmlDocPtr doc, const std::string &source_path) {
  xmlNodePtr root;
  ValueRef value;

  _source_name = source_path;

  root = xmlDocGetRootElement(doc);
  if (root) {
    root = root->children;

    while (root && xmlStrcmp(root->name, (xmlChar *)"value") != 0)
      root = root->next;
    if (root)
      value = unserialize_from_xml(root);
  }

  return value;
}

ValueRef internal::Unserializer::unserialize_from_xml(xmlNodePtr node) {
  traverse_xml_creating_objects(node);

  return traverse_xml_recreating_tree(node);
}

void internal::Unserializer::traverse_xml_creating_objects(xmlNodePtr node) {
  xmlNodePtr child;
  std::string prop;

  if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (xmlChar *)"value") != 0)
    return;

  prop = base::xml::getProp(node, "type");
  if (prop.empty())
    throw std::runtime_error(
      std::string("Node ").append((char *)node->name).append(" in xml doesn't have a type property"));

  switch (str_to_type(prop)) {
    case ListType:
    case DictType:
      child = node->children;
      while (child) {
        traverse_xml_creating_objects(child);
        child = child->next;
      }
      break;

    case ObjectType: {
      ObjectRef object = unserialize_object_step1(node);
      if (object.is_valid())
        _cache[object->id()] = object;

      child = node->children;
      while (child) {
        traverse_xml_creating_objects(child);
        child = child->next;
      }
    } break;
    default:
      break;
  }
}

ValueRef internal::Unserializer::traverse_xml_recreating_tree(xmlNodePtr node) {
  if (strcmp((char *)node->name, "link") == 0) {
    std::string link_id;

    // this is a link instead of a value, look up for the original value and
    // return it
    link_id = base::xml::getContent(node);
    ValueRef value = find_cached(link_id);

    if (!value.is_valid() && (_invalid_cache.find(link_id) == _invalid_cache.end())) {
      // if link is not object, then quit
      std::string node_type = base::xml::getProp(node, "type");
      if (node_type.empty() || node_type != "object") {
        logWarning("%s: link of type '%s' could not be resolved during unserialized", _source_name.c_str(),
                   node_type.c_str());
        return ValueRef();
      }

      // we have looked up already
      // check if the object was loaded in the 1st step

      // if the linked object is not in the current tree, look for it in the global tree
      ObjectRef object(grt::GRT::get()->find_object_by_id(link_id, "/"));

      if (object.is_valid())
        _cache[object->id()] = object;
      else
        _invalid_cache.insert(link_id);
      value = object;

      if (!value.is_valid() /*&& base::xml::getProp(node, "key") != "owner"*/)
        logWarning("%s:%i: link '%s' <%s %s> key=%s could not be resolved\n", _source_name.c_str(), node->line,
                   link_id.c_str(), base::xml::getProp(node, "type").c_str(),
                   base::xml::getProp(node, "struct-name").c_str(), base::xml::getProp(node, "key").c_str());
    }

    return value;
  } else if (strcmp((char *)node->name, "value") != 0)
    return ValueRef();

  std::string node_type = base::xml::getProp(node, "type");
  if (node_type.empty())
    throw std::runtime_error(
      std::string("Node '").append((char *)node->name).append("' in xml doesn't have a type property"));

  Type vtype = str_to_type(node_type);
  ValueRef value;

  switch (vtype) {
    case IntegerType:
      value = IntegerRef(strtol((char *)base::xml::getContent(node).c_str(), NULL, 0));
      break;

    case DoubleType: {
      std::string tmp = base::xml::getContent(node);
      value = DoubleRef(base::atof<double>(tmp));
      break;
    }

    case StringType:
      value = StringRef(base::xml::getContent(node));
      break;

    case DictType: {
      std::string ptr;
      DictRef dict;

      // check if the dictionary was already created
      ptr = base::xml::getProp(node, "_ptr_");
      if (!ptr.empty())
        value = find_cached(ptr);

      if (!value.is_valid()) {
        std::string prop = base::xml::getProp(node, "content-type");
        if (!prop.empty()) {
          Type content_type = str_to_type(prop);
          if (content_type != UnknownType) {
            std::string content_class_name = base::xml::getProp(node, "content-struct-name");

            value = dict = DictRef(content_type, content_class_name);
          } else
            throw std::runtime_error("Error parsing XML. Invalid type " + prop);
        } else
          value = dict = DictRef(true);

        if (!ptr.empty())
          _cache[ptr] = value;
      } else
        dict = DictRef::cast_from(value);

      xmlNodePtr child = node->children;
      while (child) {
        if (child->type == XML_ELEMENT_NODE) {
          std::string key = base::xml::getProp(child, "key");
          if (!key.empty()) {
            ValueRef sub_value = traverse_xml_recreating_tree(child);
            dict.set(key, sub_value);
          }
        }
        child = child->next;
      }
      break;
    }

    case ListType: {
      Type content_type = str_to_type(base::xml::getProp(node, "content-type"));
      std::string cclass_name = base::xml::getProp(node, "content-struct-name");
      xmlNodePtr child;
      std::string prop;
      BaseListRef list;

      prop = base::xml::getProp(node, "_ptr_");

      if (!prop.empty()) {
        // look up for this ptr, in case the owner object already has created this list
        value = find_cached(prop);
        if (!value.is_valid()) {
          value = list = BaseListRef(content_type, cclass_name);

          _cache[prop] = value;
        } else
          list = BaseListRef::cast_from(value);
      } else
        value = list = BaseListRef(content_type, cclass_name);

      child = node->children;
      while (child) {
        ValueRef sub_value;

        if (child->type == XML_ELEMENT_NODE) {
          if (xmlStrcmp(child->name, (xmlChar *)"null") == 0) {
            if (!list->null_allowed()) {
              logWarning("%s: Attempt o add null value to %s list", _source_name.c_str(), cclass_name.c_str());
            }
            list.ginsert(ValueRef());
          } else {
            sub_value = traverse_xml_recreating_tree(child);

            if (sub_value.is_valid()) {
              try {
                list.ginsert(sub_value);
              } catch (const std::exception &exc) {
                logWarning("%s: Error inserting %s to list: %s", _source_name.c_str(),
                           sub_value.debugDescription().c_str(), exc.what());
                throw;
              }
            } else {
              // error!
              logWarning("%s: skipping element '%s' in unserialized document, line %i", _source_name.c_str(),
                         child->name, child->line);
              value.clear();
              break;
            }
          }
        }
        child = child->next;
      }
      break;
    }

    case ObjectType:
      // unserialize and initialize the object
      value = unserialize_object_step2(node);
      break;

    case UnknownType:
      break;
  }

  return value;
}

ObjectRef internal::Unserializer::unserialize_object_step1(xmlNodePtr node) {
  MetaClass *gstruct;
  std::string id;

  std::string prop = base::xml::getProp(node, "type");
  if (prop != "object")
    throw std::runtime_error("error unserializing object (unexpected type)");

  prop = base::xml::getProp(node, "struct-name");
  if (prop.empty())
    throw std::runtime_error("error unserializing object (missing struct-name)");

  gstruct = grt::GRT::get()->get_metaclass(prop);
  if (!gstruct) {
    logWarning("%s:%i: error unserializing object: struct '%s' unknown", _source_name.c_str(), node->line,
               prop.c_str());
    throw std::runtime_error(base::strfmt("error unserializing object (struct '%s' unknown)", prop.c_str()));
  }

  id = base::xml::getProp(node, "id");
  if (id.empty())
    throw std::runtime_error("missing id in unserialized object");

  prop = base::xml::getProp(node, "struct-checksum");
  if (!prop.empty()) {
    unsigned int checksum = (unsigned int)strtol(prop.c_str(), NULL, 0);
    if (_check_serialized_crc && checksum != gstruct->crc32()) {
      logWarning("current checksum of struct of serialized object %s (%s) differs from the one when it was saved",
                 id.c_str(), gstruct->name().c_str());
    }
  }

  ObjectRef value = gstruct->allocate();
  value->__set_id(id);

  return value;
}

ObjectRef internal::Unserializer::unserialize_object_step2(xmlNodePtr node) {
  std::string id = base::xml::getProp(node, "id");

  if (id.empty())
    throw std::runtime_error(std::string("missing id property unserializing node ").append((char *)node->name));

  ObjectRef value = ObjectRef::cast_from(find_cached(id));
  if (!value.is_valid())
    logWarning("%s: Unknown object-id '%s' in unserialized file", _source_name.c_str(), id.c_str());
  unserialize_object_contents(value, node);

  return value;
}

void internal::Unserializer::unserialize_object_contents(const ObjectRef &object, xmlNodePtr node) {
  std::string prop;
  // load values
  xmlNodePtr child;
  MetaClass *mc = object->get_metaclass();

  child = node->children;
  while (child) {
    ValueRef sub_value;

    if (child->type == XML_ELEMENT_NODE) {
      std::string key = base::xml::getProp(child, "key");

      if (!key.empty()) {
        if (!object->has_member(key)) {
          logWarning(
            "in %s: %s", object.id().c_str(),
            std::string("unserialized XML contains invalid member " + object.class_name() + "::" + key).c_str());
          // throw std::runtime_error("unserialized XML contains invalid member "+object.class_name()+"::"+key);
        } else {
          // 1st check if the value is a container and if it has already been created
          // if so, insert it to the unserialize cache for reuse by base_grt_traverse_xml_recreating_tree
          sub_value = object->get_member(key);
          if (sub_value.is_valid()) {
            std::string ptr = base::xml::getProp(child, "_ptr_");
            if (!ptr.empty())
              _cache[ptr] = sub_value;
          }

          // unpack the value (and contents)
          try {
            sub_value = traverse_xml_recreating_tree(child);
          } catch (grt::null_value &exc) {
            logWarning("%s in %s:%s %s", exc.what(), object->class_name().c_str(), key.c_str(), object->id().c_str());
            throw;
          }
          if (sub_value.is_valid()) {
            try {
              mc->set_member_internal((internal::Object *)object.valueptr(), key, sub_value, true);
            } catch (const std::exception &exc) {
              logWarning("exception setting %s<%s>:%s to %s %s", object.id().c_str(), object.class_name().c_str(),
                         key.c_str(), sub_value.debugDescription().c_str(), exc.what());
              throw;
            }
          }
        }
      }
    }
    child = child->next;
  }
}

ValueRef internal::Unserializer::unserialize_xmldata(const char *data, size_t size) {
  xmlDocPtr doc = xmlReadMemory(data, (int)size, NULL, NULL, XML_PARSE_NOENT);

  if (!doc) {
    xmlErrorPtr error = xmlGetLastError();

    if (error)
      throw std::runtime_error(base::strfmt("Could not parse XML data. Line %d, %s", error->line, error->message));
    else
      throw std::runtime_error("Could not parse XML data");
  }

  ValueRef value = unserialize_xmldoc(doc);

  xmlFreeDoc(doc);

  return value;
}
