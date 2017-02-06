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

#pragma once

#include <vector>

#include "cairo/cairo.h"

#include "base/drawing.h"
#include "mforms/base.h"
#include "mforms/drawbox.h"
#include "mforms/tabview.h"

namespace mforms {
  enum TabSwitcherType { VerticalIconSwitcher };

  enum TabElementPart {
    TabActiveBackground = 0,
    TabInactiveBackground,
    TabActiveForeground,
    TabInactiveForeground,

    TabMainCaption,
    TabSubCaption,
    TabLineColor,
    TabLastElement
  };

  class TabSwitcherPimpl;

  class MFORMS_EXPORT TabSwitcher : public DrawBox {
  public:
    TabSwitcher(TabSwitcherType type);
    virtual ~TabSwitcher();

    void attach_to_tabview(TabView *tabView);

    int add_item(const std::string &title, const std::string &sub_title, const std::string &icon_path,
                 const std::string &alt_icon_path);
    void remove_item(int index);

    void set_icon(int index, const std::string &icon_path, const std::string &alt_icon_path);

    void set_selected(int index);
    int get_selected();
    void set_needs_relayout();

    void set_collapsed(bool flag);
    bool get_collapsed();

#ifndef SWIG
    boost::signals2::signal<void()> *signal_changed() {
      return &_signal_changed;
    };
    boost::signals2::signal<void()> *signal_collapse_changed() {
      return &_signal_collapse_changed;
    };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    virtual int get_preferred_height();
    virtual void set_layout_dirty(bool value = true);

    virtual void repaint(cairo_t *cr, int x, int y, int w, int h);
    virtual bool mouse_down(mforms::MouseButton button, int x, int y);
    virtual bool mouse_up(mforms::MouseButton button, int x, int y);
    virtual bool mouse_click(mforms::MouseButton button, int x, int y);
    virtual bool mouse_enter();
    virtual bool mouse_leave();
#endif
#endif
  private:
    TabSwitcherPimpl *_pimpl;
    boost::signals2::signal<void()> _signal_changed;
    boost::signals2::signal<void()> _signal_collapse_changed;
    TabView *_tabView;
    TimeoutHandle _timeout;

    int _last_clicked;
    bool _needs_relayout;
    bool _was_collapsed;

    void tab_changed();

    bool collapse();
  };
};
