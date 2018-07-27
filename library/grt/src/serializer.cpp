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

#include "serializer.h"

#include "grtpp_util.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <glib.h>

#include "base/log.h"
#include "base/file_functions.h"

#define GRT_FILE_VERSION_TAG "grt_format"
#define GRT_FILE_VERSION "2.0"

#define new_node(node, name, value) xmlNewTextChild(node, NULL, (xmlChar *)name, (xmlChar *)value)

DEFAULT_LOG_DOMAIN("serializer")

static xmlNodePtr new_int_node(xmlNodePtr node, const char *name, int value) {
  char buffer[32];

  g_snprintf(buffer, sizeof(buffer), "%i", value);

  return new_node(node, name, buffer);
}

#define set_prop(node, name, value) xmlNewProp(node, (xmlChar *)name, (xmlChar *)value)

using namespace grt;
using namespace grt::internal;

xmlDocPtr internal::Serializer::create_xmldoc_for_value(const ValueRef &value, const std::string &doctype,
                                                        const std::string &docversion, bool list_objects_as_links) {
  xmlDocPtr doc;

  doc = xmlNewDoc((xmlChar *)"1.0");
  doc->children = xmlNewDocRawNode(doc, NULL, (xmlChar *)"data", NULL);

  xmlNewProp(doc->children, (xmlChar *)GRT_FILE_VERSION_TAG, (xmlChar *)GRT_FILE_VERSION);

  if (!doctype.empty())
    set_prop(doc->children, "document_type", doctype.c_str());
  if (!docversion.empty())
    set_prop(doc->children, "version", docversion.c_str());

  serialize_value(value, doc->children, list_objects_as_links);

  return doc;
}

internal::Serializer::Serializer() {
}

static int base_xmlSaveFile(const char *filename, xmlDocPtr doc) {
  char *local_filename;
  int result;
  FILE *file;

  if ((local_filename = g_filename_from_utf8(filename, -1, NULL, NULL, NULL)) == NULL)
    return -1;

  // Check if the file already exists and if so store under a temporary name first.
  file = base_fopen(local_filename, "r");

  if (file != NULL) {
    char *tempName = g_strdup_printf("%s.tmp", local_filename);

    result = xmlSaveFormatFile(tempName, doc, 1);
    fclose(file);

    if (result > 0) {
      // If saving the content was successful then delete the old file and use the new one.
      base_remove(local_filename);
      base_rename(tempName, local_filename);
    };
    g_free(tempName);
  } else
    result = xmlSaveFormatFile(local_filename, doc, 1);

  g_free(local_filename);

  return result;
}

/**
 ****************************************************************************
 * @brief Stores a GRT value to a file
 *
 *   This will serialize the value to XML and store it in a file that can
 * later be retrieved with base_grt_retrieve_from_file.
 * NOTE: This function is not reentrant.
 *
 * @param value the GRT value to store
 * @param filename name of file to store data
 * @param type document format type
 * @param version version of document format
 *
 ****************************************************************************/
void internal::Serializer::save_to_xml(const ValueRef &value, const std::string &path, const std::string &doctype,
                                       const std::string &docversion, bool list_objects_as_links) {
  xmlDocPtr doc;

  doc = create_xmldoc_for_value(value, doctype, docversion, list_objects_as_links);

  if (base_xmlSaveFile(path.c_str(), doc) == -1) {
    xmlFreeDoc(doc);
    throw std::runtime_error("Could not save XML data to file " + path);
  }
  xmlFreeDoc(doc);
}

bool internal::Serializer::seen(const ValueRef &value) {
  void *ptr = value.valueptr();

  if (_cache.find(ptr) != _cache.end())
    return true;

  // value is not yet in XML tree
  _cache.insert(ptr);

  return false;
}

/*
 *****************************************************************************
 * @brief Encodes a GRT value (and its sub-values) to a XML tree. (internal)
 *
 * The implementation will store simple values directly and complex values
 * (lists, dicts and objects) directly at the first reference and as
 * links on further ones.
 *
 * @param grt
 * @param parent the parent node where this value should be attached to
 * @param value the value to serialize
 * @param list_objects_as_links if this is true, the value is saved as a link
 *
 *****************************************************************************
 */
xmlNodePtr internal::Serializer::serialize_value(const ValueRef &value, xmlNodePtr parent, bool list_objects_as_links) {
  char buffer[100];
  xmlNodePtr node = NULL;

  switch (value.type()) {
    case IntegerType:
      node = new_int_node(parent, "value", (int)*IntegerRef::cast_from(value));

      set_prop(node, "type", "int");
      break;
    case DoubleType: {
      node = new_node(parent, "value", base::to_string(*DoubleRef::cast_from(value)).c_str());
      set_prop(node, "type", "real");
      break;
    }
    case StringType:
      node = new_node(parent, "value", StringRef::cast_from(value).c_str());

      set_prop(node, "type", "string");
      break;
    case ListType: {
      BaseListRef list(BaseListRef::cast_from(value));

      if (seen(value)) {
        logDebug3("found duplicate list value");
        g_snprintf(buffer, sizeof(buffer), "%p", list.valueptr());
        node = new_node(parent, "link", buffer);
        set_prop(node, "type", "list");
        return node;
      }

      node = new_node(parent, "value", NULL);

      g_snprintf(buffer, sizeof(buffer), "%p", list.valueptr());
      set_prop(node, "_ptr_", buffer);
      set_prop(node, "type", "list");
      set_prop(node, "content-type", type_to_str(list.content_type()).c_str());

      if (!list.content_class_name().empty())
        set_prop(node, "content-struct-name", list.content_class_name().c_str());

      // check if the list is part of a struct and has no 'owned' set,
      // it should serialize the contents as links
      for (size_t c = list.count(), i = 0; i < c; i++) {
        ValueRef cvalue(list.get(i));

        if (cvalue.is_valid()) {
          if (list_objects_as_links && cvalue.type() == ObjectType) {
            xmlNodePtr child = new_node(node, "link", ObjectRef::cast_from(cvalue).id().c_str());
            set_prop(child, "type", "object");
          } else
            serialize_value(cvalue, node, false);
        } else {
          new_node(node, "null", NULL);
        }
      }
      break;
    }

    case DictType: {
      DictRef dict(DictRef::cast_from(value));

      if (seen(value)) {
        g_snprintf(buffer, sizeof(buffer), "%p", value.valueptr());
        node = new_node(parent, "link", buffer);
        set_prop(node, "type", "dict");
        return node;
      }

      node = new_node(parent, "value", NULL);

      g_snprintf(buffer, sizeof(buffer), "%p", value.valueptr());
      set_prop(node, "_ptr_", buffer);
      set_prop(node, "type", "dict");

      for (Dict::const_iterator iter = dict.begin(); iter != dict.end(); ++iter) {
        xmlNodePtr child;
        std::string k(iter->first);
        ValueRef v(iter->second);

        if (v.is_valid()) {
          child = serialize_value(v, node, false);
          set_prop(child, "key", k.c_str());
        }
      }
    } break;

    case ObjectType: {
      ObjectRef object(ObjectRef::cast_from(value));

      if (!seen(object)) // owned_objects)
        node = serialize_object(object, parent);
      else {
        node = new_node(parent, "link", object->id().c_str());
        if (node) {
          set_prop(node, "type", "object");
          set_prop(node, "struct-name", object->class_name().c_str());
        }
        return node;
      }
      break;
    }

    case UnknownType:
      break;
  }
  return node;
}

bool internal::Serializer::serialize_member(const MetaClass::Member *member, const ObjectRef &object, xmlNodePtr node) {
  std::string k = member->name;
  ValueRef v;

  // don't serialize calculated values
  if (member->calculated)
    return true;

  xmlNodePtr child;

  v = object->get_member(k);

  if (v.is_valid()) {
    // if 'owned' for this member is not set to 1, then we just dump
    // a link instead of the whole object
    bool owned = member->owned_object;

    // 'owned' can be set for objects or lists,
    // if its set on a list the *contents* will be saved as links
    if (!owned && v.type() == ObjectType) {
      // dontfollow is set in the struct, so just skip this member
      //
      child = new_node(node, "link", ObjectRef::cast_from(v)->id().c_str());
      set_prop(child, "type", "object");
      set_prop(child, "struct-name", member->type.base.object_class.c_str());
    } else
      child = serialize_value(v, node, !owned);
    set_prop(child, "key", k.c_str());
  }
  return true;
}

xmlNodePtr internal::Serializer::serialize_object(const ObjectRef &object, xmlNodePtr parent) {
  xmlNodePtr node;
  char checksum[40];

  node = new_node(parent, "value", NULL);
  set_prop(node, "type", "object");
  set_prop(node, "struct-name", object->class_name().c_str());
  set_prop(node, "id", object->id().c_str());

  g_snprintf(checksum, sizeof(checksum), "0x%x", object.get_metaclass()->crc32());

  set_prop(node, "struct-checksum", checksum);

  MetaClass *stru = object->get_metaclass();

  stru->foreach_member(std::bind(&Serializer::serialize_member, this, std::placeholders::_1, object, node));

  return node;
}

std::string internal::Serializer::serialize_to_xmldata(const ValueRef &value, const std::string &type,
                                                       const std::string &version, bool list_objects_as_links) {
  xmlDocPtr doc;
  xmlChar *buffer = NULL;
  int size;

  if (value.is_valid()) {
    std::string tmp;

    doc = create_xmldoc_for_value(value, type, version, list_objects_as_links);

    xmlDocDumpFormatMemory(doc, &buffer, &size, 1);

    xmlFreeDoc(doc);
    tmp = (char *)buffer;

    xmlFree(buffer);

    return tmp;
  } else
    return "";
}
