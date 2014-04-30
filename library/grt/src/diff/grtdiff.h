/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _GRT_DIFF_H
#define _GRT_DIFF_H

#include "grtpp.h"
#include "grtpp_util.h"

namespace grt
{
class ValueRef;
class BaseListRef;
class DictRef;
//class ObjectRef;
class DiffChange;


class GrtDiff
{
protected:
  const Omf* omf;
  bool _dont_clone_values;

  virtual boost::shared_ptr<DiffChange> on_list(boost::shared_ptr<DiffChange> parent, const BaseListRef &source, const BaseListRef &target);
  virtual boost::shared_ptr<DiffChange> on_dict(boost::shared_ptr<DiffChange> parent, const DictRef &source, const DictRef &target);
  virtual boost::shared_ptr<DiffChange> on_object(boost::shared_ptr<DiffChange> parent, const ObjectRef &source, const ObjectRef &target);

  virtual boost::shared_ptr<DiffChange> on_uncompatible(boost::shared_ptr<DiffChange> parent, const ValueRef &source, const ValueRef &target);

  boost::shared_ptr<DiffChange> on_value(boost::shared_ptr<DiffChange> parent, const ValueRef &source, const ValueRef &target);
public:
  GrtDiff(const Omf* o, bool dont_clone_values = false) : omf(o), _dont_clone_values(dont_clone_values) {}
  boost::shared_ptr<DiffChange> diff(const ValueRef &source, const ValueRef &target, const Omf* omf);
  virtual ~GrtDiff() {}
};

}

#endif
