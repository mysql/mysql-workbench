/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include <mforms/drawbox.h>
#include <vector>

#include <boost/signals2/signal.hpp>
#include "grt/tree_model.h"

class Snippet;
class DbSqlEditorSnippets;

namespace mforms {
  class Menu;
};

class BaseSnippetList : public mforms::DrawBox {
protected:
  bec::ListModel* _model; // The model containing the actual snippet data.

  cairo_surface_t* _image; // The icon for all entries.
  std::vector<Snippet*> _snippets;
  int _last_width;   // The last actual width we used for layouting.
  int _layout_width; // This is rather a minimal width.
  int _layout_height;
  int _left_spacing;
  int _top_spacing;
  int _right_spacing;
  int _bottom_spacing;

  bool _single_click;
  int _selected_index;

  Snippet* _hot_snippet;
  Snippet* _selected_snippet;
  mforms::MouseButton _last_mouse_button;

  mforms::Menu* _context_menu;

  boost::signals2::signal<void()> _selection_changed_signal;

protected:
  int find_selected_index();

  void set_selected(Snippet* snippet);
  Snippet* snippet_from_point(double x, double y);

  virtual void repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) override;
  void layout();
  virtual base::Size getLayoutSize(base::Size proposedSize) override;
  Snippet* selected();
  virtual bool mouse_leave() override;
  virtual bool mouse_move(mforms::MouseButton button, int x, int y) override;
  virtual bool mouse_down(mforms::MouseButton button, int x, int y) override;
  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y) override;
  virtual bool mouse_click(mforms::MouseButton button, int x, int y) override;

public:
  BaseSnippetList(const std::string& icon_name, bec::ListModel* model);
  ~BaseSnippetList();
  boost::signals2::signal<void()>* signal_selection_changed() {
    return &_selection_changed_signal;
  }

  void clear();
  void refresh_snippets();
  int selected_index();

  void set_snippet_info(Snippet* snippet, const std::string& title, const std::string& subtitle);
  void get_snippet_info(Snippet* snippet, std::string& title, std::string& subtitle);
  base::Rect snippet_bounds(Snippet* snippet);

  // ------ Accesibility Methods -----
  virtual std::string get_acc_name() override {
    return get_name();
  }
  virtual mforms::Accessible::Role get_acc_role() override {
    return mforms::Accessible::List;
  }
  virtual int get_acc_child_count() override;
  virtual Accessible* get_acc_child(int index) override;
  virtual mforms::Accessible* hit_test(int x, int y) override;
};
