/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/xml_functions.h"
#include "base/log.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include <libxml/HTMLparser.h>

#include <glib.h>
#include <stdexcept>
#include <vector>

DEFAULT_LOG_DOMAIN("XML Functions")

static void xmlErrorHandling(void *ctx, const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  va_list args_copy;
  va_copy(args_copy, args);
  std::vector<char> buff(1 + std::vsnprintf(NULL, 0, msg, args_copy));
  va_end(args_copy);
  std::vsnprintf(buff.data(), buff.size(), msg, args);
  va_end(args);
  logError("LibXml: %s\n", buff.data());
}

xmlDocPtr base::xml::loadXMLDoc(const std::string &path) {
  xmlSetGenericErrorFunc(nullptr, xmlErrorHandling);

  if (!base::file_exists(path))
    throw std::runtime_error("unable to open XML file, doesn't exists: " + path);

  xmlDocPtr doc = xmlParseFile(path.c_str());

  if (doc == nullptr)
    throw std::runtime_error("unable to parse XML file " + path);

  return doc;
}

xmlDocPtr base::xml::xmlParseFragment(const std::string &buff) {
  return xmlParseMemory(buff.data(), (int)buff.size());
}

xmlNodePtr base::xml::getXmlRoot(xmlDocPtr doc) {
  auto cur = xmlDocGetRootElement(doc);
  if (cur == NULL)
    throw std::runtime_error("Empty document\n");
  return cur;
}

bool base::xml::nameIs(xmlNodePtr node, const std::string &name) {
  return xmlStrcmp(node->name, (const xmlChar *)name.c_str()) == 0;
}

bool base::xml::nameIs(xmlAttrPtr attrib, const std::string &name) {
  return xmlStrcmp(attrib->name, (const xmlChar *)name.c_str()) == 0;
}

void base::xml::getXMLDocMetainfo(xmlDocPtr doc, std::string &doctype, std::string &docversion) {
  xmlNodePtr root = xmlDocGetRootElement(doc);

  while (root) {
    if (root->type == XML_ELEMENT_NODE) {
      doctype = getProp(root, "document_type");
      docversion = getProp(root, "version");
      break;
    }
    root = root->next;
  }
}

std::string base::xml::getProp(xmlNodePtr node, const std::string &name) {
  xmlChar *prop = xmlGetProp(node, (xmlChar *)name.c_str());
  std::string tmp = prop ? (char *)prop : "";
  xmlFree(prop);
  return tmp;
}

std::string base::xml::getContent(xmlNodePtr node) {
  xmlChar *prop = xmlNodeGetContent(node);
  std::string tmp = prop ? (char *)prop : "";
  xmlFree(prop);
  return tmp;
}

std::string base::xml::getContentRecursive(xmlNodePtr node) {
  std::string result;
  result = base::xml::getContent(node);
  auto current = node->children;
  while (current != nullptr) {
    result += base::xml::getContent(current);
    current = current->next;
  }
  return result;
}

std::string base::xml::encodeEntities(const std::string &input) {
  int buffSize = (int)input.size() * 2 + 1;
  std::vector<unsigned char> buff(buffSize, '\0');
  int outLen = buffSize - 1, inLen = (int)input.size();

  htmlEncodeEntities(buff.data(), &outLen, (const unsigned char *)(input.c_str()), &inLen, '"');
  buff.erase(buff.begin() + outLen, buff.end());
  return std::string(buff.begin(), buff.end());
}
