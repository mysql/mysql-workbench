/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _STUB_IMAGEBOX_H_
#define _STUB_IMAGEBOX_H_

#include "stub_view.h"

namespace mforms {
  namespace stub {

    class ImageBoxWrapper : public ViewWrapper {
    protected:
      ImageBoxWrapper(::mforms::ImageBox *self) : ViewWrapper(self) {
      }

      static bool create(::mforms::ImageBox *self) {
        return true;
      }

      static void set_image(::mforms::ImageBox *self, const std::string &file) {
      }

      static void setImageData(::mforms::ImageBox *, const char *data, size_t length) {
      }

      static void set_scale_contents(ImageBox *, bool) {
      }

      static void set_image_align(ImageBox *, Alignment) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_imagebox_impl.create = &ImageBoxWrapper::create;
        f->_imagebox_impl.set_image = &ImageBoxWrapper::set_image;
        f->_imagebox_impl.set_image_data = &ImageBoxWrapper::setImageData;
        f->_imagebox_impl.set_scale_contents = &ImageBoxWrapper::set_scale_contents;
        f->_imagebox_impl.set_image_align = &ImageBoxWrapper::set_image_align;
      }
    };
  };
};

#endif /* _STUB_IMAGEBOX_H_ */
