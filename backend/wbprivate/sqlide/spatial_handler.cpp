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

GIS::SpatialHandler::SpatialHandler() :
    poGeometry(NULL), imgTransformerArgument(NULL)
{
  default_projection = "GEOGCS[\"GCS_WGS_1984\","
      "DATUM[\"WGS_1984\","
      "SPHEROID[\"WGS_84\","
      "6378137,298.257223563]],"
      "PRIMEM[\"Greenwich\",0],"
      "UNIT[\"Degree\",0.017453292519943295]]";
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
  mercator_projection = "PROJCS[\"World_Mercator\","
      "GEOGCS[\"GCS_WGS_1984\","
      "DATUM[\"WGS_1984\","
      "SPHEROID[\"WGS_1984\",6378137,298.257223563]],"
      "PRIMEM[\"Greenwich\",0],"
      "UNIT[\"Degree\",0.017453292519943295]],"
      "PROJECTION[\"Mercator_1SP\"],"
      "PARAMETER[\"False_Easting\",0],"
      "PARAMETER[\"False_Northing\",0],"
      "PARAMETER[\"Central_Meridian\",0],"
      "PARAMETER[\"latitude_of_origin\",0],"
      "UNIT[\"Meter\",1]]";
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

  imgTransformerFunc = GDALGenImgProjTransform;
}

int GIS::SpatialHandler::importFromMySQL(std::string &data)
{
  unsigned char* geom = new unsigned char[data.size() - 3];
  std::copy(data.begin() + 4, data.end(), geom);

  OGRErr ret_val = OGRGeometryFactory::createFromWkb(geom, NULL, &poGeometry);
  delete[] geom;

  if (ret_val == OGRERR_NONE)
    return 0;
  else
    return 1;
}

GDALDataset* GIS::SpatialHandler::memSetup(ProjectionView &view)
{
  double dfXRes = (double) (view.MaxLat - view.MinLat) / view.width;
  double dfYRes = (double) (view.MaxLng - view.MinLng) / view.height;

  double adfProjection[6];
  adfProjection[0] = view.MinLat;
  adfProjection[1] = dfXRes;
  adfProjection[2] = 0;
  adfProjection[3] = view.MaxLng;
  adfProjection[4] = 0;
  adfProjection[5] = -dfYRes;
  GDALDataset* ds = MEMDataset::Create("MEM:::", view.width, view.height, 1,
      GDT_UInt32, NULL);

  GDALSetGeoTransform(ds, adfProjection);
  char *projection;
  switch (view.type)
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
    view.type = ProjDefault;
    projection = &(*default_projection.begin());
  }
  GDALSetProjection(ds, projection);
  return ds;
}

void GIS::SpatialHandler::convertPoints(std::vector<double> &x,
    std::vector<double> &y)
{
  int *panSuccess = (int *) CPLCalloc(sizeof(int), x.size());

  imgTransformerFunc(imgTransformerArgument, false, x.size(), &(x[0]), &(y[0]),
      NULL, panSuccess);
  CPLFree(panSuccess);
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

void GIS::SpatialHandler::extractPoints(OGRGeometry *shape,
    std::deque<ShapeContainer> &shapes_container)
{
  OGRwkbGeometryType flat_type = wkbFlatten(shape->getGeometryType());

  if (flat_type == wkbPoint)
  {
    OGRPoint *point = (OGRPoint*)shape;
    std::vector<double> aPointX;
    std::vector<double> aPointY;
    aPointX.push_back(point->getX());
    aPointY.push_back(point->getY());
    convertPoints(aPointX, aPointY);
    shapes_container.push_back(
        convertToShapeContainer(ShapePoint, aPointX, aPointY));

  } else if (flat_type == wkbLineString)
  {
    OGRLineString *line = (OGRLineString*)shape;
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
    convertPoints(aPointX, aPointY);
    shapes_container.push_back(
        convertToShapeContainer(ShapeLineString, aPointX, aPointY));

  } else if (flat_type == wkbLinearRing)
  {
    OGRLinearRing *ring =  (OGRLinearRing *)shape;
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

    convertPoints(aPointX, aPointY);
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
    convertPoints(aPointX, aPointY);
    shapes_container.push_back(
        convertToShapeContainer(ShapePolygon, aPointX, aPointY));
     for (int i=0; i < poly->getNumInteriorRings(); ++i)
       extractPoints(poly->getInteriorRing(i), shapes_container);

  } else if (flat_type == wkbMultiPoint || flat_type == wkbMultiLineString
      || flat_type == wkbMultiPolygon || flat_type == wkbGeometryCollection)
  {
    OGRGeometryCollection *geoCollection = (OGRGeometryCollection*) shape;
    for  ( int i=0; i < geoCollection->getNumGeometries(); ++i)
      extractPoints(geoCollection->getGeometryRef(i), shapes_container);
}
}

int GIS::SpatialHandler::getOutput(ProjectionView &view,
    std::deque<ShapeContainer> &shapes_container)
{
  GDALDataset *ds = this->memSetup(view);
  imgTransformerArgument = GDALCreateGenImgProjTransformer( NULL, NULL, ds, NULL, false, 0.0, 0);

//  OGRSpatialReference *oldRef = new poGeometry->getSpatialReference();
//  if (oldRef)
//    poGeometry->assignSpatialReference(NULL);

  OGRSpatialReference *hDstSRS = new OGRSpatialReference();

  char *projection = &(*default_projection.begin());
  hDstSRS->importFromWkt(&projection);

  poGeometry->assignSpatialReference(hDstSRS);

  extractPoints(poGeometry, shapes_container);
  GDALDestroyGenImgProjTransformer(imgTransformerArgument);
  imgTransformerArgument = NULL;

  GDALClose(ds);
  return 0;
}

GIS::SpatialHandler::~SpatialHandler()
{
  // TODO Auto-generated destructor stub
}
