/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _GRT_LIST_DIFF_H
#define _GRT_LIST_DIFF_H

#include "grtpp_util.h"

namespace grt {

  class BaseListRef;
  class MultiChange;
  struct Omf;

  class GrtListDiff {
  public:
    static std::shared_ptr<MultiChange> diff(const BaseListRef &source, const BaseListRef &target, const Omf *omf);
  };
}

#endif
