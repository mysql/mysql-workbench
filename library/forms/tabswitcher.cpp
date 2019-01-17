/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/mforms.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("mforms");

#define SIDE_PADDING 12
#define TAB_SIDE_PADDING 10
#define TAB_TEXT_SPACING 8

#ifdef __APPLE__
#define TAB_FONT "Lucida Grande"
#elif _MSC_VER
#define TAB_FONT "Tahoma"
#else
#define TAB_FONT "Helvetica"
#endif
#define TITLE_FONT_SIZE 12
#define SUB_TITLE_FONT_SIZE 9
#define TITLE_TEXT_SPACING 4

#define ICON_WIDTH 32
#define ICON_HEIGHT 32

#define INITIAL_TAB_HEIGHT 58

#ifdef _MSC_VER
#define VERTICAL_STYLE_WIDTH 58
#define VERTICAL_STYLE_HEIGHT 68
#else
#define VERTICAL_STYLE_WIDTH 64
#define VERTICAL_STYLE_HEIGHT 70
#endif

#ifndef _MSC_VER
using std::max;
#endif

using namespace base;
using namespace mforms;

const int NEXT_BUTTON_INDEX = -2;
const int PREV_BUTTON_INDEX = -3;

//--------------------------------------------------------------------------------------------------

class mforms::TabSwitcherPimpl {
public:
  class TabItem : public Accessible {
  public:
    std::string title;
    std::string sub_title;
    cairo_surface_t *icon;
    cairo_surface_t *alt_icon;
    base::Rect accBounds;   // Bounds to be used for accessibility
    std::function<void(int x, int y)> actionCallback;

    TabItem(const std::function<void(int x, int y)> &clickCallback) : icon(0), alt_icon(0) {
      actionCallback = clickCallback;
    }
    ~TabItem() {
      if (icon)
        cairo_surface_destroy(icon);
      if (alt_icon)
        cairo_surface_destroy(alt_icon);
    }

    // ------ Accesibility Methods -----
    virtual std::string getAccessibilityDescription() override {
      return title;
    }

    virtual Accessible::Role getAccessibilityRole() override {
      return Accessible::ListItem;
    }

    virtual base::Rect getAccessibilityBounds() override {
      return accBounds;
    }

    virtual std::string getAccessibilityDefaultAction() override {
      return "Switch view";
    }

    virtual void accessibilityDoDefaultAction() override {
      actionCallback((int)accBounds.center().x, (int)accBounds.center().y);
    }
  };

protected:
  TabSwitcher *_owner;
  std::vector<TabItem *> _items;

  int _selected;
  int _last_clicked;


  TabSwitcherPimpl(TabSwitcher *owner) : _owner(owner), _selected(-1), _last_clicked(-1) {
  }

public:
  virtual ~TabSwitcherPimpl() {
    for (std::vector<TabItem *>::iterator iter = _items.begin(); iter != _items.end(); ++iter)
      delete *iter;
  }

  virtual bool set_collapsed(bool flag) {
    return false;
  }

  virtual bool get_collapsed() = 0;

  void set_selected(int index) {
    _selected = index;
  }

  int get_selected() {
    return _selected;
  }

  std::size_t getItemCount() {
    return _items.size();
  }

  TabItem *getItem(int index) {
    try {
      return _items[index];
    } catch (...) {
      return nullptr;
    }
  }

  virtual void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) = 0;

  virtual void set_icon(int index, const std::string &icon_path, const std::string &alt_icon_path) {
    if (index >= 0 && index < (int)_items.size()) {
      TabItem *item = _items[index];

      if (item->icon != NULL)
        cairo_surface_destroy(item->icon);
      item->icon = Utilities::load_icon(icon_path, true);

      if (item->alt_icon != NULL)
        cairo_surface_destroy(item->alt_icon);
      item->alt_icon = Utilities::load_icon(alt_icon_path, true);
    }
  }

  virtual int add_item(const std::string &title, const std::string &sub_title, const std::string &icon_path,
                       const std::string &alt_icon_path) {

    TabItem *item = new TabItem([&](int x, int y) {
      if (_owner != nullptr) {
        int index = index_from_point(x, y);
        if (index != -1) {
          _owner->set_selected(index);
          (*_owner->signal_changed())();
        }

      }

    });

    item->title = title;
    item->sub_title = sub_title;
    item->icon = Utilities::load_icon(icon_path, true);
    item->alt_icon = Utilities::load_icon(alt_icon_path, true);

    _items.push_back(item);

    if (_selected == -1)
      set_selected((int)_items.size() - 1);

    return (int)_items.size() - 1;
  }

  virtual void remove_item(int index) {
    delete _items[index];
    _items.erase(_items.begin() + index);
  }

  virtual int index_from_point(int x, int y) = 0;

  virtual bool go_back() {
    return false;
  }
  virtual bool go_next() {
    return false;
  }
};

class VerticalTabSwitcher : public mforms::TabSwitcherPimpl {
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

  base::Color _colors[TabLastElement];
  cairo_surface_t *_selection_image;
  cairo_surface_t *_up_arrow;
  cairo_surface_t *_down_arrow;
  int _up_arrow_y;
  int _down_arrow_y;
  int _first_visible;
  int _last_visible;
  bool _collapsed;

public:
  VerticalTabSwitcher(TabSwitcher *owner)
    : TabSwitcherPimpl(owner), _up_arrow_y(0), _down_arrow_y(0), _first_visible(0), _last_visible(0), _collapsed(false) {
#ifdef _MSC_VER
    if (base::Color::get_active_scheme() != base::ColorSchemeStandardWin7) {
      _colors[TabInactiveBackground] = base::Color::getApplicationColor(base::AppColorPanelHeader, false);
      _colors[TabInactiveForeground] = base::Color::getApplicationColor(base::AppColorPanelHeader, true);
      _colors[TabActiveBackground] = base::Color::getApplicationColor(base::AppColorPanelHeaderFocused, false);
      _colors[TabActiveForeground] = base::Color::getApplicationColor(base::AppColorPanelHeaderFocused, true);
    } else {
      _colors[TabInactiveBackground] = base::Color::getApplicationColor(base::AppColorPanelHeader, false);
      _colors[TabInactiveForeground] = base::Color::getApplicationColor(base::AppColorPanelHeader, true);
      _colors[TabActiveBackground] = base::Color::getApplicationColor(base::AppColorMainBackground, false);
      _colors[TabActiveForeground] = base::Color::getApplicationColor(base::AppColorPanelHeader, true);
    }
#else
    _colors[TabInactiveBackground] = Color(0x48 / 255.0, 0x48 / 255.0, 0x48 / 255.0, 1);
    _colors[TabActiveBackground] = Color(0x26 / 255.0, 0x26 / 255.0, 0x26 / 255.0, 1);
    _colors[TabActiveForeground] = Color(1, 1, 1, 1);
    _colors[TabInactiveForeground] = Color(0.6, 0.6, 0.6, 1);
#endif
    _up_arrow = NULL;
    _down_arrow = NULL;
    _selection_image = Utilities::load_icon("output_type-item_selected.png", true);
  }

  virtual ~VerticalTabSwitcher() {
    if (_up_arrow)
      cairo_surface_destroy(_up_arrow);
    if (_down_arrow)
      cairo_surface_destroy(_down_arrow);
    if (_selection_image)
      cairo_surface_destroy(_selection_image);
  }

  virtual void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
    Color color;

    cairo_save(cr);

    color = _colors[TabInactiveBackground];
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1);
    cairo_paint(cr);

    const int font_size = 10;

    cairo_select_font_face(cr, TAB_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, font_size);

    if (_first_visible > 0) {
      int items_that_fit = _owner->get_height() / VERTICAL_STYLE_HEIGHT;

      if (_first_visible + items_that_fit > (int)_items.size()) {
        _first_visible = (int)_items.size() - items_that_fit;
        if (_first_visible < 0)
          _first_visible = 0;
      }
    }

    double y = 0;
    double iy = 0;
    int ii = 0;

    _last_visible = _first_visible;
    for (std::vector<TabItem *>::iterator i = _items.begin(); i != _items.end(); ++i, ++ii) {


      base::Size size = Utilities::getImageSize((*i)->icon);
      (*i)->accBounds.size = size;
      (*i)->accBounds.pos.x = (VERTICAL_STYLE_WIDTH - (VERTICAL_STYLE_WIDTH / 64.0) * size.width) / 2;

      if (ii < _first_visible) {
        (*i)->accBounds.pos.y = ((VERTICAL_STYLE_WIDTH - size.height) / 2 - font_size) - (_first_visible - ii) * VERTICAL_STYLE_HEIGHT;
        continue;
      }

      (*i)->accBounds.pos.y = iy + (VERTICAL_STYLE_WIDTH - size.height) / 2 - font_size;

      if (y + VERTICAL_STYLE_HEIGHT > _owner->get_height()) {
        iy += VERTICAL_STYLE_HEIGHT;
        continue;
      }

      _last_visible = ii;




      if (_selected == ii) {
        color = _colors[TabActiveBackground];
        cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1);
        cairo_rectangle(cr, 0, y, VERTICAL_STYLE_WIDTH, VERTICAL_STYLE_HEIGHT);
        cairo_fill(cr);

        Utilities::paint_icon(cr, _selection_image, 0, iy + (VERTICAL_STYLE_WIDTH - size.width) / 2 + size.height / 2);
      }

      Utilities::paint_icon(cr, (*i)->icon, (*i)->accBounds.pos.x, (*i)->accBounds.pos.y,
                                                    _selected == ii ? 1.0f : 0.4f);

      if (_selected == ii) {
        color = _colors[TabActiveForeground];
      } else {
        color = _colors[TabInactiveForeground];
      }

      cairo_set_source_rgba(cr, color.red, color.green, color.blue, 1);

      std::string::size_type p = (*i)->title.find('\n');
      if (p == std::string::npos) {
        cairo_text_extents_t ext;
        cairo_text_extents(cr, (*i)->title.c_str(), &ext);

        cairo_move_to(cr, (VERTICAL_STYLE_WIDTH - ext.width) / 2, y + size.height);
        cairo_show_text(cr, (*i)->title.c_str());
      } else {
        std::string l1 = (*i)->title.substr(0, p);
        std::string l2 = (*i)->title.substr(p + 1);
        cairo_text_extents_t ext1, ext2;
        cairo_text_extents(cr, l1.c_str(), &ext1);
        cairo_text_extents(cr, l2.c_str(), &ext2);

        cairo_move_to(cr, (VERTICAL_STYLE_WIDTH - ext1.width) / 2,
                      y + size.height + 4 - (font_size + ext1.y_bearing) + (VERTICAL_STYLE_WIDTH - size.height) / 2);
        cairo_show_text(cr, l1.c_str());
        cairo_stroke(cr);
        cairo_move_to(
          cr, (VERTICAL_STYLE_WIDTH - ext2.width) / 2,
          y + size.height + 4 + font_size - (font_size + ext2.y_bearing) + (VERTICAL_STYLE_WIDTH - size.height) / 2);
        cairo_show_text(cr, l2.c_str());
        cairo_stroke(cr);
      }
      y += VERTICAL_STYLE_HEIGHT;
      iy += VERTICAL_STYLE_HEIGHT;
    }

    if (_first_visible > 0 || _last_visible < (int)_items.size() - 1) {
      if (!_up_arrow)
        _up_arrow = mforms::Utilities::load_icon("arrow_up.png");

      if (!_down_arrow)
        _down_arrow = mforms::Utilities::load_icon("arrow_down.png");

      if (_up_arrow && _down_arrow) {
        int w = cairo_image_surface_get_width(_up_arrow);
        int bottom = _owner->get_height() - 4;
        int up_h = cairo_image_surface_get_height(_up_arrow);
        int down_h = cairo_image_surface_get_height(_down_arrow);

        _up_arrow_y = bottom - up_h - 4 - down_h;

        cairo_set_source_surface(cr, _up_arrow, (VERTICAL_STYLE_WIDTH - w) / 2, _up_arrow_y);
        if (_first_visible > 0)
          cairo_paint(cr);
        else
          cairo_paint_with_alpha(cr, 0.4);

        _down_arrow_y = bottom - down_h;
        cairo_set_source_surface(cr, _down_arrow, (VERTICAL_STYLE_WIDTH - w) / 2, _down_arrow_y);
        if (_last_visible < (int)_items.size() - 1)
          cairo_paint(cr);
        else
          cairo_paint_with_alpha(cr, 0.4);
      } else
        logError("Could not load arrow_up/down.png\n");
    } else {
      _up_arrow_y = 0;
      _down_arrow_y = 0;
    }

    cairo_restore(cr);
  }

  virtual int index_from_point(int x, int y) {
    if (_items.size() == 0 || x < 0 || x > _owner->get_width() || y < 0 || y > _owner->get_height())
      return -1;

    if ((_first_visible > 0 || _last_visible < (int)_items.size() - 1) && y > _up_arrow_y) {
      if (y < _down_arrow_y)
        return PREV_BUTTON_INDEX;
      else
        return NEXT_BUTTON_INDEX;
    }

    for (size_t i = 0; i < _items.size(); i++) {
      if (y < (int)(i + 1) * VERTICAL_STYLE_HEIGHT)
        return (int)i + _first_visible;
    }
    return -1;
  }

  virtual bool set_collapsed(bool flag) {
    _collapsed = flag;
    return true;
  }

  virtual bool get_collapsed() {
    return _collapsed;
  }

  virtual bool go_back() {
    if (_first_visible > 0) {
      _first_visible--;
      _owner->set_selected(get_selected() - 1);
      return true;
    }
    return false;
  }

  virtual bool go_next() {
    if (_last_visible < (int)_items.size() - 1) {
      _first_visible++;
      _owner->set_selected(get_selected() + 1);
      return true;
    }
    return false;
  }
};

//--------------------------------------------------------------------------------------------------

TabSwitcher::TabSwitcher(TabSwitcherType type) : _tabView(0), _last_clicked(-1), _needs_relayout(true), _was_collapsed(false) {
  _timeout = 0;
  switch (type) {
    case VerticalIconSwitcher:
      _pimpl = new VerticalTabSwitcher(this);
      set_size(VERTICAL_STYLE_WIDTH, -1);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

TabSwitcher::~TabSwitcher() {
  if (_timeout)
    mforms::Utilities::cancel_timeout(_timeout);

  delete _pimpl;
}

//--------------------------------------------------------------------------------------------------

int TabSwitcher::get_preferred_height() {
  return INITIAL_TAB_HEIGHT;
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::attach_to_tabview(TabView *tabView) {
  _tabView = tabView;
  set_needs_relayout();

  scoped_connect(_tabView->signal_tab_changed(), std::bind(&TabSwitcher::tab_changed, this));
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::set_collapsed(bool flag) {
  if (_pimpl->get_collapsed() != flag) {
    if (_pimpl->set_collapsed(flag)) {
      set_size(_pimpl->get_collapsed() ? 5 : VERTICAL_STYLE_WIDTH, -1);
      _signal_collapse_changed();
    }
    set_layout_dirty();
  }
}

//--------------------------------------------------------------------------------------------------
bool TabSwitcher::get_collapsed() {
  return _pimpl->get_collapsed();
}

//--------------------------------------------------------------------------------------------------

int TabSwitcher::add_item(const std::string &title, const std::string &sub_title, const std::string &icon_path,
                          const std::string &alt_icon_path) {
  int i = _pimpl->add_item(title, sub_title, icon_path, alt_icon_path);
  set_needs_relayout();
  return i;
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::remove_item(int index) {
  _pimpl->remove_item(index);
}

//--------------------------------------------------------------------------------------------------

/**
 * Replaces the icon pair for the given item with the new values.
 */
void TabSwitcher::set_icon(int index, const std::string &icon_path, const std::string &alt_icon_path) {
  _pimpl->set_icon(index, icon_path, alt_icon_path);
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::set_selected(int index) {
  _pimpl->set_selected(index);
  if (_tabView != NULL)
    _tabView->set_active_tab(index);
  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------
int TabSwitcher::get_selected() {
  return _pimpl->get_selected();
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::set_needs_relayout() {
  _needs_relayout = true;
  set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::set_layout_dirty(bool value) {
  DrawBox::set_layout_dirty(true);
  set_needs_relayout();
}

//--------------------------------------------------------------------------------------------------

bool TabSwitcher::mouse_down(mforms::MouseButton button, int x, int y) {
  if (!DrawBox::mouse_down(button, x, y)) {
    // For now ignore which button was pressed.
    _last_clicked = _pimpl->index_from_point(x, y);
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

bool TabSwitcher::mouse_up(mforms::MouseButton button, int x, int y) {
  return DrawBox::mouse_up(button, x, y);
}

//--------------------------------------------------------------------------------------------------

bool TabSwitcher::mouse_click(mforms::MouseButton button, int x, int y) {
  bool handled = DrawBox::mouse_click(button, x, y);

  // Don't change anything if the user clicked outside of any tab.
  if (!handled && _last_clicked == _pimpl->index_from_point(x, y)) {
    if (_last_clicked > -1) {
      set_selected(_last_clicked);
      _signal_changed();
      handled = true;
    } else if (_last_clicked == PREV_BUTTON_INDEX) {
      if (_pimpl->go_back()) {
        set_needs_repaint();
        _signal_changed();
        handled = true;
      }
    } else if (_last_clicked == NEXT_BUTTON_INDEX) {
      if (_pimpl->go_next()) {
        set_needs_repaint();
        _signal_changed();
        handled = true;
      }
    }
  }
  return handled;
}

//--------------------------------------------------------------------------------------------------

bool TabSwitcher::mouse_enter() {
  if (DrawBox::mouse_enter())
    return true;

  _was_collapsed = _pimpl->get_collapsed();
  if (_was_collapsed)
    set_collapsed(false);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool TabSwitcher::collapse() {
  _timeout = 0;
  set_collapsed(true);
  return false;
}

//--------------------------------------------------------------------------------------------------

bool TabSwitcher::mouse_leave() {
  if (DrawBox::mouse_leave())
    return true;

  if (_was_collapsed) {
    _was_collapsed = false;
    _timeout = mforms::Utilities::add_timeout(0.3f, std::bind(&TabSwitcher::collapse, this));
  }
  return true;
}

//--------------------------------------------------------------------------------------------------

size_t TabSwitcher::getAccessibilityChildCount() {
  return _pimpl->getItemCount();
}

//--------------------------------------------------------------------------------------------------

Accessible *TabSwitcher::getAccessibilityChild(size_t index) {
  return dynamic_cast<Accessible*>(_pimpl->getItem(static_cast<int>(index)));
}

//--------------------------------------------------------------------------------------------------

Accessible::Role TabSwitcher::getAccessibilityRole() {
  return Accessible::List;
}

//--------------------------------------------------------------------------------------------------

base::Accessible *TabSwitcher::accessibilityHitTest(ssize_t x, ssize_t y) {
  int idx = _pimpl->index_from_point(static_cast<int>(x), static_cast<int>(y));
  if (idx == -1)
    return nullptr;
  else
    return _pimpl->getItem(idx);
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
  _pimpl->repaint(cr, areax, areay, areaw, areah);
}

//--------------------------------------------------------------------------------------------------

void TabSwitcher::tab_changed() {
  _pimpl->set_selected(_tabView->get_active_tab());
  set_needs_repaint();
}
