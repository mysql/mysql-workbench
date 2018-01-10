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

#ifndef _RELATIONSHIP_CANVAS_FLOATER_H_
#define _RELATIONSHIP_CANVAS_FLOATER_H_

#include "canvas_floater.h"

namespace wb {
  class ModelDiagramForm;

  class RelationshipFloater : public Floater {
  public:
    RelationshipFloater(ModelDiagramForm *view);
    virtual ~RelationshipFloater();

    void add_column(const std::string &name);

    boost::signals2::signal<void()> *signal_done_clicked() {
      return _button.signal_activate();
    }

    void setup_pick_target();

    void pick_next_target();

  private:
    mdc::Box _columns_box;
    mdc::TextFigure _text;
    Button _button;
    std::vector<mdc::TextFigure *> _columns;
    unsigned int _current_column;

    void setup_pick_source();
  };
};

#endif
