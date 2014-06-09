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

GIS::SpatialHandler::SpatialHandler() :
    poGeometry(NULL), geo_to_proj(NULL), proj_to_geo(NULL)
{
  robinson_projection = "PROJCS[\"World_Robinson\","
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
  mercator_projection = "PROJCS[\"World_Mercator\", "
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
  equirectangular_projection = "PROJCS[\"World_Equidistant_Cylindrical\","
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
  geodetic_wkt = "GEOGCS[\"WGS 84\", "
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

int GIS::SpatialHandler::importFromMySQL(const std::string &data)
{
  unsigned char* geom = new unsigned char[data.size() - 3];
  std::copy(data.begin() + 4, data.end(), geom);

  OGRErr ret_val = OGRGeometryFactory::createFromWkb(geom, NULL, &poGeometry);
  delete[] geom;

  if (poGeometry)
  {
    OGRSpatialReference hDstSRS;

    char *wkt = &(*geodetic_wkt.begin());
    hDstSRS.importFromWkt(&wkt);

    poGeometry->assignSpatialReference(&hDstSRS);

  }
  if (ret_val == OGRERR_NONE)
    return 0;
  else
    return 1;
}

void GIS::SpatialHandler::setupMatrix(ProjectionView &view)
{
  OGRSpatialReference sourceSRS;
  OGRSpatialReference targetSRS;
  char *wkt_src = &(*geodetic_wkt.begin());
  sourceSRS.importFromWkt(&wkt_src);

  char *wkt_dst = getProjectionWkt(view.type);
  targetSRS.importFromWkt(&wkt_dst);

  if (geo_to_proj)
    OCTDestroyCoordinateTransformation(geo_to_proj);
  if (proj_to_geo)
      OCTDestroyCoordinateTransformation(proj_to_geo);

  geo_to_proj = OGRCreateCoordinateTransformation(&sourceSRS, &targetSRS);
  proj_to_geo = OGRCreateCoordinateTransformation(&targetSRS, &sourceSRS);
  if (!geo_to_proj)
    throw std::logic_error("Unable to perform specified transformation.\n");

  double minLat = view.MinLat, maxLon = view.MaxLon, maxLat = view.MaxLat, minLon = view.MinLon;


  if (!geo_to_proj->Transform(1, &minLat, &maxLon, 0))
    fprintf(stderr, "Sorry, unable to perform this conversion\n");

  if (!geo_to_proj->Transform(1, &maxLat, &minLon, 0))
    fprintf(stderr, "Sorry, unable to perform this conversion\n");


//  fprintf(stderr, "Before conversion: %f; %f; %f; %f\n", view.MinLat, view.MaxLon, view.MaxLat, view.MinLon);
//  fprintf(stderr, "After conversion: %f; %f; %f; %f\n", minLat, maxLon, maxLat, minLon);

  _adfProjection[0] = minLat;
  _adfProjection[1] = (maxLat - minLat) / (double)view.width;
  _adfProjection[2] = 0;
  _adfProjection[3] = maxLon;
  _adfProjection[4] = 0;
  _adfProjection[5] = -(maxLon - minLon) / (double)view.height;
  GDALInvGeoTransform(_adfProjection, _invProjection);
//  for(int i=0; i<6; i++)
//  {
//    fprintf(stderr, "Projection: %d: %f\n", i, _adfProjection[i]);
//    fprintf(stderr, "Inverted: %d: %f\n", i, _invProjection[i]);
//  }
}

void GIS::SpatialHandler::convertPoints(std::vector<double> &x,
    std::vector<double> &y, ProjectionType &projection)
{
  for (size_t i =0; i< x.size(); i++)
  {
    double _x = x[i], _y = y[i];
    if (!geo_to_proj->Transform(1, &y[i], &x[i]))
      fprintf(stderr, "Some problems occurs doing point convesrion of: lon: %f, lat: %f\n", _y, _x);
  }


  for(size_t i=0; i < x.size(); i++)
  {
    fromLatLng(x[i], y[i], x[i], y[i]);
  }
}

GIS::ShapeContainer GIS::SpatialHandler::convertToShapeContainer(ShapeType type,
    std::vector<double> &x, std::vector<double> &y)
{
  ShapeContainer container;
  container.type = type;
  for (size_t i = 0; i < x.size(); ++i)
  {
    container.points.push_back(base::Point(x[i], y[i]));
  }
  return container;
}

char* GIS::SpatialHandler::getProjectionWkt(ProjectionType p)
{
  char *projection = NULL;
  switch (p)
  {
  case ProjMercator:
    projection = &(*mercator_projection.begin());
    break;
  case ProjRobinson:
    projection = &(*robinson_projection.begin());
    break;
  case ProjEquirectangular:
    projection = &(*equirectangular_projection.begin());
    break;
  default:
    throw std::logic_error("GIS: Unknown projection type\n");
  }

  return projection;
}

void GIS::SpatialHandler::extractPoints(OGRGeometry *shape,
    std::deque<ShapeContainer> &shapes_container, ProjectionType &projection)
{
  OGRwkbGeometryType flat_type = wkbFlatten(shape->getGeometryType());

  if (flat_type == wkbPoint)
  {
    OGRPoint *point = (OGRPoint*) shape;
    std::vector<double> aPointX;
    std::vector<double> aPointY;
    aPointX.push_back(point->getX());
    aPointY.push_back(point->getY());
    convertPoints(aPointX, aPointY, projection);
    shapes_container.push_back(
        convertToShapeContainer(ShapePoint, aPointX, aPointY));

  } else if (flat_type == wkbLineString)
  {
    OGRLineString *line = (OGRLineString*) shape;
    int nPoints = line->getNumPoints();
    std::vector<double> aPointX;
    std::vector<double> aPointY;
    aPointX.reserve(nPoints);
    aPointY.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0; i--)
    {
      aPointX.push_back(line->getX(i));
      aPointY.push_back(line->getY(i));
    }
    convertPoints(aPointX, aPointY, projection);
    shapes_container.push_back(
        convertToShapeContainer(ShapeLineString, aPointX, aPointY));

  } else if (flat_type == wkbLinearRing)
  {
    OGRLinearRing *ring = (OGRLinearRing *) shape;
    int nPoints = ring->getNumPoints();
    std::vector<double> aPointX;
    std::vector<double> aPointY;
    aPointX.reserve(nPoints);
    aPointY.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0; i--)
    {
      aPointX.push_back(ring->getX(i));
      aPointY.push_back(ring->getY(i));
    }

    convertPoints(aPointX, aPointY, projection);
    shapes_container.push_back(
        convertToShapeContainer(ShapeLinearRing, aPointX, aPointY));

  } else if (flat_type == wkbPolygon)
  {
    OGRPolygon *poly = (OGRPolygon*) shape;
    OGRLinearRing *ring = poly->getExteriorRing();
    int nPoints = ring->getNumPoints();
    std::vector<double> aPointX;
    std::vector<double> aPointY;
    aPointX.reserve(nPoints);
    aPointY.reserve(nPoints);
    for (int i = nPoints - 1; i >= 0; i--)
    {
      aPointX.push_back(ring->getX(i));
      aPointY.push_back(ring->getY(i));
    }
    convertPoints(aPointX, aPointY, projection);
    shapes_container.push_back(
        convertToShapeContainer(ShapePolygon, aPointX, aPointY));
    for (int i = 0; i < poly->getNumInteriorRings(); ++i)
      extractPoints(poly->getInteriorRing(i), shapes_container, projection);

  } else if (flat_type == wkbMultiPoint || flat_type == wkbMultiLineString
      || flat_type == wkbMultiPolygon || flat_type == wkbGeometryCollection)
  {
    OGRGeometryCollection *geoCollection = (OGRGeometryCollection*) shape;
    for (int i = 0; i < geoCollection->getNumGeometries(); ++i)
      extractPoints(geoCollection->getGeometryRef(i), shapes_container,
          projection);
  }
}

int GIS::SpatialHandler::getOutput(ProjectionView &view,
    std::deque<ShapeContainer> &shapes_container)
{
  switch (view.type)
  {
  case ProjMercator:
  case ProjRobinson:
  case ProjEquirectangular:
    break;
  default:
    throw std::logic_error("Unknown type of projection");
  }

  this->setupMatrix(view);

  extractPoints(poGeometry, shapes_container, view.type);

  return 0;
}

void GIS::SpatialHandler::toLatLng(int x, int y,
    double &lat, double &lon)
{
  if (geo_to_proj)
  {
    lat = _adfProjection[3] + (double) x * _adfProjection[4] + (double) y * _adfProjection[5];
    lon = _adfProjection[0] + (double) x * _adfProjection[1] + (double) y * _adfProjection[2];
    proj_to_geo->Transform(1, &lon, &lat);
  }
}

void GIS::SpatialHandler::fromLatLng(double lat,
    double lon, double &x, double &y)
{
  if (geo_to_proj)
  {
    x = _invProjection[0] + _invProjection[1] * lat;
    y = _invProjection[3] + _invProjection[5] * lon;
  }

}

GIS::SpatialHandler::~SpatialHandler()
{
  if (geo_to_proj)
    OCTDestroyCoordinateTransformation(geo_to_proj);
  if (proj_to_geo)
      OCTDestroyCoordinateTransformation(proj_to_geo);

  // TODO Auto-generated destructor stub
}
