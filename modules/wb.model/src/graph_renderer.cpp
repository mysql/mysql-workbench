/* 
 * Copyright (c) 2009, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include <climits>
#include <limits.h>

#include "graph_renderer.h"

#ifdef __GNUC__
const int GraphRenderer::K1F;
const int GraphRenderer::K1N;
const int GraphRenderer::K2;
const int GraphRenderer::K3;
#endif

/*
// equals for doubles
// algorithm by D.E.Knuth (TAOCP Vol.3 Sec. 4.2.2)
static bool eq(double d1, double d2)
{
  static int delta = 1;

  long long i1 = * (long long*) &d1;
  long long i2 = * (long long*) &d2;

  if(i1 < 0) 
  {
    i1 = -i1;//0x8000000000000000 - i1;
  }
  if(i2 < 0) 
  {
    i2 = -i2;//0x8000000000000000 - i2;
  }

  long long v = i1 - i2;
  if(v < 0)
  {
    v = -v;
  }

  return (v < delta);
}
 */

static inline bool edge_is_special(GraphEdge& edge)
{
  return edge.is_special();
}

static inline void reset_visited(GraphNode *node)
{
  node->set_visited(false);
}

double GraphNode::distance(const GraphNode& n1, const GraphNode& n2)
{
  double xdist, left, right, leftw;
  double ydist, top, bottom, toph;

  if(n1._left < n2._left) 
  {
    left = n1._left;
    leftw = n1._width;
    right = n2._left;
  } 
  else
  {
    left = n2._left;
    leftw = n2._width;
    right = n1._left;
  }

  xdist = right - left - leftw;
  if(xdist <= 0) 
  {
    xdist = 1;
  }

  if(n1._top < n2._top) 
  {
    top = n1._top;
    toph = n1._height;
    bottom = n2._top;
  } 
  else
  {
    top = n2._top;
    toph = n2._height;
    bottom = n1._top;
  }

  ydist = bottom - top - toph;
  if(ydist <= 0) 
  {
    ydist = 1;
  }

  return sqrt(xdist*xdist + ydist*ydist);
}

bool operator == (const GraphNode& n1, const GraphNode& n2) 
{
  return (n1._left == n2._left) && (n1._top == n2._top) && (n1._width == n2._width) && (n1._height == n2._height); 
}

GraphRenderer::GraphRenderer(double margin, double maxwidth, double maxheight) 
  : focus_recalced(false), _length(50), _left(0), _top(0), _right(200), _bottom(200), _avg_force(-1)
{
  _margin= margin;
  _maxw= maxwidth;
  _maxh= maxheight;
}

GraphRenderer::~GraphRenderer()
{
  GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it)
  {
    delete *it;
  }
}

void GraphRenderer::mark_neighbours(const GraphNode& node)
{
  std::for_each(_allnodes.begin(), _allnodes.end(), reset_visited);
  
  for(GraphEdgeList::const_iterator it= _alledges.begin(); it != _alledges.end(); ++it)
  {
    GraphEdge e= *it;
    if(e.contains(node))
    {
      e.get_other(node).set_visited(true);    
    }
  }
}

bool GraphRenderer::is_focus_node(const GraphNode& node) const
{
  unsigned counter= 0;

  for(GraphEdgeList::const_iterator edgeit= _alledges.begin(); edgeit != _alledges.end(); edgeit++)
  {
    GraphEdge e= *edgeit;
    if(e.contains(node))
    {
      counter++;
      if(counter > 1)
        return true;
    }
  }

  return false;
}

// horizontal with vertical
static inline bool hv_lines_intersect(double x11, double x12, double y1, double x2, double y21, double y22)
{
  return (x2 >= x11) && (x2 <= x12) && (y1 >= y21) && (y1 <= y22);
}

static inline bool nodes_intersect(double l, double t, double w, double h,
                                   double ol, double ot, double ow, double oh)
{
    // to simplify things
    // this doesn't test full inclusion (which is very improbable)
    return hv_lines_intersect(l, l+w, t, ol, ot, ot+oh)
      || hv_lines_intersect(l, l+w, t+h, ol, ot, ot+oh)
      || hv_lines_intersect(l, l+w, t, ol+ow, ot, ot+oh)
      || hv_lines_intersect(l, l+w, t+h, ol+ow, ot, ot+oh)
      || hv_lines_intersect(ol, ol+ow, ot, l, t, t+h)
      || hv_lines_intersect(ol, ol+ow, ot+oh, l, t, t+h)
      || hv_lines_intersect(ol, ol+ow, ot, l+w, t, t+h)
      || hv_lines_intersect(ol, ol+ow, ot+oh, l+w, t, t+h);
}

bool GraphRenderer::has_intersections() const
{
  GraphNodeRefList::const_iterator e= _allnodes.end();
  for(GraphNodeRefList::const_iterator it= _allnodes.begin(); it != e; ++it)
  {
    GraphNode& node= **it;
    double l= node.left();
    double t= node.top();
    double w= node.width();
    double h= node.height();

    GraphNodeRefList::const_iterator it2= it;
    it2++;
    for(; it2 != e; it2++)
    {
      GraphNode& other= **it2;
      double ol= other.left();
      double ot= other.top();
      double ow= other.width();
      double oh= other.height();

      bool ix= nodes_intersect(l, t, w, h, ol, ot, ow, oh);
      if(ix)
        return true;
    }
  }
  return false;
}

void GraphRenderer::get_delta(const GraphNode& node, double *deltax, double *deltay)
{
  static const int NNODES= 2;
  
  mark_neighbours(node);

  double ccx= node.left();
  double ccy= node.top();
  bool node_is_focus= node.is_focus();

  double all_sum_x= 0.;
  double all_sum_y= 0.;

  GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphNodeRefList::const_iterator it= _allnodes.begin(); it != e; ++it) 
  {
    const GraphNode* nei[NNODES];
    nei[0]= *it;
    if(nei[0] == &node)
    {
      if(++it == e)
        break;
      nei[0]= *it;
    }
    
    ++it;
    
    if(it != e)
    {
      nei[1]= *it;
      if(nei[1] == &node)
      {
        if(++it != e)
          nei[1]= *it;
      }
    }

    if(it == e)
    {
      nei[1]= nei[0]; // we waste calculations but save if() jumps
      --it;
    }

    double d0, d1;
    
    d0= GraphNode::distance(node, *nei[0]);
    d1= GraphNode::distance(node, *nei[1]);

    if(d0 == 0.)
      d0= 1.;
    if(d1 == 0.)
      d1= 1.;

    double dcx[NNODES];
    double dcy[NNODES];
    
    dcx[0]= ccx - nei[0]->left();
    dcy[0]= ccy - nei[0]->top();
    dcx[1]= ccx - nei[1]->left();
    dcy[1]= ccy - nei[1]->top();

    double vx0, vx1;
    double vy0, vy1;
    vx0= dcx[0];
    vy0= dcy[0];
    vx1= dcx[1];
    vy1= dcy[1];

    double dq0, dq1;
    dq0= d0*d0;
    dq1= d1*d1;
    
    bool neif[NNODES];
    neif[0]= nei[0]->is_focus();
    neif[1]= nei[1]->is_focus();

    vx0 *= GraphRenderer::K2;
    vy0 *= GraphRenderer::K2;
    vx1 *= GraphRenderer::K2;
    vy1 *= GraphRenderer::K2;

    vx0 /= dq0;
    vy0 /= dq0;
    vx1 /= dq1;
    vy1 /= dq1;

    all_sum_x += vx0;
    all_sum_y += vy0;
    all_sum_x += vx1;
    all_sum_y += vy1;

    if(neif[0])
    {
      all_sum_x += vx0;
      all_sum_y += vy0;
    }
    if(neif[1])
    {
      all_sum_x += vx1;
      all_sum_y += vy1;
    }

    if(nei[0]->is_visisted())
    {
      double k= _length - d0;
      
      vx0= k*dcx[0]/d0;
      vy0= k*dcy[0]/d0;

      k= (node_is_focus || neif[0]) ? GraphRenderer::K1F : GraphRenderer::K1N;
      vx0 /= k;
      vy0 /= k;
      
      all_sum_x += vx0;
      all_sum_y += vy0;
   }

    if(nei[1]->is_visisted())
    {
      double k= _length - d1;

      vx1= k*dcx[1]/d1;
      vy1= k*dcy[1]/d1;

      k= (node_is_focus || neif[1]) ? GraphRenderer::K1F : GraphRenderer::K1N;
      vx1 /= k;
      vy1 /= k;
      
      all_sum_x += vx1;
      all_sum_y += vy1;
    }
  }

  static const double t= 10.; // threshold
  static const double s= 4.;  // step

  if(all_sum_x >= t)
    *deltax= s;
  else if(all_sum_x <= -t)
    *deltax= -s;
  else
    *deltax= 0;

  if(all_sum_y >= t)
    *deltay= s;
  else if(all_sum_y <= -t)
    *deltay= -s;
  else
    *deltay= 0;
//!
/*
  *deltax= std::min(*deltax, _right - node.left() - node.width());
  *deltay= std::min(*deltay, _bottom - node.top() - node.height());
  *deltax= std::min(*deltax, _maxw - node.left() - node.width());
  *deltay= std::min(*deltay, _maxh - node.top() - node.height());
*/
}

GraphNode *GraphRenderer::add_node(double left, double top, double width, double height) 
{ 
  GraphNode *node= new GraphNode(left, top, width, height);
  _allnodes.push_back(node); 
  focus_recalced= false;
  return node;
}

void GraphRenderer::add_edge(GraphNode *n1, GraphNode *n2) 
{ 
  _alledges.push_back(GraphEdge(*n1, *n2)); 
  focus_recalced= false;
}

void GraphRenderer::add_special_edge(GraphNode *n1, GraphNode *n2) 
{ 
  _alledges.push_back(GraphEdge(*n1, *n2, true)); 
  focus_recalced= false;
}

void GraphRenderer::mark_reachable(GraphNode& node)
{
  if(node.is_visisted())
    return;
  
  node.set_visited(true);

  for(GraphRenderer::GraphEdgeList::iterator it= _alledges.begin(); it != _alledges.end(); ++it) 
  {
    GraphEdge& edge= *it;
    if(!edge.contains(node))
      continue;
    GraphNode& other= edge.get_other(node);
    mark_reachable(other);
  }
}

void GraphRenderer::concat_graph(GraphNode& node)
{
  mark_reachable(node);
  
  GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it)
  {
    GraphNode& next= **it;
    if(!next.is_visisted())
    {
      add_special_edge(&node, &next);
      mark_reachable(next);
    }
  }
}

void GraphRenderer::recalc_focus_nodes()
{
  if(focus_recalced)
    return;
  
  GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node = **it;
    node.set_focus(is_focus_node(node));
  }

  // add edges to avoid unconnected nodes/pieces
  std::remove_if(_alledges.begin(), _alledges.end(), edge_is_special);
  std::for_each(_allnodes.begin(), _allnodes.end(), reset_visited);
  if(_allnodes.size() > 0)
  {
    GraphNode* node = _allnodes.front();
    concat_graph(*node);
  }

  focus_recalced= true;
}

void GraphRenderer::recalc_outer_rect()
{
  _left = INT_MAX;
  _top = INT_MAX;
  _right = INT_MIN;
  _bottom = INT_MIN;

  GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node = **it;
    double left = node.left();
    double top = node.top();
    double right = left + node.width();
    double bottom = top + node.height();
    
    if(_left > left) {
      _left = left;
    } 
    if(_right < right) {
      _right = right;
    }
    if(_top > top) {
      _top = top;
    } 
    if(_bottom < bottom) {
      _bottom = bottom;
    } 
  }
}

void GraphRenderer::recalc_positions()
{
  double xf, yf;
  CoordSet cs;
  _avg_force= 0;

  GraphRenderer::GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node= **it;

    if(!node.is_movable())
      continue;

    get_delta(node, &xf, &yf);
    node.setnewleft(node.left() + xf);
    node.setnewtop(node.top() + yf);
    _avg_force += sqrt(xf*xf + yf*yf);
    std::pair<CoordSet::iterator, bool> pr = cs.insert(CoordPair(node.newleft(), node.newtop()));
    while(!pr.second) 
    {
      node.setnewleft(node.newleft() + 1);
      node.setnewtop(node.newtop() + 1);
      pr = cs.insert(CoordPair(node.newleft(), node.newtop()));
    }
  }

  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node = **it;
    if(!node.is_movable())
      continue;
    node.apply();
  }
}

void GraphRenderer::rotate_point(double *x, double *y, double angle)
{
  double x0 = *x;
  double y0 = *y;
  double sina= sin(angle);
  double cosa= cos(angle);
  *x = x0*cosa + y0*sina;
  *y = -x0*sina + y0*cosa;
}

void GraphRenderer::move(double dx, double dy)
{
  GraphRenderer::GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node = **it;
    node.setnewleft(node.left() + dx);
    node.setnewtop(node.top() + dy);
    node.apply();
  }
}

void GraphRenderer::rotate()
{
  static double pi = 3.1415926535;
  static double step = pi/300.;

  double curmsd = 0, posmsd = 0, negmsd = 0;
  double yzero = (_top + _bottom)/2.;
  double xzero = (_left + _right)/2.;

  GraphRenderer::GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node = **it;
    
    if(!node.is_movable())
      continue;

    double x = node.centerx() - xzero;
    double y = node.centery() - yzero;
    double x1 = x;
    double y1 = y;
    double x2 = x;
    double y2 = y;

    rotate_point(&x1, &y1, step);
    rotate_point(&x2, &y2, -step);

    curmsd += y*y;
    posmsd += y1*y1;
    negmsd += y2*y2;
  }

  if(posmsd > negmsd)
  {
    step = -step;
  }

  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node = **it;
    
    if(!node.is_movable())
      continue;

    double x = node.left() - xzero;
    double y = node.top() - yzero;
    rotate_point(&x, &y, step);

    //!
    /*
    xzero= std::min(xzero, _right - node.width());
    yzero= std::min(yzero, _bottom - node.height());
    xzero= std::min(xzero, _maxw - node.width());
    yzero= std::min(yzero, _maxh - node.height());
    */

    node.setnewleft(x + xzero);
    node.setnewtop( y + yzero);
    node.apply();
  }
}

void GraphRenderer::shift_to_origin()
{
  GraphRenderer::GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node= **it;
    node.setnewleft(node.left() - _left + _margin);
    node.setnewtop(node.top() - _top + _margin);
    node.apply();
  }

  _right -= _left;
  _bottom -= _top;
  _left= _top= 0;
}

void GraphRenderer::scale(double xfactor, double yfactor)
{
  GraphRenderer::GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node= **it;
    node.setnewleft(node.left()*xfactor);
    node.setnewtop(node.top()*yfactor);
    node.apply();
  }
}

bool GraphRenderer::has_nonmovable_nodes() const
{
  GraphRenderer::GraphNodeRefList::const_iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::const_iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node= **it;
    if(!node.is_movable())
      return true;
  }
  return false;
}

void GraphRenderer::scale_up()
{
  double scale_factor_x= 1;
  double scale_factor_y= 1;
  
  GraphNodeRefList::const_iterator e= _allnodes.end();
  for(GraphNodeRefList::const_iterator it= _allnodes.begin(); it != e; ++it)
  {
    GraphNode& node= **it;
    double l= node.left();
    double t= node.top();
    double w= node.width();
    double h= node.height();

    GraphNodeRefList::const_iterator it2= it;
    it2++;
    for(; it2 != e; it2++)
    {
      GraphNode& other= **it2;
      double ol= other.left();
      double ot= other.top();
      double ow= other.width();
      double oh= other.height();

      if(nodes_intersect(l, t, w, h, ol, ot, ow, oh))
      {
        double minl= 0;
        double maxl= 0;
        double minw= 0;
        
        if(l < ol)
        {
          minl= l;
          maxl= ol;
          minw= w;
        }
        else
        {
          minl= ol;
          maxl= l;
          minw= ow;
        }

        if((minl + minw + _margin) > maxl)
        {
          double f= (minw + _margin)/(maxl - minl);
          if(f > scale_factor_x)
            scale_factor_x= f;
        }

        if(t < ot)
        {
          minl= t;
          maxl= ot;
          minw= h;
        }
        else
        {
          minl= ot;
          maxl= t;
          minw= oh;
        }

        if((minl + minw + _margin) > maxl)
        {
          double f= (minw + _margin)/(maxl - minl);
          if(f > scale_factor_y)
            scale_factor_y= f;
        }
      }
    }
  }

  scale(scale_factor_x, scale_factor_y);
}

void GraphRenderer::scale_down()
{
  double maxw= _maxw - 2*_margin;
  double maxh= _maxh - 2*_margin;
  
  if((maxw >= (_right - _left)) && (maxh >= (_bottom - _top))) 
    return;
  
  double xfactor= maxw < (_right - _left) ? maxw/(_right - _left) : 1;
  double yfactor= maxh < (_bottom - _top) ? maxh/(_bottom - _top) : 1;

  scale(xfactor, yfactor);
}

void GraphRenderer::recalc()
{
  unsigned count= 0;
  unsigned iter_count= 200;
  bool has_nonmovable= has_nonmovable_nodes();

  double temp_maxw= _maxw;
  double temp_maxh= _maxh;

  _maxw= 200;
  _maxh= 200;
  
  if(!has_nonmovable)
  {
    recalc_outer_rect();
    scale_down();
  }

  _maxw= temp_maxw;
  _maxh= temp_maxh;

  recalc_focus_nodes();

  while(!is_steady() && (count < iter_count))
  {
    recalc_length();
    recalc_positions();
    rotate();
    recalc_outer_rect();
    count++;
  }

  if(!is_steady()) 
  {
    count= 0;
    recalc_focus_nodes();
    while(has_intersections() && (count < iter_count))
    {
      recalc_length();
      recalc_positions();
      rotate();
      recalc_outer_rect();
      count++;
    }
  }

  recalc_outer_rect();
  shift_to_origin();

  if(!has_nonmovable)
  {
    recalc_outer_rect();
    scale_up();

    recalc_outer_rect();
    scale_down();

    recalc_outer_rect();
    shift_to_origin();
  }
}

void GraphRenderer::get_outer_rect(double *left, double *top, double *right, double *bottom)
{
  *left= _left;
  *top= _top;
  *right= _right;
  *bottom= _bottom;
}

void GraphRenderer::recalc_length()
{
  int l[4] = {0};
  _density_ratio = 0.;
  double cx= (_left+_right)/2.;
  double cy= (_top+_bottom)/2.;

  GraphRenderer::GraphNodeRefList::iterator e= _allnodes.end();
  for(GraphRenderer::GraphNodeRefList::iterator it= _allnodes.begin(); it != e; ++it) 
  {
    GraphNode& node= **it;
    _density_ratio += node.width()*node.height();
    int idx= 0;
    idx += node.centerx() < cx ? 0 : 1;
    idx += node.centery() < cy ? 0 : 2;
    l[idx]++;
  }
  _density_ratio /= (_right - _left)*(_bottom - _top);
  _length= 1000.*_density_ratio*_density_ratio;
}
