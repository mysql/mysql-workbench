/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include "grts/structs.app.h"
#include "grts/structs.db.mgmt.h"

#include "home_screen.h"

#include "workbench/wb_context_names.h"

using namespace wb;

#ifdef __APPLE__
  #define HOME_TITLE_FONT "Helvetica Neue Light"
  #define HOME_NORMAL_FONT "Helvetica Neue Light"
  #define HOME_DETAILS_FONT "Helvetica Neue Light"
  // Info font is only used on Mac.
  #define HOME_INFO_FONT "Baskerville"
#elif defined(_WIN32)
  #define HOME_TITLE_FONT "Segoe UI"
  #define HOME_NORMAL_FONT "Segoe UI"
  #define HOME_NORMAL_FONT_XP "Tahoma"

  #define HOME_DETAILS_FONT "Segoe UI"
#else
  #define HOME_TITLE_FONT "Tahoma"
  #define HOME_NORMAL_FONT "Tahoma"
  #define HOME_DETAILS_FONT "Helvetica"
#endif

#define HOME_TITLE_FONT_SIZE 20
#define HOME_SUBTITLE_FONT_SIZE 13

#define HOME_TILES_TITLE_FONT_SIZE 16
#define HOME_SMALL_INFO_FONT_SIZE 10
#define HOME_DETAILS_FONT_SIZE 12

#define TILE_DRAG_FORMAT "com.mysql.workbench-drag-tile-format"

//--------------------------------------------------------------------------------------------------

// The following helpers are just temporary. They will be replaced by a cairo context class.
static void delete_surface(cairo_surface_t* surface)
{
  if (surface != NULL)
    cairo_surface_destroy(surface);
}

//--------------------------------------------------------------------------------------------------

static int image_width(cairo_surface_t* image)
{
  if (image != NULL)
    return cairo_image_surface_get_width(image);
  return 0;
}

//--------------------------------------------------------------------------------------------------

static int image_height(cairo_surface_t* image)
{
  if (image != NULL)
    return cairo_image_surface_get_height(image);
  return 0;
}

//--------------------------------------------------------------------------------------------------

/**
 * Helper to draw text with a hot decoration.
 */
void text_with_decoration(cairo_t* cr, double x, double y, const char* text, bool hot, double width)
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

class ConnectionInfoPopup : public mforms::Popup
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

    // Instance info next.
    line_bounds = bounds;
    line_bounds.pos.x += (bounds.width() + POPUP_LR_PADDING) / 2;
    line_bounds.pos.y += DETAILS_TOP_OFFSET;
    line_bounds.size.width = (bounds.width() - POPUP_LR_PADDING) / 2;

    // Make sure the entire right part does not extend beyond the available horizontal space.
    if (line_bounds.right() > bounds.right())
      line_bounds.pos.x -= bounds.right() - line_bounds.right();

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

    print_info_line(cr, line_bounds, _("Group Filter"), parameter_values.get_string("haGroupFilter"));
    line_bounds.pos.y += DETAILS_LINE_HEIGHT;
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
#define CONNECTIONS_TOP_PADDING   75 // The vertical offset of the first visible shortcut entry.
#define CONNECTIONS_SPACING        9 // Vertical/horizontal space between entries.

#define CONNECTIONS_TILE_WIDTH   241
#define CONNECTIONS_TILE_HEIGHT   91

struct HomeAccessibleButton : mforms::Accessible
{
  std::string name;
  std::string default_action;
  base::Rect bounds;
  boost::function <bool (int, int)> default_handler;

  // ------ Accesibility Customized Methods -----

  virtual std::string get_acc_name() { return name; }
  virtual std::string get_acc_default_action() { return default_action;}
  virtual Accessible::Role get_acc_role() { return Accessible::PushButton;}
  virtual base::Rect get_acc_bounds() { return bounds;}

  virtual void do_default_action()
  {
    if (default_handler)
      default_handler((int)bounds.center().x, (int)bounds.center().y);
  }
};

struct ConnectionEntry: mforms::Accessible
{
  db_mgmt_ConnectionRef connection;

  std::string title;
  std::string description;
  std::string user;
  std::string schema;
  bool compute_strings; // True after creation to indicate the need to compute the final display strings.
  bool second_color;    // Cache flag for checkboard position, to ease creating a drag image.

  // For filtering we need the full strings.
  std::string search_title;
  std::string search_description;
  std::string search_user;
  std::string search_schema;

  boost::function <void (int, int)> default_handler;

  std::vector<ConnectionEntry> children;

  base::Rect bounds;

  // ------ Accesibility Methods -----

  virtual std::string get_acc_name()
  {
    if (children.size())
      return base::strfmt("%s %s", title.c_str(), _("Connection Group")); 
    else
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
};

typedef enum { DropBefore, DropOn, DropAfter } DropPosition;

class ConnectionsSection: public mforms::DrawBox, public mforms::DropDelegate
{
private:
  HomeScreen *_owner;

  cairo_surface_t* _folder_icon;
  cairo_surface_t* _mouse_over_icon;
  cairo_surface_t* _mouse_over2_icon;
  cairo_surface_t* _network_icon;
  cairo_surface_t* _ha_filter_icon;
  cairo_surface_t* _page_down_icon;
  cairo_surface_t* _page_up_icon;
  cairo_surface_t* _plus_icon;
  cairo_surface_t* _sakila_icon;
  cairo_surface_t* _fabric_icon;
  cairo_surface_t* _schema_icon;
  cairo_surface_t* _user_icon;
  cairo_surface_t* _manage_icon;

  base::Color _tile_bk_color1;
  base::Color _tile_bk_color2;
  base::Color _fabric_tile_bk_color;
  base::Color _folder_tile_bk_color;
  base::Color _back_tile_bk_color;

  base::Color _tile_bk_color1_hl;
  base::Color _tile_bk_color2_hl;
  base::Color _folder_tile_bk_color_hl;
  base::Color _fabric_tile_bk_color_hl;
  base::Color _back_tile_bk_color_hl;

  ssize_t _page_start;        // Index into the list where root display starts.
  ssize_t _page_start_backup; // Copy of the current page start when we go into a folder (for restauration).
  ssize_t _active_folder;     // The index of the folder entry that is currently active.
  ssize_t _tiles_per_page;
  ssize_t _fabric_entry;

  typedef std::vector<ConnectionEntry> ConnectionVector;
  typedef ConnectionVector::iterator ConnectionIterator;
  ConnectionVector _connections;
  ConnectionVector _filtered_connections;
  bool _filtered;

  mforms::Menu *_connection_context_menu;
  mforms::Menu *_fabric_context_menu;
  mforms::Menu *_folder_context_menu;
  mforms::Menu *_generic_context_menu;

  ssize_t _hot_entry;      // The index of the connection entry under the mouse.
  ssize_t _entry_for_menu; // The index that was hot when the context menu was opened.
  bool _show_details;      // If there's a hot connection this indicates if we just show the hot state or the connection details.

  ssize_t _drag_index;     // The index of the entry that is being dragged.
  ssize_t _drop_index;     // The index of the entry that is currently the drop target.
  DropPosition _drop_position;

  HomeAccessibleButton _add_button;
  HomeAccessibleButton _manage_button;
  HomeAccessibleButton _page_up_button;
  HomeAccessibleButton _page_down_button;

  base::Rect _info_button_rect;

  ConnectionInfoPopup *_info_popup;

  boost::function <bool (int, int)> _accessible_click_handler;

  mforms::Box _search_box;
  mforms::TextEntry _search_text;

  base::Rect _mouse_down_position; // Used to determine if the user starts a drag/drop operation.

public:

  ConnectionsSection(HomeScreen *owner)
  : _search_box(true), _search_text(mforms::SmallSearchEntry)
  {
    _owner = owner;
    _page_start = 0;
    _connection_context_menu = NULL;
    _folder_context_menu = NULL;
    _fabric_context_menu = NULL;
    _generic_context_menu = NULL;
    _hot_entry = -1;
    _entry_for_menu = -1;
    _show_details = false;
    _drag_index = -1;
    _drop_index = -1;
    _active_folder = -1;
    _filtered = false;
    _fabric_entry = -1;


    std::vector<std::string> formats;
    formats.push_back(TILE_DRAG_FORMAT);           // We allow dragging tiles to reorder them.
    formats.push_back(mforms::DragFormatFileName); // We accept sql script files to open them.
    register_drop_formats(this, formats);

    _folder_icon = mforms::Utilities::load_icon("wb_tile_folder.png");
    _mouse_over_icon = mforms::Utilities::load_icon("wb_tile_mouseover.png");
    _mouse_over2_icon = mforms::Utilities::load_icon("wb_tile_mouseover_2.png");
    _network_icon = mforms::Utilities::load_icon("wb_tile_network.png");
    // TODO: We need an tile icon for the group filter
    _ha_filter_icon = mforms::Utilities::load_icon("wb_tile_network.png");
    _page_down_icon = mforms::Utilities::load_icon("wb_tile_page-down.png");
    _page_up_icon = mforms::Utilities::load_icon("wb_tile_page-up.png");
    _plus_icon = mforms::Utilities::load_icon("wb_tile_plus.png");
    _sakila_icon = mforms::Utilities::load_icon("wb_tile_sakila.png");
    _fabric_icon = mforms::Utilities::load_icon("wb_tile_fabric.png");
    _schema_icon = mforms::Utilities::load_icon("wb_tile_schema.png");
    _user_icon = mforms::Utilities::load_icon("wb_tile_user.png");
    _manage_icon = mforms::Utilities::load_icon("wb_tile_manage.png");

    _info_popup = NULL;

    _search_box.set_name("search-box");
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

    _search_text.set_placeholder_text("Filter connections");
    _search_box.add(&_search_text, true, true);
    scoped_connect(_search_text.signal_changed(), boost::bind(&ConnectionsSection::on_search_text_changed, this));
    scoped_connect(_search_text.signal_action(), boost::bind(&ConnectionsSection::on_search_text_action, this, _1));
    add(&_search_box, mforms::TopRight);
    set_padding(0, 30, CONNECTIONS_RIGHT_PADDING, 0);

    _accessible_click_handler = boost::bind(&ConnectionsSection::mouse_click, this,
      mforms::MouseButtonLeft, _1, _2);

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

  ~ConnectionsSection()
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

  void update_colors()
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
    _fabric_tile_bk_color = base::Color::parse("#0e0e0e");
    _fabric_tile_bk_color_hl = base::Color::parse("#2f2f2f");
#else
    _folder_tile_bk_color = base::Color::parse("#178ec5");
    _folder_tile_bk_color_hl = base::Color::parse("#63a6c5");
    _fabric_tile_bk_color = base::Color::parse("#444444");
    _fabric_tile_bk_color_hl = base::Color::parse("#535353");
#endif

    _back_tile_bk_color = base::Color::parse("#d9532c");
    _back_tile_bk_color_hl = base::Color::parse("#d97457");

    bool high_contrast = base::Color::is_high_contrast_scheme();
    _search_text.set_front_color(high_contrast ? "#000000" : "#FFFFFF");
    _search_text.set_placeholder_color(high_contrast ? "#303030" : "#A0A0A0");
    _search_text.set_back_color(high_contrast ? "#ffffff" : "#474747");
  }

  //------------------------------------------------------------------------------------------------

  void focus_search_box()
  {
    _search_text.focus();
  }

  //------------------------------------------------------------------------------------------------

  bool is_hot_connection_folder()
  // This method will return true for both fabric and folder connections
  // This validation needs to be done outside
  {
    bool is_folder;

    if (_filtered)
      is_folder = _filtered_connections[_hot_entry].children.size() > 0;
    else if (_active_folder > -1)
      is_folder = _connections[_active_folder].children[_hot_entry].children.size() > 1;
    else
      is_folder = _connections[_hot_entry].children.size() > 1;

    return is_folder;
  }

  //------------------------------------------------------------------------------------------------

  bool is_hot_connection_fabric()
  {
    bool is_fabric;
    if (_filtered)
      is_fabric = _filtered_connections[_hot_entry].connection.is_valid() && _filtered_connections[_hot_entry].connection->driver()->name() == "MySQLFabric";
    else
      is_fabric = _connections[_hot_entry].connection.is_valid() && _connections[_hot_entry].connection->driver()->name() == "MySQLFabric";

    return is_fabric;
  }

  //------------------------------------------------------------------------------------------------

  void on_search_text_changed()
  {
    std::string filter = _search_text.get_string_value();
    _filtered_connections.clear();

    _filtered = !filter.empty();
    if (_filtered)
    {
      ConnectionVector current_connections = (_active_folder < 0) ? _connections : _connections[_active_folder].children;
      for (ConnectionIterator iterator = current_connections.begin(); iterator != current_connections.end(); ++iterator)
      {
        // Always keep the first entry if we are in a folder. It's not filtered.
        if (_active_folder >= 0 && iterator == current_connections.begin())
          _filtered_connections.push_back(*iterator);
        else
          if (base::contains_string(iterator->search_title, filter, false) ||
              base::contains_string(iterator->search_description, filter, false) ||
              base::contains_string(iterator->search_user, filter, false) ||
              base::contains_string(iterator->search_schema, filter, false))
            _filtered_connections.push_back(*iterator);
      }
    }
    set_needs_repaint();
  }

  //------------------------------------------------------------------------------------------------

  void on_search_text_action(mforms::TextEntryAction action)
  {
    if (action == mforms::EntryActivate)
    {
      if (_active_folder > -1)
      {
        // Within a folder.
        switch (_filtered_connections.size())
        {
        case 1: // Just the back tile. Return to top level.
          _page_start = _page_start_backup;
          _active_folder = -1;
          _filtered = false;
          _search_text.set_value("");
          set_needs_repaint();
          break;

        case 2: // Exactly one entry matched the filter. Activate it.
          _owner->trigger_callback(ActionOpenConnectionFromList, _filtered_connections[1].connection);
          break;
        }
      }
      else
      {
        if (!_filtered_connections.empty())
        {
          // If only one entry is visible through filtering activate it. I.e. for a group show its content
          // and for a connection open it.
          if (_filtered_connections[0].children.size() > 1)
          {
            _page_start_backup = _page_start;
            _page_start = 0;

            // Loop through the unfiltered list to find the index of the group we are about to open.
            _active_folder = -1; // Just a defensive action. Should never play a role.
            for (size_t i = 0; i < _connections.size(); ++i)
            {
              if (_connections[i].title == _filtered_connections[0].title)
              {
                _active_folder = (int)i;
                break;
              }
            }
            _filtered = false;
            _search_text.set_value("");
            set_needs_repaint();
          }
          else
            _owner->trigger_callback(ActionOpenConnectionFromList, _filtered_connections[0].connection);
        }
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Computes the index for the given position, regardless if that is actually backed by an existing
   * entry or not.
   */
  ssize_t calculate_index_from_point(int x, int y)
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

  ssize_t entry_from_point(int x, int y, bool &in_details_area)
  {
    in_details_area = false;

    ssize_t index = calculate_index_from_point(x, y);
    ssize_t count = _connections.size();
    if (_filtered)
      count = _filtered_connections.size(); // For both, main list or folder.
    else
      if (_active_folder > -1)
        count = _connections[_active_folder].children.size();
    if (index < count)
    {
      x -= CONNECTIONS_LEFT_PADDING;
      in_details_area = (x % (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING)) > 3 * CONNECTIONS_TILE_WIDTH / 4.0;
      return index;
    }

    return -1;
  }

  //------------------------------------------------------------------------------------------------

  base::Rect bounds_for_entry(int index)
  {
    base::Rect result(CONNECTIONS_LEFT_PADDING, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
    int tiles_per_row = (get_width() - CONNECTIONS_LEFT_PADDING - CONNECTIONS_RIGHT_PADDING) / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

    int column = index % tiles_per_row;
    int row = index / tiles_per_row;
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
  db_mgmt_ConnectionRef connection_from_index(ssize_t index)
  {
    if (index < 0 || (_active_folder > -1 && index == 0))
      return db_mgmt_ConnectionRef();

    if (_filtered)
    {
      if (_filtered_connections[index].children.size() > 1)
        return db_mgmt_ConnectionRef();
      return _filtered_connections[index].connection;
    }
    else
    {
      if (_active_folder > -1)
      {
        if (_connections[_active_folder].children[index].children.size() > 1)
          return db_mgmt_ConnectionRef();
        return _connections[_active_folder].children[index].connection;
      }
      else
      {
        if (_connections[index].children.size() > 1)
          return db_mgmt_ConnectionRef();
        return _connections[index].connection;
      }
    }
  }

  //------------------------------------------------------------------------------------------------

  bool is_group(int index)
  {
    if (index < 0)
      return false;

    if (_filtered)
      return _filtered_connections[index].children.size() > 1;
    else
    {
      if (_active_folder > -1)
        return (index == 0) || _connections[_active_folder].children[index].children.size() > 1;
      else
        return _connections[index].children.size() > 1;
    }
  }
  
  //------------------------------------------------------------------------------------------------

  /**
   * Draws and icon followed by the given text. The given position is that of the upper left corner
   * of the image.
   */
  void draw_icon_with_text(cairo_t *cr, double x, double y, cairo_surface_t *icon,
    const std::string &text, double alpha, bool high_contrast)
  {
    cairo_set_source_surface(cr, icon, floor(x), floor(y));
    cairo_paint_with_alpha(cr, alpha);

    x += image_width(icon) + 3;

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text.c_str(), &extents);

    double component = 0xF9 / 255.0;
    if (high_contrast)
      component = 1;
#ifdef __APPLE__
    cairo_set_source_rgba(cr, component, component, component, 0.6 * alpha);
#else
    cairo_set_source_rgba(cr, component, component, component, alpha);
#endif

    cairo_move_to(cr, x, (int)(y + image_height(icon) / 2.0 + extents.height / 2.0));
    cairo_show_text(cr, text.c_str());
    cairo_stroke(cr);
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Separate tile drawing for the special back tile (to return from a folder).
   */
  void draw_back_tile(cairo_t *cr, ConnectionEntry &entry, bool hot)
  {
    base::Color current_color = hot ? _back_tile_bk_color_hl : _back_tile_bk_color;
    cairo_fill(cr);

#ifdef __APPLE__
    cairo_new_sub_path(cr);

    double radius = 8;
    double degrees = M_PI / 180.0;
    entry.bounds.use_inter_pixel = false;
    cairo_arc(cr, entry.bounds.left() + entry.bounds.width() - radius, entry.bounds.top() + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, entry.bounds.left() + entry.bounds.width() - radius, entry.bounds.top() + entry.bounds.height() - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, entry.bounds.left() + radius, entry.bounds.top() + entry.bounds.height() - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, entry.bounds.left() + radius, entry.bounds.top() + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, current_color.red, current_color.green, current_color.blue, 1);
    cairo_fill(cr);

    // Border.
    entry.bounds.use_inter_pixel = true;
    cairo_arc(cr, -2 + entry.bounds.right() - radius, 1 + entry.bounds.top() + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, -2 + entry.bounds.right() - radius, -2 + entry.bounds.bottom() - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(cr, 1 + entry.bounds.left() + radius, -2 + entry.bounds.bottom() - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(cr, 1 + entry.bounds.left() + radius, 1 + entry.bounds.top() + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.19);
    cairo_set_line_width(cr, 3);
    cairo_stroke(cr);
#else
    entry.bounds.use_inter_pixel = false;
    cairo_rectangle(cr, entry.bounds.left(), entry.bounds.top(), entry.bounds.width(), entry.bounds.height());
    cairo_set_source_rgb(cr, current_color.red, current_color.green, current_color.blue);
    cairo_fill(cr);

    // Border.
    entry.bounds.use_inter_pixel = true;
    cairo_rectangle(cr, entry.bounds.left(), entry.bounds.top(), entry.bounds.width() - 1, entry.bounds.height() - 1);
    cairo_set_source_rgba(cr, 1, 1, 1, 0.125);
    cairo_set_line_width(cr, 1);
    cairo_stroke(cr);
#endif

    // Title string.
    double x = entry.bounds.left() + 10;
    double y = entry.bounds.top() + 27;
    cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);
    cairo_set_source_rgb(cr, 0xF9 / 255.0, 0xF9 / 255.0, 0xF9 / 255.0);

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, _("< back"));
    cairo_stroke(cr);
  }

  //------------------------------------------------------------------------------------------------

  void draw_tile(cairo_t *cr, ConnectionEntry &entry, bool hot, double alpha, bool for_dragging, 
    bool high_contrast)
  {
    bool is_fabric = entry.connection.is_valid() && entry.connection->driver()->name() == "MySQLFabric";
    base::Color current_color;
    if (is_fabric)
      current_color = hot ? _fabric_tile_bk_color_hl : _fabric_tile_bk_color;
    else if (entry.children.size() > 0)
      current_color = hot ? _folder_tile_bk_color_hl : _folder_tile_bk_color;
    else
    {
#ifndef __APPLE__
      if (entry.second_color)
        current_color = hot ? _tile_bk_color2_hl : _tile_bk_color2;
      else
#endif
        // No checker board for Mac.
        current_color = hot ? _tile_bk_color1_hl : _tile_bk_color1;
    }

    base::Rect bounds = entry.bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

#ifdef __APPLE__
    cairo_new_sub_path(cr);

    double radius = 8;
    double degrees = M_PI / 180.0;

    bounds.use_inter_pixel = false;
    cairo_arc(cr, bounds.left() + bounds.width() - radius, bounds.top() + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(cr, bounds.left() + bounds.width() - radius, bounds.top() + entry.bounds.height() - radius, radius, 0 * degrees, 90 * degrees);
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
    cairo_surface_t *back_icon = is_fabric ? _fabric_icon : entry.children.size() ? _folder_icon : _sakila_icon;
    
    double x = bounds.left() + bounds.width() - image_width(back_icon);
    double y = bounds.top() + bounds.height() - image_height(back_icon);
    cairo_set_source_surface(cr, back_icon, x, y);
    cairo_paint_with_alpha(cr, image_alpha * alpha);

    double component = 0xF9 / 255.0;
    if (high_contrast)
      component = 1;
    cairo_set_source_rgba(cr, component, component, component, alpha);

    if (hot && _show_details && (entry.children.size() == 0 || is_fabric))
    {
#ifdef __APPLE__
      // On OS X we show the usual italic small i letter instead of the peeling corner.
      cairo_select_font_face(cr, HOME_INFO_FONT, CAIRO_FONT_SLANT_ITALIC, CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);

      _info_button_rect = base::Rect(bounds.right() - 15, bounds.bottom() - 10, 10, 10);
      cairo_move_to(cr, _info_button_rect.left(), _info_button_rect.top());
      cairo_show_text(cr, "i");
      cairo_stroke(cr);

#else
      cairo_surface_t *overlay = _mouse_over_icon;
      x = bounds.left() + bounds.width() - image_width(overlay);
      cairo_set_source_surface(cr, overlay, x, bounds.top());
      cairo_paint_with_alpha(cr, alpha);

      cairo_set_source_rgba(cr, component, component, component, alpha);
#endif
    }

    cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_TILES_TITLE_FONT_SIZE);

    // Title string.
    x = (int)bounds.left() + 10.5; // Left offset from the border to caption, user and network icon.
    y = bounds.top() + 27; // Distance from top to the caption base line.

    if (entry.compute_strings)
    {
      // On first render compute the actual string to show. We only need to do this once
      // as neither the available space changes nor is the entry manipulated.
      double available_width = bounds.width() - 21;
      entry.title = mforms::Utilities::shorten_string(cr, entry.title, available_width);
    }

    cairo_move_to(cr, x, y);
    cairo_show_text(cr, entry.title.c_str());
    cairo_stroke(cr);

    cairo_set_font_size(cr, HOME_SMALL_INFO_FONT_SIZE);
    if (entry.children.size() > 0 && !is_fabric)
    {
#ifdef __APPLE__
      cairo_set_source_rgba(cr, component, component, component, 0.6 * alpha);
#endif
      
      std::string info = base::to_string(entry.children.size() - 1) + " " + _("Connections");
      y = bounds.top() + 55;
      cairo_move_to(cr, x, y);
      cairo_show_text(cr, info.c_str());
      cairo_stroke(cr);
    }
    else
    {
      if (entry.compute_strings)
      {
        double available_width = bounds.width() - 24 - image_width(_network_icon);
        entry.description = mforms::Utilities::shorten_string(cr, entry.description, available_width);

        available_width = bounds.center().x - x - image_width(_user_icon) - 6; // -6 is the spacing between text and icons.
        entry.user = mforms::Utilities::shorten_string(cr, entry.user, available_width);

        entry.schema = mforms::Utilities::shorten_string(cr, entry.schema, available_width);
      }

      y = bounds.top() + 56 - image_height(_user_icon);
      draw_icon_with_text(cr, x, y, _user_icon, entry.user, alpha, high_contrast);

      y = bounds.top() + 74 - image_height(_network_icon);
      draw_icon_with_text(cr, x, y, _network_icon, entry.description, alpha, high_contrast);

      if (is_fabric)
      {
        std::string ha_filter = base::strip_text(entry.connection->parameterValues().get("haGroupFilter").repr());

        std::string text(_("All Groups"));
        if (ha_filter.length())
        {
          std::vector<std::string> groups = base::split(ha_filter, ",");

          // Creates the legend to be displayed on the filter icon
          if (groups.size() > 2)
            text = base::strfmt("%s and %d others", groups[0].c_str(), groups.size() - 1);
          else
            text = ha_filter;
        }

        y = bounds.top() + 56 - image_height(_schema_icon);
        draw_icon_with_text(cr, bounds.center().x, y, _ha_filter_icon, text, alpha, high_contrast);
      }
      else
      {
        y = bounds.top() + 56 - image_height(_schema_icon);
        draw_icon_with_text(cr, bounds.center().x, y, _schema_icon,
          entry.schema.empty() ? _("n/a") : entry.schema, alpha, high_contrast);
      }
    }

    entry.compute_strings = false;
  }

  //------------------------------------------------------------------------------------------------

  void draw_paging_part(cairo_t *cr, int current_page, int pages, bool high_contrast)
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

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
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
    if (_active_folder > -1)
    {
      title += " / " + _connections[_active_folder].title;
      connections = &_connections[_active_folder].children;
    }
    else
      connections = &_connections;

    if (_filtered)
      connections = &_filtered_connections;

    cairo_show_text(cr, title.c_str());
    cairo_stroke(cr);

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
    base::Rect bounds(0, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
    bool done = false;
    while (!done)
    {
      bounds.pos.x = CONNECTIONS_LEFT_PADDING;
      double alpha = bounds.bottom() > height ? 0.25 : 1;
      for (int column = 0; column < tiles_per_row; column++)
      {
        int index = (int)(_page_start + row * tiles_per_row + column);
        if (index >= (int)connections->size())
        {
          done = true;
          break;
        }
        else
        {
          // Updates the bounds on the tile
          (*connections)[index].bounds = bounds;
          bool draw_hot = (int)index == _hot_entry;

          int draw_position = (row % 2) + column;
          if (index == 0 && _active_folder > -1)
            draw_back_tile(cr, (*connections)[index], draw_hot);
          else
          {
            (*connections)[index].second_color = (draw_position % 2) != 0;
            draw_tile(cr, (*connections)[index], draw_hot, alpha, false, high_contrast);
          }

          // Draw drop indicator.
          if (index == _drop_index)
          {
            if (high_contrast)
              cairo_set_source_rgb(cr, 0, 0, 0);
            else
              cairo_set_source_rgb(cr, 1, 1, 1);

            if (_drop_position == DropOn)
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
              if (_drop_position == DropAfter)
                x = bounds.right() + 4.5;
              cairo_move_to(cr, x, bounds.top());
              cairo_line_to(cr, x, bounds.bottom());
              cairo_set_line_width(cr, 3);
              cairo_stroke(cr);
              cairo_set_line_width(cr, 1);
            }
          }
        }
        bounds.pos.x += CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING;
      }

      row++;
      bounds.pos.y += CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING;
      if (bounds.top() >= height)
        done = true;
    }

    // See if we need to draw the paging indicator.
    height -= CONNECTIONS_TOP_PADDING;
    int rows_per_page = height / (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING);
    if (rows_per_page < 1)
      rows_per_page = 1;
    int rows = (int)ceil(connections->size() / (float)tiles_per_row);
    _tiles_per_page = tiles_per_row * rows_per_page;
    int pages = (int)ceil(rows / (float)rows_per_page);
    if (pages > 1)
    {
      int current_row = (int)ceil(_page_start / (float)tiles_per_row);
      int current_page = (int)ceil(current_row / (float)rows_per_page);
      draw_paging_part(cr, current_page, pages, high_contrast);
    }
    else
    {
      _page_up_button.bounds = base::Rect();
      _page_down_button.bounds = base::Rect();
      _page_start = 0; // Size increased to cover the full content.
    }
  }

  //------------------------------------------------------------------------------------------------

  void add_connection(const db_mgmt_ConnectionRef &connection, const std::string &title,
    const std::string &description, const std::string &user, const std::string &schema)
  {
    ConnectionEntry entry;
    
    entry.connection = connection;
    entry.title = title;
    entry.description = description;
    entry.user = user;
    entry.schema = schema;
    entry.compute_strings = true;
    entry.second_color = false;

    entry.search_title = title;
    entry.search_description = description;
    entry.search_user = user;
    entry.search_schema = schema;

    entry.default_handler = boost::bind(&ConnectionsSection::mouse_click, this,
      mforms::MouseButtonLeft, _1, _2);

    std::string::size_type slash_position = title.find("/");
    if (slash_position != std::string::npos)
    {
      // A child entry.
      std::string parent_name = title.substr(0, slash_position);
      entry.title = title.substr(slash_position + 1);
      entry.search_title = entry.title;
      bool found_parent = false;
      for (ConnectionIterator iterator = _connections.begin(); iterator != _connections.end(); iterator++)
      {
        if (iterator->title == parent_name && (iterator->children.size() > 0 || iterator->connection->driver()->name() == "MySQLFabric" ))
        {
          found_parent = true;
          iterator->children.push_back(entry);
          break;
        }
      }

      if (!found_parent)
      {
        ConnectionEntry parent;
        parent.connection = db_mgmt_ConnectionRef();
        parent.title = parent_name;
        parent.compute_strings = true;
        parent.second_color = false;
        parent.search_title = parent_name;

        ConnectionEntry back_entry;
        back_entry.connection = db_mgmt_ConnectionRef();
        back_entry.title = "< back";

        parent.children.push_back(back_entry);
        parent.children.push_back(entry);
        _connections.push_back(parent);
      }
    }
    else
      _connections.push_back(entry);
    set_layout_dirty(true);
  }

  //------------------------------------------------------------------------------------------------

  void clear_connections()
  {
    _entry_for_menu = -1;
    _active_folder = -1;
    _connections.clear();
    _filtered = false;
    _filtered_connections.clear();
    _search_text.set_value("");

    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_down(mforms::MouseButton button, int x, int y)
  {
    if (button == mforms::MouseButtonLeft && _hot_entry > -1)
      _mouse_down_position = base::Rect(x - 4, y - 4, 8, 8); // Center a 8x8 pixels rect around the mouse position.
    return false; // Continue with standard mouse handling.
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_up(mforms::MouseButton button, int x, int y)
  {
    _mouse_down_position = base::Rect();
    return false;
  }

  //--------------------------------------------------------------------------------------------------
  
  virtual bool mouse_double_click(mforms::MouseButton button, int x, int y)
  {
    switch (button)
    {
      case mforms::MouseButtonLeft:
      {
        // In order to allow quick clicking for page flipping we also handle double clicks for this.
        if (_page_up_button.bounds.contains(x, y))
        {
          _page_start -= _tiles_per_page;
          if (_page_start < 0)
            _page_start = 0;
          set_needs_repaint();
          return true;
        }

        if (_page_down_button.bounds.contains(x, y))
        {
          _page_start += _tiles_per_page;
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

  virtual bool mouse_click(mforms::MouseButton button, int x, int y)
  {
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
            // Page up clicked. Doesn't happen if we are on the first page already.
            _page_start -= _tiles_per_page;
            if (_page_start < 0)
              _page_start = 0;
            set_needs_repaint();
            return true;
          }

          if (_page_down_button.bounds.contains(x, y))
          {
            _page_start += _tiles_per_page;
            set_needs_repaint();
            return true;
          }

          if (_hot_entry > -1)
          {
            if (_active_folder > -1 && _hot_entry == 0)
            {
              if (_fabric_entry > -1)
                _fabric_entry = -1;
              
              // Returning to root list.
              _page_start = _page_start_backup;
              _active_folder = -1;
              _filtered = false;
              _search_text.set_value("");
              
              set_needs_repaint();
              return true;
            }

            bool is_fabric = is_hot_connection_fabric();
            bool is_folder = !is_fabric && is_hot_connection_folder();

#ifdef __APPLE__
            bool show_info = _info_button_rect.contains_flipped(x, y);
#else
            bool show_info = _show_details;
#endif

            if (show_info && (!is_folder || is_fabric))
            {
              show_info_popup();
              return true;
            }
            
            if (is_fabric)
            {
              // Creates the fabric connections only if they have not been already created
              // since the last connection refresh
              int created_connections = grt::IntegerRef::cast_from(_connections[_hot_entry].connection->parameterValues().get("connections_created"));
              if (!created_connections)
              {
                grt::GRT *grt = _connections[_hot_entry].connection->get_grt();
                grt::BaseListRef args(grt);
                args->insert_unchecked(_connections[_hot_entry].connection);

                grt::ValueRef result = grt->call_module_function("WBFabric", "create_connections", args);
                std::string error = grt::StringRef::extract_from(result);

                if (error.length())
                {
                  mforms::Utilities::show_error("MySQL Fabric Connection Error", error, "OK");
                  return true;
                }
                else
                  // Sets the flag to indicate the connections have been crated for this fabric node
                  _connections[_hot_entry].connection->parameterValues().set("connections_created", grt::IntegerRef(1));
              }
              
              _fabric_entry = _hot_entry;
            }

            if (is_folder || is_fabric)
            {
              // Drilling into a folder.
              _page_start_backup = _page_start;
              _page_start = 0;
              _active_folder = _hot_entry;
              _filtered = false;
              _search_text.set_value("");
              set_needs_repaint();
              return true;
            }

            // Anything else.
            if (_filtered)
              _owner->trigger_callback(ActionOpenConnectionFromList,
                                       _filtered_connections[_hot_entry].connection
                                       );
            else
              _owner->trigger_callback(ActionOpenConnectionFromList,
                                       (_active_folder == -1) ? _connections[_hot_entry].connection :
                                       _connections[_active_folder].children[_hot_entry].connection
                                       );
            return true;
          }
        }
        break;

      case mforms::MouseButtonRight:
      {
        mforms::Menu *context_menu = NULL;

        if (_hot_entry > -1)
        {
          if (_active_folder > -1)
          {
            // There can't be any folder, as we don't support folder nesting.
            if (_hot_entry > 0)
              context_menu = _connection_context_menu;
          }
          else
          {
            if (is_hot_connection_fabric())
              context_menu = _fabric_context_menu;
            else if (is_hot_connection_folder())
              context_menu = _folder_context_menu;
            else
              context_menu = _connection_context_menu;
          }

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

  bool mouse_leave()
  {
    // Ignore mouse leaves if we are showing the info popup. We want the entry to stay hot.
    if (_info_popup != NULL)
      return true;

    if (_hot_entry > -1)
    {
      _hot_entry = -1;
      _show_details = false;
      set_needs_repaint();
    }
    return false;
  }

  //------------------------------------------------------------------------------------------------

  virtual bool mouse_move(mforms::MouseButton button, int x, int y)
  {
    bool in_details_area;
    ssize_t entry = entry_from_point(x, y, in_details_area);

    if (entry > -1 && !_mouse_down_position.empty() && (!_mouse_down_position.contains(x, y)))
    {
      if (entry == 0 && _active_folder > -1) // Back tile. Cancel drag operation.
      {
        _mouse_down_position = base::Rect();
        return true;
      }

      if (button == mforms::MouseButtonNone) // Cancel drag if the mouse button was released.
        return true;

      return do_tile_drag((int)entry, x, y);
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
          if (_hot_entry > - 1)
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

  void set_context_menu(mforms::Menu *menu, HomeScreenMenuType type)
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
          menu->set_handler(boost::bind(&ConnectionsSection::handle_folder_command, this, _1));
        }
        break;

      case HomeMenuConnectionFabric:
        if (_fabric_context_menu != NULL)
          _fabric_context_menu->release();
        _fabric_context_menu = menu;
        if (_fabric_context_menu != NULL)
        {
          _fabric_context_menu->retain();
          menu->set_handler(boost::bind(&ConnectionsSection::handle_command, this, _1));
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

  void handle_command(const std::string &command)
  {
    grt::ValueRef item;
    if (_entry_for_menu > -1)
    {
      if (_active_folder > -1)
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
          if (_filtered)
            item = _filtered_connections[_entry_for_menu].connection;
          else
            item = _connections[_active_folder].children[_entry_for_menu].connection;
        }
      }
      else
      {
        if (_filtered)
          item = _filtered_connections[_entry_for_menu].connection;
        else
          item = _connections[_entry_for_menu].connection;
      }
    }
    
    _owner->handle_context_menu(item, command);
    _entry_for_menu = -1;
  }

  //------------------------------------------------------------------------------------------------

  void handle_folder_command(const std::string &command)
  {
    grt::ValueRef item;

    // We have to pass on a valid connection (for the group name).
    // All child items have the same group name (except the dummy entry for the back tile).
    if (_filtered)
      item = grt::StringRef(_filtered_connections[_entry_for_menu].title);
    else
      item = grt::StringRef(_connections[_entry_for_menu].title);
    _owner->handle_context_menu(item, command);
    _entry_for_menu = -1;
  }

  //------------------------------------------------------------------------------------------------

  void menu_open()
  {
    ssize_t first_index = _active_folder > -1 ? 1 : 0;
    ssize_t last_index;

    if (_filtered)
      last_index = _filtered_connections.size() - 1;
    else
      last_index = _active_folder > -1 ? _connections[_active_folder].children.size() - 1 : _connections.size() - 1;
    if (_connection_context_menu != NULL)
    {
      _connection_context_menu->set_item_enabled(_connection_context_menu->get_item_index("move_connection_to_top"), _entry_for_menu > first_index);
      _connection_context_menu->set_item_enabled(_connection_context_menu->get_item_index("move_connection_up"), _entry_for_menu > first_index);
      _connection_context_menu->set_item_enabled(_connection_context_menu->get_item_index("move_connection_down"), _entry_for_menu < last_index);
      _connection_context_menu->set_item_enabled(_connection_context_menu->get_item_index("move_connection_to_end"), _entry_for_menu < last_index);
    }
    if (_folder_context_menu != NULL)
    {
      _folder_context_menu->set_item_enabled(_folder_context_menu->get_item_index("move_connection_to_top"), _entry_for_menu > first_index);
      _folder_context_menu->set_item_enabled(_folder_context_menu->get_item_index("move_connection_up"), _entry_for_menu > first_index);
      _folder_context_menu->set_item_enabled(_folder_context_menu->get_item_index("move_connection_down"), _entry_for_menu < last_index);
      _folder_context_menu->set_item_enabled(_folder_context_menu->get_item_index("move_connection_to_end"), _entry_for_menu < last_index);
    }
    if (_fabric_context_menu != NULL)
    {
      _fabric_context_menu->set_item_enabled(_fabric_context_menu->get_item_index("move_connection_to_top"), _entry_for_menu > first_index);
      _fabric_context_menu->set_item_enabled(_fabric_context_menu->get_item_index("move_connection_up"), _entry_for_menu > first_index);
      _fabric_context_menu->set_item_enabled(_fabric_context_menu->get_item_index("move_connection_down"), _entry_for_menu < last_index);
      _fabric_context_menu->set_item_enabled(_fabric_context_menu->get_item_index("move_connection_to_end"), _entry_for_menu < last_index);
    }
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Displays the info popup for the hot entry and enters a quasi-modal-state.
   */
  void show_info_popup()
  {
    if (_info_popup != NULL || _parent == NULL)
      return;

    // We have checked in the hit test already that we are on a valid connection object.
    std::pair<int, int> pos = _parent->client_to_screen(_parent->get_x(), _parent->get_y());

    // Stretch the popup window over all 3 sections, but keep the info area in our direct parent's bounds
    base::Rect host_bounds = base::Rect(pos.first, pos.second, _parent->get_parent()->get_width(), _parent->get_parent()->get_height());

    int width = get_width();
    width -= CONNECTIONS_LEFT_PADDING + CONNECTIONS_RIGHT_PADDING;
    int tiles_per_row = width / (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING);

    size_t top_entry = _hot_entry - _page_start;
    size_t row = top_entry / tiles_per_row;
    size_t column = top_entry % tiles_per_row;
    pos.first = (int)(CONNECTIONS_LEFT_PADDING + column * (CONNECTIONS_TILE_WIDTH + CONNECTIONS_SPACING));
    pos.second = (int)(CONNECTIONS_TOP_PADDING + row * (CONNECTIONS_TILE_HEIGHT + CONNECTIONS_SPACING));
    base::Rect item_bounds = base::Rect(pos.first, pos.second, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
    db_mgmt_ConnectionRef connection;

    if (_filtered)
      connection = _filtered_connections[_hot_entry].connection;
    else
      connection = _active_folder > -1 ?
        _connections[_active_folder].children[_hot_entry].connection :
        _connections[_hot_entry].connection;

    db_mgmt_ServerInstanceRef instance;
    grt::ListRef<db_mgmt_ServerInstance> instances = _owner->rdbms()->storedInstances();
    for (grt::ListRef<db_mgmt_ServerInstance>::const_iterator iterator = instances.begin();
      iterator != instances.end(); iterator++)
    {
      if ((*iterator)->connection() == connection)
      {
        instance = *iterator;
        break;
      }
    }

    int info_width =  _parent->get_width();
    if (info_width < 735)
      info_width = (int)host_bounds.width();
    _info_popup = mforms::manage(new ConnectionInfoPopup(_owner, connection, instance, host_bounds, item_bounds, info_width));
    scoped_connect(_info_popup->on_close(), boost::bind(&ConnectionsSection::popup_closed, this));
  }

  //------------------------------------------------------------------------------------------------

  void hide_info_popup()
  {
    if (_info_popup != NULL)
    {
      _hot_entry = -1;
      _show_details = false;

      _info_popup->release();
      _info_popup = NULL;

      set_needs_repaint();
    }
  }

  //------------------------------------------------------------------------------------------------

  void popup_closed()
  {
    hide_info_popup();
  }

  //------------------------------------------------------------------------------------------------

  void cancel_operation()
  {
    _owner->cancel_script_loading();
  }

  //------------------------------------------------------------------------------------------------
  
  virtual int get_acc_child_count()
  { 
    // At least 2 is returned because of the add and manage icons.
    int ret_val = 2;


    if (_filtered)
      ret_val += (int)_filtered_connections.size();
    else
      if (_active_folder == -1)
        ret_val += (int)_connections.size();
      else
      {
        // Adds one because of the back tile
        ret_val++;
        ret_val += (int)_connections[_active_folder].children.size();
      }

    // Adds 2 because of the pageup/pagedown icons if the
    // icons are being displayed.
    if (_page_up_button.bounds.width())
      ret_val += 2;

    return ret_val; 
  }

  virtual Accessible* get_acc_child(int index)
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
              accessible = &_filtered_connections[index];
            else
              index -= (int)_filtered_connections.size();
          }
          else
          {
            if (_active_folder == -1)
            {
              if (index < (int)_connections.size())
                accessible = &_connections[index];
              else
                index -= (int)_connections.size();
            }
            else
            {
              if (index < (int)_connections[_active_folder].children.size())
                accessible = &_connections[_active_folder].children[index];
              else
                index -= (int)_connections[_active_folder].children.size();
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
  
  virtual std::string get_acc_name()
  {
    return get_name();
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
    else if (_manage_button.bounds.contains(x, y))
      accessible = &_manage_button;
    else if (_page_up_button.bounds.contains(x, y))
      accessible = &_page_up_button;
    else if (_page_down_button.bounds.contains(x, y))
      accessible = &_page_down_button;
    else
    {
      bool in_details_area = false;
      ssize_t entry = entry_from_point(x, y, in_details_area);
    
      if (entry != -1)
      {
        if (_filtered)
          accessible = &_filtered_connections[entry];
        else
          if (_active_folder == -1)
            accessible = &_connections[entry];
          else
            accessible = &_connections[_active_folder].children[entry];
      }
    }

    return accessible;
  }

  //------------------------------------------------------------------------------------------------
  
  bool do_tile_drag(int index, int x, int y)
  {
    _hot_entry = -1;
    set_needs_repaint();
    
    mforms::DragDetails details;
    details.allowedOperations = mforms::DragOperationMove;
    details.location = base::Point(x, y);

    details.image = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);
    cairo_t *cr = cairo_create(details.image);
    base::Rect bounds = bounds_for_entry(index);
    details.hotspot.x = x - bounds.pos.x;
    details.hotspot.y = y - bounds.pos.y;

    // We know we have no back tile here.
    ConnectionEntry entry;

    if (_filtered)
      entry = _filtered_connections[index];
    else
      entry = _active_folder > -1 ?

    _connections[_active_folder].children[index] :
    _connections[index];

    draw_tile(cr, entry, false, 1, true, false); // There's no drag tile actually in high contrast mode.

    _drag_index = index;
    mforms::DragOperation operation = do_drag_drop(details, &entry, TILE_DRAG_FORMAT);
    _mouse_down_position = base::Rect();
    cairo_surface_destroy(details.image);
    cairo_destroy(cr);

    _drag_index = -1;
    _drop_index = -1;
    set_needs_repaint();

    if (operation == mforms::DragOperationMove) // The actual move is done in the drop delegate method.
      return true;

    return false;
  }

  //------------------------------------------------------------------------------------------------
  
  // Drop delegate implementation.
  mforms::DragOperation drag_over(View *sender, base::Point p, const std::vector<std::string> &formats)
  {
    if (std::find(formats.begin(), formats.end(), mforms::DragFormatFileName) != formats.end())
    {
      // Indicate we can accept files if one of the connection tiles is hit.
      bool in_details_area;
      ssize_t entry = entry_from_point((int)p.x, (int)p.y, in_details_area);

      if (entry == -1)
        return mforms::DragOperationNone;

      if (!connection_from_index(entry).is_valid())
        return mforms::DragOperationNone;

      if (_hot_entry != entry)
      {
        _hot_entry = entry;
        set_needs_repaint();
      }
      return mforms::DragOperationCopy;
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
      else
        if (_active_folder > -1)
          count = (int)_connections[_active_folder].children.size();

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

      DropPosition position = DropBefore;
      if (column == tiles_per_row)
        position = DropAfter;
      else
      {
        if (index >= count)
        {
          index = count - 1;
          position = DropAfter;
        }
        else
        {
          // Tile hit. Depending on which side of the tile's center the mouse is use a position
          // before or after that tile. Back tiles have no "before" position, but only "on" or "after".
          // Folder tiles have "before", "on" and "after" positions. Connection tiles only have "before"
          // and "after".
          base::Rect bounds = bounds_for_entry(index);
          if (is_group(index))
          {
            // In a group take the first third as hit area for "before", the second as "on" and the
            // last one as "after".
            if (p.x > bounds.left() + bounds.width() / 3)
            {
              if (p.x > bounds.right() - bounds.width() / 3)
                position = DropAfter;
              else
                position = DropOn;
            }
          }
          else
          {
            if (p.x > bounds.xcenter())
              position = DropAfter;
          }
        }
      }

      // Check that the drop position does not resolve to the dragged item.
      // Don't allow dragging a group on a group either.
      if (index == _drag_index ||
          (index + 1 == _drag_index && position == DropAfter) ||
          (index - 1 == _drag_index && position == DropBefore) ||
          (position == DropOn && is_group((int)_drag_index) && is_group(index)))
      {
        index = -1;
      }
      else
        if (!_filtered && _active_folder > -1 && index == 0 && position == DropBefore)
          position = DropOn; // Drop on back tile.

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

  mforms::DragOperation files_dropped(View *sender, base::Point p, const std::vector<std::string> &file_names)
  {
    bool in_details_area;
    ssize_t entry = entry_from_point((int)p.x, (int)p.y, in_details_area);
    if (entry == -1)
      return mforms::DragOperationNone;

    db_mgmt_ConnectionRef connection = connection_from_index(entry);
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

  mforms::DragOperation data_dropped(mforms::View *sender, base::Point p, void *data, const std::string &format)
  {
    if (format == TILE_DRAG_FORMAT && _drop_index > -1)
    {
      mforms::DragOperation result = mforms::DragOperationNone;

      // Can be invalid if we move a group.
      db_mgmt_ConnectionRef connection = connection_from_index(_drag_index);
      ConnectionEntry *source_entry = static_cast<ConnectionEntry*>(data);

      // Get a reference to the grt either from the first entry (if that is a connection tile)
      // or the first child entry (after the back tile) if it is a group.
      grt::GRT *grt;
      if (_connections[0].connection.is_valid())
        grt = _connections[0].connection->get_grt();
      else
        grt = _connections[0].children[1].connection->get_grt();

      ConnectionEntry entry;
      if (_filtered)
      {
        if (_drop_index < (int)_filtered_connections.size())
          entry = _filtered_connections[_drop_index];
      }
      else
        if (_active_folder > -1)
        {
          if (_drop_index < (int)_connections[_active_folder].children.size())
            entry = _connections[_active_folder].children[_drop_index];
        }
        else
        {
          if (_drop_index < (int)_connections.size())
            entry = _connections[_drop_index];
        }

      bool is_back_tile = entry.title == "< back";

      // Drop target is a group.
      grt::DictRef details(grt);
      if (connection.is_valid())
        details.set("object", connection);
      else
        details.set ("object", grt::StringRef(source_entry->title));

      // Because the connection changes will reload the entire list we try to restore
      // the last active folder and page position.
      ssize_t last_group = _active_folder;
      size_t last_page_start = _page_start;
      size_t last_page_start_backup = _page_start_backup;
      if (_drop_position == DropOn)
      {
        // Drop on a group (or back tile).
        if (is_back_tile)
          details.set("group", grt::StringRef("*Ungrouped*"));
        else
          details.set("group", grt::StringRef(entry.title));
        _owner->trigger_callback(ActionMoveConnectionToGroup, details);
      }
      else
      {
        // Drag from one position to another within a group (root or active group).
        size_t to = _drop_index;
        if (_active_folder > - 1)
          to--; // The back tile has no representation in the global list.
        if (_drop_position == DropAfter)
          to++;

        details.set("to", grt::IntegerRef((int)to));
        _owner->trigger_callback(ActionMoveConnection, details);
      }
      result = mforms::DragOperationMove;

      if (last_group > -1 && _connections[last_group].children.size() > 0)
        _active_folder = last_group;
      _page_start = last_page_start;
      _page_start_backup = last_page_start_backup;

      _drop_index = -1;
      set_needs_repaint();

      return result;
    }
    return mforms::DragOperationNone;
  }
  
};

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

  // ------ Accesibility Methods -----
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
  cairo_surface_t* _page_down_icon;
  cairo_surface_t* _page_up_icon;
  cairo_surface_t* _plus_icon;
  cairo_surface_t* _schema_icon;
  cairo_surface_t* _time_icon;
  cairo_surface_t* _folder_icon;
  cairo_surface_t* _size_icon;
  cairo_surface_t* _close_icon;
  cairo_surface_t* _open_icon;
  cairo_surface_t* _action_icon;

  ssize_t _page_start;
  ssize_t _entries_per_page;
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
  HomeAccessibleButton _page_up_button;
  HomeAccessibleButton _page_down_button;

  base::Rect _close_button_rect;
  base::Rect _use_default_button_rect;

  DisplayMode _hot_heading;
  base::Rect _model_heading_rect;
  base::Rect _sql_heading_rect;
  base::Rect _mixed_heading_rect;
public:

  DocumentsSection(HomeScreen *owner)
  {
    _owner = owner;
    _page_start = 0;
    _model_context_menu = NULL;
    _model_action_menu = NULL;
    _hot_entry = -1;
    _active_entry = -1;
    _display_mode = ModelsOnly;
    _hot_heading = Nothing;
    _entries_per_page = 0;
    _entries_per_row = 0;
    _show_selection_message = false;

    _page_down_icon = mforms::Utilities::load_icon("wb_tile_page-down.png");
    _page_up_icon = mforms::Utilities::load_icon("wb_tile_page-up.png");
    _plus_icon = mforms::Utilities::load_icon("wb_tile_plus.png");
    _model_icon = mforms::Utilities::load_icon("wb_doc_model.png");
    _sql_icon = mforms::Utilities::load_icon("wb_doc_sql.png");
    _schema_icon = mforms::Utilities::load_icon("wb_tile_schema.png");
    _time_icon = mforms::Utilities::load_icon("wb_tile_time.png");
    _folder_icon = mforms::Utilities::load_icon("wb_tile_folder_mini.png");
    _size_icon = mforms::Utilities::load_icon("wb_tile_number.png");
    _close_icon = mforms::Utilities::load_icon("wb_close.png");
    _open_icon = mforms::Utilities::load_icon("wb_tile_open.png");
    _action_icon = mforms::Utilities::load_icon("wb_tile_more.png");

    _add_button.name = "Add Model";
    _add_button.default_action = "Create New Model";
    _add_button.default_handler = _accessible_click_handler;

    _open_button.name = "Open Model";
    _open_button.default_action = "Open Existing Model";
    _open_button.default_handler = _accessible_click_handler;

    _action_button.name = "Create Model Options";
    _action_button.default_action = "Open Create Model Options Menu";
    _action_button.default_handler = _accessible_click_handler;

    _page_up_button.name = "Page Up";
    _page_up_button.default_action = "Move Model Pages Up";
    _page_up_button.default_handler = _accessible_click_handler;
    
    _page_down_button.name = "Page Down";
    _page_down_button.default_action = "Move Model Pages Down";
    _page_down_button.default_handler = _accessible_click_handler;
  }

  //------------------------------------------------------------------------------------------------

  ~DocumentsSection()
  {
    if (_model_context_menu != NULL)
      _model_context_menu->release();

    delete_surface(_page_down_icon);
    delete_surface(_page_up_icon);
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

#define DOCUMENTS_LEFT_PADDING     40
#define DOCUMENTS_RIGHT_PADDING    40
#define DOCUMENTS_TOP_PADDING      64
#define DOCUMENTS_VERTICAL_SPACING 26

#define DOCUMENTS_ENTRY_WIDTH     250 // No spacing horizontally.
#define DOCUMENTS_ENTRY_HEIGHT     60
#define DOCUMENTS_HEADING_SPACING  10 // Spacing between a heading part and a separator.
#define DOCUMENTS_TOP_BASELINE     40 // Vertical space from top border to title base line.

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
    size_t index = _page_start + row * _entries_per_row + column;
    if (index < count)
      return index;

    return -1;
  }

  //------------------------------------------------------------------------------------------------

  /**
   * Draws and icon followed by the given text. The given position is that of the upper left corner
   * of the image.
   */
  void draw_icon_with_text(cairo_t *cr, int x, int y, cairo_surface_t *icon,
    const std::string &text, bool high_contrast)
  {
    cairo_set_source_surface(cr, icon, x, y);
    cairo_paint(cr);

    x += image_width(icon) + 3;

    cairo_text_extents_t extents;
    cairo_text_extents(cr, text.c_str(), &extents);

    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgba(cr, 0xF9 / 255.0, 0xF9 / 255.0, 0xF9 / 255.0, 0.5);
    cairo_move_to(cr, x , (int)(y + image_height(icon) / 2.0 + extents.height / 2.0));
    cairo_show_text(cr, text.c_str());
    cairo_stroke(cr);
  }

  //------------------------------------------------------------------------------------------------

  void draw_paging_part(cairo_t *cr, int current_page, int pages, bool high_contrast)
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

  void draw_entry(cairo_t *cr, const DocumentEntry &entry, bool hot, bool high_contrast)
  {
    cairo_set_source_surface(cr, _model_icon, entry.bounds.left(), entry.bounds.top());
    cairo_paint(cr);

    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgb(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0);
    cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);
    int x = (int)(entry.bounds.left() + image_width(_model_icon) + 10);
    int y = (int)entry.bounds.top() + 18;
    if (hot)
    {
      double width = 0;
      cairo_text_extents_t extents;
      cairo_text_extents(cr, entry.title.c_str(), &extents);
      width = ceil(extents.width);

      cairo_save(cr);
      if (high_contrast)
        cairo_set_source_rgb(cr, 1, 1, 1);
      else
        cairo_set_source_rgb(cr, 0, 0, 0);
      text_with_decoration(cr, x-1, y, entry.title.c_str(), true, width);
      text_with_decoration(cr, x+1, y, entry.title.c_str(), true, width);
      text_with_decoration(cr, x, y-1, entry.title.c_str(), true, width);
      text_with_decoration(cr, x, y+1, entry.title.c_str(), true, width);
      cairo_restore(cr);

      text_with_decoration(cr, x, y, entry.title.c_str(), true, width);
    }
    else
      text_with_decoration(cr, x, y, entry.title_shorted.c_str(), false, 0);

    cairo_set_font_size(cr, HOME_SMALL_INFO_FONT_SIZE);

    draw_icon_with_text(cr, x, (int)entry.bounds.top() + 26, _folder_icon,
                        entry.folder_shorted, high_contrast);
    if (entry.is_model)
      draw_icon_with_text(cr, x, (int)entry.bounds.top() + 40, _schema_icon,
      entry.schemas.empty() ? "--" : entry.schemas_shorted, high_contrast);
    else
      draw_icon_with_text(cr, x, (int)entry.bounds.top() + 40, _size_icon,
        entry.size.empty() ? "--" : entry.size, high_contrast);
    draw_icon_with_text(cr, x, (int)entry.bounds.top() + 54, _time_icon, entry.last_accessed,
      high_contrast);
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

#define MESSAGE_WIDTH 200
#define MESSAGE_HEIGHT 75

  void draw_selection_message(cairo_t *cr, bool high_contrast)
  {
    // Attach the message to the current active entry as this is what is used when
    // a connection is opened.
    ssize_t column = (_active_entry - _page_start) % _entries_per_row;
    ssize_t row = (_active_entry - _page_start) / _entries_per_row;
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

    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgb(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0);
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

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
  {
    int width = get_width();
    int height = get_height();

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

    bool high_contrast = base::Color::is_high_contrast_scheme();
    // Heading for switching display mode. Draw heading hot only when we support more sections.
    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgba(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0, _display_mode == ModelsOnly ? 1 : 0.2);
    text_with_decoration(cr, _model_heading_rect.left(), _model_heading_rect.top(), _("Models"),
      false /*_hot_heading == ModelsOnly*/, _model_heading_rect.width());

    if (high_contrast)
      cairo_set_operator(cr, CAIRO_OPERATOR_XOR);

    cairo_set_source_surface(cr, _plus_icon, _add_button.bounds.left(), _add_button.bounds.top());
    cairo_paint(cr);

    cairo_set_source_surface(cr, _open_icon, _open_button.bounds.left(), _open_button.bounds.top());
    cairo_paint(cr);

    cairo_set_source_surface(cr, _action_icon, _action_button.bounds.left(), _action_button.bounds.top());
    cairo_paint(cr);

    /* Disabled for now.
    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgba(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0, _display_mode == ScriptsOnly ? 1 : 0.2);
    text_with_decoration(cr, _sql_heading_rect.left(), _sql_heading_rect.top(), _("SQL Scripts"),
      _hot_heading == ScriptsOnly, _sql_heading_rect.width());

    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgba(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0, _display_mode == Mixed ? 1 : 0.2);
    text_with_decoration(cr, _mixed_heading_rect.left(), _mixed_heading_rect.top(), _("Recent Documents"),
      _hot_heading == Mixed, _mixed_heading_rect.width());

    // Finally the separator lines. Text rects are flipped!
    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgba(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0, 0.2);
    cairo_move_to(cr, _sql_heading_rect.left() - DOCUMENTS_HEADING_SPACING + 0.5, _sql_heading_rect.top() - _sql_heading_rect.height() + 3);
    cairo_line_to(cr, _sql_heading_rect.left() - DOCUMENTS_HEADING_SPACING + 0.5, _sql_heading_rect.top() + 3);
    cairo_move_to(cr, _mixed_heading_rect.left() - DOCUMENTS_HEADING_SPACING + 0.5, _mixed_heading_rect.top() - _sql_heading_rect.height() + 3);
    cairo_line_to(cr, _mixed_heading_rect.left() - DOCUMENTS_HEADING_SPACING + 0.5, _mixed_heading_rect.top() + 3);
    cairo_stroke(cr);
*/
    
    if (high_contrast)
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
        size_t index = _page_start + row * entries_per_row + column;
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
            draw_entry(cr, _filtered_documents[index], (size_t)_hot_entry == index, high_contrast);
        }
        bounds.pos.x += DOCUMENTS_ENTRY_WIDTH;
      }
      if (draw_hot_entry)
        draw_entry(cr, _filtered_documents[_hot_entry], true, high_contrast);

      row++;
      bounds.pos.y += DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING;
      if (bounds.top() >= height)
        done = true;
    }

    // See if we need to draw the paging indicator.
    height -= DOCUMENTS_TOP_PADDING;
    int rows_per_page = height / (DOCUMENTS_ENTRY_HEIGHT + DOCUMENTS_VERTICAL_SPACING);
    if (rows_per_page < 1)
      rows_per_page = 1;
    int rows = (int)ceil(_filtered_documents.size() / (float)entries_per_row);
    _entries_per_page = entries_per_row * rows_per_page;
    int pages = (int)ceil(rows / (float)rows_per_page);
    if (pages > 1)
    {
      int current_row = (int)ceil(_page_start / (float)entries_per_row);
      int current_page = (int)ceil(current_row / (float)rows_per_page);
      draw_paging_part(cr, current_page, pages, high_contrast);
    }
    else
    {
      _page_up_button.bounds = base::Rect();
      _page_down_button.bounds = base::Rect();
      _page_start = 0; // Size increased to cover the full content.
    }

    if (_show_selection_message)
      draw_selection_message(cr, high_contrast);
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
    entry.is_model = base::tolower(base::extension(path)) == ".mwb";
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
            _page_start = 0;
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
            _page_start = 0;
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

        if (_page_up_button.bounds.contains(x, y))
        {
          // Page up clicked. Doesn't happen if we are on the first page already.
          _page_start -= _entries_per_page;
          if (_page_start < 0)
            _page_start = 0;
          set_needs_repaint();
          return true;
        }

        if (_page_down_button.bounds.contains(x, y))
        {
          _page_start += _entries_per_page;
          set_needs_repaint();
          return true;
        }

        if (_model_heading_rect.contains_flipped(x, y))
        {
          if (_display_mode != ModelsOnly)
          {
            _display_mode = ModelsOnly;
            _page_start = 0;
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
            _page_start = 0;
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
            _page_start = 0;
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
              _page_start = 0;
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
              _page_start = 0;
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

          if (_page_up_button.bounds.contains(x, y))
          {
            _owner->cancel_script_loading();

            // Page up clicked. Doesn't happen if we are on the first page already.
            _page_start -= _entries_per_page;
            if (_page_start < 0)
              _page_start = 0;
            set_needs_repaint();
            return true;
          }

          if (_page_down_button.bounds.contains(x, y))
          {
            _owner->cancel_script_loading();

            _page_start += _entries_per_page;
            set_needs_repaint();
            return true;
          }

          if (_model_heading_rect.contains_flipped(x, y))
          {
            _owner->cancel_script_loading();

            if (_display_mode != ModelsOnly)
            {
              _display_mode = ModelsOnly;
              _page_start = 0;
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
              _page_start = 0;
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
              _page_start = 0;
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

    // Adds a child for each paging icon if shown
    if (_page_up_button.bounds.width())
      ret_val += 2;

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
          else
          {
            index -= (int)_filtered_documents.size();
            accessible = index ? &_page_down_button : &_page_up_button;
          }
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
    else if (_page_up_button.bounds.contains(x, y))
      accessible = &_page_up_button;
    else if (_page_down_button.bounds.contains(x, y))
      accessible = &_page_down_button;
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

struct ShortcutEntry : mforms::Accessible
{
  app_StarterRef shortcut;

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

class ShortcutSection: public mforms::DrawBox
{
private:
  HomeScreen *_owner;
  cairo_surface_t* _default_shortcut_icon;

  std::vector<ShortcutEntry> _shortcuts;
  typedef std::vector<ShortcutEntry>::iterator ShortcutIterator;

  app_StarterRef _hot_shortcut;
  app_StarterRef _active_shortcut; // For the context menu.
  mforms::Menu _shortcut_context_menu;

  boost::function <bool (int, int)> _accessible_click_handler;

  HomeAccessibleButton _page_up_button;
  HomeAccessibleButton _page_down_button;

  ssize_t _page_start;
  ssize_t _shortcuts_per_page;
  cairo_surface_t *_page_down_icon;
  cairo_surface_t *_page_up_icon;

public:
  ShortcutSection(HomeScreen *owner)
  {
    _owner = owner;
    _hot_shortcut = app_StarterRef();
    _active_shortcut = app_StarterRef();
    _default_shortcut_icon = mforms::Utilities::load_icon("wb_starter_generic_52.png");
    _page_down_icon = mforms::Utilities::load_icon("wb_tile_page-down.png");
    _page_up_icon = mforms::Utilities::load_icon("wb_tile_page-up.png");

    _page_start = 0;

    _accessible_click_handler = boost::bind(&ShortcutSection::mouse_click, this,
      mforms::MouseButtonLeft, _1, _2);

    _page_up_button.name = "Page Up";
    _page_up_button.default_action = "Move Shortcut Pages Up";
    _page_up_button.default_handler = _accessible_click_handler;
    
    _page_down_button.name = "Page Down";
    _page_down_button.default_action = "Move Shortcut Pages Down";
    _page_down_button.default_handler = _accessible_click_handler;

    /* Disabled for now until we can add new shortcuts.
    _shortcut_context_menu.add_item(_("Remove Shortcut"), "remove_shortcut");
    _shortcut_context_menu.set_handler(boost::bind(&ShortcutSection::handle_command, this, _1));
    _*/
  }

  ~ShortcutSection()
  {
    delete_surface(_default_shortcut_icon);
    delete_surface(_page_down_icon);
    delete_surface(_page_up_icon);

    clear_shortcuts();
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Called from the context menu.
   */
  void handle_command(const std::string &command)
  {
    if (command == "remove_shortcut")
      _owner->trigger_callback(ActionRemoveShortcut, _active_shortcut);
    _active_shortcut = app_StarterRef();
  }

  //--------------------------------------------------------------------------------------------------

  void draw_paging_part(cairo_t *cr, int current_page, int pages, bool high_contrast)
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

    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgb(cr, 0x5E / 255.0, 0x5E / 255.0, 0x5E / 255.0);
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

#define SHORTCUTS_LEFT_PADDING  55
#define SHORTCUTS_TOP_PADDING   75 // The vertical offset of the first shortcut entry.
#define SHORTCUTS_RIGHT_PADDING 25
#define SHORTCUTS_ROW_HEIGHT    50
#define SHORTCUTS_SPACING       18 // Vertical space between entries.

  void repaint(cairo_t *cr, int areax, int areay, int areaw, int areah)
  {
    layout(cr);
  
    int height = get_height();

    cairo_select_font_face(cr, HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, HOME_TITLE_FONT_SIZE);

    bool high_contrast = base::Color::is_high_contrast_scheme();
    if (high_contrast)
      cairo_set_source_rgb(cr, 0, 0, 0);
    else
      cairo_set_source_rgb(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0);
    cairo_move_to(cr, SHORTCUTS_LEFT_PADDING, 45);
    cairo_show_text(cr, _("Shortcuts"));
    cairo_stroke(cr);

    // Shortcuts block.
    int yoffset = SHORTCUTS_TOP_PADDING;
    if (_shortcuts.size() > 0 && yoffset < height)
    {
      cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);

      for (ShortcutIterator iterator = _shortcuts.begin() + _page_start; iterator != _shortcuts.end();
        iterator++)
      {
        double alpha = (yoffset + SHORTCUTS_ROW_HEIGHT) > height ? 0.25 : 1;

        iterator->acc_bounds.pos.x = SHORTCUTS_LEFT_PADDING;
        iterator->acc_bounds.pos.y = yoffset;
        iterator->acc_bounds.size.width = get_width() - (SHORTCUTS_LEFT_PADDING + SHORTCUTS_RIGHT_PADDING);
        iterator->acc_bounds.size.height = SHORTCUTS_ROW_HEIGHT;

        cairo_set_source_surface(cr, iterator->icon, SHORTCUTS_LEFT_PADDING, yoffset);
        cairo_paint_with_alpha(cr, alpha);

        if (!iterator->title.empty())
        {
          if (high_contrast)
            cairo_set_source_rgba(cr, 0, 0, 0, alpha);
          else
            cairo_set_source_rgba(cr, 0xf3 / 255.0, 0xf3 / 255.0, 0xf3 / 255.0, alpha);
          text_with_decoration(cr, iterator->title_bounds.left(), iterator->title_bounds.top() + yoffset,
            iterator->title.c_str(), iterator->shortcut == _hot_shortcut, iterator->title_bounds.width());
        }

        yoffset += SHORTCUTS_ROW_HEIGHT + SHORTCUTS_SPACING;
        if (yoffset >= height)
          break;
      }

      // See if we need to draw the paging indicator.
      height -= SHORTCUTS_TOP_PADDING;
      _shortcuts_per_page = height / (SHORTCUTS_ROW_HEIGHT + SHORTCUTS_SPACING);
      if (_shortcuts_per_page < 1)
        _shortcuts_per_page = 1;
      int pages = (int)ceil(_shortcuts.size() / (float)_shortcuts_per_page);
      if (pages > 1)
      {
        int current_page = (int)ceil(_page_start / (float)_shortcuts_per_page);
        draw_paging_part(cr, current_page, pages, high_contrast);
      }
      else
      {
        _page_up_button.bounds = base::Rect();
        _page_down_button.bounds = base::Rect();
        _page_start = 0; // Size increased to cover the full content.
      }
    }
  }

  //--------------------------------------------------------------------------------------------------

  int shortcut_from_point(int x, int y)
  {
    if (x < SHORTCUTS_LEFT_PADDING || y < SHORTCUTS_TOP_PADDING || x > get_width() - SHORTCUTS_RIGHT_PADDING)
      return -1;

    y -= SHORTCUTS_TOP_PADDING;
    int point_in_row = y % (SHORTCUTS_ROW_HEIGHT + SHORTCUTS_SPACING);
    if (point_in_row >= SHORTCUTS_ROW_HEIGHT)
      return -1; // In the spacing between entries.

    size_t row = y / (SHORTCUTS_ROW_HEIGHT + SHORTCUTS_SPACING);
    size_t height = get_height() - SHORTCUTS_TOP_PADDING;
    size_t row_bottom = row * (SHORTCUTS_ROW_HEIGHT + SHORTCUTS_SPACING) + SHORTCUTS_ROW_HEIGHT;
    if (row_bottom > height)
      return -1; // The last shortcut is dimmed if it goes over the bottom border.
                               // Take it out from the hit test too.

    row += _page_start;
    if (row < _shortcuts.size())
      return (int)row;

    return -1;
  }

  //--------------------------------------------------------------------------------------------------

  /**
   * Adds a new shortcut entry to the internal list. The function performs some sanity checks.
   */
  void add_shortcut(const std::string& icon_name, const grt::ValueRef& object)
  {
    app_StarterRef shortcut = app_StarterRef::cast_from(object);

    ShortcutEntry entry;
    
    entry.shortcut = shortcut;

    // See if we can load the icon. If not use the placeholder.
    entry.icon = mforms::Utilities::load_icon(icon_name);
    if (entry.icon == NULL)
      entry.icon = _default_shortcut_icon;

    _shortcuts.push_back(entry);
    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  void clear_shortcuts()
  {
    for (ShortcutIterator iterator= _shortcuts.begin(); iterator != _shortcuts.end(); iterator++)
      if (iterator->icon != _default_shortcut_icon)
        delete_surface(iterator->icon);
    _shortcuts.clear();
    set_layout_dirty(true);
  }

  //--------------------------------------------------------------------------------------------------

  void layout(cairo_t* cr)
  {
    if (is_layout_dirty())
    {
      set_layout_dirty(false);

      double icon_xoffset = SHORTCUTS_LEFT_PADDING;
      double text_xoffset = icon_xoffset + 60;

      double yoffset = SHORTCUTS_TOP_PADDING;

      double text_width = get_width() - text_xoffset - SHORTCUTS_RIGHT_PADDING;

      cairo_select_font_face(cr, HOME_NORMAL_FONT, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size(cr, HOME_SUBTITLE_FONT_SIZE);

      cairo_font_extents_t font_extents;
      cairo_font_extents(cr, &font_extents);
      double text_height = ceil(font_extents.height);

      // Compute bounding box for each shortcut entry.
      for (ShortcutIterator iterator = _shortcuts.begin(); iterator != _shortcuts.end(); iterator++)
      {
        int icon_height = image_height(iterator->icon);

        std::string title = iterator->shortcut->title();
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

        yoffset += SHORTCUTS_ROW_HEIGHT + SHORTCUTS_SPACING;
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
          if (_page_up_button.bounds.contains(x, y))
          {
            // Page up clicked. Doesn't happen if we are on the first page already.
            _page_start -= _shortcuts_per_page;
            if (_page_start < 0)
              _page_start = 0;
            set_needs_repaint();
            return true;
          }

          if (_page_down_button.bounds.contains(x, y))
          {
            _page_start += _shortcuts_per_page;
            set_needs_repaint();
            return true;
          }

          if (_hot_shortcut.is_valid())
            _owner->trigger_callback(ActionShortcut, _hot_shortcut);
        }
        break;

      case mforms::MouseButtonRight:
        {
          if (_hot_shortcut.is_valid())
          {
            _active_shortcut = _hot_shortcut;
            _shortcut_context_menu.popup_at(this, x, y);
            return true;
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
    if (_hot_shortcut.is_valid())
    {
      _hot_shortcut = app_StarterRef();
      set_needs_repaint();
      return true;
    }
    return false;
  }

  //--------------------------------------------------------------------------------------------------

  virtual bool mouse_move(mforms::MouseButton button, int x, int y)
  {
    app_StarterRef shortcut;
    int row = shortcut_from_point(x, y);
    if (row > -1)
      shortcut = _shortcuts[row].shortcut;
    if (shortcut != _hot_shortcut)
    {
      _hot_shortcut = shortcut;
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
    int ret_val = 0;

    ret_val += (int)_shortcuts.size();

    // Adds 2 for the paging buttons if shown
    if (_page_up_button.bounds.width())
      ret_val += 2;

    return ret_val;
  }

  //------------------------------------------------------------------------------------------------
  
  virtual Accessible* get_acc_child(int index)
  { 
    mforms::Accessible* accessible = NULL;

    if (index < (int)_shortcuts.size())
      accessible = &_shortcuts[index];
    else
    {
      index -= (int)_shortcuts.size();
      accessible = index ? &_page_down_button : &_page_up_button;
    }

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

    if (_page_up_button.bounds.contains(x, y))
      accessible = &_page_up_button;
    else if (_page_down_button.bounds.contains(x, y))
      accessible = &_page_down_button;
    else
    {
      int row = shortcut_from_point(x, y);
      if (row != -1)
        accessible = &_shortcuts[row];
    }

    return accessible;
  }

};

//----------------- HomeScreen ---------------------------------------------------------------------

#include "workbench/wb_command_ui.h"

HomeScreen::HomeScreen(CommandUI *cmdui, db_mgmt_ManagementRef rdbms)
  : AppView(true, "home", true)
{
  _rdbms = rdbms;

  _callback = NULL;
  _user_data = NULL;

  mforms::Box *top_part = mforms::manage(new mforms::Box(false));
  _connection_section = new ConnectionsSection(this);
  _connection_section->set_name("Home Connections Section");

  top_part->add(_connection_section, true, true);

  _document_section = new DocumentsSection(this);
  _document_section->set_name("Home Models Section");
  _document_section->set_size(-1, 236);
  top_part->add(_document_section, false, true);

  add(top_part, true, true);

  _shortcut_section = new ShortcutSection(this);
  _shortcut_section->set_name("Home Shortcuts Section");
  _shortcut_section->set_size(300, -1);
  add(_shortcut_section, false, true);
  
  _menubar = mforms::manage(cmdui->create_menubar_for_context(WB_CONTEXT_HOME_GLOBAL));
  //_toolbar = mforms::manage(cmdui->create_toolbar(""));

  update_colors();

  Box::scoped_connect(signal_resized(), boost::bind(&HomeScreen::on_resize, this));
  base::NotificationCenter::get()->add_observer(this, "GNColorsChanged");
}

//--------------------------------------------------------------------------------------------------

HomeScreen::~HomeScreen()
{
  base::NotificationCenter::get()->remove_observer(this);

  delete _shortcut_section;
  delete _connection_section;
  delete _document_section;
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::update_colors()
{
#ifdef __APPLE__
  _connection_section->set_back_color("#323232");
  _document_section->set_back_color("#343434");
  _shortcut_section->set_back_color("#373737");
#else
  bool high_contrast = base::Color::is_high_contrast_scheme();

  _connection_section->set_back_color(high_contrast ? "#f0f0f0" : "#1d1d1d");
  _document_section->set_back_color(high_contrast ? "#f8f8f8" : "#242424");
  _shortcut_section->set_back_color(high_contrast ? "#ffffff" : "#303030");
#endif
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

void HomeScreen::clear_shortcuts()
{
  _shortcut_section->clear_shortcuts();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::add_shortcut(const grt::ValueRef& object, const std::string& icon_name)
{
  _shortcut_section->add_shortcut(icon_name, object);
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::clear_connections()
{
  _connection_section->clear_connections();
}

//--------------------------------------------------------------------------------------------------

void HomeScreen::add_connection(db_mgmt_ConnectionRef connection, const std::string &title,
  const std::string &description, const std::string &user, const std::string &schema)
{
  _connection_section->add_connection(connection, title, description, user, schema);
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
    case HomeMenuConnectionFabric:
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
