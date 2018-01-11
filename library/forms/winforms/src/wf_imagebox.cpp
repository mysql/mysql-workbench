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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_imagebox.h"

#include "base/string_utilities.h"

using namespace System::Drawing;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace MySQL::Forms;

//--------------------------------------------------------------------------------------------------

ImageBoxWrapper::ImageBoxWrapper(mforms::ImageBox *backend) : ViewWrapper(backend) {
}

//--------------------------------------------------------------------------------------------------

bool ImageBoxWrapper::create(mforms::ImageBox *backend) {
  ImageBoxWrapper *wrapper = new ImageBoxWrapper(backend);
  PictureBox ^ imagebox = ImageBoxWrapper::Create<PictureBox>(backend, wrapper);
  imagebox->SizeMode = PictureBoxSizeMode::CenterImage;
  return true;
}

//--------------------------------------------------------------------------------------------------

void ImageBoxWrapper::set_image(mforms::ImageBox *backend, const std::string &file) {
  if (!file.empty()) {
    PictureBox ^ box = ImageBoxWrapper::GetManagedObject<PictureBox>(backend);
    String ^ name = CppStringToNative(mforms::App::get()->get_resource_path(file));

    try {
      if (File::Exists(name))
        box->Load(Path::GetFullPath(name));
      else
        box->Load(Application::StartupPath + "/" + name);
    } catch (...) {
      mforms::Utilities::show_error(_("Error while loading image"), _("An error occured while loading image ") + file,
                                    _("Close"));
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ImageBoxWrapper::set_image_align(mforms::ImageBox *backend, mforms::Alignment alignment) {
  // Alignment within a picture box is only partially adjustable. Embed the box in an own container
  // to have more control over the alignment.
  PictureBox ^ box = ImageBoxWrapper::GetManagedObject<PictureBox>(backend);
  switch (alignment) {
    case mforms::TopLeft: {
      box->SizeMode = PictureBoxSizeMode::Normal;
      break;
    }

    case mforms::MiddleCenter: {
      box->SizeMode = PictureBoxSizeMode::CenterImage;
      break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void ImageBoxWrapper::set_image_data(mforms::ImageBox *backend, const char *data, size_t length) {
  if (data != NULL && length > 0) {
    UnmanagedMemoryStream ^ stream = gcnew UnmanagedMemoryStream((byte *)data, length);
    try {
      Image ^ image = Image::FromStream(stream);
      PictureBox ^ box = ImageBoxWrapper::GetManagedObject<PictureBox>(backend);
      box->Image = image;
    } catch (...) {
      mforms::Utilities::show_error(_("Error while loading image data"), _("The data seems not to be valid."),
                                    _("Close"));
    }

    delete stream;
  }
}

//--------------------------------------------------------------------------------------------------

void ImageBoxWrapper::set_scale_contents(mforms::ImageBox *backend, bool flag) {
  PictureBox ^ box = ImageBoxWrapper::GetManagedObject<PictureBox>(backend);
  if (flag)
    box->SizeMode = PictureBoxSizeMode::StretchImage;
  else
    box->SizeMode = PictureBoxSizeMode::AutoSize;
}

//--------------------------------------------------------------------------------------------------

void ImageBoxWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_imagebox_impl.create = &ImageBoxWrapper::create;
  f->_imagebox_impl.set_image = &ImageBoxWrapper::set_image;
  f->_imagebox_impl.set_scale_contents = &ImageBoxWrapper::set_scale_contents;
  f->_imagebox_impl.set_image_align = &ImageBoxWrapper::set_image_align;
  f->_imagebox_impl.set_image_data = &ImageBoxWrapper::set_image_data;
}

//--------------------------------------------------------------------------------------------------
