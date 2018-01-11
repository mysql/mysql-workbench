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

#include "grtpp_util.h"
#include "wb_model_file.h"
#include "upgrade_helper.h"
#include "base/string_utilities.h"
#include "base/log.h"

using namespace wb;
using namespace grt;
using namespace base;

DEFAULT_LOG_DOMAIN("upgrade_helper.cpp")

void XMLTraverser::cache_object_nodes(xmlNodePtr node) {
  if (node == NULL) {
    logError("XMLTraverser::cache_object_nodes node is NULL");
    return;
  }
  xmlNodePtr child = node->children;
  while (child) {
    if (strcmp((char *)child->name, "value") == 0) {
      if (node_prop(child, "type") == "object") {
        nodes_by_id[node_prop(child, "id")] = child;
      }
    }
    cache_object_nodes(child);
    child = child->next;
  }
}

XMLTraverser::XMLTraverser(xmlDocPtr adoc) : doc(adoc) {
  root = xmlDocGetRootElement(doc);

  cache_object_nodes(root);
}

xmlNodePtr XMLTraverser::get_root() {
  xmlNodePtr node = root->children;

  while (node) {
    if (strcmp((char *)node->name, "value") == 0)
      return node;
    node = node->next;
  }
  return NULL;
}

std::string XMLTraverser::node_prop(xmlNodePtr node, const char *prop) {
  xmlChar *s = xmlGetProp(node, (xmlChar *)prop);
  std::string str(s ? (char *)s : "");
  xmlFree(s);
  return str;
}

bool XMLTraverser::delete_object_item(xmlNodePtr objnode, const char *name) {
  xmlNodePtr item = objnode->children;
  bool found = false;

  while (item) {
    if (xmlStrcmp(item->name, (xmlChar *)"value") == 0) {
      if (node_prop(item, "key") == name) {
        found = true;

        xmlUnlinkNode(item);
        xmlFreeNode(item);
      }
      if (found)
        break;
    }

    item = item->next;
  }
  return found;
}

xmlNodePtr XMLTraverser::get_object(const char *id) {
  if (nodes_by_id.find(id) == nodes_by_id.end())
    return 0;
  return nodes_by_id[id];
}

xmlNodePtr XMLTraverser::get_object_by_path(const char *path) {
  gchar **parts = g_strsplit(path, "/", -1);
  xmlNodePtr node = get_root();

  for (int i = 1; parts[i] && node; i++) {
    bool isindex = true;
    for (int j = 0; parts[i][j]; j++) {
      if (!isdigit(parts[i][j])) {
        isindex = false;
        break;
      }
    }

    if (isindex)
      node = get_object_child_by_index(node, base::atoi<int>(parts[i], 0));
    else
      node = get_object_child(node, parts[i]);
  }
  g_strfreev(parts);

  return node;
}

xmlNodePtr XMLTraverser::get_object_child(xmlNodePtr object, const char *key) {
  xmlNodePtr child = object->children;
  while (child) {
    if (object->type == XML_ELEMENT_NODE) {
      if (strcmp((char *)child->name, "value") == 0) {
        if (node_prop(child, "key") == key)
          return child;
      } else if (strcmp((char *)child->name, "link") == 0) {
        if (node_prop(child, "key") == key) {
          xmlChar *content = xmlNodeGetContent(child);
          xmlNodePtr linked = get_object((char *)content);
          xmlFree(content);
          return linked;
        }
      }
    }
    child = child->next;
  }
  return NULL;
}

xmlNodePtr XMLTraverser::get_object_child_by_index(xmlNodePtr object, int index) {
  xmlNodePtr child = object->children;
  while (child) {
    if (child->type == XML_ELEMENT_NODE) {
      if (index == 0)
        break;
      index--;
    }
    child = child->next;
  }

  if (child) {
    if (strcmp((char *)child->name, "value") == 0) {
      return child;
    } else if (strcmp((char *)child->name, "link") == 0) {
      xmlChar *content = xmlNodeGetContent(child);
      xmlNodePtr linked = get_object((char *)content);
      xmlFree(content);
      return linked;
    }
  }
  return NULL;
}

double XMLTraverser::get_object_double_value(xmlNodePtr object, const char *key) {
  xmlNodePtr node = get_object_child(object, key);
  if (node) {
    xmlChar *content = xmlNodeGetContent(node);
    double value = atof((char *)content);
    xmlFree(content);
    return value;
  }
  return 0.0;
}

void XMLTraverser::set_object_child(xmlNodePtr object, const char *key, xmlNodePtr value) {
  value = xmlAddChild(object, value);

  xmlSetProp(value, (xmlChar *)"key", (xmlChar *)key);
}

void XMLTraverser::set_object_link(xmlNodePtr object, const char *key, xmlNodePtr target_object) {
  xmlNodePtr link;
  std::string target_id = node_prop(target_object, "id");
  std::string struct_name = node_prop(target_object, "struct-name");

  delete_object_item(object, key);

  link = xmlNewTextChild(object, NULL, (xmlChar *)"link", (xmlChar *)target_id.c_str());
  xmlNewProp(link, (xmlChar *)"type", (xmlChar *)"object");
  xmlNewProp(link, (xmlChar *)"struct-name", (xmlChar *)struct_name.c_str());
  xmlNewProp(link, (xmlChar *)"key", (xmlChar *)key);
}

void XMLTraverser::set_object_link_literal(xmlNodePtr object, const char *key, const char *value,
                                           const char *struct_name) {
  xmlNodePtr link;

  delete_object_item(object, key);

  link = xmlNewTextChild(object, NULL, (xmlChar *)"link", (xmlChar *)value);
  xmlNewProp(link, (xmlChar *)"type", (xmlChar *)"object");
  xmlNewProp(link, (xmlChar *)"struct-name", (xmlChar *)struct_name);
  xmlNewProp(link, (xmlChar *)"key", (xmlChar *)key);
}

std::vector<xmlNodePtr> XMLTraverser::scan_objects_of_type(const char *struct_name) {
  std::vector<xmlNodePtr> list;

  for (std::map<std::string, xmlNodePtr>::iterator iter = nodes_by_id.begin(); iter != nodes_by_id.end(); ++iter) {
    if (node_prop(iter->second, "struct-name") == struct_name)
      list.push_back(iter->second);
  }

  return list;
}

std::list<xmlNodePtr> XMLTraverser::scan_nodes_with_key(const char *name, xmlNodePtr parent) {
  std::list<xmlNodePtr> list;
  xmlNodePtr node;

  if (!parent)
    parent = get_root();

  for (node = parent->children; node != NULL; node = node->next) {
    if (node->type == XML_ELEMENT_NODE &&
        (xmlStrcmp(node->name, (xmlChar *)"value") == 0 || xmlStrcmp(node->name, (xmlChar *)"link") == 0)) {
      if (node_prop(node, "key").compare(name) == 0)
        list.push_back(node);

      std::list<xmlNodePtr> sublist(scan_nodes_with_key(name, node));
      list.merge(sublist);
    }
  }
  return list;
}

static void traverse_subtree_node(xmlNodePtr parent, const std::function<bool(xmlNodePtr, xmlNodePtr)> &callback) {
  for (xmlNodePtr node = parent->children; node != NULL; node = node->next) {
    if (node->type == XML_ELEMENT_NODE &&
        (xmlStrcmp(node->name, (xmlChar *)"value") == 0 || xmlStrcmp(node->name, (xmlChar *)"link") == 0)) {
      if (callback(parent, node))
        traverse_subtree_node(node, callback);
    }
  }
}

void XMLTraverser::traverse_subtree(const char *path, const std::function<bool(xmlNodePtr, xmlNodePtr)> &callback) {
  xmlNodePtr node = get_object_by_path(path);

  if (node)
    traverse_subtree_node(node, callback);
}

xmlNodePtr create_grt_object_node(const char *id, const char *struct_type) {
  xmlNodePtr node = xmlNewNode(NULL, (xmlChar *)"value");

  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)"object");
  xmlNewProp(node, (xmlChar *)"struct-name", (xmlChar *)struct_type);
  xmlNewProp(node, (xmlChar *)"id", (xmlChar *)id);

  return node;
}

void set_grt_object_item(xmlNodePtr objnode, const char *name, xmlNodePtr item) {
  xmlAddChild(objnode, item);

  xmlNewProp(item, (xmlChar *)"key", (xmlChar *)name);
}

void set_grt_object_item_link(xmlNodePtr objnode, const char *name, const char *struct_type, const char *oid) {
  xmlNodePtr node = xmlNewTextChild(objnode, NULL, (xmlChar *)"link", (xmlChar *)oid);

  xmlNewProp(node, (xmlChar *)"key", (xmlChar *)name);
  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)"object");
  xmlNewProp(node, (xmlChar *)"struct-name", (xmlChar *)struct_type);
}

void set_grt_object_item_value(xmlNodePtr objnode, const char *name, const char *value) {
  xmlNodePtr node = xmlNewTextChild(objnode, NULL, (xmlChar *)"value", (xmlChar *)value);

  xmlNewProp(node, (xmlChar *)"key", (xmlChar *)name);
  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)"string");
}

void set_grt_object_item_value(xmlNodePtr objnode, const char *name, double value) {
  xmlNodePtr node = xmlNewTextChild(objnode, NULL, (xmlChar *)"value", (xmlChar *)strfmt("%f", value).c_str());

  xmlNewProp(node, (xmlChar *)"key", (xmlChar *)name);
  xmlNewProp(node, (xmlChar *)"type", (xmlChar *)"real");
}

void find_replace_xml_attribute(xmlNodePtr root, const char *attr, const char *from, const char *to) {
  xmlChar *tmp = xmlGetProp(root, (xmlChar *)attr);
  if (tmp && strcmp((char *)tmp, from) == 0)
    xmlSetProp(root, (xmlChar *)attr, (xmlChar *)to);
  if (tmp)
    xmlFree(tmp);

  xmlNodePtr child = root->children;
  while (child) {
    if (child->type == XML_ELEMENT_NODE)
      find_replace_xml_attribute(child, attr, from, to);
    child = child->next;
  }
}

// from_list and to_list must have the same number of items
void find_replace_xml_attributes(xmlNodePtr root, const char **attr_list, const char **from_list,
                                 const char **to_list) {
  for (const char **attr = attr_list; *attr != NULL; attr++) {
    xmlChar *tmp = xmlGetProp(root, (xmlChar *)*attr);

    if (tmp) {
      for (int i = 0; from_list[i] != NULL; i++) {
        const char *from = from_list[i];
        const char *to = to_list[i];

        if (strcmp((char *)tmp, from) == 0) {
          xmlSetProp(root, (xmlChar *)*attr, (xmlChar *)to);
          break;
        }
      }
      xmlFree(tmp);
    }
  }

  xmlNodePtr child = root->children;
  while (child) {
    if (child->type == XML_ELEMENT_NODE)
      find_replace_xml_attributes(child, attr_list, from_list, to_list);
    child = child->next;
  }
}

void rename_xml_grt_members(xmlNodePtr root, const char **klass, const char **name_from, const char **name_to) {
  xmlChar *klass_name = xmlGetProp(root, (xmlChar *)"struct-name");

  xmlNodePtr child = root->children;
  while (child) {
    if (child->type == XML_ELEMENT_NODE) {
      if (klass_name) {
        xmlChar *key = xmlGetProp(child, (xmlChar *)"key");
        if (key) {
          for (int i = 0; klass[i]; i++) {
            if (strcmp(klass[i], (char *)klass_name) == 0 && strcmp(name_from[i], (char *)key) == 0) {
              xmlSetProp(child, (xmlChar *)"key", (xmlChar *)name_to[i]);
              break;
            }
          }
          xmlFree(key);
        }
      }

      rename_xml_grt_members(child, klass, name_from, name_to);
    }
    child = child->next;
  }

  if (klass_name)
    xmlFree(klass_name);
}

void delete_xml_grt_members(xmlNodePtr root, const char **klass, const char **name) {
  xmlChar *klass_name = xmlGetProp(root, (xmlChar *)"struct-name");

  xmlNodePtr child = root->children;
  while (child) {
    xmlNodePtr item = child;
    child = child->next;

    if (item->type == XML_ELEMENT_NODE) {
      bool deleted = false;

      if (klass_name) {
        xmlChar *key = xmlGetProp(item, (xmlChar *)"key");
        if (key) {
          for (int i = 0; klass[i]; i++) {
            if (strcmp(klass[i], (char *)klass_name) == 0 && strcmp(name[i], (char *)key) == 0) {
              xmlUnlinkNode(item);
              xmlFreeNode(item);
              deleted = true;
              break;
            }
          }
          xmlFree(key);
        }
      }
      if (!deleted)
        delete_xml_grt_members(item, klass, name);
    }
  }

  if (klass_name)
    xmlFree(klass_name);
}
