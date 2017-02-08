#ifndef _MDC_IMAGE_MANAGER_H_
#define _MDC_IMAGE_MANAGER_H_

#include "mdc_common.h"

namespace mdc {

  class MYSQLCANVAS_PUBLIC_FUNC ImageManager {
    std::list<std::string> _search_paths;

    std::map<std::string, cairo_surface_t *> _cache;

    ImageManager();

    cairo_surface_t *find_file(const std::string &name);

  public:
    static ImageManager *get_instance();

    cairo_surface_t *get_image(const std::string &name);
    bool release_image(const std::string &name);

    cairo_surface_t *get_image_nocache(const std::string &name);

    void add_search_path(const std::string &directory);
  };

} // end of mdc namespace

#endif /* _MDC_IMAGE_MANAGER_H_ */
