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

#pragma once

#include <mforms/base.h>
#include <mforms/view.h>

namespace mforms {
  class ImageBox;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct ImageBoxImplPtrs {
    bool (*create)(ImageBox *);
    void (*set_image)(ImageBox *, const std::string &);
    void (*set_image_data)(ImageBox *, const char *data, size_t length);
    void (*set_scale_contents)(ImageBox *, bool);
    void (*set_image_align)(ImageBox *, Alignment);
  };
#endif
#endif

  /** Shows an image file. */
  class MFORMS_EXPORT ImageBox : public View {
  public:
    ImageBox();

    /** Sets the path to the image file.

     Image formats accepted depend on platform, but usually png and jpeg will work. */
    void set_image(const std::string &file);

#ifndef SWIG
    void set_image_data(const char *data, size_t length);
#endif

    /** Whether image should be scaled to fit the imagebox. */
    void set_scale_contents(bool flag);

    /** How the image should be aligned in case there's space left in the image box */
    void set_image_align(Alignment alignment);

  protected:
    ImageBoxImplPtrs *_imagebox_impl;
  };
};
