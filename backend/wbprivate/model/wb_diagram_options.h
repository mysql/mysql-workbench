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

#ifndef _WB_DIAGRAM_OPTIONS_H_
#define _WB_DIAGRAM_OPTIONS_H_

#include "mdc.h"

#include "workbench/wb_backend_public_interface.h"

#include "grts/structs.model.h"

#include "base/trackable.h"

namespace wb {
  class WBContext;

  class MYSQLWBBACKEND_PUBLIC_FUNC DiagramOptionsBE : public base::trackable {
    friend class SizerFigure;

    mdc::CanvasView *_view;
    model_DiagramRef _target_view;
    class SizerFigure *_sizer;
    std::string _name;

    boost::signals2::signal<void()> _changed_signal;

    void get_min_size_in_pages(int &xc, int &yc);

  public:
    DiagramOptionsBE(mdc::CanvasView *view, model_DiagramRef target_view, WBContext *wb);
    ~DiagramOptionsBE();

    void update_size();

    std::string get_name();
    void set_name(const std::string &name);

    int get_xpages();
    int get_ypages();
    void set_xpages(int c);
    void set_ypages(int c);

    void get_max_page_counts(int &max_xpages, int &max_ypages);

    void commit();

    boost::signals2::signal<void()> *signal_changed() {
      return &_changed_signal;
    }
  };
};

#endif //  _WB_DIAGRAM_OPTIONS_H_
