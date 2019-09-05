/*
 * Copyright (c) 2014, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "mforms/home_screen_connections.h"

#include "mforms/menu.h"
#include "mforms/popup.h"
#include "mforms/imagebox.h"
#include "mforms/scrollpanel.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/log.h"
#include "base/any.h"

DEFAULT_LOG_DOMAIN("home");

using namespace base;
using namespace mforms;

//----------------- ConnectionsSection ---------------------------------------------------------------------------------

class mforms::ConnectionEntry : public base::Accessible {
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

  base::Rect bounds;

  //------ Accessibility Methods ---------------------------------------------------------------------------------------

  virtual std::string getAccessibilityDescription() override {
    return title;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual std::string getAccessibilityTitle() override {
    return title;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual std::string getAccessibilityValue() override {
    std::string result = "host: " + description;
    if (!schema.empty())
      result += ", schema: " + schema;
    if (!user.empty())
      result += ", user: " + user;
    return result;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual Accessible::Role getAccessibilityRole() override {
    return Accessible::PushButton;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Rect getAccessibilityBounds() override {
    return bounds;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void accessibilityDoDefaultAction() override {
    activate();
  };

  //--------------------------------------------------------------------------------------------------------------------

  virtual std::string getAccessibilityDefaultAction() override {
    return "click";
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual bool accessibilityGrabFocus() override {
    owner->setFocusOnEntry(this);
    return true;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void accessibilityShowMenu() override {
    if (owner->_connection_context_menu != nullptr) {
      owner->_connection_context_menu->popup_at(owner, static_cast<int>(bounds.xcenter()),
        static_cast<int>(bounds.ycenter()));
    }
  };

  //--------------------------------------------------------------------------------------------------------------------

  /**
   * Draws the icon followed by the given text. The given position is that of the upper left corner
   * of the image.
   */
  void draw_icon_with_text(cairo_t *cr, double x, double y, cairo_surface_t *icon, const std::string &text,
                           double alpha) {
    if (icon) {
      mforms::Utilities::paint_icon(cr, icon, x, y);
      x += imageWidth(icon) + 3;
    }

    base::Color titleColor = getTitleColor();
    cairo_set_source_rgba(cr, titleColor.red, titleColor.green, titleColor.blue, titleColor.alpha);

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

  //--------------------------------------------------------------------------------------------------------------------

public:
  enum ItemPosition { First, Last, Other };

  //--------------------------------------------------------------------------------------------------------------------

  ConnectionEntry(ConnectionsSection *aowner) : owner(aowner), compute_strings(false) {
    draw_info_tab = true;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual bool is_movable() const {
    return true;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Color getTitleColor() const {
    return owner->_titleColor;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Color getBackgroundColor(bool hot) const {
    return hot ? owner->_backgroundColorHot : owner->_backgroundColor;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual cairo_surface_t *get_background_icon() const {
    return owner->_sakila_icon;
  }

  //--------------------------------------------------------------------------------------------------------------------

  void draw_tile_background(cairo_t *cr, bool hot, double alpha, bool for_dragging) const {
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

    if (owner->_owner->isDarkModeActive())
      cairo_set_source_rgba(cr, backColor.red + 0.1, backColor.green + 0.1, backColor.blue + 0.1, alpha);
    else
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

  //--------------------------------------------------------------------------------------------------------------------

  virtual void draw_tile(cairo_t *cr, bool hot, double alpha, bool for_dragging) {
    base::Color titleColor = getTitleColor();
    base::Rect bounds = this->bounds;
    if (for_dragging)
      bounds.pos = base::Point(0, 0);

    draw_tile_background(cr, hot, alpha, for_dragging);

    cairo_set_source_rgba(cr, titleColor.red, titleColor.green, titleColor.blue, alpha);

    std::string systemFont = base::OSConstants::defaultFontName();
    cairo_select_font_face(cr, systemFont.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TILES_TITLE_FONT_SIZE);

    // Title string.
    double x = (int)bounds.left() + 10.5; // Left offset from the border to caption, user and network icon.
    double y = bounds.top() + 27;         // Distance from top to the caption base line.

    if (compute_strings) {
      // On first render compute the actual string to show. We only need to do this once
      // as neither the available space changes nor is the entry manipulated.

      // We try to shrink titles in the middle. If there's a colon in it we assume it's a port number
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

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

  virtual void activate() {
    owner->_owner->trigger_callback(HomeScreenAction::ActionOpenConnectionFromList, connectionId);
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual mforms::Menu *context_menu() const {
    return owner->_connection_context_menu;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void menu_open(ItemPosition pos) const {
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

  //--------------------------------------------------------------------------------------------------------------------

};

//----------------------------------------------------------------------------------------------------------------------

class mforms::FolderEntry : public ConnectionEntry, public std::enable_shared_from_this<mforms::FolderEntry> {
protected:

  //--------------------------------------------------------------------------------------------------------------------

  virtual std::string getAccessibilityDescription() override {
    return "Connection Group";
  }

  virtual std::string getAccessibilityTitle() override {
    return title;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void accessibilityShowMenu() override {
    if (owner->_folder_context_menu != nullptr) {
      owner->_folder_context_menu->popup_at(owner, static_cast<int>(bounds.xcenter()),
        static_cast<int>(bounds.ycenter()));
    }
  };

  //--------------------------------------------------------------------------------------------------------------------

public:
  std::vector<std::shared_ptr<ConnectionEntry> > children;

  //--------------------------------------------------------------------------------------------------------------------

  FolderEntry(ConnectionsSection *aowner) : ConnectionEntry(aowner) {
    draw_info_tab = false;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void draw_tile_text(cairo_t *cr, double x, double y, double alpha) override {
    base::Color titleColor = getTitleColor();
    cairo_set_source_rgba(cr, titleColor.red, titleColor.green, titleColor.blue, titleColor.alpha);

    std::string info = std::to_string(children.size() - 1) + " " + _("Connections");
    y = bounds.top() + 55;
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, info.c_str());
    cairo_stroke(cr);
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual mforms::Menu *context_menu() const override {
    return owner->_folder_context_menu;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void menu_open(ItemPosition pos) const override {
    mforms::Menu *menu = context_menu();

    menu->set_item_enabled(menu->get_item_index("move_connection_to_top"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_up"), pos != First);
    menu->set_item_enabled(menu->get_item_index("move_connection_down"), pos != Last);
    menu->set_item_enabled(menu->get_item_index("move_connection_to_end"), pos != Last);
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void activate() override {
    owner->change_to_folder(shared_from_this());
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Color getTitleColor() const override {
    return owner->_folderTitleColor;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Color getBackgroundColor(bool hot) const override {
    return hot ? owner->_folderBackgroundColorHot : owner->_folderBackgroundColor;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual cairo_surface_t *get_background_icon() const override {
    return owner->_folder_icon;
  }

  //--------------------------------------------------------------------------------------------------------------------

};

class mforms::FolderBackEntry : public ConnectionEntry {
protected:

  //--------------------------------------------------------------------------------------------------------------------

  virtual void accessibilityShowMenu() override {
  };

  //--------------------------------------------------------------------------------------------------------------------

  virtual std::string getAccessibilityDescription() override {
      return "Back";
  }

  //--------------------------------------------------------------------------------------------------------------------

public:

  FolderBackEntry(ConnectionsSection *aowner) : ConnectionEntry(aowner) {
    title = "< back";
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual bool is_movable() const override {
    return false;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Color getTitleColor() const override {
    return owner->_folderTitleColor;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual base::Color getBackgroundColor(bool hot) const override {
    return hot ? owner->_backTileBackgroundColorHot : owner->_backTileBackgroundColor;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual cairo_surface_t *get_background_icon() const override {
    return owner->_folder_icon;
  }

  //--------------------------------------------------------------------------------------------------------------------

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

  //--------------------------------------------------------------------------------------------------------------------

  virtual mforms::Menu *context_menu() const override {
    return nullptr;
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void menu_open(ItemPosition pos) const override {
  }

  //--------------------------------------------------------------------------------------------------------------------

  virtual void activate() override {
    owner->change_to_folder(std::shared_ptr<FolderEntry>());
  }

  //--------------------------------------------------------------------------------------------------------------------

};

//----------------- ConnectionsWelcomeScreen ---------------------------------------------------------------------------

ConnectionsWelcomeScreen::ConnectionsWelcomeScreen(HomeScreen *owner) : _owner(owner) {
  logDebug("Creating Connections Welcome Screen\n");

  _closeHomeScreenButton.title = "Close Welcome Message Screen";
  _closeHomeScreenButton.description = "Close Welcome Message Screen";
  _closeHomeScreenButton.defaultHandler = [this]() {
    _owner->trigger_callback(HomeScreenAction::CloseWelcomeMessage, base::any());
  };

  _browseDocButton.title = "Browse Documentation >";
  _browseDocButton.description = "Browse Documentation";
  _browseDocButton.defaultHandler =  [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionOpenDocs, base::any());
  };

  _readBlogButton.title = "Read the Blog >";
  _readBlogButton.description = "Open Blog";
  _readBlogButton.defaultHandler =  [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionOpenBlog, base::any());
  };

  _discussButton.title = "Discuss on the Forums >";
  _discussButton.description = "Open Forum";
  _discussButton.defaultHandler =  [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionOpenForum, base::any());
  };

  _closeIcon = nullptr;

  _heading = "Welcome to MySQL Workbench";
  _content = {
    "MySQL Workbench is the official graphical user interface (GUI) tool for MySQL. It allows you to design,",
    "create and browse your database schemas, work with database objects and insert data as well as",
    "design and run SQL queries to work with stored data. You can also migrate schemas and data from other",
    "database vendors to your MySQL database."
  };
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

Accessible::Role ConnectionsWelcomeScreen::getAccessibilityRole() {
  return base::Accessible::Pane;
}

//----------------------------------------------------------------------------------------------------------------------

std::string ConnectionsWelcomeScreen::getAccessibilityTitle() {
  return _heading;
}

//----------------------------------------------------------------------------------------------------------------------

std::string ConnectionsWelcomeScreen::getAccessibilityDescription() {
  return "Home Screen Welcome Page";
}

//----------------------------------------------------------------------------------------------------------------------

std::string ConnectionsWelcomeScreen::getAccessibilityValue() {
  std::string result;
  for (auto &line : _content)
    result += line + "\n";
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

size_t ConnectionsWelcomeScreen::getAccessibilityChildCount() {
  return 4;
}

//----------------------------------------------------------------------------------------------------------------------

Accessible* ConnectionsWelcomeScreen::getAccessibilityChild(size_t index) {
  switch (index) {
    case 1:
      return &_browseDocButton;
    case 2:
      return &_readBlogButton;
    case 3:
      return &_discussButton;
    default:
      return &_closeHomeScreenButton;
  }
}

//----------------------------------------------------------------------------------------------------------------------

base::Rect ConnectionsWelcomeScreen::getAccessibilityBounds() {
  return base::Rect(0, 100, 500, 700);
}

//----------------------------------------------------------------------------------------------------------------------

Accessible* ConnectionsWelcomeScreen::accessibilityHitTest(ssize_t x, ssize_t y) {
  if (_browseDocButton.bounds.contains(static_cast<double>(x), static_cast<double>(y))) {
    return &_browseDocButton;
  }

  if (_discussButton.bounds.contains(static_cast<double>(x), static_cast<double>(y))) {
    return &_discussButton;
  }

  if (_readBlogButton.bounds.contains(static_cast<double>(x), static_cast<double>(y))) {
    return &_readBlogButton;
  }

  if (_closeHomeScreenButton.bounds.contains(static_cast<double>(x), static_cast<double>(y))) {
    return &_closeHomeScreenButton;
  }

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsWelcomeScreen::repaint(cairo_t *cr, int areax, int areay, int areaw, int areah) {
  Size size = Utilities::getImageSize(_closeIcon);
  _closeHomeScreenButton.bounds = base::Rect(get_width() - size.width - 8, 8, size.width, size.height);

  cairo_save(cr);
  mforms::Utilities::paint_icon(cr, _closeIcon, _closeHomeScreenButton.bounds.left(), _closeHomeScreenButton.bounds.top());

  int yoffset = 100;

  cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                         CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 3);
  cairo_set_source_rgb(cr, _textColor.red, _textColor.green, _textColor.blue);

  cairo_text_extents_t extents;
  cairo_text_extents(cr, _heading.c_str(), &extents);

  double x;
  x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
  cairo_move_to(cr, x, yoffset);
  cairo_show_text(cr, _heading.c_str());
  yoffset += mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 3;

  for (auto &line : _content) {
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);
    cairo_text_extents(cr, line.c_str(), &extents);
    x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, line.c_str());
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
    cairo_text_extents(cr, btn->title.c_str(), &extents);
    x = get_width() * pos - (extents.width / 2 + extents.x_bearing);
    cairo_move_to(cr, floor(x), floor(yoffset));
    cairo_show_text(cr, btn->title.c_str());
    btn->bounds = base::Rect(ceil(x), floor(yoffset + extents.y_bearing), ceil(extents.width), ceil(extents.height));
    pos += 0.25;
  }

  _totalHeight = yoffset + 20;

  cairo_restore(cr);
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsWelcomeScreen::updateColors() {
  if (_owner->isDarkModeActive()) {
    _textColor = base::Color::parse("#F4F4F4");
  } else {
    _textColor = base::Color::parse("#505050");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void ConnectionsWelcomeScreen::updateIcons() {
  cairo_surface_destroy(_closeIcon);
  if (_owner->isDarkModeActive())
    _closeIcon = Utilities::load_icon("home_screen_close_dark.png", true);
  else
    _closeIcon = Utilities::load_icon("home_screen_close_light.png", true);
}

//----------------------------------------------------------------------------------------------------------------------

bool ConnectionsWelcomeScreen::mouse_click(mforms::MouseButton button, int x, int y) {
  if (button == MouseButtonLeft) {
    HomeAccessibleButton * button = dynamic_cast<HomeAccessibleButton *>(accessibilityHitTest(x, y));
    if (button != nullptr) {
      button->accessibilityDoDefaultAction();
      return true;
    }
  }
  return false;
}

//------------------ ConnectionsSection --------------------------------------------------------------------------------

ConnectionsSection::ConnectionsSection(HomeScreen *owner) : HomeScreenSection("sidebar_wb.png"),
  _search_box(true), _search_text(mforms::SmallSearchEntry), _showWelcomeHeading(true) {

  _owner = owner;
  _welcomeScreen = nullptr;
  _container = nullptr;
  _connection_context_menu = nullptr;
  _folder_context_menu = nullptr;
  _generic_context_menu = nullptr;
  _drag_index = -1;
  _drop_index = -1;
  _filtered = false;

  _folder_icon = nullptr;
  _network_icon = nullptr;
  _plus_icon = nullptr;
  _sakila_icon = nullptr;
  _user_icon = nullptr;
  _manage_icon = nullptr;

  std::vector<std::string> formats;
  formats.push_back(mforms::HomeScreenSettings::TILE_DRAG_FORMAT); // We allow dragging tiles to reorder them.
  formats.push_back(mforms::DragFormatFileName);                   // We accept sql script files to open them.
  register_drop_formats(this, formats);

  _search_box.set_name("Connection Search Box");

  _search_box.set_spacing(5);
  _search_text.set_size(150, -1);

#ifdef _MSC_VER
  _search_text.set_bordered(false);
  _search_text.set_size(-1, 18);
  _search_text.set_font(mforms::HomeScreenSettings::HOME_NORMAL_FONT);
  _search_box.set_size(-1, 18);
#else
  _search_box.set_padding(8, 1, 8, 5);
  _search_box.set_size(160, 25);
#endif

#ifdef _MSC_VER
  mforms::ImageBox *image = mforms::manage(new mforms::ImageBox, false);
  image->set_image("search_sidebar.png");
  image->set_image_align(mforms::MiddleCenter);
  _search_box.add(image, false, false);
#endif
  _search_text.set_name("Search Text");
  _search_text.set_placeholder_text("Filter connections");
  _search_text.set_bordered(false);
  _search_box.add(&_search_text, true, true);
  scoped_connect(_search_text.signal_changed(), std::bind(&ConnectionsSection::on_search_text_changed, this));
  scoped_connect(_search_text.signal_action(),
                 std::bind(&ConnectionsSection::on_search_text_action, this, std::placeholders::_1));
  add(&_search_box, mforms::TopRight);

  set_padding(0, 30, CONNECTIONS_RIGHT_PADDING, 0);

  _add_button.title = "Add Connection";
  _add_button.description = "Add Connection";
  _add_button.defaultHandler = [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionNewConnection, base::any());
  };

  _manage_button.title = "Manage Connections";
  _manage_button.description = "Manage Connections";
  _manage_button.defaultHandler = [this]() {
    _owner->trigger_callback(HomeScreenAction::ActionManageConnections, base::any());
  };

  _rescanButton.title = "Rescan servers";
  _rescanButton.description = "Rescan Servers";
  _rescanButton.defaultHandler = [this]() {
    _owner->trigger_callback(HomeScreenAction::RescanLocalServers, base::any());
  };
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
  deleteSurface(_network_icon);
  deleteSurface(_plus_icon);
  deleteSurface(_sakila_icon);
  deleteSurface(_user_icon);
  deleteSurface(_manage_icon);
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::updateColors() {
  if (_owner->isDarkModeActive()) {
    _titleColor = base::Color::parse("#F4F4F4");
    _folderTitleColor = base::Color::parse("#F0F0F0");
    _backgroundColor = base::Color::parse("#505050");
    _backgroundColorHot = base::Color::parse("#626160");
    _folderBackgroundColor = base::Color::parse("#3477a6");
    _folderBackgroundColorHot = base::Color::parse("#4699b8");
    _backTileBackgroundColor = base::Color::parse("#d9532c");
    _backTileBackgroundColorHot = base::Color::parse("#d97457");
  } else {
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
    _search_text.set_placeholder_color("#303030");
    _search_text.set_back_color("#ffffff");
#endif
  }

  if (_welcomeScreen != nullptr)
    _welcomeScreen->updateColors();
}

//------------------------------------------------------------------------------------------------

void ConnectionsSection::updateIcons() {
  if (_owner->isDarkModeActive()) {
    deleteSurface(_sakila_icon);
    _sakila_icon = mforms::Utilities::load_icon("wb_tile_sakila_dark.png");

    deleteSurface(_manage_icon);
    _manage_icon = mforms::Utilities::load_icon("wb_tile_manage_dark.png");

    deleteSurface(_folder_icon);
    _folder_icon = mforms::Utilities::load_icon("wb_tile_folder.png");

    deleteSurface(_network_icon);
    _network_icon = mforms::Utilities::load_icon("wb_tile_network_dark.png");

    deleteSurface(_plus_icon);
    _plus_icon = mforms::Utilities::load_icon("wb_tile_plus_dark.png");

    deleteSurface(_user_icon);
    _user_icon = mforms::Utilities::load_icon("wb_tile_user_dark.png");
  } else {
    deleteSurface(_sakila_icon);
    _sakila_icon = mforms::Utilities::load_icon("wb_tile_sakila_light.png");

    deleteSurface(_manage_icon);
    _manage_icon = mforms::Utilities::load_icon("wb_tile_manage_light.png");

    deleteSurface(_folder_icon);
    _folder_icon = mforms::Utilities::load_icon("wb_tile_folder.png");

    deleteSurface(_network_icon);
    _network_icon = mforms::Utilities::load_icon("wb_tile_network_light.png");

    deleteSurface(_plus_icon);
    _plus_icon = mforms::Utilities::load_icon("wb_tile_plus_light.png");

    deleteSurface(_user_icon);
    _user_icon = mforms::Utilities::load_icon("wb_tile_user_light.png");
  }

  if (_welcomeScreen != nullptr)
    _welcomeScreen->updateIcons();
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

  updateFocusableAreas();
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

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<ConnectionEntry> ConnectionsSection::entry_from_point(int x, int y) const {
  std::shared_ptr<ConnectionEntry> entry;

  ConnectionVector connections(displayed_connections());
  for (ConnectionVector::iterator conn = connections.begin(); conn != connections.end(); ++conn) {
    if ((*conn)->bounds.contains(x, y)) {
      entry = *conn;
      break;
    }
  }

  return entry;
}

//----------------------------------------------------------------------------------------------------------------------

std::shared_ptr<ConnectionEntry> ConnectionsSection::entry_from_index(ssize_t index) const {
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
                         CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE);

  cairo_set_source_rgba(cr, _titleColor.red, _titleColor.green, _titleColor.blue, _titleColor.alpha);
  cairo_move_to(cr, CONNECTIONS_LEFT_PADDING, yoffset);

  ConnectionVector const& connections(displayed_connections());
  std::string title = _("MySQL Connections");
  if (_active_folder)
    title += " / " + _active_folder->title;

  cairo_show_text(cr, title.c_str());

  // The + button after the title.
  cairo_text_extents_t extents;
  cairo_text_extents(cr, title.c_str(), &extents);
  double text_width = ceil(extents.width);

  _add_button.bounds = base::Rect(CONNECTIONS_LEFT_PADDING + text_width + 10, yoffset - imageHeight(_plus_icon) + 2,
                                  imageWidth(_plus_icon), imageHeight(_plus_icon));

  cairo_set_source_surface(cr, _plus_icon, _add_button.bounds.left(), _add_button.bounds.top());
  cairo_paint(cr);

  _manage_button.bounds = base::Rect(_add_button.bounds.right() + 4, yoffset - imageHeight(_manage_icon) + 2,
                                     imageWidth(_manage_icon), imageHeight(_manage_icon));
  cairo_set_source_surface(cr, _manage_icon, _manage_button.bounds.left(), _manage_button.bounds.top());
  cairo_paint(cr);

  int row = 0;

  base::Rect bounds(0, CONNECTIONS_TOP_PADDING, CONNECTIONS_TILE_WIDTH, CONNECTIONS_TILE_HEIGHT);

  if (connections.size() == 0) {
    std::string line1 = "MySQL Workbench could not detect any MySQL server running.";
    std::string line2 = "This means that MySQL is not installed or is not running.";

    double x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
    int yoffset = static_cast<int>(bounds.top()) + 30;
    cairo_text_extents_t extents;
    cairo_set_source_rgb(cr, _titleColor.red, _titleColor.green, _titleColor.blue);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);
    cairo_text_extents(cr, line1.c_str(), &extents);

    x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
    yoffset += static_cast<int>(extents.height) + 10;

    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, line1.c_str());

    cairo_text_extents(cr, line2.c_str(), &extents);

    x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
    yoffset += static_cast<int>(extents.height) + 10;

    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, line2.c_str());

    cairo_select_font_face(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT, CAIRO_FONT_SLANT_NORMAL,
                          CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, mforms::HomeScreenSettings::HOME_TITLE_FONT_SIZE * 0.8);
    cairo_set_source_rgb(cr, 0x1b / 255.0, 0xad / 255.0, 0xe8 / 255.0);
    cairo_text_extents(cr, _rescanButton.title.c_str(), &extents);

    x = get_width() / 2 - (extents.width / 2 + extents.x_bearing);
    yoffset += static_cast<int>(extents.height) + 10;

    cairo_move_to(cr, x, yoffset);
    cairo_show_text(cr, _rescanButton.title.c_str());

    _rescanButton.bounds = base::Rect(x, yoffset - extents.height - 5, extents.width, extents.height + 10);

    return;
  }

  std::size_t index = 0;
  bool done = false;
  while (!done) {
    if (index >= connections.size())
      break; // we're done

    bounds.pos.x = CONNECTIONS_LEFT_PADDING;
    for (int column = 0; column < tiles_per_row; column++) {
      // Update the stored bounds of the tile.
      connections[index]->bounds = bounds;

      bool draw_hot = connections[index] == _hot_entry;
      connections[index]->draw_tile(cr, draw_hot, 1.0, false);

      // Draw drop indicator.
      if (static_cast<ssize_t>(index) == _drop_index) {
        if (mforms::App::get()->isDarkModeActive())
          cairo_set_source_rgb(cr, 1, 1, 1);
        else
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

  mforms::DrawBox::repaint(cr, areax, areay, areaw, areah);
}

//----------------------------------------------------------------------------------------------------------------------

base::Size ConnectionsSection::getLayoutSize(base::Size proposedSize) {
  ConnectionVector const& connections(displayed_connections());

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

const char* ConnectionsSection::getTitle() {
  return "Connections Section";
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

      folder->description = parent_name;
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

void ConnectionsSection::updateFocusableAreas() {
  clearFocusableAreas();
  if (!_filtered_connections.empty()) {
    for (const auto &it: _filtered_connections) {
      mforms::FocusableArea fArea;
      fArea.activate = [it]() { it->activate(); };
      fArea.getBounds = [it]() { return it->getAccessibilityBounds(); };
      fArea.showContextMenu = [it, this]() {
        const auto bounds = it->getAccessibilityBounds();
        // We need to set the _entry_for_menu otherwise,
        // some options may crash the app.
        _entry_for_menu = entry_from_point(static_cast<int>(bounds.center().x), static_cast<int>(bounds.center().y));
        it->context_menu()->popup_at(this, static_cast<int>(bounds.center().x), static_cast<int>(bounds.center().y));
      };
      addFocusableArea(fArea);
    }
  } else {
    ConnectionVector current_connections = !_active_folder ? _connections : _active_folder->children;
    for (const auto &it: current_connections) {
      mforms::FocusableArea fArea;
      fArea.activate = [it]() { it->activate(); };
      fArea.getBounds = [it]() { return it->getAccessibilityBounds(); };
      fArea.showContextMenu = [it, this]() {
        const auto bounds = it->getAccessibilityBounds();
        // We need to set the _entry_for_menu otherwise,
        // some options may crash the app.
        _entry_for_menu = entry_from_point(static_cast<int>(bounds.center().x), static_cast<int>(bounds.center().y));
        it->context_menu()->popup_at(this, static_cast<int>(bounds.center().x), static_cast<int>(bounds.center().y));
      };
      addFocusableArea(fArea);
    }
  }
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::setFocusOnEntry(ConnectionEntry const* entry) {
  return setFocusOnArea(entry->bounds.center());
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
  clearFocusableAreas();
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
    updateFocusableAreas();
    set_layout_dirty(true);
  } else if (folder) {
    _active_folder = folder;
    // Drilling into a folder.
    _filtered = false;
    _search_text.set_value("");
    updateFocusableAreas();
    set_layout_dirty(true);
  }
}

//--------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_down(mforms::MouseButton button, int x, int y) {
  mforms::DrawBox::mouse_down(button, x, y);
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

      if (_rescanButton.bounds.contains(x, y)) {
        _owner->trigger_callback(HomeScreenAction::RescanLocalServers, base::any());
        return true;
      }

      if (_hot_entry) {
        _hot_entry->activate();
        return true;
      }

      break;
    }

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

      break;
    }

    default:
      break;
  }

  return false;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_leave() {
  if (_hot_entry) {
    _hot_entry.reset();
    set_needs_repaint();
  }
  return false;
}

//------------------------------------------------------------------------------------------------

bool ConnectionsSection::mouse_move(mforms::MouseButton button, int x, int y) {

  std::shared_ptr<ConnectionEntry> entry = entry_from_point(x, y);

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
      if (entry != _hot_entry) {
        _hot_entry = entry;
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
    ConnectionVector const& items(displayed_connections());

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

std::string ConnectionsSection::getAccessibilityTitle() {
  return "Home Screen Connections List";
}

//------------------------------------------------------------------------------------------------

size_t ConnectionsSection::getAccessibilityChildCount() {
  size_t ret_val = 2; // 2 for create + manage icons.

  if (_filtered)
    ret_val += _filtered_connections.size();
  else if (!_active_folder)
    ret_val += _connections.size();
  else
    ret_val += _active_folder->children.size();

  return ret_val;
}

//------------------------------------------------------------------------------------------------

base::Accessible* ConnectionsSection::getAccessibilityChild(size_t index) {
  base::Accessible* accessible = nullptr;

  switch (index) {
    case 0:
      accessible = &_add_button;
      break;
    case 1:
      accessible = &_manage_button;
      break;
    default: {
      index -= 2; // Offset the index for the buttons handled above.

      if (_filtered) {
        if (index < _filtered_connections.size())
          accessible = _filtered_connections[index].get();
      } else {
        if (!_active_folder) {
          if (index < _connections.size())
            accessible = _connections[index].get();
        } else {
          if (index < _active_folder->children.size())
            accessible = _active_folder->children[index].get();
        }
      }
    }
  }

  return accessible;
}

//------------------------------------------------------------------------------------------------

base::Accessible::Role ConnectionsSection::getAccessibilityRole() {
  return Accessible::List;
}

//------------------------------------------------------------------------------------------------

base::Accessible* ConnectionsSection::accessibilityHitTest(ssize_t x, ssize_t y) {
  base::Accessible* accessible = nullptr;

  if (_add_button.bounds.contains(static_cast<double>(x), static_cast<double>(y)))
    accessible = &_add_button;
  else if (_manage_button.bounds.contains(static_cast<double>(x), static_cast<double>(y)))
    accessible = &_manage_button;
  else {
    std::shared_ptr<ConnectionEntry> entry = entry_from_point(static_cast<int>(x), static_cast<int>(y));

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
    std::shared_ptr<ConnectionEntry> entry = entry_from_point((int)p.x, (int)p.y);

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

//----------------------------------------------------------------------------------------------------------------------

mforms::DragOperation ConnectionsSection::files_dropped(View *sender, base::Point p,
                                                        mforms::DragOperation allowedOperations,
                                                        const std::vector<std::string> &file_names) {
  std::shared_ptr<ConnectionEntry> entry = entry_from_point(static_cast<int>(p.x), static_cast<int>(p.y));
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

//----------------------------------------------------------------------------------------------------------------------

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

//----------------------------------------------------------------------------------------------------------------------

mforms::View *ConnectionsSection::getContainer() {
  if (_container == nullptr) {
    _container = mforms::manage(new mforms::Box(false));
    _container->set_name("Home Screen Content Host");
    _welcomeScreen = mforms::manage(new ConnectionsWelcomeScreen(_owner));
    if (!_showWelcomeHeading)
      _welcomeScreen->show(false);
    _welcomeScreen->set_name("Home Screen Welcome Page");
    _welcomeScreen->setInternalName("welcomeScreen");
    _welcomeScreen->set_layout_dirty(true);
    _container->add(_welcomeScreen, false, true);
    _container->add(this, true, true);
  }
  return _container;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::View *ConnectionsSection::get_parent() const {
  return _container->get_parent();
}

//----------------------------------------------------------------------------------------------------------------------

ConnectionsSection::ConnectionVector const& ConnectionsSection::displayed_connections() const {
  if (_filtered)
    return _filtered_connections;
  else if (_active_folder)
    return _active_folder->children;
  else
    return _connections;
}

//----------------------------------------------------------------------------------------------------------------------
