/* 
 * Copyright (c) 2007, 2012, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "mdc_image.h"
#include "mdc_image_manager.h"

using namespace mdc;
using namespace base;

ImageFigure::ImageFigure(Layer *layer)
: Figure(layer)
{
  _image= 0;
}


ImageFigure::~ImageFigure()
{
  cairo_surface_destroy(_image);
}



Size ImageFigure::get_image_size() const
{
  return Size(cairo_image_surface_get_width(_image), cairo_image_surface_get_height(_image));
}



Size ImageFigure::calc_min_size()
{
  Size size(1,1);
  if (_image && _auto_sizing)
  {
    size= get_image_size();
    size.width+= 2*_xpadding;
    size.height+= 2*_ypadding;
  }

  return size;
}


void ImageFigure::draw_contents(CairoCtx *cr)
{
  if (_image)
  {
    int w= cairo_image_surface_get_width(_image);
    int h= cairo_image_surface_get_height(_image);
    Point pos= get_position();
    
    pos.x= (get_size().width-w)/2;
    pos.y= (get_size().height-h)/2;

    pos.round();
    
    cr->save();
    //cr->translate(pos);
    cr->scale(get_size().width/w, get_size().height/h);
    cr->set_source_surface(_image, 0, 0);
    cr->paint();
    cr->restore();
  }
}


bool ImageFigure::set_image(cairo_surface_t *surface)
{
  if (_image != surface)
  {
    if (_image)
      cairo_surface_destroy(_image);
    _image= cairo_surface_reference(surface);
    set_size(get_image_size());
    _min_size_invalid= true;
    set_needs_relayout();
  }
  return true;
}


bool ImageFigure::set_image(const std::string &path)
{
  cairo_surface_t *image= ImageManager::get_instance()->get_image_nocache(path);
  if (image)
  {
    bool flag= set_image(image);
    cairo_surface_destroy(image);
    return flag;
  }
  return false;
}

