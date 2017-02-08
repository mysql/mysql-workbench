/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/popover.h"
#include "../lf_mforms.h"
#include "../lf_view.h"

#include <gtkmm/window.h>
#include <gtkmm/fixed.h>
#include <gdkmm/devicemanager.h>
#include "gtk_helpers.h"

namespace {
  //==============================================================================
  //
  //==============================================================================

  class WBFixed : public Gtk::Fixed {
  public:
    void set_has_window(bool has_window = true) {
      Gtk::Fixed::set_has_window(has_window);
    }
  };

  class PopoverWidget : public Gtk::Window {
  public:
    PopoverWidget(Gtk::Window* parent, mforms::PopoverStyle style);

    void show_popover(const int x, const int y, const mforms::StartPosition pos);
    void set_size(const int w, const int h);
    void set_content(Gtk::Widget* w, bool move_only = false);

  protected:
    //    virtual bool on_expose_event(GdkEventExpose* event);
    virtual bool on_draw(const ::Cairo::RefPtr< ::Cairo::Context>& cr);
    virtual bool on_configure_event(GdkEventConfigure* ce);

  private:
    void parent_key_release(GdkEventKey* ev);
    bool tooltip_signal_event(GdkEvent* e);
    bool parent_configure_event(GdkEvent* e);
    void create_shape_path(cairo_t* cr, const int offset);
    void recalc_shape_mask();
    void adjust_position();
    void adjust_child_position();
    mforms::PopoverStyle _style;
    int _old_parent_x; // last saved x coord of parent window
    int _old_parent_y; // last saved y coord of parent window
    mforms::StartPosition _content_pos;
    int _handle_x;
    int _handle_y;

    WBFixed _fixed;
    Gtk::Box* _hbox;
    int _old_w;
    int _old_h;
    bool _ignore_configure;

    const int R;   // Radius of round corner
    const int hh;  // Length of tear(handle) triangle side
    const int hh1; // Height of tear(handle) triangle
  };
  //------------------------------------------------------------------------------
  PopoverWidget::PopoverWidget(Gtk::Window* parent, mforms::PopoverStyle style)
    : Gtk::Window(style == mforms::PopoverStyleTooltip ? Gtk::WINDOW_POPUP : Gtk::WINDOW_TOPLEVEL),
      _style(style),
      _old_parent_x(-1),
      _old_parent_y(-1),
      _content_pos(mforms::StartLeft),
      _handle_x(-1),
      _handle_y(-1),
      _old_w(-1),
      _old_h(-1),
      _ignore_configure(false),
      R(30),
      hh(30),
      hh1(0.883025 * hh) {
    if (_style == mforms::PopoverStyleTooltip) {
      set_type_hint(Gdk::WINDOW_TYPE_HINT_TOOLTIP);
      set_app_paintable(true);
      set_resizable(false);
      set_name("gtk-tooltip");
      set_border_width(2);

      _hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
      add(*_hbox);
      signal_event().connect(sigc::mem_fun(this, &PopoverWidget::tooltip_signal_event));
      parent->add_events(Gdk::KEY_RELEASE_MASK);
      parent->signal_key_release_event().connect_notify(sigc::mem_fun(this, &PopoverWidget::parent_key_release));

      show();
    } else {
      set_decorated(false);
      set_transient_for(*parent);
      parent->signal_event().connect(sigc::mem_fun(this, &PopoverWidget::parent_configure_event));
      set_skip_pager_hint(true);
      set_skip_taskbar_hint(true);

      _fixed.set_has_window(false);
      add(_fixed);
      _fixed.show_all();
    }
  }

  void PopoverWidget::parent_key_release(GdkEventKey* ev) {
    hide();
  }
  //------------------------------------------------------------------------------
  bool PopoverWidget::tooltip_signal_event(GdkEvent* ev) {
    if (ev->type == GDK_LEAVE_NOTIFY)
      hide();

    return false;
  }
  //------------------------------------------------------------------------------
  bool PopoverWidget::parent_configure_event(GdkEvent* ev) {
    if (ev->type != GDK_CONFIGURE)
      return false;

    GdkEventConfigure* e = (GdkEventConfigure*)ev;

    if (_old_parent_y >= 0 && _old_parent_x >= 0) {
      const int diff_x = e->x - _old_parent_x;
      const int diff_y = e->y - _old_parent_y;
      int this_x = 0;
      int this_y = 0;
      get_position(this_x, this_y);
      move(this_x + diff_x, this_y + diff_y);
      _old_parent_x = e->x;
      _old_parent_y = e->y;
    } else {
      _old_parent_x = e->x;
      _old_parent_y = e->y;
    }

    return false;
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::create_shape_path(cairo_t* cr, const int offset) {
    int x = offset;
    int y = offset;
    int W = get_width() - (offset ? (1 + offset) : 0);
    int H = get_height() - (offset ? (1 + offset) : 0);

    // printf("create_shape_path x=%i, y=%i, W=%i, H=%i\n", x, y, W, H);

    switch (_content_pos) {
      case mforms::StartLeft: {
        W -= hh1;
        break;
      }
      case mforms::StartRight: {
        x += hh1;
        W -= hh1;
        break;
      }
      case mforms::StartAbove: {
        H -= hh1;
        break;
      }
      case mforms::StartBelow: {
        y += hh1;
        H -= hh1;
        break;
      }
    }

    cairo_new_path(cr);
    cairo_move_to(cr, x + R, y); // 1
    if (_content_pos == mforms::StartBelow) {
      cairo_line_to(cr, x + (W / 4) - hh1 / 2, y); // 2
      cairo_line_to(cr, x + (W / 4), y - hh1);     // 2
      cairo_line_to(cr, x + (W / 4) + hh1 / 2, y); // 2
      cairo_line_to(cr, x + (W - R), y);           // 2
    } else
      cairo_line_to(cr, x + (W - R), y);                  // 2
    cairo_curve_to(cr, x + W, y, x + W, y, x + W, y + R); // 3

    if (_content_pos == mforms::StartLeft) {
      cairo_line_to(cr, x + W, y + H / 4 - hh1 / 2); // 4
      cairo_line_to(cr, x + W + hh1, y + H / 4);     // 4
      cairo_line_to(cr, x + W, y + H / 4 + hh1 / 2); // 4
      cairo_line_to(cr, x + W, y + (H - R));         // 4
    } else
      cairo_line_to(cr, x + W, y + (H - R));                            // 4
    cairo_curve_to(cr, x + W, y + H, x + W, y + H, x + (W - R), y + H); // 5
    if (_content_pos == mforms::StartAbove) {
      cairo_line_to(cr, x + W / 4 + hh1 / 2, y + H); // 6
      cairo_line_to(cr, x + W / 4, y + H + hh1);     // 6
      cairo_line_to(cr, x + W / 4 - hh1 / 2, y + H); // 6
      cairo_line_to(cr, x + R, y + H);               // 6
    } else
      cairo_line_to(cr, x + R, y + H);                      // 6
    cairo_curve_to(cr, x, y + H, x, y + H, x, y + (H - R)); // 7
    if (_content_pos == mforms::StartRight) {
      cairo_line_to(cr, x, y + H / 4 + hh1 / 2); // 8
      cairo_line_to(cr, x - hh1, y + H / 4);     // 8
      cairo_line_to(cr, x, y + H / 4 - hh1 / 2); // 8
      cairo_line_to(cr, x, y + R);               // 8
    } else
      cairo_line_to(cr, x, y + R);            // 8
    cairo_curve_to(cr, x, y, x, y, x + R, y); // 9
    cairo_close_path(cr);
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::set_size(const int w, const int h) {
    if (w > 1 && h > 1) {
      if (_style == mforms::PopoverStyleNormal) {
        if (w != _old_w || h != _old_h)
          adjust_child_position();
      }
      resize(w, h);
    }
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::set_content(Gtk::Widget* w, bool move_only) {
    if (_style == mforms::PopoverStyleTooltip) {
      _hbox->pack_start(*w, false, false, 0);
    } else {
      int x = 0;
      int y = 0;
      int W = get_width();
      int H = get_height();

      switch (_content_pos) {
        case mforms::StartLeft: {
          W -= hh1;
          break;
        }
        case mforms::StartRight: {
          x += hh1;
          W -= hh1;
          break;
        }
        case mforms::StartAbove: {
          H -= hh1;
          break;
        }
        case mforms::StartBelow: {
          y += hh1;
          H -= hh1;
          break;
        }
      }

      // clear existing content
      if (move_only)
        _fixed.move(*w, x + 10, y + 10);
      else
        _fixed.put(*w, x + 10, y + 10);
      _fixed.show_all();
    }
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::recalc_shape_mask() {
    const int w = get_width();
    const int h = get_height();

    if (this->get_window()) {
      cairo_surface_t* mask =
        cairo_image_surface_create(CAIRO_FORMAT_A1, this->get_window()->get_width(), this->get_window()->get_height());
      cairo_t* cr = cairo_create(mask);
      if (cr) {
        cairo_save(cr);
        cairo_rectangle(cr, 0, 0, w, h);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
        cairo_set_line_width(cr, 0.2);
        create_shape_path(cr, 0);
        cairo_fill_preserve(cr);
      }
      cairo_region_t* mask_region = gdk_cairo_region_create_from_surface(mask);

      gtk_widget_shape_combine_region(GTK_WIDGET(gobj()), mask_region);
    }
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::adjust_position() {
    int w = get_width();
    int h = get_height();

    int x = _handle_x;
    int y = _handle_y;

    if (_style == mforms::PopoverStyleTooltip) {
      Gdk::Rectangle rect;
      get_screen()->get_monitor_geometry(get_screen()->get_monitor_at_point(x, y), rect);

      if ((x + w) > rect.get_width()) {
        _content_pos = mforms::StartLeft;
      }

      if ((y + h) > rect.get_height()) {
        _content_pos = mforms::StartAbove;
      }
    }

    switch (_content_pos) {
      case mforms::StartAbove: {
        x -= 3 * w / 4;
        if (_style == mforms::PopoverStyleTooltip)
          y -= (h + 5);
        else
          y -= h;
        break;
      }
      case mforms::StartBelow: {
        x -= w / 4;
        break;
      }
      case mforms::StartRight: {
        if (_style == mforms::PopoverStyleTooltip) {
          y = _handle_y + 10;
          x += 10;
        } else
          y -= 3 * h / 4;
        break;
      }
      case mforms::StartLeft: {
        x -= w;
        if (_style == mforms::PopoverStyleTooltip)
          y = _handle_y + 10;
        else
          y -= h / 4;
        break;
      }
    }
    move(x, y);
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::adjust_child_position() {
    std::vector<Gtk::Widget*> children = _fixed.get_children();
    for (std::size_t i = 0; i < children.size(); ++i) {
      set_content(children[i], true);
    }
  }

  //------------------------------------------------------------------------------
  void PopoverWidget::show_popover(const int rx, const int ry, const mforms::StartPosition pos) {
    if (_style == mforms::PopoverStyleTooltip) {
      Glib::RefPtr<Gdk::Window> wnd = this->get_window();
      if (wnd) {
        int xx;
        int yy;
        Gdk::ModifierType mask;
        wnd->get_pointer(xx, yy, mask);
        if ((mask & Gdk::BUTTON1_MASK) || (mask & Gdk::BUTTON2_MASK) || (mask & Gdk::BUTTON3_MASK))
          return;
      }
    }

    int x = rx, y = ry;
    if (x < 0 && y < 0) {
      Glib::RefPtr<Gdk::Display> dsp = Gdk::Display::get_default();
      if (dsp) {
        Glib::RefPtr<Gdk::DeviceManager> dvm = dsp->get_device_manager();
        if (dvm) {
          Glib::RefPtr<Gdk::Device> dev = dvm->get_client_pointer();
          if (dev)
            dev->get_position(x, y);
        }
      }
    }

    _content_pos = pos;
    _handle_x = x;
    _handle_y = y;

    adjust_position();
    if (_style == mforms::PopoverStyleTooltip)
      show_all();
    else {
      recalc_shape_mask();
      adjust_child_position();
      show_all();
    }
  }

  ////------------------------------------------------------------------------------
  // bool PopoverWidget::on_expose_event(GdkEventExpose* event)
  //{
  //  if (_style == mforms::PopoverStyleTooltip)
  //  {
  //    //Because gtkmm didn't allow me to pass widget param as null we're forced to use gtk function.
  //    gtk_paint_flat_box(const_cast<GtkStyle*>(this->get_style()->gobj()),
  //                   Glib::unwrap(this->get_window()), ((GtkStateType)(Gtk::STATE_NORMAL)),
  //                   ((GtkShadowType)(Gtk::SHADOW_OUT)), NULL,
  //                             GTK_WIDGET(gobj()), "tooltip", 0, 0, this->get_allocation().get_width(),
  //                             this->get_allocation().get_height());
  //    //We need it here, because without it we will not be able to move the window.
  //    adjust_position();
  //    return Gtk::Window::on_expose_event(event);
  //  }
  //
  //  Gtk::Window::on_expose_event(event);
  //
  //  if (_handle_x >= 0 && _handle_y >= 0)
  //  {
  //    Cairo::RefPtr<Cairo::Context> context(get_window()->create_cairo_context());
  //    cairo_t *cr = context->cobj();
  //
  //    if (cr)
  //    {
  //      cairo_save(cr);
  //      create_shape_path(cr, 1);
  //      cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
  //      cairo_set_line_width(cr, 0.5);
  //      cairo_stroke(cr);
  //      cairo_restore(cr);
  //    }
  //  }
  //
  //  return false;
  //}

  bool PopoverWidget::on_draw(const ::Cairo::RefPtr< ::Cairo::Context>& cr) {
    if (_style == mforms::PopoverStyleTooltip) {
      // Because gtkmm didn't allow me to pass widget param as null we're forced to use gtk function.

      this->get_style_context()->render_frame(get_window()->create_cairo_context(), 0, 0,
                                              this->get_allocation().get_width(), this->get_allocation().get_height());

      //    gtk_paint_flat_box(const_cast<GtkStyle*>(this->get_style()->gobj()),
      //                   Glib::unwrap(this->get_window()),
      //                   ((GtkStateType)(Gtk::STATE_NORMAL)),
      //                   ((GtkShadowType)(Gtk::SHADOW_OUT)), NULL,
      //                             GTK_WIDGET(gobj()), "tooltip", 0, 0, this->get_allocation().get_width(),
      //                             this->get_allocation().get_height());
      // We need it here, because without it we will not be able to move the window.
      adjust_position();
      return Gtk::Window::on_draw(cr);
    }

    Gtk::Window::on_draw(cr);

    if (_handle_x >= 0 && _handle_y >= 0) {
      Cairo::RefPtr<Cairo::Context> context(get_window()->create_cairo_context());
      cairo_t* cr = context->cobj();

      if (cr) {
        cairo_save(cr);
        create_shape_path(cr, 1);
        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_set_line_width(cr, 0.5);
        cairo_stroke(cr);
        cairo_restore(cr);
      }
    }

    return false;
  }

  //------------------------------------------------------------------------------
  bool PopoverWidget::on_configure_event(GdkEventConfigure* ce)

  {
    if (_style == mforms::PopoverStyleTooltip)
      return false;

    if (_ignore_configure) {
      _ignore_configure = false;
      return false;
    }

    if (_handle_x >= 0 && _handle_y >= 0) {
      if (_old_w != ce->width || _old_h != ce->height) {
        _old_w = ce->width;
        _old_h = ce->height;
        recalc_shape_mask(); // calc only on resize

        std::vector<Gtk::Widget*> widgets = _fixed.get_children();
        if (widgets.size() > 0) {
          Gtk::Widget* w = widgets[0];
          int W = ce->width;
          int H = ce->height;

          switch (_content_pos) {
            case mforms::StartLeft:
            case mforms::StartRight: {
              W -= hh1;
              break;
            }
            case mforms::StartAbove:
            case mforms::StartBelow: {
              H -= hh1;
              break;
            }
          }

          w->set_size_request(W - 20, H - 20);
          adjust_position();
          _ignore_configure = true;
        }
      }
    }

    return false;
  }
} // namespace

//------------------------------------------------------------------------------
static void delete_PopoverWidget(void* o) {
  delete (PopoverWidget*)o;
}

//------------------------------------------------------------------------------
static bool create(mforms::Popover* self, mforms::PopoverStyle style) {
  if (get_mainwindow() == nullptr)
    throw std::runtime_error("Need main window to continue.");

  PopoverWidget* w = new PopoverWidget(get_mainwindow(), style);
  self->set_data(w, delete_PopoverWidget);
  return w;
}

//------------------------------------------------------------------------------
static void set_content(mforms::Popover* self, mforms::View* content) {
  PopoverWidget* w = self->get_data<PopoverWidget>();
  mforms::gtk::ViewImpl* view = content->get_data<mforms::gtk::ViewImpl>();
  w->set_content(view->get_outer());
}

//------------------------------------------------------------------------------
static void set_size(mforms::Popover* self, int w, int h) {
  PopoverWidget* widget = self->get_data<PopoverWidget>();
  widget->set_size(w, h);
}

//------------------------------------------------------------------------------
static void show(mforms::Popover* self, int x, int y, mforms::StartPosition pos) {
  PopoverWidget* widget = self->get_data<PopoverWidget>();
  widget->show_popover(x, y, pos);
}

//------------------------------------------------------------------------------
static void close(mforms::Popover* self) {
  PopoverWidget* w = self->get_data<PopoverWidget>();
  w->hide();
}

static void show_and_track(mforms::Popover* self, mforms::View* owner, int x, int y, mforms::StartPosition pos) {
  show(self, x, y, pos);
}

namespace mforms {
  namespace gtk {
    void Popover_init() {
      ::mforms::ControlFactory* f = ::mforms::ControlFactory::get_instance();

      f->_popover_impl.create = create;
      f->_popover_impl.set_content = set_content;
      f->_popover_impl.set_size = set_size;
      f->_popover_impl.show = show;
      f->_popover_impl.close = close;
      f->_popover_impl.show_and_track = show_and_track;
    }
  }
}
