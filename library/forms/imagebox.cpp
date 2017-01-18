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

#include "mforms/mforms.h"

using namespace mforms;

ImageBox::ImageBox() {
  _imagebox_impl = &ControlFactory::get_instance()->_imagebox_impl;

  _imagebox_impl->create(this);
}

void ImageBox::set_image(const std::string &file) {
  _imagebox_impl->set_image(this, file);
}

void ImageBox::set_image_data(const char *data, size_t length) {
  _imagebox_impl->set_image_data(this, data, length);
}

void ImageBox::set_scale_contents(bool flag) {
  _imagebox_impl->set_scale_contents(this, flag);
}

void ImageBox::set_image_align(Alignment alignment) {
  _imagebox_impl->set_image_align(this, alignment);
}