/*
 * Copyright (c) 2014, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "geom_draw_box.h"
#include "base/log.h"

#include "grt/spatial_handler.h"

DEFAULT_LOG_DOMAIN("GeomDrawBox");

void GeomDrawBox::draw_ring(cairo_t *cr, OGRRawPoint *points, int num_points, double scale, double x, double y,
                            double height) {
  cairo_move_to(cr, (points[0].x - x) * scale, height - (points[0].y - y) * scale);
  for (int i = 1; i < num_points; i++) {
    cairo_line_to(cr, (points[i].x - x) * scale, height - (points[i].y - y) * scale);
  }
}

void GeomDrawBox::draw_ring_vertices(cairo_t *cr, OGRRawPoint *points, int num_points, double scale, double x, double y,
                                     double height) {
  cairo_arc(cr, (points[0].x - x) * scale, height - (points[0].y - y) * scale, 2, 0, 2 * M_PI);
  cairo_fill(cr);
  for (int i = 1; i < num_points; i++) {
    cairo_arc(cr, (points[i].x - x) * scale, height - (points[i].y - y) * scale, 2, 0, 2 * M_PI);
    cairo_fill(cr);
  }
}

void GeomDrawBox::draw_polygon(cairo_t *cr, OGRPolygon *poly, double scale, double x, double y, double height) {
  const OGRLinearRing *ring = poly->getExteriorRing();
  if (ring->getNumPoints() > 0) {
    OGRRawPoint *points = new OGRRawPoint[ring->getNumPoints()];
    ring->getPoints(points);

    draw_ring(cr, points, ring->getNumPoints(), scale, x, y, height);
    cairo_set_line_width(cr, 1);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke_preserve(cr);
    cairo_set_source_rgb(cr, 0.8, 0.8, 0.8);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 0, 0);
    draw_ring_vertices(cr, points, ring->getNumPoints(), scale, x, y, height);

    delete[] points;
  }
}

void GeomDrawBox::set_data(const std::string &text) {
  spatial::Importer importer;
  importer.import_from_mysql(text);
  _srid = importer.getSrid();
  _geom = importer.steal_data();
  set_needs_repaint();
}

int GeomDrawBox::getSrid() const {
  return _srid;
}

void GeomDrawBox::draw_geometry(cairo_t *cr, OGRGeometry *geom, double scale, double x, double y, double height) {
  switch (geom->getGeometryType()) {
    case wkbPolygon:
      draw_polygon(cr, dynamic_cast<OGRPolygon *>(geom), scale, x, y, height);
      break;
    case wkbMultiPolygon: {
      OGRGeometryCollection *geoCollection = dynamic_cast<OGRGeometryCollection *>(geom);
      for (int i = 0; i < geoCollection->getNumGeometries(); ++i)
        draw_geometry(cr, geoCollection->getGeometryRef(i), scale, x, y, height);
    } break;
    default:
      logWarning("Can't paint geometry type %s\n", geom->getGeometryName());
      break;
  }
}

void GeomDrawBox::repaint(cairo_t *cr, int x, int y, int w, int h) {
  if (_geom) {
    OGREnvelope env;
    _geom->getEnvelope(&env);

    double fig_width = env.MaxX - env.MinX;
    double fig_height = env.MaxY - env.MinY;
    double scale;
    int padding = 5;

    if (fig_width > fig_height)
      scale = (get_width() - padding * 2) / fig_width;
    else
      scale = (get_height() - padding * 2) / fig_height;

    cairo_translate(cr, padding, padding);

    draw_geometry(cr, _geom, scale, env.MinX, env.MinY, get_height() - padding * 2);
  }
}
