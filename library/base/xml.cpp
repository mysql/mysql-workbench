/*
 * Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */


#include <glib.h>
#include <stdexcept>
#include "base/xml.h"

xmlDocPtr base::xml::loadXMLDoc(const std::string &path)
{
  xmlDocPtr doc;

  char * localFilename = nullptr;
  if ((localFilename = g_filename_from_utf8(path.c_str(),-1,NULL,NULL,NULL)) == NULL)
    throw std::runtime_error("can't open XML file " + path);

//  if (parseEntity)
//    doc = xmlParseEntity(localFilename);
//  else
    doc = xmlParseFile(localFilename);

  g_free(localFilename);
  if (doc == nullptr)
    throw std::runtime_error("unable to parse XML file " + path);

  return doc;
}

xmlNodePtr base::xml::getXmlRoot(xmlDocPtr doc)
{
  auto cur = xmlDocGetRootElement(doc);
  if (cur == NULL)
    throw std::runtime_error("Empty document\n");
  return cur;
}

bool base::xml::compareName(xmlNodePtr node, const std::string &name)
{
  return !xmlStrcmp(node->name, (const xmlChar *) name.c_str());
}

bool base::xml::compareName(xmlAttrPtr attrib, const std::string &name)
{
  return !xmlStrcmp(attrib->name, (const xmlChar *) name.c_str());
}

void base::xml::getXMLDocMetainfo(xmlDocPtr doc, std::string &doctype, std::string &docversion)
{
  xmlNodePtr root = xmlDocGetRootElement(doc);

  while (root)
  {
    if (root->type == XML_ELEMENT_NODE)
    {
      doctype = getProp(root, "document_type");
      docversion = getProp(root, "version");
      break;
    }
    root = root->next;
  }
}

std::string base::xml::getProp(xmlNodePtr node, const std::string &name)
{
  xmlChar *prop = xmlGetProp(node, (xmlChar*)name.c_str());
  std::string tmp = prop ? (char*)prop : "";
  xmlFree(prop);
  return tmp;
}

std::string base::xml::getContent(xmlNodePtr node)
{
  xmlChar *prop = xmlNodeGetContent(node);
  std::string tmp = prop ? (char*)prop : "";
  xmlFree(prop);
  return tmp;
}
