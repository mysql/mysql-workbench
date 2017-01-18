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

#pragma once

#include "grt.h"
#include "grtpp_util.h"

namespace grt {
  class ValueRef;
  class BaseListRef;
  class DictRef;
  class DiffChange;

  class GrtDiff {
  protected:
    const Omf *omf;
    bool _dont_clone_values;

    virtual std::shared_ptr<DiffChange> on_list(std::shared_ptr<DiffChange> parent, const BaseListRef &source,
                                                const BaseListRef &target);
    virtual std::shared_ptr<DiffChange> on_dict(std::shared_ptr<DiffChange> parent, const DictRef &source,
                                                const DictRef &target);
    virtual std::shared_ptr<DiffChange> on_object(std::shared_ptr<DiffChange> parent, const ObjectRef &source,
                                                  const ObjectRef &target);

    virtual std::shared_ptr<DiffChange> on_uncompatible(std::shared_ptr<DiffChange> parent, const ValueRef &source,
                                                        const ValueRef &target);

    std::shared_ptr<DiffChange> on_value(std::shared_ptr<DiffChange> parent, const ValueRef &source,
                                         const ValueRef &target);

  public:
    GrtDiff(const Omf *o, bool dont_clone_values = false) : omf(o), _dont_clone_values(dont_clone_values) {
    }
    std::shared_ptr<DiffChange> diff(const ValueRef &source, const ValueRef &target, const Omf *omf);
    virtual ~GrtDiff() {
    }
  };
}
