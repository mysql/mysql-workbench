/* 
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _LF_VIEW_H_
#define _LF_VIEW_H_

#include <mforms/mforms.h>
#include <gdkmm/event.h>

#include "lf_base.h"
#include "lf_mforms.h"

namespace mforms { namespace gtk {

class DataWrapper
{
private:
  void *_data;
public:
  DataWrapper(void *data)
  {
    _data = data;
  }

  void *GetData() { return _data; };
};

class ViewImpl : public ObjectImpl
{
public:
  virtual Gtk::Widget *get_outer() const= 0;
  // get the widget that does the actual work. most of the time it will be the same as the outer one
  virtual Gtk::Widget *get_inner() const;

protected:
  ViewImpl(::mforms::View *view);
  static void destroy(::mforms::View *self);
  static void show(::mforms::View *self, bool show);
  static bool is_shown(::mforms::View *self);
  static bool is_fully_visible(::mforms::View *self);
  static void set_tooltip(::mforms::View *self, const std::string &text);
  static void set_font(::mforms::View *self, const std::string &fontDescription);
  static int get_width(::mforms::View *self);
  static int get_height(::mforms::View *self);
  static int get_preferred_width(::mforms::View *self);
  virtual int get_preferred_width();
  static int get_preferred_height(::mforms::View *self);
  virtual int get_preferred_height();
  static int get_x(::mforms::View *self);
  static int get_y(::mforms::View *self);
  static void set_size(::mforms::View *self, int w, int h);
  virtual void set_size(int width, int height);
  static void set_position(::mforms::View *self, int x, int y);
  static std::pair<int, int>client_to_screen(::mforms::View *self, int x, int y);
  static void set_enabled(::mforms::View *self, bool flag);
  static bool is_enabled(::mforms::View *self);
  static void set_name(::mforms::View *self, const std::string &name);
  virtual void set_name(const std::string &name);
  static void relayout(::mforms::View *view);
  static void set_needs_repaint(::mforms::View *view);
  void size_changed();
  void on_focus_grab();
  bool on_button_release(GdkEventButton* btn);
  bool on_button_press(GdkEventButton* btn);
  void setup();
  virtual void move_child(ViewImpl *child, int x, int y);
  static void suspend_layout(::mforms::View *view, bool flag);
  virtual void suspend_layout(bool flag) {}
  static void set_front_color(::mforms::View *self, const std::string &color);
  virtual void set_front_color(const std::string &color) {};
  static void set_back_color(::mforms::View *self, const std::string &color);
  static std::string get_front_color(::mforms::View *self);
  static std::string get_back_color(::mforms::View *self);
  virtual void set_back_color(const std::string &color);
  static mforms::Style *get_style(::mforms::View *self);
  virtual mforms::Style *get_style_impl();
  static void set_back_image(::mforms::View *self, const std::string &path, mforms::Alignment alig);
  static void flush_events(::mforms::View *self);
  static void set_padding(::mforms::View *self, int left, int top, int right, int bottom);
  virtual void set_padding_impl(int left, int top, int right, int bottom);
  static void set_allow_drag(::mforms::View* self, const bool flag);
  static void register_drop_formats(::mforms::View* self, DropDelegate *target, const std::vector<std::string> &formats);
  void register_drop_formats(const std::vector<std::string> &formats, DropDelegate *target);
  mforms::DropPosition get_drop_position();
  static void focus(::mforms::View *view);
  static mforms::DropPosition get_drop_position(::mforms::View *self);
  static bool has_focus(::mforms::View *view);
  static DragOperation drag_text(::mforms::View *self, ::mforms::DragDetails details, const std::string &text);
  static DragOperation drag_data(::mforms::View *self, ::mforms::DragDetails details, void *data,const std::string &format);
  DragOperation drag_data(::mforms::DragDetails details, void *data,const std::string &format);

protected:
  Glib::RefPtr<Gdk::Pixbuf> _back_image;
  mforms::Alignment _back_image_alignment;
  Gdk::Event* _last_btn_down;

  //need this to find out later the format
  std::map<std::string,size_t> _drop_formats;
  DropDelegate *_target;

  std::map<std::string,DataWrapper> _drop_data;

  mforms::Style *_style;
  //can be null
  cairo_surface_t *_drag_image;
//
//  /**
//   * holds a void ptr to the data being dragged and std::string mime type
//   */
//  std::map<std::string, void*> _drag_data;

  // This will only work if the specific subclass supports drawing backgroud images
  // in that case it will add on_expose_event to the expose signal
  virtual void set_back_image(const std::string &path, mforms::Alignment alig);

  // for supporting subclasses that support background painting
  bool on_expose_event(GdkEventExpose *event, Gtk::Widget *target);

  bool slot_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);
  void slot_drag_data_delete(const Glib::RefPtr<Gdk::DragContext> &context);
  bool slot_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int, int, guint time);
  void slot_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, const Gtk::SelectionData &data, guint info, guint time);
  void slot_drag_begin(const Glib::RefPtr<Gdk::DragContext> &context);
  void slot_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &data, guint, guint time);
  void slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context);
  bool slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context,Gtk::DragResult result);

public:
  static void init();
  static Gtk::Widget *get_widget_for_view(mforms::View *view);
  static mforms::View *get_view_for_widget(Gtk::Widget *w);
};

bool expose_event_slot(GdkEventExpose* event, Gtk::Widget* w);
void set_bgcolor(Gtk::Widget*, const std::string& color);

};

inline Gtk::Widget *widget_for_view(mforms::View *view)
{
  return gtk::ViewImpl::get_widget_for_view(view);
}


inline mforms::View *view_for_widget(Gtk::Widget *w)
{
  return gtk::ViewImpl::get_view_for_widget(w);
}

};

#endif
