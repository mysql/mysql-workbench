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

#ifndef _LF_IMAGEBOX_H_
#define _LF_IMAGEBOX_H_

#include "mforms/label.h"

#include "lf_view.h"
#include "mforms/app.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN(DOMAIN_MFORMS_BE);

namespace mforms {
  namespace gtk {

    class ImageBoxImpl : public ViewImpl {
    protected:
      mutable Gtk::Image _image;
      bool _scale;
      virtual Gtk::Widget *get_outer() const {
        return &_image;
      }

      ImageBoxImpl(::mforms::ImageBox *self) : ViewImpl(self) {
        //_image = Gtk::manage(new Gtk::Image());
        _image.set_alignment(0.5, 0.5);
        _scale = false;
        _image.signal_realize().connect(sigc::mem_fun(this, &ImageBoxImpl::on_realize));

        setup();
      }

      void on_realize() {
        if (_scale) {
          int w, h;
          int iw, ih;
          Glib::RefPtr<Gdk::Pixbuf> pb = _image.get_pixbuf();
          _image.get_size_request(w, h);
          if ((w > 0 || h > 0) && pb) {
            double ratio;
            iw = pb->get_width();
            ih = pb->get_height();
            ratio = (double)iw / ih;
            if (w < 0)
              pb = pb->scale_simple(h * ratio, h, Gdk::INTERP_BILINEAR);
            else if (h < 0)
              pb = pb->scale_simple(w, w / ratio, Gdk::INTERP_BILINEAR);
            else if (w > h)
              pb = pb->scale_simple(h / ratio, h, Gdk::INTERP_BILINEAR);
            else
              pb = pb->scale_simple(w, w / ratio, Gdk::INTERP_BILINEAR);
            _image.set(pb);
          }
        }
      }

      static bool create(::mforms::ImageBox *self) {
        return new ImageBoxImpl(self) != 0;
      }

      static void set_image(::mforms::ImageBox *self, const std::string &file) {
        ImageBoxImpl *image = self->get_data<ImageBoxImpl>();

        if (image) {
          std::string p = mforms::App::get()->get_resource_path(file);
          if (p.empty())
            g_warning("image %s not found", file.c_str());
          else
            image->_image.set(p);
        }
      }

      static void set_image_data(::mforms::ImageBox *self, const char *data, size_t length) {
        ImageBoxImpl *image = self->get_data<ImageBoxImpl>();
        if (image) {
          try {
            Glib::RefPtr<Gdk::PixbufLoader> loader(Gdk::PixbufLoader::create());
            loader->write((const guint8 *)data, length);
            loader->close();
            image->_image.set(loader->get_pixbuf());
          } catch (std::exception &e) {
            logError("Exception loading image data: %s\n", e.what());
            return;
          }
        }
      }

      static void set_scale_contents(::mforms::ImageBox *self, bool flag) {
        ImageBoxImpl *image = self->get_data<ImageBoxImpl>();

        if (image) {
          image->_scale = flag;
          if (flag) {
          }
        }
      }

      static void set_image_align(::mforms::ImageBox *self, ::mforms::Alignment alignment) {
        ImageBoxImpl *image = self->get_data<ImageBoxImpl>();

        if (image) {
          switch (alignment) {
            case mforms::BottomLeft:
              image->_image.set_alignment(0.0, 1.0);
              break;
            case mforms::MiddleLeft:
              image->_image.set_alignment(0.0, 0.5);
              break;
            case mforms::TopLeft:
              image->_image.set_alignment(0.0, 0.0);
              break;
            case mforms::BottomCenter:
              image->_image.set_alignment(0.5, 1.0);
              break;
            case mforms::TopCenter:
              image->_image.set_alignment(0.5, 0.0);
              break;
            case mforms::MiddleCenter:
              image->_image.set_alignment(0.5, 0.5);
              break;
            case mforms::BottomRight:
              image->_image.set_alignment(1.0, 1.0);
              break;
            case mforms::MiddleRight:
              image->_image.set_alignment(1.0, 0.5);
              break;
            case mforms::TopRight:
              image->_image.set_alignment(1.0, 0.0);
              break;
            case mforms::NoAlign:
              break;
          }
        }
      }

    public:
      static void init() {
        ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

        f->_imagebox_impl.create = &ImageBoxImpl::create;
        f->_imagebox_impl.set_image = &ImageBoxImpl::set_image;
        f->_imagebox_impl.set_image_data = &ImageBoxImpl::set_image_data;
        f->_imagebox_impl.set_scale_contents = &ImageBoxImpl::set_scale_contents;
        f->_imagebox_impl.set_image_align = &ImageBoxImpl::set_image_align;
      }
    };
  };
};

#endif /* _LF_IMAGEBOX_H_ */
