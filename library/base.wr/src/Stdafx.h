/*
 * Copyright (c) 2013, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <string>
#include <vector>
#include <map>
#include <list>
#include <vcclr.h>
#include <msclr\lock.h>
#include <fcntl.h>
#include <io.h>
#include <iosfwd>
#include <fstream>
#include <iostream>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace base {
  class Color;
}

#pragma make_public(base::Color)

namespace bec {
  class UIForm;
  struct MenuItem;
  struct ToolbarItem;
}

#pragma make_public(bec::UIForm)
#pragma make_public(bec::MenuItem)
#pragma make_public(bec::ToolbarItem)
