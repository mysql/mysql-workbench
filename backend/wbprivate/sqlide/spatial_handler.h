/*
 * Copyright (c) 2014, Oracle and/or its affiliates. All rights reserved.
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

#ifndef SPATIAL_HANDLER_H_
#define SPATIAL_HANDLER_H_

#include <gdal/ogrsf_frmts.h>
#include <gdal/ogr_api.h>
#include <gdal/gdal_pam.h>
#include <gdal/memdataset.h>
#include <gdal/gdal_alg.h>
#include <deque>
#include "base/geometry.h"

#include "mdc.h"

namespace GIS
{
enum ProjectionType
{
  ProjMercator = 1, ProjEquirectangular = 2, ProjRobinson = 3
};
enum ShapeType
{
  ShapeUnknown, ShapePoint, ShapeLineString, ShapeLinearRing, ShapePolygon
};

struct ShapeContainer
{
  ShapeType type;
  std::vector<base::Point> points;
  bool interrupt;
};

struct ProjectionView
{
  int width;
  int height;
  double MaxLat;
  double MaxLon;
  double MinLat;
  double MinLon;
  ProjectionType type;
};

class SpatialHandler
{
  OGRGeometry *poGeometry;
  std::string robinson_projection;
  std::string mercator_projection;
  std::string equirectangular_projection;
  std::string geodetic_wkt;
  double _adfProjection[6];
  double _invProjection[6];
  OGRCoordinateTransformation *geo_to_proj;
  OGRCoordinateTransformation *proj_to_geo;

  bool _interrupt;
private:

protected:
  void setupMatrix(ProjectionView &view);
  void extractPoints(OGRGeometry *shape,
      std::deque<ShapeContainer> &shapes_container, ProjectionType &projection);
  void convertPoints(std::vector<double> &x, std::vector<double> &y,
      ProjectionType &projection);
  ShapeContainer convertToShapeContainer(ShapeType type, std::vector<double> &x,
      std::vector<double> &y);
  char* getProjectionWkt(ProjectionType p);
public:
  SpatialHandler();
  int importFromMySQL(const std::string &data);
  int importFromWKT(std::string data);
  virtual ~SpatialHandler();
  int getOutput(ProjectionView &view,
      std::deque<ShapeContainer> &shapes_container);
  void toLatLng(int x, int y, double &lat,
      double &lon);
  void fromLatLng(double lat, double lon, double &x,
      double &y);

  void interrupt();
};

}



/* Spatial Object Model

 Feature - corresponds to the value of a single geometry column of a row in a resultset
 Identified by the layer (resultset) and row_id and may contain one or more attributes, which are the rest
 of the columns of the resultset.

 Layer - corresponds to a single resutset or data source to be displayed.
 Can be toggled to be shown or not.
 Contains a list of features that are part of that layer.
 Should allow identifying the feature that is located at a specific coordinate.
 */

namespace spatial
{
  class Layer;

  class Feature
  {
    Layer *_owner;
    int _row_id;
    GIS::SpatialHandler _geometry;
    std::deque<GIS::ShapeContainer> _shapes;

  public:
    Feature(Layer *layer, int row_id, const std::string &data, bool wkt);
    ~Feature();

    void interrupt();

    void render(GIS::ProjectionView &visible_area);
    void repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area);

    int row_id() const { return _row_id; }
  };

  class Layer
  {
    friend class Feature;

  protected:
    std::list<Feature*> _features;

    int _layer_id;
    base::Color _color;
    float _render_progress;
    bool _show;
    bool _interrupt;

  public:
    Layer(int layer_id, base::Color color);
    virtual ~Layer();

    virtual void process() {}

    void interrupt();

    bool hidden();
    int layer_id();

    void set_show(bool flag);

    size_t size() { return _features.size(); }

    void add_feature(int row_id, const std::string &geom_data, bool wkt);
    virtual void render(GIS::ProjectionView &visible_area);
    void repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area);
    float query_render_progress();
  };
};
#endif /* SPATIAL_HANDLER_H_ */
