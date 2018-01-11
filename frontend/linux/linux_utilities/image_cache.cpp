/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <glib.h>
#include <stdio.h>
#include "image_cache.h"
#include "grt/common.h"

//------------------------------------------------------------------------------
Glib::RefPtr<Gdk::Pixbuf> ImageCache::image_from_filename(const std::string &filename, bool cache) {
  Glib::RefPtr<Gdk::Pixbuf> im(0);

  const std::string path = bec::IconManager::get_instance()->get_icon_path(filename);

  im = image_from_path(path, cache);

  return im;
}

//------------------------------------------------------------------------------
Glib::RefPtr<Gdk::Pixbuf> ImageCache::image_from_path(const std::string &path, bool cache) {
  Glib::RefPtr<Gdk::Pixbuf> im(0);

  if (!path.empty()) {
    base::MutexLock lock(_sync);
    ImageMap::iterator it = _images.find(path);
    if (_images.end() != it)
      im = it->second;
    else {
      // Try to load the image
      try {
        im = Gdk::Pixbuf::create_from_file(path);
        if (cache)
          _images[path] = im;
      } catch (...) {
        g_message("Failed to load image from '%s'\n", path.c_str());
      }
    }
  }

  return im;
}

//------------------------------------------------------------------------------
ImageCache *ImageCache::get_instance() {
  static ImageCache *imgs = new ImageCache;

  return imgs;
}
