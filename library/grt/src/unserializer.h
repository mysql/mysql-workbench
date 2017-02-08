/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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
