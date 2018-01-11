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

namespace grt {
  namespace internal {
    class Unserializer {
    public:
      Unserializer(bool check_crc);

      ValueRef load_from_xml(const std::string &path, std::string *doctype = 0, std::string *docversion = 0);

      ValueRef unserialize_xmldoc(xmlDocPtr doc, const std::string &source_path = "");

      ValueRef unserialize_xmldata(const char *data, size_t size);

    protected:
      std::string _source_name;
      std::map<std::string, ValueRef> _cache;
      std::set<std::string> _invalid_cache;
      bool _check_serialized_crc;

      ValueRef unserialize_from_xml(xmlNodePtr node);
      ValueRef traverse_xml_recreating_tree(xmlNodePtr node);
      void traverse_xml_creating_objects(xmlNodePtr node);

      ObjectRef unserialize_object_step1(xmlNodePtr node);
      ObjectRef unserialize_object_step2(xmlNodePtr node);
      void unserialize_object_contents(const ObjectRef &object, xmlNodePtr node);
      ValueRef find_cached(const std::string &id);
    };
  };
};
