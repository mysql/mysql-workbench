/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _LF_VIEW_H_
#define _LF_VIEW_H_

#include "mforms/mforms.h"
#include <gdkmm/event.h>

#include "lf_base.h"
#include "lf_mforms.h"
#include "main_app.h"

namespace mforms {
  namespace gtk {

    class DataWrapper {
    private:
      void *_data;

    public:
      DataWrapper(void *data) {
        _data = data;
      }

      void *GetData() {
        return _data;
      };
    };

    mforms::ModifierKey GetModifiers(const guint state, const guint keyval);
    mforms::KeyCode GetKeys(const guint keyval);

    class ViewImpl : public ObjectImpl {
    public:
      virtual Gtk::Widget *get_outer() const = 0;
      // get the widget that does the actual work. most of the time it will be the same as the outer one
      virtual Gtk::Widget *get_inner() const;

    protected:
      ViewImpl(::mforms::View *view);
      static void destroy(::mforms::View *self);
      static void show(::mforms::View *self, bool show);
      virtual void show(bool show);
      static bool is_shown(::mforms::View *self);
      static bool is_fully_visible(::mforms::View *self);
      static void set_tooltip(::mforms::View *self, const std::string &text);
      static void set_font(::mforms::View *self, const std::string &fontDescription);
      static int get_width(const ::mforms::View *self);
      static int get_height(const ::mforms::View *self);
      static int get_preferred_width(::mforms::View *self);
      virtual int get_preferred_width();
      static int get_preferred_height(::mforms::View *self);
      virtual int get_preferred_height();
      static int get_x(const ::mforms::View *self);
      static int get_y(const ::mforms::View *self);
      static void set_size(::mforms::View *self, int w, int h);
      virtual void set_size(int width, int height);
      static void set_min_size(::mforms::View *self, int w, int h);
      virtual void set_min_size(int width, int height);
      static void set_position(::mforms::View *self, int x, int y);
      static std::pair<int, int> client_to_screen(::mforms::View *self, int x, int y);
      static void set_enabled(::mforms::View *self, bool flag);
      static bool is_enabled(::mforms::View *self);
      static void set_name(::mforms::View *self, const std::string &name);
      virtual void set_name(const std::string &name);
      static void relayout(::mforms::View *view);
      static void set_needs_repaint(::mforms::View *view);
      void size_changed();
      void on_focus_grab();
      bool on_button_release(GdkEventButton *btn);
      bool on_button_press(GdkEventButton *btn);

      void setup();
      virtual void move_child(ViewImpl *child, int x, int y);
      static void suspend_layout(::mforms::View *view, bool flag);
      virtual void suspend_layout(bool flag) {
      }
      static void set_front_color(::mforms::View *self, const std::string &color);
      virtual void set_front_color(const std::string &color){};
      static void set_back_color(::mforms::View *self, const std::string &color);
      static std::string get_front_color(::mforms::View *self);
      static std::string get_back_color(::mforms::View *self);
      virtual void set_back_color(const std::string &color);
      static void set_back_image(::mforms::View *self, const std::string &path, mforms::Alignment alig);
      static void flush_events(::mforms::View *self);
      static void set_padding(::mforms::View *self, int left, int top, int right, int bottom);
      virtual void set_padding_impl(int left, int top, int right, int bottom);
      static void register_drop_formats(::mforms::View *self, DropDelegate *target,
                                        const std::vector<std::string> &formats);
      void register_drop_formats(const std::vector<std::string> &formats, DropDelegate *target);
      mforms::DropPosition get_drop_position();
      static void focus(::mforms::View *view);
      static mforms::DropPosition get_drop_position(::mforms::View *self);
      static bool has_focus(::mforms::View *view);
      static DragOperation drag_text(::mforms::View *self, ::mforms::DragDetails details, const std::string &text);
      static DragOperation drag_data(::mforms::View *self, ::mforms::DragDetails details, void *data,
                                     const std::string &format);
      DragOperation drag_data(::mforms::DragDetails details, void *data, const std::string &format);

    protected:
      Glib::RefPtr<Gdk::Pixbuf> _back_image;
      mforms::Alignment _back_image_alignment;
      Gdk::Event *_last_btn_down;

      // need this to find out later the format
      std::map<std::string, size_t> _drop_formats;
      DropDelegate *_target;

      std::map<std::string, DataWrapper> _drop_data;

      // can be null
      cairo_surface_t *_drag_image;
      runtime::loop _loop;

      //
      //  /**
      //   * holds a void ptr to the data being dragged and std::string mime type
      //   */
      //  std::map<std::string, void*> _drag_data;

      // This will only work if the specific subclass supports drawing backgroud images
      // in that case it will add on_expose_event to the expose signal
      virtual void set_back_image(const std::string &path, mforms::Alignment alig);

      // for supporting subclasses that support background painting
      bool on_draw_event(const ::Cairo::RefPtr< ::Cairo::Context> &context, Gtk::Widget *target);

      bool slot_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time);
      void slot_drag_data_delete(const Glib::RefPtr<Gdk::DragContext> &context);
      bool slot_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int, int, guint time);
      void slot_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                   const Gtk::SelectionData &data, guint info, guint time);
      void slot_drag_begin(const Glib::RefPtr<Gdk::DragContext> &context);
      void slot_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &data, guint,
                              guint time);
      void slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context);
      bool slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::DragResult result);

    public:
      static void init();
      static Gtk::Widget *get_widget_for_view(mforms::View *view);
      static mforms::View *get_view_for_widget(Gtk::Widget *w);
    };

    bool draw_event_slot(const ::Cairo::RefPtr< ::Cairo::Context> &context, Gtk::Widget *w);
    enum WBColor { BG_COLOR, FG_COLOR };

    void set_color(Gtk::Widget *, const std::string &color, const WBColor col);
    base::Color *get_color(Gtk::Widget *w, const WBColor colr);
  };

  inline Gtk::Widget *widget_for_view(mforms::View *view) {
    return gtk::ViewImpl::get_widget_for_view(view);
  }

  inline mforms::View *view_for_widget(Gtk::Widget *w) {
    return gtk::ViewImpl::get_view_for_widget(w);
  }
};

#endif
