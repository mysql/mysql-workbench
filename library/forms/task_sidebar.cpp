/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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
 * 02110-1301  USATaskSidebar
 */

/**
 * mforms interface to the advanced sidebar, which implements an iTunes like interface.
 */

#include "mforms/task_sidebar.h"

using namespace mforms;

static std::map<std::string, TaskSidebar* (*)()>* sidebar_factory = NULL;

//--------------------------------------------------------------------------------------------------

mforms::TaskSidebar::TaskSidebar() : Box(false) {
}

//--------------------------------------------------------------------------------------------------
TaskSidebar* TaskSidebar::create(const std::string& type) {
  if (!sidebar_factory || sidebar_factory->find(type) == sidebar_factory->end())
    throw std::invalid_argument("Invalid sidebar type " + type);

  return (*sidebar_factory)[type]();
}

//--------------------------------------------------------------------------------------------------

void TaskSidebar::register_factory(const std::string& type, TaskSidebar* (*create)()) {
  if (!sidebar_factory)
    sidebar_factory = new std::map<std::string, TaskSidebar* (*)()>();

  (*sidebar_factory)[type] = create;
}

//--------------------------------------------------------------------------------------------------
