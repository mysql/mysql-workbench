#ifndef __GRAPHRENDERER_H__
#define __GRAPHRENDERER_H__

#include <math.h>
#include <list>
#include <algorithm>
#include <set>
#include <functional>

class GraphNode
{
  double _left, _top, _width, _height;
  double _newleft, _newtop;
  bool _visited, _focus, _movable;

public:

  enum Coord { CX, CY };

  GraphNode(const GraphNode& n) : _left(n._left), _top(n._top), _width(n._width), _height(n._height), 
    _newleft(n._newleft), _newtop(n._newtop), _visited(n._visited), _focus(n._focus), _movable(n._movable) {}
  GraphNode(double l, double t, double w = 50., double h = 50.) : _left(l), _top(t), _width(w), _height(h), _newleft(l), _newtop(t),
    _visited(false), _focus(false), _movable(true) {}

  double centerx() const { return _left + _width/2.; }
  double centery() const { return _top + _height/2.; }

  double left() const { return _left; }
  double top() const { return _top; }
  double width() const { return _width; }
  double height() const { return _height; }

  void setnewleft(double l) { _newleft = l; }
  void setnewtop(double t) { _newtop = t; }

  double newleft() const { return _newleft; }
  double newtop() const { return _newtop; }

  double coord(Coord c) const { return c == CX ? _left : _top; }
  
  void apply() { _left= _newleft; _top= _newtop; }

  void set_visited(bool v) { _visited= v; }
  bool is_visisted() const { return _visited; }

  void set_focus(bool f) { _focus= f; }
  bool is_focus() const { return _focus; }

  void set_movable(bool m) { _movable= m; }
  bool is_movable() const { return _movable; }

  friend bool operator == (const GraphNode& n1, const GraphNode& n2);
  static double distance(const GraphNode& n1, const GraphNode& n2);
};

class GraphEdge
{
  GraphNode& _n1;
  GraphNode& _n2;
  bool _special;  // flag to show that the node was added to concatenate graph parts

public:

  GraphEdge(GraphNode& n1, GraphNode &n2) : _n1(n1), _n2(n2), _special(false) {}
  GraphEdge(GraphNode& n1, GraphNode &n2, bool special) : _n1(n1), _n2(n2), _special(special) {}

  GraphEdge& operator = (const GraphEdge& other) { this->_n1= other._n1; this->_n2= other._n2; this->_special= other._special; return *this; }

  bool contains(const GraphNode& n) const { return (n == _n1) || (n == _n2); }
  GraphNode& get_first() const { return _n1; }
  GraphNode& get_second() const { return _n2; }
  GraphNode& get_other(const GraphNode& n) const { return (n == _n1) ? _n2 : _n1; }
  double get_length() const { return sqrt(_n1.centerx()*_n2.centerx() + _n1.centery()*_n2.centery()); }

  bool is_special() const { return _special; }
};

template<class V, int N= 10, int M= 10>
class SequenceStats
{
  int _count;
  V _seq[N+M];

public:
  SequenceStats();
  ~SequenceStats() {}

  void add(V v);
  bool is_steady() const;  // do the last N values fit into the range formed by previous M values
                           // in other words - did the sequence become steady
};

template<class V, int N, int M> SequenceStats<V, N, M>::SequenceStats()
{
  _count= 0;
  for(int i = 0; i < N+M; i++) 
  {
    _seq[i]= 0;
  }
}

template<class V, int N, int M> void SequenceStats<V, N, M>::add(V v)
{
  for(int i = 1; i < (N+M); i++) 
  {
    _seq[i-1]= _seq[i];
  }
  _seq[N+M-1]= v;
  _count++;
}

template<class V, int N, int M> bool SequenceStats<V, N, M>::is_steady() const
{
  if(_count < (N+M))
  {
    return false;
  }
  
  V lo= _seq[0], hi= _seq[0];

  for(int i = 1; i < N; i++) 
  {
    if(_seq[i] < lo)
    {
      lo= _seq[i];
    }
    else if(_seq[i] > hi)
    {
      hi= _seq[i];
    }
  }

  for(int i= N; i < M; i++)
  {
    if(_seq[i] < lo)
    {
      return false;
    }
    if(_seq[i] > hi)
    {
      return false;
    }
  }

  return true;
}

class GraphRenderer
{
public:
  
  typedef std::list<GraphNode *> GraphNodeRefList;
  typedef std::list<GraphEdge> GraphEdgeList;

private:
 
  typedef std::pair<double, double> CoordPair;
  typedef std::set<CoordPair> CoordSet;
  typedef SequenceStats<double, 100, 200> LengthStats;
  typedef SequenceStats<double, 2, 100> LocStats;

  static const int K1F = 3;
  static const int K1N = 1;
  static const int K2 = 1000;
  static const int K3 = 1000;

  bool focus_recalced;
  double _density_ratio;
  
  double _length;                       // zero-resistance edge length
  double _margin, _maxw, _maxh;         // workspace edge margin, max width and height
  double _left, _top, _right, _bottom;  // enclosing rect
  double _avg_force;
  GraphEdgeList _alledges;
  GraphNodeRefList _allnodes;

  //double get_delta(const GraphNode& node, GraphNode::Coord coord);
  void get_delta(const GraphNode& node, double *xdelta, double *ydelta);

  bool has_nonmovable_nodes() const;
  bool is_focus_node(const GraphNode& node) const;
  // special edge is an edge that is not present on the model
  // but added only to connect standalone nodes
  void add_special_edge(GraphNode *n1, GraphNode *n2);  
  void mark_reachable(GraphNode& node);
  void mark_neighbours(const GraphNode& node);
  void concat_graph(GraphNode& node);

  void shorten_lengths();
  void recalc_length();
  void recalc_positions();
  void recalc_outer_rect();
  void rotate_point(double *x, double *y, double angle);
  void recalc_focus_nodes();
  void shift_to_origin();
  void scale_up();
  void scale_down();
  void scale(double xfactor, double yfactor);

public:
  GraphRenderer(double margin, double maxwidth, double maxheight);
  ~GraphRenderer();

  GraphNode* add_node(double left, double top, double right, double bottom);
  void add_edge(GraphNode *, GraphNode *);

  const GraphNodeRefList& get_all_nodes() const { return _allnodes; }
  GraphNodeRefList& get_all_nodes() { return _allnodes; }
  const GraphEdgeList& get_all_edges() const { return _alledges; }

  void move(double dx, double dy);
  void recalc();
  void rotate();
  void get_outer_rect(double *left, double *top, double *right, double *bottom);
  bool is_steady() const { return (_avg_force < 2) && (_avg_force >= 0) && !has_intersections(); }
  bool has_intersections() const;

  // stats
  double get_density_ratio() const { return _density_ratio; }
  double get_length() const { return _length; }
  double get_avg_force() const { return _avg_force; }
};

#endif
