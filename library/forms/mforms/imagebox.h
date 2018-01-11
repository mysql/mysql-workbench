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
