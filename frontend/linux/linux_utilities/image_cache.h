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

#ifndef __IMAGE_CACHE_H__
#define __IMAGE_CACHE_H__

#include <gdkmm/pixbuf.h>
#include <map>
#include <string>

#include "base/threading.h"
#include "grt/icon_manager.h"

class ImageCache {
public:
  ImageCache() {
  }

  Glib::RefPtr<Gdk::Pixbuf> image_from_path(const std::string& name, bool cache = true);
  Glib::RefPtr<Gdk::Pixbuf> image_from_filename(const std::string& name, bool cache = true);
  Glib::RefPtr<Gdk::Pixbuf> image(bec::IconId name);

  static ImageCache* get_instance();

private:
  typedef std::map<std::string, Glib::RefPtr<Gdk::Pixbuf> > ImageMap;

  ImageMap _images;
  base::Mutex _sync;
};

//------------------------------------------------------------------------------
inline Glib::RefPtr<Gdk::Pixbuf> ImageCache::image(bec::IconId icon) {
  std::string path = bec::IconManager::get_instance()->get_icon_path(icon);

  // if (path.empty())
  //  g_message("Cannot locate image '%s' (%i)", bec::IconManager::get_instance()->get_icon_file(icon).c_str(), icon);

  return image_from_path(path);
}

#endif
