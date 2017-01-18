
#ifndef _UPGRADE_HELPER_H_
#define _UPGRADE_HELPER_H_

// Helper classes and functions for upgrading options and model files.

#include "grt/common.h"
#include "wb_model_file.h"

using namespace wb;
using namespace grt;

struct xmlString {
  xmlChar *str;

  xmlString(xmlChar *s) : str(s) {
  }

  ~xmlString() {
    if (str)
      xmlFree(str);
  }

  operator xmlChar *() {
    return str;
  }

  operator const char *() {
    return (const char *)str;
  }

  operator std::string() {
    return std::string((const char *)str);
  }

  operator bool() {
    return str != 0;
  }
};

class XMLTraverser {
private:
  xmlDocPtr doc;
  xmlNodePtr root;
  std::map<std::string, xmlNodePtr> nodes_by_id;

  void cache_object_nodes(xmlNodePtr node);

public:
  XMLTraverser(xmlDocPtr adoc);
  xmlNodePtr get_root();
  static std::string node_prop(xmlNodePtr node, const char *prop);
  bool delete_object_item(xmlNodePtr objnode, const char *name);
  xmlNodePtr get_object(const char *id);
  xmlNodePtr get_object_by_path(const char *path);
  xmlNodePtr get_object_child_by_index(xmlNodePtr object, int index);
  xmlNodePtr get_object_child(xmlNodePtr object, const char *key);
  void set_object_child(xmlNodePtr object, const char *key, xmlNodePtr value);
  void set_object_link(xmlNodePtr object, const char *key, xmlNodePtr target_object);
  void set_object_link_literal(xmlNodePtr object, const char *key, const char *value, const char *struct_name);
  std::vector<xmlNodePtr> scan_objects_of_type(const char *struct_name);
  std::list<xmlNodePtr> scan_nodes_with_key(const char *name, xmlNodePtr parent = NULL);

  double get_object_double_value(xmlNodePtr object, const char *key);

  // return value of callback determines if that node should be traversed too
  void traverse_subtree(const char *path, const std::function<bool(xmlNodePtr, xmlNodePtr)> &callback);
};

xmlNodePtr create_grt_object_node(const char *id, const char *struct_type);
void set_grt_object_item(xmlNodePtr objnode, const char *name, xmlNodePtr item);
void set_grt_object_item_link(xmlNodePtr objnode, const char *name, const char *struct_type, const char *oid);
void set_grt_object_item_value(xmlNodePtr objnode, const char *name, const char *value);
void set_grt_object_item_value(xmlNodePtr objnode, const char *name, double value);

void find_replace_xml_attribute(xmlNodePtr root, const char *attr, const char *from, const char *to);

void find_replace_xml_attributes(xmlNodePtr root, const char **attr, const char **from, const char **to);

void rename_xml_grt_members(xmlNodePtr root, const char **klass, const char **name_from, const char **name_to);
void delete_xml_grt_members(xmlNodePtr root, const char **klass, const char **name);

inline xmlNodePtr first_xml_element(xmlNodePtr node) {
  while (node && node->type != XML_ELEMENT_NODE)
    node = node->next;
  return node;
}

#endif //_UPGRADE_HELPER_H_
