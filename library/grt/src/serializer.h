/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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
