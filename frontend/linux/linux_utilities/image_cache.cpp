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
