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

#ifndef SPATIAL_HANDLER_H_
#define SPATIAL_HANDLER_H_

#include <ogrsf_frmts.h>
#include <ogr_api.h>
#include <gdal_pam.h>
#include <memdataset.h>
#include <gdal_alg.h>
#include <gdal.h>
#include <deque>
#include "base/geometry.h"
#include "wbpublic_public_interface.h"

#include "mdc.h"
/* Spatial Object Model

 Feature - corresponds to the value of a single geometry column of a row in a resultset
 Identified by the layer (resultset) and row_id and may contain one or more attributes, which are the rest
 of the columns of the resultset.

 Layer - corresponds to a single resutset or data source to be displayed.
 Can be toggled to be shown or not.
 Contains a list of features that are part of that layer.
 Should allow identifying the feature that is located at a specific coordinate.
 */

namespace spatial {

  std::string WBPUBLICBACKEND_PUBLIC_FUNC stringFromErrorCode(const OGRErr &val);
  std::string WBPUBLICBACKEND_PUBLIC_FUNC fetchAuthorityCode(const std::string &wkt);

  struct WBPUBLICBACKEND_PUBLIC_FUNC ProjectionView {
    int width;
    int height;
    double MaxLat;
    double MaxLon;
    double MinLat;
    double MinLon;
    friend bool operator==(const ProjectionView &v1, const ProjectionView &v2);
    friend bool operator!=(const ProjectionView &v1, const ProjectionView &v2);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC Envelope {
  public:
    Envelope();
    Envelope(double left, double top, double right, double bottom);
    bool converted;
    base::Point top_left;
    base::Point bottom_right;
    friend bool operator==(const Envelope &env1, const Envelope &env2);
    friend bool operator!=(const Envelope &env1, const Envelope &env2);
    bool is_init();
    bool within(const base::Point &p) const;
  };

  bool operator==(const ProjectionView &v1, const ProjectionView &v2);
  bool operator!=(const ProjectionView &v1, const ProjectionView &v2);
  bool operator==(const Envelope &env1, const Envelope &env2);
  bool operator!=(const Envelope &env1, const Envelope &env2);

  enum ProjectionType { ProjMercator = 1, ProjEquirectangular = 2, ProjRobinson = 3, ProjBonne = 4, ProjGeodetic = 5 };

  enum ShapeType {
    ShapeUnknown, ShapePoint, ShapeLineString, ShapeLinearRing, ShapePolygon, ShapeMultiPoint,
    ShapeMultiLineString, ShapeMultiPolygon, ShapeGeometryCollection
  };

  std::string shape_description(ShapeType shp);
  ShapeType ogrTypeToWb(const OGRwkbGeometryType type);

  enum AxisType { AxisLat = 1, AxisLon = 2 };

  class WBPUBLICBACKEND_PUBLIC_FUNC ShapeContainer {
  protected:
    double distance_linearring(const base::Point &p) const;
    double distance_line(const std::vector<base::Point> &point_list, const base::Point &p) const;
    double distance_polygon(const base::Point &p) const;
    double distance_point(const base::Point &p) const;

  public:
    ShapeContainer();
    ShapeType type;
    std::vector<base::Point> points;
    Envelope bounding_box;
    double distance(const base::Point &p) const;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC Projection {
  protected:
    OGRSpatialReference _mercator_srs;
    OGRSpatialReference _equirectangular_srs;
    OGRSpatialReference _robinson_srs;
    OGRSpatialReference _geodetic_srs;
    OGRSpatialReference _bonne_srs;

  public:
    static Projection &get_instance();
    bool check_libproj_availability();
    OGRSpatialReference *get_projection(ProjectionType);

  private:
    Projection();
    Projection(Projection const &);
    void operator=(Projection const &);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC Importer {
    OGRGeometry *_geometry;
    bool _interrupt;
    void extract_points(OGRGeometry *shape, std::deque<ShapeContainer> &shapes_container);
    int _srid;

  public:
    Importer();
    ~Importer();
    int import_from_mysql(const std::string &data);
    int import_from_wkt(std::string data);
    void get_points(std::deque<ShapeContainer> &shapes_container);
    void get_envelope(Envelope &env);
    void interrupt();
    std::string getName() const;
    ShapeType getType() const;

    int getSrid() const;
    std::string as_wkt();
    std::string as_kml();
    std::string as_json();
    std::string as_gml();

    OGRGeometry *steal_data();
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC Converter {
    base::RecMutex _projection_protector;
    double _adf_projection[6];
    double _inv_projection[6];
    OGRCoordinateTransformation *_geo_to_proj;
    OGRCoordinateTransformation *_proj_to_geo;
    OGRSpatialReference *_source_srs;
    OGRSpatialReference *_target_srs;
    ProjectionView _view;
    bool _interrupt;

  public:
    Converter(ProjectionView view, OGRSpatialReference *src_srs, OGRSpatialReference *dst_srs);
    ~Converter();
    void change_projection(OGRSpatialReference *src_srs = NULL, OGRSpatialReference *dst_srs = NULL);
    void change_projection(ProjectionView view, OGRSpatialReference *src_srs = NULL,
                           OGRSpatialReference *dst_srs = NULL);
    void from_projected(double lat, double lon, int &x, int &y);
    void to_projected(int x, int y, double &lat, double &lon);

    bool to_latlon(int x, int y, double &lat, double &lon);
    bool from_latlon(double lat, double lon, int &x, int &y);

    bool from_latlon_to_proj(double &lat, double &lon);
    bool from_proj_to_latlon(double &lat, double &lon);
    static std::string dec_to_dms(double angle, AxisType axis, int precision);
    void transform_points(std::deque<ShapeContainer> &shapes_container);
    void transform_envelope(spatial::Envelope &env);
    void interrupt();
  };

  class Layer;

  class WBPUBLICBACKEND_PUBLIC_FUNC Feature {
    Layer *_owner;
    int _row_id;
    Importer _geometry;
    std::deque<ShapeContainer> _shapes;
    spatial::Envelope _env_screen;

  public:
    Feature(Layer *layer, int row_id, const std::string &data, bool wkt);
    ~Feature();

    void interrupt();
    void get_envelope(spatial::Envelope &env, const bool &screen_coords = false);
    void render(spatial::Converter *converter);
    void repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area,
                 base::Color fill_color = base::Color::invalid());

    int row_id() const {
      return _row_id;
    }
    double distance(const base::Point &p, const double &allowed_distance = 4.0);
  };

  typedef int LayerId;
  WBPUBLICBACKEND_PUBLIC_FUNC LayerId new_layer_id();

  class WBPUBLICBACKEND_PUBLIC_FUNC Layer {
    friend class Feature;

  protected:
    std::deque<Feature *> _features;

    LayerId _layer_id;
    base::Color _color;
    float _render_progress;
    bool _show;
    bool _interrupt;
    spatial::Envelope _spatial_envelope;
    bool _fill_polygons;

  public:
    Layer(LayerId layer_id, base::Color color);
    virtual ~Layer();

    virtual void load_data() {
    }

    void interrupt();

    bool hidden();
    LayerId layer_id();

    void set_show(bool flag);

    size_t size() {
      return _features.size();
    }

    base::Color color() {
      return _color;
    }
    bool fill() {
      return _fill_polygons;
    }

    void add_feature(int row_id, const std::string &geom_data, bool wkt);
    virtual void render(spatial::Converter *converter);
    spatial::Feature *feature_closest(const base::Point &p, const double &allowed_distance = 4.0);
    void set_fill_polygons(bool fill);
    bool get_fill_polygons();
    virtual void repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area);
    float query_render_progress();
    spatial::Envelope get_envelope();
  };
};
#endif /* SPATIAL_HANDLER_H_ */
