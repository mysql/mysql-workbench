/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "common.h"
#include "aalcommon.h"

namespace mga {

/**
 * An XPath like implementation for accessibility. Instead of xml tags we use roles to filter
 * child elements. Otherwise the path syntax is very similar to that of XPath.
 * Only predicates are not supported at full detail.
 */
class APath {
public:
  APath(aal::Accessible *root);

  aal::AccessibleList execute(aal::Accessible *anchor, std::string const& path, bool includeEmptyNames,
    bool includeEmptyIDs, bool includeInternal);

private:
  aal::Accessible *_root;
  
  using StringListIterator = std::vector<std::string>::iterator;

  aal::AccessibleList executePath(aal::Accessible *anchor, StringListIterator &begin, StringListIterator const& end,
    bool includeEmptyNames, bool includeEmptyIDs, bool includeInternal);
  aal::AccessibleList getParents(aal::Accessible *element) const;
  aal::AccessibleList getSiblings(aal::Accessible *element, bool leading, bool trailing) const;
};

}
