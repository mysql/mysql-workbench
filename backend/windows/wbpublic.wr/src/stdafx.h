/*
 * Copyright (c) 2008, 2023, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif

#pragma warning(disable : 4793) // 'vararg' causes native code generation

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <vcclr.h> // .net interop helpers

#define _USE_MATH_DEFINES
#include <math.h>

#include <string>
#include <vector>
#include <stdexcept>
#include <stdarg.h>
#include <algorithm>
#include <map>
#include <list>
#include <sstream>
#include <cctype>
#include <algorithm>
#include <exception>
#include <stack>

#include <glib.h>

#include <boost/signals2.hpp>
#include <boost/ref.hpp>

#include <gl/gl.h>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-svg.h>

#include "grts/structs.model.h"
#include "grts/structs.db.query.h"
#include "grts/structs.db.mysql.h"
#include "grts/structs.workbench.physical.h"

#pragma make_public(std::exception)

namespace bec {
  class RoutineEditorBE;
  class DBObjectMasterFilterBE;
  class ListModel;
  class ValueInspectorBE;
  class GrtStringListModel;
  class BaseEditor;
  class RoutineGroupEditorBE;
  class RoleEditorBE;
  class RolePrivilegeListBE;
  class RoleObjectListBE;
  class UserEditorBE;
  class RoleTreeBE;
  class ObjectRoleListBE;
  class ObjectPrivilegeListBE;
  class SchemaEditorBE;
  class TreeModel;
  struct NodeId;
  class GRTManager;
  class RefreshUI;
  class IndexListBE;
  class ViewEditorBE;
  class TableColumnsListBE;
  class TableEditorBE;
}

#pragma make_public(bec::RoutineEditorBE)
#pragma make_public(bec::DBObjectMasterFilterBE)
#pragma make_public(bec::ListModel)
#pragma make_public(bec::ValueInspectorBE)
#pragma make_public(bec::GrtStringListModel)
#pragma make_public(bec::BaseEditor)
#pragma make_public(bec::RoutineGroupEditorBE)
#pragma make_public(bec::RoleEditorBE)
#pragma make_public(bec::RolePrivilegeListBE)
#pragma make_public(bec::RoleObjectListBE)
#pragma make_public(bec::UserEditorBE)
#pragma make_public(bec::RoleTreeBE)
#pragma make_public(bec::ObjectRoleListBE)
#pragma make_public(bec::ObjectPrivilegeListBE)
#pragma make_public(bec::SchemaEditorBE)
#pragma make_public(bec::TreeModel)
#pragma make_public(bec::NodeId)
#pragma make_public(bec::GRTManager)
#pragma make_public(bec::RefreshUI)
#pragma make_public(bec::IndexListBE)
#pragma make_public(bec::ViewEditorBE)
#pragma make_public(bec::TableColumnsListBE)
#pragma make_public(bec::TableEditorBE)

namespace grt {
  class ValueRef;
  class Module;
  class DictRef;
}

#pragma make_public(grt::ValueRef)
#pragma make_public(grt::Module)
#pragma make_public(grt::DictRef)

namespace mdc {
  struct KeyInfo;
}

#pragma make_public(mdc::KeyInfo)

namespace mdc {
  class CanvasView;
}

#pragma make_public(mdc::CanvasView)

class GrtThreadedTask;

#pragma make_public(GrtThreadedTask)
