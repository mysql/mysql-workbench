/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _MDC_STRAIGHT_LINE_LAYOUTER_H_
#define _MDC_STRAIGHT_LINE_LAYOUTER_H_

#include "mdc_line.h"

namespace mdc {

  class Connector;

  // Just a straight line from starting point to the end.
  class MYSQLCANVAS_PUBLIC_FUNC StraightLineLayouter : public LineLayouter {
  public:
    StraightLineLayouter(Connector *sconn, Connector *econn);
    virtual ~StraightLineLayouter();

    virtual Connector *get_start_connector() const {
      return _start_conn;
    }
    virtual Connector *get_end_connector() const {
      return _end_conn;
    }

    virtual std::vector<base::Point> get_points();
    virtual base::Point get_start_point();
    virtual base::Point get_end_point();

    virtual void update();

  protected:
    Connector *_start_conn;
    Connector *_end_conn;

    base::Point _start;
    base::Point _end;

    virtual void connector_changed(Connector *conn);
  };

#if 0
// A looping/cycle line
class MYSQLCANVAS_PUBLIC_FUNC LoopingLineLayouter : public LineLayouter
{
};
#endif

} // end of mdc namespace

#endif /* _MDC_STRAIGHT_LINE_LAYOUTER_H_ */
