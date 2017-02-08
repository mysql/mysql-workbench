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

#include "snippet_list.h"
#include "snippet_popover.h"

#include "base/geometry.h"
#include "base/notifications.h"
#include "base/drawing.h"
#include "base/string_utilities.h"

#include "mforms/utilities.h"
#include "mforms/menu.h"

#include "grt/tree_model.h"

using namespace mforms;
using namespace base;

#ifdef __APPLE__
#define SNIPPET_DETAILS_FONT "Lucida Grande"
#define SNIPPET_NORMAL_FONT "Tahoma"
#elif _WIN32
#define SNIPPET_DETAILS_FONT "Arial"
#define SNIPPET_NORMAL_FONT "Tahoma"
#else
#define SNIPPET_DETAILS_FONT "Helvetica"
#define SNIPPET_NORMAL_FONT "Tahoma"
#endif

#define SNIPPET_NORMAL_FONT_SIZE 11
#define SNIPPET_DETAILS_FONT_SIZE 10

//--------------------------------------------------------------------------------------------------
// TODO: move this to a central cairo helper class.
static int image_width(cairo_surface_t* image) {
  if (image != NULL)
    return cairo_image_surface_get_width(image);
  return 0;
}

//--------------------------------------------------------------------------------------------------

static int image_height(cairo_surface_t* image) {
  if (image != NULL)
    return cairo_image_surface_get_height(image);
  return 0;
}

//----------------- Snippet ------------------------------------------------------------------------

class Snippet : public mforms::Accessible {
private:
  cairo_surface_t* _icon;
  std::string _title;
  std::string _description;
  std::string _shortened_title; // Contains the description shortened and with ellipses if the
  // full title doesn't fit into the available space.
  std::string _shortened_description; // Ditto for the description
  double _last_text_width;            // The last width for which the shortened description has been
  // computed. Used to avoid unnecessary re-computation.
  double _title_offset;       // Vertical position of the title.
  double _description_offset; // Ditto for description.
  double _title_width;        // Width of the (possibly shortened) title. For text decoration.
  base::Rect _bounds;         // The link's bounds when it was drawn the last time.
  int _text_height;
  bool _enabled; // Draw the button in enabled or disabled state.

public:
  Snippet(cairo_surface_t* icon, const std::string& title, const std::string& description, bool enabled) {
    _icon = (icon != NULL) ? cairo_surface_reference(icon) : NULL;
    _title = title;
    _description = description;
    _last_text_width = 0;

    _title_offset = 0;
    _description_offset = 0;
    _title_width = 0;
    _text_height = 0;
    _enabled = enabled;
  }

  //------------------------------------------------------------------------------------------------

  virtual ~Snippet() {
    if (_icon != NULL)
      cairo_surface_destroy(_icon);
  }

//------------------------------------------------------------------------------------------------

#define SNIPPET_PADDING 2      // Left and right padding.
#define SNIPPET_ICON_SPACING 4 // Horizontal distance between icon and text.
#define SNIPPET_TEXT_SPACING 4 // Vertical distance between title and description.

  void layout(cairo_t* cr) {
    // Re-compute shortened title and its position.
    cairo_select_font_face(cr, SNIPPET_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, SNIPPET_NORMAL_FONT_SIZE);

    _shortened_title = Utilities::shorten_string(cr, _title, _last_text_width);

    cairo_text_extents_t title_extents;
    cairo_text_extents(cr, _shortened_title.c_str(), &title_extents);
    _title_offset = (int)-title_extents.y_bearing + 2;
    _title_width = title_extents.width;

    // Same for the description.
    cairo_select_font_face(cr, SNIPPET_DETAILS_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, SNIPPET_DETAILS_FONT_SIZE);

    // Compress all consecutive non-printable chars into a single one and convert this then
    // into a space char for display.
    static std::string non_printable_chars;
    if (non_printable_chars.empty()) {
      for (int i = 1; i < ' '; i++)
        non_printable_chars.push_back(i);
    }

    _shortened_description = _description;
    for (size_t next = _shortened_description.find_first_of(non_printable_chars); next != std::string::npos;
         next = _shortened_description.find_first_of(non_printable_chars, next)) {
      _shortened_description[next] = 1; // A placeholder.
      next++;
    }

    std::string single_placeholder(1, 1);
    std::string double_placeholder(2, 1);
    do {
      size_t position = _shortened_description.find(double_placeholder);
      if (position != std::string::npos)
        _shortened_description.replace(position, 2, single_placeholder);
      else {
        // We are done. Do the final replace and go out of the loop.
        base::replaceStringInplace(_shortened_description, single_placeholder, " ");
        break;
      }
    } while (true);

    _shortened_description = Utilities::shorten_string(cr, _shortened_description, _last_text_width);

    cairo_text_extents_t description_extents;
    cairo_text_extents(cr, _shortened_description.c_str(), &description_extents);
    _description_offset = _title_offset - (int)description_extents.y_bearing + SNIPPET_TEXT_SPACING;

    // Determine overall text height. This is used to center the text during paint.
    _text_height = (int)ceil(title_extents.height + description_extents.height + SNIPPET_TEXT_SPACING);
  }

  virtual void paint(cairo_t* cr, base::Rect bounds, bool hot, bool active) {
    _bounds = bounds;
    cairo_save(cr);

    double icon_width = image_width(_icon);
    double icon_height = image_height(_icon);

    double new_width = bounds.size.width - 2 * SNIPPET_PADDING - icon_width - SNIPPET_ICON_SPACING;

    if (new_width != _last_text_width) {
      _last_text_width = new_width;
      layout(cr);
    }

    cairo_set_line_width(cr, 1);

    // Fill a blue background if the item is active.
    if (active && _enabled) {
      cairo_set_source_rgb(cr, 0x5a / 255.0, 0x85 / 255.0, 0xdc / 255.0);
      cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.size.width, bounds.size.height);
      cairo_fill(cr);
    } else {
      // If not selected fill with the default background and draw a white separator line.
      cairo_set_source_rgb(cr, 0xf2 / 255.0, 0xf2 / 255.0, 0xf2 / 255.0);
      cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.size.width, bounds.size.height);
      cairo_fill(cr);

      cairo_set_source_rgb(cr, 1, 1, 1);
      cairo_move_to(cr, bounds.left(), bounds.bottom());
      cairo_line_to(cr, bounds.right(), bounds.bottom());
      cairo_stroke(cr);
    }

    if (_icon) {
      cairo_set_source_surface(cr, _icon, bounds.left() + SNIPPET_PADDING,
                               bounds.top() + (int)ceil((bounds.height() - icon_height) / 2));
      if (_enabled)
        cairo_paint(cr);
      else
        cairo_paint_with_alpha(cr, 0.25);
    }

    int text_offset = (int)(bounds.height() - _text_height) / 2;
    cairo_select_font_face(cr, SNIPPET_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, SNIPPET_NORMAL_FONT_SIZE);

    if (!_enabled)
      cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
    else if (active)
      cairo_set_source_rgb(cr, 1, 1, 1);
    else if (hot)
      cairo_set_source_rgb(cr, 90 / 255.0, 147 / 255.0, 220 / 255.0);
    else
      cairo_set_source_rgb(cr, 34 / 255.0, 34 / 255.0, 34 / 255.0);
    double offset = bounds.left() + SNIPPET_PADDING + ((icon_width > 0) ? icon_width + SNIPPET_ICON_SPACING : 0);

    cairo_move_to(cr, offset, bounds.top() + _title_offset + text_offset);
    cairo_show_text(cr, _shortened_title.c_str());
    cairo_stroke(cr);

    cairo_select_font_face(cr, SNIPPET_DETAILS_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, SNIPPET_DETAILS_FONT_SIZE);

    if (!_enabled)
      cairo_set_source_rgb(cr, 0.75, 0.75, 0.75);
    else if (active)
      cairo_set_source_rgb(cr, 1, 1, 1);
    else
      cairo_set_source_rgb(cr, 182 / 255.0, 182 / 255.0, 182 / 255.0);

    cairo_move_to(cr, offset, bounds.top() + _description_offset + text_offset);
    cairo_show_text(cr, _shortened_description.c_str());
    cairo_stroke(cr);

    // Hot style implementation is currently done manually (draw a line).
    // TODO: replace this with font decoration once pango is incorporated.
    if (hot && _enabled) {
      cairo_set_source_rgb(cr, 90 / 255.0, 147 / 255.0, 220 / 255.0);
      cairo_move_to(cr, offset, bounds.top() + _title_offset + text_offset + 1.5);
      cairo_line_to(cr, offset + _title_width, bounds.top() + _title_offset + text_offset + 1.5);
      cairo_stroke(cr);
    }

    cairo_restore(cr);
  }

  //------------------------------------------------------------------------------------------------

  bool contains(double x, double y) {
    return _bounds.contains(x, y);
  }

  //------------------------------------------------------------------------------------------------

  bool enabled() {
    return _enabled;
  };

  //------------------------------------------------------------------------------------------------

  base::Rect bounds() {
    return _bounds;
  };

  //------------------------------------------------------------------------------------------------

  std::string title() {
    return _title;
  };

  //------------------------------------------------------------------------------------------------

  void title(const std::string& text) {
    _title = text;
    _last_text_width = 0;
  };

  //------------------------------------------------------------------------------------------------

  std::string description() {
    return _description;
  };

  //------------------------------------------------------------------------------------------------

  void description(const std::string& text) {
    _description = text;
    _last_text_width = 0;
  };

  //------------------------------------------------------------------------------------------------

  // ------ Accesibility Methods -----
  virtual std::string get_acc_name() {
    return _title;
  }
  virtual mforms::Accessible::Role get_acc_role() {
    return mforms::Accessible::ListItem;
  }
  virtual base::Rect get_acc_bounds() {
    return _bounds;
  }

  // End of internal class Snippets.
};

//----------------- SnippetList --------------------------------------------------------------------

int BaseSnippetList::find_selected_index() {
  // Rather unlikely the selected link is not in the vector (since we got it from there) but...
  std::vector<Snippet*>::iterator location = std::find(_snippets.begin(), _snippets.end(), _selected_snippet);
  return (location == _snippets.end()) ? -1 : (int)(location - _snippets.begin());
}

//------------------------------------------------------------------------------------------------

BaseSnippetList::BaseSnippetList(const std::string& icon_name, bec::ListModel* model) : _model(model) {
  // Not sure we need that spacing, so I leave it that way for now.
  _left_spacing = 0;
  _top_spacing = 0;
  _right_spacing = 3;
  _bottom_spacing = 0;

  _hot_snippet = 0;

  _single_click = false;
  _selected_snippet = 0;
  _selected_index = -1;
  _last_mouse_button = MouseButtonNone;

  _image = Utilities::load_icon(icon_name);
}

//------------------------------------------------------------------------------------------------

BaseSnippetList::~BaseSnippetList() {
  clear();
  if (_image != NULL)
    cairo_surface_destroy(_image);
}

//------------------------------------------------------------------------------------------------

void BaseSnippetList::clear() {
  _selected_snippet = 0;
  _selected_index = -1;
  for (std::vector<Snippet*>::iterator iterator = _snippets.begin(); iterator != _snippets.end(); iterator++)
    delete *iterator;
  _snippets.clear();

  if (!is_destroying())
    set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void BaseSnippetList::refresh_snippets() {
  clear();
  for (size_t i = 0; i < _model->count(); i++) {
    std::string caption;
    _model->get_field(bec::NodeId(i), 0, caption);
    std::string description;
    bool skip_image = false;
    if (!_model->get_field(bec::NodeId(i), 1, description))
      skip_image = true;

    Snippet* snippet = new Snippet(skip_image ? NULL : _image, caption, description, true);
    _snippets.push_back(snippet);
  }
  set_layout_dirty(true);
  relayout();
  _selection_changed_signal();
}

//------------------------------------------------------------------------------------------------

#define SNIPPET_HEIGHT 34 // The height of a single action link.
#define SNIPPET_SPACING 0 // Vertical distance between two snippets.

void BaseSnippetList::repaint(cairo_t* cr, int areax, int areay, int areaw, int areah) {
  layout();

  double width = get_width();

  Rect snippet_bounds(_left_spacing, _top_spacing, width - _left_spacing - _right_spacing, SNIPPET_HEIGHT);

  for (std::vector<Snippet*>::const_iterator iterator = _snippets.begin(); iterator != _snippets.end(); iterator++) {
    (*iterator)->paint(cr, snippet_bounds, *iterator == _hot_snippet, *iterator == _selected_snippet);
    snippet_bounds.pos.y += snippet_bounds.size.height + SNIPPET_SPACING;
  }
}

//------------------------------------------------------------------------------------------------

void BaseSnippetList::layout() {
  if (is_layout_dirty() || (_last_width != get_width())) {
    _last_width = get_width();
    set_layout_dirty(false);

    _layout_height = _top_spacing;
    _layout_width = _left_spacing + _right_spacing;

    if (_snippets.size() > 0)
      _layout_height += (int)_snippets.size() * SNIPPET_HEIGHT + ((int)_snippets.size() - 1) * SNIPPET_SPACING;

    if (_image != NULL) {
      // If an image is set then this defines the minimal width.
      _layout_width += SNIPPET_ICON_SPACING + image_width(_image);
    }

    if (_layout_height < SNIPPET_HEIGHT)
      _layout_height = SNIPPET_HEIGHT;
    _layout_height += _bottom_spacing;
  }
}

//------------------------------------------------------------------------------------------------

base::Size BaseSnippetList::getLayoutSize(base::Size proposedSize) {
  layout();

  return base::Size(_layout_width, _layout_height);
}

//------------------------------------------------------------------------------------------------

Snippet* BaseSnippetList::selected() {
  return _selected_snippet;
}

//------------------------------------------------------------------------------------------------

bool BaseSnippetList::mouse_leave() {
  if (DrawBox::mouse_leave())
    return true;

  if (_hot_snippet != NULL) {
    _hot_snippet = NULL;
    set_needs_repaint();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool BaseSnippetList::mouse_move(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_move(button, x, y))
    return true;

  if (_single_click) {
    Snippet* snippet = snippet_from_point(x, y);
    if (snippet != _hot_snippet) {
      _hot_snippet = snippet;
      set_needs_repaint();
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool BaseSnippetList::mouse_down(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_down(button, x, y))
    return true;

  if (button == MouseButtonLeft || button == MouseButtonRight) {
    Snippet* snippet = snippet_from_point(x, y);
    set_selected(snippet);
    return true;
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool BaseSnippetList::mouse_double_click(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_double_click(button, x, y))
    return true;

  bool result = false;
  if (button == MouseButtonRight) {
    // If this double click results from a quick !right + right click then handle it as single right click.
    if (_last_mouse_button != MouseButtonRight) {
      _context_menu->popup_at(this, x + 3, y + 3);
      result = true;
    }
  }
  _last_mouse_button = MouseButtonNone;

  return result;
}

//------------------------------------------------------------------------------------------------

bool BaseSnippetList::mouse_click(mforms::MouseButton button, int x, int y) {
  if (DrawBox::mouse_click(button, x, y))
    return true;

  // Keep the last pressed button. A quick series of two different button clicks might be interpreted
  // as a double click of the second button. So we need to handle that.
  _last_mouse_button = button;
  if (button == MouseButtonRight) {
    _context_menu->popup_at(this, x + 3, y + 3);
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------------------------

int BaseSnippetList::selected_index() {
  return _selected_index;
}

//------------------------------------------------------------------------------------------------

void BaseSnippetList::set_selected(Snippet* snippet) {
  if (_selected_snippet != snippet) {
    _selected_snippet = snippet;
    _selected_index = find_selected_index();
    set_needs_repaint();
    _selection_changed_signal();
  }
}

//------------------------------------------------------------------------------------------------

Snippet* BaseSnippetList::snippet_from_point(double x, double y) {
  if (x >= 0 && x < get_width() && y >= 0 && y <= get_height()) {
    for (std::vector<Snippet*>::const_iterator iterator = _snippets.begin(); iterator != _snippets.end(); iterator++) {
      if ((*iterator)->contains(x, y) && (*iterator)->enabled())
        return *iterator;
    }
  }
  return NULL;
}

//------------------------------------------------------------------------------------------------

void BaseSnippetList::set_snippet_info(Snippet* snippet, const std::string& title, const std::string& subtitle) {
  if (snippet) {
    snippet->title(title);
    snippet->description(subtitle);
  }
}

//------------------------------------------------------------------------------------------------

void BaseSnippetList::get_snippet_info(Snippet* snippet, std::string& title, std::string& subtitle) {
  if (snippet) {
    title = snippet->title();
    subtitle = snippet->description();
  }
}

//------------------------------------------------------------------------------------------------

base::Rect BaseSnippetList::snippet_bounds(Snippet* snippet) {
  return snippet->bounds();
}

//------------------------------------------------------------------------------------------------
int BaseSnippetList::get_acc_child_count() {
  return (int)_snippets.size();
}

//------------------------------------------------------------------------------------------------
Accessible* BaseSnippetList::get_acc_child(int index) {
  mforms::Accessible* accessible = NULL;

  if ((size_t)index < _snippets.size())
    accessible = _snippets[index];

  return accessible;
}

//------------------------------------------------------------------------------------------------
mforms::Accessible* BaseSnippetList::hit_test(int x, int y) {
  return snippet_from_point(x, y);
}
