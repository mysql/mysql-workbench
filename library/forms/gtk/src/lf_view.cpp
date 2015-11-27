/* 
 * Copyright (c) 2008, 2015, Oracle and/or its affiliates. All rights reserved.
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
#include <gdkmm/types.h>

#include "../lf_view.h"
#include "base/util_functions.h"
#include "base/log.h"
DEFAULT_LOG_DOMAIN("mforms.linux")

namespace mforms { namespace gtk {



  // get the widget that does the actual work. most of the time it will be the same as the outer one
Gtk::Widget *ViewImpl::get_inner() const
{
  return get_outer();
}

ViewImpl::ViewImpl(::mforms::View *view)
  : ObjectImpl(view), _back_image_alignment(mforms::NoAlign),  _last_btn_down(NULL), _target(NULL), _drag_image(NULL)
{
    _style = 0;
}


void ViewImpl::destroy(::mforms::View *self)
{
  // Nothing to do here. Freeing platform objects happens in lf_base.h, via data free function.
}

void ViewImpl::show(::mforms::View *self, bool show)
{
  ViewImpl *view = self->get_data<ViewImpl>();

  if ( view )
  {
    Gtk::Widget* widget = view->get_outer();
    if (show)
      widget->show();
    else
      widget->hide();
  }
}

bool ViewImpl::is_shown(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
    return view->get_outer()->is_visible();
  return false;
}

bool ViewImpl::is_fully_visible(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    //INFO: in gtk3 we can use gtk_widget_is_visible and this code can be removed.
    Gtk::Widget *w = view->get_outer();

    while (w->is_visible())
    {
      if (w->get_parent() == NULL)
        return true;

      Gtk::Notebook *n = dynamic_cast<Gtk::Notebook*>(w->get_parent());
      if (n)
        if (n->page_num(*w) != n->get_current_page()) //We need to check if we're on active tab, if not return false.
          return false;

      w = w->get_parent();
    };
  }
  return false;
}

void ViewImpl::set_tooltip(::mforms::View *self, const std::string &text)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
#if GTK_VERSION_GT(2,10)
    view->get_outer()->set_tooltip_text(text);
    view->get_outer()->set_has_tooltip(!text.empty());
#endif
  }
}

void ViewImpl::set_font(::mforms::View *self, const std::string &fontDescription)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    // apply font settings
  }
}
  
int ViewImpl::get_width(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
  {
    Gtk::Widget* widget = view->get_outer();
    //int w, h;
    //widget->get_size_request(w, h);
    return widget->get_allocation().get_width();
  }
  return 0;
}

int ViewImpl::get_height(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
  {
    Gtk::Widget* widget = view->get_outer();
    //int w, h;
    //widget->get_size_request(w, h);
    return widget->get_allocation().get_height();
  }
  return 0;
}

int ViewImpl::get_preferred_width(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
  {
    return view->get_preferred_width();
  }
  return 0;
}

int ViewImpl::get_preferred_width()
{
  int w, h;
  get_outer()->get_size_request(w, h);
  return w;
}

int ViewImpl::get_preferred_height(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
    return view->get_preferred_height();
  return 0;
}

int ViewImpl::get_preferred_height()
{
  int w, h;
  get_outer()->get_size_request(w, h);
  return h;
}

int ViewImpl::get_x(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
  {
    Gtk::Widget* widget = view->get_outer();
    return widget->get_allocation().get_x();
  }
  return 0;
}

int ViewImpl::get_y(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
  {
    Gtk::Widget* widget = view->get_outer();
    return widget->get_allocation().get_y();
  }
  return 0;
}

void ViewImpl::set_size(::mforms::View *self, int w, int h)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
    view->set_size(w, h);
}

void ViewImpl::set_size(int width, int height)
{
  get_outer()->set_size_request(width, height);
}

void ViewImpl::set_position(::mforms::View *self, int x, int y)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    mforms::View* parent = self->get_parent();
    if (parent)
    {
      ViewImpl* parent_view_impl = parent->get_data<ViewImpl>();
      if (parent_view_impl)
        parent_view_impl->move_child(view, x, y);
    }
  }
}

std::pair<int, int> ViewImpl::client_to_screen(::mforms::View *self, int x, int y)
{
  ViewImpl *view = self->get_data<ViewImpl>();

  if ( view )
  {
    Gtk::Widget* widget = view->get_outer();
    if (widget)
    {
      Glib::RefPtr<Gdk::Window> wnd = widget->get_window();
      if (wnd)
      {
        #if GTK_VERSION_LT(2,18)
         int originx = 0;
         int originy = 0;
         wnd->get_origin(originx, originy);
         point.x += orginx;
         point.y += originy;
        #else
         int newx = x;
         int newy = y;
         wnd->get_root_coords(x, y, newx, newy);
         x = newx;
         y = newy;
         return std::pair<int, int>(newx, newy);
        #endif
      }
    }
  }
  return std::pair<int, int>(0, 0);
}
  
void ViewImpl::set_enabled(::mforms::View *self, bool flag)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    Gtk::Widget* widget = view->get_outer();
    widget->set_sensitive(flag);
  }
}

bool ViewImpl::is_enabled(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    Gtk::Widget* widget = view->get_outer();
    return widget->get_sensitive();
  }
  return false;
}


void ViewImpl::set_name(::mforms::View *self, const std::string &name)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if ( view )
    view->set_name(name);
}

void ViewImpl::set_name(const std::string &name)
{

}

void ViewImpl::relayout(::mforms::View *view)
{
  // noop
}

void ViewImpl::set_needs_repaint(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    Gtk::Widget *widget = view->get_outer();
    if (widget)
    {
//      Glib::RefPtr<Gdk::Window> window = widget->get_window();
//      if (window)
//        window->process_updates(true);
//      else
      widget->queue_draw();
    }
  }
}

void ViewImpl::suspend_layout(::mforms::View *self, bool flag)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
    view->suspend_layout(flag);
}

void ViewImpl::size_changed()
{
  if (get_outer() && get_outer()->is_realized())
  {
    ::mforms::View* owner_view = dynamic_cast< ::mforms::View*>(owner);
    if (owner_view)
      (*owner_view->signal_resized())();
  }
}

void ViewImpl::on_focus_grab()
{
  if (get_outer() && get_outer()->is_realized())
  {
    ::mforms::View* owner_view = dynamic_cast< ::mforms::View*>(owner);
    if (owner_view)
      owner_view->focus_changed();
  }
}

bool ViewImpl::has_focus(mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
    return view->get_inner()->has_focus();
  return false;
}

bool ViewImpl::on_button_release(GdkEventButton* btn)
{
  if(_last_btn_down)
  {
    delete _last_btn_down;
    _last_btn_down = NULL;
  }
  return true;
}

bool ViewImpl::on_button_press(GdkEventButton* btn)
{

  if(_last_btn_down == NULL)
    _last_btn_down = new Gdk::Event((GdkEvent*)btn);
  return true;
}

void ViewImpl::setup()
{
  get_outer()->signal_size_allocate().connect(sigc::hide(sigc::mem_fun(this, &ViewImpl::size_changed)));
  get_outer()->signal_grab_focus().connect(sigc::mem_fun(this, &ViewImpl::on_focus_grab));
  get_outer()->signal_realize().connect(sigc::mem_fun(this, &ViewImpl::size_changed));
  get_outer()->show();
  get_outer()->signal_button_press_event().connect(sigc::mem_fun(this, &ViewImpl::on_button_press));
  get_outer()->signal_button_release_event().connect(sigc::mem_fun(this, &ViewImpl::on_button_release));
  get_outer()->signal_drag_data_get().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_data_get));
  get_outer()->signal_drag_end().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_end));
  get_outer()->signal_drag_failed().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_failed));
  get_outer()->signal_drag_begin().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_begin));
}


void ViewImpl::move_child(ViewImpl *child, int x, int y)
{
  throw std::logic_error("container does not implement required method");
}

void ViewImpl::set_front_color(::mforms::View *self, const std::string &color)
{
  ViewImpl *view= self->get_data<ViewImpl>();
  Gtk::Widget *w = view->get_inner();
  if (w)
  {
    if (color.empty())
      w->unset_fg(Gtk::STATE_NORMAL);
    else
    {
      Gdk::Color c(color.substr(1));
      w->get_colormap()->alloc_color(c);
      w->modify_fg(Gtk::STATE_NORMAL, c);
    }
  }
  view->set_front_color(color);
}

void ViewImpl::set_back_color(const std::string &color)
{
  Gtk::Widget *w = this->get_inner();
  if (w)
  {
    mforms::gtk::set_bgcolor(w, color);
    if (color.empty())
    {
      w->unset_bg(Gtk::STATE_NORMAL);
      w->unset_base(Gtk::STATE_NORMAL);
    }
    else
    {
      Gdk::Color gcolor(color);
      w->get_colormap()->alloc_color(gcolor);
      w->modify_bg(Gtk::STATE_NORMAL, gcolor);
      w->modify_base(Gtk::STATE_NORMAL, gcolor);
    }
  }
}

void ViewImpl::set_back_color(::mforms::View *self, const std::string &color)
{
  ViewImpl *view= self->get_data<ViewImpl>();
  if (view)
    view->set_back_color(color);
}

std::string ViewImpl::get_front_color(::mforms::View *self)
{
 // ViewImpl *view= self->get_data<ViewImpl>();
  //TODO
  return "";
}

std::string ViewImpl::get_back_color(::mforms::View *self)
{
  ViewImpl *view= self->get_data<ViewImpl>();
  base::Color *color = (base::Color*)g_object_get_data(G_OBJECT(view->get_inner()->gobj()), "bg");
  if (color)
  {
    return color->to_html();
  }
  return "";
}


bool ViewImpl::on_expose_event(GdkEventExpose *event, Gtk::Widget *target)
{
  int x, y;
  int iwidth, fwidth;
  int iheight, fheight;

  if (_back_image)
  {
    x = 0;
    y = 0;
    iwidth = _back_image->get_width();
    iheight = _back_image->get_height();
    fwidth = target->get_width();
    fheight = target->get_height();

    switch (_back_image_alignment)
    {
      case mforms::TopLeft:
        x = 0;
        y = 0;
        break;
      case mforms::TopCenter:
        x = (iwidth+fwidth) / 2;
        y = 0;
        break;
      case mforms::TopRight:
        x = fwidth - iwidth;
        y = 0;
        break;
      case mforms::MiddleLeft:
        x = 0;
        y = (iheight+fheight) / 2;
        break;
      case mforms::MiddleCenter:
        x = (iwidth+fwidth) / 2;
        y = (iheight+fheight) / 2;
        break;
      case mforms::MiddleRight:
        x = fwidth - iwidth;
        y = (iheight+fheight) / 2;
        break;
      case mforms::BottomLeft:
        x = 0;
        y = fheight - iheight;
        break;
      case mforms::BottomCenter:
        x = (iwidth+fwidth) / 2;
        y = fheight - iheight;
        break;
      case mforms::BottomRight:
        x = fwidth - iwidth;
        y = fheight - iheight;
        break;
      default:
        break;
    }

    _back_image->render_to_drawable(target->get_window(), target->get_style()->get_fg_gc(Gtk::STATE_NORMAL), 0, 0, x, y, iwidth, iheight, Gdk::RGB_DITHER_NONE, 0, 0);

    return true;
  }
  return false;
}

void ViewImpl::set_back_image(::mforms::View *self, const std::string &path, mforms::Alignment ali)
{
  ViewImpl *view= self->get_data<ViewImpl>();
  view->set_back_image(path, ali);
}

void ViewImpl::set_back_image(const std::string &path, mforms::Alignment alig)
{
  if (path.empty())
  {
    _back_image.reset();
    return;
  }

  std::string file = mforms::App::get()->get_resource_path(path);
  try
  {
    _back_image = Gdk::Pixbuf::create_from_file(file);
    _back_image_alignment = alig;
  }
  catch (Glib::Error &exc)
  {
    log_error("Could not load image for background: %s: %s\n", file.c_str(), exc.what().c_str());
  }
}

void ViewImpl::flush_events(::mforms::View *self)
{
  while (Gtk::Main::events_pending())
    Gtk::Main::iteration();
}

void ViewImpl::set_padding(::mforms::View *self, int left, int top, int right, int bottom)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  view->set_padding_impl(left, top, right, bottom);
}

void ViewImpl::set_padding_impl(int left, int top, int right, int bottom)
{}


mforms::Style *ViewImpl::get_style(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (!view->_style) {
      view->_style = new mforms::Style();

      Gtk::Widget *widget = view->get_inner();
      Glib::RefPtr<const Gtk::Style> gtk_style = widget->get_style();
      Gdk::Color bg_normal = gtk_style->get_background(Gtk::STATE_NORMAL);
      Gdk::Color bg_active = gtk_style->get_background(Gtk::STATE_SELECTED);
      //Gdk::Color bg_active = gtk_style->get_background(Gtk::STATE_ACTIVE);

      view->_style->bg[mforms::STATE_NORMAL] = base::Color(bg_normal.get_red_p(), bg_normal.get_green_p(), bg_normal.get_blue_p());
      view->_style->bg[mforms::STATE_ACTIVE] = base::Color(bg_active.get_red_p(), bg_active.get_green_p(), bg_active.get_blue_p());
  }

  return view->_style;
}

mforms::Style *ViewImpl::get_style_impl()
{
    return 0;
}


//------------------------------------------------------------------------------
void ViewImpl::set_allow_drag(::mforms::View* self, const bool flag)
{
    ViewImpl *view = self->get_data<ViewImpl>();
    if (view)
    {
      std::vector<Gtk::TargetEntry> targets;

//      targets.push_back(Gtk::TargetEntry("text/plain", Gtk::TargetFlags(0)));
//      targets.push_back(Gtk::TargetEntry("text/uri-list", Gtk::TargetFlags(0)));
//      targets.push_back(Gtk::TargetEntry("UTF8_STRING", Gtk::TargetFlags(0)));
//      targets.push_back(Gtk::TargetEntry("STRING", Gtk::TargetFlags(0)));
//      targets.push_back(Gtk::TargetEntry("TEXT", Gtk::TargetFlags(0)));

      Gtk::Widget* widget = view->get_outer();

      if (widget)
      {
        widget->drag_source_set(targets, Gdk::MODIFIER_MASK, Gdk::ACTION_COPY);
        //widget->signal_drag_begin().connect(sigc::mem_fun(view, &ViewImpl::drag_begin));
        widget->signal_drag_data_get().connect(sigc::mem_fun(view, &ViewImpl::slot_drag_data_get));
        widget->signal_drag_data_delete().connect(sigc::mem_fun(view, &ViewImpl::slot_drag_data_delete));
      }
    }
}

//------------------------------------------------------------------------------
void ViewImpl::register_drop_formats(::mforms::View* self, DropDelegate *target, const std::vector<std::string> &formats)
{
    ViewImpl *view = self->get_data<ViewImpl>();
    if (view)
    {
      view->register_drop_formats(formats, target);
    }
}

void ViewImpl::register_drop_formats(const std::vector<std::string> &formats, DropDelegate *target)
{
  _target = target;
  std::vector<Gtk::TargetEntry> targets;
  _drop_formats.clear();
  for(size_t i=0; i < formats.size(); ++i)
  {
    targets.push_back(Gtk::TargetEntry(formats[i],Gtk::TargetFlags(0), i));
    _drop_formats.insert(std::pair<std::string, size_t>(formats[i], i));
  }
  //We add two more formats, first is for allow files drop, second to allow paste text from other applications.
  targets.push_back(Gtk::TargetEntry("text/uri-list", Gtk::TargetFlags(0), formats.size()));
  _drop_formats.insert(std::pair<std::string, size_t>("text/uri-list", formats.size()));
  targets.push_back(Gtk::TargetEntry("STRING", Gtk::TargetFlags(0), formats.size()));
    _drop_formats.insert(std::pair<std::string, size_t>("STRING", formats.size()));

  Gtk::Widget* widget = this->get_outer();
  if (widget)
  {
    widget->drag_dest_set(targets, Gtk::DEST_DEFAULT_HIGHLIGHT, Gdk::ACTION_COPY|Gdk::ACTION_MOVE);

    Glib::RefPtr<Gtk::TargetList> target_list = Glib::RefPtr<Gtk::TargetList>(Gtk::TargetList::create(targets));
    widget->drag_dest_set_target_list(target_list); //need to explicitly specify target list, cause the one, set by drag_dest_set is not working :/

    widget->signal_drag_motion().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_motion));
    widget->signal_drag_drop().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_drop));
    widget->signal_drag_data_received().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_data_received));
  }
}

//------------------------------------------------------------------------------
void ViewImpl::slot_drag_begin(const Glib::RefPtr<Gdk::DragContext> &context)
{
  if (_drag_image)
  {
    // Convert pixel fromat from ARGB to ABGR.
    int i = 0;
    int width = cairo_image_surface_get_width(_drag_image);
    int height = cairo_image_surface_get_height(_drag_image);
    unsigned char *data = cairo_image_surface_get_data(_drag_image);

    while (i < 4 * width * height)
    {
      unsigned char temp = data[i];
      data[i] = data[i + 2];
      data[i + 2] = temp;
      i += 4;
    }

    Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create_from_data(data, Gdk::COLORSPACE_RGB, true, 8,
                                                                      width,
                                                                      height,
                                                                      cairo_image_surface_get_stride(_drag_image));
    context->set_icon(pixbuf, 0, 0);
  }

}

//------------------------------------------------------------------------------
// The drag_data_get signal is emitted on the drag source when the drop site requests the data which is dragged. 
void ViewImpl::slot_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &data, guint, guint time)
{
  const Glib::ustring target = data.get_target();
  std::map<std::string,DataWrapper>::iterator it = _drop_data.find(target);
  if (it != _drop_data.end()) //TODO need to store in _drop_data also data size for data to be sent
  {
    if (target == "STRING")
    {
      data.set(target, 8, reinterpret_cast<const guchar*>(((std::string*)(it->second.GetData()))->c_str()), ((std::string*)(it->second.GetData()))->size());
    }
    else
      data.set(target, 8, reinterpret_cast<const guint8*>(&it->second), sizeof(it->second));
  }
}

//The drag_end signal is emmited on the drag source when the drag was finished
void ViewImpl::slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context)
{
  _drop_data.clear();
  _drag_image = NULL;
  Gtk::Main::quit(); //call quit(), cause in do_drag_drop we called run()
}

//The drag_end signal is emmited on the drag source when the drag was failed due to user cancel, timeout, etc.
bool ViewImpl::slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context,Gtk::DragResult result)
{
  if (result != Gtk::DRAG_RESULT_NO_TARGET && result != Gtk::DRAG_RESULT_USER_CANCELLED)
    Gtk::Main::quit(); //call quit(), cause in do_drag_drop we called run()
  _drag_image = NULL;
  return false;
}
//------------------------------------------------------------------------------
// The drag_data_delete signal is emitted on the drag source when a drag with the action Gdk::ACTION_MOVE is successfully completed. 
void ViewImpl::slot_drag_data_delete(const Glib::RefPtr<Gdk::DragContext> &context)
{}

//------------------------------------------------------------------------------
// The drag_drop signal is emitted on the drop site when the user drops the data onto the widget. 
bool ViewImpl::slot_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time)
{
  mforms::View *view = dynamic_cast<mforms::View*>(owner);
  Gtk::Widget *widget = ViewImpl::get_widget_for_view(view);

  mforms::DropDelegate *drop_delegate = _target;
  if (drop_delegate == NULL)
    drop_delegate = reinterpret_cast<mforms::DropDelegate*>(owner);

  if (drop_delegate == NULL || view == NULL || widget == NULL)
    return false; // abandon drop cause we can't accept anything

  std::vector<std::string> targets(context->get_targets());
  if (targets.empty()) //is not valid drop drop site :(
    return false;

  //There should be only one element in targets, so pick it up
  std::string target = targets[0];
  if (targets.size() > 1)
  {//if not, we need to pick up the best one.
    std::vector<std::string>::iterator it = std::find(targets.begin(), targets.end(), "text/uri-list");
    if (it != targets.end())
      target = *it;
    else //Try STRING
      it = std::find(targets.begin(), targets.end(), "STRING");
      if (it != targets.end())
        target = *it;
  }

  widget->drag_get_data(context, target, time);


//  std::vector<Glib::ustring>::iterator iter;
//  if ((iter= std::find(targets.begin(), targets.end(), "text/uri-list")) != targets.end())
//  {
//    Glib::ustring type= *iter;
//    g_message("ask for %s", iter->c_str());
//
//    get_outer()->drag_get_data(context, "DataObject", time);
//
//    return true;
//  }

//  if ((iter= std::find(targets.begin(), targets.end(), "text/uri-list")) != targets.end())
//  {
//    Glib::ustring type= *iter;
//    g_message("ask for %s", iter->c_str());
//
//    get_outer()->drag_get_data(context, "DataObject", time);
//
//    return true;
//  }

  return true;
}
//------------------------------------------------------------------------------
// The drag_motion signal is emitted on the drop site when the user moves the cursor over the widget during a drag. 
bool ViewImpl::slot_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time)
{
  mforms::DropDelegate *drop_delegate = _target;
  if (drop_delegate == NULL)
    drop_delegate = dynamic_cast<mforms::DropDelegate*>(owner);

  bool ret_val = false;
  if (drop_delegate != NULL)
  {
    std::vector<std::string> targets = context->get_targets();
    //We need to fix a formats a little, so file will be recognized
    bool accept_string = false;

    for(std::vector<std::string>::iterator it = targets.begin(); it < targets.end(); it++)
    {
      if ((*it) == "text/uri-list")
      {
        targets.push_back(mforms::DragFormatFileName);
        break;
      }

      if ((*it) == "STRING")
      {
        accept_string = true;
        break;
      }
    }

    mforms::DragOperation operation, allowedOperations = mforms::DragOperationNone;

    if ((context->get_suggested_action() & Gdk::ACTION_COPY) == Gdk::ACTION_COPY)
      allowedOperations = allowedOperations | mforms::DragOperationCopy;
    if ((context->get_suggested_action() & Gdk::ACTION_MOVE) == Gdk::ACTION_MOVE)
        allowedOperations = allowedOperations | mforms::DragOperationMove;

    operation = drop_delegate->drag_over((mforms::View*)owner, base::Point(x, y), allowedOperations, targets);
    switch(operation)
    {

    case DragOperationCopy:
    case DragOperationMove:
      ret_val = true;
      break;
    case DragOperationNone:
    default:
      //STRING type is handled in different way, we support it by default
      ret_val = accept_string;
      break;
    }
  }

  if (ret_val)
  {
    context->drag_status(context->get_suggested_action(), time);

    get_outer()->drag_highlight();

    return true;
  }
  else
    context->drag_refuse(time);

  return false;
}

//------------------------------------------------------------------------------
// The drag_data_received signal is emitted on the drop site when the dragged data has been received. 
// called when drag is from outside of WB
void ViewImpl::slot_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, const Gtk::SelectionData &data, guint info, guint time)
{

  DataWrapper *dwrapper = (DataWrapper*)data.get_data();

  mforms::DropDelegate *drop_delegate = _target;
  if (drop_delegate == NULL)
    drop_delegate = dynamic_cast<mforms::DropDelegate*>(owner);

  if (drop_delegate == NULL || !dwrapper)
    return;

  std::vector<std::string> files;
  if (data.get_length() >= 0 && data.get_format() == 8)
     files = data.get_uris();

  mforms::DragOperation allowedOperations = mforms::DragOperationNone;

  if ((context->get_suggested_action() & Gdk::ACTION_COPY) == Gdk::ACTION_COPY)
    allowedOperations = allowedOperations | mforms::DragOperationCopy;
  if ((context->get_suggested_action() & Gdk::ACTION_MOVE) == Gdk::ACTION_MOVE)
    allowedOperations = allowedOperations | mforms::DragOperationMove;

  if (files.empty())
  {
    std::string tmpstr = std::vector<std::string>(context->get_targets())[0];

    drop_delegate->data_dropped((mforms::View*)owner, base::Point(x, y), allowedOperations, dwrapper->GetData(), tmpstr);
  }
  else
  {
    //We need to convert uris to file names
    for(std::vector<std::string>::iterator it = files.begin(); it < files.end(); ++it)
        (*it) = Glib::filename_from_uri((*it));

    drop_delegate->files_dropped((mforms::View*)owner, base::Point(x, y), allowedOperations, files);
  }

  context->drag_finish(true, false, time);
}

mforms::DragOperation ViewImpl::drag_text(::mforms::View *self, ::mforms::DragDetails details, const std::string &text)
{
  return mforms::DragOperationNone;
}

mforms::DragOperation ViewImpl::drag_data(::mforms::View *self, ::mforms::DragDetails details, void *data,const std::string &format)
{
  DragOperation dop = mforms::DragOperationNone;
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
  {
    return view->drag_data(details, data, format);
  }
  return dop;
}

DragOperation ViewImpl::drag_data(::mforms::DragDetails details, void *data,const std::string &format)
{
  DragOperation drag_op = mforms::DragOperationNone;
  Gtk::Widget* widget = get_outer();
  if (widget)
  {
     Gdk::DragAction actions = Gdk::ACTION_DEFAULT;

   if ((details.allowedOperations & mforms::DragOperationCopy) != 0)
     actions |= Gdk::ACTION_COPY;
   if ((details.allowedOperations & mforms::DragOperationMove) != 0)
     actions |= Gdk::ACTION_MOVE;

   std::map<std::string,size_t>::iterator it = _drop_formats.find(format);
   if (it == _drop_formats.end())
   {
     std::pair<std::map<std::string,size_t>::iterator, bool> insertptr = _drop_formats.insert(std::make_pair(format,_drop_formats.size()));
     if (!insertptr.second)
     {
       return drag_op;
     }

     it = insertptr.first;
   }

   _drop_data.clear();
   DataWrapper dwrapper(data);
   _drop_data.insert(std::pair<std::string,DataWrapper>(format,dwrapper));

   std::vector<Gtk::TargetEntry> targets;
   targets.push_back(Gtk::TargetEntry(it->first.c_str(), Gtk::TargetFlags(0), it->second));

   const Glib::RefPtr<Gtk::TargetList> tlist = Gtk::TargetList::create(targets);
   _drag_image = details.image;
   if (_last_btn_down)
     Glib::RefPtr<Gdk::DragContext> context = widget->drag_begin(tlist, actions, 1, _last_btn_down->gobj());
   else
     Glib::RefPtr<Gdk::DragContext> context = widget->drag_begin(tlist, actions, 1, NULL);
   Gtk::Main::run();


   drag_op = details.allowedOperations;

  }
  return drag_op;
}

bool expose_event_slot(GdkEventExpose* event, Gtk::Widget* widget)
{
  GdkWindow* wnd = event->window;
  base::Color *color = (base::Color*)g_object_get_data(G_OBJECT(widget->gobj()), "bg");

  //log_debug3("expose event, obj %p, color %p", obj, color);

  if (color)
  {
    cairo_t *cr = gdk_cairo_create(wnd);

    cairo_set_source_rgb(cr, color->red, color->green, color->blue);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    gdk_cairo_region(cr, event->region);

    cairo_fill(cr);

    cairo_destroy(cr);
  }
  return false;
}

static void destroy_color(base::Color *col)
{
  if (col)
    delete col;
}

void set_bgcolor(Gtk::Widget* w, const std::string& strcolor)
{
  if (!strcolor.empty())
  {
    base::Color *col = new base::Color(strcolor);
    if (col->is_valid())
      g_object_set_data_full(G_OBJECT(w->gobj()), "bg", (void*)col, (GDestroyNotify)destroy_color);
  }
}


void ViewImpl::focus(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view && view->get_inner())
    view->get_inner()->grab_focus();
}

mforms::DropPosition ViewImpl::get_drop_position()
{
  return mforms::DropPositionUnknown;
}

mforms::DropPosition ViewImpl::get_drop_position(::mforms::View *self)
{
  ViewImpl *view = self->get_data<ViewImpl>();
  if (view)
    return view->get_drop_position();

  return mforms::DropPositionUnknown;
}


void ViewImpl::init()
{
  ::mforms::ControlFactory *f= ::mforms::ControlFactory::get_instance();

  f->_view_impl.destroy               = &ViewImpl::destroy;
  f->_view_impl.get_width              = &ViewImpl::get_width;
  f->_view_impl.get_height            = &ViewImpl::get_height;
  f->_view_impl.get_preferred_width   = &ViewImpl::get_preferred_width;
  f->_view_impl.get_preferred_height  = &ViewImpl::get_preferred_height;
  f->_view_impl.set_size              = &ViewImpl::set_size;

  f->_view_impl.get_x                 = &ViewImpl::get_x;
  f->_view_impl.get_y                 = &ViewImpl::get_y;
  f->_view_impl.set_position          = &ViewImpl::set_position;
  f->_view_impl.client_to_screen      = &ViewImpl::client_to_screen;

  f->_view_impl.show                  = &ViewImpl::show;
  f->_view_impl.is_shown              = &ViewImpl::is_shown;
  f->_view_impl.is_fully_visible      = &ViewImpl::is_fully_visible;

  f->_view_impl.set_tooltip           = &ViewImpl::set_tooltip;
  f->_view_impl.set_font              = &ViewImpl::set_font;
  f->_view_impl.set_name              = &ViewImpl::set_name;

  f->_view_impl.set_enabled           = &ViewImpl::set_enabled;
  f->_view_impl.is_enabled            = &ViewImpl::is_enabled;

  f->_view_impl.suspend_layout        = &ViewImpl::suspend_layout;
  f->_view_impl.relayout              = &ViewImpl::relayout;
  f->_view_impl.set_needs_repaint     = &ViewImpl::set_needs_repaint;
  f->_view_impl.set_front_color       = &ViewImpl::set_front_color;
  f->_view_impl.get_front_color       = &ViewImpl::get_front_color;
  f->_view_impl.set_back_color        = &ViewImpl::set_back_color;
  f->_view_impl.get_back_color        = &ViewImpl::get_back_color;
  f->_view_impl.set_back_image        = &ViewImpl::set_back_image;
  f->_view_impl.flush_events          = &ViewImpl::flush_events;
  f->_view_impl.set_padding           = &ViewImpl::set_padding;
  f->_view_impl.get_style             = &ViewImpl::get_style;
  f->_view_impl.drag_data             = &ViewImpl::drag_data;
  f->_view_impl.drag_text             = &ViewImpl::drag_text;

//  f->_view_impl.set_allow_drag       = &ViewImpl::set_allow_drag;
  f->_view_impl.register_drop_formats = &ViewImpl::register_drop_formats;
  f->_view_impl.focus                 = &ViewImpl::focus;
  f->_view_impl.has_focus             = &ViewImpl::has_focus;
  f->_view_impl.get_drop_position     = &ViewImpl::get_drop_position;
};


Gtk::Widget* ViewImpl::get_widget_for_view(mforms::View *view)
{
  ViewImpl *vi = view->get_data<ViewImpl>();

  if (vi)
  {
    Gtk::Widget *w = vi->get_outer();
    w->set_data("mforms::View", view);
    return w;
  }
  return NULL;
}


mforms::View *ViewImpl::get_view_for_widget(Gtk::Widget *w)
{
  mforms::View *view = (mforms::View*)w->get_data("mforms::View");
  if (view)
    return view;
  return NULL;
}


};
};
