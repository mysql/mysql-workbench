/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

      static void set_scale_contents(ImageBox *, bool) {
      }

      static void set_image_align(ImageBox *, Alignment) {
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_imagebox_impl.create = &ImageBoxWrapper::create;
        f->_imagebox_impl.set_image = &ImageBoxWrapper::set_image;
        f->_imagebox_impl.set_scale_contents = &ImageBoxWrapper::set_scale_contents;
        f->_imagebox_impl.set_image_align = &ImageBoxWrapper::set_image_align;
      }
    };
  };
};

#endif /* _STUB_IMAGEBOX_H_ */
