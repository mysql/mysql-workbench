/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _LF_DRAWBOX_H_
#define _LF_DRAWBOX_H_

#include "mforms/label.h"
#include "lf_view.h"

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
      mutable Gtk::EventBox _darea;
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
        return &_darea;
      }

      DrawBoxImpl(::mforms::DrawBox *self);
      virtual ~DrawBoxImpl();
      bool repaint(const ::Cairo::RefPtr< ::Cairo::Context> &context, ::mforms::DrawBox *self);
      bool relayout(::mforms::DrawBox *self);
      void on_size_allocate(Gtk::Allocation &alloc, ::mforms::DrawBox *self);
      void mouse_cross_event(GdkEventCrossing *event, ::mforms::DrawBox *self);
      bool mouse_button_event(GdkEventButton *event, ::mforms::DrawBox *self);
      bool mouse_move_event(GdkEventMotion *event, ::mforms::DrawBox *self);

      static bool create(::mforms::DrawBox *self);
      static void set_needs_repaint(::mforms::DrawBox *self);
      static void add(::mforms::DrawBox *self, ::mforms::View *view, mforms::Alignment alignment);
      static void remove(::mforms::DrawBox *self, ::mforms::View *view);
      static void move(::mforms::DrawBox *self, ::mforms::View *view, int x, int y);
      virtual void set_padding_impl(int left, int top, int right, int bottom);

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
