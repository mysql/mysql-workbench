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

#include "../lf_mforms.h"
#include <gdkmm/types.h>

#include "../lf_view.h"
#include "base/util_functions.h"
#include "base/log.h"
#include <atkmm.h>
#include "gtk_helpers.h"
#include <gdk/gdkkeysyms-compat.h>
#include <gtkmm/stylecontext.h>

DEFAULT_LOG_DOMAIN("mforms.linux")

namespace mforms {
  namespace gtk {

    mforms::ModifierKey GetModifiers(const guint state, const guint keyval) {
      mforms::ModifierKey modifiers = mforms::ModifierNoModifier;
      Gdk::ModifierType mod_type = Gtk::AccelGroup::get_default_mod_mask();

      if ((state & mod_type) != 0) {
        if ((state & mod_type) == GDK_CONTROL_MASK)
          modifiers = modifiers | mforms::ModifierControl;
        if ((state & mod_type) == GDK_SHIFT_MASK)
          modifiers = modifiers | mforms::ModifierShift;
        if ((state & mod_type) == GDK_MOD1_MASK)
          modifiers = modifiers | mforms::ModifierAlt;
        if ((state & mod_type) == GDK_SUPER_MASK)
          modifiers = modifiers | mforms::ModifierCommand;
        if ((state & mod_type) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
          modifiers = modifiers | mforms::ModifierControl | mforms::ModifierShift;
        if ((state & mod_type) == (GDK_CONTROL_MASK | GDK_MOD1_MASK))
          modifiers = modifiers | mforms::ModifierControl | mforms::ModifierAlt;
        if ((state & mod_type) == (GDK_CONTROL_MASK | GDK_SUPER_MASK))
          modifiers = modifiers | mforms::ModifierControl | mforms::ModifierCommand;
        if ((state & mod_type) == (GDK_SHIFT_MASK | GDK_MOD1_MASK))
          modifiers = modifiers | mforms::ModifierShift | mforms::ModifierAlt;
        if ((state & mod_type) == (GDK_SHIFT_MASK | GDK_SUPER_MASK))
          modifiers = modifiers | mforms::ModifierShift | mforms::ModifierCommand;
        if ((state & mod_type) == (GDK_SUPER_MASK | GDK_MOD1_MASK))
          modifiers = modifiers | mforms::ModifierCommand | mforms::ModifierAlt;
      }
      return modifiers;
    }

    mforms::KeyCode GetKeys(const guint keyval) {
      mforms::KeyCode code = mforms::KeyUnkown;
      switch (keyval) {
        case GDK_Home:
          code = mforms::KeyHome;
          break;
        case GDK_End:
          code = mforms::KeyEnd;
          break;
        case GDK_Page_Up:
          code = mforms::KeyPrevious;
          break;
        case GDK_Page_Down:
          code = mforms::KeyNext;
          break;
        case GDK_Up:
          code = mforms::KeyUp;
          break;
        case GDK_Down:
          code = mforms::KeyDown;
          break;

        case GDK_KEY_Return:
          code = mforms::KeyReturn;
          break;
        case GDK_KEY_Tab:
        case GDK_KEY_ISO_Left_Tab:
          code = mforms::KeyTab;
          break;

        case GDK_KEY_KP_Enter:
          code = mforms::KeyEnter;
          break;
        case GDK_KEY_Menu:
          code = mforms::KeyMenu;
          break;
        case  GDK_KEY_F1:
          code = mforms::KeyF1;
          break;
        case  GDK_KEY_F2:
          code = mforms::KeyF2;
          break;
        case  GDK_KEY_F3:
          code = mforms::KeyF3;
          break;
        case  GDK_KEY_F4:
          code = mforms::KeyF4;
          break;
        case  GDK_KEY_F5:
          code = mforms::KeyF5;
          break;
        case  GDK_KEY_F6:
          code = mforms::KeyF6;
          break;
        case  GDK_KEY_F7:
          code = mforms::KeyF7;
          break;
        case  GDK_KEY_F8:
          code = mforms::KeyF8;
          break;
        case  GDK_KEY_F9:
          code = mforms::KeyF9;
          break;
        case  GDK_KEY_F10:
          code = mforms::KeyF10;
          break;
        case  GDK_KEY_F11:
          code = mforms::KeyF11;
          break;
        case  GDK_KEY_F12:
          code = mforms::KeyF12;
          break;
        case GDK_KEY_Shift_L:
        case GDK_KEY_Shift_R:
        case GDK_KEY_Alt_L:
        case GDK_KEY_Alt_R:
        case GDK_KEY_Control_L:
        case GDK_KEY_Control_R:
        case GDK_KEY_Super_L:
        case GDK_KEY_Super_R:
          code = mforms::KeyModifierOnly;
          break;
        default:
          if ((keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z) || (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z))
            code = mforms::KeyChar;
          break;
      }

      return code;
    }

    // get the widget that does the actual work. most of the time it will be the same as the outer one
    Gtk::Widget *ViewImpl::get_inner() const {
      return get_outer();
    }

    ViewImpl::ViewImpl(::mforms::View *view)
      : ObjectImpl(view),
        _back_image_alignment(mforms::NoAlign),
        _last_btn_down(NULL),
        _target(NULL),
        _drag_image(NULL) {
    }

    void ViewImpl::destroy(::mforms::View *self) {
      // Nothing to do here. Freeing platform objects happens in lf_base.h, via data free function.
    }

    void ViewImpl::show(::mforms::View *self, bool show) {
      ViewImpl *view = self->get_data<ViewImpl>();

      if (view) {
        view->show(show);
      }
    }

    void ViewImpl::show(bool show) {
      Gtk::Widget *widget = get_outer();
      if (show)
        widget->show();
      else
        widget->hide();
    }

    bool ViewImpl::is_shown(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        return view->get_outer()->is_visible();
      return false;
    }

    bool ViewImpl::is_fully_visible(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        // INFO: in gtk3 we can use gtk_widget_is_visible and this code can be removed.
        Gtk::Widget *w = view->get_outer();

        while (w->is_visible()) {
          if (w->get_parent() == NULL)
            return true;

          Gtk::Notebook *n = dynamic_cast<Gtk::Notebook *>(w->get_parent());
          if (n)
            if (n->page_num(*w) !=
                n->get_current_page()) // We need to check if we're on active tab, if not return false.
              return false;

          w = w->get_parent();
        };
      }
      return false;
    }

    void ViewImpl::set_tooltip(::mforms::View *self, const std::string &text) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
#if GTK_VERSION_GT(2, 10)
        view->get_outer()->set_tooltip_text(text);
        view->get_outer()->set_has_tooltip(!text.empty());
#endif
      }
    }

    void ViewImpl::set_font(::mforms::View *self, const std::string &fontDescription) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        // apply font settings
      }
    }

    int ViewImpl::get_width(const ::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        return widget->get_allocation().get_width();
      }
      return 0;
    }

    int ViewImpl::get_height(const ::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        return widget->get_allocation().get_height();
      }
      return 0;
    }

    int ViewImpl::get_preferred_width(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        return view->get_preferred_width();
      }
      return 0;
    }

    int ViewImpl::get_preferred_width() {
      int minimum, natural;
      get_outer()->get_preferred_width(minimum, natural);
      return natural;
    }

    int ViewImpl::get_preferred_height(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        return view->get_preferred_height();
      return 0;
    }

    int ViewImpl::get_preferred_height() {
      int minimum, natural;
      get_outer()->get_preferred_height(minimum, natural);
      return natural;
    }

    int ViewImpl::get_x(const ::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        return widget->get_allocation().get_x();
      }
      return 0;
    }

    int ViewImpl::get_y(const ::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        return widget->get_allocation().get_y();
      }
      return 0;
    }

    void ViewImpl::set_size(::mforms::View *self, int w, int h) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        view->set_size(w, h);
    }

    void ViewImpl::set_size(int width, int height) {
      Gtk::Window *wnd = dynamic_cast<Gtk::Window *>(get_outer());
      if (wnd != nullptr) {
        wnd->set_default_size(width, height);
      } else
        get_outer()->set_size_request(width, height);
    }

    void ViewImpl::set_min_size(::mforms::View *self, int w, int h) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        view->set_min_size(w, h);
    }

    void ViewImpl::set_min_size(int width, int height) {
      get_outer()->set_size_request(width, height);
    }

    void ViewImpl::set_position(::mforms::View *self, int x, int y) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        mforms::View *parent = self->get_parent();
        if (parent) {
          ViewImpl *parent_view_impl = parent->get_data<ViewImpl>();
          if (parent_view_impl)
            parent_view_impl->move_child(view, x, y);
        }
      }
    }

    std::pair<int, int> ViewImpl::client_to_screen(::mforms::View *self, int x, int y) {
      ViewImpl *view = self->get_data<ViewImpl>();

      if (view) {
        Gtk::Widget *widget = view->get_outer();
        if (widget) {
          Glib::RefPtr<Gdk::Window> wnd = widget->get_window();
          if (wnd) {
#if GTK_VERSION_LT(2, 18)
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

    void ViewImpl::set_enabled(::mforms::View *self, bool flag) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        widget->set_sensitive(flag);
      }
    }

    bool ViewImpl::is_enabled(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        return widget->get_sensitive();
      }
      return false;
    }

    void ViewImpl::set_name(::mforms::View *self, const std::string &name) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        view->set_name(name);
    }

    void ViewImpl::set_name(const std::string &name) {
      get_outer()->set_name(name);

      {
        Glib::RefPtr<Atk::Object> acc = get_outer()->get_accessible();
        if (acc)
          acc->set_name(name);
      }
      if (get_outer() != get_inner() && get_inner()) {
        Glib::RefPtr<Atk::Object> acc = get_inner()->get_accessible();
        if (acc)
          acc->set_name(name);
      }
    }

    void ViewImpl::relayout(::mforms::View *view) {
      // noop
    }

    void ViewImpl::set_needs_repaint(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        Gtk::Widget *widget = view->get_outer();
        if (widget) {
          //      Glib::RefPtr<Gdk::Window> window = widget->get_window();
          //      if (window)
          //        window->process_updates(true);
          //      else
          widget->queue_draw();
        }
      }
    }

    void ViewImpl::suspend_layout(::mforms::View *self, bool flag) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        view->suspend_layout(flag);
    }

    void ViewImpl::size_changed() {
      if (get_outer() && get_outer()->get_realized()) {
        ::mforms::View *owner_view = dynamic_cast< ::mforms::View *>(owner);
        if (owner_view)
          (*owner_view->signal_resized())();
      }
    }

    void ViewImpl::on_focus_grab() {
      if (get_outer() && get_outer()->get_realized()) {
        ::mforms::View *owner_view = dynamic_cast< ::mforms::View *>(owner);
        if (owner_view)
          owner_view->focus_changed();
      }
    }

    bool ViewImpl::has_focus(mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        return view->get_inner()->has_focus();
      return false;
    }

    bool ViewImpl::on_button_release(GdkEventButton *btn) {
      if (_last_btn_down) {
        delete _last_btn_down;
        _last_btn_down = NULL;
      }
      return true;
    }

    bool ViewImpl::on_button_press(GdkEventButton *btn) {
      if (_last_btn_down == NULL)
        _last_btn_down = new Gdk::Event((GdkEvent *)btn);
      return true;
    }

    void ViewImpl::setup() {
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

      get_outer()->signal_focus_in_event().connect([this](GdkEventFocus *evt) {
        if (!owner->is_destroying()) {
          ::mforms::View *ownerView = dynamic_cast< ::mforms::View *>(owner);
          if (ownerView != nullptr)
            return ownerView->focusIn();
        }
        return false;
      });

      get_outer()->signal_focus_out_event().connect([this](GdkEventFocus *evt) {
        if (!owner->is_destroying()) {
          ::mforms::View *ownerView = dynamic_cast< ::mforms::View *>(owner);
          if (ownerView != nullptr)
            return ownerView->focusOut();
        }
        return false;
      });

      get_outer()->signal_enter_notify_event().connect_notify([this](GdkEventCrossing *event) {
        if (!owner->is_destroying()) {
          ::mforms::View *ownerView = dynamic_cast< ::mforms::View *>(owner);
          if (ownerView != nullptr)
            ownerView->mouse_enter();
        }
      });

      get_outer()->signal_leave_notify_event().connect_notify([this](GdkEventCrossing *event) {
        if (!owner->is_destroying()) {
          ::mforms::View *ownerView = dynamic_cast< ::mforms::View *>(owner);
          if (ownerView != nullptr)
            ownerView->mouse_leave();
        }
      });

      get_outer()->signal_key_press_event().connect([this](GdkEventKey *event) {
        if (!owner->is_destroying()) {
          ::mforms::View *ownerView = dynamic_cast< ::mforms::View *>(owner);
          if (ownerView != nullptr)
            return ownerView->keyPress(GetKeys(event->keyval), GetModifiers(event->state, event->keyval));
        }
        return false;
      });

      get_outer()->signal_key_release_event().connect([this](GdkEventKey *event) {
        if (!owner->is_destroying()) {
          ::mforms::View *ownerView = dynamic_cast< ::mforms::View *>(owner);
          if (ownerView != nullptr)
            return ownerView->keyRelease(GetKeys(event->keyval), GetModifiers(event->state, event->keyval));
        }
        return false;
      });

      get_outer()->add_events(Gdk::FOCUS_CHANGE_MASK|Gdk::KEY_PRESS_MASK|Gdk::KEY_RELEASE_MASK);
    }

    void ViewImpl::move_child(ViewImpl *child, int x, int y) {
      throw std::logic_error("container does not implement required method");
    }

    void ViewImpl::set_front_color(::mforms::View *self, const std::string &color) {
      ViewImpl *view = self->get_data<ViewImpl>();
      Gtk::Widget *w = view->get_inner();
      if (w) {
        mforms::gtk::set_color(w, color, FG_COLOR);
        if (!color.empty())
          w->override_color(color_to_rgba(Gdk::Color(color)), Gtk::STATE_FLAG_NORMAL);
        else
          w->unset_color(Gtk::STATE_FLAG_NORMAL);
      }
      view->set_front_color(color);
    }

    void ViewImpl::set_back_color(const std::string &color) {
      Gtk::Widget *w = this->get_inner();
      if (w) {
        mforms::gtk::set_color(w, color, BG_COLOR);

        auto provider = Gtk::CssProvider::create();
        if (color.empty())
          provider->load_from_data("* { background-color: rgba(0, 0, 0, 0); }");
        else
          provider->load_from_data("* { background-color: " + color + "; }");
        w->get_style_context()->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_USER);

        // Outer widget should also receive color, cause when someone will set padding there will be frame.
        Gtk::Widget *o = this->get_outer();
        if (o && o != w) {
          auto provider = Gtk::CssProvider::create();
          if (color.empty())
            provider->load_from_data("* { background-color: rgba(0, 0, 0, 0); }");
          else
            provider->load_from_data("* { background-color: " + color + "; }");
          o->get_style_context()->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_USER);
        }
      }
    }

    void ViewImpl::set_back_color(::mforms::View *self, const std::string &color) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        view->set_back_color(color);
    }

    std::string ViewImpl::get_front_color(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      base::Color *c = mforms::gtk::get_color(view->get_inner(), FG_COLOR);
      if (c)
        return c->to_html();
      return "";
    }

    std::string ViewImpl::get_back_color(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      base::Color *c = mforms::gtk::get_color(view->get_inner(), BG_COLOR);
      if (c)
        return c->to_html();
      return "";
    }

    bool ViewImpl::on_draw_event(const ::Cairo::RefPtr< ::Cairo::Context> &context, Gtk::Widget *target) {
      int x, y;
      int iwidth, fwidth;
      int iheight, fheight;

      if (_back_image) {
        x = 0;
        y = 0;
        iwidth = _back_image->get_width();
        iheight = _back_image->get_height();
        fwidth = target->get_width();
        fheight = target->get_height();

        switch (_back_image_alignment) {
          case mforms::TopLeft:
            x = 0;
            y = 0;
            break;
          case mforms::TopCenter:
            x = (iwidth + fwidth) / 2;
            y = 0;
            break;
          case mforms::TopRight:
            x = fwidth - iwidth;
            y = 0;
            break;
          case mforms::MiddleLeft:
            x = 0;
            y = (iheight + fheight) / 2;
            break;
          case mforms::MiddleCenter:
            x = (iwidth + fwidth) / 2;
            y = (iheight + fheight) / 2;
            break;
          case mforms::MiddleRight:
            x = fwidth - iwidth;
            y = (iheight + fheight) / 2;
            break;
          case mforms::BottomLeft:
            x = 0;
            y = fheight - iheight;
            break;
          case mforms::BottomCenter:
            x = (iwidth + fwidth) / 2;
            y = fheight - iheight;
            break;
          case mforms::BottomRight:
            x = fwidth - iwidth;
            y = fheight - iheight;
            break;
          default:
            break;
        }

        ::Cairo::RefPtr< ::Cairo::Context> ctx = target->get_window()->create_cairo_context();
        Gdk::Cairo::set_source_pixbuf(ctx, _back_image, x, y);
        ctx->paint();

        return true;
      }
      return false;
    }

    void ViewImpl::set_back_image(::mforms::View *self, const std::string &path, mforms::Alignment ali) {
      ViewImpl *view = self->get_data<ViewImpl>();
      view->set_back_image(path, ali);
    }

    void ViewImpl::set_back_image(const std::string &path, mforms::Alignment alig) {
      if (path.empty()) {
        _back_image.reset();
        return;
      }

      std::string file = mforms::App::get()->get_resource_path(path);
      try {
        _back_image = Gdk::Pixbuf::create_from_file(file);
        _back_image_alignment = alig;
      } catch (Glib::Error &exc) {
        logError("Could not load image for background: %s: %s\n", file.c_str(), exc.what().c_str());
      }
    }

    void ViewImpl::flush_events(::mforms::View *self) {
      while (Gtk::Main::events_pending())
        Gtk::Main::iteration();
    }

    void ViewImpl::set_padding(::mforms::View *self, int left, int top, int right, int bottom) {
      ViewImpl *view = self->get_data<ViewImpl>();
      view->set_padding_impl(left, top, right, bottom);
    }

    void ViewImpl::set_padding_impl(int left, int top, int right, int bottom) {
    }

    //------------------------------------------------------------------------------
    void ViewImpl::register_drop_formats(::mforms::View *self, DropDelegate *target,
                                         const std::vector<std::string> &formats) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        view->register_drop_formats(formats, target);
      }
    }

    void ViewImpl::register_drop_formats(const std::vector<std::string> &formats, DropDelegate *target) {
      _target = target;
      std::vector<Gtk::TargetEntry> targets;
      _drop_formats.clear();
      for (size_t i = 0; i < formats.size(); ++i) {
        targets.push_back(Gtk::TargetEntry(formats[i], Gtk::TargetFlags(0), i));
        _drop_formats.insert(std::pair<std::string, size_t>(formats[i], i));
      }
      // We add two more formats, first is for allow files drop, second to allow paste text from other applications.
      targets.push_back(Gtk::TargetEntry("text/uri-list", Gtk::TargetFlags(0), formats.size()));
      _drop_formats.insert(std::pair<std::string, size_t>("text/uri-list", formats.size()));
      targets.push_back(Gtk::TargetEntry("STRING", Gtk::TargetFlags(0), formats.size()));
      _drop_formats.insert(std::pair<std::string, size_t>("STRING", formats.size()));

      Gtk::Widget *widget = this->get_outer();
      if (widget) {
        widget->drag_dest_set(targets, Gtk::DEST_DEFAULT_HIGHLIGHT, Gdk::ACTION_COPY | Gdk::ACTION_MOVE);

        Glib::RefPtr<Gtk::TargetList> target_list = Glib::RefPtr<Gtk::TargetList>(Gtk::TargetList::create(targets));
        widget->drag_dest_set_target_list(
          target_list); // need to explicitly specify target list, cause the one, set by drag_dest_set is not working :/

        widget->signal_drag_motion().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_motion));
        widget->signal_drag_drop().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_drop));
        widget->signal_drag_data_received().connect(sigc::mem_fun(this, &ViewImpl::slot_drag_data_received));
      }
    }

    //------------------------------------------------------------------------------
    void ViewImpl::slot_drag_begin(const Glib::RefPtr<Gdk::DragContext> &context) {
      if (_drag_image)
        context->set_icon(Cairo::RefPtr<Cairo::Surface>(new Cairo::Surface(_drag_image)));
    }

    //------------------------------------------------------------------------------
    // The drag_data_get signal is emitted on the drag source when the drop site requests the data which is dragged.
    void ViewImpl::slot_drag_data_get(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::SelectionData &data, guint,
                                      guint time) {
      const Glib::ustring target = data.get_target();
      std::map<std::string, DataWrapper>::iterator it = _drop_data.find(target);
      if (it != _drop_data.end()) // TODO need to store in _drop_data also data size for data to be sent
      {
        if (target == "STRING") {
          data.set(target, 8, reinterpret_cast<const guchar *>(((std::string *)(it->second.GetData()))->c_str()),
                   ((std::string *)(it->second.GetData()))->size());
        } else
          data.set(target, 8, reinterpret_cast<const guint8 *>(&it->second), sizeof(it->second));
      }
    }

    // The drag_end signal is emmited on the drag source when the drag was finished
    void ViewImpl::slot_drag_end(const Glib::RefPtr<Gdk::DragContext> &context) {
      _drop_data.clear();
      _drag_image = NULL;
      _loop.quit(); // cause in do_drag_drop we called run()
    }

    // The drag_end signal is emmited on the drag source when the drag was failed due to user cancel, timeout, etc.
    bool ViewImpl::slot_drag_failed(const Glib::RefPtr<Gdk::DragContext> &context, Gtk::DragResult result) {
      if (result != Gtk::DRAG_RESULT_NO_TARGET && result != Gtk::DRAG_RESULT_USER_CANCELLED)
        _loop.quit(); // cause in do_drag_drop we called run()

      _drag_image = NULL;
      return false;
    }
    //------------------------------------------------------------------------------
    // The drag_data_delete signal is emitted on the drag source when a drag with the action Gdk::ACTION_MOVE is
    // successfully completed.
    void ViewImpl::slot_drag_data_delete(const Glib::RefPtr<Gdk::DragContext> &context) {
    }

    //------------------------------------------------------------------------------
    // The drag_drop signal is emitted on the drop site when the user drops the data onto the widget.
    bool ViewImpl::slot_drag_drop(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
      mforms::View *view = dynamic_cast<mforms::View *>(owner);
      Gtk::Widget *widget = ViewImpl::get_widget_for_view(view);

      mforms::DropDelegate *drop_delegate = _target;
      if (drop_delegate == NULL)
        drop_delegate = reinterpret_cast<mforms::DropDelegate *>(owner);

      if (drop_delegate == NULL || view == NULL || widget == NULL)
        return false; // abandon drop cause we can't accept anything

      std::vector<std::string> targets(context->list_targets());
      if (targets.empty()) // is not valid drop drop site :(
        return false;

      // There should be only one element in targets, so pick it up
      std::string target = targets[0];
      if (targets.size() > 1) { // if not, we need to pick up the best one.
        std::vector<std::string>::iterator it = std::find(targets.begin(), targets.end(), "text/uri-list");
        if (it != targets.end())
          target = *it;
        else // Try STRING
          it = std::find(targets.begin(), targets.end(), "STRING");
        if (it != targets.end())
          target = *it;
      }

      widget->drag_get_data(context, target, time);

// We have to send this event because as of GTK#a0e805684860306b80109814499d0cdaba5fd6ba
// there's still some unreleased event.
#if GTK_CHECK_VERSION(3, 18, 0)
      if (_last_btn_down != nullptr) {
        GdkEvent *send_event;
        send_event = gdk_event_new(GDK_BUTTON_RELEASE);
        send_event->button.window = context->get_source_window()->gobj();
        send_event->button.send_event = TRUE;
        send_event->button.time = time;
        send_event->button.x = x;
        send_event->button.y = y;
        send_event->button.axes = NULL;
        send_event->button.state = 0;
        send_event->button.button = _last_btn_down->gobj()->button.button;
        send_event->button.device = context->get_device()->gobj();
        send_event->button.x_root = 0;
        send_event->button.y_root = 0;

        gtk_propagate_event(widget->gobj(), send_event);
        gdk_event_free(send_event);
      }
#endif

      return true;
    }
    //------------------------------------------------------------------------------
    // The drag_motion signal is emitted on the drop site when the user moves the cursor over the widget during a drag.
    bool ViewImpl::slot_drag_motion(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y, guint time) {
      mforms::DropDelegate *drop_delegate = _target;
      if (drop_delegate == NULL)
        drop_delegate = dynamic_cast<mforms::DropDelegate *>(owner);

      bool ret_val = false;
      if (drop_delegate != NULL) {
        std::vector<std::string> targets = context->list_targets();
        // We need to fix a formats a little, so file will be recognized
        bool accept_string = false;

        for (std::vector<std::string>::iterator it = targets.begin(); it < targets.end(); it++) {
          if ((*it) == "text/uri-list") {
            targets.push_back(mforms::DragFormatFileName);
            break;
          }

          if ((*it) == "STRING") {
            accept_string = true;
            break;
          }
        }

        mforms::DragOperation operation, allowedOperations = mforms::DragOperationNone;

        if ((context->get_suggested_action() & Gdk::ACTION_COPY) == Gdk::ACTION_COPY)
          allowedOperations = allowedOperations | mforms::DragOperationCopy;
        if ((context->get_suggested_action() & Gdk::ACTION_MOVE) == Gdk::ACTION_MOVE)
          allowedOperations = allowedOperations | mforms::DragOperationMove;

        operation = drop_delegate->drag_over((mforms::View *)owner, base::Point(x, y), allowedOperations, targets);
        switch (operation) {
          case DragOperationCopy:
          case DragOperationMove:
            ret_val = true;
            break;
          case DragOperationNone:
          default:
            // STRING type is handled in different way, we support it by default
            ret_val = accept_string;
            break;
        }
      }

      if (ret_val) {
        context->drag_status(context->get_suggested_action(), time);

        get_outer()->drag_highlight();

        return true;
      } else
        context->drag_refuse(time);

      return false;
    }

    //------------------------------------------------------------------------------
    // The drag_data_received signal is emitted on the drop site when the dragged data has been received.
    // called when drag is from outside of WB
    void ViewImpl::slot_drag_data_received(const Glib::RefPtr<Gdk::DragContext> &context, int x, int y,
                                           const Gtk::SelectionData &data, guint info, guint time) {
      DataWrapper *dwrapper = (DataWrapper *)data.get_data();

      mforms::DropDelegate *drop_delegate = _target;
      if (drop_delegate == NULL)
        drop_delegate = dynamic_cast<mforms::DropDelegate *>(owner);

      if (drop_delegate == NULL || !dwrapper)
        return;

      std::vector<Glib::ustring> files;
      if (data.get_length() >= 0 && data.get_format() == 8)
        files = data.get_uris();

      mforms::DragOperation allowedOperations = mforms::DragOperationNone;

      if ((context->get_suggested_action() & Gdk::ACTION_COPY) == Gdk::ACTION_COPY)
        allowedOperations = allowedOperations | mforms::DragOperationCopy;
      if ((context->get_suggested_action() & Gdk::ACTION_MOVE) == Gdk::ACTION_MOVE)
        allowedOperations = allowedOperations | mforms::DragOperationMove;

      if (files.empty()) {
        std::string tmpstr = *context->list_targets().begin();

        drop_delegate->data_dropped((mforms::View *)owner, base::Point(x, y), allowedOperations, dwrapper->GetData(),
                                    tmpstr);
      } else {
        // We need to convert uris to file names
        for (std::vector<Glib::ustring>::iterator it = files.begin(); it < files.end(); ++it)
          (*it) = Glib::filename_from_uri((*it));

        std::vector<std::string> files_std;
        files_std.assign(files.begin(), files.end());

        drop_delegate->files_dropped((mforms::View *)owner, base::Point(x, y), allowedOperations, files_std);
      }

      context->drag_finish(true, false, time);
    }

    mforms::DragOperation ViewImpl::drag_text(::mforms::View *self, ::mforms::DragDetails details,
                                              const std::string &text) {
      return mforms::DragOperationNone;
    }

    mforms::DragOperation ViewImpl::drag_data(::mforms::View *self, ::mforms::DragDetails details, void *data,
                                              const std::string &format) {
      DragOperation dop = mforms::DragOperationNone;
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view) {
        return view->drag_data(details, data, format);
      }
      return dop;
    }

    DragOperation ViewImpl::drag_data(::mforms::DragDetails details, void *data, const std::string &format) {
      DragOperation drag_op = mforms::DragOperationNone;
      Gtk::Widget *widget = get_outer();
      if (widget) {
        Gdk::DragAction actions = Gdk::ACTION_DEFAULT;

        if ((details.allowedOperations & mforms::DragOperationCopy) != 0)
          actions |= Gdk::ACTION_COPY;
        if ((details.allowedOperations & mforms::DragOperationMove) != 0)
          actions |= Gdk::ACTION_MOVE;

        std::map<std::string, size_t>::iterator it = _drop_formats.find(format);
        if (it == _drop_formats.end()) {
          std::pair<std::map<std::string, size_t>::iterator, bool> insertptr =
            _drop_formats.insert(std::make_pair(format, _drop_formats.size()));
          if (!insertptr.second) {
            return drag_op;
          }

          it = insertptr.first;
        }

        _drop_data.clear();
        DataWrapper dwrapper(data);
        _drop_data.insert(std::pair<std::string, DataWrapper>(format, dwrapper));

        std::vector<Gtk::TargetEntry> targets;
        targets.push_back(Gtk::TargetEntry(it->first.c_str(), Gtk::TargetFlags(0), it->second));

        const Glib::RefPtr<Gtk::TargetList> tlist = Gtk::TargetList::create(targets);
        _drag_image = details.image;
        Glib::RefPtr<Gdk::DragContext> context;
        if (_last_btn_down)
          context = widget->drag_begin(tlist, actions, 1, _last_btn_down->gobj());
        else
          context = widget->drag_begin(tlist, actions, 1, NULL);
        _loop.run();

        drag_op = details.allowedOperations;
      }
      return drag_op;
    }

    bool draw_event_slot(const ::Cairo::RefPtr< ::Cairo::Context> &context, Gtk::Widget *widget) {
      return false;
    }

    static void destroy_color(base::Color *col) {
      if (col)
        delete col;
    }

    void set_color(Gtk::Widget *w, const std::string &color, const mforms::gtk::WBColor col_type) {
      std::string key;
      switch (col_type) {
        case mforms::gtk::BG_COLOR:
          key = "BG_COLOR";
          break;
        case mforms::gtk::FG_COLOR:
          key = "FG_COLOR";
          break;
      }
      if (!color.empty()) {
        base::Color *col = new base::Color(color);
        if (col->is_valid())
          g_object_set_data_full(G_OBJECT(w->gobj()), key.c_str(), (void *)col, (GDestroyNotify)destroy_color);
      } else {
        base::Color *color = (base::Color *)g_object_get_data(G_OBJECT(w->gobj()), key.c_str());
        if (color)
          delete color;
        g_object_set_data(G_OBJECT(w->gobj()), key.c_str(), NULL);
      }
    }

    base::Color *get_color(Gtk::Widget *w, const mforms::gtk::WBColor col_type) {
      std::string key;
      switch (col_type) {
        case mforms::gtk::BG_COLOR:
          key = "BG_COLOR";
          break;
        case mforms::gtk::FG_COLOR:
          key = "FG_COLOR";
          break;
      }
      return (base::Color *)g_object_get_data(G_OBJECT(w->gobj()), key.c_str());
    }

    void ViewImpl::focus(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view && view->get_inner())
        view->get_inner()->grab_focus();
    }

    mforms::DropPosition ViewImpl::get_drop_position() {
      return mforms::DropPositionUnknown;
    }

    mforms::DropPosition ViewImpl::get_drop_position(::mforms::View *self) {
      ViewImpl *view = self->get_data<ViewImpl>();
      if (view)
        return view->get_drop_position();

      return mforms::DropPositionUnknown;
    }

    void ViewImpl::init() {
      ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

      f->_view_impl.destroy = &ViewImpl::destroy;
      f->_view_impl.get_width = &ViewImpl::get_width;
      f->_view_impl.get_height = &ViewImpl::get_height;
      f->_view_impl.get_preferred_width = &ViewImpl::get_preferred_width;
      f->_view_impl.get_preferred_height = &ViewImpl::get_preferred_height;
      f->_view_impl.set_size = &ViewImpl::set_size;
      f->_view_impl.set_min_size = &ViewImpl::set_min_size;

      f->_view_impl.get_x = &ViewImpl::get_x;
      f->_view_impl.get_y = &ViewImpl::get_y;
      f->_view_impl.set_position = &ViewImpl::set_position;
      f->_view_impl.client_to_screen = &ViewImpl::client_to_screen;

      f->_view_impl.show = &ViewImpl::show;
      f->_view_impl.is_shown = &ViewImpl::is_shown;
      f->_view_impl.is_fully_visible = &ViewImpl::is_fully_visible;

      f->_view_impl.set_tooltip = &ViewImpl::set_tooltip;
      f->_view_impl.set_font = &ViewImpl::set_font;
      f->_view_impl.set_name = &ViewImpl::set_name;

      f->_view_impl.set_enabled = &ViewImpl::set_enabled;
      f->_view_impl.is_enabled = &ViewImpl::is_enabled;

      f->_view_impl.suspend_layout = &ViewImpl::suspend_layout;
      f->_view_impl.relayout = &ViewImpl::relayout;
      f->_view_impl.set_needs_repaint = &ViewImpl::set_needs_repaint;
      f->_view_impl.set_front_color = &ViewImpl::set_front_color;
      f->_view_impl.get_front_color = &ViewImpl::get_front_color;
      f->_view_impl.set_back_color = &ViewImpl::set_back_color;
      f->_view_impl.get_back_color = &ViewImpl::get_back_color;
      f->_view_impl.set_back_image = &ViewImpl::set_back_image;
      f->_view_impl.flush_events = &ViewImpl::flush_events;
      f->_view_impl.set_padding = &ViewImpl::set_padding;
      f->_view_impl.drag_data = &ViewImpl::drag_data;
      f->_view_impl.drag_text = &ViewImpl::drag_text;

      f->_view_impl.register_drop_formats = &ViewImpl::register_drop_formats;
      f->_view_impl.focus = &ViewImpl::focus;
      f->_view_impl.has_focus = &ViewImpl::has_focus;
      f->_view_impl.get_drop_position = &ViewImpl::get_drop_position;
    };

    Gtk::Widget *ViewImpl::get_widget_for_view(mforms::View *view) {
      ViewImpl *vi = view->get_data<ViewImpl>();

      if (vi) {
        Gtk::Widget *w = vi->get_outer();
        w->set_data("mforms::View", view);
        return w;
      }
      return NULL;
    }

    mforms::View *ViewImpl::get_view_for_widget(Gtk::Widget *w) {
      mforms::View *view = (mforms::View *)w->get_data("mforms::View");
      if (view)
        return view;
      return NULL;
    }
  };
};
