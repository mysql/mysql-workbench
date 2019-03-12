/*
 * Copyright (c) 2016, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/home_screen_documents.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "mforms/app.h"

#include "mforms/home_screen.h"

using namespace mforms;

//----------------- DocumentEntry --------------------------------------------------------------------------------------

bool DocumentEntry::operator<(const DocumentEntry &other) const {
  return other.timestamp < timestamp; // Sort from newest do oldest.
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentEntry::setTitle(const std::string &t) {
  title = t;
}

//----------------------------------------------------------------------------------------------------------------------

std::string DocumentEntry::getAccessibilityDescription() {
  return title;
}

//----------------------------------------------------------------------------------------------------------------------

base::Accessible::Role DocumentEntry::getAccessibilityRole() {
  return Accessible::ListItem;
}

//----------------------------------------------------------------------------------------------------------------------

base::Rect DocumentEntry::getAccessibilityBounds() {
  return bounds;
}

//----------------------------------------------------------------------------------------------------------------------

std::string DocumentEntry::getAccessibilityDefaultAction() {
  return "Open Model";
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentEntry::accessibilityDoDefaultAction() {
  if (default_handler)
    default_handler((int)bounds.center().x, (int)bounds.center().y);
}

//----------------- DocumentsSection -----------------------------------------------------------------------------------

DocumentsSection::DocumentsSection(mforms::HomeScreen *owner) : HomeScreenSection("sidebar_modeling.png") {
  _owner = owner;

  _add_button.title = "Add Model";
  _add_button.description = "Create new model button";
  _add_button.defaultHandler = [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionNewEERModel, base::any());
  };

  _open_button.title = "Open Model";
  _open_button.description = "Open existing model button";
  _open_button.defaultHandler = [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionOpenEERModel, base::any());
  };

  _action_button.title = "Create Model Options";
  _action_button.description = "Open model options menu button";
  _action_button.defaultHandler = [this]() {
    _model_action_menu->popup_at(this, static_cast<int>(_action_button.bounds.xcenter()),
      static_cast<int>(_action_button.bounds.ycenter()));
  };
}

//----------------------------------------------------------------------------------------------------------------------

DocumentsSection::~DocumentsSection() {
  if (_model_context_menu != nullptr)
    _model_context_menu->release();

  deleteIcons();
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t DocumentsSection::entry_from_point(int x, int y) {
  int width = get_width();
  if (x < DOCUMENTS_LEFT_PADDING || x > (width - DOCUMENTS_RIGHT_PADDING) || y < DOCUMENTS_TOP_PADDING)
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

  std::size_t count = _filtered_documents.size();
  std::size_t index = row * _entries_per_row + column;
  if (index < count)
    return index;

  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Draws and icon followed by the given text. The given position is that of the upper left corner
 * of the image.
 */
void DocumentsSection::draw_icon_with_text(cairo_t *cr, int x, int y, cairo_surface_t *icon, const std::string &text) {
  base::Size imageSize;
  if (icon != nullptr) {
    imageSize = mforms::Utilities::getImageSize(icon);

    mforms::Utilities::paint_icon(cr, icon, x, y);
    x += (int)imageSize.width + 3;
  }

  cairo_text_extents_t extents;
  cairo_text_extents(cr, text.c_str(), &extents);

  cairo_set_source_rgb(cr, _textColor.red, _textColor.green, _textColor.blue);
  cairo_move_to(cr, x, (int)(y + imageSize.height / 2.0 + extents.height / 2.0));
  cairo_show_text(cr, text.c_str());
  cairo_stroke(cr);
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::draw_entry(cairo_t *cr, const DocumentEntry &entry, bool hot) {
  const int icon_top = 26;
  const int detail_spacing = 15;
  mforms::Utilities::paint_icon(cr, _model_icon, entry.bounds.left(), entry.bounds.top() + icon_top);

  base::Size iconSize = mforms::Utilities::getImageSize(_model_icon);

  cairo_set_source_rgb(cr, _textColor.red, _textColor.green, _textColor.blue);
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);
  int x = (int)entry.bounds.left();
  int y = (int)entry.bounds.top() + 18;
  if (hot) {
    double width = 0;
    cairo_text_extents_t extents;
    cairo_text_extents(cr, entry.title.c_str(), &extents);
    width = ceil(extents.width);

    textWithDecoration(cr, x, y, entry.title.c_str(), true, width);
  } else {
    textWithDecoration(cr, x, y, entry.title_shorted.c_str(), false, 0);
  }

  x += (int)iconSize.width + 10;

  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SMALL_INFO_FONT_SIZE);

  draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top, _folder_icon, entry.folder_shorted);
  if (entry.is_model)
    draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top + detail_spacing, _schema_icon,
                        entry.schemas.empty() ? "--" : entry.schemas_shorted);
  else
    draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top + detail_spacing, _size_icon,
                        entry.size.empty() ? "--" : entry.size);
  draw_icon_with_text(cr, x, (int)entry.bounds.top() + icon_top + 2 * detail_spacing, _time_icon,
                      entry.last_accessed);
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::update_filtered_documents() {
  _filtered_documents.clear();
  _filtered_documents.reserve(_documents.size());
  switch (_display_mode) {
    case ModelsOnly: {
      // std::copy_if is C++11 only, so we do it manually.
      for (DocumentIterator source = _documents.begin(); source != _documents.end(); source++) {
        if (source->is_model)
          _filtered_documents.push_back(*source);
      }
      break;
    }

    case ScriptsOnly: {
      for (DocumentIterator source = _documents.begin(); source != _documents.end(); source++) {
        if (!source->is_model)
          _filtered_documents.push_back(*source);
      }
      break;
    }

    default: // Mixed mode. All types are shown.
      _filtered_documents = _documents;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::draw_selection_message(cairo_t *cr) {
  // Attach the message to the current active entry as this is what is used when
  // a connection is opened.
  ssize_t column = _active_entry % _entries_per_row;
  ssize_t row = _active_entry / _entries_per_row;
  int hotspot_x = (int)(DOCUMENTS_LEFT_PADDING + (column + 0.5) * DOCUMENTS_ENTRY_WIDTH);
  int hotspot_y = (int)(DOCUMENTS_TOP_PADDING + (row + 1) * DOCUMENTS_ENTRY_HEIGHT);
  base::Rect message_rect =
    base::Rect(hotspot_x - MESSAGE_WIDTH / 2, hotspot_y + POPUP_TIP_HEIGHT, MESSAGE_WIDTH, MESSAGE_HEIGHT);
  if (message_rect.pos.x < 10)
    message_rect.pos.x = 10;
  if (message_rect.right() > get_width() - 10)
    message_rect.pos.x = get_width() - message_rect.width() - 10;

  bool flipped = false;
  if (message_rect.bottom() > get_height() - 10) {
    flipped = true;
    message_rect.pos.y -= MESSAGE_HEIGHT + 2 * POPUP_TIP_HEIGHT + DOCUMENTS_ENTRY_HEIGHT - 10;
  }

  cairo_set_source_rgba(cr, _textColor.red, _textColor.green, _textColor.blue, 0.9);
  cairo_rectangle(cr, message_rect.left(), message_rect.top(), MESSAGE_WIDTH, MESSAGE_HEIGHT);
  cairo_move_to(cr, message_rect.left(), message_rect.top());
  if (flipped) {
    cairo_rel_line_to(cr, MESSAGE_WIDTH, 0);
    cairo_rel_line_to(cr, 0, MESSAGE_HEIGHT);
    cairo_line_to(cr, hotspot_x + POPUP_TIP_HEIGHT, message_rect.bottom());
    cairo_rel_line_to(cr, -POPUP_TIP_HEIGHT, POPUP_TIP_HEIGHT);
    cairo_rel_line_to(cr, -POPUP_TIP_HEIGHT, -POPUP_TIP_HEIGHT);
    cairo_line_to(cr, message_rect.left(), message_rect.bottom());
  } else {
    cairo_line_to(cr, hotspot_x - POPUP_TIP_HEIGHT, message_rect.top());
    cairo_rel_line_to(cr, POPUP_TIP_HEIGHT, -POPUP_TIP_HEIGHT);
    cairo_rel_line_to(cr, POPUP_TIP_HEIGHT, POPUP_TIP_HEIGHT);
    cairo_line_to(cr, message_rect.right(), message_rect.top());
    cairo_rel_line_to(cr, 0, MESSAGE_HEIGHT);
    cairo_rel_line_to(cr, -MESSAGE_WIDTH, 0);
  }

  cairo_fill(cr);

  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_DETAILS_FONT_SIZE);
  cairo_font_extents_t extents;
  cairo_font_extents(cr, &extents);

  int y = (int)(message_rect.top() + extents.height + 4);

  cairo_set_source_rgb(cr, _textColor.red, _textColor.green, _textColor.blue);
  cairo_move_to(cr, message_rect.left() + 10, y);
  cairo_show_text(cr, _("Please select a connection"));

  y += (int)ceil(extents.height);
  cairo_move_to(cr, message_rect.left() + 10, y);
  cairo_show_text(cr, _("to open this script with."));

  std::string use_default = _("Use Default");
  cairo_text_extents_t text_extents;
  cairo_text_extents(cr, use_default.c_str(), &text_extents);
  int x = (int)(message_rect.left() + (MESSAGE_WIDTH - text_extents.width) / 2);
  y = (int)message_rect.bottom() - 15;
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, use_default.c_str());
  _use_default_button_rect = base::Rect(x - 7.5, y - ceil(text_extents.height) - 5.5, ceil(text_extents.width) + 16,
                                        ceil(text_extents.height) + 12);
  cairo_rectangle(cr, _use_default_button_rect.left(), _use_default_button_rect.top(), _use_default_button_rect.width(),
                  _use_default_button_rect.height());
  cairo_stroke(cr);

  _close_button_rect = base::Rect(message_rect.right() - imageWidth(_close_icon) - 4, message_rect.top() + 6,
                                  imageWidth(_close_icon), imageHeight(_close_icon));
  cairo_set_source_surface(cr, _close_icon, _close_button_rect.left(), _close_button_rect.top());
  cairo_paint(cr);
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::layout(cairo_t *cr) {
  if (is_layout_dirty()) {
    set_layout_dirty(false);

    cairo_text_extents_t extents;

    // Keep in mind text rectangles are flipped (top is actually the base line of the text).
    double heading_left = DOCUMENTS_LEFT_PADDING;
    cairo_text_extents(cr, _("Models"), &extents);
    double text_width = ceil(extents.width);
    _model_heading_rect = base::Rect(heading_left, DOCUMENTS_TOP_BASELINE, text_width, ceil(extents.height));

    // Models (+) ...
    heading_left += text_width + DOCUMENTS_HEADING_SPACING;
    _add_button.bounds = base::Rect(heading_left, DOCUMENTS_TOP_BASELINE - imageHeight(_plus_icon),
                                    imageWidth(_plus_icon), imageHeight(_plus_icon));

    _open_button.bounds = base::Rect(_add_button.bounds.right() + 4, DOCUMENTS_TOP_BASELINE - imageHeight(_open_icon),
                                     imageWidth(_open_icon), imageHeight(_open_icon));

    _action_button.bounds =
      base::Rect(_open_button.bounds.right() + 4, DOCUMENTS_TOP_BASELINE - imageHeight(_action_icon),
                 imageWidth(_action_icon), imageHeight(_action_icon));

    /* Disabled for now.
     // (+) | ...
     heading_left += 2 * DOCUMENTS_HEADING_SPACING + imageWidth(_plus_icon) + 1;
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
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SUBTITLE_FONT_SIZE);

    int model_icon_width = imageWidth(_model_icon);
    int sql_icon_width = imageWidth(_sql_icon);
    for (std::vector<DocumentEntry>::iterator iterator = _documents.begin(); iterator != _documents.end(); iterator++) {
      double details_width = DOCUMENTS_ENTRY_WIDTH - 10 - (iterator->is_model ? model_icon_width : sql_icon_width);
      if (iterator->title_shorted.empty() && !iterator->title.empty())
        iterator->title_shorted = mforms::Utilities::shorten_string(cr, iterator->title, details_width);

      if (iterator->folder_shorted.empty() && !iterator->folder.empty()) {
        // shorten the string while reversed, so that we truncate the beginning of the string instead of the end
        gchar *rev = g_utf8_strreverse(iterator->folder.data(), (gssize)iterator->folder.size());
        iterator->folder_shorted = mforms::Utilities::shorten_string(cr, rev, details_width);
        if (iterator->folder_shorted.compare(rev) != 0) // string was shortened
        {
          g_free(rev);
          iterator->folder_shorted =
            iterator->folder_shorted.substr(0,
                                            iterator->folder_shorted.size() - 3); // strip the ...
          rev = g_utf8_strreverse(iterator->folder_shorted.data(), (gssize)iterator->folder_shorted.size());
          iterator->folder_shorted = std::string("...") + rev;
          g_free(rev);
        } else {
          g_free(rev);
          iterator->folder_shorted = iterator->folder;
        }
      }

      if (iterator->schemas_shorted.empty() && !iterator->schemas.empty())
        iterator->schemas_shorted =
          mforms::Utilities::shorten_string(cr, iterator->schemas, details_width - 10 - imageWidth(_schema_icon));
    }

    update_filtered_documents();
  }
}

//----------------------------------------------------------------------------------------------------------------------

const char* DocumentsSection::getTitle() {
  return "Documents Section";
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::cancelOperation() {
  _pending_script = "";
  hide_connection_select_message();
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::setFocus() {
  // pass
}

//----------------------------------------------------------------------------------------------------------------------

bool DocumentsSection::canHandle(HomeScreenMenuType type) {
  switch (type) {
    case HomeMenuDocumentModelAction:
    case HomeMenuDocumentModel:
    case HomeMenuDocumentSQLAction:
    case HomeMenuDocumentSQL:
      return true;
    default:
      return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::setContextMenu(mforms::Menu *menu, HomeScreenMenuType type) {
  if (canHandle(type) && type == HomeMenuDocumentModel) {
    if (_model_context_menu != NULL)
      _model_context_menu->release();
    _model_context_menu = menu;
    if (_model_context_menu != NULL)
      _model_context_menu->retain();

    menu->set_handler(std::bind(&DocumentsSection::handle_command, this, std::placeholders::_1));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type) {
  if (canHandle(type) && type == HomeMenuDocumentModelAction) {
    if (_model_action_menu != NULL)
      _model_action_menu->release();
    _model_action_menu = menu;
    if (_model_context_menu != NULL)
      _model_action_menu->retain();

    menu->set_handler(std::bind(&DocumentsSection::handle_command, this, std::placeholders::_1));
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::updateColors() {
  if (_owner->isDarkModeActive()) {
    _textColor = base::Color::parse("#F4F4F4");
  } else {
    _textColor = base::Color::parse("#505050");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::updateIcons() {
  deleteIcons();

  if (_owner->isDarkModeActive()) {
    _model_icon = mforms::Utilities::load_icon("wb_doc_model.png", true);
    _schema_icon = mforms::Utilities::load_icon("wb_tile_schema_dark.png", true);
    _time_icon = mforms::Utilities::load_icon("wb_tile_time_dark.png", true);
    _folder_icon = mforms::Utilities::load_icon("wb_tile_folder_mini_dark.png", true);
    _plus_icon = mforms::Utilities::load_icon("wb_tile_plus_dark.png");
    _sql_icon = mforms::Utilities::load_icon("wb_doc_sql.png");
    _size_icon = mforms::Utilities::load_icon("wb_tile_number_dark.png");
    _close_icon = mforms::Utilities::load_icon("wb_close.png");
    _open_icon = mforms::Utilities::load_icon("wb_tile_open_dark.png");
    _action_icon = mforms::Utilities::load_icon("wb_tile_more_dark.png");
  } else {
    _model_icon = mforms::Utilities::load_icon("wb_doc_model.png", true);
    _schema_icon = mforms::Utilities::load_icon("wb_tile_schema_light.png", true);
    _time_icon = mforms::Utilities::load_icon("wb_tile_time_light.png", true);
    _folder_icon = mforms::Utilities::load_icon("wb_tile_folder_mini_light.png", true);
    _plus_icon = mforms::Utilities::load_icon("wb_tile_plus_light.png");
    _sql_icon = mforms::Utilities::load_icon("wb_doc_sql.png");
    _size_icon = mforms::Utilities::load_icon("wb_tile_number_light.png");
    _close_icon = mforms::Utilities::load_icon("wb_close.png");
    _open_icon = mforms::Utilities::load_icon("wb_tile_open_light.png");
    _action_icon = mforms::Utilities::load_icon("wb_tile_more_light.png");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
  int width = get_width();
  int height = get_height();

  cairo_set_line_width(cr, 1);
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

  layout(cr);

  width -= DOCUMENTS_LEFT_PADDING + DOCUMENTS_RIGHT_PADDING;
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);
  int entries_per_row = width / DOCUMENTS_ENTRY_WIDTH;

  // Heading for switching display mode. Draw heading hot only when we support more sections.
  cairo_set_source_rgb(cr, _textColor.red, _textColor.green, _textColor.blue);
  textWithDecoration(cr, _model_heading_rect.left(), _model_heading_rect.top(), _("Models"),
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
  while (!done) {
    bool draw_hot_entry = false;
    bounds.pos.x = DOCUMENTS_LEFT_PADDING;
    for (int column = 0; column < entries_per_row; column++) {
      std::size_t index = row * entries_per_row + column;
      if (index >= _filtered_documents.size()) {
        done = true;
        break;
      } else {
        _filtered_documents[index].bounds = bounds;
        if ((std::size_t)_hot_entry == index)
          draw_hot_entry = true;
        else
          draw_entry(cr, _filtered_documents[index], (std::size_t)_hot_entry == index);
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

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::add_document(const std::string &path, const time_t &time, const std::string schemas,
                                    long file_size) {
  DocumentEntry entry;
  entry.path = path;
  entry.timestamp = time;
  entry.schemas = schemas;
  entry.default_handler = std::bind(&DocumentsSection::accessibleHandler, this, std::placeholders::_1, std::placeholders::_2);

  entry.setTitle(base::strip_extension(base::basename(path)));
  if (entry.title.empty())
    entry.title = "???";    //  Don't use setTitle, because it would set the accessibility name to ???, which is an invalid name
  entry.is_model = base::hasSuffix(path, ".mwb") || base::hasSuffix(path, ".mwbd");
  entry.folder = base::dirname(path);

  if (time > 0) {
    struct tm *ptm = localtime(&time);
    char buffer[32];
    strftime(buffer, 32, "%d %b %y, %H:%M", ptm);
    entry.last_accessed = buffer;
  }
  if (file_size == 0)
    entry.size = "--";
  else {
// Format file size in human readable format. 1000 bytes per K on OSX, otherwise 1024.
#ifdef __APPLE__
    double unit_size = 1000;
#else
    double unit_size = 1024;
#endif
    int i = 0;
    double size = file_size;
    const char *units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    while (size > unit_size) {
      size /= unit_size;
      i++;
    }
    entry.size = base::strfmt("%.*f %s", i, size, units[i]);
  }
  _documents.push_back(entry);
  set_layout_dirty(true);
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::clear_documents() {
  _documents.clear();
  set_layout_dirty(true);
}

//----------------------------------------------------------------------------------------------------------------------

bool DocumentsSection::mouse_double_click(mforms::MouseButton button, int x, int y) {
  return this->mouse_click(button, x, y);
}

//----------------------------------------------------------------------------------------------------------------------

bool DocumentsSection::mouse_click(mforms::MouseButton button, int x, int y) {
  switch (button) {
    case mforms::MouseButtonLeft: {
      if (_show_selection_message && _close_button_rect.contains(x, y)) {
        _owner->cancelOperation();
        return true;
      }

      if (_add_button.bounds.contains(x, y)) {
        if (_display_mode != ModelsOnly) {
          _display_mode = ModelsOnly;
          update_filtered_documents();
          set_needs_repaint();
        }

        _owner->trigger_callback(HomeScreenAction::ActionNewEERModel, base::any());
        return true;
      }

      if (_open_button.bounds.contains(x, y)) {
        if (_display_mode != ModelsOnly) {
          _display_mode = ModelsOnly;
          update_filtered_documents();
          set_needs_repaint();
        }

        _owner->trigger_callback(HomeScreenAction::ActionOpenEERModel, base::any());
        return true;
      }

      if (_action_button.bounds.contains(x, y)) {
        if (_display_mode == ModelsOnly && _model_action_menu != NULL)
          _model_action_menu->popup_at(this, x, y);
      }

      if (_model_heading_rect.contains_flipped(x, y)) {
        _owner->cancelOperation();

        if (_display_mode != ModelsOnly) {
          _display_mode = ModelsOnly;
          update_filtered_documents();
          set_needs_repaint();
        }
        return true;
      }

      if (_sql_heading_rect.contains_flipped(x, y)) {
        if (_display_mode != ScriptsOnly) {
          _display_mode = ScriptsOnly;
          update_filtered_documents();
          set_needs_repaint();
        }
        return true;
      }

      if (_mixed_heading_rect.contains_flipped(x, y)) {
        _owner->cancelOperation();

        if (_display_mode != Mixed) {
          _display_mode = Mixed;
          update_filtered_documents();
          set_needs_repaint();
        }
        return true;
      }

      // Anything else.
      _active_entry = entry_from_point(x, y);
      if (_active_entry > -1) {
        _owner->cancelOperation();

        if (_filtered_documents[_active_entry].is_model)
          _owner->trigger_callback(HomeScreenAction::ActionOpenEERModelFromList,
                                   _filtered_documents[_active_entry].path);
        else {
          _pending_script = _filtered_documents[_active_entry].path;
          show_connection_select_message();
        }

        return true;
      }
    } break;

    case mforms::MouseButtonRight: {
      _owner->cancelOperation();

      if (_display_mode == ModelsOnly) {
        _active_entry = entry_from_point(x, y);
        if (_active_entry > -1 && _model_context_menu != NULL) {
          _model_context_menu->popup_at(this, x, y);
          return true;
        }
      }
    } break;

    default:
      break;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool DocumentsSection::mouse_leave() {
  if (_hot_heading != Nothing || _hot_entry > -1) {
    _hot_heading = Nothing;
    _hot_entry = -1;
    set_needs_repaint();
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool DocumentsSection::mouse_move(mforms::MouseButton button, int x, int y) {
  bool result = false;
  ssize_t entry = entry_from_point(x, y);
  if (entry != _hot_entry) {
    _hot_entry = entry;
    result = true;
  }

  if (entry == -1) {
    DisplayMode mode;
    // No document hit, but perhaps one of the titles.
    if (_model_heading_rect.contains_flipped(x, y))
      mode = ModelsOnly;
    else if (_sql_heading_rect.contains_flipped(x, y))
      mode = ScriptsOnly;
    else if (_mixed_heading_rect.contains_flipped(x, y))
      mode = Mixed;
    else
      mode = Nothing;

    if (mode != _hot_heading) {
      _hot_heading = mode;
      result = true;
    }
  }

  if (result)
    set_needs_repaint();

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::handle_command(const std::string &command) {
  if (_active_entry > -1)
    _owner->handleContextMenu(_filtered_documents[_active_entry].path, command);
  else
    _owner->handleContextMenu(base::any(), command);

  _active_entry = -1;
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::show_connection_select_message() {
  _show_selection_message = true;
  set_needs_repaint();
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::hide_connection_select_message() {
  _show_selection_message = false;
  set_needs_repaint();
}

//----------------------------------------------------------------------------------------------------------------------

size_t DocumentsSection::getAccessibilityChildCount() {
  // Initial value due to the add/open/create EER Model icons
  int ret_val = 3;
  ret_val += (int)_filtered_documents.size();

  return ret_val;
}

//----------------------------------------------------------------------------------------------------------------------

base::Accessible* DocumentsSection::getAccessibilityChild(size_t index) {
  base::Accessible* accessible = nullptr;
  switch (index) {
    case 0:
      accessible = &_add_button;
      break;
    case 1:
      accessible = &_open_button;
      break;
    case 2:
      accessible = &_action_button;
      break;
    default: {
      index -= 3;

      if (index < _filtered_documents.size())
        accessible = &_filtered_documents[index];
    }
  }

  return accessible;
}

//----------------------------------------------------------------------------------------------------------------------

base::Accessible::Role DocumentsSection::getAccessibilityRole() {
  return Accessible::List;
}

//----------------------------------------------------------------------------------------------------------------------

base::Accessible* DocumentsSection::accessibilityHitTest(ssize_t x, ssize_t y) {
  base::Accessible* accessible = nullptr;

  if (_add_button.bounds.contains(static_cast<double>(x), static_cast<double>(y)))
    accessible = &_add_button;
  else if (_open_button.bounds.contains(static_cast<double>(x), static_cast<double>(y)))
    accessible = &_open_button;
  else if (_action_button.bounds.contains(static_cast<double>(x), static_cast<double>(y)))
    accessible = &_action_button;
  else {
    ssize_t entry = entry_from_point(static_cast<int>(x), static_cast<int>(y));

    if (entry != -1)
      accessible = &_filtered_documents[entry];
  }

  return accessible;
}

//----------------------------------------------------------------------------------------------------------------------

bool DocumentsSection::accessibleHandler(int x, int y) {
  mouse_move(MouseButtonLeft, x, y);
  return mouse_click(MouseButtonLeft, x, y);
}

//----------------------------------------------------------------------------------------------------------------------

void DocumentsSection::deleteIcons() {
  deleteSurface(_plus_icon);
  deleteSurface(_model_icon);
  deleteSurface(_sql_icon);
  deleteSurface(_schema_icon);
  deleteSurface(_time_icon);
  deleteSurface(_folder_icon);
  deleteSurface(_size_icon);
  deleteSurface(_close_icon);
  deleteSurface(_open_icon);
  deleteSurface(_action_icon);
}

//----------------------------------------------------------------------------------------------------------------------
