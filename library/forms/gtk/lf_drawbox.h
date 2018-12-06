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

#ifndef _LF_DRAWBOX_H_
#define _LF_DRAWBOX_H_

#include "mforms/label.h"
#include "lf_view.h"
#include "mforms_acc.h"
#include <atkmm/selection.h>
#include <glib.h>
#include <sigc++/connection.h>

namespace mforms {
  namespace gtk {
    class DrawBoxImpl : public ViewImpl {

      struct AlignControl {
        mforms::Alignment _align;
        int _x;
        int _y;
      };
      struct PaddingInfo {
        int _left;
        int _right;
        int _top;
        int _bottom;
      };

      void *on_repaint();

    protected:
      Gtk::EventBox* _darea;
      MFormsObject *_mformsGTK;
      int _fixed_width;
      int _fixed_height;
      Gtk::Fixed *_fixed;
      bool _relayout_pending;
      PaddingInfo _padding;
      mforms::MouseButton _last_btn;
      base::Point _mousePos;
      bool _drag_in_progress;

      sigc::connection _sig_relayout;

      std::map<Gtk::Widget *, AlignControl> _alignments;

      virtual Gtk::Widget *get_outer() const {
        return _darea;
      }

      DrawBoxImpl(::mforms::DrawBox *self);
      virtual ~DrawBoxImpl();
      bool repaint(const ::Cairo::RefPtr< ::Cairo::Context> &context, ::mforms::DrawBox *self);
      bool relayout(::mforms::DrawBox *self);
      void on_size_allocate(Gtk::Allocation &alloc, ::mforms::DrawBox *self);
      bool mouse_button_event(GdkEventButton *event, ::mforms::DrawBox *self);
      bool mouse_move_event(GdkEventMotion *event, ::mforms::DrawBox *self);

      static bool create(::mforms::DrawBox *self);
      static void set_needs_repaint(::mforms::DrawBox *self);
      static void add(::mforms::DrawBox *self, ::mforms::View *view, mforms::Alignment alignment);
      static void remove(::mforms::DrawBox *self, ::mforms::View *view);
      static void move(::mforms::DrawBox *self, ::mforms::View *view, int x, int y);
      virtual void set_padding_impl(int left, int top, int right, int bottom);
      static void drawFocus(::mforms::DrawBox *self, cairo_t *cr, const base::Rect r);

    public:
      static void init();

      virtual void set_size(int width, int height);

      void add(::mforms::View *view, mforms::Alignment alignment);
      void remove(::mforms::View *view);
      void move(::mforms::View *view, int x, int y);
    };
  };
};

#endif /* _LF_DRAWBOX_H_ */
