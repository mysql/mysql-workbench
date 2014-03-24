#include "sqlide/grid_view_model.h"
#include "sqlide/grid_view.h"
#include "linux_utilities/gtk_helpers.h"
#include "custom_renderers.h"
#include "base/string_utilities.h"

GridViewModel::Ref GridViewModel::create(bec::GridModel::Ref model, GridView *view, const std::string &name)
{
  return Ref(new GridViewModel(model, view, name));
}

GridViewModel::GridViewModel(bec::GridModel::Ref model, GridView *view, const std::string &name)
:
Glib::ObjectBase(typeid(GridViewModel)),
ListModelWrapper(model.get(), view, name),
_model(model),
_view(view),
_row_numbers_visible(true)
{
  view->set_rules_hint(true); // enable alternating row colors
  set_fake_column_value_getter(sigc::mem_fun(this, &GridViewModel::get_cell_value));
  //set_fake_column_value_setter(sigc::mem_fun(this, &GridViewModel::set_cell_value));
}

GridViewModel::~GridViewModel()
{
}

template <size_t ValueTypeCode= bec::GridModel::StringType>
struct ValueTypeTraits
{
  typedef Glib::ustring ValueType;
  typedef Gtk::CellRendererText Renderer;
  typedef Glib::ustring RendererValueType;
};

template<>
struct ValueTypeTraits<bec::GridModel::NumericType>
{
  typedef int ValueType;
  typedef Gtk::CellRendererSpin Renderer;
  typedef Glib::ustring RendererValueType;
};

template<>
struct ValueTypeTraits<bec::GridModel::FloatType>
{
  typedef double ValueType;
  typedef Gtk::CellRendererText Renderer;
  typedef Glib::ustring RendererValueType;
};

int GridViewModel::refresh(bool reset_columns)
{
  model_changed(bec::NodeId(), -1);

  if (reset_columns)
  {
    ColumnsModel &columns= model();
    columns.reset();
    _col_index_map.clear();

    // aux columns
    Gtk::TreeModelColumn<Gdk::Color> *color_column;
    {
      color_column= new Gtk::TreeModelColumn<Gdk::Color>;
      columns.add_model_column(color_column, -1);

      if (_row_numbers_visible)
      {
        Gtk::TreeViewColumn *col= add_column<ValueTypeTraits<> >(-2, "#", RO, NULL);
        col->get_first_cell_renderer()->property_cell_background()= "LightGray";
        col->set_min_width(30);
      }
    }

    bool is_model_editable= !_model->is_readonly();
    for (int index= 0, count= _model->get_column_count(); index < count; ++index)
    {
      Editable is_col_editable= (is_model_editable && (bec::GridModel::BlobType != _model->get_column_type(index))) ? EDITABLE : RO;
      std::string label= bec::replace_string(base::sanitize_utf8(_model->get_column_caption(index)), "_", "__");
      bec::GridModel::ColumnType type= _model->get_column_type(index);
      Gtk::TreeViewColumn *col;
      switch (type)
      {
      case bec::GridModel::NumericType:
        col = add_column<ValueTypeTraits<bec::GridModel::NumericType> >(index, label, is_col_editable, 0);
        col->set_min_width(30);
        break;
      case bec::GridModel::FloatType:
        col = add_column<ValueTypeTraits<bec::GridModel::FloatType> >(index, label, is_col_editable, 0);
        col->set_min_width(30);
        break;
      default:
        col = add_column<ValueTypeTraits<bec::GridModel::StringType> >(index, label, is_col_editable, 0);
        col->set_min_width(80);
        break;
      }
    }
  }

  return 0;
}

template <typename ValueTypeTraits>
Gtk::TreeViewColumn * GridViewModel::add_column(int index, const std::string &name, Editable editable, Gtk::TreeModelColumnBase *color_column)
{
  ColumnsModel &columns= model();

  typedef Gtk::TreeModelColumn<typename ValueTypeTraits::ValueType> ModelColumn;
  ModelColumn *col = new ModelColumn();
  columns.add_model_column(col, index);
  Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > *icon = new Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> >();
  columns.add_model_column(icon, index);

  typedef CustomRenderer<typename ValueTypeTraits::Renderer, typename ValueTypeTraits::RendererValueType, typename ValueTypeTraits::ValueType> CustomRenderer;
  CustomRenderer *renderer= Gtk::manage(new CustomRenderer());
  renderer->floating_point_visible_scale(_model->floating_point_visible_scale());
  renderer->set_edit_state= sigc::bind(sigc::mem_fun(_model.get(), &::bec::GridModel::set_edited_field), index);
  Gtk::TreeViewColumn *treeview_column= renderer->bind_columns(_view, name, index, col, icon);
  if (index >= 0 || index == -2)
  {
    treeview_column->signal_clicked().connect(sigc::bind(sigc::mem_fun(_view, &GridView::on_column_header_clicked), treeview_column, index));
    treeview_column->set_clickable();
  }
  if (color_column)
    treeview_column->add_attribute(renderer->property_cell_background_gdk(), *color_column);
  _col_index_map[treeview_column]= index;
// XXX This causes the column auto-sizing to not work well. Enable it back once the auto-sizing is fixed.
//  set_ellipsize(index, true);
  //TODO: implement editable wothout first row
  if (editable == EDITABLE || editable == EDITABLE_WO_FIRST)
  {
    renderer->property_editable()= true;
    renderer->signal_edited().connect(sigc::bind(sigc::mem_fun(*this, &GridViewModel::after_cell_edit<typename ValueTypeTraits::ValueType>), sigc::ref(*col)));
    renderer->signal_edited().connect(sigc::mem_fun(_view, &GridView::on_cell_edited));
    renderer->signal_editing_started().connect(sigc::bind(sigc::mem_fun(_view, &GridView::on_cell_editing_started), treeview_column));
    renderer->signal_editing_canceled().connect(sigc::mem_fun(_view, &GridView::on_cell_editing_done));
  }

  return treeview_column;
}

void GridViewModel::set_ellipsize(const int column, const bool on)
{
  Gtk::TreeViewColumn *col = 0;
  for(std::map<Gtk::TreeViewColumn*, int>::const_iterator end = _col_index_map.end(), it = _col_index_map.begin();
      it != end;
      ++it)
  {
    if (it->second == column)
    {
      col = it->first;
      break;
    }
  }

  if (col)
  {
    const std::vector<Gtk::CellRenderer*> rends = col->get_cell_renderers();
    const int rends_size = rends.size();
    for (int i = 0; i < rends_size; ++i)
    {
      CustomRendererOps* cr = dynamic_cast<CustomRendererOps*>(rends[i]);
      if (cr)
      {
        Gtk::CellRendererText* crt = dynamic_cast<Gtk::CellRendererText*>(cr->data_renderer());
        crt->property_ellipsize() = on ? Pango::ELLIPSIZE_END : Pango::ELLIPSIZE_NONE;
        crt->property_ellipsize_set() = on;
      }
    }
  }
}


int GridViewModel::column_index(Gtk::TreeViewColumn* col)
{
  std::map<Gtk::TreeViewColumn*, int>::const_iterator i= _col_index_map.find(col);
  return (_col_index_map.end() == i) ? -1 : i->second;
}

void GridViewModel::get_cell_value(const iterator& iter, int column, GType type, Glib::ValueBase& value)
{
  bec::NodeId node= node_for_iter(iter);
  if (!node.is_valid())
    return;

  switch (column)
  {
  case -1:
  {
//    static const Gdk::Color odd_row_color("white");
//    static const Gdk::Color even_row_color("snow2");
//    const Gdk::Color *color= (node[0]%2) ? &even_row_color : &odd_row_color;

    g_value_init(value.gobj(), GDK_TYPE_COLOR);
    g_value_set_boxed(value.gobj(), 0);
  }
    break;
  case -2:
  {
    if (type == GDK_TYPE_PIXBUF)
      g_value_init(value.gobj(), GDK_TYPE_PIXBUF);
    else
    {
      std::ostringstream oss;
      int row= node[0]+1;
      if (_model->is_readonly() || (row < _model->count()))
        oss << row;
      else
        oss << "*";
      set_glib_string(value, oss.str().c_str());
    }
  }
    break;
  }
}

void GridViewModel::set_cell_value(const iterator& itier, int column, GType type, const Glib::ValueBase& value)
{
}

bool GridViewModel::handle_popup_event(GdkEvent* event)
{
  return false;
}

void GridViewModel::get_value_vfunc(const iterator& iter, int column, Glib::ValueBase& value) const
{
  ListModelWrapper::get_value_vfunc(iter, column, value);
  before_render(column, &value);
}
