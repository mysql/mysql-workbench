#include "stdafx.h"

#include <grts/structs.model.h>

#include <grtpp_util.h>

#include "wbcanvas/model_diagram_impl.h"
#include "wbcanvas/model_layer_impl.h"

//================================================================================
// model_Diagram


void model_Diagram::init()
{
}

void model_Diagram::set_data(ImplData *data)
{
  _data= data;
}

model_Diagram::~model_Diagram()
{
}


void model_Diagram::rootLayer(const model_LayerRef &value)
{
  grt::ValueRef ovalue(_rootLayer);
  // this member is owned by this object
  if (_rootLayer.is_valid())
    _rootLayer->get_data()->set_in_view(false);
  _rootLayer= value;
  if (_rootLayer.is_valid())
    _rootLayer->get_data()->set_in_view(true);
  owned_member_changed("rootLayer", ovalue, value);
}


void model_Diagram::addConnection(const model_ConnectionRef &connection)
{
  _data->add_connection(connection);
}


void model_Diagram::addFigure(const model_FigureRef &figure)
{
  _data->add_figure(figure);
}


void model_Diagram::deleteLayer(const model_LayerRef &layer)
{
  _data->delete_layer(layer);
}


void model_Diagram::removeConnection(const model_ConnectionRef &connection)
{
  _data->remove_connection(connection);
}


void model_Diagram::removeFigure(const model_FigureRef &figure)
{
  _data->remove_figure(figure);
}


void model_Diagram::blockUpdates(long flag)
{
  _data->block_updates(flag != 0);
}


void model_Diagram::selectObject(const model_ObjectRef &object)
{
  _data->select_object(object);
}


void model_Diagram::setPageCounts(long xpages, long ypages)
{
  _data->set_page_counts(xpages, ypages);
}


void model_Diagram::unselectAll()
{
  _data->unselect_all();
}


void model_Diagram::unselectObject(const model_ObjectRef &object)
{
  _data->unselect_object(object);
}

