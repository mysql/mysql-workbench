/*
 * Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/home_screen_connections.h"

#include "mforms/menu.h"
#include "mforms/popup.h"
#include "mforms/imagebox.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "base/any.h"

DEFAULT_LOG_DOMAIN("home");

using namespace mforms;

//--------------------------------------------------------------------------------------------------

class mforms::ConnectionInfoPopup : public mforms::Popup {
private:
  ConnectionsSection *_owner;

  base::Rect _free_area;
  int _info_width;
  std::string _connectionId;

  base::Rect _button1_rect;
  base::Rect _button2_rect;
  base::Rect _button3_rect;
  base::Rect _button4_rect;
  base::Rect _close_button_rect;

  cairo_surface_t *_close_icon;

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

  ConnectionInfoPopup(ConnectionsSection *owner, const std::string connectionId, base::Rect host_bounds,
                      base::Rect free_area, int info_width)
    : Popup(mforms::PopupPlain) {
    _owner = owner;
    _connectionId = connectionId;

    _close_icon = mforms::Utilities::load_icon("home_screen_close.png");

    // Host bounds is the overall size the popup should cover.
    // The free area is a hole in that overall area which should not be covered to avoid darkening it.
    _free_area = free_area;
    _info_width = info_width;
    set_size((int)host_bounds.width(), (int)host_bounds.height());
    show((int)host_bounds.left(), (int)host_bounds.top());
  }

  //------------------------------------------------------------------------------------------------

  ~ConnectionInfoPopup() {
    deleteSurface(_close_icon);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Draws a button with the given text at the given position. The button width depends on the
   * text. Font face and size are set already.
   * Result is the actual button bounds rectangle we can use for hit tests later.
   */
  base::Rect draw_button(cairo_t *cr, base::Point position, std::string text, bool right_aligned = false) {
    cairo_text_extents_t extents;
    cairo_text_extents(cr, text.c_str(), &extents);

    base::Rect button_rect =
      base::Rect(position.x, position.y, extents.width + 2 * POPUP_BUTTON_PADDING, POPUP_BUTTON_HEIGHT);
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

  void repaint(cairo_t *cr, int x, int y, int w, int h) {
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

    cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
    cairo_fill(cr);

    // Determine which side of the free area we can show the popup. We use the lower part as long
    // as there is enough room.
    base::Point tip = base::Point((int)_free_area.xcenter(), 0);
    base::Rect content_bounds = bounds;
    content_bounds.pos.x += POPUP_LR_PADDING;
    content_bounds.size.width = _info_width - 2 * POPUP_LR_PADDING;
    double right = (int)bounds.left() + _info_width;
    double top;
    if (bounds.bottom() - _free_area.bottom() >= POPUP_HEIGHT + POPUP_TIP_HEIGHT + 2) {
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
    } else {
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

    auto connectionInfo = _owner->getConnectionInfoCallback(_connectionId);

    // The title.
    cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_move_to(cr, content_bounds.left(), content_bounds.top() + 16);
    cairo_show_text(cr, connectionInfo["name"].as<std::string>().c_str());
    cairo_stroke(cr);

    // All the various info.
    cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_DETAILS_FONT, CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_DETAILS_FONT_SIZE);
    print_details_text(cr, content_bounds);

    // Buttons at the bottom.
    base::Point position = base::Point(content_bounds.left(), content_bounds.bottom() - POPUP_BUTTON_HEIGHT);
    _button1_rect = draw_button(cr, position, _("Edit Connection..."));

    bool pending = false;
    if (!connectionInfo["serverInfo"].isNull()) {
      mforms::anyMap serverInfo = connectionInfo["serverInfo"];
      pending = getAnyMapValueAs<ssize_t>(serverInfo, "setupPending") == 1;
      if (!pending && !connectionInfo["isLocalConnection"].as<bool>() &&
          getAnyMapValueAs<ssize_t>(serverInfo, "remoteAdmin") == 0 &&
          getAnyMapValueAs<ssize_t>(serverInfo, "windowsAdmin") == 0)
        pending = true;
    } else
      pending = true;

    if (pending) {
      position.x += _button1_rect.width() + POPUP_BUTTON_SPACING;
      if (connectionInfo["isLocalConnection"].as<bool>())
        _button2_rect = draw_button(cr, position, _("Configure Local Management..."));
      else
        _button2_rect = draw_button(cr, position, _("Configure Remote Management..."));
    } else
      _button2_rect = base::Rect();

    // The last button is right-aligned.
    position.x = right - POPUP_LR_PADDING;
    _button4_rect = draw_button(cr, position, _("Connect"), true);

    // Finally the close button.
    _close_button_rect =
      base::Rect(right - imageWidth(_close_icon) - 10, top + 10, imageWidth(_close_icon), imageHeight(_close_icon));
    cairo_set_source_surface(cr, _close_icon, _close_button_rect.left(), _close_button_rect.top());

    cairo_paint(cr);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Prints a single details line with name left aligned in the given bounds and info right aligned.
   * The height value in the given bounds is not set and is ignored. The position is as used by
   * cairo for text output (lower-left corner).
   */
  void print_info_line(cairo_t *cr, base::Rect bounds, std::string name, std::string info) {
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
  void print_details_text(cairo_t *cr, base::Rect bounds) {
    // Connection info first.
    base::Rect line_bounds = bounds;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;

    // Use POPUP_LR_PADDIND as space between the two columns.
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    mforms::anyMap connectionInfo = _owner->getConnectionInfoCallback(_connectionId);

    mforms::anyMap serverInfo;
    if (!connectionInfo["serverInfo"].isNull())
      serverInfo = getAnyMapValueAs<mforms::anyMap>(connectionInfo, "serverInfo");

    std::string server_version = getAnyMapValueAs<std::string>(connectionInfo, "serverVersion");

    print_info_line(cr, line_bounds, _("MySQL Version"), server_version);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    if (connectionInfo.find("lastConnected") != connectionInfo.end()) {
      time_t time = connectionInfo["lastConnected"].as<ssize_t>();
      if (time == 0)
        print_info_line(cr, line_bounds, _("Last connected"), "");
      else {
        struct tm *ptm = localtime(&time);
        char buffer[32];
        strftime(buffer, 32, "%d %B %Y %H:%M", ptm);
        print_info_line(cr, line_bounds, _("Last connected"), buffer);
      }
      line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    }

    if (connectionInfo.find("sshHost") != connectionInfo.end()) {
      std::string sshHost = connectionInfo["sshHost"];
      if (!sshHost.empty()) {
        std::string sshUser = connectionInfo["sshUserName"];
        print_info_line(cr, line_bounds, _("Using Tunnel"), sshUser + "@" + sshHost);
      }
    }

    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string user_name = getAnyMapValueAs<std::string>(connectionInfo, "userName");

    print_info_line(cr, line_bounds, _("User Account"), user_name);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string password_stored = _("<not stored>");
    std::string password;
    bool find_result = false;

    try {
      find_result =
        mforms::Utilities::find_password(connectionInfo["hostIdentifier"].as<std::string>(), user_name, password);
    } catch (std::exception &except) {
      logWarning("Exception caught when trying to find a password for '%s' connection: %s\n",
                 connectionInfo["name"].as<std::string>().c_str(), except.what());
    }

    if (find_result) {
      password = "";
      password_stored = _("<stored>");
    }
    print_info_line(cr, line_bounds, _("Password"), password_stored);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    print_info_line(cr, line_bounds, _("Network Address"), getAnyMapValueAs<std::string>(connectionInfo, "hostName"));
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    ssize_t port = getAnyMapValueAs<ssize_t>(connectionInfo, "port");
    print_info_line(cr, line_bounds, _("TCP/IP Port"), std::to_string(port));

    line_bounds = bounds;
    line_bounds.pos.x += (bounds.width() + POPUP_LR_PADDING) / 2;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    // Make sure the entire right part does not extend beyond the available horizontal space.
    if (line_bounds.right() > bounds.right())
      line_bounds.pos.x -= bounds.right() - line_bounds.right();

    {
      bool pending = false;
      if (!connectionInfo["serverInfo"].isNull()) {
        mforms::anyMap serverInfo = connectionInfo["serverInfo"];
        pending = getAnyMapValueAs<ssize_t>(serverInfo, "setupPending") == 1;
        if (!pending && !connectionInfo["isLocalConnection"].as<bool>() &&
            getAnyMapValueAs<ssize_t>(serverInfo, "remoteAdmin") == 0 &&
            getAnyMapValueAs<ssize_t>(serverInfo, "windowsAdmin") == 0)
          pending = true;
      } else
        pending = true;

      if (pending) {
        if (connectionInfo["isLocalConnection"].as<bool>())
          print_info_line(cr, line_bounds, _("Local management not set up"), " ");
        else
          print_info_line(cr, line_bounds, _("Remote management not set up"), " ");
      } else {
        if (connectionInfo["isLocalConnection"].as<bool>()) {
          print_info_line(cr, line_bounds, _("Local management"), "Enabled");
          line_bounds.pos.y += 6 * DETAILS_LINE_HEIGHT; // Same layout as for remote mgm. So config file is at bottom.
          print_info_line(cr, line_bounds, _("Config Path"),
                          getAnyMapValueAs<std::string>(serverInfo, "sys.config.path"));
        } else if (!connectionInfo["loginInfo"].isNull()) {
          mforms::anyMap loginInfo = connectionInfo["loginInfo"];
          bool windowsAdmin = getAnyMapValueAs<ssize_t>(serverInfo, "windowsAdmin") == 1;

          std::string os = getAnyMapValueAs<std::string>(serverInfo, "serverOS");
          if (os.empty()) // If there's no OS set (yet) then use the generic system identifier (which is not that
                          // specific, but better than nothing).
            os = getAnyMapValueAs<std::string>(serverInfo, "sys.system");
          if (os.empty() && windowsAdmin)
            os = "Windows";
          print_info_line(cr, line_bounds, _("Operating System"), os);
          line_bounds.pos.y += DETAILS_LINE_HEIGHT;

          if (windowsAdmin) {
            print_info_line(cr, line_bounds, _("Remote management via"), "WMI");
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            std::string host_name = getAnyMapValueAs<std::string>(loginInfo, "wmi.hostName");
            print_info_line(cr, line_bounds, _("Target Server"),
                            getAnyMapValueAs<std::string>(loginInfo, "wmi.hostName"));
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;
            print_info_line(cr, line_bounds, _("WMI user"), getAnyMapValueAs<std::string>(loginInfo, "wmi.userName"));
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            std::string password_key = "wmi@" + host_name;
            user_name = getAnyMapValueAs<std::string>(loginInfo, "wmi.userName");
            if (mforms::Utilities::find_password(password_key, user_name, password)) {
              password = "";
              password_stored = _("<stored>");
            } else
              password_stored = _("<not stored>");
            print_info_line(cr, line_bounds, _("WMI Password"), user_name);
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            line_bounds.pos.y += DETAILS_LINE_HEIGHT; // Empty line by design. Separated for easier extension.
            print_info_line(cr, line_bounds, _("Config Path"),
                            getAnyMapValueAs<std::string>(serverInfo, "sys.config.path"));
          } else {
            print_info_line(cr, line_bounds, _("Remote management via"), "SSH");
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            line_bounds.pos.y += DETAILS_LINE_HEIGHT; // Empty line by design. Separated for easier extension.

            std::string host_name = getAnyMapValueAs<std::string>(loginInfo, "ssh.hostName");
            print_info_line(cr, line_bounds, _("SSH Target"), host_name);
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;
            print_info_line(cr, line_bounds, _("SSH User"), getAnyMapValueAs<std::string>(loginInfo, "ssh.userName"));
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            std::string security = (getAnyMapValueAs<ssize_t>(loginInfo, "ssh.useKey", (ssize_t)0) != 0)
                                     ? _("Public Key")
                                     : _("Password ") + password_stored;
            print_info_line(cr, line_bounds, _("SSH Security"), security);
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;
            print_info_line(cr, line_bounds, _("SSH Port"),
                            getAnyMapValueAs<std::string>(loginInfo, "ssh.port", std::string("22")));
          }
        }
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  virtual bool mouse_up(mforms::MouseButton button, int x, int y) {
    if (button == mforms::MouseButtonLeft) {
      // We are going to destroy ourselves when starting an action, so we have to cache
      // values we need after destruction. The self destruction is also the reason why we
      // use mouse_up instead of mouse_click.
      HomeScreen *owner = _owner->_owner;

      std::string id = _connectionId; // Have to copy the value as we might get released now.

      if (_button1_rect.contains(x, y)) {
        set_modal_result(1); // Just a dummy value to close ourselves.
        owner->handleContextMenu(id, "manage_connections");
      } else if (_button2_rect.contains(x, y)) {
        set_modal_result(1);
        owner->trigger_callback(HomeScreenAction::ActionSetupRemoteManagement, id);
      } else if (_button3_rect.contains(x, y)) {
        set_modal_result(1);
        owner->handleContextMenu(id, "");
      } else if (_button4_rect.contains(x, y)) {
        set_modal_result(1);
        owner->handleContextMenu(id, "open_connection");
      } else if (_close_button_rect.contains(x, y))
        set_modal_result(1);
    }
    return false;
  }
};

//----------------- ConnectionsSection -------------------------------------------------------------

class mforms::ConnectionEntry : mforms::Accessible {
  friend class ConnectionsSection;

public:
  std::string connectionId;

protected:
  ConnectionsSection *owner;

  std::string title;
  std::string description;
  std::string user;
  std::string schema;
  bool compute_strings; // True after creation to indicate the need to compute the final display strings.
  bool draw_info_tab;

  // For filtering we need the full strings.
  std::string search_title;
  std::string search_description;
  std::string search_user;
  std::string search_schema;

  std::function<void(int, int)> default_handler;

  base::Rect bounds;

  // ------ Accesibility Methods -----

  virtual std::string get_acc_name() {
    return title;
  }

  virtual std::string get_acc_description() {
    return base::strfmt("desc:%s;schema:%s;user:%s", description.c_str(), schema.c_str(), user.c_str());
  }

  virtual Accessible::Role get_acc_role() {
    return Accessible::ListItem;
  }
  virtual base::Rect get_acc_bounds() {
    return bounds;
  }
  virtual std::string get_acc_default_action() {
    return "Open Connection";
  }

  virtual void do_default_action() {
    if (default_handler) {
      // Calls the click at the center of the items
      default_handler((int)bounds.center().x, (int)bounds.center().y);
    }
  };

  /**
   * Draws and icon followed by the given text. The given position is that of the upper left corner
   * of the image.
   */
  void draw_icon_with_text(cairo_t *cr, double x, double y, cairo_surface_t *icon, const std::string &text,
                           double alpha) {
    if (icon) {
      mforms::Utilities::paint_icon(cr, icon, x, y);
      x += imageWidth(icon) + 3;
    }

    cairo_set_source_rgba(cr, 51 / 255.0, 51 / 255.0, 51 / 255.0, alpha);

    std::vector<std::string> texts = base::split(text, "\n");

    for (size_t index = 0; index < texts.size(); index++) {
      cairo_text_extents_t extents;
      std::string line = texts[index];
      cairo_text_extents(cr, line.c_str(), &extents);

      cairo_move_to(cr, x, (int)(y + imageHeight(icon) / 2.0 + extents.height / 2.0 + (index * (extents.height + 3))));
      cairo_show_text(cr, line.c_str());
      cairo_stroke(cr);
    }
  }

public:
  enum ItemPosition { First, Last, Other };

  ConnectionEntry(ConnectionsSection *aowner) : owner(aowner), compute_strings(false) {
    draw_info_tab = true;
  }

  virtual std::string section_name() {
    return "";
  }

  virtual bool is_movable() {
    return true;
  }

  virtual base::Color getTitleColor() {
    return owner->_titleColor;
  }

  virtual base::Color getBackgroundColor(bool hot) {
    return hot ? owner->_backgroundColorHot : owner->_backgroundColor;
  }

  virtual cairo_surface_t *get_background_icon() {
    return owner->_sakila_icon;
  }

  void draw_tile_background(cairo_t *cr, bool hot, double alpha, bool for_dragging) {
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

    float image_alpha = 0.25;

    // Background icon.
    bounds.use_inter_pixel = false;
    cairo_surface_t *back_icon = get_background_icon();

    double x = bounds.left() + bounds.width() - imageWidth(back_icon);
    double y = bounds.top() + bounds.height() - imageHeight(back_icon);
    cairo_set_source_surface(cr, back_icon, x, y);
    cairo_paint_with_alpha(cr, image_alpha * alpha);
  }

  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging) {
    base::Color titleColor = getTitleColor();
    base::Rect bounds = this->bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

    draw_tile_background(cr, hot, alpha, for_dragging);

    cairo_set_source_rgba(cr, titleColor.red, titleColor.green, titleColor.blue, alpha);

    if (hot && owner->_show_details && draw_info_tab) {
#ifdef __APPLE__
      // On OS X we show the usual italic small i letter instead of the peeling corner.
      cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_INFO_FONT, CAIRO_FONT_SLANT_ITALIC,
                             CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);

      owner->_info_button_rect = base::Rect(bounds.right() - 15, bounds.bottom() - 10, 10, 10);
      cairo_move_to(cr, owner->_info_button_rect.left(), owner->_info_button_rect.top());
      cairo_show_text(cr, "i");
      cairo_stroke(cr);

#else
      cairo_surface_t *overlay = owner->_mouse_over_icon;
      cairo_set_source_surface(cr, overlay, bounds.left() + bounds.width() - imageWidth(overlay), bounds.top());
      cairo_paint_with_alpha(cr, alpha);

      cairo_set_source_rgba(cr, 1, 1, 1, alpha);
#endif
    }

    std::string systemFont = base::OSConstants::defaultFontName();
    cairo_select_font_face(cr, systemFont.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);

    // Title string.
    double x = (int)bounds.left() + 10.5; // Left offset from the border to caption, user and network icon.
    double y = bounds.top() + 27;         // Distance from top to the caption base line.

    if (compute_strings) {
      // On first render compute the actual string to show. We only need to do this once
      // as neither the available space changes nor is the entry manipulated.

      // We try to shrink titles at the middle, if there's a colon in it we assume it's a port number
      // and thus, we shrink everything before the colon.
      if (title.find(':') != std::string::npos) {
        double available_width = bounds.width() - 21;
        std::string left, right;
        cairo_text_extents_t extents;
        base::partition(title, ":", left, right);
        right = ":" + right;
        cairo_text_extents(cr, right.c_str(), &extents);
        available_width -= extents.width;
        title = mforms::Utilities::shorten_string(cr, left, available_width) + right;
      } else {
        double available_width = bounds.width() - 21;
        title = mforms::Utilities::shorten_string(cr, title, available_width);
      }
    }

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, title.c_str());
    cairo_stroke(cr);

    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_SMALL_INFO_FONT_SIZE);

    draw_tile_text(cr, x, y, alpha);

    compute_strings = false;
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha) {
    if (compute_strings) {
      double available_width = bounds.width() - 24 - imageWidth(owner->_network_icon);
      description = mforms::Utilities::shorten_string(cr, description, available_width);

      available_width =
        bounds.center().x - x - imageWidth(owner->_user_icon) - 6; // -6 is the spacing between text and icons.
      user = mforms::Utilities::shorten_string(cr, user, available_width);

      schema = mforms::Utilities::shorten_string(cr, schema, available_width);
    }

    y = bounds.top() + 56 - imageHeight(owner->_user_icon);
    draw_icon_with_text(cr, x, y, owner->_user_icon, user, alpha);

    y = bounds.top() + 74 - imageHeight(owner->_network_icon);
    draw_icon_with_text(cr, x, y, owner->_network_icon, description, alpha);
  }

  virtual void activate(std::shared_ptr<ConnectionEntry> thisptr, int x, int y) {
    // Anything else.
    owner->_owner->trigger_callback(HomeScreenAction::ActionOpenConnectionFromList, connectionId);
  }

  virtual mforms::Menu *context_menu() {
    return owner->_connection_context_menu;
  }

  virtual void menu_open(ItemPosition pos) {
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
  virtual mforms::ConnectionInfoPopup *show_info_popup() {
    mforms::View *scrollPanel = owner->get_parent();
    mforms::View *main = scrollPanel->get_parent();

    // We have checked in the hit test already that we are on a valid connection object.
    std::pair<int, int> pos = main->client_to_screen(main->get_x(), main->get_y());

    // Place the popup window over the full WB client area, but keep the info area in our direct parent's bounds
    base::Rect hostBounds = base::Rect(pos.first, pos.second, main->get_width(), main->get_height());

    int width = owner->get_width();
    ConnectionsSection::ConnectionVector connections(owner->displayed_connections());

    size_t top_entry = std::find(connections.begin(), connections.end(), owner->_hot_entry) - connections.begin();
    base::Rect tileBounds = owner->bounds_for_entry(top_entry, width);
    tileBounds.pos.x += scrollPanel->get_x() + owner->get_x();
    tileBounds.pos.y += owner->get_y();

    return mforms::manage(new ConnectionInfoPopup(owner, connectionId, hostBounds, tileBounds, main->get_width()));
  }
};

class mforms::FolderEntry : public ConnectionEntry {
protected:
  virtual std::string get_acc_name() override {
    return base::strfmt("%s %s", title.c_str(), _("Connection Group"));
  }

public:
  std::vector<std::shared_ptr<ConnectionEntry> > children;

  FolderEntry(ConnectionsSection *aowner) : ConnectionEntry(aowner) {
    draw_info_tab = false;
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha) override {
    base::Color titleColor = getTitleColor();
    cairo_set_source_rgba(cr, titleColor.red, titleColor.green, titleColor.blue, titleColor.alpha);

    std::string info = std::to_string(children.size() - 1) + " " + _("Connections");
    y = bounds.top() + 55;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, info.c_str());
    cairo_stroke(cr);
  }

  virtual mforms::Menu *context_menu() override {
    return owner->_folder_context_menu;
  }

  virtual void menu_open(ItemPosition pos) override {
    mforms::Menu *menu = context_menu();

    menu->set_item_enabled(menu->get_item_index("move_connection_to_top"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_up"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_down"), pos != Last);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_end"), pos != Last);
  }

  virtual void activate(std::shared_ptr<ConnectionEntry> thisptr, int x, int y) override {
    owner->change_to_folder(std::dynamic_pointer_cast<FolderEntry>(thisptr));
    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }

  virtual base::Color getTitleColor() override {
    return owner->_folderTitleColor;
  }

  virtual base::Color getBackgroundColor(bool hot) override {
    return hot ? owner->_folderBackgroundColorHot : owner->_folderBackgroundColor;
  }

  virtual cairo_surface_t *get_background_icon() override {
    return owner->_folder_icon;
  }

  virtual mforms::ConnectionInfoPopup *show_info_popup() override {
    return NULL;
  }
};

class mforms::FolderBackEntry : public ConnectionEntry {
public:
  FolderBackEntry(ConnectionsSection *aowner) : ConnectionEntry(aowner) {
    title = "< back";
  }

  virtual bool is_movable() override {
    return false;
  }

  virtual base::Color getTitleColor() override {
    return owner->_folderTitleColor;
  }

  virtual base::Color getBackgroundColor(bool hot) override {
    return hot ? owner->_backTileBackgroundColorHot : owner->_backTileBackgroundColor;
  }

  virtual cairo_surface_t *get_background_icon() override {
    return owner->_folder_icon;
  }

  /**
   * Separate tile drawing for the special back tile (to return from a folder).
   */
  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging) override {
    draw_tile_background(cr, hot, alpha, for_dragging);

    // Title string.
    double x = bounds.left() + 10;
    double y = bounds.top() + 27;
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);
    base::Color titleColor = getTitleColor();
    cairo_set_source_rgba(cr, titleColor.red, titleColor.green, titleColor.blue, titleColor.alpha);

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, _("< back"));
    cairo_stroke(cr);
  }

  virtual mforms::Menu *context_menu() override {
    return NULL;
  }

  virtual void menu_open(ItemPosition pos) override {
  }

  virtual mforms::ConnectionInfoPopup *show_info_popup() override {
    return NULL;
  }

  virtual void activate(std::shared_ptr<ConnectionEntry> thisptr, int x, int y) override {
    owner->change_to_folder(std::shared_ptr<FolderEntry>());
    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }
};

//----------------- ConnectionsWelcomeScreen ---------------------------------------------------------------------------

ConnectionsWelcomeScreen::ConnectionsWelcomeScreen(HomeScreen *owner) : _owner(owner) {
  _closeHomeScreenButton.name = "Close Welcome Message Screen";
  _closeHomeScreenButton.default_action = "Close Welcome Message Screen";
  _closeHomeScreenButton.default_handler = _accessible_click_handler;

  _browseDocButton.name = "Browse Documentation >";
  _browseDocButton.default_action = "Open documentation";
  _browseDocButton.default_handler = _accessible_click_handler;

  _readBlogButton.name = "Read the Blog >";
  _readBlogButton.default_action = "Open MySQL Workbench Blog webpage";
  _readBlogButton.default_handler = _accessible_click_handler;

  _discussButton.name = "Discuss on the Forums >";
  _discussButton.default_action = "Open MySQL Workbench Forums";
  _discussButton.default_handler = _accessible_click_handler;

  _closeIcon = mforms::Utilities::load_icon("home_screen_close.png");
}

//----------------------------------------------------------------------------------------------------------------------

ConnectionsWelcomeScreen::~ConnectionsWelcomeScreen() {
  deleteSurface(_closeIcon);
}

//----------------------------------------------------------------------------------------------------------------------

base::Size ConnectionsWelcomeScreen::getLayoutSize(base::Size proposedSize) {
  return base::Size(proposedSize.width, _totalHeight); // Height doesn't change. Constant content.
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsWelcomeScreen::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
  _closeHomeScreenButton.bounds =
    base::Rect(get_width() - imageWidth(_closeIcon) - ConnectionsSection::CONNECTIONS_RIGHT_PADDING,
               ConnectionsSection::CONNECTIONS_RIGHT_PADDING - imageHeight(_closeIcon), imageWidth(_closeIcon),
               imageHeight(_closeIcon));

  cairo_set_source_surface(cr, _closeIcon, _closeHomeScreenButton.bounds.left(), _closeHomeScreenButton.bounds.top());
  cairo_paint(cr);

  int yoffset = 100;

  cairo_save(cr);
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 3);
  cairo_set_source_rgb(cr, 49 / 255.0, 49 / 255.0, 49 / 255.0);

  std::string heading = "Welcome to MySQL Workbench";

  cairo_text_extents_t extents;
  cairo_text_extents(cr, heading.c_str(), &extents);
  double x;
  x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
  cairo_move_to(cr, x, yoffset);
  cairo_show_text(cr, heading.c_str());
  yoffset += mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 3;

  std::vector<std::string> description = {
    "MySQL Workbench is the official graphical user interface (GUI) tool for MySQL. It allows you to design,",
    "create and browse your database schemas, work with database objects and insert data as well as",
    "design and run SQL queries to work with stored data. You can also migrate schemas and data from other",
    "database vendors to your MySQL database."};

  for (auto txt : description) {
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);
    cairo_text_extents(cr, txt.c_str(), &extents);
    x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, txt.c_str());
    yoffset += (int)extents.height + 10;
  }

  yoffset += 40;

  // Draw heading links
  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);

  cairo_set_source_rgb(cr, 0x1b / 255.0, 0xad / 255.0, 0xe8 / 255.0);
  double pos = 0.25;
  for (auto btn : { &_browseDocButton, &_readBlogButton, &_discussButton }) {
    cairo_text_extents(cr, btn->name.c_str(), &extents);
    x = get_width() * pos - (extents.width / 2 + extents.x_bearing);
    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, btn->name.c_str());
    btn->bounds = base::Rect(x, yoffset - extents.height, extents.width, extents.height);
    pos += 0.25;
  }

  _totalHeight = yoffset + 20;

  cairo_restore(cr);
}

bool ConnectionsWelcomeScreen::mouse_click(mforms::MouseButton button, int x, int y) {
  // everything below this relies on _hot_entry, which will become out of sync
  // if the user pops up the context menu and then clicks (or right clicks) in some
  // other tile... so we must first update _hot_entry before doing any actions
  mouse_move(mforms::MouseButtonNone, x, y);

  switch (button) {
    case mforms::MouseButtonLeft: {
      if (_browseDocButton.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::ActionOpenDocs, base::any());
        return true;
      }

      if (_discussButton.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::ActionOpenForum, base::any());
        return true;
      }

      if (_readBlogButton.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::ActionOpenBlog, base::any());
        return true;
      }

      if (_closeHomeScreenButton.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::CloseWelcomeMessage, base::any());
        return true;
      }
    }
    default:
      break;
  }

  return false;
}

//------------------------------------------------------------------------------------------------

ConnectionsSection::ConnectionsSection(HomeScreen *owner)
  : HomeScreenSection("sidebar_wb.png"),
    _search_box(true),
    _search_text(mforms::SmallSearchEntry),
    _showWelcomeHeading(true),
    _welcomeScreen(nullptr),
    _container(nullptr) {

  _owner = owner;
  _connection_context_menu = NULL;
  _folder_context_menu = NULL;
  _generic_context_menu = NULL;
  _show_details = false;
  _drag_index = -1;
  _drop_index = -1;
  _filtered = false;

  std::vector<std::string> formats;
  formats.push_back(mforms::HomeScreenSettings::TILE_DRAG_FORMAT); // We allow dragging tiles to reorder them.
  formats.push_back(mforms::DragFormatFileName);                   // We accept sql script files to open them.
  register_drop_formats(this, formats);

  _folder_icon = mforms::Utilities::load_icon("wb_tile_folder.png");
  _mouse_over_icon = mforms::Utilities::load_icon("wb_tile_mouseover.png");
  _mouse_over2_icon = mforms::Utilities::load_icon("wb_tile_mouseover_2.png");
  _network_icon = mforms::Utilities::load_icon("wb_tile_network.png");
  // TODO: We need a tile icon for the group filter and the status.
  _ha_filter_icon = mforms::Utilities::load_icon("wb_tile_network.png");
  _plus_icon = mforms::Utilities::load_icon("wb_tile_plus.png");
  _sakila_icon = mforms::Utilities::load_icon("wb_tile_sakila.png");
  _schema_icon = mforms::Utilities::load_icon("wb_tile_schema.png");
  _user_icon = mforms::Utilities::load_icon("wb_tile_user.png");
  _manage_icon = mforms::Utilities::load_icon("wb_tile_manage.png");

  _info_popup = NULL;

  _search_box.set_name("Search Box");
  _search_box.set_spacing(5);
  _search_text.set_size(150, -1);

  update_colors();

#ifdef _WIN32
  _search_text.set_bordered(false);
  _search_text.set_size(-1, 18);
  _search_text.set_font(mforms::HomeScreenSettings::HOME_NORMAL_FONT);
  _search_box.set_size(-1, 18);
#else
  _search_box.set_padding(8, 1, 8, 5);
  _search_box.set_size(160, 25);
#endif

#ifdef _WIN32
  mforms::ImageBox *image = mforms::manage(new mforms::ImageBox, false);
  image->set_image("search_sidebar.png");
  image->set_image_align(mforms::MiddleCenter);
  _search_box.add(image, false, false);
#endif
  _search_text.set_name("Search Entry");
  _search_text.set_placeholder_text("Filter connections");
  _search_text.set_bordered(false);
  _search_box.add(&_search_text, true, true);
  scoped_connect(_search_text.signal_changed(), std::bind(&ConnectionsSection::on_search_text_changed, this));
  scoped_connect(_search_text.signal_action(),
                 std::bind(&ConnectionsSection::on_search_text_action, this, std::placeholders::_1));
  add(&_search_box, mforms::TopRight);

  set_padding(0, 30, CONNECTIONS_RIGHT_PADDING, 0);

  _accessible_click_handler = std::bind(&ConnectionsSection::mouse_click, this, mforms::MouseButtonLeft,
                                        std::placeholders::_1, std::placeholders::_2);

  _add_button.name = "Add Connection";
  _add_button.default_action = "Open New Connection Wizard";
  _add_button.default_handler = _accessible_click_handler;

  _manage_button.name = "Manage Connections";
  _manage_button.default_action = "Open Connection Management Dialog";
  _manage_button.default_handler = _accessible_click_handler;
}

//------------------------------------------------------------------------------------------------

ConnectionsSection::~ConnectionsSection() {
  if (_connection_context_menu != NULL)
    _connection_context_menu->release();
  if (_folder_context_menu != NULL)
    _folder_context_menu->release();
  if (_generic_context_menu != NULL)
    _generic_context_menu->release();

  deleteSurface(_folder_icon);
  deleteSurface(_mouse_over_icon);
  deleteSurface(_mouse_over2_icon);
  deleteSurface(_network_icon);
  deleteSurface(_ha_filter_icon);
  deleteSurface(_plus_icon);
  deleteSurface(_sakila_icon);
  deleteSurface(_schema_icon);
  deleteSurface(_user_icon);
  deleteSurface(_manage_icon);

  if (_info_popup != NULL)
    delete _info_popup;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::update_colors() {
  _titleColor = base::Color::parse("#505050");
  _folderTitleColor = base::Color::parse("#F0F0F0");
  _backgroundColor = base::Color::parse("#F4F4F4");
  _backgroundColorHot = base::Color::parse("#D5D5D5");
  _folderBackgroundColor = base::Color::parse("#3477a6");
  _folderBackgroundColorHot = base::Color::parse("#4699b8");
  _backTileBackgroundColor = base::Color::parse("#d9532c");
  _backTileBackgroundColorHot = base::Color::parse("#d97457");

#ifndef __APPLE__
  _search_text.set_front_color("#000000");
#endif
  _search_text.set_placeholder_color("#303030");
  _search_text.set_back_color("#ffffff");
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::focus_search_box() {
  _search_text.focus();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::showWelcomeHeading(bool state) {
  _showWelcomeHeading = state;
  if (_welcomeScreen != nullptr)
    _welcomeScreen->show(state);

  set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::on_search_text_changed() {
  std::string filter = _search_text.get_string_value();
  _filtered_connections.clear();

  _filtered = !filter.empty();
  if (_filtered) {
    ConnectionVector current_connections = !_active_folder ? _connections : _active_folder->children;
    for (ConnectionIterator iterator = current_connections.begin(); iterator != current_connections.end(); ++iterator) {
      // Always keep the first entry if we are in a folder. It's not filtered.
      if (_active_folder && (iterator == current_connections.begin()))
        _filtered_connections.push_back(*iterator);
      else if (base::contains_string((*iterator)->search_title, filter, false) ||
               base::contains_string((*iterator)->search_description, filter, false) ||
               base::contains_string((*iterator)->search_user, filter, false) ||
               base::contains_string((*iterator)->search_schema, filter, false))
        _filtered_connections.push_back(*iterator);
    }
  }

  set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::on_search_text_action(mforms::TextEntryAction action) {
  if (action == mforms::EntryEscape) {
    _search_text.set_value("");
    on_search_text_changed();
  } else if (action == mforms::EntryActivate) {
    if (_active_folder) {
      // Within a folder.
      switch (_filtered_connections.size()) {
        case 1: // Just the back tile. Return to top level.
          _active_folder.reset();
          _filtered = false;
          _search_text.set_value("");
          set_needs_repaint();
          break;

        case 2: // Exactly one entry matched the filter. Activate it.
          _owner->trigger_callback(HomeScreenAction::ActionOpenConnectionFromList,
                                   _filtered_connections[1]->connectionId);
          break;
      }
    } else {
      if (!_filtered_connections.empty()) {
        FolderEntry *folder = dynamic_cast<FolderEntry *>(_filtered_connections[0].get());
        // If only one entry is visible through filtering activate it. I.e. for a group show its content
        // and for a connection open it.
        if (folder && folder->children.size() > 1) {
          // Loop through the unfiltered list to find the index of the group we are about to open.
          _active_folder.reset(); // Just a defensive action. Should never play a role.
          for (size_t i = 0; i < _connections.size(); ++i) {
            if (_connections[i]->title == _filtered_connections[0]->title) {
              _active_folder = std::dynamic_pointer_cast<FolderEntry>(_connections[i]);
              break;
            }
          }
          _filtered = false;
          _search_text.set_value("");
          set_needs_repaint();
        } else
          _owner->trigger_callback(HomeScreenAction::ActionOpenConnectionFromList,
                                   _filtered_connections[0]->connectionId);
      }
    }
  }
}

//------------------------------------------------------------------------------------------------

/**
 * Computes the index for the given position, regardless if that is actually backed by an existing
 * entry or not.
 *
 * This will not work in section separated folders, but it doesn't matter
 * atm because this is only used for drag/drop
 */
ssize_t ConnectionsSection::calculate_index_from_point(int x, int y) {
  int width = get_width();
  if (x < CONNECTIONS_LEFT_PADDING || x > (width - CONNECTIONS_RIGHT_PADDING) || y < CONNECTIONS_TOP_PADDING)
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

std::shared_ptr<ConnectionEntry> ConnectionsSection::entry_from_point(int x, int y, bool &in_details_area) {
  in_details_area = false;
  std::shared_ptr<ConnectionEntry> entry;

  ConnectionVector connections(displayed_connections());
  for (ConnectionVector::iterator conn = connections.begin(); conn != connections.end(); ++conn) {
    if ((*conn)->bounds.contains(x, y)) {
      entry = *conn;
      break;
    }
  }

  if (entry) {
    x -= CONNECTIONS_LEFT_PADDING;
    in_details_area = (x % (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING)) > 3 * CONNECTIONS_TILE_WIDTH / 4.0;
  }

  return entry;
}

std::shared_ptr<ConnectionEntry> ConnectionsSection::entry_from_index(ssize_t index) {
  ssize_t count = displayed_connections().size();
  if (index < count) {
    return displayed_connections()[index];
  }
  return std::shared_ptr<ConnectionEntry>();
}

//----------------------------------------------------------------------------------------------------------------------

base::Rect ConnectionsSection::bounds_for_entry(size_t index, size_t width) {
  base::Rect result(CONNECTIONS_LEFT_PADDING, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
  size_t tiles_per_row = (width - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) /
                         (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  if (tiles_per_row == 0)
    return result;

  size_t column = index % tiles_per_row;
  size_t row = index / tiles_per_row;
  result.pos.x += column * (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);
  result.pos.y += row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the connection id stored under the given index. Returns an empty string if the index
 * describes a folder or back tile.
 * Properly takes into account if we are in a folder or not and if we have filtered entries currently.
 */
std::string ConnectionsSection::connectionIdFromIndex(ssize_t index) {
  if (index < 0 || (_active_folder && index == 0))
    return "";

  return displayed_connections()[index]->connectionId;
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsSection::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
  if (is_layout_dirty()) {
    getContainer()->get_parent()->relayout();
    set_layout_dirty(false);
  }
  
  int yoffset = 45;

  int width = get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING;

  int tiles_per_row = width / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);
  if (tiles_per_row < 1)
    tiles_per_row = 1;

  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

  cairo_set_source_rgb(cr, 49 / 255.0, 49 / 255.0, 49 / 255.0);
  cairo_move_to(cr, CONNECTIONS_LEFT_PADDING, yoffset);

  ConnectionVector &connections(displayed_connections());
  std::string title = _("MySQL Connections");
  if (_active_folder)
    title += " / " + _active_folder->title;

  cairo_show_text(cr, title.c_str());

  // The + button after the title.
  cairo_text_extents_t extents;
  cairo_text_extents(cr, title.c_str(), &extents);
  double text_width = ceil(extents.width);

  _add_button.bounds = base::Rect(CONNECTIONS_LEFT_PADDING + text_width + 10, yoffset - imageHeight(_plus_icon),
                                  imageWidth(_plus_icon), imageHeight(_plus_icon));

  cairo_set_source_surface(cr, _plus_icon, _add_button.bounds.left(), _add_button.bounds.top());
  cairo_paint(cr);

  _manage_button.bounds = base::Rect(_add_button.bounds.right() + 10, yoffset - imageHeight(_manage_icon),
                                     imageWidth(_manage_icon), imageHeight(_manage_icon));
  cairo_set_source_surface(cr, _manage_icon, _manage_button.bounds.left(), _manage_button.bounds.top());
  cairo_paint(cr);

  int row = 0;
  // number of tiles that act as a filler
  int filler_tiles = 0;
  std::string current_section;

  base::Rect bounds(0, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
  std::size_t index = 0;
  bool done = false;
  while (!done) {
    if (index >= connections.size())
      break; // we're done

    bounds.pos.x = CONNECTIONS_LEFT_PADDING;
    for (int column = 0; column < tiles_per_row; column++) {
      std::string section = connections[index]->section_name();
      if (!section.empty() && current_section != section) {
        current_section = section;
        bounds.pos.y += mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE + CONNECTIONS_SPACING;

        // draw the section title
        cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);
        cairo_set_source_rgb(cr, 59 / 255.0, 59 / 255.0, 59 / 255.0);
        cairo_text_extents(cr, current_section.c_str(), &extents);
        cairo_move_to(cr, CONNECTIONS_LEFT_PADDING, bounds.pos.y - (extents.height + extents.y_bearing) - 4);
        cairo_show_text(cr, current_section.c_str());
      }

      // if the name of the next section is different, then we add some filler space after this tile
      if (!current_section.empty() && (size_t)index < connections.size() - 1 &&
          connections[index + 1]->section_name() != current_section) {
        int tiles_occupied = tiles_per_row - column;
        filler_tiles += tiles_occupied;
        column += (tiles_occupied - 1);
      }

      // Updates the bounds on the tile
      connections[index]->bounds = bounds;

      bool draw_hot = connections[index] == _hot_entry;
      connections[index]->draw_tile(cr, draw_hot, 1.0, false);

      // Draw drop indicator.

      // This shouldn't be a problem as I don't think there will be more than that many connections.
      if ((ssize_t)index == _drop_index) {
        cairo_set_source_rgb(cr, 0, 0, 0);
        if (_drop_position == mforms::DropPositionOn) {
          double x = bounds.left() - 4;
          double y = bounds.ycenter();
          cairo_move_to(cr, x, y - 15);
          cairo_line_to(cr, x + 15, y);
          cairo_line_to(cr, x, y + 15);
          cairo_fill(cr);
        } else {
          double x = bounds.left() - 4.5;
          if (_drop_position == mforms::DropPositionRight)
            x = bounds.right() + 4.5;
          cairo_move_to(cr, x, bounds.top());
          cairo_line_to(cr, x, bounds.bottom());
          cairo_set_line_width(cr, 3);
          cairo_stroke(cr);
          cairo_set_line_width(cr, 1);
        }
      }

      index++;
      bounds.pos.x += CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING;
      if (index >= connections.size()) {
        done = true; // we're done
        break;
      }
    }

    row++;
    bounds.pos.y += CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING;
  }
}

//----------------------------------------------------------------------------------------------------------------------

base::Size ConnectionsSection::getLayoutSize(base::Size proposedSize) {
  ConnectionVector &connections(displayed_connections());

  size_t height;
  if (connections.empty())
    height = CONNECTIONS_TOP_PADDING + CONNECTIONS_BOTTOM_PADDING;
  else {
    base::Rect bounds = bounds_for_entry(connections.size() - 1, (size_t)proposedSize.width);
    height = (size_t)bounds.bottom() + CONNECTIONS_BOTTOM_PADDING;
  }

  return base::Size(proposedSize.width, (double)height);
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsSection::cancelOperation() {
  // noop
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsSection::setFocus() {
  _search_text.focus();
}

//----------------------------------------------------------------------------------------------------------------------

bool ConnectionsSection::canHandle(HomeScreenMenuType type) {
  switch (type) {
    case HomeMenuConnection:
    case HomeMenuConnectionGroup:
    case HomeMenuConnectionGeneric:
      return true;
    default:
      return false;
  }
  return false;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::setContextMenu(mforms::Menu *menu, HomeScreenMenuType type) {
  if (canHandle(type)) {
    switch (type) {
      case HomeMenuConnectionGroup:
        if (_folder_context_menu != NULL)
          _folder_context_menu->release();
        _folder_context_menu = menu;
        if (_folder_context_menu != NULL) {
          _folder_context_menu->retain();
          menu->set_handler(std::bind(&ConnectionsSection::handle_folder_command, this, std::placeholders::_1));
        }
        break;

      case HomeMenuConnection:
        if (_connection_context_menu != NULL)
          _connection_context_menu->release();
        _connection_context_menu = menu;
        if (_connection_context_menu != NULL) {
          _connection_context_menu->retain();
          menu->set_handler(std::bind(&ConnectionsSection::handle_command, this, std::placeholders::_1));
        }
        break;

      default:
        if (_generic_context_menu != NULL)
          _generic_context_menu->release();
        _generic_context_menu = menu;
        if (_generic_context_menu != NULL) {
          _generic_context_menu->retain();
          menu->set_handler(std::bind(&ConnectionsSection::handle_command, this, std::placeholders::_1));
        }
        break;
    }

    if (menu != NULL)
      scoped_connect(menu->signal_will_show(), std::bind(&ConnectionsSection::menu_open, this));
  }
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::setContextMenuAction(mforms::Menu *menu, HomeScreenMenuType type) {
  // pass
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::addConnection(const std::string &connectionId, const std::string &title,
                                       const std::string &description, const std::string &user,
                                       const std::string &schema) {
  std::shared_ptr<ConnectionEntry> entry;

  entry = std::shared_ptr<ConnectionEntry>(new ConnectionEntry(this));

  entry->connectionId = connectionId;
  entry->title = title;
  entry->description = description;
  entry->user = user;
  entry->schema = schema;
  entry->compute_strings = true;

  entry->search_title = title;
  entry->search_description = description;
  entry->search_user = user;
  entry->search_schema = schema;

  entry->default_handler = std::bind(&ConnectionsSection::mouse_click, this, mforms::MouseButtonLeft,
                                     std::placeholders::_1, std::placeholders::_2);

  std::string::size_type slash_position = title.find("/");
  if (slash_position != std::string::npos) {
    // A child entry->
    std::string parent_name = title.substr(0, slash_position);
    entry->title = title.substr(slash_position + 1);
    entry->search_title = entry->title;
    bool found_parent = false;
    for (ConnectionIterator iterator = _connections.begin(); iterator != _connections.end(); iterator++) {
      if ((*iterator)->title == parent_name) {
        if (FolderEntry *folder = dynamic_cast<FolderEntry *>(iterator->get())) {
          found_parent = true;
          folder->children.push_back(entry);
          break;
        }
      }
    }

    // If the parent was not found, a folder should be created
    if (!found_parent) {
      std::shared_ptr<FolderEntry> folder(new FolderEntry(this));

      folder->title = parent_name;
      folder->compute_strings = true;
      folder->search_title = parent_name;

      folder->children.push_back(std::shared_ptr<ConnectionEntry>(new FolderBackEntry(this)));
      folder->children.push_back(entry);
      _connections.push_back(std::dynamic_pointer_cast<ConnectionEntry>(folder));
      if (!_active_folder_title_before_refresh_start.empty() &&
          _active_folder_title_before_refresh_start == folder->title) {
        _active_folder = std::dynamic_pointer_cast<FolderEntry>(_connections.back());
        _active_folder_title_before_refresh_start.clear();
      }
    }
  } else
    _connections.push_back(entry);

  set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::clear_connections(bool clear_state) {
  if (clear_state) {
    _filtered = false;
    _filtered_connections.clear();
    _search_text.set_value("");
    _active_folder_title_before_refresh_start = "";
  } else {
    if (_active_folder)
      _active_folder_title_before_refresh_start = _active_folder->title;
  }
  _entry_for_menu.reset();
  _active_folder.reset();
  _connections.clear();

  set_layout_dirty(true);
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::change_to_folder(std::shared_ptr<FolderEntry> folder) {
  if (_active_folder && !folder) {
    // Returning to root list.

    _active_folder.reset();
    _filtered = false;
    _search_text.set_value("");
    set_layout_dirty(true);
  } else if (folder) {
    _active_folder = folder;
    // Drilling into a folder.
    _filtered = false;
    _search_text.set_value("");
    set_layout_dirty(true);
  }
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_down(mforms::MouseButton button, int x, int y) {
  if (button == mforms::MouseButtonLeft && _hot_entry)
    _mouse_down_position = base::Rect(x - 4, y - 4, 8, 8); // Center a 8x8 pixels rect around the mouse position.
  return false;                                            // Continue with standard mouse handling.
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_up(mforms::MouseButton button, int x, int y) {
  _mouse_down_position = base::Rect();
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_double_click(mforms::MouseButton button, int x, int y) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_click(mforms::MouseButton button, int x, int y) {
  // everything below this relies on _hot_entry, which will become out of sync
  // if the user pops up the context menu and then clicks (or right clicks) in some
  // other tile... so we must first update _hot_entry before doing any actions
  mouse_move(mforms::MouseButtonNone, x, y);

  switch (button) {
    case mforms::MouseButtonLeft: {
      if (_add_button.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::ActionNewConnection, base::any());
        return true;
      }

      if (_manage_button.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::ActionManageConnections, base::any());
        return true;
      }

      //      if (_browseDocButton.bounds.contains(x, y)) {
      //        _owner->trigger_callback(HomeScreenAction::ActionOpenDocs, base::any());
      //        return true;
      //      }
      //
      //      if (_discussButton.bounds.contains(x, y)) {
      //        _owner->trigger_callback(HomeScreenAction::ActionOpenForum, base::any());
      //        return true;
      //      }
      //
      //      if (_readBlogButton.bounds.contains(x, y)) {
      //        _owner->trigger_callback(HomeScreenAction::ActionOpenBlog, base::any());
      //        return true;
      //      }

      if (_hot_entry) {
#ifdef __APPLE__
        bool show_info = _info_button_rect.contains_flipped(x, y);
#else
        bool show_info = _show_details;
#endif

        if (show_info && !_info_popup && _parent && (_info_popup = _hot_entry->show_info_popup())) {
          scoped_connect(_info_popup->on_close(), std::bind(&ConnectionsSection::popup_closed, this));

          return true;
        }

        _hot_entry->activate(_hot_entry, x, y);

        return true;
      }
    } break;

    case mforms::MouseButtonRight: {
      mforms::Menu *context_menu = NULL;

      if (_hot_entry) {
        context_menu = _hot_entry->context_menu();

        _entry_for_menu = _hot_entry;
      } else
        context_menu = _generic_context_menu;

      // At this point the context menu and the associated entry have been selected
      if (context_menu)
        context_menu->popup_at(this, x, y);
    } break;

    default:
      break;
  }

  return false;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_leave() {
  // Ignore mouse leaves if we are showing the info popup. We want the entry to stay hot.
  if (_info_popup != NULL)
    return true;

  if (_hot_entry) {
    _hot_entry.reset();
    _show_details = false;
    set_needs_repaint();
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_move(mforms::MouseButton button, int x, int y) {

  bool in_details_area;
  std::shared_ptr<ConnectionEntry> entry = entry_from_point(x, y, in_details_area);

  if (entry && !_mouse_down_position.empty() && (!_mouse_down_position.contains(x, y))) {
    if (!entry->is_movable()) {
      _mouse_down_position = base::Rect();
      return true;
    }

    if (button == mforms::MouseButtonNone) // Cancel drag if the mouse button was released.
      return true;

    return do_tile_drag(calculate_index_from_point(x, y), x, y);
  } else {
    // Only do hit tracking if no mouse button is pressed to avoid situations like
    // mouse down outside any tile, drag over a tile, release mouse button -> click
    // (or hover effects in general).
    if (button == mforms::MouseButtonNone) {
      if (entry != _hot_entry || _show_details != in_details_area) {
        _hot_entry = entry;
#ifndef __APPLE__
        if (_hot_entry)
          _show_details = in_details_area;
#else
        _show_details = true;
#endif
        set_needs_repaint();
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::handle_command(const std::string &command) {
  std::string item;
  if (_entry_for_menu) {
    if (_active_folder) {
      if (command == "delete_connection_all") {
        // We only want to delete all connections in the active group. This is the same as
        // removing the group entirely, since the group is formed by connections in it.
        _entry_for_menu = _active_folder;
        handle_folder_command("delete_connection_group");
        return;
      } else {
        item = _entry_for_menu->connectionId;
      }
    } else {
      item = _entry_for_menu->connectionId;
    }
  }

  _owner->handleContextMenu(item, command);
  _entry_for_menu.reset();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::handle_folder_command(const std::string &command) {
  {
    // We have to pass on a valid connection (for the group name).
    // All child items have the same group name (except the dummy entry for the back tile).
    std::string title;
    if (_entry_for_menu)
      title = _entry_for_menu->title;

    title += "/";

    _owner->handleContextMenu(title, command);
    _entry_for_menu.reset();
  }
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::menu_open() {
  if (_entry_for_menu) {
    ConnectionVector &items(displayed_connections());

    if (items.empty())
      _entry_for_menu->menu_open(ConnectionEntry::Other);
    else if (items.front() == _entry_for_menu)
      _entry_for_menu->menu_open(ConnectionEntry::First);
    else if (items.back() == _entry_for_menu)
      _entry_for_menu->menu_open(ConnectionEntry::Last);
    else
      _entry_for_menu->menu_open(ConnectionEntry::Other);
  }
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::hide_info_popup() {
  if (_info_popup != NULL) {
    _hot_entry.reset();
    _show_details = false;

    _info_popup->release();
    _info_popup = NULL;

    set_needs_repaint();
  }
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::popup_closed() {
  hide_info_popup();
}

//------------------------------------------------------------------------------------------------

int ConnectionsSection::get_acc_child_count() {
  // At least 2 is returned because of the add and manage icons.
  size_t ret_val = 2;

  if (_filtered)
    ret_val += (int)_filtered_connections.size();
  else if (!_active_folder)
    ret_val += (int)_connections.size();
  else {
    // Adds one because of the back tile
    ret_val++;
    ret_val += _active_folder->children.size();
  }

  return (int)ret_val;
}

mforms::Accessible *ConnectionsSection::get_acc_child(int index) {
  mforms::Accessible *accessible = NULL;

  switch (index) {
    case 0:
      accessible = &_add_button;
      break;
    case 1:
      accessible = &_manage_button;
      break;
    default: {
      // Removes 2 to get the real connection index.
      // Note that if at this point index is bigger than the list
      // size, it means it is referring to the pageup/pagedown icons.
      index -= 2;

      if (_filtered) {
        if (index < (int)_filtered_connections.size())
          accessible = _filtered_connections[index].get();
        else
          index -= (int)_filtered_connections.size();
      } else {
        if (!_active_folder) {
          if (index < (int)_connections.size())
            accessible = _connections[index].get();
          else
            index -= (int)_connections.size();
        } else {
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

std::string ConnectionsSection::get_acc_name() {
  return get_name();
}

//------------------------------------------------------------------------------------------------

mforms::Accessible::Role ConnectionsSection::get_acc_role() {
  return Accessible::List;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible *ConnectionsSection::hit_test(int x, int y) {
  mforms::Accessible *accessible = NULL;

  if (_add_button.bounds.contains(x, y))
    accessible = &_add_button;
  else if (_manage_button.bounds.contains(x, y))
    accessible = &_manage_button;
  else {
    bool in_details_area = false;
    std::shared_ptr<ConnectionEntry> entry = entry_from_point(x, y, in_details_area);

    if (entry)
      accessible = entry.get();
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::do_tile_drag(ssize_t index, int x, int y) {
  _hot_entry.reset();
  set_needs_repaint();

  if (index >= 0) {
    mforms::DragDetails details;
    details.allowedOperations = mforms::DragOperationMove;
    details.location = base::Point(x, y);

    details.image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
    cairo_t *cr = cairo_create(details.image);
    base::Rect bounds = bounds_for_entry(index, get_width());
    details.hotspot.x = x - bounds.pos.x;
    details.hotspot.y = y - bounds.pos.y;

    // We know we have no back tile here.
    std::shared_ptr<ConnectionEntry> entry = entry_from_index(index);
    if (entry) {
      entry->draw_tile(cr, false, 1, true);

      _drag_index = index;
      mforms::DragOperation operation =
        do_drag_drop(details, entry.get(), mforms::HomeScreenSettings::TILE_DRAG_FORMAT);

      _mouse_down_position = base::Rect();
      cairo_surface_destroy(details.image);
      cairo_destroy(cr);

      _drag_index = -1;
      _drop_index = -1;
      set_layout_dirty(true);

      if (operation == mforms::DragOperationMove) // The actual move is done in the drop delegate method.
        return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------------------------

// Drop delegate implementation.
mforms::DragOperation ConnectionsSection::drag_over(View *sender, base::Point p,
                                                    mforms::DragOperation allowedOperations,
                                                    const std::vector<std::string> &formats) {
  if (allowedOperations == mforms::DragOperationNone)
    return allowedOperations;

  if (std::find(formats.begin(), formats.end(), mforms::DragFormatFileName) != formats.end()) {
    // Indicate we can accept files if one of the connection tiles is hit.
    bool in_details_area;
    std::shared_ptr<ConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y, in_details_area);

    if (!entry)
      return mforms::DragOperationNone;

    if (entry->connectionId.empty())
      return mforms::DragOperationNone;

    if (_hot_entry != entry) {
      _hot_entry = entry;
      set_needs_repaint();
    }
    return allowedOperations & mforms::DragOperationCopy;
  }

  if (std::find(formats.begin(), formats.end(), mforms::HomeScreenSettings::TILE_DRAG_FORMAT) != formats.end()) {
    // A tile is being dragged. Find the target index and drop location for visual feedback.
    // Computation here is more relaxed than the normal hit test as we want to allow dropping
    // left, right and below the actual tiles area too as well as between tiles.
    if (p.y < CONNECTIONS_TOP_PADDING) {
      if (_drop_index > -1) {
        _drop_index = -1;
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
    int tiles_per_row =
      (width - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

    int column = -1; // Left to the first column.
    if (p.x > (width - CONNECTIONS_RIGHT_PADDING))
      column = tiles_per_row; // After last column.
    else if (p.x >= CONNECTIONS_LEFT_PADDING)
      column = (int)((p.x - CONNECTIONS_LEFT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING));

    int row = (int)(p.y / (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING));

    int row_bottom = row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING) + CONNECTIONS_TILE_HEIGHT;
    if (row_bottom > get_height()) {
      if (_drop_index > -1) {
        _drop_index = -1;
        set_needs_repaint();
      }
      return mforms::DragOperationNone; // Drop on the dimmed row. No drop action here.
    }

    int index = (int)(row * tiles_per_row);
    if (column == tiles_per_row)
      index += column - 1;
    else if (column > -1)
      index += column;

    mforms::DropPosition position = mforms::DropPositionLeft;
    if (column == tiles_per_row)
      position = mforms::DropPositionRight;
    else {
      if (index >= count) {
        index = count - 1;
        position = mforms::DropPositionRight;
      } else {
        // Tile hit. Depending on which side of the tile's center the mouse is use a position
        // before or after that tile. Back tiles have no "before" position, but only "on" or "after".
        // Folder tiles have "before", "on" and "after" positions. Connection tiles only have "before"
        // and "after".
        base::Rect bounds = bounds_for_entry(index, get_width());
        std::shared_ptr<ConnectionEntry> entry = entry_from_index(index);
        if (entry && dynamic_cast<FolderEntry *>(entry.get())) {
          // In a group take the first third as hit area for "before", the second as "on" and the
          // last one as "after".
          if (p.x > bounds.left() + bounds.width() / 3) {
            if (p.x > bounds.right() - bounds.width() / 3)
              position = mforms::DropPositionRight;
            else
              position = mforms::DropPositionOn;
          }
        } else {
          if (p.x > bounds.xcenter())
            position = mforms::DropPositionRight;
        }
      }
    }

    // Check that the drop position does not resolve to the dragged item.
    // Don't allow dragging a group on a group either.
    if (_drag_index > -1 &&
        (index == _drag_index || (index + 1 == _drag_index && position == mforms::DropPositionRight) ||
         (index - 1 == _drag_index && position == mforms::DropPositionLeft) ||
         (position == mforms::DropPositionOn && dynamic_cast<FolderEntry *>(entry_from_index(_drag_index).get())))) {
      index = -1;
    } else if (!_filtered && _active_folder && index == 0 && position == mforms::DropPositionLeft) {
      position = mforms::DropPositionOn; // Drop on back tile.
    }

    if (_drop_index != index || _drop_position != position) {
      _drop_index = index;
      _drop_position = position;
      set_needs_repaint();
    }

    return mforms::DragOperationMove;
  }

  return mforms::DragOperationNone;
}

//------------------------------------------------------------------------------------------------

mforms::DragOperation ConnectionsSection::files_dropped(View *sender, base::Point p,
                                                        mforms::DragOperation allowedOperations,
                                                        const std::vector<std::string> &file_names) {
  bool in_details_area;
  std::shared_ptr<ConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y, in_details_area);
  if (!entry)
    return mforms::DragOperationNone;

  if (!entry->connectionId.empty()) {
    // Allow only sql script files to be dropped.
    std::vector<std::string> files;
    for (size_t i = 0; i < file_names.size(); ++i)
      if (base::tolower(base::extension(file_names[i])) == ".sql")
        files.push_back(file_names[i]);

    if (files.size() == 0)
      return mforms::DragOperationNone;

    HomeScreenDropFilesInfo dInfo;
    dInfo.connectionId = entry->connectionId;
    dInfo.files = files;
    _owner->trigger_callback(HomeScreenAction::ActionFilesWithConnection, dInfo);
  }

  return mforms::DragOperationCopy;
}

//------------------------------------------------------------------------------------------------

mforms::DragOperation ConnectionsSection::data_dropped(mforms::View *sender, base::Point p,
                                                       mforms::DragOperation allowedOperations, void *data,
                                                       const std::string &format) {
  if (format == mforms::HomeScreenSettings::TILE_DRAG_FORMAT && _drop_index > -1) {
    mforms::DragOperation result = mforms::DragOperationNone;

    // Can be invalid if we move a group.
    std::string connectionId = connectionIdFromIndex(_drag_index);
    ConnectionEntry *source_entry = static_cast<ConnectionEntry *>(data);

    std::shared_ptr<ConnectionEntry> entry;
    if (_filtered) {
      if (_drop_index < (int)_filtered_connections.size())
        entry = _filtered_connections[_drop_index];
    } else if (_active_folder) {
      if (_drop_index < (int)_active_folder->children.size())
        entry = _active_folder->children[_drop_index];
    } else {
      if (_drop_index < (int)_connections.size())
        entry = _connections[_drop_index];
    }

    if (!entry)
      return result;

    bool is_back_tile = entry->title == "< back";

    // Drop target is a group.
    HomeScreenDropInfo dropInfo;
    if (!connectionId.empty()) {
      dropInfo.valueIsConnectionId = true;
      dropInfo.value = connectionId;
    } else
      dropInfo.value = source_entry->title + "/";

    if (_drop_position == mforms::DropPositionOn) {
      // Drop on a group (or back tile).
      if (is_back_tile)
        dropInfo.group = "*Ungrouped*";
      else
        dropInfo.group = entry->title;
      _owner->trigger_callback(HomeScreenAction::ActionMoveConnectionToGroup, dropInfo);
    } else {
      // Drag from one position to another within a group (root or active group).
      size_t to = _drop_index;
      if (_active_folder)
        to--; // The back tile has no representation in the global list.
      if (_drop_position == mforms::DropPositionRight)
        to++;
      dropInfo.to = to;
      _owner->trigger_callback(HomeScreenAction::ActionMoveConnection, dropInfo);
    }
    result = mforms::DragOperationMove;

    _drop_index = -1;
    set_layout_dirty(true);

    return result;
  }
  return mforms::DragOperationNone;
}

mforms::View *ConnectionsSection::getContainer() {
  if (_container == nullptr) {
    _container = mforms::manage(new mforms::Box(false));
    _welcomeScreen = new ConnectionsWelcomeScreen(_owner);
    if (!_showWelcomeHeading)
      _welcomeScreen->show(false);
    _welcomeScreen->set_name("Home Screen Welcome Message");
    _welcomeScreen->set_layout_dirty(true);
    _container->add(_welcomeScreen, false, true);
    _container->add(this, true, true);
  }
  return _container;
}

mforms::View *ConnectionsSection::get_parent() const {
  return _container->get_parent();
}

ConnectionsSection::ConnectionVector &ConnectionsSection::displayed_connections() {
  if (_filtered)
    return _filtered_connections;
  else if (_active_folder)
    return _active_folder->children;
  else
    return _connections;
}
