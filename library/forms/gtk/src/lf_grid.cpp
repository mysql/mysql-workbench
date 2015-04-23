/* 
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "../lf_mforms.h"
#include "../lf_view.h"
#include "mforms/grid.h"

#include <boost/shared_ptr.hpp>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <gtkmm/liststore.h>
#include <gtkmm/celleditable.h>
#include <gtkmm/eventbox.h>
#include <gtkmm/box.h>
#include <gtkmm/scrolledwindow.h>
#include <gdk/gdkkeysyms.h>
#include "gtk_helpers.h"

namespace
{

static Gtk::TreePath cast_path(const mforms::GridPath& s)
{
  Gtk::TreePath p;
  const int size = s.size();
  for (int i = 0; i < size; ++i)
    p.push_back(s[i]);

  return p;
}

static mforms::GridPath cast_path(const Gtk::TreePath& s)
{
  mforms::GridPath p;
  const int size = s.size();
  for (int i = 0; i < size; ++i)
    p.append(s[i]);

  return p;
}

#define ActionIconVisibiltyQuark   ("avis")
//==============================================================================
//
//==============================================================================
class GridCell
{
  public:
    typedef Gtk::TreeModelColumn<GridCell*>   Column;
    typedef std::vector<std::string>      EnumDef;
    typedef boost::shared_ptr<EnumDef>  EnumDefRef;

    ~GridCell();
    void reset();

    GridCell();
    GridCell(const std::string& atext);
    explicit GridCell(const bool v);

    void set(const std::string& s);
    void set(const bool v);

    const std::string& text() const;
    bool get_value(bool *b) const;
    bool get_value(std::string *s) const;
    std::string as_string() const;

    void set_editable(const bool is_editable) {_editable = is_editable;}
    bool is_editable() const {return _editable;}

    void set_type(const mforms::CellType ct);
    mforms::CellType type() const {return _type;}

    void set_enum_def(EnumDef* list);
    const EnumDefRef get_enum_def() const {return _enum_def;}

    void set_fg(const double red, const double green, const double blue);
    Gdk::Color& fg() {return _fg;}
    bool fg_set() const {return _fg_set;}

    void set_bg(const double red, const double green, const double blue);
    Gdk::Color& bg() {return _bg;}
    bool bg_set() const {return _bg_set;}

    void set_attr(const int attr); // mask built from CellAttr defs
    bool has_attr(const mforms::CellAttr attr) {return _attr & attr;}

    void set_shade(const mforms::Shade cs) {_shade |= (1 << cs);}
    void reset_shade(const mforms::Shade cs) {_shade = 1 << cs;}
    void unset_shade(const mforms::Shade cs) {_shade &= ~(1 << cs);}
    bool has_shade(const mforms::Shade cs) const {return _shade & (1 << cs);}

    void set_action_icon(const std::string& iconpath, const mforms::IconVisibility visible, const mforms::IconPos pos);
    mforms::IconVisibility get_action_icon_visiblity(const mforms::IconPos pos);
    Glib::RefPtr<Gdk::Pixbuf>& get_action_icon(const mforms::IconPos pos);

  private:
    mforms::CellType             _type;
    std::string                  _text;
    bool                         _bool;
    bool                         _editable;
    EnumDefRef                   _enum_def;
    bool                         _fg_set;
    Gdk::Color                   _fg;
    bool                         _bg_set;
    Gdk::Color                   _bg;
    int                          _shade;
    int                          _attr;
    Glib::RefPtr<Gdk::Pixbuf>    _left_icon;
    Glib::RefPtr<Gdk::Pixbuf>    _right_icon;
};

//==============================================================================
class GridModelRow
{
  public:
    GridModelRow() {}
    GridModelRow(const size_t n) : _cell(n) {}
    GridModelRow(const GridModelRow& r) : _cell(r._cell)
    {
      if (r._tag.get())
        _tag.reset(new std::string(*r._tag));
      else
        _tag.reset(0); // TODO: maybe better to have ""

      if (r._caption.get())
        _caption.reset(new std::string(*r._caption));
      else
        _caption.reset(0); // TODO: maybe better to have ""
    }

    GridModelRow& operator=(const GridModelRow& r)
    {
      if (&r != this)
      {
        _cell = r._cell;

        if (r._tag.get())
          _tag.reset(new std::string(*r._tag));
        else
          _tag.reset(0); // TODO: maybe better to have ""

        if (r._caption.get())
          _caption.reset(new std::string(*r._caption));
        else
          _caption.reset(0); // TODO: maybe better to have ""
      }
      return *this;
    }

    void resize(const int s)
    {
      _cell.resize(s);
    }

    size_t size() const
    {
      return _cell.size();
    }

    GridCell& operator[](const int n)
    {
      return _cell[n];
    }
    const GridCell& operator[] (const int n) const
    {
      return _cell[n];
    }

    void set_tag(const std::string& tag)
    {
      if (_tag.get())
        (*_tag) = tag;
      else
        _tag.reset(new std::string(tag));
    }

    std::string get_tag() const
    {
      return _tag.get() ? *_tag : std::string();
    }

    void set_caption(const std::string& caption)
    {
      if (_caption.get())
        (*_caption) = caption;
      else
        _caption.reset(new std::string(caption));
    }

    std::string get_caption() const
    {
      return _caption.get() ? *_caption : std::string();
    }
  private:
    std::deque<GridCell>       _cell;
    std::auto_ptr<std::string>  _tag;
    std::auto_ptr<std::string>  _caption;
};

//==============================================================================
class GridModel : public Glib::Object, public Gtk::TreeModel, 
                         virtual public sigc::trackable
{
  protected:
    GridModel(Gtk::TreeView *treeview, const std::string& name);

  public:
    typedef GridModelRow     Cells;

    ~GridModel();

    static Glib::RefPtr<GridModel> create(Gtk::TreeView *treeview, const std::string& name)
    {
      return Glib::RefPtr<GridModel>(new GridModel(treeview, name));
    }

    void add_column();
    int columns_count() const {return _ncols;}
    void clear();

    int get_children_count(const mforms::GridPath& path);

    mforms::GridPath  append_row(const std::string& gid);
    mforms::GridPath  append_row(const mforms::GridPath& path);
    mforms::GridPath  insert_row(const mforms::GridPath& path);

    void remove_row(const mforms::GridPath& path);

    GridCell* cell(const int gid, const int rid, const int column);
    const GridCell* cell(const int gid, const int rid, const int column) const;
    GridCell* cell(const Gtk::TreePath& path, const int col_id, Gtk::TreeIter* iter);
    GridCell* cell(const mforms::GridPath& path, const int col_id);
    GridCell* cell(const Gtk::TreeIter& iter, const int col_id);

    int group_index(const std::string& group_id);
    Cells* row_from_iter(const Gtk::TreeIter& iter);
    Cells* row_from_path(const mforms::GridPath& path);
    std::string group_name(const Gtk::TreeIter& iter);

    void set_cell_attr(const mforms::GridPath& path, const int col_id, const int attr);

  protected:
    virtual Gtk::TreeModelFlags get_flags_vfunc() const;
    virtual int get_n_columns_vfunc() const;
    virtual GType get_column_type_vfunc(int index) const;

    virtual bool iter_next_vfunc(const iterator& iter, iterator& iter_next) const;
    virtual bool get_iter_vfunc(const Path& path, iterator& iter) const;
    virtual bool iter_children_vfunc(const iterator& parent, iterator& iter) const;
    virtual bool iter_parent_vfunc(const iterator& child, iterator& iter) const;
    virtual bool iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const;
    virtual bool iter_nth_root_child_vfunc(int n, iterator& iter) const;
    virtual bool iter_has_child_vfunc(const iterator& iter) const;
    virtual int iter_n_children_vfunc(const iterator& iter) const;
    virtual int iter_n_root_children_vfunc() const;
    virtual TreeModel::Path get_path_vfunc(const iterator& iter) const;
    virtual bool iter_is_valid(const iterator& iter) const;
    virtual void get_value_vfunc(const iterator& iter, int column, Glib::ValueBase& value) const;
    virtual void set_value_impl(const iterator& row, int column, const Glib::ValueBase& value);
    virtual void get_value_impl(const iterator& row, int column, Glib::ValueBase& value) const;

  protected:
    void reset_iter(iterator& iter) const throw();

  private:
    bool fill(const int gid, const int rid, Gtk::TreeIter* iter, Gtk::TreePath *path);
    int                                           _stamp;
    int                                           _ncols;

    struct GridRow
    {
      std::string            header_text;
      Cells                  row;
      std::deque<Cells>      rows;
    };

    mutable std::deque<GridRow>     _root;

    void setup_group(GridRow* row, const std::string& group_name);
    void set_group_name(GridRow* row, const std::string& name);
    std::string group_name(GridRow* row) const;
};

//==============================================================================
//
//==============================================================================
class GridCellEditable : public Gtk::EventBox, public Gtk::CellEditable
{
  public:
    GridCellEditable();

    void set_type(const mforms::CellType type) {_type = type;}
    void set_enum_def(const GridCell::EnumDefRef def);

    void set_text(const std::string& s);
    std::string get_text();
    void set_path(const Glib::ustring& path) {_path = path;}
    Glib::ustring path() const {return _path;};
    Gtk::TreePath gtk_path() const {return Gtk::TreePath(_path);};

  protected:
    virtual void start_editing_vfunc(GdkEvent* event);
    virtual void on_editing_done();

  private:
    bool on_key_release(GdkEventKey* ev);

    mforms::CellType                    _type;
    Gtk::ComboBoxEntryText      _combo;
    Glib::ustring               _path;
};

class GridView;

//==============================================================================
//
//==============================================================================
class GridCellRenderer : public Gtk::CellRendererText
{
  public:
    GridCellRenderer(const int model_column, Gtk::TreeView* tv, GridView* gv);

    void cell_data(Gtk::CellRenderer*, const Gtk::TreeModel::iterator& it);

    void activate(const Gtk::TreePath& path);

    bool hits_click_area(const int x, const int y, const Gdk::Rectangle& area, const mforms::CellType type);

    int column() const {return _colid;}

    GridCell* cell_from(const Glib::ustring& path);
    GridCell* cell_from(const Gtk::TreePath& path);
    GridCell* cell_from(const Gtk::TreeIter& path);
  protected:
    virtual void on_edited(const Glib::ustring& path, const Glib::ustring& new_text);
    virtual void on_editing_canceled();

    #ifdef GLIBMM_VFUNCS_ENABLED
    virtual void render_vfunc(const Glib::RefPtr<Gdk::Drawable>& window, Gtk::Widget& widget, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, const Gdk::Rectangle& expose_area, Gtk::CellRendererState flags);
    virtual Gtk::CellEditable* start_editing_vfunc(GdkEvent* event, Gtk::Widget& widget, const Glib::ustring& path, const Gdk::Rectangle& background_area, const Gdk::Rectangle& cell_area, Gtk::CellRendererState flags);
    virtual void get_size_vfunc(Gtk::Widget& widget, const Gdk::Rectangle* cell_area, int* x_offset, int* y_offset, int* width, int* height) const;
    #endif //GLIBMM_VFUNCS_ENABLED

  private:
    void do_shading(const Glib::RefPtr<Gdk::Drawable>& window, const Gdk::Rectangle& background_area);
    void combo_item_selected(const Glib::ustring& path);
    void editable_edit_done();
    const int                     _colid;
    Gtk::TreeView                *_tv;
    Glib::RefPtr<GridModel>       _store;
    GridCellEditable              _editable;
    GridCell                     *_cell;
    GridView                     *_gv;
    Gtk::TreeModel::iterator      _iter;
};

//==============================================================================
//
//==============================================================================
class GridView : public mforms::gtk::ViewImpl
{
  protected:
    virtual Gtk::Widget* get_outer() const { return &_scroll; }
  public:
    typedef mforms::GridPath  Path;
    typedef std::string     Value;

    GridView(mforms::Grid* self);

    int add_column(const std::string& name);
    void clear();

    int get_children_count(const Path& path);
    bool is_node_expanded(const Path& path);
    void set_node_expanded(const Path& path, const bool expanded);

    void set_column_width(const int column, const int width);

    Path append_row(const std::string& group_name);
    Path append_row(const Path& path);
    Path insert_row(const Path& path);
    void remove_row(const Path& path);

    bool set_value(const Path& rid, const int col_id, const std::string& cv, const bool editable);
    bool set_value(const Path& rid, const int col_id, const char* cv, const bool editable);
    bool set_value(const Path& rid, const int col_id, bool cv, const bool editable);
    std::string get_value(const Path& rid, const int col_id);
    std::string get_value(const Path& rid, const int col_id, mforms::CellType* type);

    void set_cell_type(const Path& path, const int col_id, const mforms::CellType type);
    void set_cell_attr(const Path& path, const int col_id, const int attr);

    bool set_fg(const Path& rid, const int col_id, const double r, const double g, const double b);
    bool set_bg(const Path& rid, const int col_id, const double r, const double g, const double b);

    // ownership of the vector @list will be passed to the GridCell
    bool set_enum_def(const Path& rid, const int col_id, std::vector<std::string>* list);
    // ownership of the char** @list will not be passed to the GridCell
    bool set_enum_def(const Path& rid, const int col_id, const char** const list);

    void shade(const Path& rid, const mforms::Shade shade, const int col_id = -1);
    void unshade(const Path& rid, const mforms::Shade shade, const int col_id = -1);
    bool has_shade(const Path& rid, const int col_id, const mforms::Shade s);
    void scroll_to_row(const Path& rid);

    void set_row_tag(const Path& path, const std::string& tag);
    std::string get_row_tag(const Path& path);

    void set_row_caption(const Path& path, const std::string& tag);
    std::string get_row_caption(const Path& path);

    void set_action_icon(const Path& rid, const int col, const std::string& iconpath, const mforms::IconVisibility visible, const mforms::IconPos pos);
    void popup_context_menu();

    void content_edited(const Path& p, const int col)
    {
      (*_mgrid->signal_content_edited())(p, col);
    }

    int last_pointer_x() const {return _last_pointer_x;}
    int last_pointer_y() const {return _last_pointer_y;}


  private:
    void _do_init();
    bool on_event_slot(GdkEvent*);

    mforms::Grid                 *_mgrid;

    Glib::RefPtr<GridModel>             _model;
    mutable Gtk::ScrolledWindow         _scroll;
    Gtk::TreeView                       _tree;
    bool                                _init_done;

    void text_cell_data(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& it, const int colid);

    void edited_slot(const Glib::ustring& path, const Glib::ustring& new_value, const int col);
    void edited_bool_slot(const Glib::ustring& path, const int col);
    void row_activated_slot(const Gtk::TreeModel::Path&, Gtk::TreeViewColumn*);

    void track_pointer(GdkEventMotion* event)
    {
      _tree.widget_to_tree_coords(event->x, event->y, _last_pointer_x, _last_pointer_y);
    }

    int   _last_pointer_x;
    int   _last_pointer_y;
};


#define dprintf(...)
//#define dprintf     printf

//------------------------------------------------------------------------------
GridCell::GridCell()
     : _type(mforms::CellInvalid)
     , _fg_set(false)
     , _bg_set(false)
     , _shade(mforms::ShadeNone)
     , _attr(mforms::AlignLeft)
{}

//------------------------------------------------------------------------------
GridCell::GridCell(const std::string& atext)
     : _type(mforms::CellText)
     , _text(atext)
     , _fg_set(false)
     , _bg_set(false)
     , _shade(mforms::ShadeNone)
     , _attr(mforms::AlignLeft)
{}

//------------------------------------------------------------------------------
GridCell::GridCell(const bool v)
     : _type(mforms::CellBool)
     , _bool(v)
     , _fg_set(false)
     , _bg_set(false)
     , _shade(mforms::ShadeNone)
     , _attr(mforms::AlignLeft)
{}

//------------------------------------------------------------------------------
GridCell::~GridCell()
{
  reset();
}

//------------------------------------------------------------------------------
void GridCell::reset()
{
  _enum_def.reset();
  _text.clear();
  _bool = 0;
  _type = mforms::CellInvalid;
  _attr = mforms::AlignLeft;
}

//------------------------------------------------------------------------------
void GridCell::set_type(const mforms::CellType ct)
{
  _type = ct;
  if (_type == mforms::CellHeader || _type == mforms::CellGroupHeader)
    _editable = false;
}

//------------------------------------------------------------------------------
void GridCell::set(const std::string& s)
{
  _text = s;
  if (_type != mforms::CellEnum)
    _type = mforms::CellText;
}

//------------------------------------------------------------------------------
void GridCell::set(const bool s)
{
  _type = mforms::CellBool;
  _bool = s;
  _enum_def.reset();
}

//------------------------------------------------------------------------------
const std::string& GridCell::text() const
{
  return _text;
}

//------------------------------------------------------------------------------
bool GridCell::get_value(bool *b) const
{
  const bool ret = _type == mforms::CellBool;
  if (ret)
    *b = _bool;
  return ret;
}

//------------------------------------------------------------------------------
bool GridCell::get_value(std::string *s) const
{
  bool ret = (_type != mforms::CellInvalid);
  switch (_type)
  {
    case mforms::CellText:
    case mforms::CellEnum:
      *s = _text;
      break;
    default:
      ret = false;
      break;
  }
  return ret;
}

//------------------------------------------------------------------------------
std::string GridCell::as_string() const
{
  std::string ret;
  switch (_type)
  {
    case mforms::CellText:
    case mforms::CellEnum:
    case mforms::CellHeader:
      ret = _text;
      break;
    case mforms::CellBool:
      ret = _bool ? "1" : "0";
      break;
    default:
      break;
  }
  return ret;
}

//------------------------------------------------------------------------------
void GridCell::set_attr(const int attr)
{
  _attr = attr;
}

//------------------------------------------------------------------------------
void GridCell::set_enum_def(GridCell::EnumDef* list)
{
  _enum_def = EnumDefRef(list);
  _type = mforms::CellEnum;
  _editable = true;
}

//------------------------------------------------------------------------------
void GridCell::set_fg(const double red, const double green, const double blue)
{
  _fg.set_rgb_p(red, green, blue);
  _fg_set = true;
}

//------------------------------------------------------------------------------
void GridCell::set_bg(const double red, const double green, const double blue)
{
  _bg.set_rgb_p(red, green, blue);
  _bg_set = true;
}

//------------------------------------------------------------------------------
void GridCell::set_action_icon(const std::string& iconpath, const mforms::IconVisibility visible, const mforms::IconPos pos)
{
  Glib::RefPtr<Gdk::Pixbuf> &pb = (pos == mforms::IconLeft) ? _left_icon : _right_icon;
  if (iconpath.empty())
    pb.reset();
  else
  {
    if ((pos == mforms::IconRight && _type == mforms::CellEnum))
      pb.reset();
    else
    {
      pb = Gdk::Pixbuf::create_from_file(mforms::App::get()->get_resource_path(iconpath));
      pb->set_data(ActionIconVisibiltyQuark, (void*)visible);
    }
  }
}

//------------------------------------------------------------------------------
mforms::IconVisibility GridCell::get_action_icon_visiblity(const mforms::IconPos pos)
{
  Glib::RefPtr<Gdk::Pixbuf> &pb = (pos == mforms::IconLeft) ? _left_icon : _right_icon;
  return pb ? ((mforms::IconVisibility)((long)pb->get_data(ActionIconVisibiltyQuark))) : mforms::NotVisible;
}

//------------------------------------------------------------------------------
Glib::RefPtr<Gdk::Pixbuf>& GridCell::get_action_icon(const mforms::IconPos pos)
{
  return (pos == mforms::IconLeft) ? _left_icon : _right_icon;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
GridModel::GridModel(Gtk::TreeView *treeview, const std::string& name)
     : Glib::ObjectBase(typeid(GridModel))
     , Glib::Object()
     , Gtk::TreeModel()
     , _stamp(3)
     , _ncols(0)
{}

//------------------------------------------------------------------------------
GridModel::~GridModel()
{}

//------------------------------------------------------------------------------
GridModel::Cells* GridModel::row_from_iter(const Gtk::TreeIter& iter)
{
  GridModel::Cells* ret = 0;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    GridRow* root_row = ((int)_root.size() > gid && gid >= 0) ? &(_root[gid]) : 0;
    if (root_row)
    {
      const int rid = (long)it->user_data2;
      if (rid >= 0)
      {
        if (rid < (int)root_row->rows.size())
          ret = &(root_row->rows[rid]);
      }
      else
        ret = &(root_row->row);
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
GridModel::Cells* GridModel::row_from_path(const mforms::GridPath& path)
{
  GridModel::Cells* ret = 0;
  if (path.size() > 0)
  {
    const int gid = path[0];
    GridRow* root_row = ((int)_root.size() > gid && gid >= 0) ? &(_root[gid]) : 0;
    if (root_row)
    {
      const int rid = path.size() > 1 ? path[1] : -1;
      if (rid >= 0)
      {
        if (rid < (int)root_row->rows.size())
          ret = &(root_row->rows[rid]);
      }
      else
        ret = &(root_row->row);
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
void GridModel::add_column()
{
  ++_ncols;
  const int root_size = _root.size();
  for (int i = 0; i < root_size; ++i)
  {
    _root[i].row.resize(_ncols);
    std::deque<Cells>& rows = _root[i].rows;
    const int rows_count = rows.size();
    for (int j = 0; j < rows_count; ++j)
      rows[j].resize(_ncols);
  }
}

//------------------------------------------------------------------------------
void GridModel::clear()
{
  const int rows_count = _root.size();
  for (int i = rows_count - 1; i >= 0; --i)
  {
    Gtk::TreeIter iter;
    Gtk::TreePath path;
    fill(i, -1, &iter, &path);
    row_deleted(path);
    _root.pop_back();
  }
}

//------------------------------------------------------------------------------
Gtk::TreeModelFlags GridModel::get_flags_vfunc() const
{
  return Gtk::TreeModelFlags(0);
}

//------------------------------------------------------------------------------
int GridModel::get_n_columns_vfunc() const
{
  return _ncols;
}

//------------------------------------------------------------------------------
GType GridModel::get_column_type_vfunc(int index) const
{
  return G_TYPE_STRING;
}

//------------------------------------------------------------------------------
bool GridModel::iter_next_vfunc(const iterator& iter, iterator& iter_next) const
{
  bool ret = false;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    GtkTreeIter* itn = iter_next.gobj();
    itn->stamp = 0; // set stamp to initial invalid value
    long gid = (long)it->user_data;
    long rid = (long)it->user_data2;
    if (gid >= 0)
    {
      itn->user_data3 = (void*)-1;
      const GridRow* const row = (long)_root.size() > gid ? &(_root[gid]) : 0;
      // advance row first, if we have row id != -1, otherwise advance gid
      if (rid >= 0 && row)
      {
        const std::deque<Cells>& rows = row->rows;
        if (++rid < (long)rows.size())
        {
          itn->stamp = _stamp;
          itn->user_data  = it->user_data;
          itn->user_data2 = (void*)rid;
          ret = true;
        }
      }
      else if (++gid < (long)_root.size())
      {
        itn->stamp = _stamp;
        itn->user_data  = (void*)gid;
        itn->user_data2 = (void*)-1;
        ret = true;
      }
    }
  }
  dprintf("_stamp=%i, iter_next_vfunc(iter = '%s', iter_next='%s') = %i\n", _stamp, iter2str(iter).c_str(), iter2str(iter_next).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
bool GridModel::get_iter_vfunc(const Path& path, iterator& iter) const
{
  bool ret = false;
  GtkTreeIter* it = iter.gobj();
  const int path_size = path.size();

  if (path_size > 0)
  {
    const long gid = path[0];
    if (gid >= 0 && gid < (long)_root.size())
    {
      it->stamp      = _stamp;
      it->user_data  = (void*)gid;
      it->user_data3 = (void*)-1;
      const GridRow* const row = &(_root[gid]);
      if (path_size > 1)
        it->user_data2 = reinterpret_cast<void*>(path[1] < (long)row->rows.size() ? path[1]: -1);
      else
        it->user_data2 = (void*)-1;
      ret = true;
    }
  }

  dprintf("_stamp=%i, get_iter_vfunc(path = '%s', iter='%s') = %i\n", _stamp, path.to_string().c_str(), iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
bool GridModel::iter_children_vfunc(const iterator& parent, iterator& iter) const
{
  bool ret = false;
  const GtkTreeIter* const pit = parent.gobj();
  GtkTreeIter* it = iter.gobj();
  it->stamp      = 0;
  it->user_data  = (void*)-1;
  it->user_data2 = (void*)-1;
  it->user_data3 = (void*)-1;

  if (pit && pit->stamp == _stamp)
  {
    const int gid = (long)pit->user_data;
    const int rid = (long)pit->user_data2;
    if (gid >= 0 && gid < (int)_root.size() && rid == -1)
    {
      it->stamp = _stamp;
      const GridRow* const root_row = &(_root[gid]);
      if (root_row->rows.size() > 0)
      {
        it->user_data  = pit->user_data;
        it->user_data2 = 0;
        it->user_data3 = (void*)-1;
        ret = true;
      }
      else
        it->stamp = 0;
    }
  }
  dprintf("_stamp=%i, iter_children_vfunc(parent = '%s', iter='%s') = %i\n", _stamp, iter2str(parent).c_str(), iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
bool GridModel::iter_parent_vfunc(const iterator& child, iterator& iter) const
{
  bool ret = false;
  const GtkTreeIter* const cit = child.gobj();
  if (cit && cit->stamp == _stamp)
  {
    GtkTreeIter* it = iter.gobj();
    if (it)
    {
      it->stamp = _stamp;
      it->user_data = cit->user_data;
      it->user_data2 = it->user_data3 = (void*)-1;
      ret = true;
    }
  }
  dprintf("_stamp=%i, iter_parent_vfunc(child = '%s', iter='%s') = %i\n", _stamp, iter2str(child).c_str(), iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
bool GridModel::iter_nth_child_vfunc(const iterator& parent, int n, iterator& iter) const
{
  bool ret = false;
  const GtkTreeIter* const pit = parent.gobj();
  if (pit && pit->stamp == _stamp)
  {
    GtkTreeIter* it = iter.gobj();
    if (it)
    {
      const long gid = (long)pit->user_data;
      if (gid >= 0 && (long)_root.size() > gid && (long)_root[gid].rows.size() > n && n >= 0)
      {
        it->stamp      = _stamp;
        it->user_data  = pit->user_data;
        it->user_data2 = reinterpret_cast<void*>(n);
        it->user_data3 = (void*)-1;
        ret = true;
      }
      else
        it->stamp = 0;
    }
  }
  dprintf("_stamp=%i, iter_nth_child_vfunc(parent = '%s', n=%i, iter='%s') = %i\n", _stamp, iter2str(parent).c_str(), n, iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
bool GridModel::iter_nth_root_child_vfunc(int n, iterator& iter) const
{
  bool ret = false;
  GtkTreeIter* it = iter.gobj();
  if (it)
  {
    if (n >= 0 && n < (long)_root.size())
    {
      it->stamp      = _stamp;
      it->user_data  = reinterpret_cast<void*>(n);
      it->user_data2 = (void*)-1;
      it->user_data3 = (void*)-1;
      ret = true;
    }
    else
      it->stamp = 0;
  }
  dprintf("_stamp=%i, iter_nth_root_children_vfunc(n = %i, iter='%s') = %i\n", _stamp, n, iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
bool GridModel::iter_has_child_vfunc(const iterator& iter) const
{
  bool ret = false;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    const int rid = (long)it->user_data2;
    if (rid == -1)
      ret = gid >= 0 && (int)_root.size() > gid && _root[gid].rows.size() > 0;
  }
  dprintf("_stamp=%i, iter_has_child_vfunc(iter='%s') = %i\n", _stamp, iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
int GridModel::iter_n_children_vfunc(const iterator& iter) const
{
  int ret = 0;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    const int rid = (long)it->user_data2;
    if (rid == -1)
    {
      if (gid > 0 && gid < (int)_root.size())
        ret = _root[gid].rows.size();
    }
  }
  dprintf("_stamp=%i, iter_n_children_vfunc(iter='%s') = %i\n", _stamp, iter2str(iter).c_str(), ret);
  return ret;
}

//------------------------------------------------------------------------------
int GridModel::iter_n_root_children_vfunc() const
{
  dprintf("_stamp=%i, iter_n_root_children_vfunc() = %i\n", _stamp, _root.size());
  return _root.size();
}

//------------------------------------------------------------------------------
Gtk::TreeModel::Path GridModel::get_path_vfunc(const iterator& iter) const
{
  Gtk::TreeModel::Path  path;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    if (gid >= 0 && (int)_root.size() > gid)
    {
      path.append_index(gid);
      const int rid = (long)it->user_data2;
      if (rid >= 0 && rid < (int)_root[gid].rows.size())
        path.append_index(rid);
    }
  }
  return path;
}


//------------------------------------------------------------------------------
bool GridModel::iter_is_valid(const iterator& iter) const
{
  bool ret = false;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    if (gid >= 0 && (int)_root.size() > gid)
    {
      ret = true;
      const int rid = (long)it->user_data2;
      if (rid >= 0 && rid >= (int)(_root[gid].rows.size()))
        ret = false;
    }
  }
  return ret;
}

//------------------------------------------------------------------------------
void GridModel::set_value_impl(const iterator& row, int column, const Glib::ValueBase& value)
{
  const GtkTreeIter* const it = row.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    const int rid = (long)it->user_data2;
    GridCell* gcell = cell(gid, rid, column);

    if (gcell)
    {
      const GValue* const gv = value.gobj();
      const GType type = G_VALUE_TYPE(gv);

      switch (type)
      {
        case G_TYPE_BOOLEAN:
          {
            const bool val = g_value_get_boolean(gv);
            gcell->set(val);
            break;
          }
        case G_TYPE_STRING:
          {
            const std::string val = g_value_get_string(gv);
            gcell->set(val);
            break;
          }
        default:
          {
            break;
          }
      }//case
    }// if(cell)
  }//if (it...
}

//------------------------------------------------------------------------------
void GridModel::get_value_vfunc(const iterator& iter, int column, Glib::ValueBase& value) const
{
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    const int rid = (long)it->user_data2;
    const GridCell* gcell = cell(gid, rid, column);

    if (gcell)
    {
      GValue* gv = value.gobj();

      switch (gcell->type())
      {
        case mforms::CellBool:
          {
            bool bv = false;
            if (gcell->get_value(&bv))
            {
              g_value_init(gv, G_TYPE_BOOLEAN);
              g_value_set_boolean(gv, bv);
            }
            break;
          }
        case mforms::CellText:
        case mforms::CellEnum:
          {
            std::string sv;
            if (gcell->get_value(&sv))
            {
              g_value_init(gv, G_TYPE_STRING);
              g_value_set_string(gv, sv.c_str());
            }
            break;
          }
        case mforms::CellInvalid:
        case mforms::CellHeader:
        case mforms::CellGroupHeader:
          break;
      }
    }
  }
}

//------------------------------------------------------------------------------
void GridModel::get_value_impl(const iterator& row, int column, Glib::ValueBase& value) const
{
  get_value_vfunc(row, column, value);
}

//------------------------------------------------------------------------------
void GridModel::reset_iter(iterator& iter) const throw()
{
  GtkTreeIter* it = iter.gobj();
  if (it)
  {
    it->stamp = 0;
    it->user_data = it->user_data2 = it->user_data3 = (void*)-1;
  }
}


//------------------------------------------------------------------------------
void GridModel::setup_group(GridRow* row, const std::string& group_name)
{
  if (row)
  {
    row->row.resize(_ncols);
    set_group_name(row, group_name);

    GridCell* gc = 0;
    for (int i = ((int)(row->row.size())) - 1; i >= 0; --i)
    {
      if ((gc = &(row->row[i])))
      {
        gc->set(false);
        gc->set_type(mforms::CellHeader);
      }
    }
  }
}

//------------------------------------------------------------------------------
void GridModel::set_group_name(GridRow* row, const std::string& name)
{
  row->header_text = name;
}

//------------------------------------------------------------------------------
std::string GridModel::group_name(GridRow* row) const
{
  return row->header_text;
}

//------------------------------------------------------------------------------
std::string GridModel::group_name(const Gtk::TreeIter& iter)
{
  std::string ret;
  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    if (gid >= 0 && (int)_root.size() > gid)
      ret = group_name(&(_root[gid]));
  }
  return ret;
}

//------------------------------------------------------------------------------
int GridModel::group_index(const std::string& group_id)
{
  int index = -1;
  const int root_size = _root.size();
  for (int i = 0; i < root_size; ++i)
  {
    if (_root[i].header_text == group_id)
    {
      index = i;
      break;
    }
  }
  return index;
}

//------------------------------------------------------------------------------
int GridModel::get_children_count(const mforms::GridPath& path)
{
  int ret = 0;
  const int path_size = path.size();

  if (path_size == 0)
    ret = _root.size();
  else if (path_size == 1)
  {
    const int gid = path[0];
    if (gid >= 0 && (unsigned int)gid < _root.size())
      ret = _root[gid].rows.size();
  }

  return ret;
}

//------------------------------------------------------------------------------
mforms::GridPath GridModel::append_row(const mforms::GridPath& path)
{
  Gtk::TreeIter giter;
  Gtk::TreePath gpath;

  if (path.size() == 1)
  {
    const int gid = path[0];
    if (gid >= 0 && gid < (int)_root.size())
    {
      GridRow* row = &(_root[gid]);
      if (row)
      {
        row->rows.push_back(Cells(_ncols));
        const int rid = row->rows.size() - 1;
        fill(gid, rid, &giter, &gpath);
        row_inserted(gpath, giter);

        Gtk::TreeIter piter;
        Gtk::TreePath ppath;
        fill(gid, -1, &piter, &ppath);
        row_has_child_toggled(ppath, piter);
      }
    }
  }
  else if (path.size() == 0)
  {
    _root.push_back(GridRow());
    const int gid = _root.size() - 1;

    GridRow* row = &(_root.back());
    row->row.resize(_ncols);

    fill(gid, -1, &giter, &gpath);
    row_inserted(gpath, giter);
  }

  return cast_path(gpath);
}

//------------------------------------------------------------------------------
// To add group pass non-existing group name
mforms::GridPath GridModel::append_row(const std::string& gid)
{
  int gindex = group_index(gid);

  GridRow *row = 0;
  if (gindex < 0)
  {
    _root.push_back(GridRow());
    gindex = _root.size() - 1;

    row = &(_root.back());
    setup_group(row, gid);

    Gtk::TreeIter giter;
    Gtk::TreePath gpath;
    fill(gindex, -1, &giter, &gpath);
    row_inserted(gpath, giter);
  }

  Gtk::TreeIter giter;
  Gtk::TreePath rpath;
  fill(gindex, -1, &giter, &rpath);

  return cast_path(rpath);
}

//------------------------------------------------------------------------------
mforms::GridPath GridModel::insert_row(const mforms::GridPath& path)
{
  // empty path -> append_row
  // path is [X], insert before row X
  // path is [X:Y], insert before row X:Y
  Gtk::TreeIter giter;
  Gtk::TreePath gpath;
  const int       gid = path.size() >= 1 ? path[0] : -1;
  const int       rid = path.size() > 1  ? path[1] : -1;
  const int root_size = _root.size();

  if (gid >= 0 && gid < root_size)
  {
    if (rid >= 0)
    {
      // path is [X:Y], insert before row X:Y
      GridRow* row = &(_root[gid]);
      if (row)
      {
        if ((int)row->rows.size() > rid)
        {
          typedef std::deque<Cells>  RowCells;
          RowCells::iterator it = row->rows.begin() + rid;
          row->rows.insert(it, Cells(_ncols));

          fill(gid, rid, &giter, &gpath);
          row_inserted(gpath, giter);

          Gtk::TreeIter piter;
          Gtk::TreePath ppath;
          fill(gid, -1, &piter, &ppath);
          row_has_child_toggled(ppath, piter);
        }
      }
    }
    else
    {
      // path is [X], insert before row X in the _root deque
      typedef std::deque<GridRow> Root;
      Root::iterator it = _root.begin() + gid;
      _root.insert(it, GridRow());

      GridRow* row = &(_root[gid]);
      row->row.resize(_ncols);

      fill(gid, -1, &giter, &gpath);
      row_inserted(gpath, giter);
    }
  }

  //if (do_append)
  //  return append_row(path); //append row

  return cast_path(gpath);
}

//------------------------------------------------------------------------------
void GridModel::remove_row(const mforms::GridPath& path)
{
  Gtk::TreeIter giter;
  Gtk::TreePath gpath;

  if (path.size() == 1)
  {
    const int gid = path[0];
    fill(gid, -1, &giter, &gpath);

    std::deque<GridRow>::iterator it = _root.begin() + gid;
    _root.erase(it);

    row_deleted(gpath);
  }
  else if (path.size() == 2)
  {
    const int gid = path[0];
    const int rid = path[1];

    std::deque<Cells>::iterator it = _root[gid].rows.begin() + rid;
    _root[gid].rows.erase(it);

    fill(gid, rid, &giter, &gpath);
    row_deleted(gpath);
  }
}

//------------------------------------------------------------------------------
GridCell* GridModel::cell(const int gid, const int rid, const int column)
{
  GridCell* cell = 0;
  GridRow* row = ((int)_root.size() > gid && gid >= 0) ? &(_root[gid]) : 0;
  Cells* cells = 0;
  if (row)
  {
    if (rid >= 0)
    {
      if (rid < (int)row->rows.size())
        cells = &(row->rows[rid]);
    }
    else
      cell = &(row->row[column]);
  }

  if (cells && (int)cells->size() > column)
    cell = &((*cells)[column]);

  return cell;
}

//------------------------------------------------------------------------------
const GridCell* GridModel::cell(const int gid, const int rid, const int column) const
{
  const GridCell* cell = 0;
  const GridRow* row = ((int)_root.size() > gid && gid >= 0) ? &(_root[gid]) : 0;
  const Cells* cells = 0;
  if (row)
  {
    if (rid >= 0)
    {
      if (rid < (int)row->rows.size())
        cells = &(row->rows[rid]);
    }
    else
      cell = &(row->row[column]);
  }

  if (cells && (int)cells->size() > column)
    cell = &((*cells)[column]);

  return cell;
}

//------------------------------------------------------------------------------
GridCell* GridModel::cell(const Gtk::TreePath& path, const int column, Gtk::TreeIter* iter)
{
  GridCell* rcell = 0;

  if (get_iter_vfunc(path, *iter))
  {
    const int gid = path.size() > 0 ? path[0] : -1;
    const int rid = path.size() > 1 ? path[1] : -1;

    rcell = cell(gid, rid, column);

    if (rcell)
    {
      fill(-1, -1, iter, 0);

      GtkTreeIter* it = iter->gobj();
      it->stamp = 0;
    }
  }
  return rcell;
}

//------------------------------------------------------------------------------
GridCell* GridModel::cell(const mforms::GridPath& path, const int column)
{
  const int gid = path.size() > 0 ? path[0] : -1;
  const int rid = path.size() > 1 ? path[1] : -1;

  GridCell* rcell = cell(gid, rid, column);

  return rcell;
}

//------------------------------------------------------------------------------
GridCell* GridModel::cell(const Gtk::TreeIter& iter, const int column)
{
  GridCell* gcell = 0;

  const GtkTreeIter* const it = iter.gobj();
  if (it && it->stamp == _stamp)
  {
    const int gid = (long)it->user_data;
    const int rid = (long)it->user_data2;

    gcell = cell(gid, rid, column);
  }

  return gcell;
}

//------------------------------------------------------------------------------
bool GridModel::fill(const int gid, const int rid, Gtk::TreeIter* iter, Gtk::TreePath *path)
{
  GtkTreeIter* it = iter->gobj();
  it->stamp = _stamp;
  it->user_data = reinterpret_cast<void*>(gid);
  it->user_data2 = reinterpret_cast<void*>(rid);
  it->user_data3 = (void*)-1;

  if (path && gid >= 0)
    path->append_index(gid);
  if (path && rid >= 0)
    path->append_index(rid);

  return true;
}

//------------------------------------------------------------------------------
void GridModel::set_cell_attr(const mforms::GridPath& path, const int col_id, const int attr)
{
  GridCell *gc = cell(path, col_id);
  if (gc)
    gc->set_attr(attr);
}

#define CELLCHECK_XOFF  8
#define CELLCHECK_YOFF  4

//------------------------------------------------------------------------------
GridCellEditable::GridCellEditable()
     : Glib::ObjectBase("GridCellEditable")
{
  _combo.get_entry()->property_has_frame() = false;
  _combo.get_entry()->gobj()->is_cell_renderer = true;
  add(_combo);
  show_all_children();

  signal_key_release_event().connect(sigc::mem_fun(this, &GridCellEditable::on_key_release));
  _combo.signal_changed().connect(sigc::mem_fun(_combo.get_entry(), &Gtk::Entry::grab_focus));
}

//------------------------------------------------------------------------------
bool GridCellEditable::on_key_release(GdkEventKey* ev)
{
  if (ev->keyval == GDK_Escape || ev->keyval == GDK_Return)
    editing_done();
  return true;
}

//------------------------------------------------------------------------------
void GridCellEditable::start_editing_vfunc(GdkEvent* event)
{
  _combo.get_entry()->start_editing(event);
  _combo.get_entry()->grab_focus();
}

//------------------------------------------------------------------------------
void GridCellEditable::on_editing_done()
{
  hide();
}

//------------------------------------------------------------------------------
void GridCellEditable::set_text(const std::string& s)
{
  _combo.get_entry()->set_text(s);
}

//------------------------------------------------------------------------------
std::string GridCellEditable::get_text()
{
  return _combo.get_entry()->get_text();
}

//------------------------------------------------------------------------------
void GridCellEditable::set_enum_def(const GridCell::EnumDefRef def)
{
  if (def)
  {
    _combo.clear_items();
    const int size = def->size();
    for (int i = 0; i < size; ++i)
    {
      _combo.append_text((*def)[i]);
    }
  }
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
GridCellRenderer::GridCellRenderer(const int model_column, Gtk::TreeView* tv, GridView* gv)
                 : Glib::ObjectBase(typeid(GridCellRenderer))
                 , _colid(model_column)
                 , _tv(tv)
                 , _cell(0)
                 , _gv(gv)
{
  _editable.signal_editing_done().connect(sigc::mem_fun(this, &GridCellRenderer::editable_edit_done));
}

//------------------------------------------------------------------------------
GridCell* GridCellRenderer::cell_from(const Glib::ustring& path)
{
  GridCell *cell(0);

  if (!_store)
    _store = Glib::RefPtr<GridModel>::cast_static(_tv->get_model());

  if (_store)
  {
    Gtk::TreeIter iter;
    cell = _store->cell(Gtk::TreePath(path), _colid, &iter);
  }

  return cell;
}

//------------------------------------------------------------------------------
GridCell* GridCellRenderer::cell_from(const Gtk::TreePath& path)
{
  GridCell *cell(0);

  if (!_store)
    _store = Glib::RefPtr<GridModel>::cast_static(_tv->get_model());

  if (_store)
  {
    Gtk::TreeIter iter;
    cell = _store->cell(path, _colid, &iter);
  }

  return cell;
}

//------------------------------------------------------------------------------
GridCell* GridCellRenderer::cell_from(const Gtk::TreeIter& iter)
{
  GridCell *cell(0);

  if (!_store)
    _store = Glib::RefPtr<GridModel>::cast_static(_tv->get_model());

  if (_store)
  {
    cell = _store->cell(iter, _colid);
  }

  return cell;
}

//------------------------------------------------------------------------------
void GridCellRenderer::editable_edit_done()
{
  edited(_editable.path(), _editable.get_text());
  _gv->content_edited(cast_path(_editable.gtk_path()), _colid);
}

//------------------------------------------------------------------------------
void GridCellRenderer::on_editing_canceled()
{
}

//------------------------------------------------------------------------------
void GridCellRenderer::on_edited(const Glib::ustring& path, const Glib::ustring& new_text)
{
  GridCell *cell = cell_from(path);
  if (cell)
  {
    cell->set(new_text);
    _gv->content_edited(cast_path(Gtk::TreePath(path)), _colid);
  }
}

//------------------------------------------------------------------------------
void blurred_line(cairo_t* cr, const int x1, const int y1, const int x2, const int y2)
{
  const int    width[] = {5  , 1};
  const double color[] = {0.8, 0};
  const int size = sizeof(width) / sizeof(int);

  for (int i = 0; i < size; ++i)
  {
    cairo_set_line_width(cr, width[i]);
    cairo_set_source_rgb(cr, 1, color[i], color[i]);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_stroke(cr);
  }
}

//------------------------------------------------------------------------------
void GridCellRenderer::do_shading(const Glib::RefPtr<Gdk::Drawable>& window, const Gdk::Rectangle& background_area)
{
  Cairo::RefPtr<Cairo::Context> ctxmm = window->create_cairo_context();
  cairo_t* cr = ctxmm->cobj();
  cairo_save(cr);
  if (_cell->has_shade(mforms::ShadeFind))
  {
    const GridModel::Cells* row = _store->row_from_iter(_iter);
    if (row)
    {
      const GridCell* prev_cell = (row && (_colid - 1) >= 0)? &((*row)[_colid - 1]) : 0;
      const GridCell* next_cell = (row && ((_colid + 1) < (int)row->size()))? &((*row)[_colid + 1]) : 0;

      //cairo_set_source_rgba(cr, 1, 0, 0, 1);
      cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
      //cairo_set_line_width(cr, 1);

      const int x1 = background_area.get_x();
      const int y1 = background_area.get_y();
      const int x2 = x1 + background_area.get_width();
      const int y2 = y1 + background_area.get_height();
      blurred_line(cr, x1, y1 + 2, x2, y1 + 2);
      blurred_line(cr, x1, y2 - 2, x2, y2 - 2);

      bool draw_left = false;
      if (prev_cell)
      {
        if (!prev_cell->has_shade(mforms::ShadeFind))
          draw_left = true;
      }
      else
        draw_left = true;

      if (draw_left)
        blurred_line(cr, x1, y1 + 2, x1, y2 - 2);

      bool draw_right = false;
      if (next_cell)
      {
        if (!next_cell->has_shade(mforms::ShadeFind))
          draw_right = true;
      }
      else
        draw_right = true;

      if (draw_right)
        blurred_line(cr, x2, y1 + 2, x2, y2 - 2);

      cairo_stroke(cr);
    }
  }

  if (_cell->has_shade(mforms::ShadeFilter))
  {
    const Gdk::Color clr = _tv->get_style()->get_base(Gtk::STATE_NORMAL);
    cairo_set_source_rgba(cr, clr.get_red_p(), clr.get_green_p(), clr.get_blue_p(), 0.6);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    gdk_cairo_rectangle(cr, background_area.gobj());
    cairo_fill(cr);
  }
  cairo_restore(cr);
}

//------------------------------------------------------------------------------
void GridCellRenderer::get_size_vfunc(Gtk::Widget& widget, const Gdk::Rectangle* cell_area, int* x_offset, int* y_offset, int* width, int* height) const
{
  Gtk::CellRendererText::get_size_vfunc(widget, cell_area, x_offset, y_offset, width, height);
  if (height && _cell && _cell->type() == mforms::CellHeader)
    *height += 4;
}

#ifdef DEBUG
std::string rect2str(const Gdk::Rectangle& r)
{
  char buf[128];
  sprintf(buf, "(%i,%i,%i,%i)", r.get_x(), r.get_y(), r.get_width(), r.get_height());
  return buf;
}
#endif

//------------------------------------------------------------------------------
static bool is_in_rect(const int x, const int y, const int x1, const int y1, const int w, const int h)
{
  const int dx = x - x1;
  const int dy = y - y1;
  return (dx >= 0) && (dy >= 0) && (dx <= w) && (dy <= h);
}

//------------------------------------------------------------------------------
void GridCellRenderer::render_vfunc(const Glib::RefPtr<Gdk::Drawable>& window
                                      ,Gtk::Widget& widget
                                      ,const Gdk::Rectangle& bg_area
                                      ,const Gdk::Rectangle& cell_area
                                      ,const Gdk::Rectangle& expose_area,
                                      Gtk::CellRendererState flags
                                      )
{
  bool needs_text_render = true;
  property_underline() = Pango::UNDERLINE_NONE;
  property_alignment() = Pango::ALIGN_LEFT;
  property_foreground_set() = false;
  property_background_set() = false;
  set_alignment(0, 0.01);
  property_scale() = 1;
  bool draw_bg = false;
  bool draw_header = false;
  Gdk::Color bg;
  std::string cont;
  mforms::CellType type = mforms::CellInvalid;

  if (_cell)
  {
    type = _cell->type();
    if (_cell->fg_set())
    {
      Gdk::Color &color = _cell->fg();
      widget.get_colormap()->alloc_color(color, true, true);
      property_foreground_gdk() = color;
    }

    if (_cell->bg_set() && !(flags & Gtk::CELL_RENDERER_SELECTED))
    {
      widget.get_colormap()->alloc_color(_cell->bg(), true, true);
      bg = _cell->bg();
      draw_bg = true;
    }

    if (_cell->has_attr(mforms::Underline))
      property_underline() = Pango::UNDERLINE_LOW;

    if (_cell->has_attr(mforms::AlignRight))
    {
      property_alignment() = Pango::ALIGN_RIGHT;
      set_alignment(1.0, 0.5);
    }

    if (type == mforms::CellHeader)
    {
      bg = _tv->get_style()->get_base(Gtk::STATE_PRELIGHT);
      draw_bg = true;
      if (_colid == _store->columns_count() - 1)
      {
        property_text() = _store->group_name(_iter);

        Gdk::Color fg = _tv->get_style()->get_text(Gtk::STATE_PRELIGHT);
        property_foreground_gdk() = fg;
        draw_header = true;
      }
      needs_text_render = false;
    }
    if (type == mforms::CellGroupHeader)
    {
      const GridModel::Cells* row = _store->row_from_iter(_iter);
      if (row)
      {
        property_text() = row->get_caption();
        draw_header = true;
        needs_text_render = false;
      }
    }
  }

  if (draw_bg)
  {
    Cairo::RefPtr<Cairo::Context> ctxmm = window->create_cairo_context();
    cairo_t* cr = ctxmm->cobj();
    cairo_set_source_rgba(cr, bg.get_red_p(), bg.get_green_p(), bg.get_blue_p(), 0.9);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    gdk_cairo_rectangle(cr, bg_area.gobj());

    cairo_fill(cr);
  }

  int left_icon_width  = 0;
  int right_icon_width = 0;
  bool editable = false;

  if (_cell)
  {
    editable = _cell->is_editable();
    const int mouse_x = _gv->last_pointer_x();
    const int mouse_y = _gv->last_pointer_y();

    // Fix cell_area and expose_area if we have action icons
    const bool mouse_ptr_hit_icon_rect = is_in_rect(mouse_x, mouse_y, bg_area.get_x(), bg_area.get_y(), bg_area.get_width(), bg_area.get_height());
    const Glib::RefPtr<Gdk::Pixbuf> left  = _cell->get_action_icon(mforms::IconLeft);
    if (left)
    {
      const mforms::IconVisibility icon_visibility = _cell->get_action_icon_visiblity(mforms::IconLeft);
      left_icon_width = left->get_width();
      //const bool mouse_ptr_hit_icon_rect = is_in_rect(mouse_x, mouse_y, bg_area.get_x(), bg_area.get_y(), left_icon_width, bg_area.get_height());
      if (icon_visibility == mforms::ShowAlways || (icon_visibility == mforms::ShowOnHover && mouse_ptr_hit_icon_rect))
        window->draw_pixbuf(left, 0, 0, bg_area.get_x(), bg_area.get_y(), -1, bg_area.get_height(), Gdk::RGB_DITHER_NONE, 0, 0);
    }

    const Glib::RefPtr<Gdk::Pixbuf> right = _cell->get_action_icon(mforms::IconRight);
    if (right)
    {
      const mforms::IconVisibility icon_visibility = _cell->get_action_icon_visiblity(mforms::IconRight);
      right_icon_width = right->get_width();
      const int dest_x = bg_area.get_x() + bg_area.get_width() - right_icon_width;
      //const bool mouse_ptr_hit_icon_rect = is_in_rect(mouse_x, mouse_y, dest_x, bg_area.get_y(), right_icon_width, bg_area.get_height());
      if (icon_visibility == mforms::ShowAlways || (icon_visibility == mforms::ShowOnHover && mouse_ptr_hit_icon_rect))
        window->draw_pixbuf(right, 0, 0, dest_x, bg_area.get_y(), -1, bg_area.get_height(), Gdk::RGB_DITHER_NONE, 0, 0);
    }
  }

  Gdk::Rectangle cell_area_new(cell_area);
  if (left_icon_width > 0)
  {
    cell_area_new.set_x(cell_area_new.get_x() + left_icon_width);
    cell_area_new.set_width(cell_area_new.get_width() - left_icon_width);
  }

  if (right_icon_width > 0)
  {
    cell_area_new.set_width(cell_area_new.get_width() - right_icon_width);
  }

  if (type == mforms::CellBool)
  {
    bool b = false;
    if (_cell->get_value(&b))
    {
      GtkWidget *wdg = widget.gobj();
      gtk_paint_check(wdg->style,
                      window->gobj(),
                      GTK_STATE_NORMAL, b ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
                      cell_area_new.gobj(), wdg, "cellcheck",
                      cell_area_new.get_x(),
                      cell_area_new.get_y(),
                      13, 13);
      needs_text_render = false;
    }
  }

  if (editable && type != mforms::CellBool)
  {
      GtkWidget *wdg = widget.gobj();
      gtk_paint_shadow(wdg->style,
                      window->gobj(),
                      GTK_STATE_NORMAL, GTK_SHADOW_IN,
                      bg_area.gobj(), wdg, "box",
                      bg_area.get_x(),
                      bg_area.get_y(),
                      bg_area.get_width(),
                      bg_area.get_height());
  }

  if (needs_text_render)
    Gtk::CellRendererText::render_vfunc(window, widget, bg_area, cell_area_new, cell_area_new, flags);

  if (draw_header)
  {
    Gdk::Rectangle r = bg_area;
    r.set_x(20);
    r.set_width(_tv->get_width() - 20);
    property_scale() = 1.1;

    Gtk::CellRendererText::render_vfunc(window, widget, r, r, r, flags);
  }

  // paint shading
  do_shading(window, bg_area);
}

//------------------------------------------------------------------------------
void GridCellRenderer::cell_data(Gtk::CellRenderer* cr, const Gtk::TreeModel::iterator& it)
{
  property_text() = "";
  property_editable() = false;
  _cell = 0;
  _iter = it;

  if (it)
  {
    _cell = cell_from(it);
    if (_cell)
    {
      const mforms::CellType type = _cell->type();
      if (type != mforms::CellInvalid)
      {
        std::string s;
        if (_cell->get_value(&s))
        {
          property_markup() = s;
          if (type == mforms::CellText || type == mforms::CellEnum)
            property_editable() = _cell->is_editable();
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
bool GridCellRenderer::hits_click_area(const int x, const int y, const Gdk::Rectangle& area, const mforms::CellType type)
{
  bool ret = false;
  switch (type)
  {
    case mforms::CellBool:
    {
      const int xa = area.get_x();
      const int ya = area.get_y();
      ret = (x >= xa && y >= ya && x <= (xa + 13) && y <= (ya + 13));
      break;
    }
    case mforms::CellEnum:
    case mforms::CellText:
    {
      int xo = 0, yo = 0, w = 0, h = 0;
      get_size(*_tv, area, xo, yo, w, h);
      xo += area.get_x();
      yo += area.get_y();
      ret = (x >= xo && y >= yo && x <= (xo + w) && y <= (yo + h));
      break;
    }
    case mforms::CellInvalid:
    case mforms::CellHeader:
    case mforms::CellGroupHeader:
      break;
  }

  return ret;
}

//------------------------------------------------------------------------------
void GridCellRenderer::activate(const Gtk::TreePath& path)
{
  GridCell *cell = cell_from(path);
  if (cell && cell->type() == mforms::CellBool)
  {
    bool b = false;
    if (cell->get_value(&b))
    {
      cell->set(!b);
      _store->row_changed(path, _store->get_iter(path));
      _gv->content_edited(cast_path(path), _colid);
    }
  }
}

bool adjust_celleditable_size(GdkEvent* e, Gtk::Widget* widget, const int new_x, const int new_width)
{
  Glib::RefPtr<Gdk::Window> window = widget->get_window();
  if (window && e->type == GDK_EXPOSE)
  {
    int x = 0, y = 0, w = 0, h = 0, d = 0;
    window->get_geometry(x, y, w, h, d);
    window->move_resize(new_x, y, new_width, h);
  }

  return false;
}

//------------------------------------------------------------------------------
Gtk::CellEditable* GridCellRenderer::start_editing_vfunc(GdkEvent* event,
                                                               Gtk::Widget& widget,
                                                               const Glib::ustring& path,
                                                               const Gdk::Rectangle& bg_area,
                                                               const Gdk::Rectangle& cell_area,
                                                               Gtk::CellRendererState flags)
{
  Gtk::CellEditable*  ce(0);
  GridCell*     cell = cell_from(path);
  const bool can_edit = cell && !cell->has_shade(mforms::ShadeFilter);
  Gdk::Rectangle  cell_area_new(cell_area);
  int new_x  = cell_area.get_x();
  int new_width = cell_area.get_width();

  if (can_edit)
  {
    if (cell)
    {
      const Glib::RefPtr<Gdk::Pixbuf> left  = cell->get_action_icon(mforms::IconLeft);
      const Glib::RefPtr<Gdk::Pixbuf> right = cell->get_action_icon(mforms::IconLeft);
      const int left_icon_width  = left ? left->get_width() : 0;
      const int right_icon_width = right ? right->get_width() : 0;

      if (left_icon_width > 0)
      {
        new_x += left_icon_width;
        new_width -= left_icon_width;
      }

      if (right_icon_width > 0)
        new_width -= right_icon_width;

      _editable.set_type(cell->type());
      switch (cell->type())
      {
        case mforms::CellEnum:
        {
          _editable.set_enum_def(cell->get_enum_def());
          std::string s;
          cell->get_value(&s);

          _editable.set_text(s);
          _editable.set_path(path);

          ce = &_editable;

          _editable.show_all();
          break;
        }
        default:
          break;
      }
    }
    else
      _editable.set_type(mforms::CellText);
  }

  if (can_edit && !ce)
  {
    ce = Gtk::CellRendererText::start_editing_vfunc(event, widget, path, cell_area_new, cell_area_new, flags);
    Gtk::Widget* wid = dynamic_cast<Gtk::Widget*>(ce);
    if (wid)
      wid->signal_event().connect(sigc::bind(sigc::ptr_fun(adjust_celleditable_size), wid, new_x, new_width));
  }

  return ce;
}







//------------------------------------------------------------------------------
GridView::GridView(mforms::Grid* self)
         : mforms::gtk::ViewImpl(self)
         , _mgrid(self)
         , _init_done(false)
{
  _tree.add_events(Gdk::POINTER_MOTION_MASK);
  _tree.signal_motion_notify_event().connect_notify(sigc::mem_fun(this, &GridView::track_pointer));
  _tree.set_level_indentation(0);
  _scroll.add(_tree);
  _scroll.show_all();

  _tree.set_headers_visible(true);
  _tree.signal_event().connect(sigc::mem_fun(this, &GridView::on_event_slot), false);
  _tree.signal_row_activated().connect(sigc::mem_fun(this, &GridView::row_activated_slot));
  _tree.set_level_indentation(10);
}

//------------------------------------------------------------------------------
void GridView::_do_init()
{
  _model = GridModel::create(&_tree, "");
  _tree.set_model(_model);
  _init_done = true;
}

//------------------------------------------------------------------------------
int GridView::add_column(const std::string& name)
{
  if (!_init_done)
    _do_init();

  _model->add_column();
  const int colid = _model->columns_count() - 1;

  GridCellRenderer*    crt = Gtk::manage(new GridCellRenderer(colid, &_tree, this));
  Gtk::TreeViewColumn*     tvcol = Gtk::manage(new Gtk::TreeViewColumn(name, *crt));

  tvcol->set_cell_data_func(*crt, sigc::mem_fun(crt, &GridCellRenderer::cell_data));
  tvcol->set_resizable(true);

  _tree.append_column(*tvcol);
  return colid;
}

//------------------------------------------------------------------------------
void GridView::clear()
{
  // clear the model
  if (_model)
    _model->clear();
}

//------------------------------------------------------------------------------
void GridView::row_activated_slot(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* col)
{
  GridCellRenderer* cr = dynamic_cast<GridCellRenderer*>(col->get_first_cell_renderer());
  if (cr)
  {
    GridCell* cell = cr->cell_from(path);
    if (cell)
    {
      if (cell->type() == mforms::CellHeader)
      {
        if (_tree.row_expanded(path))
          _tree.collapse_row(path);
        else
          _tree.expand_row(path, false);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool GridView::on_event_slot(GdkEvent* e)
{
  if (!(e->type == GDK_BUTTON_RELEASE || e->type == GDK_BUTTON_PRESS))
    return false;

  bool ret = false;
  GdkEventButton* ev = (GdkEventButton*)e;

  if (ev->button == 1)
  {
    Gtk::TreePath path;
    Gtk::TreeViewColumn *col(0);
    int cell_x = 0;
    int cell_y = 0;
    if (_tree.get_path_at_pos(ev->x, ev->y, path, col, cell_x, cell_y))
    {
      if (col)
      {
        Gdk::Rectangle cell_rect;
        _tree.get_cell_area(path, *col, cell_rect);
        Gdk::Rectangle bg_rect;
        _tree.get_background_area(path, *col, bg_rect);
        GridCellRenderer* cr = dynamic_cast<GridCellRenderer*>(col->get_first_cell_renderer());
        if (cr)
        {
          GridCell* cell = cr->cell_from(path);
          if (cell && !cell->has_shade(mforms::ShadeFilter))
          {
            const Glib::RefPtr<Gdk::Pixbuf> left_icon   = cell->get_action_icon(mforms::IconLeft);
            const Glib::RefPtr<Gdk::Pixbuf> right_icon  = cell->get_action_icon(mforms::IconRight);

            const int left_icon_width  = left_icon ? left_icon->get_width() : 0;
            const int right_icon_width = right_icon ? right_icon->get_width() : 0;

            if (left_icon && (ev->x < (bg_rect.get_x() + left_icon_width)))
            {
              if (e->type == GDK_BUTTON_RELEASE)
                (*_mgrid->signal_content_action())(cast_path(path), cr->column(), mforms::IconLeft);
              _tree.set_cursor(path);
              ret = true;
            }
            else if (right_icon && (ev->x > (bg_rect.get_x() + bg_rect.get_width() - right_icon_width)))
            {
              if (e->type == GDK_BUTTON_RELEASE)
                (*_mgrid->signal_content_action())(cast_path(path), cr->column(), mforms::IconRight);
              _tree.set_cursor(path);
              ret = true;
            }
            else if (ev->type == GDK_BUTTON_RELEASE)
            {
              const bool hits = cr->hits_click_area(ev->x - left_icon_width, ev->y, cell_rect, cell->type());
              switch (cell->type())
              {
                case mforms::CellBool:
                {
                  if (hits)
                  {
                    if (cell->is_editable())
                      cr->activate(path);
                    else
                      (*_mgrid->signal_ro_content_clicked())(cast_path(path), cr->column());
                  }
                  break;
                }
                case mforms::CellEnum:
                case mforms::CellText:
                {
                  if (hits && !cell->is_editable())
                    (*_mgrid->signal_ro_content_clicked())(cast_path(path), cr->column());
                  break;
                }
                case mforms::CellInvalid:
                case mforms::CellHeader:
                case mforms::CellGroupHeader:
                  break;
              } // switch (cell->type())
            } // if (left_icon && (ev->x < (bg_rect.get_x() + left_icon_width)))
          } // if (cell && !cell->has_shade(mforms::ShadeFilter))
        } // if (cr)
      } // if (col)
    } // if (_tree.get_path_at_pos(ev->x, ev->y, path, col, cell_x, cell_y))
  } // if (ev->button == 1)
  else if (ev->button == 3)
  {
    Gtk::TreePath path;
    Gtk::TreeViewColumn *col(0);
    int cell_x = 0;
    int cell_y = 0;
    int column = -1;
    if (_tree.get_path_at_pos(ev->x, ev->y, path, col, cell_x, cell_y))
    {
      GridCellRenderer* cr = dynamic_cast<GridCellRenderer*>(col->get_first_cell_renderer());
      if (cr)
        column = cr->column();
    }
    else
      path.clear();

    (*_mgrid->signal_context_menu())(cast_path(path), column);
  } // if (ev->button == 3)

  return ret;
}


//------------------------------------------------------------------------------
int GridView::get_children_count(const Path& path)
{
  return _model->get_children_count(path);
}

//------------------------------------------------------------------------------
bool GridView::is_node_expanded(const Path& path)
{
  Gtk::TreePath gtk_path = cast_path(path);
  return _tree.row_expanded(gtk_path);
}

//------------------------------------------------------------------------------
void GridView::set_node_expanded(const Path& path, const bool expanded)
{
  Gtk::TreePath gtk_path = cast_path(path);
  if (expanded)
    _tree.expand_row(gtk_path, false);
  else
    _tree.collapse_row(gtk_path);
}

//------------------------------------------------------------------------------
void GridView::set_column_width(const int column, const int width)
{
  Gtk::TreeViewColumn* col = _tree.get_column(column);
  if (col)
    col->set_min_width(width);
}

//------------------------------------------------------------------------------
void GridView::set_cell_attr(const mforms::GridPath& path, const int col_id, const int attr)
{
  _model->set_cell_attr(path, col_id, attr);
}

//------------------------------------------------------------------------------
GridView::Path GridView::append_row(const std::string& group_name)
{
  if (!_init_done)
    _do_init();

  return _model->append_row(group_name);
}

//------------------------------------------------------------------------------
GridView::Path GridView::append_row(const Path& path)
{
  if (!_init_done)
    _do_init();

  return _model->append_row(path);
}

//------------------------------------------------------------------------------
GridView::Path GridView::insert_row(const Path& path)
{
  if (!_init_done)
    _do_init();

  return _model->insert_row(path);
}

//------------------------------------------------------------------------------
void GridView::remove_row(const Path& path)
{
  _model->remove_row(path);
}

//------------------------------------------------------------------------------
bool GridView::set_value(const GridView::Path& rid, const int col_id, const GridView::Value& cv, const bool editable)
{
  Gtk::TreeIter iter;
  const Gtk::TreePath path = cast_path(rid);
  GridCell *cell = _model->cell(path, col_id, &iter);
  if (cell)
  {
    cell->set_editable(editable);
    cell->set(cv);
    _model->row_changed(path, iter);
  }
  return cell;
}

//------------------------------------------------------------------------------
bool GridView::set_value(const GridView::Path& rid, const int col_id, const char* cv, const bool editable)
{
  return set_value(rid, col_id, std::string(cv), editable);
}

//------------------------------------------------------------------------------
bool GridView::set_value(const GridView::Path& rid, const int col_id, bool cv, const bool editable)
{
  Gtk::TreeIter iter;
  const Gtk::TreePath path = cast_path(rid);
  GridCell *cell = _model->cell(path, col_id, &iter);
  if (cell)
  {
    cell->set_editable(editable);
    cell->set(cv);
    _model->row_changed(path, iter);
  }
  return cell;
}

//------------------------------------------------------------------------------
std::string GridView::get_value(const Path& rid, const int col_id)
{
  std::string s;
  const GridCell *cell = _model->cell(rid, col_id);

  if (cell)
    s = cell->as_string();

  return s;
}

//------------------------------------------------------------------------------
std::string GridView::get_value(const Path& rid, const int col_id, mforms::CellType* type)
{
  std::string s;
  const GridCell *cell = _model->cell(rid, col_id);

  if (cell)
  {
    s = cell->as_string();
    if (type)
      *type = cell->type();
  }

  return s;
}

//------------------------------------------------------------------------------
void GridView::set_cell_type(const Path& path, const int col_id, const mforms::CellType type)
{
  GridCell *cell;
  if (col_id >= 0)
  {
    cell = _model->cell(path, col_id);
    if (cell)
      cell->set_type(type);
  }
  else
  {
    const int size = _model->get_n_columns();
    for (int i = 0; i < size; ++i)
    {
      cell = _model->cell(path, i);
      if (cell)
        cell->set_type(type);
    }
  }
}

//------------------------------------------------------------------------------
bool GridView::set_fg(const Path& rid, const int col_id, const double r, const double g, const double b)
{
  GridCell *cell = _model->cell(rid, col_id);
  if (cell)
    cell->set_fg(r, g, b);
  return cell;
}

//------------------------------------------------------------------------------
bool GridView::set_bg(const Path& rid, const int col_id, const double r, const double g, const double b)
{
  GridCell *cell = _model->cell(rid, col_id);
  if (cell)
    cell->set_bg(r, g, b);
  return cell;
}

//------------------------------------------------------------------------------
bool GridView::set_enum_def(const Path& rid, const int col_id, std::vector<std::string>* list)
{
  GridCell *cell = _model->cell(rid, col_id);
  if (cell)
    cell->set_enum_def(list);
  return cell;
}

//------------------------------------------------------------------------------
bool GridView::set_enum_def(const Path& rid, const int col_id, const char** const list)
{
  if (list)
  {
    std::auto_ptr<GridCell::EnumDef> def(new GridCell::EnumDef());

    for (const char** l = list; *l != 0; ++l)
      def->push_back(*l);

    if (def->size() > 0)
      set_enum_def(rid, col_id, def.release());
  }

  return list != 0;
}

//------------------------------------------------------------------------------
void GridView::shade(const Path& rid, const mforms::Shade shade, const int col_id)
{
  GridCell *cell;
  if (col_id >= 0)
  {
    cell = _model->cell(rid, col_id);
    if (cell)
      cell->set_shade(shade);
  }
  else
  {
    const int size = _model->get_n_columns();
    for (int i = 0; i < size; ++i)
    {
      cell = _model->cell(rid, i);
      if (cell)
        cell->set_shade(shade);
    }
  }
}

//------------------------------------------------------------------------------
void GridView::unshade(const Path& rid, const mforms::Shade shade, const int col_id)
{
  GridCell *cell;
  if (col_id >= 0)
  {
    cell = _model->cell(rid, col_id);
    if (cell)
      cell->set_shade(shade);
  }
  else
  {
    const int size = _model->get_n_columns();
    for (int i = 0; i < size; ++i)
    {
      cell = _model->cell(rid, i);
      if (cell)
        cell->unset_shade(shade);
    }
  }
}

//------------------------------------------------------------------------------
bool GridView::has_shade(const Path& rid, const int col_id, const mforms::Shade s)
{
  bool ret = false;
  GridCell *cell;
  if (col_id >= 0)
  {
    cell = _model->cell(rid, col_id);
    if (cell)
      ret = cell->has_shade(s);
  }
  return ret;
}

//------------------------------------------------------------------------------
void GridView::scroll_to_row(const Path& rid)
{
  _tree.scroll_to_row(cast_path(rid));
}

//------------------------------------------------------------------------------
void GridView::set_row_tag(const mforms::GridPath& path, const std::string& tag)
{
  GridModel::Cells* row = _model->row_from_path(path);
  if (row)
    row->set_tag(tag);
}

//------------------------------------------------------------------------------
std::string GridView::get_row_tag(const mforms::GridPath& path)
{
  GridModel::Cells* row = _model->row_from_path(path);
  return row ? row->get_tag() : std::string();
}

//------------------------------------------------------------------------------
void GridView::set_row_caption(const mforms::GridPath& path, const std::string& tag)
{
  GridModel::Cells* row = _model->row_from_path(path);
  if (row)
    row->set_caption(tag);
}

//------------------------------------------------------------------------------
std::string GridView::get_row_caption(const mforms::GridPath& path)
{
  GridModel::Cells* row = _model->row_from_path(path);
  return row ? row->get_caption() : std::string();
}

//------------------------------------------------------------------------------
void GridView::set_action_icon(const mforms::GridPath&   rid
                              ,const int                       col
                              ,const std::string&              iconpath
                              ,const mforms::IconVisibility    visible
                              ,const mforms::IconPos           pos)
{
  GridCell *cell = 0;
  if (col >= 0)
  {
    cell = _model->cell(rid, col);
    if (cell)
      cell->set_action_icon(iconpath, visible, pos);
  }
}

//------------------------------------------------------------------------------
void GridView::popup_context_menu()
{
  mforms::Menu* menu = _mgrid->get_context_menu();
  menu->popup();
}

}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static bool create(mforms::Grid* self)
{
  GridView* gv = new GridView(self);
  return gv;
}

//------------------------------------------------------------------------------
static int add_column(mforms::Grid* self, const std::string& name)
{
  GridView* cb = self->get_data<GridView>();
  return cb->add_column(name);
}

//------------------------------------------------------------------------------
static void clear(mforms::Grid* self)
{
  GridView* cb = self->get_data<GridView>();
  cb->clear();
}

//------------------------------------------------------------------------------
static int get_children_count(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  return cb->get_children_count(path);
}

//------------------------------------------------------------------------------
static bool is_node_expanded(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  return cb->is_node_expanded(path);
}

//------------------------------------------------------------------------------
static void set_node_expanded(mforms::Grid* self, const mforms::GridPath& path, const bool expanded)
{
  GridView* cb = self->get_data<GridView>();
  cb->set_node_expanded(path, expanded);
}

//------------------------------------------------------------------------------
static void set_column_width(mforms::Grid* self, const int column, const int width)
{
  GridView* cb = self->get_data<GridView>();
  cb->set_column_width(column, width);
}

//------------------------------------------------------------------------------
static mforms::GridPath append_header(mforms::Grid* self, const std::string& gid)
{
  GridView* cb = self->get_data<GridView>();
  return cb->append_row(gid);
}

//------------------------------------------------------------------------------
static mforms::GridPath append_row(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  return cb->append_row(path);
}

//------------------------------------------------------------------------------
static mforms::GridPath insert_row(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  return cb->insert_row(path);
}

//------------------------------------------------------------------------------
static void remove_row(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  cb->remove_row(path);
}

//------------------------------------------------------------------------------
static bool set_str_value(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, const std::string& cv, const bool editable)
{
  GridView* cb = self->get_data<GridView>();
  return cb->set_value(rid, col_id, cv, editable);
}

//------------------------------------------------------------------------------
static bool set_bool_value(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, bool cv, const bool editable)
{
  GridView* cb = self->get_data<GridView>();
  return cb->set_value(rid, col_id, cv, editable);
}

//------------------------------------------------------------------------------
static std::string get_value(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, mforms::CellType* type)
{
  GridView* cb = self->get_data<GridView>();
  return cb->get_value(rid, col_id, type);
}

//------------------------------------------------------------------------------
static void set_cell_type(mforms::Grid* self, const mforms::GridPath& path, const int col_id, const mforms::CellType type)
{
  GridView* cb = self->get_data<GridView>();
  cb->set_cell_type(path, col_id, type);
}

//------------------------------------------------------------------------------
static void set_cell_attr(mforms::Grid* self, const mforms::GridPath& path, const int col_id, const int attr)
{
  GridView* cb = self->get_data<GridView>();
  cb->set_cell_attr(path, col_id, attr);
}

//------------------------------------------------------------------------------
static bool set_fg(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, const double r, const double g, const double b)
{
  GridView* cb = self->get_data<GridView>();
  return cb->set_fg(rid, col_id, r, g, b);
}

//------------------------------------------------------------------------------
static bool set_bg(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, const double r, const double g, const double b)
{
  GridView* cb = self->get_data<GridView>();
  return cb->set_bg(rid, col_id, r, g, b);
}

//------------------------------------------------------------------------------
static bool set_enum_def(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, std::vector<std::string>* list)
{
  GridView* cb = self->get_data<GridView>();
  return cb->set_enum_def(rid, col_id, list);
}

//------------------------------------------------------------------------------
static bool set_enum_def_c(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, const char** const list)
{
  GridView* cb = self->get_data<GridView>();
  return cb->set_enum_def(rid, col_id, list);
}

//------------------------------------------------------------------------------
static void shade(mforms::Grid* self, const mforms::GridPath& rid, const mforms::Shade shade, const int col_id)
{
  GridView* cb = self->get_data<GridView>();
  return cb->shade(rid, shade, col_id);
}

//------------------------------------------------------------------------------
static void unshade(mforms::Grid* self, const mforms::GridPath& rid, const mforms::Shade shade, const int col_id)
{
  GridView* cb = self->get_data<GridView>();
  return cb->unshade(rid, shade, col_id);
}

//------------------------------------------------------------------------------
static bool has_shade(mforms::Grid* self, const mforms::GridPath& rid, const int col_id, const mforms::Shade s)
{
  GridView* cb = self->get_data<GridView>();
  return cb->has_shade(rid, col_id, s);
}

//------------------------------------------------------------------------------
static void scroll_to_row(mforms::Grid* self, const mforms::GridPath& rid)
{
  GridView* cb = self->get_data<GridView>();
  return cb->scroll_to_row(rid);
}

//------------------------------------------------------------------------------
static void set_row_tag(mforms::Grid* self, const mforms::GridPath& path, const std::string& tag)
{
  GridView* cb = self->get_data<GridView>();
  cb->set_row_tag(path, tag);
}

//------------------------------------------------------------------------------
static std::string get_row_tag(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  return cb->get_row_tag(path);
}

//------------------------------------------------------------------------------
static void set_row_caption(mforms::Grid* self, const mforms::GridPath& path, const std::string& caption)
{
  GridView* cb = self->get_data<GridView>();
  cb->set_row_caption(path, caption);
}

//------------------------------------------------------------------------------
static std::string get_row_caption(mforms::Grid* self, const mforms::GridPath& path)
{
  GridView* cb = self->get_data<GridView>();
  return cb->get_row_caption(path);
}

//------------------------------------------------------------------------------
static void set_action_icon(mforms::Grid* self, const mforms::GridPath& rid, const int col, const std::string& iconpath, const mforms::IconVisibility visible, const mforms::IconPos pos)
{
  GridView* gv = self->get_data<GridView>();
  gv->set_action_icon(rid, col, iconpath, visible, pos);
}

//------------------------------------------------------------------------------
static void popup_context_menu(mforms::Grid* self)
{
  GridView* gv = self->get_data<GridView>();
  gv->popup_context_menu();
}

//------------------------------------------------------------------------------
void Grid_init()
{
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();
  f->_grid_impl.create = &create;
  f->_grid_impl.add_column = &add_column;
  f->_grid_impl.clear = &clear;
  f->_grid_impl.get_children_count = &get_children_count;
  f->_grid_impl.is_node_expanded = &is_node_expanded;
  f->_grid_impl.set_node_expanded = &set_node_expanded;
  f->_grid_impl.set_column_width = &set_column_width;
  f->_grid_impl.append_header = append_header;
  f->_grid_impl.append_row = append_row;
  f->_grid_impl.insert_row = insert_row;
  f->_grid_impl.remove_row = remove_row;
  f->_grid_impl.set_str_value = set_str_value;
  f->_grid_impl.set_bool_value = set_bool_value;
  f->_grid_impl.get_value = get_value;
  f->_grid_impl.set_cell_type = set_cell_type;
  f->_grid_impl.set_cell_attr = set_cell_attr;
  f->_grid_impl.set_fg = set_fg;
  f->_grid_impl.set_bg = set_bg;
  f->_grid_impl.set_enum_def = set_enum_def;
  f->_grid_impl.set_enum_def_c = set_enum_def_c;
  f->_grid_impl.shade = shade;
  f->_grid_impl.unshade = unshade;
  f->_grid_impl.has_shade = has_shade;
  f->_grid_impl.scroll_to_row = &scroll_to_row;
  f->_grid_impl.set_row_tag = set_row_tag;
  f->_grid_impl.get_row_tag = get_row_tag;
  f->_grid_impl.set_row_caption = set_row_caption;
  f->_grid_impl.get_row_caption = get_row_caption;
  f->_grid_impl.set_action_icon = set_action_icon;
  f->_grid_impl.popup_context_menu = popup_context_menu;
}
