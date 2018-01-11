/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"
#include "wb_model.h"
#include "grts/structs.workbench.physical.h"
#include "grt/grt_manager.h"
#include "grtpp_undo_manager.h"
#include "base/string_utilities.h"
#include "base/wb_iterators.h"
#include "base/file_utilities.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#define PI_38 M_PI * 0.375
#define PI_58 M_PI * 0.625

using namespace grt;
using namespace std; // In VS min/max are not in the std namespace, so we have to split that.
using namespace base;

GRT_MODULE_ENTRY_POINT(WbModelImpl);

// plugin registration

static void def_export_view_plugin(const char *aName, const char *aCaption, grt::ListRef<app_Plugin> &list) {
  app_PluginRef plugin(grt::Initialized);
  app_PluginObjectInputRef pdef(grt::Initialized);

  plugin->name((std::string("wb.model.") + aName).c_str());

  plugin->caption(aCaption);
  plugin->moduleName("WbModel");
  plugin->moduleFunctionName(aName);
  plugin->pluginType("normal");
  plugin->rating(100);
  plugin->showProgress(1);
  pdef->name("activeDiagram");
  pdef->objectStructName("model.Diagram");
  pdef->owner(plugin);
  plugin->inputValues().insert(pdef);
  plugin->groups().insert("Application/Workbench");

  list.insert(plugin);
}

/*
#define def_export_view_plugin(aName, aCaption)\
  {\
    app_PluginRef plugin;\
    app_PluginObjectInputRef pdef;\
    plugin->name("wb.model."aName);\
    plugin->caption(aCaption);\
    plugin->moduleName("WbModel");\
    plugin->moduleFunctionName(aName);\
    plugin->pluginType("normal");\
    plugin->rating(100);\
    plugin->showProgress(1);\
    pdef->name("activeDiagram");\
    pdef->objectStructName("model.Diagram");\
    pdef->owner(plugin);\
    plugin->inputValues().insert(pdef);\
    plugin->groups().insert("Application/Workbench");\
    list.insert(plugin);\
  }
*/

static void def_export_catalog_plugin(const char *aName, const char *aCaption, grt::ListRef<app_Plugin> &list) {
  app_PluginRef plugin(grt::Initialized);
  app_PluginObjectInputRef pdef1(grt::Initialized);
  app_PluginObjectInputRef pdef2(grt::Initialized);

  plugin->name((std::string("wb.model.") + aName).c_str());
  plugin->caption(aCaption);
  plugin->moduleName("WbModel");
  plugin->moduleFunctionName(aName);
  plugin->pluginType("normal");
  plugin->rating(100);
  plugin->showProgress(1);
  pdef1->name("activeModel");
  pdef1->objectStructName("workbench.physical.Model");
  pdef1->owner(plugin);
  plugin->inputValues().insert(pdef1);

  pdef2->name("activeCatalog");
  pdef2->objectStructName("db.Catalog");
  pdef2->owner(plugin);
  plugin->inputValues().insert(pdef2);
  plugin->groups().insert("database/Database");
  list.insert(plugin);
}
/*
#define def_export_catalog_plugin(aName, aCaption)\
  {\
    app_PluginRef plugin;\
    app_PluginObjectInputRef pdef;\
    plugin->name("wb.model."aName);\
    plugin->caption(aCaption);\
    plugin->moduleName("WbModel");\
    plugin->moduleFunctionName(aName);\
    plugin->pluginType("normal");\
    plugin->rating(100);\
    plugin->showProgress(1);\
    pdef.name("activeCatalog");\
    pdef.objectStructName("db.Catalog");\
    pdef.owner(plugin);\
    plugin->inputValues().insert(pdef);\
    plugin->groups().insert("database/Database");\
    list.insert(plugin);\
  }
*/

static void def_figure_selection_plugin(const std::string &aName, const std::string &aCaption, const std::string &aCard,
                                        grt::ListRef<app_Plugin> &list) {
  app_PluginRef plugin(grt::Initialized);
  app_PluginSelectionInputRef pdef(grt::Initialized);

  plugin->name(("wb.model." + aName).c_str());

  plugin->caption(aCaption);
  plugin->moduleName("WbModel");
  plugin->moduleFunctionName(aName);
  plugin->pluginType("normal");
  plugin->rating(100);
  plugin->showProgress(0);
  pdef->name("activeDiagram");
  pdef->objectStructNames().insert(model_Figure::static_class_name());
  pdef->argumentCardinality(aCard);
  pdef->owner(plugin);
  plugin->inputValues().insert(pdef);
  plugin->groups().insert("Application/Workbench");
  list.insert(plugin);
}

ListRef<app_Plugin> WbModelImpl::getPluginInfo() {
  ListRef<app_Plugin> list(true);

  def_export_view_plugin("center", "Center Diagram Contents", list);
  def_export_view_plugin("autolayout", "Autolayout Figures", list);

  def_export_catalog_plugin("createDiagramWithCatalog", "Autoplace Objects of the Catalog on New Model", list);

  def_figure_selection_plugin("fitObjectsToContents", "Reset Object Size", "+", list);
  def_export_view_plugin("collapseAllObjects", "Collapse Objects", list);
  def_export_view_plugin("expandAllObjects", "Expand Objects", list);

  return list;
}

//----------------------------------------------------------------------

struct Rect {
  Rect(double left_val, double top_val, double width_val, double height_val)
    : left(left_val), top(top_val), width(width_val), height(height_val) {
  }
  Rect() : left(0.), top(0.), width(0.), height(0.) {
  }
  double left;
  double top;
  double width;
  double height;
};

inline Rect figure_coords(const model_FigureRef &figure) {
  return Rect(figure->left(), figure->top(), figure->width(), figure->height());
}

template <typename T1, typename T2>
void overwrite_default_option(T2 &value, const std::string &name, const grt::DictRef &options) {
  if (options.is_valid() && options.has_key(name))
    value = T1::cast_from(options.get(name));
}

template <typename T>
void overwrite_default_option(T &value, const std::string &name, const grt::DictRef &options,
                              bool init_with_empty_value) {
  if (options.is_valid() && options.has_key(name)) {
    value = T::cast_from(options.get(name));
    if (init_with_empty_value && !value.is_valid())
      value = T(grt::Initialized);
  }
}

//----------------------------------------------------------------------

std::string WbModelImpl::getTemplateDirFromName(const std::string &template_name) {
  // get pointer to the GRT

  std::string template_base_dir =
    base::makePath(bec::GRTManager::get()->get_basedir(), "modules/data/wb_model_reporting");

  // reformat the template name, replace spaces with _
  char *temp = g_strdup(template_name.c_str());
  char *ptr = temp;
  while ((ptr = strchr(ptr, ' ')))
    *ptr = '_';

  std::string template_dir(temp);
  g_free(temp);

  template_dir.append(".tpl");

  return base::makePath(template_base_dir, template_dir);
}

WbModelImpl::WbModelImpl(grt::CPPModuleLoader *ldr)
  : grt::ModuleImplBase(ldr), _use_objects_from_catalog(false), _undo_man(NULL) {
}

void WbModelImpl::begin_undo_group() {
  _undo_man = grt::GRT::get()->get_undo_manager();
  if (_undo_man)
    _undo_man->begin_undo_group();
}

void WbModelImpl::end_undo_group(const std::string &action_desc) {
  if (_undo_man) {
    _undo_man->end_undo_group();
    _undo_man->set_action_description(action_desc);
  }
}

int WbModelImpl::autolayout(model_DiagramRef view) {
  int result = 0;
  ListRef<model_Object> selection = view->selection();
  ListRef<model_Layer> layers = view->layers();

  //! _gcView->canvas()->beginUpdate();

  begin_undo_group();

  do_autolayout(view->rootLayer(), selection);
  for (std::size_t i = 0, layerCount = layers.count(); i != layerCount; ++i) {
    result = do_autolayout(layers.get(i), selection);
    if (0 != result)
      break;
  }

  end_undo_group(std::string(_("Autolayout Model '")).append(view->name()).append(_("'")));

  //!_gcView->canvas()->endUpdate();
  //!_gcView->canvas()->refresh();*/

  return result;
}

//==============================================================================
//
//==============================================================================
class Layouter {
public:
  Layouter(const model_LayerRef &layer);

  void add_figure_to_layout(const model_FigureRef &figure);
  void connect(const model_FigureRef &f1, const model_FigureRef &f2);

  int do_layout();

private:
  struct Node;

  static bool compare_node_links(const Node &n1, const Node &n2);

  bool shuffle();
  double calc_energy();
  double calc_node_energy(const std::size_t i, const Node &n);
  long distance_to_node(const std::size_t n1, const std::size_t n2, bool *is_horiz = NULL);
  double calc_node_pair(const std::size_t i1, const std::size_t i2);
  void prepare_layout_stages();

  const double _w;
  const double _h;

  struct Node {
    Node(const model_FigureRef &figure);
    void move_by(const long dx, const long dy);
    void move(const long x, const long y);
    bool is_linked_to(const ssize_t node) const;

    long w;
    long h;
    long x1;
    long y1;
    long x2;
    long y2;
    model_FigureRef fig;
    std::vector<ssize_t> linked;
  };
  typedef std::vector<Node> NodesList;
  NodesList _all_figures;
  NodesList _figures;
  long _min_dist; // desired dist between nodes
  double _min_energy;
  int _cell_w;
  int _cell_h;
  model_LayerRef _layer;
};

//------------------------------------------------------------------------------
Layouter::Node::Node(const model_FigureRef &figure)
  : w((long)figure->width()),
    h((long)figure->height()),
    x1((long)figure->left()),
    y1((long)figure->top()),
    x2(x1 + w),
    y2(y1 + h),
    fig(figure) {
}

//------------------------------------------------------------------------------
void Layouter::Node::move(const long x, const long y) {
  x1 = x;
  y1 = y;
  x2 = x1 + w;
  y2 = y1 + h;
}

//------------------------------------------------------------------------------
void Layouter::Node::move_by(const long dx, const long dy) {
  x1 += dx;
  y1 += dy;
  x2 += dx;
  y2 += dy;
}

//------------------------------------------------------------------------------
bool Layouter::Node::is_linked_to(const ssize_t node) const {
  bool found = false;

  for (ssize_t i = linked.size() - 1; i >= 0; --i) {
    if (linked[i] == node) {
      found = true;
      break;
    }
  }

  return found;
}

//------------------------------------------------------------------------------
Layouter::Layouter(const model_LayerRef &layer)
  : _w(layer->width()), _h(layer->height()), _min_dist(80), _min_energy(0), _cell_w(0), _cell_h(0), _layer(layer) {
  const ListRef<model_Figure> figures = layer->figures();

  for (std::size_t i = 0; i < figures->count(); ++i)
    _all_figures.push_back(figures[i]);
}

//------------------------------------------------------------------------------
void Layouter::add_figure_to_layout(const model_FigureRef &figure) {
  for (std::size_t i = 0; i < _all_figures.size(); ++i) {
    if (_all_figures[i].fig == figure) {
      _figures.push_back(figure);
    }
  }
}

//------------------------------------------------------------------------------
void Layouter::connect(const model_FigureRef &f1, const model_FigureRef &f2) {
  ssize_t n1 = -1;
  ssize_t n2 = -1;

  for (std::size_t i = 0; i < _figures.size(); ++i) {
    Node &n = _figures[i];
    if (n1 == -1 && n.fig == f1)
      n1 = i;
    if (n2 == -1 && n.fig == f2)
      n2 = i;
    if (n1 >= 0 && n2 >= 0)
      break;
  }

  if (n1 >= 0 && n2 >= 0) {
    Node &node1 = _figures[n1];
    Node &node2 = _figures[n2];
    node1.linked.push_back(n2);
    node2.linked.push_back(n1);
  }
}

//------------------------------------------------------------------------------
long Layouter::distance_to_node(const std::size_t i1, const std::size_t i2, bool *is_horiz) {
  const Node &n1 = _figures[i1];
  const Node &n2 = _figures[i2];
  const long x11 = n1.x1;
  const long y11 = n1.y1;
  const long x12 = n1.x2;
  const long y12 = n1.y2;

  const long x21 = n2.x1;
  const long y21 = n2.y1;
  const long x22 = n2.x2;
  const long y22 = n2.y2;

  const long cx1 = x11 + (x12 - x11) / 2;
  const long cy1 = y11 + (y12 - y11) / 2;
  const long cx2 = x21 + (x22 - x21) / 2;
  const long cy2 = y21 + (y22 - y21) / 2;
  const long dcx = cx2 - cx1;
  const double qr = atan2((double)dcx, (double)(cy2 - cy1));

  double dx = 0;
  double dy = 0;
  double l1 = 0;
  double l2 = 0;
  if (qr > M_PI_2) {
    dy = y11 - y22;
    dx = x21 - x12;
    l1 = dy ? ::fabs(dy / cos(qr)) : ::fabs(dx);
    l2 = dx ? ::fabs(dx / sin(qr)) : ::fabs(dy);
  } else if (0.0 < qr && qr <= M_PI_2) {
    dy = y21 - y12;
    dx = x21 - x12;
    if (dy > dx)
      l1 = l2 = dy ? ::fabs(dy / cos(qr)) : ::fabs(dx);
    else
      l1 = l2 = dx ? ::fabs(dx / sin(qr)) : ::fabs(dy);
  } else if (qr < -M_PI_2) {
    dy = y11 - y22;
    dx = -(x22 - x11);
    if (dy > dx)
      l1 = l2 = dy ? ::fabs(dy / cos(qr)) : ::fabs(dx);
    else
      l1 = l2 = dx ? ::fabs(dx / sin(qr)) : ::fabs(dy);
  } else {
    dy = y21 - y12;
    if (abs(dcx) > (x12 - x11) / 2)
      dx = x11 - x22;
    else
      dx = dcx;
    if (dy > dx)
      l1 = l2 = dy ? ::fabs(dy / cos(qr)) : ::fabs(dx);
    else
      l1 = l2 = (dx && qr != 0.0) ? ::fabs(dx / sin(qr)) : ::fabs(dy);
  }

  // printf("qr %f (cos(qr) = %f, sin(rq) = %f), l1 %li, l2 %li, dy %li, dx %li\n", qr, cos(qr), sin(qr), l1, l2, dy,
  // dx);

  const double aqr = ::fabs(qr);
  if (is_horiz)
    *is_horiz = PI_38 < aqr && aqr < PI_58;
  return l1 < l2 ? (long)l1 : (long)l2;
}

//------------------------------------------------------------------------------
inline double line_len2(long x1, long y1, long x2, long y2) {
  return sqrt(pow((double)(x2 - x1), 2) + pow((double)(y2 - y1), 2));
}

//------------------------------------------------------------------------------
double Layouter::calc_node_pair(const std::size_t i1, const std::size_t i2) {
  const Node *n1 = &(_figures[i1]);
  const Node *n2 = &(_figures[i2]);
  const bool is_linked = n1->is_linked_to(i2) || n2->is_linked_to(i1);

  long S1 = n1->w * n1->h;
  long S2 = n2->w * n2->h;
  if (S1 > S2) {
    std::swap(n1, n2);
    std::swap(S1, S2);
  }

  const long x11 = n1->x1;
  const long y11 = n1->y1;
  const long x12 = n1->x2;
  const long y12 = n1->y2;

  const long x21 = n2->x1;
  const long y21 = n2->y1;
  const long x22 = n2->x2;
  const long y22 = n2->y2;

  const long cx1 = x11 + (x12 - x11) / 2;
  const long cy1 = y11 + (y12 - y11) / 2;
  const long cx2 = x21 + (x22 - x21) / 2;
  const long cy2 = y21 + (y22 - y21) / 2;

  // Detect if nodes overlap
  const bool is_overlap = ((x12 >= x21) && (x22 >= x11) && (y12 >= y21) && (y22 >= y11));

  static const double overlap_quot = 1000.0;

  double e = 0.0;
  double distance = 0;
  if (is_overlap) {
    distance = line_len2(cx1, cy1, cx2, cy2);

    // calc area of overlap
    const long sx1 = x11 > x21 ? x11 : x21;
    const long sy1 = y11 > y21 ? y11 : y21;
    const long sx2 = x12 < x22 ? x12 : x22;
    const long sy2 = y12 < y22 ? y12 : y22;
    const long dsx = sx2 - sx1;
    const long dsy = sy2 - sy1;

    const long Sov = dsx * dsy;

    if (distance == 0.0)
      distance = 0.0000001;

    e = _min_dist * 1 / distance * 100 + Sov;
    e *= overlap_quot;
  } else {
    bool is_horiz = false;
    distance = distance_to_node(i1, i2, &is_horiz);

    if (distance <= _min_dist) {
      if (distance != 0) {
        if (is_linked)
          e += _min_dist + overlap_quot * 1 / distance;
        else
          e += _min_dist + overlap_quot * _min_dist / distance;
      } else {
        e += overlap_quot;
      }
    } else {
      e += distance;
      if (is_linked)
        e += distance * distance;
    }
  }

  return e;
}

//------------------------------------------------------------------------------
double Layouter::calc_energy() {
  double e = 0.0;

  std::size_t size = _figures.size();
  for (std::size_t i = 0; i < size; ++i) {
    const Node &node = _figures[i];
    if ((node.x1 < 0) || (node.y1 < 0) || (node.x2 + 20 > _w) || (node.y2 + 20 > _h))
      e += 1000000000000.0;

    for (std::size_t j = i + 1; j < size; ++j) {
      if (j >= size)
        break;
      e += calc_node_pair(i, j);
    }
  }

  return e;
}

//------------------------------------------------------------------------------
double Layouter::calc_node_energy(const std::size_t node_i, const Node &node) {
  double e = 0.0;

  if ((node.x1 < 0) || (node.y1 < 0) || (node.x2 + 20 > _w) || (node.y2 + 20 > _h))
    e += 1000000000000.0;

  for (std::size_t i = 0; i < _figures.size(); ++i) {
    if (node_i != i)
      e += calc_node_pair(node_i, i);
  }

  return e;
}

//------------------------------------------------------------------------------
bool Layouter::shuffle() {
  bool found_smaller_energy = false;
  const int step = (rand() % 5) + 1;

  for (std::size_t i = 0; i < _figures.size(); ++i) {
    Node &n = _figures[i];
    const int wstep = _cell_w * step;
    const int hstep = _cell_w * step;
    double node_energy = calc_node_energy(i, n);

    const int wsteps[] = {wstep, -wstep, 0, 0};
    const int hsteps[] = {0, 0, hstep, -hstep};
    for (int ns = sizeof(wsteps) / sizeof(int) - 1; ns >= 0; --ns) {
      n.move_by(wsteps[ns], hsteps[ns]);
      const double energy = calc_node_energy(i, n);
      if (energy < node_energy) {
        node_energy = energy;
        found_smaller_energy = true;
      } else
        n.move_by(-wsteps[ns], -hsteps[ns]);
    }
  }

  if (found_smaller_energy)
    _min_energy = calc_energy();

  return found_smaller_energy;
}

//------------------------------------------------------------------------------
bool Layouter::compare_node_links(const Node &n1, const Node &n2) {
  return n1.linked.size() > n2.linked.size();
}

//------------------------------------------------------------------------------
void Layouter::prepare_layout_stages() {
  double total_w = 0;
  double total_h = 0;
  std::sort(_figures.begin(), _figures.end(), compare_node_links);

  // reset layout
  for (size_t i = 0; i < _figures.size(); ++i) {
    Node &n = _figures[i];
    // place all tables in some initial position
    n.move((long)_w / 4, (long)_h / 4);

    // Calculate total dimensions and find max cell size.
    total_w += n.w;
    total_h += n.h;
    if (_cell_w < n.w)
      _cell_w = (int)n.w;
    if (_cell_h < n.h)
      _cell_h = (int)n.h;
  }
  _cell_w = (int)(1.1 * _cell_w);
}

//------------------------------------------------------------------------------
int Layouter::do_layout() {
  prepare_layout_stages();

  _min_energy = calc_energy();
  int de0_count = 10; // de=0 count
  double de = 1;      // energy delta
  double prev_energy = 0;
  while (de0_count > 0) {
    shuffle();
    de = prev_energy - _min_energy;
    prev_energy = _min_energy;
    if (de == 0)
      --de0_count;
    else
      de0_count = 10;
  }

  // update actual figures with new coords
  for (std::size_t i = 0; i < _figures.size(); ++i) {
    Node &n = _figures[i];
    model_FigureRef &f = n.fig;

    f->left(n.x1);
    f->top(n.y1);
  }
  return 0;
}

//------------------------------------------------------------------------------
int WbModelImpl::do_autolayout(const model_LayerRef &layer, ListRef<model_Object> &selection) {
  Layouter layout(layer);
  if (selection.count() > 0) {
    for (std::size_t i = 0; i < selection->count(); ++i) {
      const model_ObjectRef figure = selection[i];
      if (workbench_physical_TableFigureRef::can_wrap(figure) || workbench_physical_ViewFigureRef::can_wrap(figure))
        layout.add_figure_to_layout(model_FigureRef::cast_from(figure));
    }
  } else {
    const ListRef<model_Figure> figures = layer->figures();
    for (std::size_t i = 0; i < figures->count(); ++i) {
      const model_ObjectRef figure = figures[i];
      if (workbench_physical_TableFigureRef::can_wrap(figure) || workbench_physical_ViewFigureRef::can_wrap(figure))
        layout.add_figure_to_layout(model_FigureRef::cast_from(figure));
    }
  }

  ListRef<model_Connection> connections = layer->owner()->connections();
  for (std::size_t i = 0; i < connections->count(); ++i) {
    const model_ConnectionRef conn = connections[i];
    layout.connect(conn->startFigure(), conn->endFigure());
  }

  return layout.do_layout();
}

static bool calculate_view_size(const app_PageSettingsRef &page, double &width, double &height) {
  if (page->paperType().is_valid()) {
    width = page->paperType()->width();
    height = page->paperType()->height();

    width -= page->marginLeft() + page->marginRight();
    height -= page->marginTop() + page->marginBottom();

    width *= page->scale();
    height *= page->scale();

    if (page->orientation() == "landscape")
      std::swap(width, height);

    return true;
  } else {
    width = 1000;
    height = 1000;
    return false;
  }
}

workbench_physical_DiagramRef WbModelImpl::add_model_view(
  const db_CatalogRef &catalog, int xpages,
  int ypages) { // XXX TODO move this to Workbench module so we can reuse the same code as from wb_component
  // also add code to place db objects or figures in canvas
  workbench_physical_DiagramRef view(grt::Initialized);

  workbench_physical_ModelRef model = workbench_physical_ModelRef::cast_from(catalog->owner());

  app_PageSettingsRef page(app_PageSettingsRef::cast_from(grt::GRT::get()->get("/wb/doc/pageSettings")));
  double width, height;

  calculate_view_size(page, width, height);

  std::string name_prefix = "Model";
  std::string view_class_name = base::replaceString(model->get_metaclass()->name(), ".Model", ".Diagram");
  std::string name = grt::get_name_suggestion_for_list_object(model->diagrams(), name_prefix, true);

  model_DiagramRef diagram = model->addNewDiagram(0);

  view->name(StringRef(name));
  view->width(DoubleRef(width * xpages));
  view->height(DoubleRef(height * ypages));
  view->zoom(DoubleRef(1));

  return view;
}

static workbench_physical_DiagramRef create_view_for_object_count(workbench_physical_ModelRef model, int object_count) {
  int xpages = 2, ypages = 1;
  int pages;
  // guesstimate about 15 objects per page
  pages = (int)ceil(object_count / 15.0);
  ypages = (int)sqrt((float)pages);
  if (ypages < 1)
    ypages = 1;
  xpages = (int)ceil((float)pages / ypages);
  if (xpages < 1)
    xpages = 1;

  workbench_physical_DiagramRef view = workbench_physical_DiagramRef::cast_from(model->addNewDiagram(false));

  view->setPageCounts(xpages, ypages);

  return view;
}

int WbModelImpl::createDiagramWithObjects(workbench_physical_ModelRef model, grt::ListRef<GrtObject> objects) {
  std::size_t object_count = objects.count();

  if (object_count > 0) {
    begin_undo_group();
    workbench_physical_DiagramRef view = create_view_for_object_count(model, (int)object_count);

    do_autoplace_any_list(view, objects);
    ListRef<db_Table> tables(true);
    for (std::size_t n = 0, count = objects.count(); n < count; ++n) {
      if (db_TableRef::can_wrap(objects[n])) {
        db_TableRef table = db_TableRef::cast_from(objects[n]);
        if (table.is_valid())
          tables.insert(table);
      }
    }
    autoplace_relations(view, tables);

    end_undo_group(_("Create Diagram with Objects"));

    bec::GRTManager::get()->run_once_when_idle(std::bind(&WbModelImpl::autolayout, this, view));
  }

  return 0;
}

int WbModelImpl::createDiagramWithCatalog(workbench_physical_ModelRef model, db_CatalogRef catalog) {
  std::size_t object_count = 0;
  ListRef<db_Schema> schemata = catalog->schemata();
  for (std::size_t n = 0, count = schemata.count(); n < count; ++n) {
    db_SchemaRef schema = schemata[n];
    object_count += schema->tables().count();
    object_count += schema->views().count() / 3;         // views and routine groups
    object_count += schema->routineGroups().count() / 2; // take less space
  }
  if (object_count > 250)
    throw logic_error(
      "Cannot create diagram: too many objects to place.\nTry dividing your model into several sub-diagrams with less "
      "than 200 objects each.");

  DictRef wb_options = DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));

  begin_undo_group();
  workbench_physical_DiagramRef diagram = create_view_for_object_count(model, (int)object_count);

  for (std::size_t n = 0, count = schemata.count(); n < count; ++n) {
    db_SchemaRef schema = schemata[n];
    model_FigureRef figure;

    GRTLIST_FOREACH(db_Table, schema->tables(), table) {
      figure = diagram->placeTable(*table, 0.0, 0.0);
      if (figure.is_valid())
        figure->color(wb_options.get_string(figure.class_name() + ":Color"));
    }
    GRTLIST_FOREACH(db_View, schema->views(), view) {
      figure = diagram->placeView(*view, 0.0, 0.0);
      if (figure.is_valid())
        figure->color(wb_options.get_string(figure.class_name() + ":Color"));
    }
    GRTLIST_FOREACH(db_RoutineGroup, schema->routineGroups(), rg) {
      figure = diagram->placeRoutineGroup(*rg, 0.0, 0.0);
      if (figure.is_valid())
        figure->color(wb_options.get_string(figure.class_name() + ":Color"));
    }
    autoplace_relations(diagram, schema->tables());
  }

  autolayout(diagram);

  end_undo_group(_("Create Diagram with Catalog"));

  return 0;
}

int WbModelImpl::do_autoplace_any_list(const model_DiagramRef &view, ListRef<GrtObject> &obj_list) {
  if (!obj_list.is_valid())
    return 0;
  std::size_t count = obj_list.count();
  if (!count)
    return 0;

  workbench_physical_DiagramRef diagram(workbench_physical_DiagramRef::cast_from(view));

  DictRef wb_options = DictRef::cast_from(grt::GRT::get()->get("/wb/options/options"));

  GrtObjectRef object;
  model_FigureRef figure;
  model_LayerRef layer = view->rootLayer();

  for (std::size_t n = 0; n < count; ++n) {
    object = obj_list.get(n);

    if (object.is_instance("db.Table")) {
      figure = diagram->placeTable(db_TableRef::cast_from(object), 0.0, 0.0);
    } else if (object.is_instance("db.View")) {
      figure = diagram->placeView(db_ViewRef::cast_from(object), 0.0, 0.0);
    } else if (object.is_instance("db.RoutineGroup")) {
      figure = diagram->placeRoutineGroup(db_RoutineGroupRef::cast_from(object), 0.0, 0.0);
    } else
      continue;
    if (figure.is_valid())
      figure->color(wb_options.get_string(figure.class_name() + ":Color"));
  }

  return 0;
}

int WbModelImpl::autoplace_relations(
  const model_DiagramRef &view,
  const ListRef<db_Table> &tables) { // XXX TODO remove this (should be handled automatically by wb_component_physical)
  for (std::size_t t = 0, count = tables.count(); t < count; ++t) {
    db_TableRef table = tables.get(t);
    ListRef<db_ForeignKey> fkeys = table->foreignKeys();
    for (std::size_t f = 0, count = fkeys.count(); f < count; ++f)
      handle_fklist_change(view, table, fkeys.get(f), true);
  }
  return 0;
}

void WbModelImpl::handle_fklist_change(const model_DiagramRef &view, const db_TableRef &table,
                                       const db_ForeignKeyRef &fk, bool added) { // XXX TODO remove this
  if (!view.is_valid())
    return;

  if (!fk.is_valid()) {
    if (!added) {
      // all FKs from table removed
    }
  } else {
    if (!fk->referencedTable().is_valid() || !fk->owner().is_valid())
      return;

    if (added) {
      // we have to go through all views in the model and find all table figures
      // that correspond to the FK for creating the relationship in all these views

      grt::ListRef<model_Figure> figures(view->figures());
      workbench_physical_TableFigureRef table1, table2;

      for (std::size_t d = figures.count(), f = 0; f < d; f++) {
        model_FigureRef fig(figures[f]);

        if (fig.is_instance(workbench_physical_TableFigure::static_class_name())) {
          workbench_physical_TableFigureRef tablefig(workbench_physical_TableFigureRef::cast_from(fig));

          if (tablefig->table() == table) {
            table1 = tablefig;
            if (table2.is_valid())
              break;
          }
          if (tablefig->table() == fk->referencedTable()) {
            table2 = tablefig;
            if (table1.is_valid())
              break;
          }
        }
      }
      if (table1.is_valid() &&
          table2.is_valid()) { // both tables in the relationship are in this view, so create the connection

        // but 1st check if it already exists
        grt::ListRef<model_Connection> connections(view->connections());
        bool found = false;

        for (std::size_t d = connections.count(), j = 0; j < d; j++) {
          model_ConnectionRef conn(connections[j]);

          if (conn.is_instance(workbench_physical_Connection::static_class_name())) {
            workbench_physical_ConnectionRef pconn(workbench_physical_ConnectionRef::cast_from(conn));

            if (pconn->foreignKey() == fk) {
              found = true;
              break;
            }
          }
        }

        // connection doesnt exist yet, create it
        if (!found) {
          workbench_physical_ConnectionRef conn(grt::Initialized);
          conn->owner(view);
          conn->startFigure(table1);
          conn->endFigure(table2);
          conn->caption(fk->name());
          conn->foreignKey(fk);

          view->connections().insert(conn);
        }
      }
    } else {
      // we have to go through all views in the model and find all connections
      // that correspond to the FK for removing them

      grt::ListRef<model_Connection> connections(view->connections());

      for (grt::ListRef<model_Connection>::const_reverse_iterator conn = connections.rbegin();
           conn != connections.rend(); ++conn) {
        if ((*conn).is_instance(workbench_physical_Connection::static_class_name())) {
          workbench_physical_ConnectionRef pconn(workbench_physical_ConnectionRef::cast_from(*conn));

          if (pconn->foreignKey() == fk) {
            // remove this connection
            connections.remove_value(pconn);
          }
        }
      }
    }
  }
}

int WbModelImpl::center(model_DiagramRef view) {
  Rect model_bounds;
  model_LayerRef rootLayer(view->rootLayer());
  double view_width = rootLayer->width();
  double view_height = rootLayer->height();
  double xmin = view_width, ymin = view_height, xmax = 0, ymax = 0;

  // find out model bounds
  for (std::size_t c = rootLayer->subLayers().count(), i = 0; i < c; i++) {
    model_LayerRef layer(rootLayer->subLayers().get(i));

    xmin = min(xmin, *layer->left());
    ymin = min(ymin, *layer->top());

    xmax = max(xmax, *layer->left() + *layer->width());
    ymax = max(ymax, *layer->top() + *layer->height());
  }

  for (std::size_t c = rootLayer->figures().count(), i = 0; i < c; i++) {
    model_FigureRef figure(rootLayer->figures().get(i));

    xmin = min(xmin, *figure->left());
    ymin = min(ymin, *figure->top());

    xmax = max(xmax, *figure->left() + *figure->width());
    ymax = max(ymax, *figure->top() + *figure->height());
  }

  model_bounds.left = xmin;
  model_bounds.top = ymin;
  model_bounds.width = xmax - xmin;
  model_bounds.height = ymax - ymin;

  // center if possible
  if (model_bounds.width <= view_width && model_bounds.height <= view_height) {
    double xoffs, yoffs;

    // determine offset to move everything so it gets centered
    xoffs = (view_width - model_bounds.width) / 2 - model_bounds.left;
    yoffs = (view_height - model_bounds.height) / 2 - model_bounds.top;

    begin_undo_group();

    // center layers that are on rootLayer
    for (std::size_t c = rootLayer->subLayers().count(), i = 0; i < c; i++) {
      model_LayerRef layer(rootLayer->subLayers().get(i));

      layer->left(*layer->left() + xoffs);
      layer->top(*layer->top() + yoffs);
    }

    for (std::size_t c = rootLayer->figures().count(), i = 0; i < c; i++) {
      model_FigureRef figure(rootLayer->figures().get(i));

      figure->left(*figure->left() + xoffs);
      figure->top(*figure->top() + yoffs);
    }

    end_undo_group(_("Center Model"));
  }

  return 0;
}

int WbModelImpl::fitObjectsToContents(const grt::ListRef<model_Object> &selection) {
  for (std::size_t c = selection.count(), i = 0; i < c; i++) {
    if (selection[i].is_instance<model_Figure>()) {
      model_FigureRef figure(model_FigureRef::cast_from(selection[i]));
      ssize_t o = *figure->manualSizing();
      if (o != 0) {
        figure->manualSizing(0);
      }
    }
  }
  return 0;
}

int WbModelImpl::expandAllObjects(model_DiagramRef view) {
  grt::ListRef<model_Figure> figures(view->figures());

  for (std::size_t c = figures.count(), i = 0; i < c; i++) {
    figures[i]->expanded(1);
  }

  return 0;
}

int WbModelImpl::collapseAllObjects(model_DiagramRef view) {
  grt::ListRef<model_Figure> figures(view->figures());

  for (std::size_t c = figures.count(), i = 0; i < c; i++) {
    figures[i]->expanded(0);
  }

  return 0;
}
