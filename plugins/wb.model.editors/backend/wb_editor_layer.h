/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/editor_base.h"
#include "grts/structs.workbench.physical.h"

#include "wb_editor_backend_public_interface.h"

class WBEDITOR_BACKEND_PUBLIC_FUNC LayerEditorBE : public bec::BaseEditor {
  workbench_physical_LayerRef _layer;

public:
  LayerEditorBE(const workbench_physical_LayerRef &layer);

  virtual bool should_close_on_delete_of(const std::string &oid);

  void set_color(const std::string &color);
  std::string get_color();

  void set_name(const std::string &name);
  std::string get_name();

  virtual std::string get_title();
};
