/*
 * Copyright (c) 2016, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once
#include "common.h"
#include <libxml/tree.h>
#include <string>

namespace base {
  namespace xml {
    BASELIBRARY_PUBLIC_FUNC xmlDocPtr loadXMLDoc(const std::string &path, bool asEntity = false);
    BASELIBRARY_PUBLIC_FUNC xmlDocPtr xmlParseFragment(const std::string &buff);
    BASELIBRARY_PUBLIC_FUNC xmlNodePtr getXmlRoot(xmlDocPtr doc);
    BASELIBRARY_PUBLIC_FUNC bool nameIs(xmlNodePtr node, const std::string &name);
    BASELIBRARY_PUBLIC_FUNC bool nameIs(xmlAttrPtr attrib, const std::string &name);
    BASELIBRARY_PUBLIC_FUNC void getXMLDocMetainfo(xmlDocPtr doc, std::string &doctype, std::string &docversion);
    BASELIBRARY_PUBLIC_FUNC std::string getProp(xmlNodePtr node, const std::string &name);
    BASELIBRARY_PUBLIC_FUNC std::string getContent(xmlNodePtr node);
    BASELIBRARY_PUBLIC_FUNC std::string getContentRecursive(xmlNodePtr node);
    BASELIBRARY_PUBLIC_FUNC std::string encodeEntities(const std::string &input);
  }; /* end of namespace xml */
};   /* end of namespace base */
