/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef __CHANGE_FACTORY_H
#define __CHANGE_FACTORY_H

#include <string>
#include "grtpp.h"

namespace grt
{
class DiffChange;
class DictRef;
class BaseListRef;
class MultiChange;
struct ChangeSet;


struct ChangeFactory 
{
static boost::shared_ptr<DiffChange> create_value_added_change(boost::shared_ptr<DiffChange> parent, const ValueRef &source, const ValueRef &target, bool dupvalue = true);
static boost::shared_ptr<DiffChange> create_value_removed_change(boost::shared_ptr<DiffChange> parent, const ValueRef &source, const ValueRef &target);

static boost::shared_ptr<DiffChange> create_object_attr_modified_change(boost::shared_ptr<DiffChange> parent, const ObjectRef &source, const ObjectRef &target, const std::string &attr, boost::shared_ptr<DiffChange> change);
static boost::shared_ptr<MultiChange> create_object_modified_change(boost::shared_ptr<DiffChange> parent, const ObjectRef &source, const ObjectRef &target, ChangeSet &changes);

static boost::shared_ptr<MultiChange> create_dict_change(boost::shared_ptr<DiffChange> parent, const DictRef &source, const DictRef &target, ChangeSet &changes);

static boost::shared_ptr<DiffChange> create_dict_item_added_change(boost::shared_ptr<DiffChange> parent, const DictRef &source, const DictRef &target, const std::string &key, ValueRef v, bool dupvalue = true);
static boost::shared_ptr<DiffChange> create_dict_item_modified_change(boost::shared_ptr<DiffChange> parent, const DictRef &source, const DictRef &target, const std::string &key, boost::shared_ptr<DiffChange> change);
static boost::shared_ptr<DiffChange> create_dict_item_removed_change(boost::shared_ptr<DiffChange> parent, const DictRef &source, const DictRef &target, const std::string &key);

static boost::shared_ptr<DiffChange> create_simple_value_change(boost::shared_ptr<DiffChange> parent, const ValueRef &source, const ValueRef &target);
};


}

#endif
