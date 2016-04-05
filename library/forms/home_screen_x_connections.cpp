/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/home_screen_x_connections.h"
#include "mforms/menu.h"
#include "mforms/popup.h"
#include "mforms/imagebox.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "mforms/home_screen.h"

DEFAULT_LOG_DOMAIN("home");

using namespace mforms;

inline void delete_surface(cairo_surface_t* surface)
{
  if (surface != NULL)
    cairo_surface_destroy(surface);
}

//--------------------------------------------------------------------------------------------------

class mforms::XConnectionInfoPopup : public mforms::Popup
{
private:
  HomeScreen *_owner;

  base::Rect _free_area;
  int _info_width;

  dataTypes::XProject _project;
  base::Rect _button1_rect;
  base::Rect _button2_rect;
  base::Rect _button3_rect;
  base::Rect _button4_rect;
  base::Rect _close_button_rect;

  cairo_surface_t* _close_icon;

public:
  const int POPUP_HEIGHT = 240;
  const int POPUP_TIP_HEIGHT = 14;
  const int POPUP_LR_PADDING = 53; // Left and right padding.
  const int POPUP_TB_PADDING = 24; // Top and bottom padding.
  const int POPUP_BUTTON_MIN_WIDTH = 88;
  const int POPUP_BUTTON_HEIGHT = 24;
  const int POPUP_BUTTON_SPACING = 19; // Horizontal space between adjacent buttons.
  const int POPUP_BUTTON_PADDING = 11; // Horizontal space between button border and text.

  const int DETAILS_TOP_OFFSET = 44;
  const int DETAILS_LINE_HEIGHT = 18;
  const int DETAILS_LINE_WIDTH = 340;

  XConnectionInfoPopup(HomeScreen *owner, const dataTypes::XProject &project,
                       base::Rect host_bounds, base::Rect free_area, int info_width)
  : Popup(mforms::PopupPlain)
  {
    _owner = owner;
    _project = project;

    _close_icon = mforms::Utilities::load_icon("wb_close.png");

    // Host bounds is the overall size the popup should cover.
    // The free area is a hole in that overall area which should not be covered to avoid darkening it.
    _free_area = free_area;
    _info_width = info_width;
    set_size((int)host_bounds.width(), (int)host_bounds.height());
    show((int)host_bounds.left(), (int)host_bounds.top());
  }

  //------------------------------------------------------------------------------------------------

  ~XConnectionInfoPopup()
  {
    delete_surface(_close_icon);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Draws a button with the given text at the given position. The button width depends on the
   * text. Font face and size are set already.
   * Result is the actual button bounds rectangle we can use for hit tests later.
   */
  base::Rect draw_button(cairo_t *cr, base::Point position, std::string text, bool right_aligned = false)
  {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text.c_str(), &extents);

    base::Rect button_rect = base::Rect(position.x, position.y,
                                        extents.width + 2 * POPUP_BUTTON_PADDING, POPUP_BUTTON_HEIGHT);
    if (button_rect.width() < POPUP_BUTTON_MIN_WIDTH)
      button_rect.size.width = POPUP_BUTTON_MIN_WIDTH;

    if (right_aligned)
      button_rect.pos.x -= button_rect.width();

    button_rect.use_inter_pixel = true;
    cairo_rectangle(cr, button_rect.left(), button_rect.top(), button_rect.width(), button_rect.height());
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);

    double x = (int)(button_rect.left() + (button_rect.width() - extents.width) / 2.0);
    double y = (int)(button_rect.bottom() - (button_rect.height() - extents.height) / 2.0);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text.c_str());
    cairo_stroke(cr);

    return button_rect;
  }

  //------------------------------------------------------------------------------------------------

  void repaint(cairo_t *cr, int x, int y, int w, int h)
  {
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_WINDING);

    base::Rect bounds = get_content_rect();
    cairo_set_line_width(cr, 1);
    cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.width(), bounds.height());

    // Exclude free area by specifying it counterclockwise.
    cairo_move_to(cr, _free_area.left(), _free_area.top());
    cairo_rel_line_to(cr, 0, _free_area.height());
    cairo_rel_line_to(cr, _free_area.width(), 0);
    cairo_rel_line_to(cr, 0, -_free_area.height());
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
    cairo_fill(cr);

    // Determine which side of the free area we can show the popup. We use the lower part as long
    // as there is enough room.
    base::Point tip = base::Point((int)_free_area.xcenter(), 0);
    base::Rect content_bounds = bounds;
    content_bounds.pos.x += POPUP_LR_PADDING;
    content_bounds.size.width = _info_width - 2 *  POPUP_LR_PADDING;
    double right = (int)bounds.left() + _info_width;
    double top;
    if (bounds.bottom() - _free_area.bottom() >= POPUP_HEIGHT + POPUP_TIP_HEIGHT + 2)
    {
      // Below the free area.
      tip.y += (int)_free_area.bottom() + 2;
      top = tip.y + POPUP_TIP_HEIGHT;
      cairo_move_to(cr, bounds.left(), top);
      cairo_line_to(cr, tip.x - POPUP_TIP_HEIGHT, top);
      cairo_line_to(cr, tip.x, tip.y);
      cairo_line_to(cr, tip.x + POPUP_TIP_HEIGHT, top);
      cairo_line_to(cr, right, top);
      cairo_line_to(cr, right, top + POPUP_HEIGHT);
      cairo_line_to(cr, bounds.left(), top + POPUP_HEIGHT);

      cairo_set_source_rgb(cr, 1, 1, 1);
      cairo_fill(cr);

      content_bounds.pos.y = tip.y + POPUP_TIP_HEIGHT;
    }
    else
    {
      // Above the free area.
      tip.y += _free_area.top() - 2;
      top = tip.y - POPUP_TIP_HEIGHT - POPUP_HEIGHT;
      cairo_move_to(cr, bounds.left(), top);
      cairo_line_to(cr, right, top);
      cairo_line_to(cr, right, tip.y - POPUP_TIP_HEIGHT);
      cairo_line_to(cr, tip.x + POPUP_TIP_HEIGHT, tip.y - POPUP_TIP_HEIGHT);
      cairo_line_to(cr, tip.x, tip.y);
      cairo_line_to(cr, tip.x - POPUP_TIP_HEIGHT, tip.y - POPUP_TIP_HEIGHT);
      cairo_line_to(cr, bounds.left(), tip.y - POPUP_TIP_HEIGHT);

      cairo_set_source_rgb(cr, 1, 1, 1);
      cairo_fill(cr);

      content_bounds.pos.y = tip.y - POPUP_TIP_HEIGHT - POPUP_HEIGHT;
    }

    content_bounds.pos.y += POPUP_TB_PADDING;
    content_bounds.size.height = POPUP_HEIGHT - 2 * POPUP_TB_PADDING;

    // The title.
    cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, content_bounds.left(), content_bounds.top() + 16);
    cairo_show_text(cr, _project.name.c_str());
    cairo_stroke(cr);

    // All the various info.
    cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_DETAILS_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_DETAILS_FONT_SIZE);
    print_details_text(cr, content_bounds);

    // Buttons at the bottom.
    base::Point position = base::Point(content_bounds.left(), content_bounds.bottom() - POPUP_BUTTON_HEIGHT);
    _button1_rect = draw_button(cr, position, _("Edit Connection..."));

    // The last button is right-aligned.
    position.x = right - POPUP_LR_PADDING;
    _button4_rect = draw_button(cr, position, _("Connect"), true);


    // Finally the close button.
    base::Size size = mforms::Utilities::getImageSize(_close_icon);
    _close_button_rect = base::Rect(right - size.width - 10, top + 10, size.width, size.height);
    cairo_set_source_surface(cr, _close_icon, _close_button_rect.left(), _close_button_rect.top());
    cairo_paint(cr);
  }
  //------------------------------------------------------------------------------------------------

  /**
   * Prints a single details line with name left aligned in the given bounds and info right aligned.
   * The height value in the given bounds is not set and is ignored. The position is as used by
   * cairo for text output (lower-left corner).
   */
  void print_info_line(cairo_t *cr, base::Rect bounds, std::string name, std::string info)
  {
    if (info.empty())
    info = _("<unknown>");

    cairo_text_extents_t extents;
    cairo_text_extents(cr, info.c_str(), &extents);

    cairo_move_to(cr, bounds.left(), bounds.top());
    cairo_show_text(cr, name.c_str());

    cairo_move_to(cr, bounds.right() - extents.width, bounds.top());
    cairo_show_text(cr, info.c_str());

    cairo_stroke(cr);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Prints all info details for the current connection. Font face, size and color must be set up
   * already.
   */
  void print_details_text(cairo_t *cr, base::Rect bounds)
  {
    // Connection info first.
    base::Rect line_bounds = bounds;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;

    // Use POPUP_LR_PADDIND as space between the two columns.
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    line_bounds.pos.y += DETAILS_LINE_HEIGHT;




    if (_project.connection.ssh.isValid())
      print_info_line(cr, line_bounds, _("Using Tunnel"), _project.connection.ssh.uri());

    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    std::string user_name = _project.connection.userName;
    print_info_line(cr, line_bounds, _("User Account"), user_name);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string password_stored = _("<not stored>");
    std::string password;
    bool find_result = false;
    
    try
    {
      find_result = mforms::Utilities::find_password(_project.connection.uri(), user_name, password);
    }
    catch(std::exception &except)
    {
      logWarning("Exception caught when trying to find a password for '%s' connection: %s\n", _project.name.c_str(), except.what());
    }
    
    if (find_result)
    {
      password = "";
      password_stored = _("<stored>");
    }
    print_info_line(cr, line_bounds, _("Password"), password_stored);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    print_info_line(cr, line_bounds, _("Network Address"), _project.connection.hostName);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    print_info_line(cr, line_bounds, _("TCP/IP Port"), base::to_string(_project.connection.port));


    line_bounds = bounds;
    line_bounds.pos.x += (bounds.width() + POPUP_LR_PADDING) / 2;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    // Make sure the entire right part does not extend beyond the available horizontal space.
    if (line_bounds.right() > bounds.right())
    line_bounds.pos.x -= bounds.right() - line_bounds.right();
  }

  //------------------------------------------------------------------------------------------------

  virtual bool mouse_up(mforms::MouseButton button, int x, int y)
  {
    if (button == mforms::MouseButtonLeft)
    {
      // We are going to destroy ourselves when starting an action, so we have to cache
      // values we need after destruction. The self destruction is also the reason why we
      // use mouse_up instead of mouse_click.
      HomeScreen *owner = _owner;
      dataTypes::XProject project = _project;
//      db_mgmt_ConnectionRef connection = _connection;
//
//      if (_button1_rect.contains(x, y))
//      {
//        set_modal_result(1); // Just a dummy value to close ourselves.
//        owner->handle_context_menu(connection, "manage_connections");
//      }
//      else
//      if (_button2_rect.contains(x, y))
//      {
//        set_modal_result(1);
//        owner->trigger_callback(ActionSetupRemoteManagement, connection);
//      }
//      else
//      if (_button3_rect.contains(x, y))
//      {
//        set_modal_result(1);
//        owner->handle_context_menu(connection, "");
//      }
//      else
      if (_button4_rect.contains(x, y))
      {
        set_modal_result(1);
        owner->openConnection(project);
      }
      else
      if (_close_button_rect.contains(x, y))
      set_modal_result(1);
    }
    return false;
  }
};


//----------------- XConnectionsSection -------------------------------------------------------------

class mforms::XConnectionEntry: mforms::Accessible
{
  friend class XConnectionsSection;
public:
  dataTypes::XProject project;

protected:
  XConnectionsSection *owner;

  std::string title;
  base::Rect titleBounds;
  std::string description;
  base::Rect descriptionBounds;

  bool computeStrings; // True after creation to indicate the need to compute the final display strings.
  bool draw_info_tab;

  boost::function <void (int, int)> default_handler;

  base::Rect bounds;

  // ------ Accesibility Methods -----

  virtual std::string get_acc_name()
  {
    return title;
  }

  virtual std::string get_acc_description()
  {
    return base::strfmt("desc:%s", description.c_str());
  }

  virtual Accessible::Role get_acc_role() { return Accessible::ListItem;}
  virtual base::Rect get_acc_bounds() { return bounds;}
  virtual std::string get_acc_default_action() { return "Open Connection";}

  virtual void do_default_action()
  {
    if (default_handler)
    {
      // Calls the click at the center of the items
      default_handler((int)bounds.center().x, (int)bounds.center().y);
    }
  };

  /**
   * Draws and icon followed by the given text. The given position is that of the upper left corner
   * of the image.
   */
  void draw_icon_with_text(cairo_t *cr, double x, double y, cairo_surface_t *icon,
                                            const std::string &text, double alpha)
  {
    base::Size size = mforms::Utilities::getImageSize(icon);
    if (icon)
    {
      mforms::Utilities::paint_icon(cr, icon, x, y);
      x += size.width + 3;
    }
    double component = 0xF9 / 255.0;
    cairo_set_source_rgba(cr, component, component, component, alpha);

    std::vector<std::string> texts = base::split(text, "\n");

    for (size_t index = 0; index < texts.size(); index++)
    {
      cairo_text_extents_t extents;
      std::string line = texts[index];
      cairo_text_extents(cr, line.c_str(), &extents);

      cairo_move_to(cr, x, (int)(y + size.height / 2.0 + extents.height / 2.0 + (index * (extents.height + 3))));
      cairo_show_text(cr, line.c_str());
      cairo_stroke(cr);
    }
  }

public:
  enum ItemPosition
  {
    First,
    Last,
    Other
  };

  XConnectionEntry(XConnectionsSection *aowner)
  : owner(aowner), computeStrings(false)
  {
    draw_info_tab = true;
  }

  virtual std::string section_name()
  {
    return "";
  }

  virtual bool is_movable()
  {
    return true;
  }

  virtual base::Color getTitleColor()
  {
    return owner->_titleColor;
  }

  virtual base::Color getDescriptionColor()
  {
    return owner->_descriptionColor;
  }

  virtual base::Color getBackgroundColor(bool hot)
  {
    return hot ? owner->_backgroundColorHot : owner->_backgroundColor;
  }

  virtual cairo_surface_t *get_background_icon()
  {
    return nullptr;
  }

  void draw_tile_background(cairo_t *cr, bool hot, double alpha, bool for_dragging)
  {
    base::Color backColor = getBackgroundColor(hot);

    base::Rect bounds = this->bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

    bounds.use_inter_pixel = false;
    cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.width(), bounds.height());
    cairo_set_source_rgba(cr, backColor.red, backColor.green, backColor.blue, alpha);
    cairo_fill(cr);

    // Border.
    bounds.use_inter_pixel = true;
    cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.width() - 1, bounds.height() - 1);
    cairo_set_source_rgba(cr, backColor.red - 0.05, backColor.green - 0.05, backColor.blue - 0.05, alpha);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
    
    cairo_surface_t *back_icon = get_background_icon();
    if (back_icon != nullptr)
    {
      float image_alpha = 0.25;

      // Background icon.
      bounds.use_inter_pixel = false;

      base::Size size = mforms::Utilities::getImageSize(back_icon);
      double x = bounds.left() + bounds.width() - size.width;
      double y = bounds.top() + bounds.height() - size.height;
      cairo_set_source_surface(cr, back_icon, x, y);
      cairo_paint_with_alpha(cr, image_alpha * alpha);
    }
  }

  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging)
  {
    base::Rect bounds = this->bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

    draw_tile_background(cr, hot, alpha, for_dragging);

    if (owner->_xTileIcon != nullptr)
    {
      bounds.use_inter_pixel = false;

      base::Size imageSize = mforms::Utilities::getImageSize(owner->_xTileIcon);
      double y = bounds.top() + (bounds.height() - imageSize.height) / 2;
      mforms::Utilities::paint_icon(cr, owner->_xTileIcon, bounds.left() + 10, y);

      bounds.set_xmin(bounds.left() + 10 + imageSize.width);
    }

    draw_tile_text(cr, bounds, alpha);

    if (hot && owner->_showDetails && draw_info_tab)
    {
#ifdef __APPLE__
      // On OS X we show the usual italic small i letter instead of the peeling corner.
      cairo_select_font_face(cr, HOME_INFO_FONT, CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);

      owner->_info_button_rect = base::Rect(bounds.right() - 15, bounds.bottom() - 10, 10, 10);
      cairo_move_to(cr, owner->_info_button_rect.left(), owner->_info_button_rect.top());
      cairo_show_text(cr, "i");
      cairo_stroke(cr);

#else

      cairo_surface_t *overlay = owner->_mouse_over_icon;
      base::Size imageSize = mforms::Utilities::getImageSize(overlay);
      cairo_set_source_surface(cr, overlay, bounds.left() + bounds.width() - imageSize.width, bounds.top());
      cairo_paint_with_alpha(cr, alpha);

#endif
    }
  }

  virtual void draw_tile_text(cairo_t *cr, base::Rect bounds, double alpha)
  {
    std::string systemFont = base::OSConstants::defaultFontName();
    cairo_select_font_face(cr, systemFont.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

    if (computeStrings)
    {
      // On first render compute the actual string to show and their position. We only need to do this once
      // as neither the available space changes nor is the entry manipulated.
      cairo_text_extents_t extents;

      cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);
      title = mforms::Utilities::shorten_string(cr, title, bounds.width() - 21);

      cairo_text_extents(cr, title.c_str(), &extents);
      titleBounds = base::Rect((int)bounds.left() + 10.5, bounds.top() + ceil(extents.height / 2), ceil(extents.width), ceil(extents.height));

      cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SMALL_INFO_FONT_SIZE);
      description = mforms::Utilities::shorten_string(cr, description, bounds.width() - 21);

      cairo_text_extents(cr, description.c_str(), &extents);
      descriptionBounds = base::Rect((int)bounds.left() + 10.5, bounds.top(), ceil(extents.width), ceil(extents.height));

      titleBounds.pos.y += (bounds.height() - titleBounds.height() - descriptionBounds.height()) / 2;
      descriptionBounds.pos.y = titleBounds.bottom() + 4;

      computeStrings = false;
    }

    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);

    base::Color color = getTitleColor();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, alpha);

    cairo_move_to(cr, titleBounds.left(), titleBounds.top());
    cairo_show_text(cr, title.c_str());
    cairo_stroke(cr);

    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SMALL_INFO_FONT_SIZE);

    color = getDescriptionColor();
    cairo_set_source_rgba(cr, color.red, color.green, color.blue, alpha);

    cairo_move_to(cr, descriptionBounds.left(), descriptionBounds.top());
    cairo_show_text(cr, description.c_str());
    cairo_stroke(cr);
  }


  virtual void activate(std::shared_ptr<XConnectionEntry> conn, int x, int y)
  {
    owner->_owner->openConnection(conn->project);
  }

  virtual mforms::Menu *context_menu()
  {
    return owner->_connection_context_menu;
  }

  virtual void menu_open(ItemPosition pos)
  {
    mforms::Menu *menu = context_menu();

    menu->set_item_enabled(menu->get_item_index("edit_connection"), true);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_group"), true);
    menu->set_item_enabled(menu->get_item_index("delete_connection"), true);
    menu->set_item_enabled(menu->get_item_index("delete_connection_all"), true);

    menu->set_item_enabled(menu->get_item_index("move_connection_to_top"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_up"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_down"), pos != Last);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_end"), pos != Last);
  }

  /**
   * Displays the info popup for the hot entry and enters a quasi-modal-state.
   */
  virtual mforms::XConnectionInfoPopup *show_info_popup()
  {
    mforms::View *parent = owner->get_parent();

    // We have checked in the hit test already that we are on a valid connection object.
    std::pair<int, int> pos = parent->client_to_screen(parent->get_x(), parent->get_y());

    // Stretch the popup window over all 3 sections, but keep the info area in our direct parent's bounds
    base::Rect host_bounds = base::Rect(pos.first, pos.second, parent->get_parent()->get_width(), parent->get_parent()->get_height());

    int width = owner->get_width();
    width -= XConnectionsSection::CONNECTIONS_LEFT_PADDING + XConnectionsSection::CONNECTIONS_RIGHT_PADDING;
    int tiles_per_row = width / (XConnectionsSection::CONNECTIONS_TILE_WIDTH + XConnectionsSection::CONNECTIONS_SPACING);

    XConnectionsSection::XConnectionVector connections(owner->displayed_connections());

    size_t top_entry = std::find(connections.begin(), connections.end(), owner->_hot_entry) - connections.begin();
    size_t row = top_entry / tiles_per_row;
    size_t column = top_entry % tiles_per_row;
    pos.first = (int)(XConnectionsSection::CONNECTIONS_LEFT_PADDING + column * (XConnectionsSection::CONNECTIONS_TILE_WIDTH + XConnectionsSection::CONNECTIONS_SPACING));
    pos.second = (int)(XConnectionsSection::CONNECTIONS_TOP_PADDING + row * (XConnectionsSection::CONNECTIONS_TILE_HEIGHT + XConnectionsSection::CONNECTIONS_SPACING));
    base::Rect item_bounds = base::Rect(pos.first, pos.second, XConnectionsSection::CONNECTIONS_TILE_WIDTH, XConnectionsSection::CONNECTIONS_TILE_HEIGHT);

    int info_width =  parent->get_width();
    if (info_width < 735)
      info_width = (int)host_bounds.width();

    return mforms::manage(new XConnectionInfoPopup(owner->_owner, project, host_bounds, item_bounds, info_width));
  }
};


class mforms::XFolderEntry : public XConnectionEntry
{
protected:
  virtual std::string get_acc_name() override
  {
    return base::strfmt("%s %s", title.c_str(), _("Connection Group"));
  }

public:
  XConnectionsSection::XConnectionVector children;

  XFolderEntry(XConnectionsSection *aowner)
  : XConnectionEntry(aowner)
  {
    draw_info_tab = false;
  }

  virtual void draw_tile_text(cairo_t *cr, base::Rect bounds, double alpha) override
  {
    double component = 0xF9 / 255.0;
    cairo_set_source_rgba(cr, component, component, component, alpha);

    std::string info = base::to_string(children.size() - 1) + " " + _("Connections");
    cairo_move_to(cr, bounds.left(), bounds.top() + 55);
    cairo_show_text(cr, info.c_str());
    cairo_stroke(cr);
  }

  virtual mforms::Menu *context_menu() override
  {
    return owner->_folder_context_menu;
  }

  virtual void menu_open(ItemPosition pos) override
  {
    mforms::Menu *menu = context_menu();

    menu->set_item_enabled(menu->get_item_index("move_connection_to_top"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_up"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_down"), pos != Last);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_end"), pos != Last);
  }

  virtual void activate(std::shared_ptr<XConnectionEntry> thisptr, int x, int y) override
  {
    owner->change_to_folder(std::dynamic_pointer_cast<XFolderEntry>(thisptr));
    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }

  virtual base::Color getBackgroundColor(bool hot) override
  {
    return hot ? owner->_folderBackgroundColorHot : owner->_folderBackgroundColor;
  }

  virtual cairo_surface_t *get_background_icon() override
  {
    return owner->_folder_icon;
  }

  virtual mforms::XConnectionInfoPopup *show_info_popup() override
  {
    return NULL;
  }
};


class mforms::XFolderBackEntry : public XConnectionEntry
{
public:
  XFolderBackEntry(XConnectionsSection *aowner)
  : XConnectionEntry(aowner)
  {
    title = "< back";
  }

  virtual bool is_movable() override
  {
    return false;
  }

  virtual base::Color getBackgroundColor(bool hot) override
  {
    return hot ? owner->_backTileBackgroundColorHot : owner->_backTileBackgroundColor;
  }

  virtual cairo_surface_t *get_background_icon() override
  {
    return owner->_folder_icon;
  }

  /**
   * Separate tile drawing for the special back tile (to return from a folder).
   */
  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging) override
  {
    draw_tile_background(cr, hot, alpha, for_dragging);

    // Title string.
    double x = bounds.left() + 10;
    double y = bounds.top() + 27;
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);
    cairo_set_source_rgb(cr, 0xF9 / 255.0, 0xF9 / 255.0, 0xF9 / 255.0);

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, _("< back"));
    cairo_stroke(cr);
  }

  virtual mforms::Menu *context_menu() override
  {
    return NULL;
  }

  virtual void menu_open(ItemPosition pos) override
  {
  }

  virtual mforms::XConnectionInfoPopup *show_info_popup() override
  {
    return NULL;
  }

  virtual void activate(std::shared_ptr<XConnectionEntry> thisptr, int x, int y) override
  {
    owner->change_to_folder(std::shared_ptr<XFolderEntry>());
    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }
};

//------------------------------------------------------------------------------------------------

XConnectionsSection::XConnectionsSection(HomeScreen *owner) :
    HomeScreenSection("wb_starter_grt_shell_52.png")
{
  _owner = owner;
  _connection_context_menu = NULL;
  _folder_context_menu = NULL;
  _generic_context_menu = NULL;
  _showDetails = false;
  _dragIndex = -1;
  _dropIndex = -1;
  _filtered = false;

  std::vector<std::string> formats;
  formats.push_back(mforms::HomeScreenSettings::TILE_DRAG_FORMAT);           // We allow dragging tiles to reorder them.
  formats.push_back(mforms::DragFormatFileName); // We accept sql script files to open them.
  register_drop_formats(this, formats);

  _folder_icon = mforms::Utilities::load_icon("wb_tile_folder.png");
  _mouse_over_icon = mforms::Utilities::load_icon("wb_tile_mouseover.png");
  _mouse_over2_icon = mforms::Utilities::load_icon("wb_tile_mouseover_2.png");
  // TODO: We need a tile icon for the group filter and the status.
  _plus_icon = mforms::Utilities::load_icon("wb_tile_plus.png");
  _manage_icon = mforms::Utilities::load_icon("wb_tile_manage.png");
  _xTileIcon = mforms::Utilities::load_icon("wb_x_tile.png");
  _info_popup = NULL;

  update_colors();


  set_padding(0, 30, CONNECTIONS_RIGHT_PADDING, 0);

  _accessible_click_handler = boost::bind(&XConnectionsSection::mouse_click, this,
                                          mforms::MouseButtonLeft, _1, _2);

  _addButton.name = "Add Connection";
  _addButton.default_action = "Open New Connection Wizard";
  _addButton.default_handler = _accessible_click_handler;

  _manageButton.name = "Manage Connections";
  _manageButton.default_action = "Open Connection Management Dialog";
  _manageButton.default_handler = _accessible_click_handler;

  _learnButton.name = "Learn more >";
  _learnButton.default_action = "Open learning materials";
  _learnButton.default_handler = _accessible_click_handler;

  _tutorialButton.name = "Browse Tutorial >";
  _tutorialButton.default_action = "Open tutorial materials";
  _tutorialButton.default_handler = _accessible_click_handler;

  _useTraditionalButton.name = "Use traditional MySQL >";
  _useTraditionalButton.default_action = "Open traditional MySQL";
  _useTraditionalButton.default_handler = _accessible_click_handler;
}

//------------------------------------------------------------------------------------------------

XConnectionsSection::~XConnectionsSection()
{
  if (_connection_context_menu != NULL)
    _connection_context_menu->release();
  if (_folder_context_menu != NULL)
    _folder_context_menu->release();
  if (_generic_context_menu != NULL)
    _generic_context_menu->release();

  delete_surface(_folder_icon);
  delete_surface(_mouse_over_icon);
  delete_surface(_mouse_over2_icon);
  delete_surface(_plus_icon);
  delete_surface(_manage_icon);
  delete_surface(_xTileIcon);

  if (_info_popup != NULL)
    delete _info_popup;
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::update_colors()
{
  _titleColor = base::Color::parse("#505050");
  _descriptionColor = base::Color::parse("#A0A0A0");
  _folderTitleColor = base::Color::parse("#F0F0F0");
  _backgroundColor = base::Color::parse("#F4F4F4");
  _backgroundColorHot = base::Color::parse("#D5D5D5");
  _folderBackgroundColor = base::Color::parse("#3477a6");
  _folderBackgroundColorHot = base::Color::parse("#4699b8");
  _backTileBackgroundColor = base::Color::parse("#d9532c");
  _backTileBackgroundColorHot = base::Color::parse("#d97457");

}
//------------------------------------------------------------------------------------------------

/**
 * Computes the index for the given position, regardless if that is actually backed by an existing
 * entry or not.
 *
 * This will not work in section separated folders, but it doesn't matter
 * atm because this is only used for drag/drop
 */
ssize_t XConnectionsSection::calculate_index_from_point(int x, int y)
{
  int width = get_width();
  if (x < CONNECTIONS_LEFT_PADDING || x > (width - CONNECTIONS_RIGHT_PADDING) ||
      y < CONNECTIONS_TOP_PADDING)
    return -1; // Outside the tiles area.

  x -= CONNECTIONS_LEFT_PADDING;
  if ((x % (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING)) > CONNECTIONS_TILE_WIDTH)
    return -1; // Within the horizontal spacing between two tiles.

  y -= CONNECTIONS_TOP_PADDING;
  if ((y % (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING)) > CONNECTIONS_TILE_HEIGHT)
    return -1; // Within the vertical spacing between two tiles.

  width -= CONNECTIONS_LEFT_PADDING + CONNECTIONS_RIGHT_PADDING;
  int tiles_per_row = width / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);
  if (x >= tiles_per_row * (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING))
    return -1; // After the last tile in a row.

  int height = get_height() - CONNECTIONS_TOP_PADDING;
  int column = x / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);
  int row = y / (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING);

  int row_bottom = row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING) + CONNECTIONS_TILE_HEIGHT;
  if (row_bottom > height)
    return -1; // The last visible row is dimmed if not fully visible. So take it out from hit tests too.

  return row * tiles_per_row + column;
}

//------------------------------------------------------------------------------------------------

std::shared_ptr<XConnectionEntry> XConnectionsSection::entry_from_point(int x, int y, bool &in_details_area)
{
  in_details_area = false;
  std::shared_ptr<XConnectionEntry> entry;

  XConnectionVector connections(displayed_connections());
  for (XConnectionVector::iterator conn = connections.begin(); conn != connections.end(); ++conn)
  {
    if ((*conn)->bounds.contains(x, y))
    {
      entry = *conn;
      break;
    }
  }

  if (entry)
  {
    x -= CONNECTIONS_LEFT_PADDING;
    in_details_area = (x % (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING)) > 3 * CONNECTIONS_TILE_WIDTH / 4.0;
  }

  return entry;
}


std::shared_ptr<XConnectionEntry> XConnectionsSection::entry_from_index(ssize_t index)
{
  ssize_t count = displayed_connections().size();
  if (index < count)
  {
    return displayed_connections()[index];
  }
  return std::shared_ptr<XConnectionEntry>();
}

//------------------------------------------------------------------------------------------------

base::Rect XConnectionsSection::bounds_for_entry(ssize_t index)
{
  base::Rect result(CONNECTIONS_LEFT_PADDING, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
  int tiles_per_row = (get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  int column = index % tiles_per_row;
  ssize_t row = index / tiles_per_row;
  result.pos.x += column * (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);
  result.pos.y += row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING);

  return result;
}

//------------------------------------------------------------------------------------------------

/**
 * Returns the connection stored under the given index. Returns an invalid ref if the index
 * describes a folder or back tile.
 * Properly takes into account if we are in a folder or not and if we have filtered entries currently.
 */
dataTypes::XProject XConnectionsSection::projectFromIndex(ssize_t index)
{
  if (index < 0 || (_active_folder && index == 0))
    return dataTypes::XProject();

  return displayed_connections()[index]->project;
}

//------------------------------------------------------------------------------------------------

int XConnectionsSection::drawHeading(cairo_t *cr)
{

  int yoffset = 100;
  cairo_save(cr);
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 3);
  cairo_set_source_rgb(cr, 49 / 255.0, 49 / 255.0, 49 / 255.0);

  std::string heading = "Welcome to MySQL Hybrid";

  cairo_text_extents_t extents;
  cairo_text_extents(cr, heading.c_str(), &extents);
  double x;
  x = get_width()/2 - (extents.width / 2 + extents.x_bearing);
  cairo_move_to(cr, x, yoffset);
  cairo_show_text(cr, heading.c_str());
  yoffset += mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 3;

  std::vector<std::string> description = {"MySQL Hybrid is a cross(x) over between the Relational model and the NoSQL Document model.",
  "Starting with MySQL 5.7 the MySQL Server speaks a new, optimized MySQL X Protocol and the",
  "MySQL Clients offer a brand new X Developer API that aims to deliver the best of both, NoSQL and SQL"};

  for (auto txt : description)
  {
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);
    cairo_text_extents(cr, txt.c_str(), &extents);
    x = get_width()/2 - (extents.width / 2 + extents.x_bearing);
    cairo_move_to(cr, x,  yoffset);
    cairo_show_text(cr, txt.c_str());
    yoffset += extents.height + 10;
  }

  yoffset += 40;

  // Draw heading links
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);

  cairo_set_source_rgb(cr, 0x1b / 255.0, 0xad / 255.0, 0xe8 / 255.0);
  double pos = 0.25;
  for (auto btn : {&_learnButton, &_tutorialButton, &_useTraditionalButton})
  {
    cairo_text_extents(cr, btn->name.c_str(), &extents);
    x = get_width() * pos - (extents.width / 2 + extents.x_bearing);
    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, btn->name.c_str());
    btn->bounds = base::Rect(x, yoffset, x + extents.width, yoffset + extents.height);
    pos += 0.25;
  }

  cairo_restore(cr);

  yoffset += 60;
  return yoffset;
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
{
  updateHeight();

  int yoffset = drawHeading(cr);

  int width = get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING;

  int tiles_per_row = width / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

  cairo_set_source_rgb(cr, 49 / 255.0, 49 / 255.0, 49 / 255.0);
  cairo_move_to(cr, CONNECTIONS_LEFT_PADDING, yoffset);

  XConnectionVector *connections;
  std::string title = _("MySQL X Projects");
  if (_active_folder)
  {
    title += " / " + _active_folder->title;
    connections = &_active_folder->children;
  }
  else
    connections = &_connections;

  if (_filtered)
    connections = &_filtered_connections;

  cairo_show_text(cr, title.c_str());

  // The + button after the title.
  cairo_text_extents_t extents;
  cairo_text_extents(cr, title.c_str(), &extents);
  double text_width = ceil(extents.width);

  base::Size size = mforms::Utilities::getImageSize(_plus_icon);
  _addButton.bounds = base::Rect(CONNECTIONS_LEFT_PADDING + text_width + 10, yoffset - size.height, size.width, size.height);

  cairo_set_source_surface(cr, _plus_icon, _addButton.bounds.left(), _addButton.bounds.top());
  cairo_paint(cr);

  size = mforms::Utilities::getImageSize(_manage_icon);
  _manageButton.bounds = base::Rect(_addButton.bounds.right() + 10, yoffset - size.height, size.width, size.height);
  cairo_set_source_surface(cr, _manage_icon, _manageButton.bounds.left(), _manageButton.bounds.top());
  cairo_paint(cr);

  int row = 0;
  // number of tiles that act as a filler
  int filler_tiles = 0;
  std::string current_section;
  base::Rect bounds(0, yoffset + 25, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
  std::size_t index = 0;
  bool done = false;
  while (!done)
  {
    if (index >= connections->size())
      break; //we're done

    bounds.pos.x = CONNECTIONS_LEFT_PADDING;
    for (int column = 0; column < tiles_per_row; column++)
    {

        {
          std::string section = (*connections)[index]->section_name();
          if (!section.empty() && current_section != section)
          {
            current_section = section;
            bounds.pos.y += mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE + CONNECTIONS_SPACING;

            {
              // draw the section title
              cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
              cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);
              cairo_set_source_rgb(cr, 49 / 255.0, 49 / 255.0, 49 / 255.0);
              cairo_text_extents(cr, current_section.c_str(), &extents);
              cairo_move_to(cr, CONNECTIONS_LEFT_PADDING,
                bounds.pos.y - (extents.height + extents.y_bearing) - 4);
              cairo_show_text(cr, current_section.c_str());
            }
          }

          // if the name of the next section is different, then we add some filler space after this tile
          if (!current_section.empty() && (size_t)index < (*connections).size() - 1 &&
            (*connections)[index + 1]->section_name() != current_section)
          {
            int tiles_occupied = tiles_per_row - column;
            filler_tiles += tiles_occupied;
            column += (tiles_occupied - 1);
          }

          // Updates the bounds on the tile
          (*connections)[index]->bounds = bounds;

        }

        {
          bool draw_hot = (*connections)[index] == _hot_entry;
          (*connections)[index]->draw_tile(cr, draw_hot, 1.0, false);

          // Draw drop indicator.

          // This shouldn't be a problem as I don't think there will be more than that many connections.
          if ((ssize_t)index == _dropIndex)
          {
            cairo_set_source_rgb(cr, 0, 0, 0);

            if (_dropPosition == mforms::DropPositionOn)
            {
              double x = bounds.left() - 4;
              double y = bounds.ycenter();
              cairo_move_to(cr, x, y - 15);
              cairo_line_to(cr, x + 15, y);
              cairo_line_to(cr, x, y + 15);
              cairo_fill(cr);
            }
            else
            {
              double x = bounds.left() - 4.5;
              if (_dropPosition == mforms::DropPositionRight)
                x = bounds.right() + 4.5;
              cairo_move_to(cr, x, bounds.top());
              cairo_line_to(cr, x, bounds.bottom());
              cairo_set_line_width(cr, 3);
              cairo_stroke(cr);
              cairo_set_line_width(cr, 1);
            }
          }
        }
      index++;
      bounds.pos.x += CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING;
      if (index >= connections->size())
      {
        done = true; //we're done
        break;
      }
    }

    row++;
    bounds.pos.y += CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING;
  }



}

//--------------------------------------------------------------------------------------------------

int XConnectionsSection::calculateHeight()
{
  XConnectionVector *connections;
  if (_active_folder)
    connections = &_active_folder->children;
  else
    connections = &_connections;

  if (_filtered)
    connections = &_filtered_connections;
  int tiles_per_row = (get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  if (connections->empty() || tiles_per_row <=1 )
    return 0;

  return (connections->size() / tiles_per_row) * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING) + CONNECTIONS_TOP_PADDING;
}

//------------------------------------------------------------------------------------------------

std::shared_ptr<XFolderEntry> XConnectionsSection::createFolder(const dataTypes::ProjectHolder &holder)
{
  std::shared_ptr<XFolderEntry> folder(new XFolderEntry(this));
  folder->title = holder.name;
  folder->computeStrings = true;
  folder->children.push_back(std::shared_ptr<XConnectionEntry>(new XFolderBackEntry(this)));
  return folder;
}

//------------------------------------------------------------------------------------------------

std::shared_ptr<XConnectionEntry> XConnectionsSection::createConnection(const dataTypes::XProject &project)
{
  std::shared_ptr<XConnectionEntry> entry = std::shared_ptr<XConnectionEntry>(new XConnectionEntry(this));
  entry->project = project;
  entry->title = _("X Session with ") + entry->project.name;

  entry->description = _("Connection URI: ");
  switch(entry->project.connection.language)
  {
  case dataTypes::EditorJavaScript:
    entry->description += "js:///";
    break;
  case dataTypes::EditorPython:
    entry->description += "py:///";
    break;
  case dataTypes::EditorSql:
    entry->description += "py:///";
    break;
  }
  entry->description += entry->project.connection.hostName + ":" + std::to_string(entry->project.connection.port);
  entry->computeStrings = true;

  entry->default_handler = boost::bind(&XConnectionsSection::mouse_click, this, mforms::MouseButtonLeft, _1, _2);
  return entry;
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::updateHeight()
{
  int height = 355; // The top section + project list heading.

  XConnectionVector *connections;
  if (_active_folder)
    connections = &_active_folder->children;
  else
    connections = &_connections;

  if (_filtered)
    connections = &_filtered_connections;
  float tilesPerRow = (get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  if (tilesPerRow > 1)
  {
    int rowCount = ceil(connections->size() / tilesPerRow);
    if (rowCount > 0)
      height += rowCount * CONNECTIONS_TILE_HEIGHT + (rowCount - 1 ) * CONNECTIONS_SPACING;
    if (height != get_height())
        set_size(-1, height);
  }
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::cancelOperation()
{
  // pass
}

void XConnectionsSection::setFocus()
{
  // pass
}

//------------------------------------------------------------------------------------------------

bool XConnectionsSection::canHandle(HomeScreenMenuType type)
{
  return false;
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::setContextMenu(mforms::Menu *menu, HomeScreenMenuType type)
{
  // pass
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type)
{
  // pass
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::loadProjects(const dataTypes::ProjectHolder &holder)
{
    loadProjects(holder, _connections);
    updateHeight();

    set_layout_dirty(true);
    set_needs_repaint();
}


void XConnectionsSection::loadProjects(const dataTypes::ProjectHolder &holder, XConnectionVector &children)
{
  if (holder.children.empty() && holder.project.isValid())
  {
    auto entry = createConnection(holder.project);
    children.push_back(entry);
  }
  else
  {
    for (auto it : holder.children)
    {
      if (!it.isGroup)
      {
        auto entry = createConnection(it.project);
        children.push_back(entry);
      }
      else
      {
        auto folder = createFolder(it);
        loadProjects(it, folder->children);
        children.push_back(folder);
      }
    }
  }
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::clear_connections(bool clear_state)
{
  if (clear_state)
  {
    _filtered = false;
    _filtered_connections.clear();
    _active_folder_title_before_refresh_start = "";
  }
  else
  {
    if (_active_folder)
      _active_folder_title_before_refresh_start = _active_folder->title;
  }
  _entry_for_menu.reset();
  _active_folder.reset();
  _connections.clear();

  set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::change_to_folder(std::shared_ptr<XFolderEntry> folder)
{
  if (_active_folder && !folder)
  {
    // Returning to root list.
    _active_folder.reset();
    _filtered = false;
    set_needs_repaint();
  }
  else if (folder)
  {
    // Drilling into a folder.
    _active_folder = folder;
    _filtered = false;
    set_needs_repaint();
  }
}

//--------------------------------------------------------------------------------------------------

bool XConnectionsSection::mouse_down(mforms::MouseButton button, int x, int y)
{
  if (button == mforms::MouseButtonLeft && _hot_entry)
    _mouse_down_position = base::Rect(x - 4, y - 4, 8, 8); // Center a 8x8 pixels rect around the mouse position.
  return false; // Continue with standard mouse handling.
}

//--------------------------------------------------------------------------------------------------

bool XConnectionsSection::mouse_up(mforms::MouseButton button, int x, int y)
{
  _mouse_down_position = base::Rect();
  return false;
}

//--------------------------------------------------------------------------------------------------

bool XConnectionsSection::mouse_double_click(mforms::MouseButton button, int x, int y)
{
  return false;
}

//--------------------------------------------------------------------------------------------------

bool XConnectionsSection::mouse_click(mforms::MouseButton button, int x, int y)
{
  // everything below this relies on _hot_entry, which will become out of sync
  // if the user pops up the context menu and then clicks (or right clicks) in some
  // other tile... so we must first update _hot_entry before doing any actions
  mouse_move(mforms::MouseButtonNone, x, y);

  switch (button)
  {
    case mforms::MouseButtonLeft:
    {
      if (_addButton.bounds.contains(x, y))
      {
        _owner->trigger_callback(HomeScreenAction::ActionNewXConnection, base::any());
        return true;
      }

      if (_manageButton.bounds.contains(x, y))
      {
        _owner->trigger_callback(HomeScreenAction::ActionManageXConnections, base::any());
        return true;
      }

      if (_learnButton.bounds.contains(x, y))
      {
        _owner->trigger_callback(HomeScreenAction::ActionOpenXLearnMore, base::any());
        return true;
      }

      if (_tutorialButton.bounds.contains(x, y))
      {
        _owner->trigger_callback(HomeScreenAction::ActionOpenXTutorial, base::any());
        return true;
      }

      if (_useTraditionalButton.bounds.contains(x, y))
      {
        _owner->trigger_callback(HomeScreenAction::ActionOpenXTraditional, base::any());
        return true;
      }

      if (_hot_entry)
      {
#ifdef __APPLE__
        bool show_info = _info_button_rect.contains_flipped(x, y);
#else
        bool show_info = _showDetails;
#endif

        if (show_info && !_info_popup && _parent && (_info_popup = _hot_entry->show_info_popup()))
        {
          scoped_connect(_info_popup->on_close(), boost::bind(&XConnectionsSection::popup_closed, this));

          return true;
        }

        _hot_entry->activate(_hot_entry, x, y);

        return true;
      }
    }
      break;

    case mforms::MouseButtonRight:
    {
      mforms::Menu *context_menu = NULL;

      if (_hot_entry)
      {
        context_menu = _hot_entry->context_menu();

        _entry_for_menu = _hot_entry;
      }
      else
        context_menu = _generic_context_menu;

      // At this point the context menu and the associated entry have been selected
      if (context_menu)
        context_menu->popup_at(this, x, y);
    }
      break;

    default:
      break;
  }

  return false;
}

//------------------------------------------------------------------------------------------------

bool XConnectionsSection::mouse_leave()
{
  // Ignore mouse leaves if we are showing the info popup. We want the entry to stay hot.
  if (_info_popup != NULL)
    return true;

  if (_hot_entry)
  {
    _hot_entry.reset();
    _showDetails = false;
    set_needs_repaint();
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool XConnectionsSection::mouse_move(mforms::MouseButton button, int x, int y)
{
  bool in_details_area;
  std::shared_ptr<XConnectionEntry> entry = entry_from_point(x, y, in_details_area);

  if (entry && !_mouse_down_position.empty() && (!_mouse_down_position.contains(x, y)))
  {
    if (!entry->is_movable())
    {
      _mouse_down_position = base::Rect();
      return true;
    }

    if (button == mforms::MouseButtonNone) // Cancel drag if the mouse button was released.
      return true;

    return do_tile_drag(calculate_index_from_point(x, y), x, y);
  }
  else
  {
    // Only do hit tracking if no mouse button is pressed to avoid situations like
    // mouse down outside any tile, drag over a tile, release mouse button -> click
    // (or hover effects in general).
    if (button == mforms::MouseButtonNone)
    {
      if (entry != _hot_entry || _showDetails != in_details_area)
      {
        _hot_entry = entry;
#ifndef __APPLE__
        if (_hot_entry)
          _showDetails = in_details_area;
#else
        _showDetails = true;
#endif
        set_needs_repaint();
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::handle_command(const std::string &command)
{
  dataTypes::XProject *project = nullptr;
  if (_entry_for_menu)
  {
    if (_active_folder)
    {
      if (command == "delete_connection_all")
      {
        // We only want to delete all connections in the active group. This is the same as
        // removing the group entirely, since the group is formed by connections in it.
        _entry_for_menu = _active_folder;
        handle_folder_command("delete_connection_group");
        return;
      }
      else
      {
        project  = &_entry_for_menu->project;
      }
    }
    else
    {
      project  = &_entry_for_menu->project;
    }
  }

  _owner->handle_context_menu(project, command);
  _entry_for_menu.reset();
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::handle_folder_command(const std::string &command)
{
  {
    // We have to pass on a valid connection (for the group name).
    // All child items have the same group name (except the dummy entry for the back tile).
    std::string title;
    if (_entry_for_menu)
      title = _entry_for_menu->title;

    title += "/";

    _owner->handle_context_menu(title, command);
    _entry_for_menu.reset();
  }

}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::menu_open()
{
  if (_entry_for_menu)
  {
    XConnectionVector &items(displayed_connections());
  
    if (items.empty())
      _entry_for_menu->menu_open(XConnectionEntry::Other);
    else if (items.front() == _entry_for_menu)
      _entry_for_menu->menu_open(XConnectionEntry::First);
    else if (items.back() == _entry_for_menu)
      _entry_for_menu->menu_open(XConnectionEntry::Last);
    else
      _entry_for_menu->menu_open(XConnectionEntry::Other);
  }
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::hide_info_popup()
{
  if (_info_popup != NULL)
  {
    _hot_entry.reset();
    _showDetails = false;

    _info_popup->release();
    _info_popup = NULL;

    set_needs_repaint();
  }
}

//------------------------------------------------------------------------------------------------

void XConnectionsSection::popup_closed()
{
  hide_info_popup();
}

//------------------------------------------------------------------------------------------------

int XConnectionsSection::get_acc_child_count()
{
  // At least 2 is returned because of the add and manage icons.
  size_t ret_val = 2;


  if (_filtered)
    ret_val += (int)_filtered_connections.size();
  else if (!_active_folder)
    ret_val += (int)_connections.size();
  else
  {
    // Adds one because of the back tile
    ret_val++;
    ret_val += _active_folder->children.size();
  }

  return (int)ret_val;
}

mforms::Accessible* XConnectionsSection::get_acc_child(int index)
{
  mforms::Accessible* accessible = NULL;

  switch (index)
  {
    case 0:
      accessible = &_addButton;
      break;
    case 1:
      accessible = &_manageButton;
      break;
    default:
    {
      // Removes 2 to get the real connection index.
      // Note that if at this point index is bigger than the list
      // size, it means it is referring to the pageup/pagedown icons.
      index -= 2;

      if (_filtered)
      {
        if (index < (int)_filtered_connections.size())
          accessible = _filtered_connections[index].get();
        else
          index -= (int)_filtered_connections.size();
      }
      else
      {
        if (!_active_folder)
        {
          if (index < (int)_connections.size())
            accessible = _connections[index].get();
          else
            index -= (int)_connections.size();
        }
        else
        {
          if (index < (int)_active_folder->children.size())
            accessible = _active_folder->children[index].get();
          else
            index -= (int)_active_folder->children.size();
        }
      }
    }
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

std::string XConnectionsSection::get_acc_name()
{
  return get_name();
}

//------------------------------------------------------------------------------------------------

mforms::Accessible::Role XConnectionsSection::get_acc_role()
{
  return Accessible::List;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible* XConnectionsSection::hit_test(int x, int y)
{
  mforms::Accessible* accessible = NULL;

  if (_addButton.bounds.contains(x, y))
    accessible = &_addButton;
  else if (_manageButton.bounds.contains(x, y))
    accessible = &_manageButton;
  else
  {
    bool in_details_area = false;
    std::shared_ptr<XConnectionEntry> entry = entry_from_point(x, y, in_details_area);

    if (entry)
      accessible = entry.get();
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

bool XConnectionsSection::do_tile_drag(ssize_t index, int x, int y)
{
  _hot_entry.reset();
  set_needs_repaint();

  if (index >= 0)
  {
    mforms::DragDetails details;
    details.allowedOperations = mforms::DragOperationMove;
    details.location = base::Point(x, y);

    details.image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
    cairo_t *cr = cairo_create(details.image);
    base::Rect bounds = bounds_for_entry(index);
    details.hotspot.x = x - bounds.pos.x;
    details.hotspot.y = y - bounds.pos.y;

    // We know we have no back tile here.
    std::shared_ptr<XConnectionEntry> entry = entry_from_index(index);
    if (entry)
    {
      entry->draw_tile(cr, false, 1, true);

      _dragIndex = index;
      mforms::DragOperation operation = do_drag_drop(details, entry.get(), mforms::HomeScreenSettings::TILE_DRAG_FORMAT);
      _mouse_down_position = base::Rect();
      cairo_surface_destroy(details.image);
      cairo_destroy(cr);

      _dragIndex = -1;
      _dropIndex = -1;
      set_needs_repaint();

      if (operation == mforms::DragOperationMove) // The actual move is done in the drop delegate method.
        return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------------------------

// Drop delegate implementation.
mforms::DragOperation XConnectionsSection::drag_over(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                const std::vector<std::string> &formats)
{
  if (allowedOperations == mforms::DragOperationNone)
    return allowedOperations;

  if (std::find(formats.begin(), formats.end(), mforms::DragFormatFileName) != formats.end())
  {
    // Indicate we can accept files if one of the connection tiles is hit.
    bool in_details_area;
    std::shared_ptr<XConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y, in_details_area);

    if (!entry)
      return mforms::DragOperationNone;

    if (!entry->project.isValid())
      return mforms::DragOperationNone;

    if (_hot_entry != entry)
    {
      _hot_entry = entry;
      set_needs_repaint();
    }
    return allowedOperations & mforms::DragOperationCopy;
  }

  if (std::find(formats.begin(), formats.end(), mforms::HomeScreenSettings::TILE_DRAG_FORMAT) != formats.end())
  {
    // A tile is being dragged. Find the target index and drop location for visual feedback.
    // Computation here is more relaxed than the normal hit test as we want to allow dropping
    // left, right and below the actual tiles area too as well as between tiles.
    if (p.y < CONNECTIONS_TOP_PADDING)
    {
      if (_dropIndex > -1)
      {
        _dropIndex = -1;
        set_needs_repaint();
      }
      return mforms::DragOperationNone;
    }

    p.y -= CONNECTIONS_TOP_PADDING;

    int count = (int)_connections.size();
    if (_filtered)
      count = (int)_filtered_connections.size(); // For both, main list or folder.
    else if (_active_folder)
      count = (int)_active_folder->children.size();

    int width = get_width();
    int tiles_per_row = (width - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) /
    (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

    int column = -1; // Left to the first column.
    if (p.x > (width - CONNECTIONS_RIGHT_PADDING))
    column = tiles_per_row; // After last column.
    else
    if (p.x >= CONNECTIONS_LEFT_PADDING)
    column = (int)((p.x - CONNECTIONS_LEFT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING));

    int row = (int)(p.y / (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING));

    int row_bottom = row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING) + CONNECTIONS_TILE_HEIGHT;
    if (row_bottom > get_height())
    {
      if (_dropIndex > -1)
      {
        _dropIndex = -1;
        set_needs_repaint();
      }
      return mforms::DragOperationNone; // Drop on the dimmed row. No drop action here.
    }

    int index = (int)(row * tiles_per_row);
    if (column == tiles_per_row)
      index += column - 1;
    else
      if (column > -1)
        index += column;

    mforms::DropPosition position = mforms::DropPositionLeft;
    if (column == tiles_per_row)
      position = mforms::DropPositionRight;
    else
    {
      if (index >= count)
      {
        index = count - 1;
        position = mforms::DropPositionRight;
      }
      else
      {
        // Tile hit. Depending on which side of the tile's center the mouse is use a position
        // before or after that tile. Back tiles have no "before" position, but only "on" or "after".
        // Folder tiles have "before", "on" and "after" positions. Connection tiles only have "before"
        // and "after".
        base::Rect bounds = bounds_for_entry(index);
        std::shared_ptr<XConnectionEntry> entry = entry_from_index(index);
        if (entry && dynamic_cast<XFolderEntry*>(entry.get()))
        {
          // In a group take the first third as hit area for "before", the second as "on" and the
          // last one as "after".
          if (p.x > bounds.left() + bounds.width() / 3)
          {
            if (p.x > bounds.right() - bounds.width() / 3)
              position = mforms::DropPositionRight;
            else
              position = mforms::DropPositionOn;
          }
        }
        else
        {
          if (p.x > bounds.xcenter())
            position = mforms::DropPositionRight;
        }
      }
    }

    // Check that the drop position does not resolve to the dragged item.
    // Don't allow dragging a group on a group either.
    if (_dragIndex > -1 && (index == _dragIndex ||
        (index + 1 == _dragIndex && position == mforms::DropPositionRight) ||
        (index - 1 == _dragIndex && position == mforms::DropPositionLeft) ||
        (position == mforms::DropPositionOn && dynamic_cast<XFolderEntry*>(entry_from_index(_dragIndex).get()))))
    {
      index = -1;
    }
    else if (!_filtered && _active_folder && index == 0 && position == mforms::DropPositionLeft)
    {
        position = mforms::DropPositionOn; // Drop on back tile.
    }

    if (_dropIndex != index || _dropPosition != position)
    {
      _dropIndex = index;
      _dropPosition = position;
      set_needs_repaint();
    }

    return mforms::DragOperationMove;
  }

  return mforms::DragOperationNone;
}

//------------------------------------------------------------------------------------------------

mforms::DragOperation XConnectionsSection::files_dropped(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                    const std::vector<std::string> &file_names)
{
  bool in_details_area;
  std::shared_ptr<XConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y, in_details_area);
  if (!entry)
    return mforms::DragOperationNone;

  dataTypes::XProject project = entry->project;
  if (project.isValid())
  {
    std::vector<std::string> files;
    for (size_t i = 0; i < file_names.size(); ++i)
      if (base::tolower(base::extension(file_names[i])) == ".sql")
        files.push_back(file_names[i]);

    if (files.size() == 0)
    return mforms::DragOperationNone;

    //TODO: implement this, once NG will allow to open files from cmd line
//    HomeScreenDropFilesInfo dInfo;

//    grt::DictRef details(grt);
//    details.set("connection", connection);
//    details.set("files", valid_names);
//    _owner->trigger_callback(ActionFilesWithConnection, details);
  }

  return mforms::DragOperationCopy;
}

//------------------------------------------------------------------------------------------------

mforms::DragOperation XConnectionsSection::data_dropped(mforms::View *sender, base::Point p,
                                   mforms::DragOperation allowedOperations, void *data, const std::string &format)
{
  if (format == mforms::HomeScreenSettings::TILE_DRAG_FORMAT && _dropIndex > -1)
  {
    mforms::DragOperation result = mforms::DragOperationNone;

    // Can be invalid if we move a group.
    XConnectionEntry *source_entry = static_cast<XConnectionEntry*>(data);

    std::shared_ptr<XConnectionEntry> entry;
    if (_filtered)
    {
      if (_dropIndex < (int)_filtered_connections.size())
        entry = _filtered_connections[_dropIndex];
    }
    else if (_active_folder)
    {
      if (_dropIndex < (int)_active_folder->children.size())
        entry = _active_folder->children[_dropIndex];
    }
    else
    {
      if (_dropIndex < (int)_connections.size())
        entry = _connections[_dropIndex];
    }

    if (!entry)
      return result;

    bool is_back_tile = entry->title == "< back";

    // Drop target is a group.
    HomeScreenDropInfo dropInfo;
    dropInfo.value = source_entry->title + "/";

    if (_dropPosition == mforms::DropPositionOn)
    {
      // Drop on a group (or back tile).
      if (is_back_tile)
        dropInfo.group = "*Ungrouped*";
      else
        dropInfo.group = entry->title;

      _owner->trigger_callback(HomeScreenAction::ActionMoveConnectionToGroup, dropInfo);
    }
    else
    {
      // Drag from one position to another within a group (root or active group).
      size_t to = _dropIndex;
      if (_active_folder)
        to--; // The back tile has no representation in the global list.
      if (_dropPosition == mforms::DropPositionRight)
        to++;
      dropInfo.to = to;
      _owner->trigger_callback(HomeScreenAction::ActionMoveConnection, dropInfo);
    }
    result = mforms::DragOperationMove;

    _dropIndex = -1;
    set_needs_repaint();

    return result;
  }
  return mforms::DragOperationNone;
}

XConnectionsSection::XConnectionVector &XConnectionsSection::displayed_connections()
{
  if (_filtered)
    return _filtered_connections;
  else if (_active_folder)
    return _active_folder->children;
  else
    return _connections;
}
