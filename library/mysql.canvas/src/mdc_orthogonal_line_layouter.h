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

#ifndef _MDC_ORTHOGONAL_LINE_LAYOUTER_H_
#define _MDC_ORTHOGONAL_LINE_LAYOUTER_H_

#include "mdc_line.h"
#include "mdc_connector.h"

namespace mdc {

  class Connector;

  // Multiple-segment orthogonal line

  /*

  A segment can look like:

  a)              b)
  ------+       ------++
        |             ||
        |   or        ||
        |             ++------

  Segment offsets are only used in case b and it determines the
  horizontal (or vertical) offset of the segment in the middle.


  If the segment is further splitted, it will have additional
  middle nodes (marked as X) which can be dragged around.
  The 2 parts of the line connected by the middle node
  act as 2 segments and have their own segment offset.

  ----++
      ||
      ||
      +----X
           |
           |
           +-------

  */
  class MYSQLCANVAS_PUBLIC_FUNC OrthogonalLineLayouter : public LineLayouter {
    typedef LineLayouter super;

  public:
    OrthogonalLineLayouter(Connector *sconn, Connector *econn);
    virtual ~OrthogonalLineLayouter();

    virtual Connector *get_start_connector() const {
      return _linfo.start_connector();
    }
    virtual Connector *get_end_connector() const {
      return _linfo.end_connector();
    }

    virtual std::vector<base::Point> get_points();
    virtual base::Point get_start_point();
    virtual base::Point get_end_point();

    int count_sublines() const {
      return _linfo.count_sublines();
    }
    double get_segment_offset(int subline) const {
      return _linfo.subline_offset(subline);
    }
    void set_segment_offset(int subline, double offset);

    virtual void update();

  protected:
    struct LineInfo {
    private:
      Connector *_start_connector;
      Connector *_end_connector;

      // indexed by vertex
      std::vector<base::Point> _points;
      std::vector<double> _point_angles;

      // indexed by subline
      std::vector<double> _middle_offsets;

    public:
      LineInfo(Connector *start, Connector *end) : _start_connector(start), _end_connector(end) {
        _points.push_back(_start_connector->get_position());
        _points.push_back(_end_connector->get_position());

        _point_angles.push_back(0);
        _point_angles.push_back(90);

        _middle_offsets.push_back(0.0);
      }

      int count_sublines() const {
        return (int)_points.size() - 1;
      }

      Connector *start_connector() const {
        return _start_connector;
      }

      Connector *end_connector() const {
        return _end_connector;
      }

      int start_subline() const {
        return 0;
      }

      int end_subline() const {
        return (int)_points.size() / 2 - 1;
      }

      base::Point subline_start_point(int subline) const {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        return _points[subline * 2];
      }

      base::Point subline_end_point(int subline) const {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        return _points[subline * 2 + 1];
      }

      double subline_start_angle(int subline) const {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        return _point_angles[subline * 2];
      }

      double subline_end_angle(int subline) const {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        return _point_angles[subline * 2 + 1];
      }

      bool angle_is_vertical(double angle) const {
        if (angle == 90 || angle == 270)
          return true;
        return false;
      }

      bool subline_is_perpendicular(int subline) const {
        if (angle_is_vertical(subline_start_angle(subline)) != angle_is_vertical(subline_end_angle(subline)))
          return true;
        return false;
      }

      double subline_offset(int subline) const {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");

        return _middle_offsets[subline];
      }

      void set_subline_start_point(int subline, const base::Point &p, double angle) {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        _points[subline * 2] = p;
        _point_angles[subline * 2] = angle;
      }

      void set_subline_end_point(int subline, const base::Point &p, double angle) {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        _points[subline * 2 + 1] = p;
        _point_angles[subline * 2 + 1] = angle;
      }

      void set_subline_offset(int subline, double offset) {
        if (subline >= count_sublines())
          throw std::invalid_argument("bad subline");
        _middle_offsets[subline] = offset;
      }
    };

    LineInfo _linfo;
    bool _change_pending;
    bool _updating;

    virtual std::vector<base::Point> get_points_for_subline(int subline);

    virtual void connector_changed(Connector *conn);

    virtual bool update_start_point();
    virtual bool update_end_point();

    virtual std::vector<ItemHandle *> create_handles(Line *line, InteractionLayer *ilayer);
    virtual void update_handles(Line *line, std::vector<ItemHandle *> &handles);

    virtual bool handle_dragged(Line *line, ItemHandle *handle, const base::Point &pos, bool dragging);

    double angle_of_intersection_with_rect(const base::Rect &rect, const base::Point &p);
  };

} // end of mdc namespace

#endif /* _MDC_ORTHOGONAL_LINE_LAYOUTER_H_ */
