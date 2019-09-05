/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "accessible.h"

#include <gdkmm.h>
#include <cairomm/cairomm.h>
#include <cairomm/xlib_surface.h>
#include <glibmm/fileutils.h>
#include <gtkmm.h>
#include <X11/Xlib.h>
#include <gdk/gdkkeysyms.h>
#include <future>
#include <map>
#include <string>
#include <glib.h>
#include <glib-object.h>
#include "utilities.h"
#include <glib/gunicode.h>
#include "virtualkeygenerator.h"

#define ATSPI_ROLE_TABLE_COLUMN ATSPI_ROLE_LAST_DEFINED + 10

using namespace aal;


#define _HandleAtspiError(err, appMessage) \
  if (err != nullptr) \
    throw std::runtime_error(appMessage + ": " + err->message);

#define HandleAtspiError(err, appMessage) _HandleAtspiError(err, mga::Utilities::format("%s:%d[%s] %s", __FILE__, __LINE__, __PRETTY_FUNCTION__, appMessage))

std::map<int, Role> roleMap = {
     {0, Role::Unknown},
     {ATSPI_ROLE_APPLICATION, Role::Application},
     {ATSPI_ROLE_WINDOW, Role::Window},
     {ATSPI_ROLE_FILE_CHOOSER, Role::Window},
     {ATSPI_ROLE_FRAME, Role::Window },
     {ATSPI_ROLE_DIALOG, Role::Window },
     {ATSPI_ROLE_ALERT, Role::Window },
     {ATSPI_ROLE_PUSH_BUTTON, Role::Button},
     {ATSPI_ROLE_RADIO_BUTTON, Role::RadioButton},
     {ATSPI_ROLE_CHECK_BOX, Role::CheckBox},
     {ATSPI_ROLE_LABEL, Role::Label},
     {ATSPI_ROLE_STATUS_BAR, Role::Label},
     {ATSPI_ROLE_MENU, Role::Menu},
     {ATSPI_ROLE_MENU_BAR, Role::MenuBar},
     {ATSPI_ROLE_MENU_ITEM, Role::MenuItem},
     {ATSPI_ROLE_PAGE_TAB_LIST, Role::TabView},
     {ATSPI_ROLE_PAGE_TAB, Role::TabPage},
     {ATSPI_ROLE_COMBO_BOX, Role::ComboBox},
     {ATSPI_ROLE_TEXT, Role::TextBox},
     {ATSPI_ROLE_TERMINAL, Role::TextBox},
     {ATSPI_ROLE_PASSWORD_TEXT, Role::TextBox},
     {ATSPI_ROLE_CALENDAR, Role::DatePicker},
     {ATSPI_ROLE_ICON, Role::Image},
     {ATSPI_ROLE_TREE_TABLE, Role::TreeView},
     {ATSPI_ROLE_TREE, Role::TreeView},
     {ATSPI_ROLE_SLIDER, Role::Slider},
     {ATSPI_ROLE_PROGRESS_BAR, Role::ProgressIndicator},
     {ATSPI_ROLE_FILLER, Role::Pane},
     {ATSPI_ROLE_PANEL, Role::Pane},
     {ATSPI_ROLE_SCROLL_PANE, Role::ScrollBox},
     {ATSPI_ROLE_SCROLL_BAR, Role::ScrollBar},
     {ATSPI_ROLE_ANIMATION, Role::BusyIndicator},
     {ATSPI_ROLE_SPIN_BUTTON, Role::Stepper},
     {ATSPI_ROLE_SPLIT_PANE, Role::SplitContainer},
     {ATSPI_ROLE_IMAGE, Role::Image},
     {ATSPI_ROLE_CANVAS, Role::Image},
     {ATSPI_ROLE_TABLE_ROW, Role::Row},
     {ATSPI_ROLE_TABLE_COLUMN, Role::Column},
     {ATSPI_ROLE_LIST_BOX, Role::List},
     {ATSPI_ROLE_LIST, Role::List},
     {ATSPI_ROLE_LAYERED_PANE, Role::IconView},
     {ATSPI_ROLE_SEPARATOR, Role::Separator},
     {ATSPI_ROLE_TABLE_CELL, Role::Cell},
};

static std::map<AtspiAccessible *, std::vector<std::string>> InterfaceMap;
static std::map<AtspiAccessible *, std::vector<std::string>> ActionsMap;

//----------------------------------------------------------------------------------------------------------------------

static std::string convertAndFreeString(gchar *txt) {
  std::string ret(txt);
  g_free(txt);
  return ret;
}

//----------------------------------------------------------------------------------------------------------------------

Accessible::Accessible() : _accessible(nullptr), _isRoot(false), _pid(0), _role(ATSPI_ROLE_UNKNOWN) {
}

//----------------------------------------------------------------------------------------------------------------------

Accessible::Accessible(AtspiAccessible *accessible, bool isRoot) : _accessible(accessible), _isRoot(isRoot), _pid(0) {
  _role = getPlatformRole(accessible);
}

//----------------------------------------------------------------------------------------------------------------------

Accessible::Accessible(AtspiAccessible *accessible, AtspiRole forceRole) : _accessible(accessible), _isRoot(false), _pid(0), _role(forceRole) {
}


//----------------------------------------------------------------------------------------------------------------------

Accessible::~Accessible(){
  removeHighlight();

  ActionsMap.erase(_accessible);
  InterfaceMap.erase(_accessible);
  
  if (_accessible != nullptr)
    g_object_unref(_accessible);
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::clone() const {
  g_object_ref(_accessible); //It will be freed in the Accessible d-tor.
  return AccessibleRef(new Accessible(_accessible, _role == ATSPI_ROLE_APPLICATION));
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::gatherAvailableActions() const {
  AtspiAction *actionIface = getInterfaceAction();
  if (actionIface == nullptr) 
    return;
  
  if (ActionsMap.find(_accessible) != ActionsMap.end())
    return;

  GError *error = nullptr;
  auto nActions = atspi_action_get_n_actions(actionIface, &error);

  if (error != nullptr)
    return;

  auto &actions = ActionsMap[_accessible];
  
  for(auto i = 0; i < nActions; ++i)
    actions.push_back(convertAndFreeString(atspi_action_get_action_name(actionIface, i, &error)));
}

//----------------------------------------------------------------------------------------------------------------------

int Accessible::getActionIndex(const std::string &action) const {
  gatherAvailableActions();

  auto &actions = ActionsMap[_accessible];
//   if (actionsIter == ActionsMap.end()) {
//     
//     actionsIter = ActionsMap.find(_accessible);
//   }
  
  for (size_t index = 0; index < actions.size(); ++index) {
    if (actions[index].find(action) != std::string::npos)
      return (int)index;
  }
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::triggerAction(const std::string &action) const {
  AtspiAction *actionIface = getInterfaceAction();
  if (actionIface == nullptr)
    throw std::runtime_error("This object does not support actions.");
  
  int index = getActionIndex(action);
  
  if (index == -1)
    throw std::runtime_error("Action not supported for this object.");
  
  GError *error = nullptr;
  if (!atspi_action_do_action(actionIface, index, &error))
    HandleAtspiError(error, "trigger action");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::gatherAvailableInterfaces() const {
  if (InterfaceMap.find(_accessible) != InterfaceMap.end())
    return;
  
  GArray *ifaceList = atspi_accessible_get_interfaces(_accessible);
  for(guint index = 0; index < ifaceList->len; ++index) {
    std::string iFace = convertAndFreeString(g_array_index(ifaceList, char*, index));
    auto &ifaces = InterfaceMap[_accessible];
    ifaces.emplace_back(iFace);

    if (getState(ATSPI_STATE_EDITABLE) && iFace == "Text")
      ifaces.emplace_back(iFace);
  }
  g_array_free(ifaceList, true);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::interfaceAvailable(const std::string &interaface) const {
  gatherAvailableInterfaces();
  auto &ifaces = InterfaceMap[_accessible];
  
  for (size_t index = 0; index < ifaces.size(); ++index) {
    if (ifaces[index].find(interaface) != std::string::npos)
      return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

AtspiAction *Accessible::getInterfaceAction(AtspiAccessible *acc) const {
  if (acc)
    return Accessible(acc, false).getInterfaceAction();

  if (!interfaceAvailable("Action"))
    return nullptr;

  return atspi_accessible_get_action_iface(_accessible);
}

//----------------------------------------------------------------------------------------------------------------------

AtspiText *Accessible::getInterfaceText(AtspiAccessible *acc) const {
  if (acc)
    return Accessible(acc, false).getInterfaceText();

  if (!interfaceAvailable("Text"))
    return nullptr;

  return atspi_accessible_get_text_iface(_accessible);
}

//----------------------------------------------------------------------------------------------------------------------

AtspiEditableText *Accessible::getInterfaceEditableText(AtspiAccessible *acc) const {
  if (acc)
    return Accessible(acc, false).getInterfaceEditableText();

  if (!interfaceAvailable("Editable"))
    return nullptr;

  return atspi_accessible_get_editable_text(_accessible);
}

//----------------------------------------------------------------------------------------------------------------------

AtspiTable *Accessible::getInterfaceTable(AtspiAccessible *acc) const {
  if (acc)
    return Accessible(acc, false).getInterfaceTable();

  if (!interfaceAvailable("Table"))
    return nullptr;

  return atspi_accessible_get_table_iface(_accessible);
}

AtspiSelection *Accessible::getInterfaceSelection(AtspiAccessible *acc) const {
  if (acc)
    return Accessible(acc, false).getInterfaceSelection();

  if (!interfaceAvailable("Selection"))
    return nullptr;

  return atspi_accessible_get_selection_iface(_accessible);
}

//----------------------------------------------------------------------------------------------------------------------

int Accessible::getIndex() const {
  GError *error = nullptr;
  AtspiAccessible *parent = atspi_accessible_get_parent(_accessible, &error);
  AtspiTable *parentTable = atspi_accessible_get_table_iface(parent);
  int colCount = atspi_table_get_n_columns(parentTable, &error);
  int rowCount = atspi_table_get_n_rows(parentTable, &error);
  
  for (int row = 0; row < rowCount; ++row) {
    for (int col = 0; col < colCount; ++col) {
      AtspiAccessible *cell = atspi_table_get_accessible_at(parentTable, row, col, &error);
      if (cell == _accessible)
        return row * colCount + col;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::children(AccessibleList &result, bool recursive) const {
  if (_role == ATSPI_ROLE_TABLE_ROW) {
    for (auto &child : rowEntries())
      result.emplace_back(std::move(child));
    return;
  }
    

  if (_role == static_cast<AtspiRole>(ATSPI_ROLE_TABLE_COLUMN)) {
    for (auto &child : columnEntries())
      result.emplace_back(std::move(child));
    return;
  }
  
  GError *error = nullptr;
  gint count = atspi_accessible_get_child_count(_accessible, &error);
  HandleAtspiError(error, std::string("child count of ") + getName());
  for (gint index = 0; index < count; ++index) {
    AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
    HandleAtspiError(error, mga::Utilities::format("child of %s at index %d", getName(), index));
    if (!isVisible(child))
      continue;

    Accessible *childAcc = new Accessible(child, false);
    result.emplace_back(childAcc);

    if (recursive)
      childAcc->children(result, true);
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::children() const {
  if (_role == ATSPI_ROLE_TABLE_ROW)
    return rowEntries();

  if (_role == static_cast<AtspiRole>(ATSPI_ROLE_TABLE_COLUMN))
    return columnEntries();
  
  AccessibleList result;
  GError *error = nullptr;
  gint count = atspi_accessible_get_child_count(_accessible, &error);
  HandleAtspiError(error, "child count");
  for (gint index = 0; index < count; ++index) {
    AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
    HandleAtspiError(error, "child at index");
    if (isVisible(child) || getPlatformRole(child) == ATSPI_ROLE_APPLICATION)
      result.emplace_back(new Accessible(child, getPlatformRole(child) == ATSPI_ROLE_APPLICATION));
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::windows() const {
  if (_isRoot)
    return children();
  return AccessibleList();
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::tabPages() const {
  if (getRole() != Role::TabView)
    throw std::runtime_error("Can't get tab pages from " + getPlatformRoleName());
  
  AccessibleList result;

  for (auto &child : children()) {
    if (child->getRole() == Role::TabPage)
      result.push_back(std::move(child));
  }
  
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::rows() const {
  AtspiTable *tableIface = atspi_accessible_get_table_iface(_accessible);
  
  if (tableIface == nullptr)
    throw std::runtime_error("This element has no rows.");

  AccessibleList result;
  GError *error = nullptr;
  gint rowCount = atspi_table_get_n_rows(tableIface, &error);
  
  for (int rowIndex = 0; rowIndex < rowCount; ++rowIndex) {
    AtspiAccessible *cell = atspi_table_get_accessible_at(tableIface, rowIndex, 0, &error);
    result.emplace_back(new Accessible(cell, ATSPI_ROLE_TABLE_ROW));
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::rowEntries() const {
  if (_role != ATSPI_ROLE_TABLE_ROW)
    throw std::runtime_error("This element has no entries.");

  AccessibleList result;
  GError *error = nullptr;
  AtspiAccessible *parent = atspi_accessible_get_parent(_accessible, &error);
  AtspiTable *parentTable = atspi_accessible_get_table_iface(parent);
  int index = getIndex();
  gint colCount = atspi_table_get_n_columns(parentTable, &error);
  int row = index / colCount;
  
  for (int colIndex = 0; colIndex < colCount; ++colIndex) {
    AtspiAccessible *cell = atspi_table_get_accessible_at(parentTable, row, colIndex, &error);
    result.emplace_back(new Accessible(cell, ATSPI_ROLE_TABLE_CELL));
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::columns() const {
  AtspiTable *tableIface = atspi_accessible_get_table_iface(_accessible);

  AccessibleList result;
  GError *error = nullptr;
  int colCount = atspi_table_get_n_columns(tableIface, &error);
  
  for (int colIndex = 0; colIndex < colCount; ++colIndex) {
    AtspiAccessible *cell = atspi_table_get_accessible_at(tableIface, 0, colIndex, &error);
    result.emplace_back(new Accessible(cell, static_cast<AtspiRole>(ATSPI_ROLE_TABLE_COLUMN)));
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::columnEntries() const {
  AccessibleList result;
  GError *error = nullptr;

  AtspiAccessible *parent = atspi_accessible_get_parent(_accessible, &error);
  AtspiTable *parentTable = atspi_accessible_get_table_iface(parent);
  int rowCount = atspi_table_get_n_rows(parentTable, &error);

  int index = getIndex();
  gint col = atspi_table_get_column_at_index(parentTable, index, &error);
  
  for (int row = 0; row < rowCount; ++row) {
    AtspiAccessible *cell = atspi_table_get_accessible_at(parentTable, row, col, &error);
    result.emplace_back(new Accessible(cell, static_cast<AtspiRole>(ATSPI_ROLE_TABLE_CELL)));
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static AtspiAccessible* getAccFromPoint(geometry::Point p, AtspiAccessible* acc) {

  GError *error = nullptr;

  gint count = atspi_accessible_get_child_count(acc, &error);
  HandleAtspiError(error, "child count");
  auto componentIface = atspi_accessible_get_component(acc);
  // First try to get this using component iface
  // if that won't be possible, use normal iteration
  if (componentIface != nullptr) {
    AtspiAccessible *accessible = atspi_component_get_accessible_at_point(componentIface, p.x, p.y,
                                                                          ATSPI_COORD_TYPE_SCREEN, &error);
    HandleAtspiError(error, "accessible at point");
    if (accessible != nullptr) {
      auto childAcc = getAccFromPoint(p, accessible);
      if (childAcc != nullptr)
        return childAcc;
    } else if (atspi_component_contains(componentIface, p.x, p.y, ATSPI_COORD_TYPE_SCREEN, &error) && count == 0) {
      HandleAtspiError(error, "component contains");
      return acc;
    }
  }


  if (count == 0)
     return nullptr;

  for (gint index = 0; index < count; ++index) {
    AtspiAccessible *child = atspi_accessible_get_child_at_index(acc, index, &error);
    HandleAtspiError(error, "child at index");
    if (child) {
      auto componentIface = atspi_accessible_get_component(child);
      if (componentIface == nullptr)
        continue;


      AtspiRole result = atspi_accessible_get_role(child, &error);
      HandleAtspiError(error, "get role");
      if (result == ATSPI_ROLE_PAGE_TAB_LIST)
        return getAccFromPoint(p, child);


      AtspiAccessible *accessible = atspi_component_get_accessible_at_point(componentIface, p.x, p.y, ATSPI_COORD_TYPE_SCREEN, &error);
      HandleAtspiError(error, "accessibl at point");
      if (accessible == nullptr) {
        if (atspi_component_contains(componentIface,  p.x, p.y, ATSPI_COORD_TYPE_SCREEN, &error)) {
          HandleAtspiError(error, "component contains");
          return child;
        }
      }
      else
        return getAccFromPoint(p, accessible);
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::fromPoint(geometry::Point point, Accessible *application) {
  GError *error = nullptr;

  AtspiComponent *componentInterface = atspi_accessible_get_component(application->_accessible);
  if (componentInterface != nullptr) {
    AtspiAccessible *accessible = atspi_component_get_accessible_at_point(componentInterface, point.x, point.y, ATSPI_COORD_TYPE_SCREEN, &error);
    HandleAtspiError(error, "accessible at point");

    AccessibleRef accRef(new Accessible(accessible, atspi_accessible_get_role(accessible, &error) == ATSPI_ROLE_APPLICATION));
    HandleAtspiError(error, "get role");
    return accRef;
  } else {

    // If there's no component iface, we need to get the app and get it's first descendant,
    // this should be the root component.
    auto app = atspi_accessible_get_application(application->_accessible, &error);
    HandleAtspiError(error, "get application");
    auto acc = getAccFromPoint(point, app);
    if (acc != nullptr)
      return AccessibleRef(new Accessible(acc, false));
    else
      return AccessibleRef();
  }
}

//----------------------------------------------------------------------------------------------------------------------

AtspiRole Accessible::getPlatformRole(AtspiAccessible *acc) {
  GError *error = nullptr;
  AtspiRole result = atspi_accessible_get_role(acc, &error);
  HandleAtspiError(error, "get role");
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getPlatformRoleName() const {
  GError *error = nullptr;
  auto result = convertAndFreeString(atspi_accessible_get_role_name(_accessible, &error));
  HandleAtspiError(error, "get role name");
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::printNativeInfo() const {
  std::vector<std::string> parents;
  auto run = getParent();
  while (run != nullptr && run->isValid()) {
    parents.insert(parents.begin(), run->getPlatformRoleName() + " :: " + run->getName());
    run = run->getParent();
  }

  int i = 0;
  for (auto &entry : parents) {
    for(auto j = 0; j < i; ++j)
      std::cout << "*";
    i++;
    std::cout << " " << entry << std::endl;

  }

  std::cout << "Name: " << getName() << std::endl;
  std::cout << "Description: " << getDescription() << std::endl;
  std::cout << "PlatformRole: " << getPlatformRoleName() << std::endl;
  std::cout << "Implement interfaces: ";
  for(const auto &iface: InterfaceMap[_accessible])
    std::cout << iface << ", ";

  AtspiAction *actionIface = getInterfaceAction();
  if (actionIface != nullptr) {
    gatherAvailableActions();
    std::cout << "Actions: ";
    for (auto action : ActionsMap[_accessible])
      std::cout << action << ", ";
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::getState(AtspiStateType state, AtspiAccessible *acc) const {
  AtspiStateSet *stateSet = atspi_accessible_get_state_set(acc == nullptr ? _accessible : acc);
  return atspi_state_set_contains(stateSet, state);
}

//----------------------------------------------------------------------------------------------------------------------

// void HandleAtspiError(GError *err, const std::string &attr) {
//   if (err != nullptr)
//     throw std::runtime_error("Unable to access attribute: ["+attr+"] error:" + std::string(err->message));
// }

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::canFocus() const {
  return getState(ATSPI_STATE_FOCUSABLE);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isFocused() const {
  GError *error = nullptr;
  AtspiRole role = _role;
  if (role == ATSPI_ROLE_COMBO_BOX && !getState(ATSPI_STATE_FOCUSED)) {
    gint count = atspi_accessible_get_child_count(_accessible, &error);
    HandleAtspiError(error, "child count");
    for (gint index = 0; index < count; ++index) {
        AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
        HandleAtspiError(error, "get child at index");
        if (child) {
          if (getState(ATSPI_STATE_FOCUSED, child))
            return true;
        }
    }
  }

  return getState(ATSPI_STATE_FOCUSED);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setFocused() {
  AtspiComponent *componentInterface = atspi_accessible_get_component(_accessible);
  if (componentInterface != nullptr) {
    GError *error = nullptr;
    atspi_component_grab_focus(componentInterface, &error);
    HandleAtspiError(error, "grab focus");
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // before continuing we have to give it a little bit of time to realy do the focus
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getID() const {
  return "Not supported on this platform";
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getName() const {
  GError *error = nullptr;
  auto result = convertAndFreeString(atspi_accessible_get_name(_accessible, &error));
  HandleAtspiError(error, "get name");
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getHelp() const {
  return "Not supported on this platform";
}

//----------------------------------------------------------------------------------------------------------------------

aal::Role Accessible::getRole() const {
  if (_role == ATSPI_ROLE_TOGGLE_BUTTON && isExpandable())
    return Role::Expander;

  auto it = roleMap.find(_role);
  if (it != roleMap.end()) {
    return it->second;
  }
  return Role::Unknown;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isInternal() const {
  // Tells if the element is internal one for example edit of combobox
  // on GTK we can't check this
  return false;  
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::equals(Accessible *other) const {
  return other->_accessible == _accessible;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Show the element's context menu, if supported
 */

static glong getKeyCodeFromKeyval(glong keyval);

void Accessible::showMenu() {
  setFocused();
  GError *error = nullptr;
  atspi_generate_keyboard_event(getKeyCodeFromKeyval(GDK_KEY_Menu), nullptr, ATSPI_KEY_PRESSRELEASE, &error);
  HandleAtspiError(error, "context menu");
}


//----------------------------------------------------------------------------------------------------------------------

bool Accessible::menuShown() const {
    if (_role != ATSPI_ROLE_MENU)
      throw std::runtime_error("Shown attribute only valid for menus.");

    AccessibleList accessibles;
    children(accessibles, true);
    return accessibles.size() > 0;
}

//----------------------------------------------------------------------------------------------------------------------

aal::AccessibleRef Accessible::getParent() const {
  GError *error = nullptr;
  AtspiAccessible *result = atspi_accessible_get_parent(_accessible, &error);
  HandleAtspiError(error, "get parent");
  AccessibleRef accRef(new Accessible(result, getPlatformRole(result) == ATSPI_ROLE_APPLICATION));
  //HandleAtspiError(error, "parent");
  return accRef;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getContainingRow() const {
  GError *error = nullptr;
  AtspiAccessible *parent = atspi_accessible_get_parent(_accessible, &error);
  AtspiTable * parentTable = atspi_accessible_get_table_iface(parent);
  int index = getIndex();
  gint colCount = atspi_table_get_n_columns(parentTable, &error);
  int row = index / colCount;

  AtspiAccessible *cell = atspi_table_get_accessible_at(parentTable, row, 0, &error);
  return AccessibleRef(new Accessible(cell, ATSPI_ROLE_TABLE_ROW));
}

//----------------------------------------------------------------------------------------------------------------------

// This is a hack because of bug: https://bugzilla.gnome.org/show_bug.cgi?id=790009
// We assume that the scrollbars assigned to the scrollpane will be in the "first children layer"
// if not then sorry.

AccessibleRef Accessible::getHorizontalScrollBar() const {
  if (_role == ATSPI_ROLE_SCROLL_PANE) {
    GError *error = nullptr;
    gint count = atspi_accessible_get_child_count(_accessible, &error);
    HandleAtspiError(error, "get child count");
    for (gint index = 0; index < count; ++index) {
      AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
      if (getPlatformRole(child) == ATSPI_ROLE_SCROLL_BAR && getState(ATSPI_STATE_HORIZONTAL, child)) {
        return AccessibleRef(new Accessible(child, getPlatformRole(child) == ATSPI_ROLE_APPLICATION));
      }
    }
  }
  return AccessibleRef();
}

//----------------------------------------------------------------------------------------------------------------------

// This is a hack because of bug: https://bugzilla.gnome.org/show_bug.cgi?id=790009
// We assume that the scrollbars assigned to the scrollpane will be in the "first children layer"
// if not then sorry.

AccessibleRef Accessible::getVerticalScrollBar() const {
  if (_role == ATSPI_ROLE_SCROLL_PANE) {
    GError *error = nullptr;
    gint count = atspi_accessible_get_child_count(_accessible, &error);
    HandleAtspiError(error, "get child count");
    for (gint index = 0; index < count; ++index) {
      AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
      if (getPlatformRole(child) == ATSPI_ROLE_SCROLL_BAR && getState(ATSPI_STATE_VERTICAL, child)) {
        return AccessibleRef(new Accessible(child, getPlatformRole(child) == ATSPI_ROLE_APPLICATION));
      }
    }
  }
  return AccessibleRef();
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getHeader() const {
  if (_role != ATSPI_ROLE_TABLE_ROW && _role != ATSPI_ROLE_TABLE_COLUMN)
    throw std::runtime_error("This element does not have a heading.");

  GError *error = nullptr;
  int index = getIndex();
  AtspiAccessible *parent = atspi_accessible_get_parent(_accessible, &error);
  AtspiTable *parentTable = atspi_accessible_get_table_iface(parent);
  
  AtspiAccessible *header = nullptr;
  
  if (_role == ATSPI_ROLE_TABLE_ROW) {
    gint colCount = atspi_table_get_n_columns(parentTable, &error);
    int row = index / colCount;
    header = atspi_table_get_row_header(parentTable, row, &error);
  } else if (_role == static_cast<AtspiRole>(ATSPI_ROLE_TABLE_COLUMN)) {
    int col = atspi_table_get_column_at_index(parentTable, index, &error);
    header = atspi_table_get_column_header(parentTable, col, &error);
  } else if (_role == ATSPI_ROLE_TABLE_CELL){
    int col = atspi_table_get_column_at_index(parentTable, index, &error);
    header = atspi_table_get_column_header(parentTable, col, &error);
  } else
    throw std::runtime_error("This object does not have a header.");
  
  return AccessibleRef(new Accessible(header, ATSPI_ROLE_TABLE_CELL));
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getCloseButton() const {
  NOT_IMPLEMENTED;
  return AccessibleRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isEnabled() const {
  return getState(ATSPI_STATE_ENABLED);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isVisible(AtspiAccessible *acc) const {
  return getState(ATSPI_STATE_VISIBLE, acc) && getState(ATSPI_STATE_SHOWING, acc);
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getIncrementValue() {
  auto vIface = atspi_accessible_get_value_iface(_accessible);
  if (vIface != nullptr) {
    GError *error = nullptr;
    auto cVal = atspi_value_get_minimum_increment(vIface, &error);
    HandleAtspiError(error, "get minimum increment");
    return cVal;
  }
  return 0.0;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::implementsValue() {
  auto vIface = atspi_accessible_get_value_iface(_accessible);
  return vIface != nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

CheckState Accessible::getCheckState() const {
  if (getState(ATSPI_STATE_CHECKED))
    return CheckState::Checked;

  if (getState(ATSPI_STATE_INDETERMINATE))
    return CheckState::Indeterminate;
  
  return CheckState::Unchecked;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setCheckState(CheckState state) {
  throw std::runtime_error("Attribute is read-only");
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getValue() const {
  auto vIface = atspi_accessible_get_value_iface(_accessible);
  if (vIface != nullptr) {
    GError *error = nullptr;
    auto cVal = atspi_value_get_current_value(vIface, &error);
    HandleAtspiError(error, "get current value");
    return cVal;
  }
  return 0.0;
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getMaxValue() const {
  auto vIface = atspi_accessible_get_value_iface(_accessible);
  if (vIface != nullptr) {
    GError *error = nullptr;
    auto cVal = atspi_value_get_maximum_value(vIface, &error);
    HandleAtspiError(error, "get maximum value");
    return cVal;
  }
  return 0.0;
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getMinValue() const {
  auto vIface = atspi_accessible_get_value_iface(_accessible);
  if (vIface != nullptr) {
    GError *error = nullptr;
    auto cVal = atspi_value_get_minimum_value(vIface, &error);
    HandleAtspiError(error, "get minimum value");
    return cVal;
  }
  return 0.0;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setValue(const double value) {
  if (!isEnabled())
    throw std::runtime_error("The widget isn't enabled");

  if (_role == ATSPI_ROLE_PROGRESS_BAR)
    throw std::runtime_error("The widget doesn't support this functionality");

  auto vIface = atspi_accessible_get_value_iface(_accessible);
  if (vIface != nullptr) {
    GError *error = nullptr;
    atspi_value_set_current_value(vIface, value, &error);
    HandleAtspiError(error, "set current value");
  }
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getRange() const {
  AtspiValue *valueIface = atspi_accessible_get_value_iface(_accessible);
  if (valueIface == nullptr) 
    throw std::runtime_error("The range attribute is only supported for scrollbars.");
  
  GError *error = nullptr;
  gdouble minimum = atspi_value_get_minimum_value(valueIface, &error);
  HandleAtspiError(error, "get minimum value");
  gdouble maximum = atspi_value_get_maximum_value(valueIface, &error);
  HandleAtspiError(error, "get maximum value");
  
  return maximum - minimum;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getActiveTabPage() const {
  AtspiSelection *selection = atspi_accessible_get_selection_iface(_accessible);
  if (selection != nullptr) {
    GError *error = nullptr;
    auto child = atspi_selection_get_selected_child(selection, 0, &error); //Tab view can have only one active page
    HandleAtspiError(error, "get selected child");
    if (child != nullptr) {
      auto result = convertAndFreeString(atspi_accessible_get_name(child, &error));
      HandleAtspiError(error, "get name");
      return result;
    }
  }

  throw std::runtime_error("There's no active tab page");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setActiveTabPage(std::string const& name) {
  AtspiSelection *selection = atspi_accessible_get_selection_iface(_accessible);
  if (selection != nullptr) {

    GError *error = nullptr;
    gint count = atspi_accessible_get_child_count(_accessible, &error);
    HandleAtspiError(error, "get child count");
    for (gint index = 0; index < count; ++index) {
      AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
      HandleAtspiError(error, "get child at index");

      auto result = convertAndFreeString(atspi_accessible_get_name(child, &error));
      HandleAtspiError(error, "get name");
      if (result == name) {
        atspi_selection_select_child(selection, index, &error);
        HandleAtspiError(error, "select child");
        break;
      }
    }

  } else {
    throw std::runtime_error("There's nothing to activate");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::activate() {
  if (_role == ATSPI_ROLE_PAGE_TAB) {
     getParent()->setActiveTabPage(getName());
  } else if (_role == ATSPI_ROLE_MENU_ITEM || _role == ATSPI_ROLE_MENU) {
    click();
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isActiveTab() const {
  if (_role == ATSPI_ROLE_PAGE_TAB) {
    auto acc = getParent()->_accessible;
    AtspiSelection *selection = atspi_accessible_get_selection_iface(acc);
    if (selection != nullptr) {
      GError *error = nullptr;
      auto selected = atspi_selection_get_selected_child(selection, 0, &error); // There's always only one selected tab.
      auto result = convertAndFreeString(atspi_accessible_get_name(selected, &error));
      return result == getName();
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSelected() const {
  if (!getState(ATSPI_STATE_SELECTABLE))
    throw std::runtime_error("This element cannot be selected.");

  return getState(ATSPI_STATE_SELECTED);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelected(bool value) {
  if (!getState(ATSPI_STATE_SELECTABLE))
    throw std::runtime_error("This element cannot be selected.");

  if (value) {
    auto component = atspi_accessible_get_component_iface(_accessible);
    if (component != nullptr) {
      GError *error = nullptr;
      atspi_component_grab_focus(component, &error);
      HandleAtspiError(error, "grab focus");
    }
  } else {
    throw std::runtime_error("This element doesn't support unselection.");
  }
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getScrollPosition() const {
  if (_role != ATSPI_ROLE_SCROLL_BAR) {
    throw std::runtime_error("This is not a scrollbar.");
  }

  return getValue();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setScrollPosition(double value) {
  if (_role != ATSPI_ROLE_SCROLL_BAR) {
    throw std::runtime_error("This is not a scrollbar.");
  }

  setValue(value);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getTitle() const {
  throw std::runtime_error("This property is not supported on this platform");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setTitle(const std::string &title) {
  throw std::runtime_error("This property is not supported on this platform");
}

//----------------------------------------------------------------------------------------------------------------------
std::string Accessible::getDescription() const {
  GError *error = nullptr;
  auto result = convertAndFreeString(atspi_accessible_get_description(_accessible, &error));
  HandleAtspiError(error, "get description");
  return result;
}


std::string Accessible::getText() const {
  GError *error = nullptr;
  AtspiText *textIface = getInterfaceText();
  
  if (_role == ATSPI_ROLE_COMBO_BOX) {
    //  Children can be (1)menu (always) and a (2)text box (when editable)
    if (atspi_accessible_get_child_count(_accessible, &error) == 1) {
      auto selected = getSelectedIndexes();
      AtspiAccessible *menu = atspi_accessible_get_child_at_index(_accessible, 0, &error);
      AtspiAccessible *item = atspi_accessible_get_child_at_index(menu, *selected.begin(), &error);
      std::string result = convertAndFreeString(atspi_accessible_get_name(item, &error));
      return result;
    }
    AtspiAccessible *textChild = atspi_accessible_get_child_at_index(_accessible, 1, &error);
    textIface = atspi_accessible_get_text(textChild);
  }
  
  if (textIface == nullptr) {
    std::cerr << "This element does not provide Text Interface" << std::endl;
    return "";
  }

  int cCount = atspi_text_get_character_count(textIface, &error);
  HandleAtspiError(error, "get character count");
  auto result = convertAndFreeString(atspi_text_get_text(textIface, 0, cCount, &error));
  HandleAtspiError(error, "get text");
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t Accessible::getCaretPosition() const {
  AtspiText *textIface = getInterfaceText();
  if (textIface == nullptr)
    return 0;

  GError *error = nullptr;
  auto result = atspi_text_get_caret_offset(textIface, &error);
  HandleAtspiError(error, "get carret offset");
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setCaretPosition(size_t position) {
  AtspiText *textIface = getInterfaceText();
  if (textIface == nullptr)
    return;

  GError *error = nullptr;
  atspi_text_set_caret_offset(textIface, position, &error);
  HandleAtspiError(error, "set carret offset");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::insertText(size_t offset, const std::string &text) {
  AtspiEditableText *editableTextIface = getInterfaceEditableText();
  if (editableTextIface == nullptr)
    return;

  GError *error = nullptr;
  atspi_editable_text_insert_text(editableTextIface, offset, text.c_str(), text.size(), &error);
  HandleAtspiError(error, "insert text");

}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getText(size_t offset, size_t len) {
  AtspiText *textIface = getInterfaceText();
  if (textIface == nullptr)
    return "";

  GError *error = nullptr;
  auto result = convertAndFreeString(atspi_text_get_text(textIface, offset, len, &error));
  HandleAtspiError(error, "get text");
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setText(const std::string &text) {
  AtspiEditableText *editableTextIface = getInterfaceEditableText();
  
  GError *error = nullptr;

  if (_role == ATSPI_ROLE_COMBO_BOX) {
    //  Children can be (1)menu (always) and a (2)text box (when editable)
    if (atspi_accessible_get_child_count(_accessible, &error) == 1)
      throw std::runtime_error("Combobox not editable");
    
    AtspiAccessible *menu = atspi_accessible_get_child_at_index(_accessible, 0, &error);
    AtspiAccessible *textChild = atspi_accessible_get_child_at_index(_accessible, 1, &error);
    editableTextIface = getInterfaceEditableText(textChild);
    
    //  Make the selection combining to the text
    AtspiSelection *selection = getInterfaceSelection(_accessible);
    int itemCount = atspi_accessible_get_child_count(menu, &error);
    
    atspi_selection_clear_selection(selection, &error);
    
    for (int index = 0; index < itemCount; ++index) {
      AtspiAccessible *item = atspi_accessible_get_child_at_index(menu, index, &error);
      if (convertAndFreeString(atspi_accessible_get_name(item, &error)) == text)
        atspi_selection_select_child(selection, index, &error);
    }
  }

  if (editableTextIface == nullptr)
    throw std::runtime_error("Cannot set text to this control.");

  atspi_editable_text_set_text_contents(editableTextIface, text.c_str(), &error);
  HandleAtspiError(error, "set text");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::click() {
  if (_role == ATSPI_ROLE_COMBO_BOX) {
    if (getActionIndex("press") > -1) {
      triggerAction("press");
      return;
    }
  }
  
  if (getActionIndex("click") == -1)
    throw std::runtime_error("This object does not support click.");

  auto bounds = getBounds(true);
  GError *error = nullptr;

  // Sometimes the mouse first has to be moved over the element, let's do this.
  atspi_generate_mouse_event(bounds.position.x + bounds.size.width / 2, bounds.position.y + bounds.size.height / 2, "abs", &error);
  HandleAtspiError(error, "mouseMove");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));

  atspi_generate_mouse_event(bounds.position.x + bounds.size.width / 2, bounds.position.y + bounds.size.height / 2, "b1c", &error);
  HandleAtspiError(error, "click");
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::confirm() {
  // Ignored.
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::stepUp() {
  if (implementsValue()) {
    double incVal = getIncrementValue();
    double curVal = getValue();
    setValue(curVal + incVal);
  } else {
    throw std::runtime_error("This element doesn't support this property.");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::stepDown() {
  if (implementsValue()) {
    double incVal = getIncrementValue();
    double curVal = getValue();
    setValue(curVal - incVal);
  } else {
    throw std::runtime_error("This element doesn't support this property.");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollLeft() {
  if (_role == ATSPI_ROLE_SCROLL_PANE) {
    getHorizontalScrollBar()->scrollLeft();
    return;
  }
  else if (_role != ATSPI_ROLE_SCROLL_BAR) {
    throw std::runtime_error("Unable to scroll element which isn't a scrollbar: "+getPlatformRoleName());
  }
  stepDown();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollRight() {
  if (_role == ATSPI_ROLE_SCROLL_PANE) {
    getHorizontalScrollBar()->scrollRight();
    return;
  }
  if (_role != ATSPI_ROLE_SCROLL_BAR) {
    throw std::runtime_error("Unable to scroll element which isn't a scrollbar: "+getPlatformRoleName());
  }
  stepUp();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollUp() {
  if (_role == ATSPI_ROLE_SCROLL_PANE) {
    getVerticalScrollBar()->scrollUp();
    return;
  }
  if (_role != ATSPI_ROLE_SCROLL_BAR) {
    throw std::runtime_error("Unable to scroll element which isn't a scrollbar: "+getPlatformRoleName());
  }
  stepUp();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollDown() {
  if (_role == ATSPI_ROLE_SCROLL_PANE) {
    getVerticalScrollBar()->scrollDown();
    return;
  }
  if (_role != ATSPI_ROLE_SCROLL_BAR) {
    throw std::runtime_error("Unable to scroll element which isn't a scrollbar: "+getPlatformRoleName());
  }
  stepDown();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::increment() {
  if (_role != ATSPI_ROLE_SLIDER) {
    throw std::runtime_error("Unable to increment element which isn't a slider: "+getPlatformRoleName());
  }

  stepUp();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::decrement() {
  if (_role != ATSPI_ROLE_SLIDER) {
    throw std::runtime_error("Unable to decrement element which isn't a slider: "+getPlatformRoleName());
  }

  stepDown();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseDown(const geometry::Point &pos, const MouseButton button) {
  GError *error = nullptr;
  std::string name;
  switch(button) {
    case MouseButton::NoButton:
      return;
    case MouseButton::Left:
      name = "b1p";
    break;
    case MouseButton::Right:
      name = "b3p";
    break;
    case MouseButton::Middle:
      name = "b2p";
    break;
  }

  atspi_generate_mouse_event(pos.x, pos.y, name.c_str(), &error);
  HandleAtspiError(error, "mouseDown");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseUp(const geometry::Point &pos, const MouseButton button) {
  GError *error = nullptr;
  std::string name;
  switch(button) {
    case MouseButton::NoButton:
      return;
    case MouseButton::Left:
      name = "b1r";
    break;
    case MouseButton::Right:
      name = "b3r";
    break;
    case MouseButton::Middle:
      name = "b2r";
    break;
  }

  atspi_generate_mouse_event(pos.x, pos.y, name.c_str(), &error);
  HandleAtspiError(error, "mouseUp");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseMove(const geometry::Point &pos) {
  GError *error = nullptr;

  atspi_generate_mouse_event(pos.x, pos.y, "rel", &error);

  HandleAtspiError(error, "mouseMove");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseMoveTo(const geometry::Point &pos) {
  GError *error = nullptr;

  atspi_generate_mouse_event(pos.x, pos.y, "abs", &error);

  HandleAtspiError(error, "mouseMove");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseDrag(geometry::Point source, geometry::Point target, MouseButton button) {
  NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Point Accessible::getMousePosition() const {
  Glib::RefPtr<Gdk::Window> window = Gdk::Window::get_default_root_window();
  int x = 0, y = 0;
  Gdk::ModifierType mask;
  window->get_pointer(x, y, mask);
  return geometry::Point(x, y);
}

static int getKeyVal(const aal::Key k) {
  switch(k) {
    case Key::KeyReturn:
      return GDK_KEY_Return;
    case Key::KeyBackspace:
      return GDK_KEY_BackSpace;
    case Key::KeyDelete:
      return GDK_KEY_Delete;
    case Key::KeyEnd:
      return GDK_KEY_End;
    case Key::KeyEscape:
      return GDK_KEY_Escape;
    case Key::KeyLeft:
      return GDK_KEY_Left;
    case Key::KeyRight:
      return GDK_KEY_Right;
    case Key::KeyUp:
      return GDK_KEY_Up;
    case Key::KeyDown:
      return GDK_KEY_Down;
    case Key::KeyPageDown:
      return GDK_KEY_Page_Down;
    case Key::KeyPageUp:
      return GDK_KEY_Page_Up;
    case Key::KeyHome:
      return GDK_KEY_Home;

    case Key::KeyF1:
      return GDK_KEY_F1;
    case Key::KeyF2:
      return GDK_KEY_F2;
    case Key::KeyF3:
      return GDK_KEY_F3;
    case Key::KeyF4:
      return GDK_KEY_F4;
    case Key::KeyF5:
      return GDK_KEY_F5;
    case Key::KeyF6:
      return GDK_KEY_F6;
    case Key::KeyF7:
      return GDK_KEY_F7;
    case Key::KeyF8:
      return GDK_KEY_F8;
    case Key::KeyF9:
      return GDK_KEY_F9;
    case Key::KeyF10:
      return GDK_KEY_F10;
    case Key::KeyF11:
      return GDK_KEY_F11;
    case Key::KeyF12:
      return GDK_KEY_F12;
    case Key::KeyTab:
      return GDK_KEY_Tab;
    case Key::KeyPlus:
      return GDK_KEY_plus;
    case Key::KeyMinus:
      return GDK_KEY_minus;

    case Key::Key1:
      return GDK_KEY_1;
    case Key::Key2:
      return GDK_KEY_2;
    case Key::Key3:
      return GDK_KEY_3;
    case Key::Key4:
      return GDK_KEY_4;
    case Key::Key5:
      return GDK_KEY_5;
    case Key::Key6:
      return GDK_KEY_6;
    case Key::Key7:
      return GDK_KEY_7;
    case Key::Key8:
      return GDK_KEY_8;
    case Key::Key9:
      return GDK_KEY_9;
    case Key::Key0:
      return GDK_KEY_0;

    case Key::KeySpace:
      return GDK_KEY_space;
    case Key::KeyDot:
      return GDK_KEY_period;
    case Key::KeyComma:
      return GDK_KEY_comma;
    case Key::KeyColon:
      return GDK_KEY_colon;
    case Key::KeySlash:
      return GDK_KEY_slash;
    case Key::KeyBackslash:
      return GDK_KEY_backslash;
    case Key::KeyBraceLeft:
      return GDK_KEY_braceleft;
    case Key::KeyBraceRight:
      return GDK_KEY_braceright;

    case Key::KeyA:
      return GDK_KEY_a;
    case Key::KeyB:
      return GDK_KEY_b;
    case Key::KeyC:
      return GDK_KEY_c;
    case Key::KeyD:
      return GDK_KEY_d;
    case Key::KeyE:
      return GDK_KEY_e;
    case Key::KeyF:
      return GDK_KEY_f;
    case Key::KeyG:
      return GDK_KEY_g;
    case Key::KeyH:
      return GDK_KEY_h;
    case Key::KeyI:
      return GDK_KEY_i;
    case Key::KeyJ:
      return GDK_KEY_j;
    case Key::KeyK:
      return GDK_KEY_k;
    case Key::KeyL:
      return GDK_KEY_l;
    case Key::KeyM:
      return GDK_KEY_m;
    case Key::KeyN:
      return GDK_KEY_n;
    case Key::KeyO:
      return GDK_KEY_o;
    case Key::KeyP:
      return GDK_KEY_p;
    case Key::KeyQ:
      return GDK_KEY_q;
    case Key::KeyR:
      return GDK_KEY_r;
    case Key::KeyS:
      return GDK_KEY_s;
    case Key::KeyT:
      return GDK_KEY_t;
    case Key::KeyU:
      return GDK_KEY_u;
    case Key::KeyV:
      return GDK_KEY_v;
    case Key::KeyW:
      return GDK_KEY_w;
    case Key::KeyX:
      return GDK_KEY_x;
    case Key::KeyY:
      return GDK_KEY_y;
    case Key::KeyZ:
      return GDK_KEY_z;

    default:
      return 0;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static glong getKeyCodeFromKeyval(glong keyval) {
  GdkKeymapKey* keys;
  gint nKeys;

  if (gdk_keymap_get_entries_for_keyval(gdk_keymap_get_for_display(gdk_display_get_default()), keyval, &keys, &nKeys)) {
    glong keycode = keys[0].keycode;
    g_free(keys);
    return keycode;
  }
  return 0;
}

static void modifierKey(aal::Modifier mod, bool press = true) {
  if (containsModifier(mod, aal::Modifier::NoModifier))
    return;

  std::vector<std::pair<aal::Modifier, int>> modMap = {
      {aal::Modifier::ShiftLeft, GDK_KEY_Shift_L},
      {aal::Modifier::ShiftRight, GDK_KEY_Shift_R},
      {aal::Modifier::ControlLeft, GDK_KEY_Control_L},
      {aal::Modifier::ControlRight, GDK_KEY_Control_R},
      {aal::Modifier::AltLeft, GDK_KEY_Alt_L},
      {aal::Modifier::AltRight, GDK_KEY_Alt_R},
      {aal::Modifier::MetaLeft, GDK_KEY_Meta_L},
      {aal::Modifier::MetaRight, GDK_KEY_Meta_R},
  };

  AtspiKeySynthType sType = press ? ATSPI_KEY_PRESS : ATSPI_KEY_RELEASE;

  GError *error = nullptr;
  for(const auto it: modMap) {
    if (containsModifier(mod, it.first)) {
      if (!atspi_generate_keyboard_event(getKeyCodeFromKeyval(it.second), nullptr, sType, &error))
            std::cerr << "Unable to generate modifier key event" << std::endl;
          HandleAtspiError(error, (press ? "key press" : "key release"));
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyDown(const aal::Key k, aal::Modifier modifier) {
  modifierKey(modifier, true);
  GError *error = nullptr;
  if (!atspi_generate_keyboard_event(getKeyCodeFromKeyval(getKeyVal(k)), nullptr, ATSPI_KEY_PRESS, &error))
    std::cerr << "Unable to generate keyDown event" << std::endl;
  HandleAtspiError(error, "keypress");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyUp(const aal::Key k, aal::Modifier modifier) {
  GError *error = nullptr;
  atspi_generate_keyboard_event(getKeyCodeFromKeyval(getKeyVal(k)), nullptr, ATSPI_KEY_RELEASE, &error);
  HandleAtspiError(error, "keypress");
  modifierKey(modifier, false);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyPress(const aal::Key k, aal::Modifier modifier) {
  modifierKey(modifier, true);
  GError *error = nullptr;
  atspi_generate_keyboard_event(getKeyCodeFromKeyval(getKeyVal(k)), nullptr, ATSPI_KEY_PRESSRELEASE, &error);
  HandleAtspiError(error, "keypress");
  modifierKey(modifier, false);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Sends key events to the target, generated from a given string. This way there's no need to deal with keyboard layouts.
 */
void Accessible::typeString(std::string const& input) const {
  VirtualKeyGenerator gen;

  GError *error = nullptr;
  gunichar *ucs4_result = g_utf8_to_ucs4(input.c_str(), -1 , nullptr, nullptr, &error);
  if (ucs4_result) {
    gunichar *p;
    for( p = ucs4_result; p[0]; p++) {
      auto event = gen.generateEvent(p[0]);
      event.press();
      event.release();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    g_free(ucs4_result);
  } else {
    if (error)
      std::cerr << "typeString caught an error: " << error->message << std::endl;
  }
}

void Accessible::bringToFront() {
  throw std::runtime_error("Accessible::bringToFront is not supported on this platform");
}

extern GMainContext *mainGlibContext;

class highlightWindow : public Gtk::Window {
protected:
  std::string _highlightColor;
  std::thread *_timeoutThread;
  geometry::Rectangle _bounds;

  virtual bool onDraw(const ::Cairo::RefPtr< ::Cairo::Context>& ctx) {
    ctx->rectangle(0, 0, this->get_width(), this->get_height());
    auto color = Gdk::Color(_highlightColor);
    ctx->set_source_rgb(color.get_red_p(), color.get_green_p(), color.get_blue_p());
    ctx->fill();
    return false;
  }

  virtual void onShow() {
    _timeoutThread = new std::thread([this]{
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        _waitLoop = true;
    });
  }

public:
  bool _waitLoop;
  highlightWindow(const std::string &color) : Gtk::Window(Gtk::WINDOW_TOPLEVEL), _highlightColor(color), _timeoutThread(nullptr), _waitLoop(false) {

  }

  virtual ~highlightWindow() {
    if (_timeoutThread != nullptr)
      delete _timeoutThread;
  }

  void join() {
    show_now();

    while (!_waitLoop) {
      g_main_context_iteration(mainGlibContext, true);
    }
    _timeoutThread->join();
  }

  void setup(const geometry::Rectangle &rect) {
    _bounds = rect;
    set_app_paintable(true);
    set_decorated(false);
    set_skip_pager_hint(true);
    set_skip_taskbar_hint(true);

    auto display = Gdk::Display::get_default();
    auto screen = display->get_default_screen();
    auto visual = screen->get_rgba_visual();
    gtk_widget_set_visual(dynamic_cast<Gtk::Widget*>(this)->gobj(), visual->gobj());
    set_opacity(0.5);
    move(rect.position.x, rect.position.y);
    set_default_size(rect.size.width, rect.size.height);
    signal_draw().connect(sigc::mem_fun(this, &highlightWindow::onDraw));
    signal_show().connect(sigc::mem_fun(this, &highlightWindow::onShow));
  }

};

//----------------------------------------------------------------------------------------------------------------------

static highlightWindow *wnd = nullptr;
void Accessible::highlight() const {
  auto rootWindow = Gdk::Window::get_default_root_window();
  if (rootWindow) {

    if (wnd != nullptr) {
      wnd->close();
      delete wnd;
    }
    wnd = new highlightWindow("#ff0000");
    wnd->setup(getBounds(true));
    // We have to wait for the window to show up.
    wnd->join();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::removeHighlight() const {
  if (wnd != nullptr) {
    wnd->close();
    delete wnd;
    wnd = nullptr;
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getSelectedText() const {
  std::string buff;
  AtspiText *textIface = getInterfaceText();
  if (textIface != nullptr) {
    GError *error = nullptr;
    auto sCount = atspi_text_get_n_selections(textIface, &error);
    HandleAtspiError(error, "get selections");
    for (auto i = 0; i < sCount; ++i) {
      auto range = atspi_text_get_selection(textIface, i, &error);
      HandleAtspiError(error, "get selection");
      auto text = convertAndFreeString(atspi_text_get_text(textIface, range->start_offset, range->end_offset, &error));
      HandleAtspiError(error, "get text");
      buff.append(text);
      g_free(range);
    }
  }
  return buff;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectedText(std::string const& text) {
  AtspiEditableText *editableTextIface = getInterfaceEditableText();

  if (editableTextIface == nullptr)
    return;

  auto range = getSelectionRange();
  GError *error = nullptr;
  if (!atspi_editable_text_delete_text(editableTextIface, range.start, range.end, &error)) {
    HandleAtspiError(error, "delete text");
    throw std::runtime_error("Unable to set selected text");
  }

  insertText(range.start, text);
}

//----------------------------------------------------------------------------------------------------------------------

TextRange Accessible::getSelectionRange() const {
  AtspiText *textIface = getInterfaceText();
  if (textIface != nullptr) {
    GError *error = nullptr;
    auto sCount = atspi_text_get_n_selections(textIface, &error);
    HandleAtspiError(error, "get n selections");
    if (sCount > 1) {
      auto start = atspi_text_get_selection(textIface, 0, &error);
      HandleAtspiError(error, "get selection");
      auto end = atspi_text_get_selection(textIface, sCount, &error);
      HandleAtspiError(error, "get selection");
      TextRange range(start->start_offset, end->end_offset);
      g_free(start);
      g_free(end);
      return range;
    } else if (sCount == 1) {
      auto start = atspi_text_get_selection(textIface, 0, &error);
      HandleAtspiError(error, "get selection");
      TextRange range(start->start_offset, start->end_offset);
      g_free(start);
      return range;
    }
  }
  return TextRange();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectionRange(TextRange range) {
  AtspiText *textIface = getInterfaceText();
  if (textIface == nullptr)
    return;
  
  GError *error = nullptr;

  auto sCount = atspi_text_get_n_selections(textIface, &error);
  HandleAtspiError(error, "get n selection");

  for(auto i=0; i < sCount; i++) {
    atspi_text_remove_selection(textIface, i, &error);
    HandleAtspiError(error, "remove selection");
  }
  
  if (range.start != range.end) {
    atspi_text_add_selection(textIface, range.start, range.end, &error);
    HandleAtspiError(error, "add selection");
  }
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getDate() const {
  throw std::runtime_error("This element does not support date values.");
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setDate(std::string const& date) {
  throw std::runtime_error("This element does not support date values.");
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t Accessible::getCharacterCount() const {
  AtspiText *textIface = getInterfaceText();
  if (textIface == nullptr)
    return 0;

  GError *error = nullptr;
  auto count = atspi_text_get_character_count(textIface, &error);
  HandleAtspiError(error, "get character count");
  return count;
}

//----------------------------------------------------------------------------------------------------------------------

std::set<size_t> Accessible::getSelectedIndexes() const {
  std::set<size_t> result;
  auto selection = atspi_accessible_get_selection_iface(_accessible);
  
  if (selection == nullptr)
    return result;
  
  GError *error = nullptr;
  AtspiAccessible *menu = atspi_accessible_get_child_at_index(_accessible, 0, &error);
  gint count = atspi_accessible_get_child_count(menu, &error);
  HandleAtspiError(error, "get child count");
  for (gint index = 0; index < count; ++index) {
    if (atspi_selection_is_child_selected(selection, index, &error)) {
      result.insert(static_cast<size_t>(index));
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectedIndexes(std::set<size_t> const& indexes) {
  auto selection = getInterfaceSelection();
  if (selection == nullptr)
    return;
  
  GError *error = nullptr;
  for(const auto &i: indexes)
    atspi_selection_select_child(selection, i, &error);

  std::this_thread::sleep_for(std::chrono::milliseconds(10)); // before continuing we have to give it a little bit of time to realy do the focus
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Rectangle Accessible::getBounds(AtspiAccessible *acc, bool screenCoordinates) {
  GError *error = nullptr;
  AtspiComponent *componentInterface = atspi_accessible_get_component(acc);
  if (componentInterface == nullptr)
    throw std::runtime_error("Unable to get bounds for this accessible");
  
  if (screenCoordinates) {
    AtspiRect *rect = atspi_component_get_extents(componentInterface, ATSPI_COORD_TYPE_SCREEN, &error);
    geometry::Rectangle grect(rect->x, rect->y, rect->width, rect->height);
    g_boxed_free(ATSPI_TYPE_RECT, rect);
    HandleAtspiError(error, "get extents");
    return grect;
  } else {
    AtspiAccessible *parent = atspi_accessible_get_parent(acc, &error);
    if (parent == nullptr) { // This means it's the top most element no additional calculation should be needed.
      AtspiRect *rect = atspi_component_get_extents(componentInterface, ATSPI_COORD_TYPE_WINDOW, &error);
      geometry::Rectangle grect(rect->x, rect->y, rect->width, rect->height);
      g_boxed_free(ATSPI_TYPE_RECT, rect);
      HandleAtspiError(error, "get extents");
      return grect;
    } else {
      AtspiRect *rect = atspi_component_get_extents(componentInterface, ATSPI_COORD_TYPE_SCREEN, &error);

      GError *parentError = nullptr;
      AtspiComponent *parentComponentInterface = atspi_accessible_get_component(parent);
      if (parentComponentInterface == nullptr)
        throw std::runtime_error("Unable to get parent bounds for this accessible.");
      
      AtspiRect *parentRect = atspi_component_get_extents(parentComponentInterface, ATSPI_COORD_TYPE_SCREEN, &parentError);

      geometry::Rectangle grect(rect->x - parentRect->x, rect->y - parentRect->y, rect->width, rect->height);
      g_boxed_free(ATSPI_TYPE_RECT, rect);
      g_boxed_free(ATSPI_TYPE_RECT, parentRect);
      HandleAtspiError(error, "get extents");
      HandleAtspiError( parentError, "get extents");
      return grect;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Rectangle Accessible::getBounds(bool screenCoordinates) const {
  return getBounds(_accessible, screenCoordinates);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setBounds(geometry::Rectangle const& bounds) {
  AtspiComponent *componentInterface = atspi_accessible_get_component(_accessible);
  if (componentInterface == nullptr)
    throw std::runtime_error("Unable to get bounds for this accessible");

  GError *error = nullptr;
  atspi_component_set_extents(componentInterface, bounds.position.x, bounds.position.y, bounds.size.width, bounds.size.height, ATSPI_COORD_TYPE_SCREEN, &error);
  HandleAtspiError(error, "set extents");
}

//----------------------------------------------------------------------------------------------------------------------

AtspiAccessible *getFrame(AtspiAccessible *acc) {
  if (Accessible::getPlatformRole(acc) == ATSPI_ROLE_FRAME)
    return acc;

  GError *error = nullptr;
  AtspiAccessible *result = atspi_accessible_get_parent(acc, &error);
  HandleAtspiError(error, "get parent");
  if (Accessible::getPlatformRole(result) == ATSPI_ROLE_FRAME)
        return result;
  while (result != nullptr) {

    if (Accessible::getPlatformRole(result) == ATSPI_ROLE_FRAME)
      return result;

    result = atspi_accessible_get_parent(result, &error);
    HandleAtspiError(error, "get parent");
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Rectangle getWindowRect(const Glib::RefPtr<Gdk::Window> &wnd) {
  geometry::Rectangle rect;
  rect.size.width = wnd->get_width();
  rect.size.height = wnd->get_height();
  wnd->get_origin(rect.position.x, rect.position.y);
  return rect;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::takeScreenShot(const std::string &path, bool onlyWindow, geometry::Rectangle rect) const {
  Glib::RefPtr<Gdk::Window> window = Gdk::Window::get_default_root_window();
  Glib::RefPtr<Gdk::Pixbuf> px;

  geometry::Rectangle tmpRect;
  if (onlyWindow) {
    auto frame = getFrame(_accessible);
    if (frame == nullptr) {
      throw std::runtime_error("Can't get owner window");
    }

    tmpRect = getBounds(frame, true);
    if (!rect.empty()) {
      tmpRect.position.x = rect.position.x + tmpRect.position.x;
      tmpRect.position.y = rect.position.y + tmpRect.position.y;
      tmpRect.size = rect.size;
    }
    px = Gdk::Pixbuf::create(window, tmpRect.position.x, tmpRect.position.y, tmpRect.size.width, tmpRect.size.height);
  } else {
    if (rect.empty())
      tmpRect = getWindowRect(window);
    else
      tmpRect = rect;
    px = Gdk::Pixbuf::create(window, tmpRect.position.x, tmpRect.position.y, tmpRect.size.width, tmpRect.size.height);
  }

  try {
    px->save(path, "png");
  } catch (std::exception &e) {
    std::cerr << "Unable to store screenshot: " << path << std::endl;
    std::cerr << "Error was: " << e.what() << std::endl;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::saveImage(std::string const& path) const {
  NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::show() { 
  NOT_IMPLEMENTED; 
}

//----------------------------------------------------------------------------------------------------------------------

int Accessible::getPid() {
  GError *error = nullptr;
  if (_pid == 0)
    _pid = atspi_accessible_get_process_id(_accessible, &error);

  HandleAtspiError(error, "process pid");
  return _pid;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isExpandable() const {
  return getState(ATSPI_STATE_EXPANDABLE);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isExpanded() const {
  if (_role == ATSPI_ROLE_COMBO_BOX) {
    GError *error = nullptr;
    AtspiAccessible *menu = atspi_accessible_get_child_at_index(_accessible, 0, &error);
    return isVisible(menu);
  }

  return getState(ATSPI_STATE_EXPANDED);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setExpanded(bool value) {
  if (_role == ATSPI_ROLE_COMBO_BOX) {
    GError *error = nullptr;
    AtspiAccessible *menu = atspi_accessible_get_child_at_index(_accessible, 0, &error);
    if (getState(ATSPI_STATE_VISIBLE, menu) != value)
      triggerAction("press");
    return;
  }

  if (!isExpandable()) {
    std::cerr << "Element " << getName() << " is not expandable/collapsable." << std::endl;
    return;
  }
  
  if (getState(ATSPI_STATE_EXPANDED) == value)
    return;
  
  try {
    triggerAction("expand");
  } catch (...) {
    try {
      triggerAction("activate");
    } catch (...) {
      std::cerr << "Unable to find expand/collapsable action for element: " << getName() << std::endl;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setEnabled(bool value) {
  throw std::runtime_error("Attribute is read-only");
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isEditable() const {
  if (_role != ATSPI_ROLE_COMBO_BOX)
    throw std::runtime_error("The editable type is only supported for combobboxes.");

  GError *error = nullptr;
  gint count = atspi_accessible_get_child_count(_accessible, &error);
  HandleAtspiError(error, "get child count");
  for (gint index = 0; index < count; ++index) {
      AtspiAccessible *child = atspi_accessible_get_child_at_index(_accessible, index, &error);
      HandleAtspiError(error, "get child at index");
      if (child) {
        if (getPlatformRole(child) == ATSPI_ROLE_TEXT) {
          return true;
        }
      }
  }
  return false;

}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isReadOnly() const {
  return !(getState(ATSPI_STATE_ENABLED) || getState(ATSPI_STATE_EDITABLE));
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSecure() const {
  return _role == ATSPI_ROLE_PASSWORD_TEXT;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isHorizontal() const {
  bool isHoriz = getState(ATSPI_STATE_HORIZONTAL);
  bool isVert = getState(ATSPI_STATE_VERTICAL);

  if (!isHoriz && !isVert)
    throw std::runtime_error("This element does not support layout informations.");
  return isHoriz;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getByPid(const int pid) {
  AccessibleRef acc(new Accessible(atspi_get_desktop(0), false));
  auto children = acc->children();
  for(auto &elem : children) {
    if (elem->isValid() && elem->getPid() == pid)
      return AccessibleRef(new Accessible(elem->_accessible, elem->_isRoot));
  }

  return AccessibleRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::accessibilitySetup() {
  return true;
}

//----------------------------------------------------------------------------------------------------------------------
