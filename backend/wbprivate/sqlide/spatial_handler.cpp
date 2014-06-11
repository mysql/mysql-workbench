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

#include "sqlide/spatial_handler.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include "base/log.h"

DEFAULT_LOG_DOMAIN("spatial");
//
//static void ogr_error_handler(CPLErr eErrClass, int err_no, const char *msg)
//{
//  log_error("ERROR %d, %s\n", err_no, msg);
//}
//
//GIS::SpatialHandler::SpatialHandler() :
//    _geometry(NULL), _geo_to_proj(NULL), _proj_to_geo(NULL), _interrupt(false)
//{
//  _robinson_projection = "PROJCS[\"World_Robinson\","
//      "GEOGCS[\"GCS_WGS_1984\","
//      "DATUM[\"WGS_1984\","
//      "SPHEROID[\"WGS_1984\",6378137,298.257223563]],"
//      "PRIMEM[\"Greenwich\",0],"
//      "UNIT[\"Degree\",0.017453292519943295]],"
//      "PROJECTION[\"Robinson\"],"
//      "PARAMETER[\"False_Easting\",0],"
//      "PARAMETER[\"False_Northing\",0],"
//      "PARAMETER[\"Central_Meridian\",0],"
//      "UNIT[\"Meter\",1],"
//      "AUTHORITY[\"EPSG\",\"54030\"]]";
//   _mercator_projection = "PROJCS[\"World_Mercator\", "
//      "GEOGCS[\"GCS_WGS_1984\", "
//      "DATUM[\"WGS_1984\", "
//      "SPHEROID[\"WGS_1984\",6378137,298.257223563]], "
//      "PRIMEM[\"Greenwich\",0], "
//      "UNIT[\"Degree\",0.017453292519943295]], "
//      "PROJECTION[\"Mercator_1SP\"], "
//      "PARAMETER[\"False_Easting\",0], "
//      "PARAMETER[\"False_Northing\",0], "
//      "PARAMETER[\"Central_Meridian\",0], "
//      "PARAMETER[\"Standard_Parallel_1\",0], "
//      "UNIT[\"Meter\",1], "
//      "AUTHORITY[\"EPSG\",\"54004\"]]";
//  _equirectangular_projection = "PROJCS[\"World_Equidistant_Cylindrical\","
//      "GEOGCS[\"GCS_WGS_1984\","
//      "DATUM[\"WGS_1984\","
//      "SPHEROID[\"WGS_1984\",6378137,298.257223563]],"
//      "PRIMEM[\"Greenwich\",0],"
//      "UNIT[\"Degree\",0.017453292519943295]],"
//      "PROJECTION[\"Equirectangular\"],"
//      "PARAMETER[\"False_Easting\",0],"
//      "PARAMETER[\"False_Northing\",0],"
//      "PARAMETER[\"Central_Meridian\",0],"
//      "PARAMETER[\"Standard_Parallel_1\",60],"
//      "UNIT[\"Meter\",1],"
//      "AUTHORITY[\"EPSG\",\"54002\"]]";
//  _bonne_projection = "PROJCS[\"World_Bonne\", "
//      "GEOGCS[\"GCS_WGS_1984\", "
//      "DATUM[\"WGS_1984\", "
//      "SPHEROID[\"WGS_1984\",6378137,298.257223563]], "
//      "PRIMEM[\"Greenwich\",0], "
//      "UNIT[\"Degree\",0.017453292519943295]], "
//      "PROJECTION[\"Bonne\"], "
//      "PARAMETER[\"False_Easting\",0], "
//      "PARAMETER[\"False_Northing\",0], "
//      "PARAMETER[\"Central_Meridian\",0], "
//      "PARAMETER[\"Standard_Parallel_1\",60], "
//      "UNIT[\"Meter\",1], "
//      "AUTHORITY[\"EPSG\",\"54024\"]]";
//
//  _geodetic_wkt = "GEOGCS[\"WGS 84\", "
//      "DATUM[\"WGS_1984\", "
//      "SPHEROID[\"WGS 84\",6378137,298.257223563, "
//      "AUTHORITY[\"EPSG\",\"7030\"]], "
//          "AUTHORITY[\"EPSG\",\"6326\"]], "
//          "PRIMEM[\"Greenwich\",0, "
//          "AUTHORITY[\"EPSG\",\"8901\"]], "
//          "UNIT[\"degree\",0.01745329251994328, "
//          "AUTHORITY[\"EPSG\",\"9122\"]], "
//          "AUTHORITY[\"EPSG\",\"4326\"]]";
//  CPLSetErrorHandler(&ogr_error_handler);
//}
//
//
//void GIS::SpatialHandler::interrupt()
//{
//  _interrupt = true;
//}
//
//
//int GIS::SpatialHandler::import_from_mysql(const std::string &data)
//{
//  unsigned char* geom = new unsigned char[data.size() - 3];
//  std::copy(data.begin() + 4, data.end(), geom);
//
//  OGRErr ret_val = OGRGeometryFactory::createFromWkb(geom, NULL, &_geometry);
//  delete[] geom;
//
//  if (_geometry)
//  {
//    OGRSpatialReference hDstSRS;
//
//    char *wkt = &(*_geodetic_wkt.begin());
//    hDstSRS.importFromWkt(&wkt);
//
//    _geometry->assignSpatialReference(&hDstSRS);
//
//  }
//  if (ret_val == OGRERR_NONE)
//    return 0;
//  else
//    return 1;
//}
//
//
//int GIS::SpatialHandler::import_from_wkt(std::string data)
//{
//  char *d = &(*data.begin());
//  OGRErr ret_val = OGRGeometryFactory::createFromWkt(&d, NULL, &_geometry);
//
//  if (_geometry)
//  {
//    OGRSpatialReference hDstSRS;
//
//    char *wkt = &(*_geodetic_wkt.begin());
//    hDstSRS.importFromWkt(&wkt);
//
//    _geometry->assignSpatialReference(&hDstSRS);
//  }
//  if (ret_val == OGRERR_NONE)
//    return 0;
//  else
//    return 1;
//}
//
//void GIS::SpatialHandler::setup_matrix(ProjectionView &view)
//{
//  OGRSpatialReference sourceSRS;
//  OGRSpatialReference targetSRS;
//  char *wkt_src = &(*_geodetic_wkt.begin());
//  sourceSRS.importFromWkt(&wkt_src);
//
//  char *wkt_dst = get_projection_wkt(view.type);
//  targetSRS.importFromWkt(&wkt_dst);
//
//  if (_geo_to_proj)
//    OCTDestroyCoordinateTransformation(_geo_to_proj);
//  if (_proj_to_geo)
//      OCTDestroyCoordinateTransformation(_proj_to_geo);
//
//  _geo_to_proj = OGRCreateCoordinateTransformation(&sourceSRS, &targetSRS);
//  _proj_to_geo = OGRCreateCoordinateTransformation(&targetSRS, &sourceSRS);
//  if (!_geo_to_proj || !_proj_to_geo)
//    throw std::logic_error("Unable to perform specified transformation.\n");
//
//
//
//  double minLat = view.MinLat, maxLon = view.MaxLon, maxLat = view.MaxLat, minLon = view.MinLon;
//
//  if (!_geo_to_proj->Transform(1, &minLat, &maxLon, 0))
//  {
//    char * proj4;
//    targetSRS.exportToProj4(&proj4);
//    log_error("Unable to perform requested reprojection from WGS84, to %s\n", proj4);
//    CPLFree(proj4);
//  }
//
//  if (!_geo_to_proj->Transform(1, &maxLat, &minLon, 0))
//  {
//    char * proj4;
//    targetSRS.exportToProj4(&proj4);
//    log_error("Unable to perform requested reprojection from WGS84, to %s\n", proj4);
//    CPLFree(proj4);
//  }
//
//
//  _adf_projection[0] = minLat;
//  _adf_projection[1] = (maxLat - minLat) / (double)view.width;
//  _adf_projection[2] = 0;
//  _adf_projection[3] = maxLon;
//  _adf_projection[4] = 0;
//  _adf_projection[5] = -(maxLon - minLon) / (double)view.height;
//  GDALInvGeoTransform(_adf_projection, _inv_projection);
//
//}
//
//void GIS::SpatialHandler::convert_points(std::vector<double> &x,
//    std::vector<double> &y, ProjectionType &projection)
//{
//  std::deque<size_t> for_removal;
//  for (size_t i =0; i< x.size() && !_interrupt; i++)
//  {
//    if(!_geo_to_proj->Transform(1, &x[i], &y[i]))
//      for_removal.push_back(i);
//  }
//
//  std::deque<size_t>::reverse_iterator rit;
//  for (rit = for_removal.rbegin(); rit < for_removal.rend(); rit++)
//  {
//    x.erase(x.begin() + *rit);
//    y.erase(y.begin() + *rit);
//  }
//
//
//  for(size_t i=0; i < x.size() && !_interrupt; i++)
//  {
//    from_latlon(x[i], y[i], x[i], y[i]);
//  }
//}
//
//GIS::ShapeContainer GIS::SpatialHandler::convert_to_shape_container(ShapeType type,
//    std::vector<double> &x, std::vector<double> &y)
//{
//  ShapeContainer container;
//  container.type = type;
//  for (size_t i = 0; i < x.size() && !_interrupt; ++i)
//  {
//    container.points.push_back(base::Point(x[i], y[i]));
//  }
//  return container;
//}
//
//char* GIS::SpatialHandler::get_projection_wkt(ProjectionType p)
//{
//  char *projection = NULL;
//  switch (p)
//  {
//  case ProjMercator:
//    projection = &(*_mercator_projection.begin());
//    break;
//  case ProjRobinson:
//    projection = &(*_robinson_projection.begin());
//    break;
//  case ProjEquirectangular:
//    projection = &(*_equirectangular_projection.begin());
//    break;
//  case ProjBonne:
//    projection = &(*_bonne_projection.begin());
//    break;
//  default:
//    throw std::logic_error("GIS: Unknown projection type\n");
//  }
//
//  return projection;
//}
//
//void GIS::SpatialHandler::extract_points(OGRGeometry *shape,
//    std::deque<ShapeContainer> &shapes_container, ProjectionType &projection)
//{
//  OGRwkbGeometryType flat_type = wkbFlatten(shape->getGeometryType());
//
//  if (flat_type == wkbPoint)
//  {
//    OGRPoint *point = (OGRPoint*) shape;
//    std::vector<double> aPointX;
//    std::vector<double> aPointY;
//    aPointX.push_back(point->getX());
//    aPointY.push_back(point->getY());
//    convert_points(aPointX, aPointY, projection);
//    shapes_container.push_back(
//        convert_to_shape_container(ShapePoint, aPointX, aPointY));
//
//  } else if (flat_type == wkbLineString)
//  {
//    OGRLineString *line = (OGRLineString*) shape;
//    int nPoints = line->getNumPoints();
//    std::vector<double> aPointX;
//    std::vector<double> aPointY;
//    aPointX.reserve(nPoints);
//    aPointY.reserve(nPoints);
//    for (int i = nPoints - 1; i >= 0 && !_interrupt; i--)
//    {
//      aPointX.push_back(line->getX(i));
//      aPointY.push_back(line->getY(i));
//    }
//    convert_points(aPointX, aPointY, projection);
//    if (!aPointX.empty() && !aPointY.empty())
//    {
//      shapes_container.push_back(
//          convert_to_shape_container(ShapeLineString, aPointX, aPointY));
//    }
//
//  } else if (flat_type == wkbLinearRing)
//  {
//    OGRLinearRing *ring = (OGRLinearRing *) shape;
//    int nPoints = ring->getNumPoints();
//    std::vector<double> aPointX;
//    std::vector<double> aPointY;
//    aPointX.reserve(nPoints);
//    aPointY.reserve(nPoints);
//    for (int i = nPoints - 1; i >= 0 && !_interrupt; i--)
//    {
//      aPointX.push_back(ring->getX(i));
//      aPointY.push_back(ring->getY(i));
//    }
//
//    convert_points(aPointX, aPointY, projection);
//    if (!aPointX.empty() && !aPointY.empty())
//    {
//      shapes_container.push_back(
//          convert_to_shape_container(ShapeLinearRing, aPointX, aPointY));
//    }
//
//  } else if (flat_type == wkbPolygon)
//  {
//    OGRPolygon *poly = (OGRPolygon*) shape;
//    OGRLinearRing *ring = poly->getExteriorRing();
//    int nPoints = ring->getNumPoints();
//    std::vector<double> aPointX;
//    std::vector<double> aPointY;
//    aPointX.reserve(nPoints);
//    aPointY.reserve(nPoints);
//    for (int i = nPoints - 1; i >= 0; i--)
//    {
//      aPointX.push_back(ring->getX(i));
//      aPointY.push_back(ring->getY(i));
//    }
//    convert_points(aPointX, aPointY, projection);
//    if (!aPointX.empty() && !aPointY.empty())
//    {
//      shapes_container.push_back(
//          convert_to_shape_container(ShapePolygon, aPointX, aPointY));
//    }
//    for (int i = 0; i < poly->getNumInteriorRings() && !_interrupt; ++i)
//      extract_points(poly->getInteriorRing(i), shapes_container, projection);
//
//  } else if (flat_type == wkbMultiPoint || flat_type == wkbMultiLineString
//      || flat_type == wkbMultiPolygon || flat_type == wkbGeometryCollection)
//  {
//    OGRGeometryCollection *geoCollection = (OGRGeometryCollection*) shape;
//    for (int i = 0; i < geoCollection->getNumGeometries() && !_interrupt; ++i)
//      extract_points(geoCollection->getGeometryRef(i), shapes_container,
//          projection);
//  }
//}
//
//int GIS::SpatialHandler::get_output(ProjectionView &view,
//    std::deque<ShapeContainer> &shapes_container)
//{
//  switch (view.type)
//  {
//  case ProjMercator:
//  case ProjRobinson:
//  case ProjEquirectangular:
//  case ProjBonne:
//    break;
//  default:
//    throw std::logic_error("Unknown type of projection");
//  }
//
//  this->setup_matrix(view);
//
//  extract_points(_geometry, shapes_container, view.type);
//
//  return 0;
//}
//
//
//
//
//void GIS::SpatialHandler::to_latlon(int x, int y,
//    double &lat, double &lon)
//{
//  if (_geo_to_proj)
//  {
//    lat = _adf_projection[3] + (double) x * _adf_projection[4] + (double) y * _adf_projection[5];
//    lon = _adf_projection[0] + (double) x * _adf_projection[1] + (double) y * _adf_projection[2];
//    _proj_to_geo->Transform(1, &lat, &lon);
//  }
//}
//
//void GIS::SpatialHandler::from_latlon(double lat,
//    double lon, double &x, double &y)
//{
//  if (_geo_to_proj)
//  {
//    x = _inv_projection[0] + _inv_projection[1] * lat;
//    y = _inv_projection[3] + _inv_projection[5] * lon;
//  }
//
//}
//
//GIS::SpatialHandler::~SpatialHandler()
//{
//  if (_geo_to_proj)
//    OCTDestroyCoordinateTransformation(_geo_to_proj);
//  if (_proj_to_geo)
//    OCTDestroyCoordinateTransformation(_proj_to_geo);
//
//  // TODO Auto-generated destructor stub
//}


static void ogr_error_handler(CPLErr eErrClass, int err_no, const char *msg)
{
  log_error("ERROR %d, %s\n", err_no, msg);
}

spatial::Projection::Projection() : _is_projected(false), _name("Geodetic")
{
  _wkt = "GEOGCS[\"WGS 84\", "
    "DATUM[\"WGS_1984\", "
    "SPHEROID[\"WGS 84\",6378137,298.257223563, "
    "AUTHORITY[\"EPSG\",\"7030\"]], "
        "AUTHORITY[\"EPSG\",\"6326\"]], "
        "PRIMEM[\"Greenwich\",0, "
        "AUTHORITY[\"EPSG\",\"8901\"]], "
        "UNIT[\"degree\",0.01745329251994328, "
        "AUTHORITY[\"EPSG\",\"9122\"]], "
        "AUTHORITY[\"EPSG\",\"4326\"]]";
}

char* spatial::Projection::get_wkt()
{
  return &(*_wkt.begin());
}

bool spatial::Projection::is_projected()
{
  return _is_projected;
}

const char* spatial::Projection::to_string()
{
  return _name.c_str();
}

spatial::Projection::Mercator::Mercator()
{
  _is_projected = true;
  _name = "Mercator";

  _wkt = "PROJCS[\"World_Mercator\", "
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
    "AUTHORITY[\"EPSG\",\"54004\"]]";
}

spatial::Projection::Equirectangular::Equirectangular()
{
   _is_projected = true;
  _name = "Equirectangular";

  _wkt = "PROJCS[\"World_Equidistant_Cylindrical\","
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
    "AUTHORITY[\"EPSG\",\"54002\"]]";
}

spatial::Projection::Robinson::Robinson()
{
  _is_projected = true;
  _name = "Robinson";

  _wkt = "PROJCS[\"World_Robinson\","
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
    "AUTHORITY[\"EPSG\",\"54030\"]]";
}

spatial::Projection::Bonne::Bonne()
{
  _is_projected = true;
  _name = "Bonne";

  _wkt = "PROJCS[\"World_Bonne\", "
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
    "AUTHORITY[\"EPSG\",\"54024\"]]";
}

spatial::Projection spatial::ProjectionFactory::get_projection(ProjectionType type)
{
  switch(type)
    {
    case ProjMercator:
      return Projection::Mercator();

    case ProjEquirectangular:
      return Projection::Equirectangular();

    case ProjBonne:
      return Projection::Bonne();

    case ProjRobinson:
      return Projection::Robinson();

    case ProjGeodetic:
      return Projection();

    default:
      throw std::logic_error("Specified projection type is unsupported\n");
    }
}


void spatial::Importer::extract_points(OGRGeometry *shape, std::deque<ShapeContainer> &shapes_container)
{
  OGRwkbGeometryType flat_type = wkbFlatten(shape->getGeometryType());

  if (flat_type == wkbPoint)
  {
    ShapeContainer container;
    container.type = ShapePoint;
    OGRPoint *point = (OGRPoint*)shape;
    container.points.push_back(base::Point(point->getX(), point->getY()));
    shapes_container.push_back(container);

  }
  else if (flat_type == wkbLineString)
  {
    ShapeContainer container;
    container.type = ShapeLineString;
    OGRLineString *line = (OGRLineString*) shape;
    int nPoints = line->getNumPoints();

    container.points.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0 && !_interrupt; i--)
      container.points.push_back(base::Point(line->getX(i), line->getY(i)));

    shapes_container.push_back(container);

  }
  else if (flat_type == wkbLinearRing)
  {
    ShapeContainer container;
    container.type = ShapeLinearRing;
    OGRLinearRing *ring = (OGRLinearRing *) shape;
    int nPoints = ring->getNumPoints();
    container.points.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0 && !_interrupt; i--)
      container.points.push_back(base::Point(ring->getX(i), ring->getY(i)));

    shapes_container.push_back(container);

  }
  else if (flat_type == wkbPolygon)
  {
    ShapeContainer container;
    container.type = ShapePolygon;

    OGRPolygon *poly = (OGRPolygon*) shape;
    OGRLinearRing *ring = poly->getExteriorRing();
    int nPoints = ring->getNumPoints();
    container.points.reserve(nPoints);

    for (int i = nPoints - 1; i >= 0; i--)
      container.points.push_back(base::Point(ring->getX(i), ring->getY(i)));

    shapes_container.push_back(container);

    for (int i = 0; i < poly->getNumInteriorRings() && !_interrupt; ++i)
      extract_points(poly->getInteriorRing(i), shapes_container);

  }
  else if (flat_type == wkbMultiPoint || flat_type == wkbMultiLineString
      || flat_type == wkbMultiPolygon || flat_type == wkbGeometryCollection)
  {
    OGRGeometryCollection *geoCollection = (OGRGeometryCollection*) shape;
    for (int i = 0; i < geoCollection->getNumGeometries() && !_interrupt; ++i)
      extract_points(geoCollection->getGeometryRef(i), shapes_container);
  }
}

void spatial::Importer::get_points(std::deque<ShapeContainer> &shapes_container)
{
  if (_geometry)
    extract_points(_geometry, shapes_container);
}

spatial::Importer::Importer() : _geometry(NULL), _interrupt(false)
{
}

int spatial::Importer::import_from_mysql(const std::string &data)
{
  OGRErr ret_val = OGRGeometryFactory::createFromWkb((unsigned char*)const_cast<char*>(&(*(data.begin()+4))), NULL, &_geometry);

  if (_geometry)
  {
    OGRSpatialReference hDstSRS;
    char *wkt = ProjectionFactory::get_projection(ProjGeodetic).get_wkt();
    hDstSRS.importFromWkt(&wkt);

    _geometry->assignSpatialReference(&hDstSRS);

  }
  if (ret_val == OGRERR_NONE)
    return 0;
  else
    return 1;
}

int spatial::Importer::import_from_wkt(std::string data)
{
  char *d = &(*data.begin());
  OGRErr ret_val = OGRGeometryFactory::createFromWkt(&d, NULL, &_geometry);

  if (_geometry)
  {
    OGRSpatialReference hDstSRS;

    char *wkt = ProjectionFactory::get_projection(ProjGeodetic).get_wkt();
    hDstSRS.importFromWkt(&wkt);

    _geometry->assignSpatialReference(&hDstSRS);
  }
  if (ret_val == OGRERR_NONE)
    return 0;
  else
    return 1;
}

void spatial::Importer::interrupt()
{
  _interrupt = true;
}

spatial::Converter::Converter(ProjectionView view, char *src_wkt, char *dst_wkt) : _geo_to_proj(NULL), _proj_to_geo(NULL), _view(view), _interrupt(false)
{
  CPLSetErrorHandler(&ogr_error_handler);
  change_projection(src_wkt, dst_wkt);
}

spatial::Converter::~Converter()
{
  base::RecMutexLock mtx(_projection_protector);
}

void spatial::Converter::change_view(ProjectionView view)
{
  _view = view;
  change_projection();
}

void spatial::Converter::change_projection(char *src_wkt, char *dst_wkt)
{
  base::RecMutexLock mtx(_projection_protector);
  if (src_wkt != NULL)
    _sourceSRS.importFromWkt(&src_wkt);

  if (dst_wkt != NULL)
    _targetSRS.importFromWkt(&dst_wkt);

  if (dst_wkt != NULL || src_wkt != NULL)
  {
    if (_geo_to_proj != NULL)
      OCTDestroyCoordinateTransformation(_geo_to_proj);
    if (_proj_to_geo != NULL)
      OCTDestroyCoordinateTransformation(_proj_to_geo);

    _geo_to_proj = OGRCreateCoordinateTransformation(&_sourceSRS, &_targetSRS);
    _proj_to_geo = OGRCreateCoordinateTransformation(&_targetSRS, &_sourceSRS);
    if (!_geo_to_proj || !_proj_to_geo)
      throw std::logic_error("Unable to perform specified transformation.\n");
  }

  double minLat = _view.MinLat, maxLon = _view.MaxLon, maxLat = _view.MaxLat, minLon = _view.MinLon;

  if (!_geo_to_proj->Transform(1, &minLat, &maxLon, 0))
  {
    char * proj4;
    _targetSRS.exportToProj4(&proj4);
    log_error("Unable to perform requested reprojection from WGS84, to %s\n", proj4);
    CPLFree(proj4);
  }

  if (!_geo_to_proj->Transform(1, &maxLat, &minLon, 0))
  {
    char * proj4;
    _targetSRS.exportToProj4(&proj4);
    log_error("Unable to perform requested reprojection from WGS84, to %s\n", proj4);
    CPLFree(proj4);
  }


  _adf_projection[0] = minLat;
  _adf_projection[1] = (maxLat - minLat) / (double)_view.width;
  _adf_projection[2] = 0;
  _adf_projection[3] = maxLon;
  _adf_projection[4] = 0;
  _adf_projection[5] = -(maxLon - minLon) / (double)_view.height;
  GDALInvGeoTransform(_adf_projection, _inv_projection);
}

void spatial::Converter::to_latlon(int x, int y, double &lat, double &lon)
{
  base::RecMutexLock mtx(_projection_protector);
  lat = _adf_projection[3] + (double) x * _adf_projection[4] + (double) y * _adf_projection[5];
  lon = _adf_projection[0] + (double) x * _adf_projection[1] + (double) y * _adf_projection[2];
  _proj_to_geo->Transform(1, &lat, &lon);
}

void spatial::Converter::from_latlon(double lat, double lon, int &x, int &y)
{
  base::RecMutexLock mtx(_projection_protector);
  x = _inv_projection[0] + _inv_projection[1] * lat;
  y = _inv_projection[3] + _inv_projection[5] * lon;
}

void spatial::Converter::transform_points(std::deque<ShapeContainer> &shapes_container)
{

  std::deque<ShapeContainer>::iterator it;
  for(it = shapes_container.begin(); it != shapes_container.end() && !_interrupt; it++)
  {
    std::deque<size_t> for_removal;
    for (size_t i = 0; i < (*it).points.size() && !_interrupt; i++)
    {
      if(!_geo_to_proj->Transform(1, &(*it).points[i].x, &(*it).points[i].y))
        for_removal.push_back(i);
    }
    std::deque<size_t>::reverse_iterator rit;
    for (rit = for_removal.rbegin(); rit < for_removal.rend() && !_interrupt; rit++)
      (*it).points.erase((*it).points.begin() + *rit);

    for(size_t i=0; i < (*it).points.size() && !_interrupt; i++)
    {
      int x, y;
      from_latlon((*it).points[i].x, (*it).points[i].y, x, y);
      (*it).points[i].x = x;
      (*it).points[i].y = y;
    }
  }
}

void spatial::Converter::interrupt()
{
  _interrupt = true;
}

using namespace spatial;

Feature::Feature(Layer *layer, int row_id, const std::string &data, bool wkt = false)
: _owner(layer), _row_id(row_id)
{
  if (wkt)
    _geometry.import_from_wkt(data);
  else
    _geometry.import_from_mysql(data);
}

Feature::~Feature()
{
}

void Feature::render(Converter *converter)
{
  // method names must get_output() like.. camel case is only for class/struct names
  std::deque<ShapeContainer> tmp_shapes;
  _geometry.get_points(tmp_shapes);
  converter->transform_points(tmp_shapes);
//  _geometry.get_output(visible_area, tmp_shapes); //XXX separate width/height and projection type into separate params
  _shapes = tmp_shapes;
}


void Feature::interrupt()
{
  _geometry.interrupt();
}


void Feature::repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area)
{
  for (std::deque<ShapeContainer>::iterator it = _shapes.begin(); it != _shapes.end() && !_owner->_interrupt; it++)
  {
    switch (it->type)
    {
      case ShapePolygon:
        cr.new_path();
        cr.move_to((*it).points[0]);
        for (size_t i = 1; i < (*it).points.size(); i++)
          cr.line_to((*it).points[i]);
        cr.close_path();
//        cr.stroke_preserve();
//        cr.fill();
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
        // for points, we paint the marker at the exact position but reverse the scaling, so that the marker size is constant
        cr.translate((*it).points[0]);
        cr.scale(1.0/scale, 1.0/scale);
        cr.rectangle(-5, -5, 5, 5);
        cr.fill();
        cr.restore();
        break;

      default:
        log_debug("Unknown type %i\n", it->type);
        break;
    }
  }
  cr.check_state();
}



Layer::Layer(int layer_id, base::Color color)
: _layer_id(layer_id), _color(color), _show(false), _interrupt(false)
{

}

Layer::~Layer()
{
  for (std::list<Feature*>::iterator it = _features.begin(); it != _features.end(); ++it)
  {
    delete *it;
  }
}

void Layer::interrupt()
{
  _interrupt = true;
   for (std::list<Feature*>::iterator it = _features.begin(); it != _features.end(); ++it)
     (*it)->interrupt();
}

bool Layer::hidden()
{
  return !_show;
}

int Layer::layer_id()
{
  return _layer_id;
}

void Layer::set_show(bool flag)
{
  _show = flag;
}

void Layer::add_feature(int row_id, const std::string &geom_data, bool wkt)
{
  Feature *feature = new Feature(this, row_id, geom_data, wkt);
  _features.push_back(feature);
}

void Layer::repaint(mdc::CairoCtx &cr, float scale, const base::Rect &clip_area)
{
  std::deque<ShapeContainer>::const_iterator it;

  cr.save();
  cr.set_line_width(0.5);
  cr.set_color(_color);

  for (std::list<Feature*>::iterator it = _features.begin(); it != _features.end(); ++it)
    (*it)->repaint(cr, scale, clip_area);

  cr.restore();
}


float Layer::query_render_progress()
{
  return _render_progress;
}


void Layer::render(Converter *converter)
{
  _render_progress = 0.0;
  float step = 1.0 / _features.size();

  for (std::list<spatial::Feature*>::iterator iter = _features.begin(); iter != _features.end(); ++iter)
  {
    (*iter)->render(converter);
    _render_progress += step;
  }
}
