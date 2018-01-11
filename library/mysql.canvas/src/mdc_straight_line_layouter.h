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
