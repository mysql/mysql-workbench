/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MSC_VER
#include <cairo.h>
#endif

#include "mdc_image.h"
#include "mdc_image_manager.h"

/**
 * @file  mdc_image_manager.cpp
 * @brief A simple image cache to avoid frequent load calls.
 */

using namespace mdc;

ImageManager::ImageManager() {
}

ImageManager *ImageManager::get_instance() {
  static ImageManager *instance = 0;

  if (!instance)
    instance = new ImageManager();

  return instance;
}

//--------------------------------------------------------------------------------------------------

cairo_surface_t *ImageManager::find_file(const std::string &name) {
  cairo_surface_t *img = mdc::surface_from_png_image(name.c_str());

  if (img != NULL)
    return img;

  for (std::list<std::string>::const_iterator iter = _search_paths.begin(); iter != _search_paths.end(); ++iter) {
    std::string path = *iter;
#ifdef _MSC_VER
    path += "\\" + name;
#else
    path += "/" + name;
#endif

    cairo_surface_t *img = mdc::surface_from_png_image(path.c_str());

    if (img != NULL)
      return img;
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

cairo_surface_t *ImageManager::get_image(const std::string &name) {
  if (_cache.find(name) != _cache.end())
    return _cache[name];

  cairo_surface_t *img = find_file(name);
  if (img)
    _cache[name] = img;

  return img;
}

/**
 * Searches the cache for an image loaded from the given path and frees it if found.
 *
 * @param name The file name of the image.
 * @return true if the image was found and freed, otherwise false.
 */
bool ImageManager::release_image(const std::string &name) {
  std::map<std::string, cairo_surface_t *>::iterator iterator = _cache.find(name);
  if (iterator != _cache.end()) {
    cairo_surface_destroy(iterator->second);
    _cache.erase(iterator);
    return true;
  }
  return false;
}

cairo_surface_t *ImageManager::get_image_nocache(const std::string &path) {
  if (_cache.find(path) != _cache.end())
    return cairo_surface_reference(_cache[path]);

  return find_file(path);
}

void ImageManager::add_search_path(const std::string &directory) {
  if (std::find(_search_paths.begin(), _search_paths.end(), directory) == _search_paths.end())
    _search_paths.push_back(directory);
}
