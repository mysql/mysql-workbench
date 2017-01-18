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
#include "grts/structs.workbench.model.h"

#include "wb_editor_backend_public_interface.h"

class WBEDITOR_BACKEND_PUBLIC_FUNC ImageEditorBE : public bec::BaseEditor {
  workbench_model_ImageFigureRef _image;

public:
  ImageEditorBE(const workbench_model_ImageFigureRef &image);

  virtual bool should_close_on_delete_of(const std::string &oid);

  void get_size(int &w, int &h);
  void set_size(int w, int h);
  void set_width(int w);
  void set_height(int h);

  bool get_keep_aspect_ratio();
  void set_keep_aspect_ratio(bool flag);

  void set_filename(const std::string &text);
  std::string get_filename() const;

  std::string get_attached_image_path();

  virtual std::string get_title();
};
