/*
 * Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.
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

#include "home_screen_connections.h"
#include "mforms/menu.h"
#include "mforms/popup.h"
#include "mforms/imagebox.h"

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"

DEFAULT_LOG_DOMAIN("home");

using namespace wb;

inline void delete_surface(cairo_surface_t* surface)
{
  if (surface != NULL)
    cairo_surface_destroy(surface);
}

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



//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given connection is an SSH connection and returns true if so.
 */
static bool is_ssh_connection(const db_mgmt_ConnectionRef &connection)
{
  if (connection.is_valid())
  {
    std::string driver = connection->driver().is_valid() ? connection->driver()->name() : "";
    return (driver == "MysqlNativeSSH");
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given connection is a local connection (i.e. to the current box).
 */
static bool is_local_connection(const db_mgmt_ConnectionRef &connection)
{
  if (connection.is_valid())
  {
    std::string hostname= connection->parameterValues().get_string("hostName");

    if (!is_ssh_connection(connection) && (hostname == "localhost" || hostname.empty() || hostname == "127.0.0.1"))
    return true;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

class wb::ConnectionInfoPopup : public mforms::Popup
{
private:
  HomeScreen *_owner;

  base::Rect _free_area;
  int _info_width;
  db_mgmt_ConnectionRef _connection;
  db_mgmt_ServerInstanceRef _instance; // Might be invalid.
  base::Rect _button1_rect;
  base::Rect _button2_rect;
  base::Rect _button3_rect;
  base::Rect _button4_rect;
  base::Rect _close_button_rect;

  cairo_surface_t* _close_icon;

public:
  ConnectionInfoPopup(HomeScreen *owner, const db_mgmt_ConnectionRef connection,
                      const db_mgmt_ServerInstanceRef instance, base::Rect host_bounds, base::Rect free_area, int info_width)
  : Popup(mforms::PopupPlain)
  {
    _owner = owner;
    _connection = connection;
    _instance = instance;

    _close_icon = mforms::Utilities::load_icon("wb_close.png");

    // Host bounds is the overall size the popup should cover.
    // The free area is a hole in that overall area which should not be covered to avoid darkening it.
    _free_area = free_area;
    _info_width = info_width;
    set_size((int)host_bounds.width(), (int)host_bounds.height());
    show((int)host_bounds.left(), (int)host_bounds.top());
  }

  //------------------------------------------------------------------------------------------------

  ~ConnectionInfoPopup()
  {
    delete_surface(_close_icon);
  }

  //------------------------------------------------------------------------------------------------

#define POPUP_HEIGHT 240
#define POPUP_TIP_HEIGHT 14
#define POPUP_LR_PADDING 53 // Left and right padding.
#define POPUP_TB_PADDING 24 // Top and bottom padding.
#define POPUP_BUTTON_MIN_WIDTH 88
#define POPUP_BUTTON_HEIGHT 24
#define POPUP_BUTTON_SPACING 19 // Horizontal space between adjacent buttons.
#define POPUP_BUTTON_PADDING 11 // Horizontal space between button border and text.

  /**
   * Draws a button with the given text at the given position. The button width depends on the
   * text. Font face and size are set already.
   * Result is the actual button bounds rectangle we can use for hit tests later.
   */
  base::Rect draw_button(cairo_t *cr, base::Point position, std::string text, bool high_contrast,
                         bool right_aligned = false)
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
    if (high_contrast)
    cairo_set_source_rgb(cr, 0, 0, 0);
    else
    cairo_set_source_rgb(cr, 0xF6 / 255.0, 0xF6 / 255.0, 0xF6 / 255.0);
    cairo_stroke(cr);

    double x = (int)(button_rect.left() + (button_rect.width() - extents.width) / 2.0);
    double y = (int)(button_rect.bottom() - (button_rect.height() - extents.height) / 2.0);
    if (high_contrast)
    cairo_set_source_rgb(cr, 0, 0, 0);
    else
    cairo_set_source_rgb(cr, 0xF3 / 255.0, 0xF3 / 255.0, 0xF3 / 255.0);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text.c_str());
    cairo_stroke(cr);

    return button_rect;
  }

  //------------------------------------------------------------------------------------------------

  void repaint(cairo_t *cr, int x, int y, int w, int h)
  {
    bool is_fabric = _connection.is_valid() && _connection->driver()->name() == "MySQLFabric";
    bool is_managed = _connection.is_valid() && _connection->parameterValues().has_key("fabric_managed");
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

    bool high_contrast = base::Color::is_high_contrast_scheme();
    if (high_contrast)
      cairo_set_source_rgba(cr, 1, 1, 1, 0.5);
    else
      cairo_set_source_rgba(cr, 0, 0, 0, 0.5);
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

      if (high_contrast)
      cairo_set_source_rgb(cr, 1, 1, 1);
      else
      cairo_set_source_rgba(cr, 0x1d / 255.0, 0x1d / 255.0, 0x1d / 255.0, 1);
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

      if (high_contrast)
      cairo_set_source_rgb(cr, 1, 1, 1);
      else
      cairo_set_source_rgb(cr, 0x1d / 255.0, 0x1d / 255.0, 0x1d / 255.0);
      cairo_fill(cr);

      content_bounds.pos.y = tip.y - POPUP_TIP_HEIGHT - POPUP_HEIGHT;
    }

    content_bounds.pos.y += POPUP_TB_PADDING;
    content_bounds.size.height = POPUP_HEIGHT - 2 * POPUP_TB_PADDING;

    // The title.
    cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_TITLE_FONT_SIZE);
    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgb(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0);
    cairo_move_to(cr, content_bounds.left(), content_bounds.top() + 16);
    cairo_show_text(cr, _connection->name().c_str());
    cairo_stroke(cr);

    // All the various info.
    cairo_select_font_face(cr, HOME_DETAILS_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_DETAILS_FONT_SIZE);
    if (is_fabric)
      print_fabric_details_text(cr, content_bounds);
    else
      print_details_text(cr, content_bounds);

    // Buttons at the bottom.
    base::Point position = base::Point(content_bounds.left(), content_bounds.bottom() - POPUP_BUTTON_HEIGHT);
    _button1_rect = draw_button(cr, position, _("Edit Connection..."), high_contrast);

    if (!is_fabric)
    {
      if (!is_managed)
      {
        grt::DictRef serverInfo;
        if (_instance.is_valid())
        serverInfo = _instance->serverInfo();

        bool pending = !serverInfo.is_valid() || serverInfo.get_int("setupPending") == 1;
        if (!pending && !is_local_connection(_connection) && serverInfo.get_int("remoteAdmin") == 0 &&
            serverInfo.get_int("windowsAdmin") == 0)
        pending = true;

        if (pending)
        {
          position.x += _button1_rect.width() + POPUP_BUTTON_SPACING;
          if (is_local_connection(_connection))
          _button2_rect = draw_button(cr, position, _("Configure Local Management..."), high_contrast);
          else
          _button2_rect = draw_button(cr, position, _("Configure Remote Management..."), high_contrast);
        }
        else
        _button2_rect = base::Rect();
      }

      /*
       position.x += _button2_rect.width() + POPUP_BUTTON_SPACING;
       _button3_rect = draw_button(cr, position, _("Add to Favorites"), high_contrast);
       */
      // The last button is right-aligned.
      position.x = right - POPUP_LR_PADDING;
      _button4_rect = draw_button(cr, position, _("Connect"), high_contrast, true);
    }

    // Finally the close button.
    _close_button_rect = base::Rect(right - image_width(_close_icon) - 10, top + 10, image_width(_close_icon), image_height(_close_icon));
    cairo_set_source_surface(cr, _close_icon, _close_button_rect.left(), _close_button_rect.top());
    if (high_contrast)
    {
      cairo_set_operator(cr, CAIRO_OPERATOR_XOR);
      cairo_paint(cr);
      cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    }
    else
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

#define DETAILS_TOP_OFFSET   44
#define DETAILS_LINE_HEIGHT  18
#define DETAILS_LINE_WIDTH  340

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

    grt::DictRef parameter_values = _connection->parameterValues();

    grt::DictRef server_info;
    if (_instance.is_valid())
    server_info = _instance->serverInfo();

    std::string server_version = parameter_values.get_string("serverVersion");
    if (server_version.empty() && server_info.is_valid())
    server_version = server_info.get_string("serverVersion");

    print_info_line(cr, line_bounds, _("MySQL Version"), server_version);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    time_t time = parameter_values.get_int("lastConnected");
    if (time == 0)
    print_info_line(cr, line_bounds, _("Last connected"), "");
    else
    {
      struct tm * ptm = localtime(&time);
      char buffer[32];
      strftime(buffer, 32, "%d %B %Y %H:%M", ptm);
      print_info_line(cr, line_bounds, _("Last connected"), buffer);
    }
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string sshHost = parameter_values.get_string("sshHost");
    if (!sshHost.empty())
    {
      std::string sshUser = parameter_values.get_string("sshUserName");
      print_info_line(cr, line_bounds, _("Using Tunnel"), sshUser + "@" + sshHost);
    }

    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    std::string user_name = parameter_values.get_string("userName");
    print_info_line(cr, line_bounds, _("User Account"), user_name);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string password_stored = _("<not stored>");
    std::string password;
    bool find_result = false;
    
    try
    {
      find_result = mforms::Utilities::find_password(_connection->hostIdentifier(), user_name, password);
    }
    catch(std::exception &except)
    {
      log_warning("Exception caught when trying to find a password for '%s' connection: %s\n", _connection->name().c_str(), except.what());
    }
    
    if (find_result)
    {
      password = "";
      password_stored = _("<stored>");
    }
    print_info_line(cr, line_bounds, _("Password"), password_stored);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    print_info_line(cr, line_bounds, _("Network Address"), parameter_values.get_string("hostName"));
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    ssize_t port = parameter_values.get_int("port");
    print_info_line(cr, line_bounds, _("TCP/IP Port"), base::to_string(port));

    // Instance or fabric info next.
    line_bounds = bounds;
    line_bounds.pos.x += (bounds.width() + POPUP_LR_PADDING) / 2;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    // Make sure the entire right part does not extend beyond the available horizontal space.
    if (line_bounds.right() > bounds.right())
    line_bounds.pos.x -= bounds.right() - line_bounds.right();

    if (parameter_values.has_key("fabric_managed"))
    {
      print_info_line(cr, line_bounds, _("Fabric Managed Instance"), " ");
      line_bounds.pos.y += 2 * DETAILS_LINE_HEIGHT;

      print_info_line(cr, line_bounds, _("Server ID:"), parameter_values.get_string("fabric_server_uuid"));
      line_bounds.pos.y += DETAILS_LINE_HEIGHT;

      print_info_line(cr, line_bounds, _("Status:"), parameter_values.get_string("fabric_status"));
      line_bounds.pos.y += DETAILS_LINE_HEIGHT;

      print_info_line(cr, line_bounds, _("Mode:"), parameter_values.get_string("fabric_mode"));
      line_bounds.pos.y += DETAILS_LINE_HEIGHT;

      print_info_line(cr, line_bounds, _("Weight:"), parameter_values.get("fabric_weight").repr());
      line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    }
    else
    {
      grt::DictRef serverInfo;
      if (_instance.is_valid())
      serverInfo =_instance->serverInfo();

      bool pending = !serverInfo.is_valid() || serverInfo.get_int("setupPending") == 1;
      if (!pending && !is_local_connection(_connection) && serverInfo.get_int("remoteAdmin") == 0 &&
          serverInfo.get_int("windowsAdmin") == 0)
      pending = true;

      if (pending)
      {
        if (is_local_connection(_connection))
        print_info_line(cr, line_bounds, _("Local management not set up"), " ");
        else
        print_info_line(cr, line_bounds, _("Remote management not set up"), " ");
      }
      else
      {
        if (is_local_connection(_connection))
        {
          print_info_line(cr, line_bounds, _("Local management"), "Enabled");
          line_bounds.pos.y += 6 * DETAILS_LINE_HEIGHT; // Same layout as for remote mgm. So config file is at bottom.
          print_info_line(cr, line_bounds, _("Config Path"), serverInfo.get_string("sys.config.path"));
        }
        else
        {
          grt::DictRef loginInfo = _instance->loginInfo();
          bool windowsAdmin = serverInfo.get_int("windowsAdmin", 0) == 1;

          std::string os = serverInfo.get_string("serverOS");
          if (os.empty()) // If there's no OS set (yet) then use the generic system identifier (which is not that specific, but better than nothing).
          os = serverInfo.get_string("sys.system");
          if (os.empty() && windowsAdmin)
          os = "Windows";
          print_info_line(cr, line_bounds, _("Operating System"), os);
          line_bounds.pos.y += DETAILS_LINE_HEIGHT;

          if (windowsAdmin)
          {
            print_info_line(cr, line_bounds, _("Remote management via"), "WMI");
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            std::string host_name = loginInfo.get_string("wmi.hostName");
            print_info_line(cr, line_bounds, _("Target Server"), loginInfo.get_string("wmi.hostName"));
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;
            print_info_line(cr, line_bounds, _("WMI user"), loginInfo.get_string("wmi.userName"));
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            std::string password_key = "wmi@" + host_name;
            user_name = loginInfo.get_string("wmi.userName");
            if (mforms::Utilities::find_password(password_key, user_name, password))
            {
              password = "";
              password_stored = _("<stored>");
            }
            else
            password_stored = _("<not stored>");
            print_info_line(cr, line_bounds, _("WMI Password"), user_name);
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            line_bounds.pos.y += DETAILS_LINE_HEIGHT; // Empty line by design. Separated for easier extension.
            print_info_line(cr, line_bounds, _("Config Path"), serverInfo.get_string("sys.config.path"));
          }
          else
          {
            print_info_line(cr, line_bounds, _("Remote management via"), "SSH");
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            line_bounds.pos.y += DETAILS_LINE_HEIGHT; // Empty line by design. Separated for easier extension.

            std::string host_name = loginInfo.get_string("ssh.hostName");
            print_info_line(cr, line_bounds, _("SSH Target"), host_name);
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;
            print_info_line(cr, line_bounds, _("SSH User"), loginInfo.get_string("ssh.userName"));
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;

            std::string security = (loginInfo.get_int("ssh.useKey", 0) != 0) ? _("Public Key") : _("Password ") + password_stored;
            print_info_line(cr, line_bounds, _("SSH Security"), security);
            line_bounds.pos.y += DETAILS_LINE_HEIGHT;
            print_info_line(cr, line_bounds, _("SSH Port"), loginInfo.get_string("ssh.port", "22"));
          }
        }
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  void print_fabric_details_text(cairo_t *cr, base::Rect bounds)
  {
    // Connection info first.
    base::Rect line_bounds = bounds;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;

    // Use POPUP_LR_PADDIND as space between the two columns.
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    grt::DictRef parameter_values = _connection->parameterValues();

    std::string user_name = parameter_values.get_string("userName");
    print_info_line(cr, line_bounds, _("Fabric User"), user_name);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string password_stored = _("<not stored>");
    std::string password;
    if (mforms::Utilities::find_password(_connection->hostIdentifier(), user_name, password))
    {
      password = "";
      password_stored = _("<stored>");
    }
    print_info_line(cr, line_bounds, _("Password"), password_stored);
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    print_info_line(cr, line_bounds, _("Network Address"), parameter_values.get_string("hostName"));
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    ssize_t port = parameter_values.get_int("port");
    print_info_line(cr, line_bounds, _("TCP/IP Port"), base::to_string(port));
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;

    std::string group_filter =parameter_values.get_string("haGroupFilter");
    if (group_filter.length())
    {
      print_info_line(cr, line_bounds, _("Group Filter"), parameter_values.get_string("haGroupFilter"));
      line_bounds.pos.y += DETAILS_LINE_HEIGHT;
    }
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
      db_mgmt_ConnectionRef connection = _connection;

      if (_button1_rect.contains(x, y))
      {
        set_modal_result(1); // Just a dummy value to close ourselves.
        owner->handle_context_menu(connection, "manage_connections");
      }
      else
      if (_button2_rect.contains(x, y))
      {
        set_modal_result(1);
        owner->trigger_callback(ActionSetupRemoteManagement, connection);
      }
      else
      if (_button3_rect.contains(x, y))
      {
        set_modal_result(1);
        owner->handle_context_menu(connection, "");
      }
      else 
      if (_button4_rect.contains(x, y))
      {
        set_modal_result(1);
        owner->handle_context_menu(connection, "open_connection");
      }
      else
      if (_close_button_rect.contains(x, y))
      set_modal_result(1);
    }
    return false;
  }
};


//----------------- ConnectionsSection -------------------------------------------------------------

#define CONNECTIONS_LEFT_PADDING  40
#define CONNECTIONS_RIGHT_PADDING 40 // The tile spacing right to the last tile in the row does not belong to this padding.
#define CONNECTIONS_TOP_PADDING   75 // The vertical offset of the first visible shortcut entry->
#define CONNECTIONS_SPACING        9 // Vertical/horizontal space between entries.

#define CONNECTIONS_TILE_WIDTH   241
#define CONNECTIONS_TILE_HEIGHT   91


class wb::ConnectionEntry: mforms::Accessible
{
  friend class ConnectionsSection;
public:
  db_mgmt_ConnectionRef connection;
protected:
  ConnectionsSection *owner;

  std::string title;
  std::string description;
  std::string user;
  std::string schema;
  bool compute_strings; // True after creation to indicate the need to compute the final display strings.
  bool second_color;    // Cache flag for checkboard position, to ease creating a drag image.
  bool draw_info_tab;

  // For filtering we need the full strings.
  std::string search_title;
  std::string search_description;
  std::string search_user;
  std::string search_schema;

  boost::function <void (int, int)> default_handler;

  base::Rect bounds;

  // ------ Accesibility Methods -----

  virtual std::string get_acc_name()
  {
    return title;
  }

  virtual std::string get_acc_description()
  {
    return base::strfmt("desc:%s;schema:%s;user:%s", description.c_str(), schema.c_str(), user.c_str());
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
                                            const std::string &text, double alpha, bool high_contrast)
  {
    if (icon)
    {
      mforms::Utilities::paint_icon(cr, icon, x, y);
      x += image_width(icon) + 3;
    }
    double component = 0xF9 / 255.0;
    if (high_contrast)
      component = 1;
#ifdef __APPLE__
    cairo_set_source_rgba(cr, component, component, component, 0.6 * alpha);
#else
    cairo_set_source_rgba(cr, component, component, component, alpha);
#endif

    std::vector<std::string> texts = base::split(text, "\n");

    for (size_t index = 0; index < texts.size(); index++)
    {
      cairo_text_extents_t extents;
      std::string line = texts[index];
      cairo_text_extents(cr, line.c_str(), &extents);

      cairo_move_to(cr, x, (int)(y + image_height(icon) / 2.0 + extents.height / 2.0 + (index * (extents.height + 3))));
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

  ConnectionEntry(ConnectionsSection *aowner)
  : owner(aowner)
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

  virtual base::Color get_current_color(bool hot)
  {
#ifndef __APPLE__
    if (second_color)
      return hot ? owner->_tile_bk_color2_hl : owner->_tile_bk_color2;
    else
#endif
      // No checker board for Mac.
      return hot ? owner->_tile_bk_color1_hl : owner->_tile_bk_color1;
  }

  virtual cairo_surface_t *get_background_icon()
  {
    return owner->_sakila_icon;
  }

  void draw_tile_background(cairo_t *cr, bool hot, double alpha, bool for_dragging)
  {
    base::Color current_color = get_current_color(hot);

    base::Rect bounds = this->bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

#ifdef __APPLE__
    cairo_new_sub_path(cr);

    double radius = 8;
    double degrees = M_PI / 180.0;

    bounds.use_inter_pixel = false;
    cairo_arc(cr, bounds.left() + bounds.width() - radius, bounds.top() + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, bounds.left() + bounds.width() - radius, bounds.top() + this->bounds.height() - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, bounds.left() + radius, bounds.top() + bounds.height() - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, bounds.left() + radius, bounds.top() + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, current_color.red, current_color.green, current_color.blue, alpha);
    cairo_fill(cr);

    // Border.
    bounds.use_inter_pixel = true;
    cairo_arc(cr, -2 + bounds.right() - radius, 1 + bounds.top() + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, -2 + bounds.right() - radius, -2 + bounds.bottom() - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, 1 + bounds.left() + radius, -2 + bounds.bottom() - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, 1 + bounds.left() + radius, 1 + bounds.top() + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.19 * alpha);
    cairo_set_line_width(cr, 3);
    cairo_stroke(cr);

    float image_alpha = 0.3;
#else
    bounds.use_inter_pixel = false;
    cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.width(), bounds.height());
    cairo_set_source_rgba(cr, current_color.red, current_color.green, current_color.blue, alpha);
    cairo_fill(cr);

    // Border.
    bounds.use_inter_pixel = true;
    cairo_rectangle(cr, bounds.left(), bounds.top(), bounds.width() - 1, bounds.height() - 1);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.125 * alpha);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);

    float image_alpha = 1;
#endif

    // Background icon.
    bounds.use_inter_pixel = false;
    cairo_surface_t *back_icon = get_background_icon();

    double x = bounds.left() + bounds.width() - image_width(back_icon);
    double y = bounds.top() + bounds.height() - image_height(back_icon);
    cairo_set_source_surface(cr, back_icon, x, y);
    cairo_paint_with_alpha(cr, image_alpha * alpha);
  }

  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging,
                         bool high_contrast)
  {
    base::Rect bounds = this->bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

    draw_tile_background(cr, hot, alpha, for_dragging);

    double component = 0xF9 / 255.0;
    if (high_contrast)
      component = 1;
    cairo_set_source_rgba(cr, component, component, component, alpha);

    if (hot && owner->_show_details && draw_info_tab)
    {
#ifdef __APPLE__
      // On OS X we show the usual italic small i letter instead of the peeling corner.
      cairo_select_font_face(cr, HOME_INFO_FONT, CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);

      owner->_info_button_rect = base::Rect(bounds.right() - 15, bounds.bottom() - 10, 10, 10);
      cairo_move_to(cr, owner->_info_button_rect.left(), owner->_info_button_rect.top());
      cairo_show_text(cr, "i");
      cairo_stroke(cr);

#else
      cairo_surface_t *overlay = owner->_mouse_over_icon;
      cairo_set_source_surface(cr, overlay, bounds.left() + bounds.width() - image_width(overlay), bounds.top());
      cairo_paint_with_alpha(cr, alpha);

      cairo_set_source_rgba(cr, component, component, component, alpha);
#endif
    }

    cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);

    // Title string.
    double x = (int)bounds.left() + 10.5; // Left offset from the border to caption, user and network icon.
    double y = bounds.top() + 27; // Distance from top to the caption base line.

    if (compute_strings)
    {
      // On first render compute the actual string to show. We only need to do this once
      // as neither the available space changes nor is the entry manipulated.

      // we try to shrink titles at the middle, if there's a : in it we assume it's a port number
      // and thus, we shrink everything before the :
      if (title.find(':') != std::string::npos)
      {
        double available_width = bounds.width() - 21;
        std::string left, right;
        cairo_text_extents_t extents;
        base::partition(title, ":", left, right);
        right = ":"+right;
        cairo_text_extents(cr, right.c_str(), &extents);
        available_width -= extents.width;
        title = mforms::Utilities::shorten_string(cr, left, available_width)+right;
      }
      else
      {
        double available_width = bounds.width() - 21;
        title = mforms::Utilities::shorten_string(cr, title, available_width);
      }
    }

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, title.c_str());
    cairo_stroke(cr);

    cairo_set_font_size(cr, HOME_SMALL_INFO_FONT_SIZE);

    draw_tile_text(cr, x, y, alpha, high_contrast);

    compute_strings = false;
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha, bool high_contrast)
  {
    if (compute_strings)
    {
      double available_width = bounds.width() - 24 - image_width(owner->_network_icon);
      description = mforms::Utilities::shorten_string(cr, description, available_width);

      available_width = bounds.center().x - x - image_width(owner->_user_icon) - 6; // -6 is the spacing between text and icons.
      user = mforms::Utilities::shorten_string(cr, user, available_width);

      schema = mforms::Utilities::shorten_string(cr, schema, available_width);
    }

    y = bounds.top() + 56 - image_height(owner->_user_icon);
    draw_icon_with_text(cr, x, y, owner->_user_icon, user, alpha, high_contrast);

    y = bounds.top() + 74 - image_height(owner->_network_icon);
    draw_icon_with_text(cr, x, y, owner->_network_icon, description, alpha, high_contrast);
  }


  virtual void activate(boost::shared_ptr<ConnectionEntry> thisptr, int x, int y)
  {
    // Anything else.
    owner->_owner->trigger_callback(ActionOpenConnectionFromList, connection);
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
  virtual wb::ConnectionInfoPopup *show_info_popup()
  {
    mforms::View *parent = owner->get_parent();

    // We have checked in the hit test already that we are on a valid connection object.
    std::pair<int, int> pos = parent->client_to_screen(parent->get_x(), parent->get_y());

    // Stretch the popup window over all 3 sections, but keep the info area in our direct parent's bounds
    base::Rect host_bounds = base::Rect(pos.first, pos.second, parent->get_parent()->get_width(), parent->get_parent()->get_height());

    int width = owner->get_width();
    width -= CONNECTIONS_LEFT_PADDING + CONNECTIONS_RIGHT_PADDING;
    int tiles_per_row = width / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

    ConnectionsSection::ConnectionVector connections(owner->displayed_connections());

    size_t top_entry = std::find(connections.begin(), connections.end(), owner->_hot_entry) - connections.begin() - owner->_page_start;
    size_t row = top_entry / tiles_per_row;
    size_t column = top_entry % tiles_per_row;
    pos.first = (int)(CONNECTIONS_LEFT_PADDING + column * (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING));
    pos.second = (int)(CONNECTIONS_TOP_PADDING + row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING));
    base::Rect item_bounds = base::Rect(pos.first, pos.second, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);

    db_mgmt_ServerInstanceRef instance;
    grt::ListRef<db_mgmt_ServerInstance> instances = owner->_owner->rdbms()->storedInstances();
    for (grt::ListRef<db_mgmt_ServerInstance>::const_iterator iterator = instances.begin();
         iterator != instances.end(); iterator++)
    {
      if ((*iterator)->connection() == connection)
      {
        instance = *iterator;
        break;
      }
    }

    int info_width =  parent->get_width();
    if (info_width < 735)
      info_width = (int)host_bounds.width();
    return mforms::manage(new ConnectionInfoPopup(owner->_owner, connection, instance, host_bounds, item_bounds, info_width));
  }
};


class wb::FabricManagedConnectionEntry : public ConnectionEntry
{
public:
  FabricManagedConnectionEntry(ConnectionsSection *aowner)
  : ConnectionEntry(aowner)
  {
  }

  virtual bool is_movable()
  {
    return false;
  }

  virtual void menu_open(ItemPosition pos)
  {
    mforms::Menu *menu = context_menu();

    menu->set_item_enabled(menu->get_item_index("edit_connection"), false);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_group"), false);
    menu->set_item_enabled(menu->get_item_index("delete_connection"), false);
    menu->set_item_enabled(menu->get_item_index("delete_connection_all"), false);

    menu->set_item_enabled(menu->get_item_index("move_connection_to_top"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_up"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_down"), pos != Last);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_end"), pos != Last);
  }

  virtual base::Color get_current_color(bool hot)
  {
    std::string status = base::strip_text(connection->parameterValues().get("fabric_status").repr());
    if (status == "PRIMARY")
      return hot ? owner->_managed_primary_tile_bk_color_hl : owner->_managed_primary_tile_bk_color;
    else if (status == "SECONDARY")
      return hot ? owner->_managed_secondary_tile_bk_color_hl : owner->_managed_secondary_tile_bk_color;
    else if (status == "FAULTY")
      return hot ? owner->_managed_faulty_tile_bk_color_hl : owner->_managed_faulty_tile_bk_color;
    else if (status == "SPARE")
      return hot ? owner->_managed_spare_tile_bk_color_hl : owner->_managed_spare_tile_bk_color;
    return ConnectionEntry::get_current_color(hot);
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha, bool high_contrast)
  {
    ConnectionEntry::draw_tile_text(cr, x, y, alpha, high_contrast);

    std::string status = base::strip_text(connection->parameterValues().get("fabric_status").repr());
    std::string mode = base::strip_text(connection->parameterValues().get("fabric_mode").repr());

    y = bounds.top() + 56 - image_height(owner->_managed_status_icon);
    draw_icon_with_text(cr, bounds.left() + bounds.width()*2/3, y, owner->_managed_status_icon, status, alpha, high_contrast);

    y = bounds.top() + 74 - image_height(owner->_managed_status_icon);
    draw_icon_with_text(cr, bounds.left() + bounds.width()*2/3, y, owner->_managed_status_icon, mode, alpha, high_contrast);
  }

  virtual std::string section_name()
  {
    return "Group "+base::strip_text(connection->parameterValues().get("fabric_group_id").repr());
  }
};



class wb::FolderEntry : public ConnectionEntry
{
protected:
  virtual std::string get_acc_name()
  {
    return base::strfmt("%s %s", title.c_str(), _("Connection Group"));
  }

public:
  std::vector<boost::shared_ptr<ConnectionEntry> > children;

  FolderEntry(ConnectionsSection *aowner)
  : ConnectionEntry(aowner)
  {
    draw_info_tab = false;
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha, bool high_contrast)
  {
    double component = 0xF9 / 255.0;
    if (high_contrast)
      component = 1;
#ifdef __APPLE__
    cairo_set_source_rgba(cr, component, component, component, 0.8*alpha);
#else
    cairo_set_source_rgba(cr, component, component, component, alpha);
#endif

    std::string info = base::to_string(children.size() - 1) + " " + _("Connections");
    y = bounds.top() + 55;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, info.c_str());
    cairo_stroke(cr);
  }

  virtual mforms::Menu *context_menu()
  {
    return owner->_folder_context_menu;
  }

  virtual void menu_open(ItemPosition pos)
  {
    mforms::Menu *menu = context_menu();

    menu->set_item_enabled(menu->get_item_index("move_connection_to_top"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_up"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_down"), pos != Last);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_end"), pos != Last);
  }

  virtual void activate(boost::shared_ptr<ConnectionEntry> thisptr, int x, int y)
  {
    owner->change_to_folder(boost::dynamic_pointer_cast<FolderEntry>(thisptr));
    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }

  virtual base::Color get_current_color(bool hot)
  {
    return hot ? owner->_folder_tile_bk_color_hl : owner->_folder_tile_bk_color;
  }

  virtual cairo_surface_t *get_background_icon()
  {
    return owner->_folder_icon;
  }

  virtual wb::ConnectionInfoPopup *show_info_popup()
  {
    return NULL;
  }
};


class wb::FabricFolderEntry : public wb::FolderEntry
{
public:
  int total_instances;
  std::set<std::string> groups;

  FabricFolderEntry(ConnectionsSection *aowner)
  : FolderEntry(aowner), total_instances(0)
  {
    draw_info_tab = true;
  }

  virtual void activate(boost::shared_ptr<ConnectionEntry> thisptr, int x, int y)
  {
    owner->_owner->trigger_callback(ActionUpdateFabricConnections, connection);

    // the connection recreation may recreate the entry objects, so we need a fresh pointer
    ConnectionsSection::ConnectionVector conns(owner->displayed_connections());
    bool flag = false;
    for (ConnectionsSection::ConnectionVector::iterator iter = conns.begin(); iter != conns.end(); ++iter)
    {
      if ((*iter)->connection == connection)
      {
        flag = true;
        owner->change_to_folder(boost::dynamic_pointer_cast<FolderEntry>(*iter));
        break;
      }
    }
    if (!flag)
      log_error("Could not find fabric node '%s' object after refresh\n", connection->name().c_str());

    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }

  virtual mforms::Menu *context_menu()
  {
    return owner->_fabric_context_menu;
  }

  virtual base::Color get_current_color(bool hot)
  {
    return hot ? owner->_fabric_tile_bk_color_hl : owner->_fabric_tile_bk_color;
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha, bool high_contrast)
  {
    ConnectionEntry::draw_tile_text(cr, x, y, alpha, high_contrast);
    {
      std::string ha_filter = base::strip_text(connection->parameterValues().get("haGroupFilter").repr());

      std::string tile_groups;
      if (ha_filter.length())
      {
        std::vector<std::string> groups = base::split(ha_filter, ",");

        // Creates the legend to be displayed on the filter icon
        if (groups.size() > 2)
          tile_groups = base::strfmt("%s and %li others", groups[0].c_str(), (long)groups.size() - 1);
        else
          tile_groups = ha_filter;
      }

      if (tile_groups.length() > 0)
      {
        y = bounds.top() + 56 - image_height(owner->_ha_filter_icon);
        draw_icon_with_text(cr, bounds.center().x, y, owner->_ha_filter_icon, tile_groups, alpha, high_contrast);
      }
    }
  }

  virtual cairo_surface_t *get_background_icon()
  {
    return owner->_fabric_icon;
  }
};


class wb::FolderBackEntry : public ConnectionEntry
{
public:
  FolderBackEntry(ConnectionsSection *aowner)
  : ConnectionEntry(aowner)
  {
    title = "< back";
  }

  virtual bool is_movable()
  {
    return false;
  }

  virtual base::Color get_current_color(bool hot)
  {
    return hot ? owner->_back_tile_bk_color_hl : owner->_back_tile_bk_color;
  }

  virtual cairo_surface_t *get_background_icon()
  {
    return owner->_folder_icon;
  }

  /**
   * Separate tile drawing for the special back tile (to return from a folder).
   */
  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging,
                         bool high_contrast)
  {
    draw_tile_background(cr, hot, alpha, for_dragging);

    // Title string.
    double x = bounds.left() + 10;
    double y = bounds.top() + 27;
    cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);
    cairo_set_source_rgb(cr, 0xF9 / 255.0, 0xF9 / 255.0, 0xF9 / 255.0);

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, _("< back"));
    cairo_stroke(cr);
  }

  virtual mforms::Menu *context_menu()
  {
    return NULL;
  }

  virtual void menu_open(ItemPosition pos)
  {
  }

  virtual wb::ConnectionInfoPopup *show_info_popup()
  {
    return NULL;
  }

  virtual void activate(boost::shared_ptr<ConnectionEntry> thisptr, int x, int y)
  {
    owner->change_to_folder(boost::shared_ptr<FolderEntry>());
    // force a refresh of the hot_entry even if we don't move the mouse after clicking
    owner->mouse_move(mforms::MouseButtonNone, x, y);
  }
};


class wb::FabricServerEntry : public ConnectionEntry
{
public:
  wb::FabricFolderEntry *folder;

  FabricServerEntry(ConnectionsSection *aowner, wb::FabricFolderEntry *afolder)
  : ConnectionEntry(aowner), folder(afolder)
  {
  }

  virtual bool is_movable()
  {
    return false;
  }

  virtual base::Color get_current_color(bool hot)
  {
    return owner->_fabric_tile_bk_color;
  }

  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging,
                         bool high_contrast)
  {
    draw_tile_background(cr, hot, alpha, for_dragging);

    // Title string.
    double x = bounds.left() + 10;
    double y = bounds.top() + 27;
    cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);
    cairo_set_source_rgb(cr, 0xF9 / 255.0, 0xF9 / 255.0, 0xF9 / 255.0);

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, title.c_str());
    cairo_stroke(cr);

    draw_tile_text(cr, x, y, alpha, high_contrast);
  }

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha, bool high_contrast)
  {
    cairo_set_font_size(cr, HOME_SMALL_INFO_FONT_SIZE);
    y = bounds.top() + 56;
    draw_icon_with_text(cr, x, y, NULL, base::strfmt("Managed Instances: %i", folder->total_instances), alpha, high_contrast);

    y = bounds.top() + 70;
    draw_icon_with_text(cr, x, y, NULL, base::strfmt("# of HA Groups: %i", (int)folder->groups.size()), alpha, high_contrast);
  }

  virtual void menu_open(ItemPosition pos)
  {
  }

  virtual void activate(boost::shared_ptr<ConnectionEntry> thisptr, int x, int y)
  {
  }

  virtual mforms::Menu* context_menu()
  {
    return NULL;
  }

  virtual cairo_surface_t *get_background_icon()
  {
    return owner->_fabric_icon;
  }
};

//------------------------------------------------------------------------------------------------

ConnectionsSection::ConnectionsSection(HomeScreen *owner)
: _search_box(true), _search_text(mforms::SmallSearchEntry)
{
  _owner = owner;
  _page_start = 0;
  _connection_context_menu = NULL;
  _folder_context_menu = NULL;
  _fabric_context_menu = NULL;
  _generic_context_menu = NULL;
  _show_details = false;
  _drag_index = -1;
  _drop_index = -1;
  _filtered = false;

  std::vector<std::string> formats;
  formats.push_back(TILE_DRAG_FORMAT);           // We allow dragging tiles to reorder them.
  formats.push_back(mforms::DragFormatFileName); // We accept sql script files to open them.
  register_drop_formats(this, formats);

  _folder_icon = mforms::Utilities::load_icon("wb_tile_folder.png");
  _mouse_over_icon = mforms::Utilities::load_icon("wb_tile_mouseover.png");
  _mouse_over2_icon = mforms::Utilities::load_icon("wb_tile_mouseover_2.png");
  _network_icon = mforms::Utilities::load_icon("wb_tile_network.png");
  // TODO: We need a tile icon for the group filter and the status.
  _ha_filter_icon = mforms::Utilities::load_icon("wb_tile_network.png");
  _managed_status_icon = mforms::Utilities::load_icon("wb_tile_network.png");
  _page_down_icon = mforms::Utilities::load_icon("wb_tile_page-down.png");
  _page_up_icon = mforms::Utilities::load_icon("wb_tile_page-up.png");
  _plus_icon = mforms::Utilities::load_icon("wb_tile_plus.png");
  _sakila_icon = mforms::Utilities::load_icon("wb_tile_sakila.png");
  _fabric_icon = mforms::Utilities::load_icon("wb_tile_fabric.png");
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
  _search_text.set_font(HOME_NORMAL_FONT" 10");
  _search_box.set_size(-1, 18);
#else
  _search_box.set_padding(8, 1, 8, 5);
#endif

#ifdef _WIN32
  mforms::ImageBox *image = mforms::manage(new mforms::ImageBox, false);
  image->set_image("search_sidebar.png");
  image->set_image_align(mforms::MiddleCenter);
  _search_box.add(image, false, false);
#endif
  _search_text.set_name("Search Entry");
  _search_text.set_placeholder_text("Filter connections");
  _search_box.add(&_search_text, true, true);
  scoped_connect(_search_text.signal_changed(), boost::bind(&ConnectionsSection::on_search_text_changed, this));
  scoped_connect(_search_text.signal_action(), boost::bind(&ConnectionsSection::on_search_text_action, this, _1));
  add(&_search_box, mforms::TopRight);
  set_padding(0, 30, CONNECTIONS_RIGHT_PADDING, 0);

  _accessible_click_handler = boost::bind(&ConnectionsSection::mouse_click, this,
                                          mforms::MouseButtonLeft, _1, _2);
  scoped_connect(signal_resized(), boost::bind(&ConnectionsSection::on_resize, this));

  _add_button.name = "Add Connection";
  _add_button.default_action = "Open New Connection Wizard";
  _add_button.default_handler = _accessible_click_handler;

  _manage_button.name = "Manage Connections";
  _manage_button.default_action = "Open Connection Management Dialog";
  _manage_button.default_handler = _accessible_click_handler;

  _page_up_button.name = "Page Up";
  _page_up_button.default_action = "Move Connection Pages Up";
  _page_up_button.default_handler = _accessible_click_handler;

  _page_down_button.name = "Page Down";
  _page_down_button.default_action = "Move Connection Pages Down";
  _page_down_button.default_handler = _accessible_click_handler;
}

//------------------------------------------------------------------------------------------------

ConnectionsSection::~ConnectionsSection()
{
  if (_connection_context_menu != NULL)
    _connection_context_menu->release();
  if (_folder_context_menu != NULL)
    _folder_context_menu->release();
  if (_fabric_context_menu != NULL)
    _fabric_context_menu->release();
  if (_generic_context_menu != NULL)
    _generic_context_menu->release();

  delete_surface(_folder_icon);
  delete_surface(_mouse_over_icon);
  delete_surface(_mouse_over2_icon);
  delete_surface(_network_icon);
  delete_surface(_ha_filter_icon);
  delete_surface(_managed_status_icon);
  delete_surface(_page_down_icon);
  delete_surface(_page_up_icon);
  delete_surface(_plus_icon);
  delete_surface(_sakila_icon);
  delete_surface(_fabric_icon);
  delete_surface(_schema_icon);
  delete_surface(_user_icon);
  delete_surface(_manage_icon);

  if (_info_popup != NULL)
    delete _info_popup;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::update_colors()
{
#ifdef __APPLE__
  _tile_bk_color1 = base::Color::parse("#1e1e1e");
  _tile_bk_color1_hl = base::Color::parse("#3f3f3f");
#else
  _tile_bk_color1 = base::Color::parse("#666666");
  _tile_bk_color1_hl = base::Color::parse("#838383");
#endif

  _tile_bk_color2 = base::Color::parse("#868686");
  _tile_bk_color2_hl = base::Color::parse("#9b9b9b");

#ifdef __APPLE__
  _folder_tile_bk_color = base::Color::parse("#3477a6");
  _folder_tile_bk_color_hl = base::Color::parse("#4699b8");
#else
  _folder_tile_bk_color = base::Color::parse("#178ec5");
  _folder_tile_bk_color_hl = base::Color::parse("#63a6c5");
#endif

  _fabric_tile_bk_color = base::Color::parse("#349667");
  _fabric_tile_bk_color_hl = base::Color::parse("#46a889");

  _managed_primary_tile_bk_color = base::Color::parse("#13ae9e");
  _managed_primary_tile_bk_color_hl = base::Color::parse("#33cebe");
  _managed_secondary_tile_bk_color = base::Color::parse("#13b094");
  _managed_secondary_tile_bk_color_hl = base::Color::parse("#33d0b4");

  _managed_faulty_tile_bk_color = base::Color::parse("#e73414");
  _managed_faulty_tile_bk_color_hl = base::Color::parse("#ee5a40");
  _managed_spare_tile_bk_color = base::Color::parse("#8a8a8a");
  _managed_spare_tile_bk_color_hl = base::Color::parse("#9a9a9a");

  _back_tile_bk_color = base::Color::parse("#d9532c");
  _back_tile_bk_color_hl = base::Color::parse("#d97457");

  bool high_contrast = base::Color::is_high_contrast_scheme();
  _search_text.set_front_color(high_contrast ? "#000000" : "#FFFFFF");
  _search_text.set_placeholder_color(high_contrast ? "#303030" : "#A0A0A0");
  _search_text.set_back_color(high_contrast ? "#ffffff" : "#474747");
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::focus_search_box()
{
  _search_text.focus();
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::is_managed_connection(int index)
{
  bool is_managed = false;

  if (index > -1 && _active_folder)
  {
    if (_filtered)
      is_managed = _active_folder->children[index]->connection->parameterValues().has_key("fabric_managed");
    else
      is_managed = _active_folder->children[index]->connection->parameterValues().has_key("fabric_managed");
  }
  return is_managed;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::on_search_text_changed()
{
  std::string filter = _search_text.get_string_value();
  _filtered_connections.clear();

  _filtered = !filter.empty();
  if (_filtered)
  {
    ConnectionVector current_connections = !_active_folder ? _connections : _active_folder->children;
    for (ConnectionIterator iterator = current_connections.begin(); iterator != current_connections.end(); ++iterator)
    {
      // Always keep the first entry if we are in a folder. It's not filtered.
      if (_active_folder && (iterator == current_connections.begin()
                             || dynamic_cast<wb::FabricServerEntry*>(iterator->get())))
        _filtered_connections.push_back(*iterator);
      else
      if (base::contains_string((*iterator)->search_title, filter, false) ||
          base::contains_string((*iterator)->search_description, filter, false) ||
          base::contains_string((*iterator)->search_user, filter, false) ||
          base::contains_string((*iterator)->search_schema, filter, false))
      _filtered_connections.push_back(*iterator);
    }
  }
  set_needs_repaint();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::on_search_text_action(mforms::TextEntryAction action)
{
  if (action == mforms::EntryEscape)
  {
    _search_text.set_value("");
    on_search_text_changed();
  }
  else if (action == mforms::EntryActivate)
  {
    if (_active_folder)
    {
      // Within a folder.
      switch (_filtered_connections.size())
      {
        case 1: // Just the back tile. Return to top level.
          _page_start = _page_start_backup;
          _active_folder.reset();
          _filtered = false;
          _search_text.set_value("");
          set_needs_repaint();
          break;

        case 2: // Exactly one entry matched the filter. Activate it.
          _owner->trigger_callback(ActionOpenConnectionFromList, _filtered_connections[1]->connection);
          break;
      }
    }
    else
    {
      if (!_filtered_connections.empty())
      {
        FolderEntry* folder = dynamic_cast<FolderEntry*>(_filtered_connections[0].get());
        // If only one entry is visible through filtering activate it. I.e. for a group show its content
        // and for a connection open it.
        if (folder && folder->children.size() > 1)
        {
          _page_start_backup = _page_start;
          _page_start = 0;

          // Loop through the unfiltered list to find the index of the group we are about to open.
          _active_folder.reset(); // Just a defensive action. Should never play a role.
          for (size_t i = 0; i < _connections.size(); ++i)
          {
            if (_connections[i]->title == _filtered_connections[0]->title)
            {
              _active_folder = boost::dynamic_pointer_cast<FolderEntry>(_connections[i]);
              break;
            }
          }
          _filtered = false;
          _search_text.set_value("");
          set_needs_repaint();
        }
        else
        _owner->trigger_callback(ActionOpenConnectionFromList, _filtered_connections[0]->connection);
      }
    }
  }
}

//------------------------------------------------------------------------------------------------

/**
 * Computes the index for the given position, regardless if that is actually backed by an existing
 * entry or not.
 *
 * This will not work in section separated folders (like in Fabric), but it doesn't matter
 * atm because this is only used for drag/drop, which is not supported inside a Fabric folder.
 */
ssize_t ConnectionsSection::calculate_index_from_point(int x, int y)
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

  return _page_start + row * tiles_per_row + column;
}

//------------------------------------------------------------------------------------------------

boost::shared_ptr<ConnectionEntry> ConnectionsSection::entry_from_point(int x, int y, bool &in_details_area)
{
  in_details_area = false;
  boost::shared_ptr<ConnectionEntry> entry;

  ConnectionVector connections(displayed_connections());
  if (_page_start > (ssize_t)connections.size())
    return entry;
  for (ConnectionVector::iterator conn = connections.begin() + _page_start; conn != connections.end(); ++conn)
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


boost::shared_ptr<ConnectionEntry> ConnectionsSection::entry_from_index(ssize_t index)
{
  ssize_t count = displayed_connections().size();
  if (index < count)
  {
    return displayed_connections()[index];
  }
  return boost::shared_ptr<ConnectionEntry>();
}

//------------------------------------------------------------------------------------------------

base::Rect ConnectionsSection::bounds_for_entry(ssize_t index)
{
  base::Rect result(CONNECTIONS_LEFT_PADDING, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
  int tiles_per_row = (get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  index -= _page_start;

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
db_mgmt_ConnectionRef ConnectionsSection::connection_from_index(ssize_t index)
{
  if (index < 0 || (_active_folder && index == 0))
    return db_mgmt_ConnectionRef();

  return displayed_connections()[index]->connection;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::draw_paging_part(cairo_t *cr, int current_page, int pages, bool high_contrast)
{
  cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);

  std::string page_string = base::strfmt("%d/%d", ++current_page, pages);
  cairo_text_extents_t extents;
  cairo_text_extents(cr, page_string.c_str(), &extents);

  _page_down_button.bounds = base::Rect(0, 0, image_width(_page_down_icon), image_height(_page_down_icon));
  double y = get_height() - _page_down_button.bounds.width() - 6;
  double x = get_width() - extents.width - 8;
  double icon_x = x + ceil((extents.width - _page_down_button.bounds.width()) / 2.0) + 1;
  _page_down_button.bounds.pos = base::Point(icon_x, y);

  cairo_set_source_surface(cr, _page_down_icon, icon_x, y);
  if (high_contrast)
    cairo_set_operator(cr, CAIRO_OPERATOR_XOR);
  if (current_page == pages)
  {
    // If we are on the last page then dim the page down button and remove the button
    // rectangle used for hit tests (so the user can't click it).
    cairo_paint_with_alpha(cr, 0.5);
    _page_down_button.bounds = base::Rect();
  }
  else
    cairo_paint(cr);

  if (high_contrast)
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  y -= 6;

  double component = 0x5E / 255.0;
  if (high_contrast)
  component = 0;
  cairo_set_source_rgb(cr, component, component, component);
  cairo_move_to(cr, x, y);
  cairo_show_text(cr, page_string.c_str());
  cairo_stroke(cr);

  _page_up_button.bounds = base::Rect(icon_x, 0, image_width(_page_up_icon), image_height(_page_up_icon));
  y -= extents.height + 6 + _page_up_button.bounds.height();
  _page_up_button.bounds.pos.y = y;

  cairo_set_source_surface(cr, _page_up_icon, icon_x, y);
  if (high_contrast)
  cairo_set_operator(cr, CAIRO_OPERATOR_XOR);

  if (current_page == 1)
  {
    cairo_paint_with_alpha(cr, 0.5);
    _page_up_button.bounds = base::Rect();
  }
  else
    cairo_paint_with_alpha(cr, 1);

  if (high_contrast)
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
{
  int width = get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING;
  int height = get_height();

  int tiles_per_row = width / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

  cairo_select_font_face(cr, HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, HOME_TITLE_FONT_SIZE);

  bool high_contrast = base::Color::is_high_contrast_scheme();
  if (high_contrast)
    cairo_set_source_rgb(cr, 0, 0, 0);
  else
    cairo_set_source_rgb(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0);
  cairo_move_to(cr, CONNECTIONS_LEFT_PADDING, 45);

  ConnectionVector *connections;
  std::string title = _("MySQL Connections");
  if (dynamic_cast<FabricFolderEntry*>(_active_folder.get()))
  {
    title += " / " + _active_folder->title + " / Managed MySQL Servers";
    connections = &_active_folder->children;
  }
  else if (_active_folder)
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

  _add_button.bounds = base::Rect(CONNECTIONS_LEFT_PADDING + text_width + 10, 45 - image_height(_plus_icon),
                                  image_width(_plus_icon), image_height(_plus_icon));

  cairo_set_source_surface(cr, _plus_icon, _add_button.bounds.left(), _add_button.bounds.top());
  if (high_contrast)
    cairo_set_operator(cr, CAIRO_OPERATOR_XOR);
  cairo_paint(cr);

  _manage_button.bounds = base::Rect(_add_button.bounds.right() + 10, 45 - image_height(_manage_icon),
                                     image_width(_manage_icon), image_height(_manage_icon));
  cairo_set_source_surface(cr, _manage_icon, _manage_button.bounds.left(), _manage_button.bounds.top());
  cairo_paint(cr);

  if (high_contrast)
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER); // Restore default operator.

  int row = 0;
  // number of tiles that act as a filler, which are used by the fabric server title tile and also in
  // fabric groups separated by group name (or folder sections)
  int filler_tiles = 0;
  std::string current_section;
  base::Rect bounds(0, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
  bool done = false;
  int visible_page = (int)_prev_page_start.size();
  int current_page = 0;
  int num_pages = 0;
  bool draw_partial = false;
  ssize_t index = 0;
  int items_after_last_visible = 0;
  bool page_start = true;

  _next_page_start = -1;
  _entries_per_page = -1;
  while (!done)
  {
    bounds.pos.x = CONNECTIONS_LEFT_PADDING;
    double alpha = bounds.bottom() > height ? 0.25 : 1;

    for (int column = 0; column < tiles_per_row; column++)
    {
      if (index >= (int)connections->size())
      {
        // If the last item is reached and it is being painted OK then we
        // are done
        if (!draw_partial)
          done = true;
        break;
      }
      else
      {
        if (page_start)
        {
          num_pages++;
          page_start = false;
        }

        // We will just not paint anything that is not on the current page.
        bool dont_paint = current_page != visible_page;

        if (current_page > visible_page)
          items_after_last_visible++;

        if (dynamic_cast<FabricServerEntry*>((*connections)[index].get()))
        {
          base::Rect total_bounds = bounds;
          int tiles_occupied = tiles_per_row - column;
          filler_tiles += tiles_occupied;
          column += (tiles_occupied - 1);

          total_bounds.size.width = CONNECTIONS_TILE_WIDTH * tiles_occupied + CONNECTIONS_SPACING * (tiles_occupied - 1);
          if (!dont_paint)
            (*connections)[index]->bounds = total_bounds;
        }
        else
        {
          std::string section = (*connections)[index]->section_name();
          if (!section.empty() && current_section != section)
          {
            current_section = section;
            bounds.pos.y += HOME_TILES_TITLE_FONT_SIZE + CONNECTIONS_SPACING;

            if (!dont_paint)
            {
              // draw the section title
              bool high_contrast = base::Color::is_high_contrast_scheme();
              cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
              cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);
              if (high_contrast)
                cairo_set_source_rgb(cr, 0, 0, 0);
              else
                cairo_set_source_rgb(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0);
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

          if (!dont_paint)
          {
            // Updates the bounds on the tile
            (*connections)[index]->bounds = bounds;
          }
        }

        if (!dont_paint)
        {
          bool draw_hot = (*connections)[index] == _hot_entry;

          int draw_position = (row % 2) + column;
          if (!_active_folder)
            (*connections)[index]->second_color = (draw_position % 2) != 0;
          (*connections)[index]->draw_tile(cr, draw_hot, alpha, false, high_contrast);

          // Draw drop indicator.
          if (index == _drop_index)
          {
            if (high_contrast)
              cairo_set_source_rgb(cr, 0, 0, 0);
            else
              cairo_set_source_rgb(cr, 1, 1, 1);

            if (_drop_position == mforms::DropPositionOn)
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
              if (_drop_position == mforms::DropPositionRight)
                x = bounds.right() + 4.5;
              cairo_move_to(cr, x, bounds.top());
              cairo_line_to(cr, x, bounds.bottom());
              cairo_set_line_width(cr, 3);
              cairo_stroke(cr);
              cairo_set_line_width(cr, 1);
            }
          }
        }
      }
      index++;
      bounds.pos.x += CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING;
    }

    row++;
    bounds.pos.y += CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING;


    // next tile row is on screen, but doesn't totally fit
    if (bounds.bottom() >= height && bounds.top() <= height)
    {
      // We light flag to indicate the next row is partially drawn
      // And backup the indext of the first item of this row which will be
      // the first item on the next page.
      draw_partial = true;
      _next_page_start = index;
      if (_entries_per_page < 0)
        _entries_per_page = index;
    }

    // Next row is totally out of the available space
    if (bounds.top() > height)
    {
      // It is expected to usually come from a page where last row was
      // partially painted, if so, re reset the flag.
      // On this case _next_page_start contains the index of the first partially painted tile
      // which will be the first item on the next page
      if (draw_partial)
        draw_partial = false;
      else
      {
        // This case there was not row painted partially, so next page start item
        // is the current index.
        _next_page_start = index;
        if (_entries_per_page < 0)
          _entries_per_page = index;
      }
        

      // We reinit these vars so the calculation of the next page is done correctly
      bounds.pos.y = CONNECTIONS_TOP_PADDING;
      page_start = true;
      current_page++;

      // Restores index with the _next_page_start so the calculation or subsequent pages
      // is done properly
      index = _next_page_start;
    }
  }

  if (visible_page >= num_pages)
  {
    --visible_page;
    _prev_page_start.pop_back();
    set_needs_repaint();
  }
  // See if we need to draw the paging indicator.
  if (num_pages > 1)
  {
    draw_paging_part(cr, visible_page, num_pages, high_contrast);
  }
  else
  {
    _page_up_button.bounds = base::Rect();
    _page_down_button.bounds = base::Rect();
    _page_start = 0; // Size increased to cover the full content.
  }
}

//--------------------------------------------------------------------------------------------------

void ConnectionsSection::on_resize()
{
  _page_start = _prev_page_start.size() * _entries_per_page;
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::add_connection(const db_mgmt_ConnectionRef &connection, const std::string &title,
                                        const std::string &description, const std::string &user, const std::string &schema)
{
  boost::shared_ptr<ConnectionEntry> entry;

  if (connection.is_valid() && connection->driver().is_valid() && connection->driver()->name() == "MySQLFabric")
  {
    FabricFolderEntry *fabric_folder;
    entry = boost::shared_ptr<ConnectionEntry>(fabric_folder = new FabricFolderEntry(this));

    fabric_folder->children.push_back(boost::shared_ptr<ConnectionEntry>(new FolderBackEntry(this)));
    {
      boost::shared_ptr<ConnectionEntry> fabric(new FabricServerEntry(this, fabric_folder));
      fabric->title = "Fabric Server: " + *connection->name();
      fabric->connection = connection;
      fabric->second_color = false;
      fabric->search_title = title;
      fabric_folder->children.push_back(fabric);
    }
  }
  else if (connection.is_valid() && connection->parameterValues().has_key("fabric_managed"))
    entry = boost::shared_ptr<ConnectionEntry>(new FabricManagedConnectionEntry(this));
  else
    entry = boost::shared_ptr<ConnectionEntry>(new ConnectionEntry(this));

  entry->connection = connection;
  entry->title = title;
  entry->description = description;
  entry->user = user;
  entry->schema = schema;
  entry->compute_strings = true;
  entry->second_color = false;

  entry->search_title = title;
  entry->search_description = description;
  entry->search_user = user;
  entry->search_schema = schema;

  entry->default_handler = boost::bind(&ConnectionsSection::mouse_click, this,
                                      mforms::MouseButtonLeft, _1, _2);

  std::string::size_type slash_position = title.find("/");
  if (slash_position != std::string::npos)
  {
    // A child entry->
    std::string parent_name = title.substr(0, slash_position);
    entry->title = title.substr(slash_position + 1);
    entry->search_title = entry->title;
    bool found_parent = false;
    for (ConnectionIterator iterator = _connections.begin(); iterator != _connections.end(); iterator++)
    {
      if ((*iterator)->title == parent_name)
      {
        if (FabricFolderEntry *folder = dynamic_cast<FabricFolderEntry*>(iterator->get()))
        {
          found_parent = true;
          std::vector<boost::shared_ptr<ConnectionEntry> >::iterator index, end;
          index = folder->children.begin(); 
          end = folder->children.end();

          // Skips the back and server tiles
          index++;
          index++;

          std::string key = base::strfmt("%s-%s", entry->section_name().c_str(), entry->title.c_str());
          bool found = false;

          while (index != end && !found)
          {
            std::string existing_key = base::strfmt("%s-%s", (*index)->section_name().c_str(), (*index)->title.c_str());

            found = key < existing_key;
              
            if (!found)
              index++;
          }

          folder->children.insert(index, entry);
          folder->total_instances++;
          folder->groups.insert(entry->section_name());
          break;
        }
        else if (FolderEntry *folder = dynamic_cast<FolderEntry*>(iterator->get()))
        {
          found_parent = true;
          folder->children.push_back(entry);
          break;
        }
      }
    }

    // If the parent was not found, a folder should be created
    if (!found_parent)
    {
      boost::shared_ptr<FolderEntry> folder(new FolderEntry(this));

      folder->title = parent_name;
      folder->compute_strings = true;
      folder->second_color = false;
      folder->search_title = parent_name;

      folder->children.push_back(boost::shared_ptr<ConnectionEntry>(new FolderBackEntry(this)));
      folder->children.push_back(entry);
      _connections.push_back(boost::dynamic_pointer_cast<ConnectionEntry>(folder));
      if (!_active_folder_title_before_refresh_start.empty() && _active_folder_title_before_refresh_start == folder->title)
      {
        _active_folder = boost::dynamic_pointer_cast<FolderEntry>(_connections.back());
        _active_folder_title_before_refresh_start.clear();
      }
    }
  }
  else
    _connections.push_back(entry);
  set_layout_dirty(true);
  set_needs_repaint();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::clear_connections(bool clear_state)
{
  if (clear_state)
  {
    _filtered = false;
    _filtered_connections.clear();
    _search_text.set_value("");
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

void ConnectionsSection::change_to_folder(boost::shared_ptr<FolderEntry> folder)
{
  if (_active_folder && !folder)
  {
    // Returning to root list.
    _page_start = _page_start_backup;
    _next_page_start = _next_page_start_backup;
    _prev_page_start = _prev_page_start_backup;
    _active_folder.reset();
    _filtered = false;
    _search_text.set_value("");
    set_needs_repaint();
  }
  else if (folder)
  {
    _active_folder = folder;
    // Drilling into a folder.
    _page_start_backup = _page_start;
    _next_page_start_backup = _next_page_start;
    _prev_page_start_backup = _prev_page_start;
    _page_start = 0;
    _next_page_start = 0;
    _prev_page_start.clear();
    _filtered = false;
    _search_text.set_value("");
    set_needs_repaint();
  }
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_down(mforms::MouseButton button, int x, int y)
{
  if (button == mforms::MouseButtonLeft && _hot_entry)
    _mouse_down_position = base::Rect(x - 4, y - 4, 8, 8); // Center a 8x8 pixels rect around the mouse position.
  return false; // Continue with standard mouse handling.
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_up(mforms::MouseButton button, int x, int y)
{
  _mouse_down_position = base::Rect();
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_double_click(mforms::MouseButton button, int x, int y)
{
  switch (button)
  {
    case mforms::MouseButtonLeft:
    {
      // In order to allow quick clicking for page flipping we also handle double clicks for this.
      if (_page_up_button.bounds.contains(x, y))
      {
        if (!_prev_page_start.empty())
        {
          _page_start = _prev_page_start.back();
          _prev_page_start.pop_back();
          set_needs_repaint();
        }
        return true;
      }

      if (_page_down_button.bounds.contains(x, y))
      {
        _prev_page_start.push_back(_page_start);
        _page_start = _prev_page_start.size() * _entries_per_page;
        set_needs_repaint();
        return true;
      }

      break;
    }

    default: // Silence LLVM.
      break;
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_click(mforms::MouseButton button, int x, int y)
{
  // everything below this relies on _hot_entry, which will become out of sync
  // if the user pops up the context menu and then clicks (or right clicks) in some
  // other tile... so we must first update _hot_entry before doing any actions
  mouse_move(mforms::MouseButtonNone, x, y);

  switch (button)
  {
    case mforms::MouseButtonLeft:
    {
      if (_add_button.bounds.contains(x, y))
      {
        _owner->trigger_callback(ActionNewConnection, grt::ValueRef());
        return true;
      }

      if (_manage_button.bounds.contains(x, y))
      {
        _owner->trigger_callback(ActionManageConnections, grt::ValueRef());
        return true;
      }

      if (_page_up_button.bounds.contains(x, y))
      {
        if (!_prev_page_start.empty())
        {
          // Page up clicked. Doesn't happen if we are on the first page already.
          _page_start = _prev_page_start.back();
          _prev_page_start.pop_back();
          set_needs_repaint();
        }
        return true;
      }

      if (_page_down_button.bounds.contains(x, y))
      {
        _prev_page_start.push_back(_page_start);
        _page_start = _prev_page_start.size() * _entries_per_page;
        set_needs_repaint();
        return true;
      }

      if (_hot_entry)
      {
#ifdef __APPLE__
        bool show_info = _info_button_rect.contains_flipped(x, y);
#else
        bool show_info = _show_details;
#endif

        if (show_info && !_info_popup && _parent && (_info_popup = _hot_entry->show_info_popup()))
        {
          scoped_connect(_info_popup->on_close(), boost::bind(&ConnectionsSection::popup_closed, this));

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

bool ConnectionsSection::mouse_leave()
{
  // Ignore mouse leaves if we are showing the info popup. We want the entry to stay hot.
  if (_info_popup != NULL)
    return true;

  if (_hot_entry)
  {
    _hot_entry.reset();
    _show_details = false;
    set_needs_repaint();
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_move(mforms::MouseButton button, int x, int y)
{
  bool in_details_area;
  boost::shared_ptr<ConnectionEntry> entry = entry_from_point(x, y, in_details_area);

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
      if (entry != _hot_entry || _show_details != in_details_area)
      {
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

void ConnectionsSection::set_context_menu(mforms::Menu *menu, HomeScreenMenuType type)
{
  switch (type)
  {
    case HomeMenuConnectionGroup:
      if (_folder_context_menu != NULL)
        _folder_context_menu->release();
      _folder_context_menu = menu;
      if (_folder_context_menu != NULL)
      {
        _folder_context_menu->retain();
        menu->set_handler(boost::bind(&ConnectionsSection::handle_folder_command, this, _1, false));
      }
      break;

    case HomeMenuConnectionFabric:
      if (_fabric_context_menu != NULL)
        _fabric_context_menu->release();
      _fabric_context_menu = menu;
      if (_fabric_context_menu != NULL)
      {
        _fabric_context_menu->retain();
        menu->set_handler(boost::bind(&ConnectionsSection::handle_folder_command, this, _1, true));
      }
      break;

    case HomeMenuConnection:
      if (_connection_context_menu != NULL)
        _connection_context_menu->release();
      _connection_context_menu = menu;
      if (_connection_context_menu != NULL)
      {
        _connection_context_menu->retain();
        menu->set_handler(boost::bind(&ConnectionsSection::handle_command, this, _1));
      }
      break;

    default:
      if (_generic_context_menu != NULL)
        _generic_context_menu->release();
      _generic_context_menu = menu;
      if (_generic_context_menu != NULL)
      {
        _generic_context_menu->retain();
        menu->set_handler(boost::bind(&ConnectionsSection::handle_command, this, _1));
      }
      break;
  }

  if (menu != NULL)
    scoped_connect(menu->signal_will_show(), boost::bind(&ConnectionsSection::menu_open, this));
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::handle_command(const std::string &command)
{
  grt::ObjectRef item;
  if (_entry_for_menu)
  {
    if (_active_folder)
    {
      if (command == "delete_connection_all")
      {
        // We only want to delete all connections in the active group. This is the same as
        // removing the group entirely, since the group is formed by connections in it.
        _entry_for_menu = _active_folder;
        handle_folder_command("delete_connection_group", dynamic_cast<FabricFolderEntry*>(_active_folder.get()) != NULL);
        return;
      }
      else
      {
        item = _entry_for_menu->connection;
      }
    }
    else
    {
      item = _entry_for_menu->connection;
    }
  }
  _owner->handle_context_menu(item, command);
  _entry_for_menu.reset();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::handle_folder_command(const std::string &command, bool is_fabric)
{
  grt::ValueRef item;

  if (is_fabric && !base::starts_with(command, "move") && command != "internal_delete_connection_group")
  {
    if (_entry_for_menu)
      item = _entry_for_menu->connection;

    _owner->handle_context_menu(item, command);
  }
  else
  {
    // We have to pass on a valid connection (for the group name).
    // All child items have the same group name (except the dummy entry for the back tile).
    std::string title;
    if (_entry_for_menu)
      title = _entry_for_menu->title;

    title += "/";

    _owner->handle_context_menu(grt::StringRef(title), command);
    _entry_for_menu.reset();
  }

}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::menu_open()
{
  if (_entry_for_menu)
  {
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

void ConnectionsSection::hide_info_popup()
{
  if (_info_popup != NULL)
  {
    _hot_entry.reset();
    _show_details = false;

    _info_popup->release();
    _info_popup = NULL;

    set_needs_repaint();
  }
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::popup_closed()
{
  hide_info_popup();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::cancel_operation()
{
  _owner->cancel_script_loading();
}

//------------------------------------------------------------------------------------------------

int ConnectionsSection::get_acc_child_count()
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

  // Adds 2 because of the pageup/pagedown icons if the
  // icons are being displayed.
  if (_page_up_button.bounds.width())
    ret_val += 2;

  return (int)ret_val;
}

mforms::Accessible* ConnectionsSection::get_acc_child(int index)
{
  mforms::Accessible* accessible = NULL;

  switch (index)
  {
    case 0:
      accessible = &_add_button;
      break;
    case 1:
      accessible = &_manage_button;
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

      if (!accessible)
      {
        accessible = !index ? &_page_up_button : &_page_down_button;
      }
    }
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

std::string ConnectionsSection::get_acc_name()
{
  return get_name();
}

//------------------------------------------------------------------------------------------------

mforms::Accessible::Role ConnectionsSection::get_acc_role()
{
  return Accessible::List;
}

//------------------------------------------------------------------------------------------------

mforms::Accessible* ConnectionsSection::hit_test(int x, int y)
{
  mforms::Accessible* accessible = NULL;

  if (_add_button.bounds.contains(x, y))
    accessible = &_add_button;
  else if (_manage_button.bounds.contains(x, y))
    accessible = &_manage_button;
  else if (_page_up_button.bounds.contains(x, y))
    accessible = &_page_up_button;
  else if (_page_down_button.bounds.contains(x, y))
    accessible = &_page_down_button;
  else
  {
    bool in_details_area = false;
    boost::shared_ptr<ConnectionEntry> entry = entry_from_point(x, y, in_details_area);

    if (entry)
      accessible = entry.get();
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::do_tile_drag(ssize_t index, int x, int y)
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
    boost::shared_ptr<ConnectionEntry> entry = entry_from_index(index);
    if (entry)
    {
      entry->draw_tile(cr, false, 1, true, false); // There's no drag tile actually in high contrast mode.

      _drag_index = index;
      mforms::DragOperation operation = do_drag_drop(details, entry.get(), TILE_DRAG_FORMAT);
      _mouse_down_position = base::Rect();
      cairo_surface_destroy(details.image);
      cairo_destroy(cr);

      _drag_index = -1;
      _drop_index = -1;
      set_needs_repaint();

      if (operation == mforms::DragOperationMove) // The actual move is done in the drop delegate method.
        return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------------------------------

// Drop delegate implementation.
mforms::DragOperation ConnectionsSection::drag_over(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                const std::vector<std::string> &formats)
{
  if (allowedOperations == mforms::DragOperationNone)
    return allowedOperations;

  if (std::find(formats.begin(), formats.end(), mforms::DragFormatFileName) != formats.end())
  {
    // Indicate we can accept files if one of the connection tiles is hit.
    bool in_details_area;
    boost::shared_ptr<ConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y, in_details_area);

    if (!entry)
      return mforms::DragOperationNone;

    if (!entry->connection.is_valid())
      return mforms::DragOperationNone;

    if (_hot_entry != entry)
    {
      _hot_entry = entry;
      set_needs_repaint();
    }
    return allowedOperations & mforms::DragOperationCopy;
  }

  if (std::find(formats.begin(), formats.end(), TILE_DRAG_FORMAT) != formats.end())
  {
    // A tile is being dragged. Find the target index and drop location for visual feedback.
    // Computation here is more relaxed than the normal hit test as we want to allow dropping
    // left, right and below the actual tiles area too as well as between tiles.
    if (p.y < CONNECTIONS_TOP_PADDING)
    {
      if (_drop_index > -1)
      {
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
      if (_drop_index > -1)
      {
        _drop_index = -1;
        set_needs_repaint();
      }
      return mforms::DragOperationNone; // Drop on the dimmed row. No drop action here.
    }

    int index = (int)(_page_start + row * tiles_per_row);
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
        boost::shared_ptr<ConnectionEntry> entry = entry_from_index(index);
        if (entry && dynamic_cast<FolderEntry*>(entry.get()))
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
    if (_drag_index > -1 && (index == _drag_index ||
        (index + 1 == _drag_index && position == mforms::DropPositionRight) ||
        (index - 1 == _drag_index && position == mforms::DropPositionLeft) ||
        (position == mforms::DropPositionOn && dynamic_cast<FolderEntry*>(entry_from_index(_drag_index).get()))))
    {
      index = -1;
    }
    else if (!_filtered && _active_folder && index == 0 && position == mforms::DropPositionLeft)
    {
      if (is_managed_connection((int)_drag_index))
        return mforms::DragOperationNone;
      else
        position = mforms::DropPositionOn; // Drop on back tile.
    }

    if (_drop_index != index || _drop_position != position)
    {
      _drop_index = index;
      _drop_position = position;
      set_needs_repaint();
    }

    return mforms::DragOperationMove;
  }

  return mforms::DragOperationNone;
}

//------------------------------------------------------------------------------------------------

mforms::DragOperation ConnectionsSection::files_dropped(View *sender, base::Point p, mforms::DragOperation allowedOperations,
                                    const std::vector<std::string> &file_names)
{
  bool in_details_area;
  boost::shared_ptr<ConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y, in_details_area);
  if (!entry || dynamic_cast<FabricFolderEntry*>(entry.get()))
    return mforms::DragOperationNone;

  db_mgmt_ConnectionRef connection = entry->connection;
  if (connection.is_valid())
  {
    grt::GRT *grt = connection->get_grt();

    // Allow only sql script files to be dropped.
    grt::StringListRef valid_names(grt);
    for (size_t i = 0; i < file_names.size(); ++i)
      if (base::tolower(base::extension(file_names[i])) == ".sql")
        valid_names.insert(file_names[i]);

    if (valid_names.count() == 0)
    return mforms::DragOperationNone;

    grt::DictRef details(grt);
    details.set("connection", connection);
    details.set("files", valid_names);
    _owner->trigger_callback(ActionFilesWithConnection, details);
  }

  return mforms::DragOperationCopy;
}

//------------------------------------------------------------------------------------------------

mforms::DragOperation ConnectionsSection::data_dropped(mforms::View *sender, base::Point p,
                                   mforms::DragOperation allowedOperations, void *data, const std::string &format)
{
  if (format == TILE_DRAG_FORMAT && _drop_index > -1)
  {
    mforms::DragOperation result = mforms::DragOperationNone;

    // Can be invalid if we move a group.
    db_mgmt_ConnectionRef connection = connection_from_index(_drag_index);
    ConnectionEntry *source_entry = static_cast<ConnectionEntry*>(data);

    boost::shared_ptr<ConnectionEntry> entry;
    if (_filtered)
    {
      if (_drop_index < (int)_filtered_connections.size())
        entry = _filtered_connections[_drop_index];
    }
    else if (_active_folder)
    {
      if (_drop_index < (int)_active_folder->children.size())
        entry = _active_folder->children[_drop_index];
    }
    else
    {
      if (_drop_index < (int)_connections.size())
        entry = _connections[_drop_index];
    }

    if (!entry)
      return result;

    bool is_back_tile = entry->title == "< back";

    // Drop target is a group.
    grt::DictRef details(_owner->rdbms().get_grt());
    if (connection.is_valid() && connection->driver()->name()!="MySQLFabric")
      details.set("object", connection);
    else
      details.set("object", grt::StringRef(source_entry->title + "/"));

    if (_drop_position == mforms::DropPositionOn)
    {
      // Drop on a group (or back tile).
      if (is_back_tile)
        details.set("group", grt::StringRef("*Ungrouped*"));
      else
        details.set("group", grt::StringRef(entry->title));
      _owner->trigger_callback(ActionMoveConnectionToGroup, details);
    }
    else
    {
      // Drag from one position to another within a group (root or active group).
      size_t to = _drop_index;
      if (_active_folder)
        to--; // The back tile has no representation in the global list.
      if (_drop_position == mforms::DropPositionRight)
        to++;

      details.set("to", grt::IntegerRef((int)to));
      _owner->trigger_callback(ActionMoveConnection, details);
    }
    result = mforms::DragOperationMove;

    _drop_index = -1;
    set_needs_repaint();

    return result;
  }
  return mforms::DragOperationNone;
}

ConnectionsSection::ConnectionVector &ConnectionsSection::displayed_connections()
{
  if (_filtered)
    return _filtered_connections;
  else if (_active_folder)
    return _active_folder->children;
  else
    return _connections;
}
