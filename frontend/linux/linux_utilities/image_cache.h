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
