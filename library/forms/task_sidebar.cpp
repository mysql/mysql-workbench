/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

/**
 * mforms interface to the advanced sidebar, which implements an iTunes like interface.
 */

#include "mforms/task_sidebar.h"

using namespace mforms;

static std::map<std::string, TaskSidebar* (*)()> *sidebar_factory = nullptr;

//----------------------------------------------------------------------------------------------------------------------

mforms::TaskSidebar::TaskSidebar() : Box(false) {
}

//----------------------------------------------------------------------------------------------------------------------

TaskSidebar* TaskSidebar::create(const std::string& type) {
  if (!sidebar_factory || sidebar_factory->find(type) == sidebar_factory->end())
    throw std::invalid_argument("Invalid sidebar type " + type);

  return (*sidebar_factory)[type]();
}

//----------------------------------------------------------------------------------------------------------------------

void TaskSidebar::register_factory(const std::string& type, TaskSidebar* (*create)()) {
  if (!sidebar_factory)
    sidebar_factory = new std::map<std::string, TaskSidebar* (*)()>();

  (*sidebar_factory)[type] = create;
}

//--------------------------------------------------------------------------------------------------
