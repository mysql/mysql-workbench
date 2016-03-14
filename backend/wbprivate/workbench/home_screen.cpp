/* 
 * Copyright (c) 2009, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"
#include "base/file_utilities.h"

#include "mforms/popup.h"
#include "mforms/menu.h"
#include "mforms/menubar.h"
#include "mforms/utilities.h"
#include "mforms/drawbox.h"
#include "mforms/textentry.h"
#include "mforms/imagebox.h"
#include "mforms/scrollpanel.h"

#include "home_screen.h"
#include "home_screen_connections.h"
#include "home_screen_x_connections.h"

#include "workbench/wb_context_names.h"

using namespace wb;
//--------------------------------------------------------------------------------------------------

// The following helpers are just temporary. They will be replaced by a cairo context class.
inline void delete_surface(cairo_surface_t* surface)
{
  if (surface != NULL)
    cairo_surface_destroy(surface);
}

//--------------------------------------------------------------------------------------------------

static int image_width(cairo_surface_t* image)
{
  if (image != NULL)
  {
    if (mforms::Utilities::is_hidpi_icon(image) && mforms::App::get()->backing_scale_factor() > 1.0)
      return (int)(cairo_image_surface_get_width(image) / mforms::App::get()->backing_scale_factor());
    else
      return (int)cairo_image_surface_get_width(image);
  }
  return 0;
}

//--------------------------------------------------------------------------------------------------

static int image_height(cairo_surface_t* image)
{
  if (image != NULL)
  {
    if (mforms::Utilities::is_hidpi_icon(image) && mforms::App::get()->backing_scale_factor() > 1.0)
      return (int)(cairo_image_surface_get_height(image) / mforms::App::get()->backing_scale_factor());
    else
      return (int)cairo_image_surface_get_height(image);
  }
  return 0;
}

/**
 * Helper to draw text with a hot decoration.
 */
static void text_with_decoration(cairo_t* cr, double x, double y, const char* text, bool hot, double width)
{
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, text);
  cairo_stroke(cr);

  // TODO: replace this with font decoration once pango is incorporated.
  if (hot)
  {
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, x, (int)y + 2.5);
    cairo_line_to(cr, x + width, (int)y + 2.5);
    cairo_stroke(cr);
  }
}

//----------------- DocumentsSection ---------------------------------------------------------------

struct DocumentEntry: mforms::Accessible
{
  grt::StringRef path;
  time_t timestamp; // Last accessed as timestamp for sorting.

  std::string title;
  std::string title_shorted;
  std::string folder;
  std::string folder_shorted;
  std::string schemas;
  std::string schemas_shorted;
  std::string last_accessed;
  std::string size;

  base::Rect bounds;
  bool is_model;

  bool operator < (const DocumentEntry &other) const
  {
    return other.timestamp < timestamp; // Sort from newest do oldest.
  }

  //------ Accessibility Methods -----
  virtual std::string get_acc_name() { return title; }
  virtual std::string get_acc_description() 
  { 
    return base::strfmt("schemas:%s;last_accessed:%s;size:%s", schemas.c_str(), last_accessed.c_str(), size.c_str()); 
  }

  virtual Accessible::Role get_acc_role() { return Accessible::ListItem;}
  virtual base::Rect get_acc_bounds() { return bounds;}
  virtual std::string get_acc_default_action() { return "Open Model";}
};

class DocumentsSection: public mforms::DrawBox
{
private:
  HomeScreen *_owner;

  cairo_surface_t* _model_icon;
  cairo_surface_t* _sql_icon;
  cairo_surface_t* _plus_icon;
  cairo_surface_t* _schema_icon;
  cairo_surface_t* _time_icon;
  cairo_surface_t* _folder_icon;
  cairo_surface_t* _size_icon;
  cairo_surface_t* _close_icon;
  cairo_surface_t* _open_icon;
  cairo_surface_t* _action_icon;
  float _backing_scale_when_icons_loaded;

  ssize_t _entries_per_row;

  bool _show_selection_message; // Additional info to let the user a connection (when opening a script).
  base::Rect _message_close_button_rect;

  typedef std::vector<DocumentEntry>::iterator DocumentIterator;
  std::vector<DocumentEntry> _documents;
  std::vector<DocumentEntry> _filtered_documents;

  mforms::Menu *_model_context_menu;
  mforms::Menu *_model_action_menu;

  ssize_t _hot_entry;
  ssize_t _active_entry;
  enum DisplayMode {Nothing, ModelsOnly, ScriptsOnly, Mixed} _display_mode;

  boost::function <bool (int, int)> _accessible_click_handler;

  HomeAccessibleButton _add_button;
  HomeAccessibleButton _open_button;
  HomeAccessibleButton _action_button;

  base::Rect _close_button_rect;
  base::Rect _use_default_button_rect;

  DisplayMode _hot_heading;
  base::Rect _model_heading_rect;
  base::Rect _sql_heading_rect;
  base::Rect _mixed_heading_rect;
public:
  const int DOCUMENTS_LEFT_PADDING = 40;
  const int DOCUMENTS_RIGHT_PADDING = 40;
  const int DOCUMENTS_TOP_PADDING = 64;
  const int DOCUMENTS_VERTICAL_SPACING = 26;
  const int DOCUMENTS_SPACING = 20;

  const int DOCUMENTS_ENTRY_WIDTH = 250; // No spacing horizontally.
  const int DOCUMENTS_ENTRY_HEIGHT = 60;
  const int DOCUMENTS_HEADING_SPACING = 10; // Spacing between a heading part and a separator.
  const int DOCUMENTS_TOP_BASELINE = 40; // Vertical space from top border to title base line.


  const int MESSAGE_WIDTH = 200;
  const int MESSAGE_HEIGHT = 75;

  const int POPUP_TIP_HEIGHT = 14;

  DocumentsSection(HomeScreen *owner)
  {
    _owner = owner;
    _model_context_menu = NULL;
    _model_action_menu = NULL;
    _hot_entry = -1;
    _active_entry = -1;
    _display_mode = ModelsOnly;
    _hot_heading = Nothing;
    _entries_per_row = 0;
    _show_selection_message = false;
    _backing_scale_when_icons_loaded = 0.0;

    load_icons();

    _add_button.name = "Add Model";
    _add_button.default_action = "Create New Model";
    _add_button.default_handler = _accessible_click_handler;

    _open_button.name = "Open Model";
    _open_button.default_action = "Open Existing Model";
    _open_button.default_handler = _accessible_click_handler;

    _action_button.name = "Create Model Options";
    _action_button.default_action = "Open Create Model Options Menu";
    _action_button.default_handler = _accessible_click_handler;
  }

  //------------------------------------------------------------------------------------------------

  ~DocumentsSection()
  {
    if (_model_context_menu != NULL)
      _model_context_menu->release();

    delete_surface(_plus_icon);
    delete_surface(_model_icon);
    delete_surface(_sql_icon);
    delete_surface(_schema_icon);
    delete_surface(_time_icon);
    delete_surface(_folder_icon);
    delete_surface(_size_icon);
    delete_surface(_close_icon);
    delete_surface(_open_icon);
    delete_surface(_action_icon);
  }

  //------------------------------------------------------------------------------------------------

  size_t entry_from_point(int x, int y)
  {
    int width = get_width();
    if (x < DOCUMENTS_LEFT_PADDING || x > (width - DOCUMENTS_RIGHT_PADDING) ||
      y < DOCUMENTS_TOP_PADDING)
      return -1; // Outside the entries area.

    x -= DOCUMENTS_LEFT_PADDING;

    y -= DOCUMENTS_TOP_PADDING;
    if ((y % (DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING)) > DOCUMENTS_ENTRY_HEIGHT)
      return -1; // Within the vertical spacing between two entries.

    width -= DOCUMENTS_LEFT_PADDING + DOCUMENTS_RIGHT_PADDING;
    _entries_per_row = width / DOCUMENTS_ENTRY_WIDTH;
    if (x >= _entries_per_row * DOCUMENTS_ENTRY_WIDTH)
      return -1; // After the last entry in a row.

    int height = get_height() - DOCUMENTS_TOP_PADDING;
    int column = x / DOCUMENTS_ENTRY_WIDTH;
    int row = y / (DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING);

    int row_bottom = row * (DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING) + DOCUMENTS_ENTRY_HEIGHT;
    if (row_bottom > height)
      return -1; // The last visible row is dimmed if not fully visible. So take it out from hit tests too.

    size_t count = _filtered_documents.size();
    size_t index = row * _entries_per_row + column;
    if (index < count)
      return index;

    return -1;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Draws and icon followed by the given text. The given position is that of the upper left corner
   * of the image.
   */
  void draw_icon_with_text(cairo_t *cr, int x, int y, cairo_surface_t *icon, const std::string &text)
  {
    mforms::Utilities::paint_icon(cr, icon, x, y);
    x += image_width(icon) + 3;

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text.c_str(), &extents);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, x , (int)(y + image_height(icon) / 2.0 + extents.height / 2.0));
    cairo_show_text(cr, text.c_str());
    cairo_stroke(cr);
  }

  //------------------------------------------------------------------------------------------------

  void draw_entry(cairo_t *cr, const DocumentEntry &entry, bool hot)
  {
    const int icon_top = 26;
    const int detail_spacing = 15;
    mforms::Utilities::paint_icon(cr, _model_icon, entry.bounds.left(), entry.bounds.top() + icon_top);

    int icon_width, icon_height;
    mforms::Utilities::get_icon_size(_model_icon, icon_width, icon_height);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);
    int x = (int)entry.bounds.left();
    int y = (int)entry.bounds.top() + 18;
    if (hot)
    {
      double width = 0;
      cairo_text_extents_t extents;
      cairo_text_extents(cr, entry.title.c_str(), &extents);
      width = ceil(extents.width);

      cairo_save(cr);
      cairo_set_source_rgb(cr, 1, 1, 1);
      text_with_decoration(cr, x - 1, y, entry.title.c_str(), true, width);
      text_with_decoration(cr, x + 1, y, entry.title.c_str(), true, width);
      text_with_decoration(cr, x, y - 1, entry.title.c_str(), true, width);
      text_with_decoration(cr, x, y + 1, entry.title.c_str(), true, width);
      cairo_restore(cr);

      text_with_decoration(cr, x, y, entry.title.c_str(), true, width);
    }
    else
      text_with_decoration(cr, x, y, entry.title_shorted.c_str(), false, 0);

    x += icon_width + 10;

    cairo_set_font_size(cr, HOME_SMALL_INFO_FONT_SIZE);

    draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top, _folder_icon, entry.folder_shorted);
    if (entry.is_model)
      draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top + detail_spacing, _schema_icon,
      entry.schemas.empty() ? "--" : entry.schemas_shorted);
    else
      draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top + detail_spacing, _size_icon,
        entry.size.empty() ? "--" : entry.size);
    draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top + (detail_spacing * 2), _time_icon, entry.last_accessed);
  }

  //------------------------------------------------------------------------------------------------

  void update_filtered_documents()
  {
    _filtered_documents.clear();
    _filtered_documents.reserve(_documents.size());
    switch (_display_mode)
    {
    case ModelsOnly:
      {
        // std::copy_if is C++11 only, so we do it manually.
        for (DocumentIterator source = _documents.begin(); source != _documents.end(); source++)
        {
          if (source->is_model)
            _filtered_documents.push_back(*source);
        }
        break;
      }

    case ScriptsOnly:
      {
        for (DocumentIterator source = _documents.begin(); source != _documents.end(); source++)
        {
          if (!source->is_model)
            _filtered_documents.push_back(*source);
        }
        break;
      }

    default: // Mixed mode. All types are shown.
      _filtered_documents = _documents;
    }
  }

  //------------------------------------------------------------------------------------------------

  void draw_selection_message(cairo_t *cr)
  {
    // Attach the message to the current active entry as this is what is used when
    // a connection is opened.
    ssize_t column = _active_entry % _entries_per_row;
    ssize_t row = _active_entry / _entries_per_row;
    int hotspot_x = (int)(DOCUMENTS_LEFT_PADDING + (column + 0.5) * DOCUMENTS_ENTRY_WIDTH);
    int hotspot_y = (int)(DOCUMENTS_TOP_PADDING + (row + 1) * DOCUMENTS_ENTRY_HEIGHT);
    base::Rect message_rect = base::Rect(hotspot_x - MESSAGE_WIDTH / 2, hotspot_y + POPUP_TIP_HEIGHT,
                                         MESSAGE_WIDTH, MESSAGE_HEIGHT);
    if (message_rect.pos.x < 10)
      message_rect.pos.x = 10;
    if (message_rect.right() > get_width() - 10)
      message_rect.pos.x = get_width() - message_rect.width() - 10;

    bool flipped = false;
    if (message_rect.bottom() > get_height() - 10)
    {
      flipped = true;
      message_rect.pos.y -= MESSAGE_HEIGHT + 2 * POPUP_TIP_HEIGHT + DOCUMENTS_ENTRY_HEIGHT - 10;
    }

    cairo_set_source_rgba(cr, 0, 0, 0, 0.9);
    cairo_rectangle(cr, message_rect.left(), message_rect.top(), MESSAGE_WIDTH, MESSAGE_HEIGHT);
    cairo_move_to(cr, message_rect.left(), message_rect.top());
    if (flipped)
    {
      cairo_rel_line_to(cr, MESSAGE_WIDTH, 0);
      cairo_rel_line_to(cr, 0, MESSAGE_HEIGHT);
      cairo_line_to(cr, hotspot_x + POPUP_TIP_HEIGHT, message_rect.bottom());
      cairo_rel_line_to(cr, -POPUP_TIP_HEIGHT, POPUP_TIP_HEIGHT);
      cairo_rel_line_to(cr, -POPUP_TIP_HEIGHT, -POPUP_TIP_HEIGHT);
      cairo_line_to(cr, message_rect.left(), message_rect.bottom());
    }
    else
    {
      cairo_line_to(cr, hotspot_x - POPUP_TIP_HEIGHT, message_rect.top());
      cairo_rel_line_to(cr, POPUP_TIP_HEIGHT, -POPUP_TIP_HEIGHT);
      cairo_rel_line_to(cr, POPUP_TIP_HEIGHT, POPUP_TIP_HEIGHT);
      cairo_line_to(cr, message_rect.right(), message_rect.top());
      cairo_rel_line_to(cr, 0, MESSAGE_HEIGHT);
      cairo_rel_line_to(cr, -MESSAGE_WIDTH, 0);
    }

    cairo_fill(cr);

    cairo_set_font_size(cr, HOME_DETAILS_FONT_SIZE);
    cairo_font_extents_t extents;
    cairo_font_extents(cr, &extents);

    int y = (int)(message_rect.top() + extents.height + 4);

    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, message_rect.left() + 10, y);
    cairo_show_text(cr, _("Please select a connection"));

    y += (int)ceil(extents.height);
    cairo_move_to(cr, message_rect.left() + 10, y);
    cairo_show_text(cr, _("to open this script with."));

    std::string use_default = _("Use Default");
    cairo_text_extents_t text_extents;
    cairo_text_extents(cr, use_default.c_str(), &text_extents);
    int x = (int)(message_rect.left() + (MESSAGE_WIDTH - text_extents.width) / 2);
    y = (int) message_rect.bottom() - 15;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, use_default.c_str());
    _use_default_button_rect = base::Rect(x - 7.5, y - ceil(text_extents.height) - 5.5, ceil(text_extents.width) + 16, ceil(text_extents.height) + 12);
    cairo_rectangle(cr, _use_default_button_rect.left(), _use_default_button_rect.top(),
                    _use_default_button_rect.width(), _use_default_button_rect.height());
    cairo_stroke(cr);

    _close_button_rect = base::Rect(message_rect.right() - image_width(_close_icon) - 4, message_rect.top() + 6,
                                    image_width(_close_icon), image_height(_close_icon));
    cairo_set_source_surface(cr, _close_icon, _close_button_rect.left(), _close_button_rect.top());
    cairo_paint(cr);
  }

  //------------------------------------------------------------------------------------------------

  void layout(cairo_t *cr)
  {
    if (is_layout_dirty())
    {
      set_layout_dirty(false);

      cairo_text_extents_t extents;

      // Keep in mind text rectangles are flipped (top is actually the base line of the text).
      double heading_left = DOCUMENTS_LEFT_PADDING;
      cairo_text_extents(cr, _("Models"), &extents);
      double text_width = ceil(extents.width);
      _model_heading_rect = base::Rect(heading_left, DOCUMENTS_TOP_BASELINE, text_width, ceil(extents.height));

      // Models (+) ...
      heading_left += text_width + DOCUMENTS_HEADING_SPACING;
      _add_button.bounds = base::Rect(heading_left, DOCUMENTS_TOP_BASELINE - image_height(_plus_icon),
        image_width(_plus_icon), image_height(_plus_icon));

      _open_button.bounds = base::Rect(_add_button.bounds.right() + 10, DOCUMENTS_TOP_BASELINE - image_height(_open_icon),
        image_width(_open_icon), image_height(_open_icon));

      _action_button.bounds = base::Rect(_open_button.bounds.right() + 10, DOCUMENTS_TOP_BASELINE - image_height(_action_icon),
        image_width(_action_icon), image_height(_action_icon));

      /* Disabled for now.
      // (+) | ...
      heading_left += 2 * DOCUMENTS_HEADING_SPACING + image_width(_plus_icon) + 1;
      cairo_text_extents(cr, _("SQL Scripts"), &extents);
      text_width = ceil(extents.width);
      _sql_heading_rect = base::Rect(heading_left, DOCUMENTS_TOP_BASELINE, text_width, ceil(extents.height));

      // SQL Scripts | ...
      heading_left += text_width + 2 * DOCUMENTS_HEADING_SPACING + 1;
      cairo_text_extents(cr, _("Recent Documents"), &extents);
      text_width = ceil(extents.width);
      _mixed_heading_rect = base::Rect(heading_left, DOCUMENTS_TOP_BASELINE, text_width, ceil(extents.height));
*/

      // Compute the shorted strings.
      cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);

      int model_icon_width = image_width(_model_icon);
      int sql_icon_width = image_width(_sql_icon);
      for (std::vector<DocumentEntry>::iterator iterator = _documents.begin();
        iterator != _documents.end(); iterator++)
      {
        double details_width = DOCUMENTS_ENTRY_WIDTH - 10 - (iterator->is_model ? model_icon_width : sql_icon_width);
        if (iterator->title_shorted.empty() && !iterator->title.empty())
          iterator->title_shorted = mforms::Utilities::shorten_string(cr, iterator->title, details_width);

        if (iterator->folder_shorted.empty() && !iterator->folder.empty())
        {
          // shorten the string while reversed, so that we truncate the beginning of the string instead of the end
          gchar *rev = g_utf8_strreverse(iterator->folder.data(), (gssize)iterator->folder.size());
          iterator->folder_shorted = mforms::Utilities::shorten_string(cr, rev, details_width);
          if (iterator->folder_shorted.compare(rev) != 0) // string was shortened
          {
            g_free(rev);
            iterator->folder_shorted = iterator->folder_shorted.substr(0, iterator->folder_shorted.size()-3); // strip the ...
            rev = g_utf8_strreverse(iterator->folder_shorted.data(), (gssize)iterator->folder_shorted.size());
            iterator->folder_shorted = std::string("...") + rev;
            g_free(rev);
          }
          else
          {
            g_free(rev);
            iterator->folder_shorted = iterator->folder;
          }
        }

        if (iterator->schemas_shorted.empty() && !iterator->schemas.empty())
          iterator->schemas_shorted = mforms::Utilities::shorten_string(cr, iterator->schemas, details_width - 10 - image_width(_schema_icon));
      }

      update_filtered_documents();
    }
  }

  //------------------------------------------------------------------------------------------------

  void updateHeight()
  {
    int tilesPerRow = (get_width() - DOCUMENTS_LEFT_PADDING - DOCUMENTS_RIGHT_PADDING) / (DOCUMENTS_ENTRY_WIDTH + DOCUMENTS_SPACING);

    if (!_documents.empty() && tilesPerRow > 1)
    {
      int height = (_documents.size() / tilesPerRow) * (DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING) + DOCUMENTS_TOP_PADDING;
      if (height != get_height())
        set_size(-1, height);
    }
  }

  //------------------------------------------------------------------------------------------------

  void load_icons()
  {
    if (_backing_scale_when_icons_loaded != mforms::App::get()->backing_scale_factor())
    {
      // reload icons if the backing scale changed
      if (_backing_scale_when_icons_loaded != 0)
      {
        delete_surface(_model_icon);
        delete_surface(_schema_icon);
        delete_surface(_time_icon);
        delete_surface(_folder_icon);
      }
      _model_icon = mforms::Utilities::load_icon("wb_doc_model.png", true);
      _schema_icon = mforms::Utilities::load_icon("wb_tile_schema.png", true);
      _time_icon = mforms::Utilities::load_icon("wb_tile_time.png", true);
      _folder_icon = mforms::Utilities::load_icon("wb_tile_folder_mini.png", true);

      if (_backing_scale_when_icons_loaded == 0)
      {
        _plus_icon = mforms::Utilities::load_icon("wb_tile_plus.png");
        _sql_icon = mforms::Utilities::load_icon("wb_doc_sql.png");
        _size_icon = mforms::Utilities::load_icon("wb_tile_number.png");
        _close_icon = mforms::Utilities::load_icon("wb_close.png");
        _open_icon = mforms::Utilities::load_icon("wb_tile_open.png");
        _action_icon = mforms::Utilities::load_icon("wb_tile_more.png");
      }

      _backing_scale_when_icons_loaded = mforms::App::get()->backing_scale_factor();
    }
  }

  //------------------------------------------------------------------------------------------------

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
  {
    int width = get_width();
    int height = get_height();

    load_icons();

    cairo_set_line_width(cr, 1);
    cairo_select_font_face(cr, HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_TITLE_FONT_SIZE);

    layout(cr);

#ifdef __APPLE__
    // On Mac we draw a radial background gradient as if the background is lit by a single light source.
    cairo_pattern_t *pattern = cairo_pattern_create_radial(width / 2.0, -10, 10,
                                                       width / 2.0, -10, 0.6 * width);
    cairo_pattern_add_color_stop_rgba(pattern, 0, 1, 1, 1, 0.05);
    cairo_pattern_add_color_stop_rgba(pattern, 1, 1, 1, 1, 0);
    cairo_set_source(cr, pattern);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    cairo_pattern_destroy(pattern);
#endif

    width -= DOCUMENTS_LEFT_PADDING + DOCUMENTS_RIGHT_PADDING;
    cairo_set_font_size(cr, HOME_TITLE_FONT_SIZE);
    int entries_per_row = width / DOCUMENTS_ENTRY_WIDTH;

    // Heading for switching display mode. Draw heading hot only when we support more sections.
    cairo_set_source_rgb(cr, 0, 0, 0);
    text_with_decoration(cr, _model_heading_rect.left(), _model_heading_rect.top(), _("Models"),
      false /*_hot_heading == ModelsOnly*/, _model_heading_rect.width());

    cairo_set_operator(cr, CAIRO_OPERATOR_XOR);

    cairo_set_source_surface(cr, _plus_icon, _add_button.bounds.left(), _add_button.bounds.top());
    cairo_paint(cr);

    cairo_set_source_surface(cr, _open_icon, _open_button.bounds.left(), _open_button.bounds.top());
    cairo_paint(cr);

    cairo_set_source_surface(cr, _action_icon, _action_button.bounds.left(), _action_button.bounds.top());
    cairo_paint(cr);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    int row = 0;
    base::Rect bounds(0, DOCUMENTS_TOP_PADDING, DOCUMENTS_ENTRY_WIDTH, DOCUMENTS_ENTRY_HEIGHT);
    bool done = false;
    while (!done)
    {
      bool draw_hot_entry = false;
      bounds.pos.x = DOCUMENTS_LEFT_PADDING;
      for (int column = 0; column < entries_per_row; column++)
      {
        size_t index = row * entries_per_row + column;
        if (index >= _filtered_documents.size())
        {
          done = true;
          break;
        }
        else
        {
          _filtered_documents[index].bounds = bounds;
          if ((size_t)_hot_entry == index)
            draw_hot_entry = true;
          else
            draw_entry(cr, _filtered_documents[index], (size_t)_hot_entry == index);
        }
        bounds.pos.x += DOCUMENTS_ENTRY_WIDTH;
      }
      if (draw_hot_entry)
        draw_entry(cr, _filtered_documents[_hot_entry], true);

      row++;
      bounds.pos.y += DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING;
      if (bounds.top() >= height)
        done = true;
    }

    if (_show_selection_message)
      draw_selection_message(cr);
  }

  //------------------------------------------------------------------------------------------------

  void add_document(const grt::StringRef &path, const time_t &time, const std::string schemas,
    long file_size)
  {
    DocumentEntry entry;
    entry.path = path;
    entry.timestamp = time;
    entry.schemas = schemas;

    entry.title = base::strip_extension(base::basename(path));
    if (entry.title.empty())
      entry.title = "???";
    entry.is_model = base::hasSuffix(path, ".mwb") || base::hasSuffix(path, ".mwbd");
    entry.folder = base::dirname(path);

    if (time > 0)
    {
      struct tm * ptm = localtime(&time);
      char buffer[32];
      strftime(buffer, 32, "%d %b %y, %H:%M", ptm);
      entry.last_accessed = buffer;
    }
    if (file_size == 0)
      entry.size = "--";
    else
    {
      // Format file size in human readable format. 1000 bytes per K on OSX, otherwise 1024.
#ifdef __APPLE__
      double unit_size = 1000;
#else
      double unit_size = 1024;
#endif
      int i = 0;
      double size = file_size;
      const char* units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
      while (size > unit_size) {
        size /= unit_size;
        i++;
      }
      entry.size = base::strfmt("%.*f %s", i, size, units[i]);
    }
    _documents.push_back(entry);
    set_layout_dirty(true);
  }

  //------------------------------------------------------------------------------------------------

  void clear_documents()
  {
    _documents.clear();
    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y)
  {
    // Similar handling like for single mouse down.
    switch (button)
    {
      case mforms::MouseButtonLeft:
      {
        if (_add_button.bounds.contains(x, y))
        {
          if (_display_mode != ModelsOnly)
          {
            _display_mode = ModelsOnly;
            update_filtered_documents();
            set_needs_repaint();
          }

          _owner->trigger_callback(ActionNewEERModel, grt::ValueRef());
          return true;
        }

        if (_open_button.bounds.contains(x, y))
        {
          if (_display_mode != ModelsOnly)
          {
            _display_mode = ModelsOnly;
            update_filtered_documents();
            set_needs_repaint();
          }

          _owner->trigger_callback(ActionOpenEERModel, grt::ValueRef());
          return true;
        }

        if (_action_button.bounds.contains(x, y))
        {
          if (_display_mode == ModelsOnly && _model_action_menu != NULL)
          {
            _model_action_menu->popup_at(this, x, y);
            return true;
          }
        }

        if (_model_heading_rect.contains_flipped(x, y))
        {
          if (_display_mode != ModelsOnly)
          {
            _display_mode = ModelsOnly;
            update_filtered_documents();
            set_needs_repaint();
          }
          return true;
        }

        if (_sql_heading_rect.contains_flipped(x, y))
        {
          if (_display_mode != ScriptsOnly)
          {
            _display_mode = ScriptsOnly;
            update_filtered_documents();
            set_needs_repaint();
          }
          return true;
        }

        if (_mixed_heading_rect.contains_flipped(x, y))
        {
          if (_display_mode != Mixed)
          {
            _display_mode = Mixed;
            update_filtered_documents();
            set_needs_repaint();
          }
          return true;
        }

        // Anything else.
        _active_entry = entry_from_point(x, y);
        if (_active_entry > -1)
        {
          if (_filtered_documents[_active_entry].is_model)
            _owner->trigger_callback(ActionOpenEERModelFromList, _filtered_documents[_active_entry].path);
          else
            _owner->trigger_callback(ActionEditSQLScript, _filtered_documents[_active_entry].path);
          return true;
        }
      }
      break;

      case mforms::MouseButtonRight:
      {
        if (_display_mode == ModelsOnly)
        {
          _active_entry = entry_from_point(x, y);
          if (_active_entry > -1 && _model_context_menu != NULL)
          {
            _model_context_menu->popup_at(this, x, y);
            return true;
          }
        }

        break;

      default:
        break;
      }
    }

    return false;
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_click(mforms::MouseButton button, int x, int y)
  {
    switch (button)
    {
      case mforms::MouseButtonLeft:
        {
          if (_show_selection_message && _close_button_rect.contains(x, y))
          {
            _owner->cancel_script_loading();
            return true;
          }

          if (_add_button.bounds.contains(x, y))
          {
            if (_display_mode != ModelsOnly)
            {
              _display_mode = ModelsOnly;
              update_filtered_documents();
              set_needs_repaint();
            }

            _owner->trigger_callback(ActionNewEERModel, grt::ValueRef());
            return true;
          }

          if (_open_button.bounds.contains(x, y))
          {
            if (_display_mode != ModelsOnly)
            {
              _display_mode = ModelsOnly;
              update_filtered_documents();
              set_needs_repaint();
            }

            _owner->trigger_callback(ActionOpenEERModel, grt::ValueRef());
            return true;
          }

          if (_action_button.bounds.contains(x, y))
          {
            if (_display_mode == ModelsOnly && _model_action_menu != NULL)
              _model_action_menu->popup_at(this, x, y);
          }

          if (_model_heading_rect.contains_flipped(x, y))
          {
            _owner->cancel_script_loading();

            if (_display_mode != ModelsOnly)
            {
              _display_mode = ModelsOnly;
              update_filtered_documents();
              set_needs_repaint();
            }
            return true;
          }

          if (_sql_heading_rect.contains_flipped(x, y))
          {
            if (_display_mode != ScriptsOnly)
            {
              _display_mode = ScriptsOnly;
              update_filtered_documents();
              set_needs_repaint();
            }
            return true;
          }

          if (_mixed_heading_rect.contains_flipped(x, y))
          {
            _owner->cancel_script_loading();

            if (_display_mode != Mixed)
            {
              _display_mode = Mixed;
              update_filtered_documents();
              set_needs_repaint();
            }
            return true;
          }

          // Anything else.
          _active_entry = entry_from_point(x, y);
          if (_active_entry > -1)
          {
            _owner->cancel_script_loading();

            if (_filtered_documents[_active_entry].is_model)
              _owner->trigger_callback(ActionOpenEERModelFromList, _filtered_documents[_active_entry].path);
            else
              _owner->trigger_callback(ActionEditSQLScript, _filtered_documents[_active_entry].path);

            return true;
          }
        }
        break;

      case mforms::MouseButtonRight:
        {
          _owner->cancel_script_loading();

          if (_display_mode == ModelsOnly)
          {
            _active_entry = entry_from_point(x, y);
            if (_active_entry > -1 && _model_context_menu != NULL)
            {
              _model_context_menu->popup_at(this, x, y);
              return true;
            }
          }
        }
        break;

      default:
        break;
    }

    return false;
  }

  //--------------------------------------------------------------------------------------------------

  bool mouse_leave()
  {
    if (_hot_heading != Nothing || _hot_entry > -1)
    {
      _hot_heading = Nothing;
      _hot_entry = -1;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_move(mforms::MouseButton button, int x, int y)
  {
    bool result = false;
    ssize_t entry = entry_from_point(x, y);
    if (entry != _hot_entry)
    {
      _hot_entry = entry;
      result = true;
    }

    if (entry == -1)
    {
      DisplayMode mode;
      // No document hit, but perhaps one of the titles.
      if (_model_heading_rect.contains_flipped(x, y))
        mode = ModelsOnly;
      else
        if (_sql_heading_rect.contains_flipped(x, y))
          mode = ScriptsOnly;
        else
          if (_mixed_heading_rect.contains_flipped(x, y))
            mode = Mixed;
          else
            mode = Nothing;

      if (mode != _hot_heading)
      {
        _hot_heading = mode;
        result = true;
      }
    }

    if (result)
      set_needs_repaint();

    return result;
  }

  //--------------------------------------------------------------------------------------------------

  void set_context_menu(mforms::Menu *menu, bool forModels)
  {
    if (forModels)
    {
      if (_model_context_menu != NULL)
        _model_context_menu->release();
      _model_context_menu = menu;
      if (_model_context_menu != NULL)
        _model_context_menu->retain();
      
      menu->set_handler(boost::bind(&DocumentsSection::handle_command, this, _1));
    }
  }

  //------------------------------------------------------------------------------------------------

  void set_action_context_menu(mforms::Menu *menu, bool forModels)
  {
    if (forModels)
    {
      if (_model_action_menu != NULL)
        _model_action_menu->release();
      _model_action_menu = menu;
      if (_model_context_menu != NULL)
        _model_action_menu->retain();

      menu->set_handler(boost::bind(&DocumentsSection::handle_command, this, _1));
    }
  }

  //------------------------------------------------------------------------------------------------

  void handle_command(const std::string &command)
  {
    if (_active_entry > -1)
      _owner->handle_context_menu(_filtered_documents[_active_entry].path, command);
    else
      _owner->handle_context_menu(grt::ValueRef(), command);
    _active_entry = -1;
  }

  //------------------------------------------------------------------------------------------------

  void show_connection_select_message()
  {
    _show_selection_message = true;
    set_needs_repaint();
  }

  //------------------------------------------------------------------------------------------------

  void hide_connection_select_message()
  {
    _show_selection_message = false;
    set_needs_repaint();
  }

  //------------------------------------------------------------------------------------------------
  
  void cancel_operation()
  {
    _owner->cancel_script_loading();
  }

  //------------------------------------------------------------------------------------------------

  virtual int get_acc_child_count()
  { 
    // Initial value due to the add/open/create EER Model icons
    int ret_val = 3;
    ret_val += (int)_filtered_documents.size();

    return ret_val;
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible* get_acc_child(int index)
  { 
    mforms::Accessible* accessible = NULL;
    switch(index)
    {
      case 0:
        break;
      case 1:
        break;
      case 2:
        break;
      default:
        {
          index -=3;

          if (index < (int) _filtered_documents.size())
            accessible = &_filtered_documents[index];
        }
    }

    return accessible;
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible::Role get_acc_role()
  { 
    return Accessible::List;
  }

  //------------------------------------------------------------------------------------------------
  
  virtual mforms::Accessible* hit_test(int x, int y)
  { 
    mforms::Accessible* accessible = NULL;

    if (_add_button.bounds.contains(x, y))
      accessible = &_add_button;
    else if (_open_button.bounds.contains(x, y))
      accessible = &_open_button;
    else if (_action_button.bounds.contains(x, y))
      accessible = &_action_button;
    else
    {
      ssize_t entry = entry_from_point(x, y);
    
      if (entry != -1)
        accessible = &_filtered_documents[entry];
    }

    return accessible;
  }
};

//----------------- ShortcutSection ----------------------------------------------------------------

struct SidebarEntry : mforms::Accessible
{
  std::function<void()> callback;
  bool canSelect;

  cairo_surface_t *icon;
  std::string title;       // Shorted title, depending on available space.
  base::Rect title_bounds; // Relative bounds of the title text.
  base::Rect acc_bounds;   // Bounds to be used for accessibility

  // ------ Accesibility Methods -----
  virtual std::string get_acc_name() { return title; }
  virtual Accessible::Role get_acc_role() { return Accessible::ListItem; }
  virtual base::Rect get_acc_bounds() { return acc_bounds; }
  virtual std::string get_acc_default_action() { return "Open Item"; }
};

class SidebarSection: public mforms::DrawBox
{
private:
  HomeScreen *_owner;
  cairo_surface_t* _defaultEntryIcon;

  std::vector<SidebarEntry*> _entries;

  SidebarEntry *_hotEntry;
  SidebarEntry *_activeEntry; // For the context menu.
  mforms::Menu _entryContextMenu;

  boost::function <bool (int, int)> _accessible_click_handler;

public:

  const int SIDEBAR_LEFT_PADDING = 18;
  const int SIDEBAR_TOP_PADDING = 18; // The vertical offset of the first shortcut entry.
  const int SIDEBAR_RIGHT_PADDING = 25;
  const int SIDEBAR_ROW_HEIGHT = 50;
  const int SIDEBAR_SPACING = 18;// Vertical space between entries.

  SidebarSection(HomeScreen *owner)
  {
    _owner = owner;
    _hotEntry = nullptr;
    _activeEntry = nullptr;
    _defaultEntryIcon = mforms::Utilities::load_icon("wb_starter_generic_52.png", true);

    _accessible_click_handler = boost::bind(&SidebarSection::mouse_click, this,
      mforms::MouseButtonLeft, _1, _2);
  }

  ~SidebarSection()
  {
    delete_surface(_defaultEntryIcon);

    for(auto it : _entries)
      delete it;
  }

  //--------------------------------------------------------------------------------------------------

  void drawTriangle(cairo_t *cr, int x1, int y1, int x2, int y2, float alpha)
  {
    cairo_set_source_rgba(cr, 255.0, 255.0, 255.0, alpha);

    cairo_move_to(cr, x2, y1 + abs(y2 - y1) / 3);
    cairo_line_to(cr, x1 + abs(x2 - x1) * 0.6, y1 + abs(y2 - y1) / 2);
    cairo_line_to(cr, x2, y2 - abs(y2 - y1) / 3);
    cairo_fill(cr);
  }

  //------------------------------------------------------------------------------------------------

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
  {
    layout(cr);
  
    int height = get_height();

    cairo_select_font_face(cr, HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_TITLE_FONT_SIZE);

    cairo_set_source_rgb(cr, 0, 0, 0);

    // Shortcuts block.
    int yoffset = SIDEBAR_TOP_PADDING;
    if (_entries.size() > 0 && yoffset < height)
    {
      cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);

      for (auto iterator : _entries)
      {
        float alpha = (yoffset + SIDEBAR_ROW_HEIGHT) > height ? 0.25f : 1;

        iterator->acc_bounds.pos.x = SIDEBAR_LEFT_PADDING;
        iterator->acc_bounds.pos.y = yoffset;
        iterator->acc_bounds.size.width = get_width() - (SIDEBAR_LEFT_PADDING + SIDEBAR_RIGHT_PADDING);
        iterator->acc_bounds.size.height = SIDEBAR_ROW_HEIGHT;

        mforms::Utilities::paint_icon(cr, iterator->icon, SIDEBAR_LEFT_PADDING, yoffset, alpha);

        if (!iterator->title.empty())
          cairo_set_source_rgba(cr, 0, 0, 0, alpha);

        if (iterator == _activeEntry) //we need to draw an indicator
          drawTriangle(cr, get_width() - SIDEBAR_RIGHT_PADDING, yoffset, get_width(), yoffset + SIDEBAR_ROW_HEIGHT, alpha);

        yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
        if (yoffset >= height)
          break;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  int shortcutFromPoint(int x, int y)
  {
    if (x < SIDEBAR_LEFT_PADDING || y < SIDEBAR_TOP_PADDING || x > get_width() - SIDEBAR_RIGHT_PADDING)
      return -1;

    y -= SIDEBAR_TOP_PADDING;
    int point_in_row = y % (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING);
    if (point_in_row >= SIDEBAR_ROW_HEIGHT)
      return -1; // In the spacing between entries.

    size_t row = y / (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING);
    size_t height = get_height() - SIDEBAR_TOP_PADDING;
    size_t row_bottom = row * (SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING) + SIDEBAR_ROW_HEIGHT;
    if (row_bottom > height)
      return -1; // The last shortcut is dimmed if it goes over the bottom border.
                 // Take it out from the hit test too.

    if (row < _entries.size())
      return (int)row;

    return -1;
  }

//  //--------------------------------------------------------------------------------------------------
//
  /**
   * Adds a new sidebar entry to the internal list. The function performs some sanity checks.
   */
  void addEntry(const std::string& icon_name, std::function<void()> callback, bool canSelect)
  {

    SidebarEntry *entry = new SidebarEntry;

    entry->callback = callback;
    entry->canSelect = canSelect;


    // See if we can load the icon. If not use the placeholder.
    entry->icon = mforms::Utilities::load_icon(icon_name, true);
    if (entry->icon == NULL)
      entry->icon = _defaultEntryIcon;

    _entries.push_back(entry);
    if (_activeEntry == nullptr && entry->canSelect)
      _activeEntry = _entries.back();

    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  void clearEntries()
  {
    for (auto iterator : _entries)
    {
      if (iterator->icon != _defaultEntryIcon)
        delete_surface(iterator->icon);
      delete iterator;
    }

    _hotEntry = nullptr;
    _activeEntry = nullptr;
    _entries.clear();
    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  void layout(cairo_t* cr)
  {
    if (is_layout_dirty())
    {
      set_layout_dirty(false);

      double icon_xoffset = SIDEBAR_LEFT_PADDING;
      double text_xoffset = icon_xoffset + 60;

      double yoffset = SIDEBAR_TOP_PADDING;

      double text_width = get_width() - text_xoffset - SIDEBAR_RIGHT_PADDING;

      cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);

      cairo_font_extents_t font_extents;
      cairo_font_extents(cr, &font_extents);
      double text_height = ceil(font_extents.height);

      // Compute bounding box for each shortcut entry.
      for (auto iterator : _entries)
      {
        int icon_height = image_height(iterator->icon);


        std::string title = iterator->title;
        if (!title.empty())
        {
          iterator->title_bounds.pos.x = text_xoffset;

          // Text position is the lower-left corner.
          iterator->title_bounds.pos.y = icon_height / 4 + text_height / 2;
          iterator->title_bounds.size.height = text_height;

          cairo_text_extents_t extents;
          iterator->title = mforms::Utilities::shorten_string(cr, title, text_width);
          cairo_text_extents(cr, iterator->title.c_str(), &extents);
          iterator->title_bounds.size.width = extents.width;
        }

        yoffset += SIDEBAR_ROW_HEIGHT + SIDEBAR_SPACING;
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y)
  {
    return mouse_click(button, x, y); // Handle both the same way. Important especially for fast scrolling.
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_click(mforms::MouseButton button, int x, int y)
  {
    switch (button)
    {
      case mforms::MouseButtonLeft:
        {
          if (_hotEntry != nullptr && _hotEntry->canSelect)
          {
            _activeEntry = _hotEntry;
            set_needs_repaint();
          }

          if (_hotEntry != nullptr && _hotEntry->callback)
            _hotEntry->callback();
        }
        break;

      default:
        break;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  bool mouse_leave()
  {
    if (_hotEntry != nullptr)
    {
      _hotEntry = nullptr;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_move(mforms::MouseButton button, int x, int y)
  {
    SidebarEntry *shortcut = nullptr;
    int row = shortcutFromPoint(x, y);
    if (row > -1)
      shortcut = _entries[row];
    if (shortcut != _hotEntry)
    {
      _hotEntry = shortcut;
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  void cancel_operation()
  {
    _owner->cancel_script_loading();
  }

  //------------------------------------------------------------------------------------------------
  
  virtual int get_acc_child_count()
  { 
    return (int)_entries.size();
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible* get_acc_child(int index)
  { 
    mforms::Accessible* accessible = NULL;

    if (index < (int)_entries.size())
      accessible = _entries[index];

    return accessible;
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible::Role get_acc_role()
  { 
    return Accessible::List;
  }

  virtual mforms::Accessible* hit_test(int x, int y)
  { 
    mforms::Accessible* accessible = NULL;

    int row = shortcutFromPoint(x, y);
    if (row != -1)
      accessible = _entries[row];

    return accessible;
  }

};

//----------------- HomeScreen ---------------------------------------------------------------------

#include "workbench/wb_command_ui.h"

HomeScreen::HomeScreen(db_mgmt_ManagementRef rdbms)
  : AppView(true, "home", true)
{
  _rdbms = rdbms;

  _callback = nullptr;
  _user_data = nullptr;
  openMigrationCallback = nullptr;

  _sidebarSection = new SidebarSection(this);
  _sidebarSection->set_name("Home Shortcuts Section");
  _sidebarSection->set_size(85, -1);
  add(_sidebarSection, false, true);

  mforms::ScrollPanel *scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
  _xConnectionsSection = new wb::XConnectionsSection(this);
  _xConnectionsSection->set_name("Home X Connections Section");
  _xConnectionsSection->set_size(1, -1); // We need initial size for OSX.
  scroll->add(_xConnectionsSection);
  add(scroll, true, true);

  scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
  _connection_section = new wb::ConnectionsSection(this);
  _connection_section->set_name("Home Connections Section");
  _connection_section->set_size(1, -1);  // We need initial size for OSX.
  scroll->add(_connection_section);
  add(scroll, true, true);
  scroll->show(false);

  scroll = mforms::manage(new mforms::ScrollPanel(mforms::ScrollPanelNoFlags));
  _document_section = new DocumentsSection(this);
  _document_section->set_name("Home Models Section");
  _document_section->set_size(1, -1); // We need initial size for OSX.
  scroll->add(_document_section);
  add(scroll, true, true);
  scroll->show(false);

  _sidebarSection->addEntry("wb_starter_grt_shell_52.png", [this](){
    _xConnectionsSection->get_parent()->show(true);
    _xConnectionsSection->updateHeight();
    _connection_section->get_parent()->show(false);
    _document_section->get_parent()->show(false);
  }, true);

  _sidebarSection->addEntry("wb_starter_mysql_bug_reporter_52.png", [this]() {
    _xConnectionsSection->get_parent()->show(false);
    _connection_section->get_parent()->show(true);
    _connection_section->updateHeight();
    _document_section->get_parent()->show(false);
  }, true);

  _sidebarSection->addEntry("wb_starter_mysql_wb_blog_52.png", [this]() {
    _xConnectionsSection->get_parent()->show(false);
    _connection_section->get_parent()->show(false);
    _document_section->get_parent()->show(true);
    _document_section->updateHeight();
  }, true);

  _sidebarSection->addEntry("wb_starter_mysql_migration_52.png", [this]() {
    if (openMigrationCallback)
      openMigrationCallback();
  }, false);
  
//  set_menubar(mforms::manage(cmdui->create_menubar_for_context(WB_CONTEXT_HOME_GLOBAL)));
  //_toolbar = mforms::manage(cmdui->create_toolbar(""));

  update_colors();

  Box::scoped_connect(signal_resized(), boost::bind(&HomeScreen::on_resize, this));
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//--------------------------------------------------------------------------------------------------

HomeScreen::~HomeScreen()
{
  base::NotificationCenter::get()->remove_observer(this);
  clear_subviews(); // Remove our sections or the View d-tor will try to release them.

  delete _sidebarSection;
  delete _connection_section;
  delete _document_section;
  delete _xConnectionsSection;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::update_colors()
{
  set_back_color("#ffffff");
  _sidebarSection->set_back_color("#464646");
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::set_callback(home_screen_action_callback callback, void* user_data)
{
  _callback= callback;
  _user_data= user_data;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::trigger_callback(HomeScreenAction action, const grt::ValueRef &object)
{
  if (action == ActionEditSQLScript)
  {
    // Editing a script takes 2 steps. The first one happens here, as we store the request and ask the user for
    // a connection to open with that script.
    _pending_script = grt::StringRef::cast_from(object);
    _document_section->show_connection_select_message();
    return;
  }
  else
    _document_section->hide_connection_select_message();

  if (action == ActionOpenConnectionFromList)
  {
    // The second step if we are opening an SQL script. If no SQL is selected we open the connection as usual.
    if (!_pending_script.empty()&& _callback != NULL)
    {
      grt::DictRef dict;
      dict["connection"] = object;
      dict["script"] = grt::StringRef(_pending_script);
      (*_callback)(ActionEditSQLScript, dict, _user_data);
    }
  }

  if (_callback != NULL)
    (*_callback)(action, object, _user_data);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::cancel_script_loading()
{
  _pending_script = "";
  _document_section->hide_connection_select_message();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::clear_connections(bool clear_state)
{
  _connection_section->clear_connections(clear_state);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::add_connection(db_mgmt_ConnectionRef connection, const std::string &title,
  const std::string &description, const std::string &user, const std::string &schema)
{
  _connection_section->add_connection(connection, title, description, user, schema);
}


void HomeScreen::oldAuthConnections(const std::vector<db_mgmt_ConnectionRef> &list)
{
  _oldAuthList.assign(list.begin(), list.end());
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::add_document(const grt::StringRef& path, const time_t &time,
  const std::string schemas, long file_size)
{
  _document_section->add_document(path, time, schemas, file_size);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::clear_documents()
{
  _document_section->clear_documents();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::set_menu(mforms::Menu *menu, HomeScreenMenuType type)
{
  switch (type)
  {
    case HomeMenuConnection:
    case HomeMenuConnectionGroup:
    case HomeMenuConnectionGeneric:
      _connection_section->set_context_menu(menu, type);
      break;

    case HomeMenuDocumentModelAction:
      _document_section->set_action_context_menu(menu, true);
      break;

    case HomeMenuDocumentModel:
      _document_section->set_context_menu(menu, true);
      break;

    case HomeMenuDocumentSQLAction:
      _document_section->set_action_context_menu(menu, false);
      break;

    case HomeMenuDocumentSQL:
      _document_section->set_context_menu(menu, false);
      break;
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::on_resize()
{
  // Resize changes the layout so if there is pending script loading the popup is likely misplaced.
  if (!_pending_script.empty())
    cancel_script_loading();

}

//--------------------------------------------------------------------------------------------------

void HomeScreen::setup_done()
{
  _connection_section->focus_search_box();
  if (!_oldAuthList.empty())
  {
    std::string tmp;
    std::vector<db_mgmt_ConnectionRef>::const_iterator it;
    for (it = _oldAuthList.begin(); it != _oldAuthList.end(); ++it)
    {
      tmp.append("\n");
      tmp.append((*it)->name());
      tmp.append(" user name:");
      tmp.append((*it)->parameterValues().get_string("userName"));
    }

    int rc = mforms::Utilities::show_warning("Connections using old authentication protocol found",
              "While loading the stored connections some were found to use the old authentication protocol. "
              "This is no longer supported by MySQL Workbench and the MySQL client library. Click on the \"More Info\" button for a more detailed explanation.\n\n"
              "With this change it is essential that user accounts are converted to the new password storage or you can no longer connect with MySQL Workbench using these accounts.\n\n"
              "The following connections are affected:\n"
              +tmp,
              "Change", "Ignore", "More Info");
    if (rc == mforms::ResultOther)
    {
      mforms::Utilities::open_url("http://mysqlworkbench.org/2014/03/mysql-workbench-6-1-updating-accounts-using-the-old-pre-4-1-1-authentication-protocol/");
    }
    else if (rc == mforms::ResultOk)
    {
      std::vector<db_mgmt_ConnectionRef>::const_iterator it;
      for (it = _oldAuthList.begin(); it != _oldAuthList.end(); ++it)
      {
        if((*it).is_valid())
        {
          if ((*it)->parameterValues().has_key("useLegacyAuth"))
            (*it)->parameterValues().remove("useLegacyAuth");
        }
      }
      _oldAuthList.clear();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::handle_notification(const std::string &name, void *sender, base::NotificationInfo &info)
{
  if (name == "GNColorsChanged")
  {
    update_colors();
  }
}

//--------------------------------------------------------------------------------------------------
