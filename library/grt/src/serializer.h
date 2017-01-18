/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
    class Serializer {
    public:
      Serializer();

      void save_to_xml(const ValueRef &value, const std::string &path, const std::string &doctype = "",
                       const std::string &docversion = "", bool list_objects_as_links = false);

      xmlDocPtr create_xmldoc_for_value(const ValueRef &value, const std::string &doctype,
                                        const std::string &docversion, bool list_objects_as_links);

      std::string serialize_to_xmldata(const ValueRef &value, const std::string &type, const std::string &version,
                                       bool list_objects_as_links);

    protected:
      std::set<void *> _cache;

      xmlNodePtr serialize_value(const ValueRef &value, xmlNodePtr parent, bool owned_objects);
      xmlNodePtr serialize_object(const Ref<Object> &object, xmlNodePtr parent);

      bool seen(const ValueRef &value);

      bool serialize_member(const MetaClass::Member *member, const ObjectRef &object, xmlNodePtr node);
    };
  };
};
