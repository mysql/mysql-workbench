/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _CONNECTION_FIGURE_H_
#define _CONNECTION_FIGURE_H_

#include "mdc.h"
#include <grts/structs.model.h>

namespace wbfig {
  class FigureEventHub;

  class ConnectionLineLayouter : public mdc::OrthogonalLineLayouter {
    typedef mdc::OrthogonalLineLayouter super;

    virtual std::vector<mdc::ItemHandle *> create_handles(mdc::Line *line, mdc::InteractionLayer *ilayer);

    virtual bool update_start_point();
    virtual bool update_end_point();

    virtual bool handle_dragged(mdc::Line *line, mdc::ItemHandle *handle, const base::Point &pos, bool dragging);

  public:
    enum Type { NormalLine, ZLine };

  private:
    Type _type;

    virtual std::vector<base::Point> get_points_for_subline(int subline);

  public:
    ConnectionLineLayouter(mdc::Connector *sconn, mdc::Connector *econn);

    void set_type(Type type);
    Type get_type() const {
      return _type;
    }
  };

  class Connection : public mdc::Line {
    typedef mdc::Line super;

  public:
    enum DiamondType { None, Filled, LeftEmpty, RightEmpty, Empty };
    enum CaptionPos { Below, Above, Middle };

    Connection(mdc::Layer *layer, FigureEventHub *hub, model_Object *represented_object);

    void set_splitted(bool flag);

    base::Point get_middle_caption_pos(const base::Size &size, CaptionPos pos);
    base::Point get_start_caption_pos(const base::Size &size);
    base::Point get_end_caption_pos(const base::Size &size);

    void set_start_dashed(bool flag);
    void set_end_dashed(bool flag);

    virtual void render(mdc::CairoCtx *cr);
    virtual void render_gl(mdc::CairoCtx *cr);

    void set_start_figure(mdc::CanvasItem *item);
    void set_end_figure(mdc::CanvasItem *item);
    mdc::CanvasItem *get_start_figure() {
      return _start_figure;
    }
    mdc::CanvasItem *get_end_figure() {
      return _end_figure;
    }

    void set_diamond_type(DiamondType type);

    double get_segment_offset(int subline);
    void set_segment_offset(int subline, double offset);

    virtual bool on_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                          mdc::EventState state);
    virtual bool on_double_click(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_enter(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_leave(mdc::CanvasItem *target, const base::Point &point);
    virtual bool on_button_press(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                 mdc::EventState state);
    virtual bool on_button_release(mdc::CanvasItem *target, const base::Point &point, mdc::MouseButton button,
                                   mdc::EventState state);

    void set_center_captions(bool flag);
    bool get_center_captions() {
      return _center_captions;
    }
    virtual bool can_render_gl() {
      return true;
    }

  private:
    model_Object *_represented_object;
    FigureEventHub *_hub;

    mdc::CanvasItem *_start_figure;
    mdc::CanvasItem *_end_figure;

    DiamondType _diamond;
    bool _start_dashed;
    bool _end_dashed;
    bool _split;
    bool _center_captions;

    virtual bool contains_point(const base::Point &point) const;

    virtual void stroke_outline(mdc::CairoCtx *cr, float offset = 0) const;
    void stroke_outline_gl(float offset = 0) const;

    double get_middle_segment_angle();

    void update_layouter();

    virtual void mark_crossings(mdc::Line *line);
  };
};

#endif
