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

#include "wb_editor_image.h"
#include "base/string_utilities.h"

using namespace bec;

ImageEditorBE::ImageEditorBE(const workbench_model_ImageFigureRef &image) : bec::BaseEditor(image), _image(image) {
}

bool ImageEditorBE::should_close_on_delete_of(const std::string &oid) {
  if (_image.id() == oid || _image->owner().id() == oid)
    return true;

  return false;
}

void ImageEditorBE::set_filename(const std::string &text) {
  if (text != *_image->filename()) {
    AutoUndoEdit undo(this);
    _image->setImageFile(text);
    undo.end(_("Change Image"));
  }
}

void ImageEditorBE::get_size(int &w, int &h) {
  w = (int)_image->width();
  h = (int)_image->height();
}

void ImageEditorBE::set_size(int w, int h) {
  if (w > 0 && h > 0 && (w != *_image->width() || h != *_image->height())) {
    AutoUndoEdit undo(this);
    _image->width(w);
    _image->height(h);
    undo.end(_("Resize Image"));
  }
}

void ImageEditorBE::set_width(int w) {
  AutoUndoEdit undo(this);

  if (*_image->keepAspectRatio() && _image->width() > 0) {
    double aspect_ratio = _image->height() / _image->width();

    if (_image->height() != aspect_ratio * w)
      _image->height(aspect_ratio * w);
  }
  if (*_image->width() != w)
    _image->width(w);

  undo.end(_("Set Image Size"));
}

void ImageEditorBE::set_height(int h) {
  AutoUndoEdit undo(this);

  if (*_image->keepAspectRatio() && _image->height() > 0) {
    double aspect_ratio = _image->width() / _image->height();

    if (_image->width() != aspect_ratio * h)
      _image->width(aspect_ratio * h);
  }
  if (*_image->height() != h)
    _image->height(h);

  undo.end(_("Set Image Size"));
}

bool ImageEditorBE::get_keep_aspect_ratio() {
  return _image->keepAspectRatio() == 1;
}

void ImageEditorBE::set_keep_aspect_ratio(bool flag) {
  AutoUndoEdit undo(this);

  _image->keepAspectRatio(flag);

  undo.end(_("Toggle Image Aspect Ratio"));
}

std::string ImageEditorBE::get_filename() const {
  return _image->filename();
}

std::string ImageEditorBE::get_attached_image_path() {
  grt::Module *module = grt::GRT::get()->get_module("Workbench");

  if (!module)
    throw std::runtime_error("Workbench module not found");

  grt::BaseListRef args(true);

  args.ginsert(_image->filename());

  std::string value = grt::StringRef::cast_from(module->call_function("getAttachedFileTmpPath", args));

  return value;
}

std::string ImageEditorBE::get_title() {
  return base::strfmt("Image");
}
