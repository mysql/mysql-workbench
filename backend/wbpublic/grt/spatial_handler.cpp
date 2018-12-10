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

#include "spatial_handler.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include "base/log.h"

DEFAULT_LOG_DOMAIN("spatial");

#ifdef _MSC_VER
static void __stdcall ogr_error_handler(CPLErr eErrClass, int err_no, const char *msg) {
  logError("gdal error: %d, %s\n", err_no, msg);
}
#else
static void ogr_error_handler(CPLErr eErrClass, int err_no, const char *msg) {
  logError("gdal error: %d, %s\n", err_no, msg);
}
#endif

bool spatial::operator==(const ProjectionView &v1, const ProjectionView &v2) {
  return (v1.MaxLat == v2.MaxLat && v1.MaxLon == v2.MaxLon && v1.MinLat == v2.MinLat && v1.MinLon == v2.MinLon &&
          v1.height == v2.height && v1.width == v2.width);
}
bool spatial::operator!=(const ProjectionView &v1, const ProjectionView &v2) {
  return !(v1 == v2);
}

bool spatial::operator==(const Envelope &env1, const Envelope &env2) {
  return env1.bottom_right == env2.bottom_right && env1.top_left == env2.top_left;
}

bool spatial::operator!=(const Envelope &env1, const Envelope &env2) {
  return !(env1 == env2);
}

std::string spatial::stringFromErrorCode(const OGRErr &val) {
  switch (val) {
    case OGRERR_NOT_ENOUGH_DATA:
      return "Not enough data";
    case OGRERR_NOT_ENOUGH_MEMORY:
      return "Not enought memory";
    case OGRERR_UNSUPPORTED_GEOMETRY_TYPE:
      return "Unsupported geometry type";
    case OGRERR_UNSUPPORTED_OPERATION:
      return "Unsupported operation";
    case OGRERR_CORRUPT_DATA:
      return "Corrupt data";
    case OGRERR_FAILURE:
      return "Failure";
    case OGRERR_UNSUPPORTED_SRS:
      return "Unsupported SRS";
    case OGRERR_INVALID_HANDLE:
      return "Invalid handle";
    case OGRERR_NONE:
    default:
      return "";
  }
  return "";
}

std::string spatial::fetchAuthorityCode(const std::string &wkt) {
  if (wkt.empty()) {
    logError("Unable to fetch AuthorityCode, WKT was empty.");
    return "";
  }
  OGRSpatialReference srs;
  char *_wkt = (char *)const_cast<char *>(&(*(wkt.begin())));
  OGRErr err = srs.importFromWkt(&_wkt);
  if (err != OGRERR_NONE) {
    logError("ImportWKT Error: %s", stringFromErrorCode(err).c_str());
    return "";
  }

  err = srs.AutoIdentifyEPSG();
  if (err != OGRERR_NONE) {
    logError("AutoIdentifyEPSG Error: %s", stringFromErrorCode(err).c_str());
    return "";
  }
  return srs.GetAuthorityCode("GEOGCS");
}

spatial::Envelope::Envelope() : converted(false) {
  top_left.x = 180;
  top_left.y = -90;
  bottom_right.x = -180;
  bottom_right.y = 90;
}

spatial::Envelope::Envelope(double left, double top, double right, double bottom) : converted(false) {
  top_left.x = left;
  top_left.y = top;
  bottom_right.x = right;
  bottom_right.y = bottom;
  top_left.x = 180;
  top_left.y = -90;
  bottom_right.x = -180;
  bottom_right.y = 90;
}

bool spatial::Envelope::is_init() {
  return (top_left.x != 180 && top_left.y != -90 && bottom_right.x != -180 && bottom_right.y != 90);
}

bool spatial::Envelope::within(const base::Point &p) const {
  return (top_left.x <= p.x && top_left.y <= p.y && bottom_right.x >= p.x && bottom_right.y >= p.y);
}

double spatial::ShapeContainer::distance(const base::Point &p) const {
  switch (type) {
    case ShapePoint:
      return distance_point(p);
    case ShapeLineString:
      return distance_line(points, p);
    case ShapeLinearRing:
      return distance_linearring(p);
    case ShapePolygon:
      return distance_polygon(p);
    default:
      return -1;
  }
}

double spatial::ShapeContainer::distance_linearring(const base::Point &p) const {
  if (points.empty())
    return false;
  std::vector<base::Point> tmp = points;
  tmp.push_back(*tmp.begin());

  return distance_line(tmp, p);
}

// XXX see if all this code can be replaced with boost
static double distance_to_segment(const base::Point &start, const base::Point &end, const base::Point &p) {
  double dx = end.x - start.x;
  double dy = end.y - start.y;
  if (dx == 0 && dy == 0)
    return sqrt(pow(p.x - start.x, 2) + pow(p.y - start.y, 2));

  double dist_to_line = ((p.x - start.x) * dx + (p.y - start.y) * dy) / (pow(dx, 2) + pow(dy, 2));

  if (dist_to_line > 1) {
    dx = p.x - end.x;
    dy = p.y - end.y;
  } else if (dist_to_line < 0) {
    dx = p.x - start.x;
    dy = p.y - start.y;
  } else {
    dx = p.x - (start.x + dist_to_line * dx);
    dy = p.y - (start.y + dist_to_line * dy);
  }
  return sqrt(pow(dx, 2) + pow(dy, 2));
}

double spatial::ShapeContainer::distance_line(const std::vector<base::Point> &point_list, const base::Point &p) const {
  if (point_list.empty())
    return -1;

  std::vector<base::Point>::const_iterator it = point_list.begin(), it_tmp = point_list.begin();
  while (++it != point_list.end()) {
    try {
      return distance_to_segment(*it_tmp, *it, p);
    } catch (std::logic_error &) {
      // distance can raise Divide by zero exception, we silently skip this
    }

    it_tmp = it;
  }

  return -1;
}

double spatial::ShapeContainer::distance_polygon(const base::Point &p) const {
  if (points.empty())
    return -1;

  // first we check if we're in the bounding box cause maybe it's pointless to check all the polygon points
  if (!bounding_box.within(p))
    return -1;

  bool c = false;
  size_t i, j = 0;
  size_t nvert = points.size();
  for (i = 0, j = nvert - 1; i < nvert; j = i++) {
    if (((points[i].y > p.y) != (points[j].y > p.y)) &&
        (p.x < (points[j].x - points[i].x) * (p.y - points[i].y) / (points[j].y - points[i].y) + points[i].x))
      c = !c;
  }
  return c ? 0 : -1;
}

double spatial::ShapeContainer::distance_point(const base::Point &p) const {
  if (points.empty())
    return -1;

  return sqrt(pow((p.x - points[0].x), 2) + pow((p.y - points[0].y), 2));
}

spatial::ShapeContainer::ShapeContainer() : type(ShapeUnknown) {
}

std::string spatial::shape_description(ShapeType shp) {
  switch (shp) {
    case ShapePolygon:
      return "Polygon";
    case ShapeLinearRing:
      return "LinearRing";
    case ShapeLineString:
      return "LineString";
    case ShapePoint:
      return "Point";
    case ShapeUnknown:
    default:
      return "Unknown shape type";
  }
  return "";
}

spatial::ShapeType spatial::ogrTypeToWb(const OGRwkbGeometryType type)
{
  switch(type)
  {
    case wkbLineString:
      return ShapeLineString;

    case wkbLinearRing:
      return ShapeLinearRing;

    case wkbPolygon:
      return ShapePolygon;

    case wkbMultiPoint:
      return ShapeMultiPoint;

    case wkbMultiLineString:
      return ShapeMultiLineString;

    case wkbMultiPolygon:
      return ShapeMultiPolygon;

    case wkbGeometryCollection:
      return ShapeGeometryCollection;

    case wkbPoint:
      return ShapePoint;

    default:
      return ShapeUnknown;
  }
}

spatial::Projection::Projection()
{
  CPLSetErrorHandler(&ogr_error_handler);
  OGRRegisterAll();

  char *m_wkt = const_cast<char *>(
    "PROJCS[\"World_Mercator\", "
    "GEOGCS[\"GCS_WGS_1984\", "
    "DATUM[\"WGS_1984\", "
    "SPHEROID[\"WGS_1984\",6378137,298.257223563]], "
    "PRIMEM[\"Greenwich\",0], "
    "UNIT[\"Degree\",0.017453292519943295]], "
    "PROJECTION[\"Mercator_1SP\"], "
    "PARAMETER[\"False_Easting\",0], "
    "PARAMETER[\"False_Northing\",0], "
    "PARAMETER[\"Central_Meridian\",0], "
    "PARAMETER[\"Standard_Parallel_1\",0], "
    "UNIT[\"Meter\",1], "
    "AUTHORITY[\"EPSG\",\"54004\"]]");
  _mercator_srs.importFromWkt(&m_wkt);

  char *e_wkt = const_cast<char *>(
    "PROJCS[\"World_Equidistant_Cylindrical\","
    "GEOGCS[\"GCS_WGS_1984\","
    "DATUM[\"WGS_1984\","
    "SPHEROID[\"WGS_1984\",6378137,298.257223563]],"
    "PRIMEM[\"Greenwich\",0],"
    "UNIT[\"Degree\",0.017453292519943295]],"
    "PROJECTION[\"Equirectangular\"],"
    "PARAMETER[\"False_Easting\",0],"
    "PARAMETER[\"False_Northing\",0],"
    "PARAMETER[\"Central_Meridian\",0],"
    "PARAMETER[\"Standard_Parallel_1\",60],"
    "UNIT[\"Meter\",1],"
    "AUTHORITY[\"EPSG\",\"54002\"]]");
  _equirectangular_srs.importFromWkt(&e_wkt);

  char *r_wkt = const_cast<char *>(
    "PROJCS[\"World_Robinson\","
    "GEOGCS[\"GCS_WGS_1984\","
    "DATUM[\"WGS_1984\","
    "SPHEROID[\"WGS_1984\",6378137,298.257223563]],"
    "PRIMEM[\"Greenwich\",0],"
    "UNIT[\"Degree\",0.017453292519943295]],"
    "PROJECTION[\"Robinson\"],"
    "PARAMETER[\"False_Easting\",0],"
    "PARAMETER[\"False_Northing\",0],"
    "PARAMETER[\"Central_Meridian\",0],"
    "UNIT[\"Meter\",1],"
    "AUTHORITY[\"EPSG\",\"54030\"]]");
  _robinson_srs.importFromWkt(&r_wkt);

  char *g_wkt = const_cast<char *>(
    "GEOGCS[\"WGS 84\", "
    "DATUM[\"WGS_1984\", "
    "SPHEROID[\"WGS 84\",6378137,298.257223563, "
    "AUTHORITY[\"EPSG\",\"7030\"]], "
    "AUTHORITY[\"EPSG\",\"6326\"]], "
    "PRIMEM[\"Greenwich\",0, "
    "AUTHORITY[\"EPSG\",\"8901\"]], "
    "UNIT[\"degree\",0.01745329251994328, "
    "AUTHORITY[\"EPSG\",\"9122\"]], "
    "AUTHORITY[\"EPSG\",\"4326\"]]");
  _geodetic_srs.importFromWkt(&g_wkt);

  char *b_wkt = const_cast<char *>(
    "PROJCS[\"World_Bonne\", "
    "GEOGCS[\"GCS_WGS_1984\", "
    "DATUM[\"WGS_1984\", "
    "SPHEROID[\"WGS_1984\",6378137,298.257223563]], "
    "PRIMEM[\"Greenwich\",0], "
    "UNIT[\"Degree\",0.017453292519943295]], "
    "PROJECTION[\"Bonne\"], "
    "PARAMETER[\"False_Easting\",0], "
    "PARAMETER[\"False_Northing\",0], "
    "PARAMETER[\"Central_Meridian\",0], "
    "PARAMETER[\"Standard_Parallel_1\",60], "
    "UNIT[\"Meter\",1], "
    "AUTHORITY[\"EPSG\",\"54024\"]]");
  _bonne_srs.importFromWkt(&b_wkt);
}

spatial::Projection &spatial::Projection::get_instance() {
  static Projection instance;
  return instance;
}

bool spatial::Projection::check_libproj_availability() {
  OGRCoordinateTransformation *ref = OGRCreateCoordinateTransformation(&_geodetic_srs, &_robinson_srs);
  if (ref == NULL)
    return false;
  else {
    OCTDestroyCoordinateTransformation(ref);
    return true;
  }
}

OGRSpatialReference *spatial::Projection::get_projection(ProjectionType type) {
  switch (type) {
    case ProjMercator:
      return &_mercator_srs;
    case ProjEquirectangular:
      return &_equirectangular_srs;
    case ProjBonne:
      return &_bonne_srs;
    case ProjRobinson:
      return &_robinson_srs;
    case ProjGeodetic:
      return &_geodetic_srs;
    default:
      throw std::logic_error("Specified projection type is unsupported\n");
  }
}

void spatial::Importer::extract_points(OGRGeometry *shape, std::deque<ShapeContainer> &shapes_container) {
  OGRwkbGeometryType flat_type = wkbFlatten(shape->getGeometryType());

  if (flat_type == wkbPoint) {
    ShapeContainer container;
    container.type = ShapePoint;
    OGRPoint *point = (OGRPoint *)shape;
    container.points.push_back(base::Point(point->getX(), point->getY()));
    container.bounding_box.top_left = container.bounding_box.bottom_right = base::Point(point->getX(), point->getY());
    shapes_container.push_back(container);

  } else if (flat_type == wkbLineString) {
    ShapeContainer container;
    container.type = ShapeLineString;
    OGRLineString *line = (OGRLineString *)shape;
    OGREnvelope env;
    line->getEnvelope(&env);
    container.bounding_box.top_left.x = env.MinX;
    container.bounding_box.top_left.y = env.MaxY;
    container.bounding_box.bottom_right.x = env.MaxX;
    container.bounding_box.bottom_right.y = env.MinY;
    int nPoints = line->getNumPoints();

    container.points.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0 && !_interrupt; i--)
      container.points.push_back(base::Point(line->getX(i), line->getY(i)));

    shapes_container.push_back(container);

  } else if (flat_type == wkbLinearRing) {
    ShapeContainer container;
    container.type = ShapeLinearRing;
    OGRLinearRing *ring = (OGRLinearRing *)shape;
    OGREnvelope env;
    ring->getEnvelope(&env);
    container.bounding_box.top_left.x = env.MinX;
    container.bounding_box.top_left.y = env.MaxY;
    container.bounding_box.bottom_right.x = env.MaxX;
    container.bounding_box.bottom_right.y = env.MinY;
    int nPoints = ring->getNumPoints();
    container.points.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0 && !_interrupt; i--)
      container.points.push_back(base::Point(ring->getX(i), ring->getY(i)));

    shapes_container.push_back(container);

  } else if (flat_type == wkbPolygon) {
    ShapeContainer container;
    container.type = ShapePolygon;

    OGRPolygon *poly = (OGRPolygon *)shape;
    OGRLinearRing *ring = poly->getExteriorRing();
    int nPoints = ring->getNumPoints();
    container.points.reserve(nPoints);
    OGREnvelope env;
    ring->getEnvelope(&env);
    container.bounding_box.top_left.x = env.MinX;
    container.bounding_box.top_left.y = env.MaxY;
    container.bounding_box.bottom_right.x = env.MaxX;
    container.bounding_box.bottom_right.y = env.MinY;

    for (int i = nPoints - 1; i >= 0; i--)
      container.points.push_back(base::Point(ring->getX(i), ring->getY(i)));
    shapes_container.push_back(container);

    for (int i = 0; i < poly->getNumInteriorRings() && !_interrupt; ++i)
      extract_points(poly->getInteriorRing(i), shapes_container);

  } else if (flat_type == wkbMultiPoint || flat_type == wkbMultiLineString || flat_type == wkbMultiPolygon ||
             flat_type == wkbGeometryCollection) {
    OGRGeometryCollection *geoCollection = (OGRGeometryCollection *)shape;
    for (int i = 0; i < geoCollection->getNumGeometries() && !_interrupt; ++i)
      extract_points(geoCollection->getGeometryRef(i), shapes_container);
  }
}

void spatial::Importer::get_points(std::deque<ShapeContainer> &shapes_container) {
  if (_geometry)
    extract_points(_geometry, shapes_container);
}

void spatial::Importer::get_envelope(spatial::Envelope &env) {
  if (_geometry) {
    OGREnvelope ogr_env;
    _geometry->getEnvelope(&ogr_env);
    env.top_left.x = ogr_env.MinX;
    env.top_left.y = ogr_env.MaxY;
    env.bottom_right.x = ogr_env.MaxX;
    env.bottom_right.y = ogr_env.MinY;
  }
}

spatial::Importer::Importer() : _geometry(NULL), _interrupt(false), _srid(0) {
}

spatial::Importer::~Importer() {
  if (_geometry != NULL)
    CPLFree(_geometry);
}

OGRGeometry *spatial::Importer::steal_data() {
  OGRGeometry *tmp = _geometry;
  _geometry = NULL;
  return tmp;
}

int spatial::Importer::import_from_mysql(const std::string &data) {
  if (data.size() > 4) {
    // first 4 bytes is srid let's extract it:
    std::string tmp = data.substr(0, 4);
    _srid = tmp[3] << 24 | (tmp[2] & 0xff) << 16 | (tmp[1] & 0xff) << 8 | (tmp[0] & 0xff);

    OGRErr ret_val =
      OGRGeometryFactory::createFromWkb((unsigned char *)const_cast<char *>(&(*(data.begin() + 4))), NULL, &_geometry);

    if (_geometry)
      _geometry->assignSpatialReference(Projection::get_instance().get_projection(ProjGeodetic));

    if (ret_val == OGRERR_NONE)
      return 0;
  }
  return 1;
}

int spatial::Importer::import_from_wkt(std::string data) {
  char *d = &(*data.begin());
  OGRErr ret_val = OGRGeometryFactory::createFromWkt(&d, NULL, &_geometry);

  if (_geometry)
    _geometry->assignSpatialReference(Projection::get_instance().get_projection(ProjGeodetic));

  if (ret_val == OGRERR_NONE)
    return 0;
  else
    return 1;
}

int spatial::Importer::getSrid() const {
  return _srid;
}

std::string spatial::Importer::as_wkt() {
  char *data;
  if (_geometry) {
    OGRErr err;
    if ((err = _geometry->exportToWkt(&data)) != OGRERR_NONE) {
      logError("Error exporting data to WKT (%i)\n", err);
    } else {
      std::string tmp(data);
      CPLFree(data);
      return tmp;
    }
  }
  return "";
}

std::string spatial::Importer::as_kml() {
  char *data;
  if (_geometry) {
    if (!(data = _geometry->exportToKML())) {
      logError("Error exporting data to KML\n");
    } else {
      std::string tmp(data);
      CPLFree(data);
      return tmp;
    }
  }
  return "";
}

std::string spatial::Importer::as_json() {
  char *data;
  if (_geometry) {
    if (!(data = _geometry->exportToJson())) {
      logError("Error exporting data to JSON\n");
    } else {
      std::string tmp(data);
      CPLFree(data);
      return tmp;
    }
  }
  return "";
}

std::string spatial::Importer::as_gml() {
  char *data;
  if (_geometry) {
    if (!(data = _geometry->exportToGML())) {
      logError("Error exporting data to GML\n");
    } else {
      std::string tmp(data);
      CPLFree(data);
      return tmp;
    }
  }
  return "";
}

void spatial::Importer::interrupt() {
  _interrupt = true;
}

std::string spatial::Importer::getName() const
{
  if (_geometry)
    return std::string(_geometry->getGeometryName());

  return std::string();
}

spatial::ShapeType spatial::Importer::getType() const
{
  if (_geometry)
    return ogrTypeToWb(wkbFlatten(_geometry->getGeometryType()));

  return ShapeUnknown;
}

spatial::Converter::Converter(ProjectionView view, OGRSpatialReference *src_srs, OGRSpatialReference *dst_srs)
  : _geo_to_proj(NULL), _proj_to_geo(NULL), _source_srs(NULL), _target_srs(NULL), _interrupt(false) {
  change_projection(view, src_srs, dst_srs);
}

std::string spatial::Converter::dec_to_dms(double angle, AxisType axis, int precision) {
  const char *tmp = NULL;
  switch (axis) {
    case AxisLat:
      tmp = GDALDecToDMS(angle, "Lat", precision);
      break;
    case AxisLon:
      tmp = GDALDecToDMS(angle, "Long", precision);
      break;
    default:
      throw std::logic_error("Unknown axis type\n");
  }
  if (tmp)
    return tmp;
  return "";
}

spatial::Converter::~Converter() {
  base::RecMutexLock mtx(_projection_protector);
}

void spatial::Converter::change_projection(OGRSpatialReference *src_srs, OGRSpatialReference *dst_srs) {
  change_projection(_view, src_srs, dst_srs);
}

void spatial::Converter::change_projection(ProjectionView view, OGRSpatialReference *src_srs,
                                           OGRSpatialReference *dst_srs) {
  base::RecMutexLock mtx(_projection_protector);
  int recalculate = 0;

  if (view != _view) {
    _view = view;
    recalculate = 1;
  }

  if (src_srs != NULL && src_srs != _source_srs) {
    _source_srs = src_srs;
    recalculate = 2;
  }

  if (dst_srs != NULL && dst_srs != _target_srs) {
    _target_srs = dst_srs;
    recalculate = 2;
  }

  if (recalculate == 0)
    return;

  if (recalculate == 2) {
    if (_geo_to_proj != NULL)
      OCTDestroyCoordinateTransformation(_geo_to_proj);
    if (_proj_to_geo != NULL)
      OCTDestroyCoordinateTransformation(_proj_to_geo);

    _geo_to_proj = OGRCreateCoordinateTransformation(_source_srs, _target_srs);
    _proj_to_geo = OGRCreateCoordinateTransformation(_target_srs, _source_srs);
    if (!_geo_to_proj || !_proj_to_geo)
      throw std::logic_error("Unable to create coordinate transformation context.");
  }

  double minLat = _view.MinLat, maxLon = _view.MaxLon, maxLat = _view.MaxLat, minLon = _view.MinLon;

  if (!_geo_to_proj->Transform(1, &minLat, &maxLon, 0)) {
    char *proj4;
    _target_srs->exportToProj4(&proj4);
    logError("Unable to perform requested reprojection from WGS84, to %s\n", proj4);
    CPLFree(proj4);
  }

  if (!_geo_to_proj->Transform(1, &maxLat, &minLon, 0)) {
    char *proj4;
    _target_srs->exportToProj4(&proj4);
    logError("Unable to perform requested reprojection from WGS84, to %s\n", proj4);
    CPLFree(proj4);
  }

  _adf_projection[0] = minLat;
  _adf_projection[1] = (maxLat - minLat) / (double)_view.width;
  _adf_projection[2] = 0;
  _adf_projection[3] = maxLon;
  _adf_projection[4] = 0;
  _adf_projection[5] = -(maxLon - minLon) / (double)_view.height;
  if (!GDALInvGeoTransform(_adf_projection, _inv_projection))
    logError("Unable to invert equation\n");
}

void spatial::Converter::to_projected(int x, int y, double &lat, double &lon) {
  base::RecMutexLock mtx(_projection_protector);
  lat = _adf_projection[3] + (double)x * _adf_projection[4] + (double)y * _adf_projection[5];
  lon = _adf_projection[0] + (double)x * _adf_projection[1] + (double)y * _adf_projection[2];
}

void spatial::Converter::from_projected(double lat, double lon, int &x, int &y) {
  base::RecMutexLock mtx(_projection_protector);
  x = (int)(_inv_projection[0] + _inv_projection[1] * lat);
  y = (int)(_inv_projection[3] + _inv_projection[5] * lon);
}

bool spatial::Converter::to_latlon(int x, int y, double &lat, double &lon) {
  to_projected(x, y, lat, lon);
  // for lat/lon projection, coordinate order is reversed and it's lon/lat
  return from_proj_to_latlon(lon, lat);
}

bool spatial::Converter::from_latlon(double lat, double lon, int &x, int &y) {
  bool ret_val = from_latlon_to_proj(lon, lat);
  from_projected(lon, lat, x, y);
  return ret_val;
}

bool spatial::Converter::from_latlon_to_proj(double &lat, double &lon) {
  base::RecMutexLock mtx(_projection_protector);
  return _geo_to_proj->Transform(1, &lat, &lon) != 0;
}

bool spatial::Converter::from_proj_to_latlon(double &lat, double &lon) {
  base::RecMutexLock mtx(_projection_protector);
  return _proj_to_geo->Transform(1, &lat, &lon) != 0;
}

void spatial::Converter::transform_points(std::deque<ShapeContainer> &shapes_container) {
  std::deque<ShapeContainer>::iterator it;
  for (it = shapes_container.begin(); it != shapes_container.end() && !_interrupt; it++) {
    std::deque<size_t> for_removal;
    for (size_t i = 0; i < (*it).points.size() && !_interrupt; i++) {
      if (!_geo_to_proj->Transform(1, &(*it).points[i].x, &(*it).points[i].y))
        for_removal.push_back(i);
    }

    if (_geo_to_proj->Transform(1, &(*it).bounding_box.bottom_right.x, &(*it).bounding_box.bottom_right.y) &&
        _geo_to_proj->Transform(1, &(*it).bounding_box.top_left.x, &(*it).bounding_box.top_left.y)) {
      int x, y;
      from_projected((*it).bounding_box.bottom_right.x, (*it).bounding_box.bottom_right.y, x, y);
      (*it).bounding_box.bottom_right.x = x;
      (*it).bounding_box.bottom_right.y = y;
      from_projected((*it).bounding_box.top_left.x, (*it).bounding_box.top_left.y, x, y);
      (*it).bounding_box.top_left.x = x;
      (*it).bounding_box.top_left.y = y;
      (*it).bounding_box.converted = true;
    }

    if (!for_removal.empty())
      logDebug("%i points that could not be converted were skipped\n", (int)for_removal.size());

    std::deque<size_t>::reverse_iterator rit;
    for (rit = for_removal.rbegin(); rit != for_removal.rend() && !_interrupt; rit++)
      (*it).points.erase((*it).points.begin() + *rit);

    for (size_t i = 0; i < (*it).points.size() && !_interrupt; i++) {
      int x, y;
      from_projected((*it).points[i].x, (*it).points[i].y, x, y);
      (*it).points[i].x = x;
      (*it).points[i].y = y;
    }
  }
}

void spatial::Converter::transform_envelope(spatial::Envelope &env) {
  if (!env.is_init()) {
    logError("Can't transform empty envelope.\n");
    return;
  }

  if (_geo_to_proj->Transform(1, &env.top_left.x, &env.top_left.y) &&
      _geo_to_proj->Transform(1, &env.bottom_right.x, &env.bottom_right.y)) {
    int x, y;
    from_projected(env.bottom_right.x, env.bottom_right.y, x, y);
    env.bottom_right.x = x;
    env.bottom_right.y = y;
    from_projected(env.top_left.x, env.top_left.y, x, y);
    env.top_left.x = x;
    env.top_left.y = y;
    env.converted = true;
  } else {
    logError("Unable to transform envelope: %f, %f, %f, %f.\n", env.top_left.x, env.top_left.y, env.bottom_right.x,
             env.bottom_right.y);
  }
}

void spatial::Converter::interrupt() {
  _interrupt = true;
}

using namespace spatial;

Feature::Feature(Layer *layer, int row_id, const std::string &data, bool wkt = false) : _owner(layer), _row_id(row_id) {
  if (wkt)
    _geometry.import_from_wkt(data);
  else
    _geometry.import_from_mysql(data);
}

Feature::~Feature() {
}

void Feature::get_envelope(spatial::Envelope &env, const bool &screen_coords) {
  if (!screen_coords) {
    _geometry.get_envelope(env);
    return;
  }

  env = _env_screen;
}

void Feature::render(Converter *converter) {
  std::deque<ShapeContainer> tmp_shapes;
  _geometry.get_points(tmp_shapes);
  converter->transform_points(tmp_shapes);
  spatial::Envelope env;
  _geometry.get_envelope(env);
  converter->transform_envelope(env);
  _env_screen = env;

  _shapes = tmp_shapes;
}

double Feature::distance(const base::Point &p, const double &allowed_distance) {
  // we need to extend the envelope by allowed_distance cuase we don't want to make assumption based on number of shapes
  if (_env_screen.is_init()) {
    spatial::Envelope env = _env_screen;
    env.top_left.x -= allowed_distance;
    env.top_left.y -= allowed_distance;
    env.bottom_right.x += allowed_distance;
    env.bottom_right.y += allowed_distance;
    if (!env.within(p))
      return -1;
  }

  double rval = -1;
  for (std::deque<ShapeContainer>::const_iterator it = _shapes.begin(); it != _shapes.end() && !_owner->_interrupt;
       it++) {
    double dist = (*it).distance(p);
    if (dist < allowed_distance && dist != -1 && (dist < rval || rval == -1))
      rval = dist;
  }

  return rval;
}

void Feature::interrupt() {
  _geometry.interrupt();
}

void Feature::repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area, base::Color fill_color) {
  for (std::deque<ShapeContainer>::iterator it = _shapes.begin(); it != _shapes.end() && !_owner->_interrupt; it++) {
    if ((*it).points.empty()) {
      logError("%s is empty", shape_description(it->type).c_str());
      continue;
    }

    switch (it->type) {
      case ShapePolygon:
        cr.new_path();
        cr.move_to((*it).points[0]);
        for (size_t i = 1; i < (*it).points.size(); i++)
          cr.line_to((*it).points[i]);
        cr.close_path();
        if (fill_color.is_valid()) {
          cr.save();
          cr.set_color(fill_color);
          cr.fill_preserve();
          cr.restore();
        }
        cr.stroke();
        break;

      case ShapeLineString:
        cr.move_to((*it).points[0]);
        for (size_t i = 1; i < (*it).points.size(); i++)
          cr.line_to((*it).points[i]);
        cr.stroke();
        break;

      case ShapePoint:
        cr.save();
        // for points, we paint the marker at the exact position but reverse the scaling, so that the marker size is
        // constant
        cr.translate((*it).points[0]);
        cr.scale(1.0 / scale, 1.0 / scale);
        cr.rectangle(-5, -5, 5, 5);
        cr.fill();
        cr.restore();
        break;

      default:
        logDebug("Unknown type %i\n", it->type);
        break;
    }
  }
  cr.check_state();
}

static void extend_env(spatial::Envelope &env, const spatial::Envelope &env2) {
  env.top_left.x = MIN(env.top_left.x, env2.top_left.x);
  env.top_left.y = MAX(env.top_left.y, env2.top_left.y);
  env.bottom_right.x = MAX(env.bottom_right.x, env2.bottom_right.x);
  env.bottom_right.y = MIN(env.bottom_right.y, env2.bottom_right.y);
}

Layer::Layer(int layer_id, base::Color color) : _layer_id(layer_id), _color(color), _show(false), _interrupt(false) {
  _spatial_envelope.top_left.x = 180;
  _spatial_envelope.top_left.y = -90;
  _spatial_envelope.bottom_right.x = -180;
  _spatial_envelope.bottom_right.y = 90;
  _fill_polygons = false;
}

Layer::~Layer() {
  for (std::deque<Feature *>::iterator it = _features.begin(); it != _features.end(); ++it)
    delete *it;
}

void Layer::set_fill_polygons(bool fill) {
  _fill_polygons = fill;
}

bool Layer::get_fill_polygons() {
  return _fill_polygons;
}

void Layer::interrupt() {
  _interrupt = true;
  for (std::deque<Feature *>::iterator it = _features.begin(); it != _features.end(); ++it)
    (*it)->interrupt();
}

bool Layer::hidden() {
  return !_show;
}

int Layer::layer_id() {
  return _layer_id;
}

void Layer::set_show(bool flag) {
  _show = flag;
  if (flag)
    load_data();
}

void Layer::add_feature(int row_id, const std::string &geom_data, bool wkt) {
  spatial::Envelope env;
  Feature *feature = new Feature(this, row_id, geom_data, wkt);
  feature->get_envelope(env);
  extend_env(_spatial_envelope, env);
  _features.push_back(feature);
}

void Layer::repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area) {
  std::deque<ShapeContainer>::const_iterator it;

  cr.save();
  cr.set_line_width(0.5);
  base::Color color(_color); // darken colors
  color.red *= 0.6;
  color.green *= 0.6;
  color.blue *= 0.6;
  cr.set_color(color);
  for (std::deque<Feature *>::iterator it = _features.begin(); it != _features.end() && !_interrupt; ++it)
    (*it)->repaint(cr, scale, clip_area, _fill_polygons ? _color : base::Color::invalid());

  cr.restore();
}

float Layer::query_render_progress() {
  return _render_progress;
}

spatial::Envelope spatial::Layer::get_envelope() {
  return _spatial_envelope;
}

void Layer::render(Converter *converter) {
  _render_progress = 0.0;
  float step = 1.0f / _features.size();

  for (std::deque<spatial::Feature *>::iterator iter = _features.begin(); iter != _features.end() && !_interrupt;
       ++iter) {
    (*iter)->render(converter);
    _render_progress += step;
  }
}

spatial::Feature *Layer::feature_closest(const base::Point &p, const double &allowed_distance) {
  double rval = -1;
  spatial::Feature *f = NULL;
  for (std::deque<spatial::Feature *>::iterator iter = _features.begin(); iter != _features.end() && !_interrupt;
       ++iter) {
    double dist = (*iter)->distance(p, allowed_distance);
    if (dist < allowed_distance && dist != -1 && (dist < rval || rval == -1)) {
      rval = dist;
      f = (*iter);
    }
  }

  return f;
}

spatial::LayerId spatial::new_layer_id() {
  static LayerId id = 0;
  return ++id;
}
