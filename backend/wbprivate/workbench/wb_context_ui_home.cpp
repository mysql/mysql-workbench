/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <glib/gstdio.h>

#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/log.h"
#include "base/ui_form.h"

#include "mforms/menu.h"

#include "grt.h"

#include "grts/structs.app.h"
#include "grts/structs.h"

#include "grt/editor_base.h"

#include "workbench/wb_context_ui.h"
#include "workbench/wb_command_ui.h"
#include "workbench/about_box.h"

#include "model/wb_context_model.h"
#include "sqlide/wb_context_sqlide.h"
#include "sqlide/wb_sql_editor_form.h"

#include "new_server_instance_wizard.h"
#include "server_instance_editor.h"
#include "grtui/grtdb_connection_editor.h"
#include "new_connection_wizard.h"
#include "select_option_dialog.h"
#include "wb_model_file.h"
#include "base/data_types.h"
#include "mforms/home_screen.h"
#include "mforms/home_screen_connections.h"
#include "mforms/home_screen_documents.h"
#include "mforms/menubar.h"
#include <zip.h>

DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT_UI);

using namespace bec;
using namespace wb;
using namespace base;

/**
 * Helper method to construct a human-readable server description.
 */
std::string get_server_info(db_mgmt_ServerInstanceRef instance) {
  std::string text;
  std::string system = instance->serverInfo().get_string("sys.system");

  if (instance->serverInfo().get_int("remoteAdmin"))
    text = strfmt("Host: %s  Type: %s", instance->loginInfo().get_string("ssh.hostName").c_str(), system.c_str());
  else {
    if (instance->serverInfo().get_int("windowsAdmin") /* || system == "Windows"*/) {
      std::string host = instance->loginInfo().get_string("wmi.hostName");
      if (host == "localhost" || host.empty() || host == "127.0.0.1") {
        text = "Local  Type: Windows";
      } else
        text = "Host: " + host + "  Type: Windows";
    } else {
      std::string host = instance->connection().is_valid()
                           ? instance->connection()->parameterValues().get_string("hostName")
                           : "Invalid";

      if (host == "localhost" || host.empty() || host == "127.0.0.1") {
        text = strfmt("Local  Type: %s", system.c_str());
      } else
        text = strfmt("Host: %s  Type: DB Only", host.c_str());
    }
  }

  return text;
}

//--------------------------------------------------------------------------------------------------

template <class T>
void get_groups_for_movement(grt::ListRef<T> items, const grt::ValueRef &object, std::vector<std::string> &groups) {
  grt::Ref<T> item = grt::Ref<T>::cast_from(object);

  std::string item_name = item->name();
  size_t current_group_separator_position = item_name.find("/");

  std::string current_group = "";

  if (current_group_separator_position != std::string::npos) {
    current_group = item_name.substr(0, current_group_separator_position);
    groups.push_back("*Ungrouped*");
  }

  for (typename grt::ListRef<T>::const_iterator end = items.end(), inst = items.begin(); inst != end; ++inst) {
    std::string name = (*inst)->name();
    size_t group_separator_position = name.find("/");

    if (group_separator_position != std::string::npos) {
      std::string group_name = name.substr(0, group_separator_position);

      bool found = false;
      for (std::vector<std::string>::iterator end = groups.end(), index = groups.begin(); index != end && !found;
           index++) {
        found = (*index).compare(0, group_separator_position, group_name) == 0;
      }

      if (!found && group_name != current_group)
        groups.push_back(group_name);
    }
  }
}

template <class T>
bool validate_group_for_movement(grt::ListRef<T> items, const grt::ValueRef &object, std::string group) {
  bool ret_val = false;

  // Validates the group is valid...
  size_t group_position = group.find("/");

  if (group.length() == 0)
    Utilities::show_warning(_("Move To Group"),
                            _("You must select the target group from the list or type a new group."), _("Ok"));
  else if (group_position != std::string::npos)
    Utilities::show_warning(_("Move To Group"),
                            _("The selected group is invalid, should not contain the \"/\" character."), _("Ok"));
  else {
    // Gets the connection and connection list...
    grt::Ref<T> item = grt::Ref<T>::cast_from(object);

    // Creates the new connection name...
    std::string item_name = item->name();
    std::string new_item_name = "";

    group_position = item_name.find("/");

    if (group == "*Ungrouped*")
      new_item_name = item_name.substr(group_position + 1);
    else {
      if (group_position != std::string::npos)
        new_item_name = group + "/" + item_name.substr(group_position + 1);
      else
        new_item_name = group + "/" + item_name;
    }

    size_t item_index = bec::find_list_ref_item_position<T>(items, new_item_name, MatchAny, NULL, FindFull);

    if (item_index != grt::BaseListRef::npos)
      Utilities::show_warning(
        _("Move To Group"),
        _("Unable to perform the movement as there's an entry with the same name in the target group"), _("Ok"));
    else
      ret_val = true;
  }

  return ret_val;
}

template <class T>
void update_item_group(const grt::ValueRef &object, std::string group)
// At this point the movement is considered valid so we just do it...
{
  grt::Ref<T> item = grt::Ref<T>::cast_from(object);
  std::string item_name = item->name();
  size_t current_group_separator_position = item_name.find("/");

  std::string new_item_name = "";
  if (group == "*Ungrouped*")
    new_item_name = item_name.substr(current_group_separator_position + 1);
  else {
    if (current_group_separator_position != std::string::npos)
      new_item_name = group + "/" + item_name.substr(current_group_separator_position + 1);
    else
      new_item_name = group + "/" + item_name;
  }

  item->name(new_item_name);
}

template <class T>
void move_item_to_group(std::string group, grt::ListRef<T> items, const grt::ValueRef &object) {
  size_t current_item_index = 0, current_group_index = 0, sibling_index = 0, target_index = 0;
  bool sibling_move = false;
  bool item_move = false;

  // To ensure the grouping order is maintained after a move to group operation it is needed to consider
  // - If the item being moved is in a group or not
  // - If the item is being moved to a group or not
  // - Removing the first item from a group may affect the group ordering, to prevent that, the next item in the group
  //   is being moved to the position of the first item
  // - When moved to a group, the item will be placed after the last element in the group
  // - Moving to a new group or ungrouping the item doesn't affect it's position on the list

  // grt::Type object_type = object.type();

  grt::Ref<T> item = grt::Ref<T>::cast_from(object);
  std::string item_name = item->name();

  std::string item_group = "";
  size_t group_indicator_position = item_name.find("/");

  // Gets the position of the item being moved...
  current_item_index = find_list_ref_item_position<T>(items, item_name);
  if (group_indicator_position != std::string::npos) {
    item_group = item_name.substr(0, group_indicator_position + 1);

    // Gets the position of the first element on the group
    current_group_index = find_list_ref_item_position<T>(items, item_group);

    if (current_item_index == current_group_index) {
      sibling_index = find_list_ref_item_position<T>(items, item_group, MatchAfter, &item);

      if (sibling_index != grt::BaseListRef::npos)
        sibling_move = true;
    }
  }

  if (group != "*Ungrouped*") {
    // Gets the position of the last element on the target group if exists ( happily the function does that when passing
    // MatchBefore without a reference )
    std::string tmp_group = group + "/";
    target_index = find_list_ref_item_position<T>(items, tmp_group, MatchLast);

    if (target_index != grt::BaseListRef::npos) {
      item_move = true;

      if (target_index < current_item_index)
        target_index++;
    }
  }

  if (sibling_move) {
    items.reorder(sibling_index, current_item_index);

    if (sibling_index > current_item_index)
      current_item_index++;
  }

  if (item_move)
    items.reorder(current_item_index, target_index);

  update_item_group<T>(object, group);
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::show_about() {
  AboutBox::show_about(_wb->get_root()->info()->edition());
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates the main home screen (Workbench Central, Workspace) if not yet done and docks it to
 * the main application window.
 */

void WBContextUI::show_home_screen() {
  if (_home_screen != nullptr) {
    _home_screen->showSection(0);
    mforms::App::get()->select_view(_home_screen);
    return;
  }

  _initializing_home_screen = (_home_screen == NULL);
  if (_home_screen == NULL) {
    // The home screen and its content is freed in AppView::close() during undock_view(...).
    _home_screen = mforms::manage(new mforms::HomeScreen, false);
    _home_screen->set_menubar(mforms::setReleaseOnAdd(_command_ui->create_menubar_for_context(WB_CONTEXT_HOME_GLOBAL)));
    _home_screen->onHomeScreenAction =
      std::bind(&WBContextUI::handle_home_action, this, std::placeholders::_1, std::placeholders::_2);
    _home_screen->handleContextMenu =
      std::bind(&WBContextUI::handle_home_context_menu, this, std::placeholders::_1, std::placeholders::_2);

    // now we have to add sections
    _connectionsSection = mforms::manage(new mforms::ConnectionsSection(_home_screen));
    _connectionsSection->set_name("Home Screen Connections Section");
    _connectionsSection->setInternalName("homeScreenConnectionsSection");
    _connectionsSection->showWelcomeHeading(bec::GRTManager::get()->get_app_option_int("HomeScreen:HeadingMessage", 1) == 1);
    _connectionsSection->getConnectionInfoCallback = std::bind([=] (const std::string &connectionId) -> mforms::anyMap {
      return connectionToMap(getConnectionById(connectionId));
    }, std::placeholders::_1);

    _home_screen->addSection(_connectionsSection);

    _documentsSection = mforms::manage(new mforms::DocumentsSection(_home_screen));
    _documentsSection->set_name("Home Screen Documents Section");
    _documentsSection->setInternalName("homeScreenDocumentsSection");
    _home_screen->addSection(_documentsSection);

    _home_screen->addSectionEntry("Migration Section", "sidebar_migration.png", [this]() {
      logInfo("Opening Migration Wizard...\n");
      _wb->add_new_plugin_window("wb.migration.open", "Migration Wizard");
    }, false);

    _home_screen->updateColors();
    _home_screen->updateIcons();

    // Setup context menus.
    mforms::Menu *menu;
    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      groups.push_back("Menu/Home/Connections");
      bec::MenuItemList pitems = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty()) {
        menu = mforms::manage(new mforms::Menu());
        menu->add_items_from_list(pitems);
        _home_screen->set_menu(menu, HomeMenuConnectionGeneric);
      }
    }

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Open Connection"), "open_connection");
    menu->add_item(_("Edit Connection..."), "edit_connection");

    menu->add_separator();
    menu->add_item(_("Move to Group..."), "move_connection_to_group");

    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      argument_pool.add_entries_for_object("selectedConnection", db_mgmt_ConnectionRef(grt::Initialized),
                                           "db.mgmt.Connection");
      groups.push_back("Menu/Home/Connections");
      bec::MenuItemList pitems = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty()) {
        menu->add_separator();
        menu->add_items_from_list(pitems);
      }
    }

    menu->add_separator();
    menu->add_item(_("Move To Top"), "move_connection_to_top");
    menu->add_item(_("Move Up"), "move_connection_up");
    menu->add_item(_("Move Down"), "move_connection_down");
    menu->add_item(_("Move To End"), "move_connection_to_end");

    menu->add_separator();
    menu->add_item(_("Delete Connection..."), "delete_connection");
    menu->add_item(_("Delete All Connections..."), "delete_connection_all");

    _home_screen->set_menu(menu, HomeMenuConnection);
    menu->release();

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Move To Top"), "move_connection_to_top");
    menu->add_item(_("Move Up"), "move_connection_up");
    menu->add_item(_("Move Down"), "move_connection_down");
    menu->add_item(_("Move To End"), "move_connection_to_end");
    menu->add_separator();
    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);

      argument_pool.add_simple_value("selectedGroupName", grt::StringRef(""));
      groups.push_back("Menu/Home/ConnectionGroup");
      bec::MenuItemList pitems = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty()) {
        menu->add_items_from_list(pitems);
        menu->add_separator();
      }
    }
    menu->add_item(_("Delete Group..."), "delete_connection_group");

    _home_screen->set_menu(menu, HomeMenuConnectionGroup);
    menu->release();

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Open Model"), "open_model_from_list");
    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      argument_pool.add_simple_value("selectedModelFile", grt::ValueRef());
      groups.push_back("Menu/Home/ModelFiles");
      bec::MenuItemList pitems = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty()) {
        menu->add_separator();
        for (bec::MenuItemList::const_iterator iterator = pitems.begin(); iterator != pitems.end(); iterator++)
          menu->add_items_from_list(pitems);
      }
    }

    menu->add_separator();
    menu->add_item(_("Show Model File"), "show_model");
    menu->add_item(_("Remove Model File from List"), "remove_model");
    menu->add_item(_("Clear List"), "remove_model_all");
    _home_screen->set_menu(menu, HomeMenuDocumentModel);
    menu->release();

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Create EER Model from Database"), "model_from_schema");
    menu->add_item(_("Create EER Model from Script"), "model_from_script");
    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      argument_pool.add_simple_value("selectedModelFile", grt::ValueRef());
      groups.push_back("Menu/Home/ModelFiles"); // TODO: do we need a different group for the action menu?
      bec::MenuItemList pitems = bec::GRTManager::get()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty()) {
        menu->add_separator();
        for (bec::MenuItemList::const_iterator iterator = pitems.begin(); iterator != pitems.end(); iterator++)
          menu->add_items_from_list(pitems);
      }
    }

    _home_screen->set_menu(menu, HomeMenuDocumentModelAction);
    menu->release();
  }

  mforms::App::get()->dock_view(_home_screen, "maintab");

  std::string error;
  try {
    refresh_home_documents();
    refresh_home_connections();
  } catch (const std::exception *exc) {
    error = exc->what();
  } catch (const std::exception &exc) {
    error = exc.what();
  } catch (...) {
    error = "(unknown)";
  }

  if (!error.empty()) {
    std::string message =
      base::strfmt(_("Error while setting up home screen. The error message is: %s"), error.c_str());
    logError("%s\n", message.c_str());
    mforms::Utilities::show_error(_("Home Screen Error"), message, _("Close"));
  }

  _home_screen->setup_done();

  if (!_oldAuthList.empty()) {
    std::string tmp;
    std::vector<db_mgmt_ConnectionRef>::const_iterator it;
    for (it = _oldAuthList.begin(); it != _oldAuthList.end(); ++it) {
      tmp.append("\n");
      tmp.append((*it)->name());
      tmp.append(" user name:");
      tmp.append((*it)->parameterValues().get_string("userName"));
    }

    int rc = mforms::Utilities::show_warning(
      "Connections using old authentication protocol found",
      "While loading the stored connections some were found to use the old authentication protocol. "
      "This is no longer supported by MySQL Workbench and the MySQL client library. Click on the \"More Info\" button "
      "for a more detailed explanation.\n\n"
      "With this change it is essential that user accounts are converted to the new password storage or you can no "
      "longer connect with MySQL Workbench using these accounts.\n\n"
      "The following connections are affected:\n" +
        tmp,
      "Change", "Ignore", "More Info");
    if (rc == mforms::ResultOther) {
      mforms::Utilities::open_url(
        "http://mysqlworkbench.org/2014/03/"
        "mysql-workbench-6-1-updating-accounts-using-the-old-pre-4-1-1-authentication-protocol/");
    } else if (rc == mforms::ResultOk) {
      std::vector<db_mgmt_ConnectionRef>::const_iterator it;
      for (it = _oldAuthList.begin(); it != _oldAuthList.end(); ++it) {
        if ((*it).is_valid()) {
          if ((*it)->parameterValues().has_key("useLegacyAuth"))
            (*it)->parameterValues().remove("useLegacyAuth");
        }
      }
      _oldAuthList.clear();
    }
  }

  _home_screen->showSection(0);
  _initializing_home_screen = false;
}

//--------------------------------------------------------------------------------------------------

db_mgmt_ConnectionRef WBContextUI::getConnectionById(const std::string &id) {
  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
  for (std::size_t i = 0; i < connections->count(); ++i) {
    if (connections[i].id() == id)
      return connections[i];
  }

  return db_mgmt_ConnectionRef();
}

//--------------------------------------------------------------------------------------------------

static bool isSSHConnection(const db_mgmt_ConnectionRef &connection) {
  if (connection.is_valid()) {
    std::string driver = connection->driver().is_valid() ? connection->driver()->name() : "";
    return (driver == "MysqlNativeSSH");
  }
  return false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Determines if the given connection is a local connection (i.e. to the current box).
 */
static bool isLocalConnection(const db_mgmt_ConnectionRef &connection) {
  if (connection.is_valid()) {
    std::string hostname = connection->parameterValues().get_string("hostName");

    if (!isSSHConnection(connection) && (hostname == "localhost" || hostname.empty() || hostname == "127.0.0.1"))
      return true;
  }
  return false;
}

anyMap WBContextUI::connectionToMap(db_mgmt_ConnectionRef connection) {
  anyMap output;

  if (!connection.is_valid())
    return output;

  db_mgmt_ServerInstanceRef instance;
  grt::ListRef<db_mgmt_ServerInstance> instances = _wb->get_root()->rdbmsMgmt()->storedInstances();
  for (grt::ListRef<db_mgmt_ServerInstance>::const_iterator iterator = instances.begin(); iterator != instances.end();
       iterator++) {
    if ((*iterator)->connection() == connection) {
      instance = *iterator;
      break;
    }
  }

  output = grt::convert(connection->parameterValues());

  if (instance.is_valid() && instance->serverInfo().is_valid())
    output.insert({"serverInfo", grt::convert(instance->serverInfo())});
  else
    output.insert({"serverInfo", base::any()});

  if (instance.is_valid() && instance->loginInfo().is_valid())
    output.insert({"loginInfo", grt::convert(instance->loginInfo())});
  else
    output.insert({"loginInfo", base::any()});

  output.insert({"isLocalConnection", isLocalConnection(connection)});
  output.insert({"isSSHConnection", isSSHConnection(connection)});
  output.insert({"hostIdentifier", std::string(connection->hostIdentifier())});
  std::string name = connection->name();
  output.insert({"name", name});
  return output;
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes a connection from the stored connections list along with all associated data
 * (including its server instance entry).
 */
void WBContextUI::remove_connection(const db_mgmt_ConnectionRef &connection) {
  grt::BaseListRef args(true);
  args->insert_unchecked(connection);

  grt::ValueRef result = grt::GRT::get()->call_module_function("Workbench", "deleteConnection", args);
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::handle_home_context_menu(const base::any &object, const std::string &action) {
  if (action == "open_connection") {
    handle_home_action(HomeScreenAction::ActionOpenConnectionFromList, object);
  } else if (action == "delete_connection") {
    db_mgmt_ConnectionRef connection = getConnectionById(object.as<std::string>());

    if (!connection.is_valid()) {
      logError("Invalid connection has been found during action: %s\n", action.c_str());
      return;
    }
    std::string name = connection->name();
    std::string title;
    std::string warning;

    title = _("Delete Connection");
    warning = strfmt(_("Do you want to delete connection %s?"), name.c_str());

    int answer = Utilities::show_warning(title, warning, _("Delete"), _("Cancel"));
    if (answer == mforms::ResultOk) {
      remove_connection(connection);
      refresh_home_connections();
    }
  } else if (action == "manage_connections" || action == "edit_connection") {
    handle_home_action(HomeScreenAction::ActionManageConnections, object);
  } else if (action == "move_connection_to_top") {
    // We enter here for both: groups and connections. The object for groups must be a StringRef
    // with the name of the group. For connections it's the connection ref.
    // Similar for the other move_* actions.
    db_mgmt_ConnectionRef val = getConnectionById(object.as<std::string>());
    if (!val.is_valid()) {
      logError("Invalid connection has been found during action: %s\n", action.c_str());
      return;
    }
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(connections, val, MoveTop);
    refresh_home_connections(false);
  } else if (action == "move_connection_up") {
    grt::ValueRef val = getConnectionById(object.as<std::string>());
    if (!val.is_valid()) {
      logError("Invalid connection has been found during action: %s\n", action.c_str());
      return;
    }
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(connections, val, MoveUp);
    refresh_home_connections(false);
  } else if (action == "move_connection_down") {
    db_mgmt_ConnectionRef val = getConnectionById(object.as<std::string>());
    if (!val.is_valid()) {
      logError("Invalid connection has been found during action: %s\n", action.c_str());
      return;
    }
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(connections, val, MoveDown);
    refresh_home_connections(false);
  } else if (action == "move_connection_to_end") {
    db_mgmt_ConnectionRef val = getConnectionById(object.as<std::string>());
    if (!val.is_valid()) {
      logError("Invalid connection has been found during action: %s\n", action.c_str());
      return;
    }
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(connections, val, MoveBottom);
    refresh_home_connections(false);
  } else if (action == "move_connection_to_group") {
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

    // Create the available groups for the movement...
    std::vector<std::string> groups;
    db_mgmt_ConnectionRef val = getConnectionById(object.as<std::string>());
    if (!val.is_valid()) {
      logError("Invalid connection has been found during action: %s\n", action.c_str());
      return;
    }
    get_groups_for_movement<db_mgmt_Connection>(connections, val, groups);

    SelectOptionDialog dialog(_("Move To Group"), _("Pick a group to move the selected connection "
                                                    "to\nor type a name to move it to a new one."),
                              groups);
    dialog.set_validation_function(
      std::bind(&validate_group_for_movement<db_mgmt_Connection>, connections, val, std::placeholders::_1));
    std::string result = dialog.run();

    // At this point the movement is considered valid so we just do it.
    if (result != "") {
      move_item_to_group<db_mgmt_Connection>(result, connections, val);
      refresh_home_connections();
    }
  } else if (action == "delete_connection_group" || action == "internal_delete_connection_group") {
    std::string group = object;
    int answer = mforms::ResultOk;

    // Internal deletion does not require the prompt
    if (action == "delete_connection_group") {
      std::string text = strfmt(_("Do you really want to delete all the connections in group: %s?"),
                                base::left(group, (unsigned int)group.length() - 1).c_str());
      answer = Utilities::show_warning(_("Delete Connection Group"), text, _("Delete"), _("Cancel"));
    }

    if (answer == mforms::ResultOk) {
      grt::BaseListRef args(true);
      std::string val = object;
      args->insert_unchecked(grt::StringRef(val));

      grt::ValueRef result = grt::GRT::get()->call_module_function("Workbench", "deleteConnectionGroup", args);

      // Internal deletion does not require the UI update
      if (action == "delete_connection_group")
        refresh_home_connections();
    }
  } else if (action == "delete_connection_all") {
    std::string text = _("Do you really want to delete all defined connections?");
    int answer = Utilities::show_warning(_("Delete All Connections"), text, _("Delete"), _("Cancel"));
    if (answer == mforms::ResultOk) {
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
      while (connections->count() > 0)
        remove_connection(connections[0]);
      refresh_home_connections();
    }
  } else if (action == "open_model_from_list") {
    handle_home_action(HomeScreenAction::ActionOpenEERModelFromList, object);
  } else if (action == "model_from_schema") {
    handle_home_action(HomeScreenAction::ActionNewModelFromDB, object);
  } else if (action == "model_from_script") {
    handle_home_action(HomeScreenAction::ActionNewModelFromScript, object);
  } else if (action == "show_model") {
    std::string file = object;
    mforms::Utilities::reveal_file(file);
  } else if (action == "remove_model") {
    std::string file = object;
    _wb->get_root()->options()->recentFiles()->remove(grt::StringRef(file));

    bool remove_auto_save = false;
    if (file.size() > 5 && file.substr(file.size() - 5) == ".mwbd")
      remove_auto_save = true;
    else {
      std::map<std::string, std::string> auto_save_models(WBContextModel::auto_save_files());

      if (auto_save_models.find(file) != auto_save_models.end()) {
        file = auto_save_models[file];
        remove_auto_save = true;
      } else if (auto_save_models.find(base::basename(file)) != auto_save_models.end()) {
        file = auto_save_models[base::basename(file)];
        remove_auto_save = true;
      }
    }

    if (remove_auto_save) {
      try {
        base::remove_recursive(file);

        // Refreshes the list of autosaved files...
        WBContextModel::detect_auto_save_files(_wb->get_auto_save_dir());
      } catch (std::exception &exc) {
        logError("Error removing model file %s: %s\n", file.c_str(), exc.what());
      }
    }
    refresh_home_documents();
  } else if (action == "remove_model_all") {
    std::string text = _("Do you want to remove all entries from the model list?");
    int answer = Utilities::show_warning(_("Clear Model Entry List"), text, _("Delete"), _("Cancel"));
    if (answer == mforms::ResultOk) {
      grt::StringListRef file_names(_wb->get_root()->options()->recentFiles());
      for (ssize_t index = file_names->count() - 1; index >= 0; index--) {
        if (g_str_has_suffix(file_names[index].c_str(), ".mwb"))
          file_names->remove(index);
      }
      refresh_home_documents();
    }
  } else {
    bec::ArgumentPool argument_pool;
    _wb->update_plugin_arguments_pool(argument_pool);

    if (object.is<std::string>()) {
      std::string val = object;
      db_mgmt_ConnectionRef connection = getConnectionById(val);

      if (connection.is_valid())
        argument_pool.add_entries_for_object("selectedConnection", connection);
      else if (base::hasSuffix(val, ".mwb"))
        argument_pool.add_simple_value("selectedModelFile", grt::StringRef(val)); // assume a model file
      else
        argument_pool.add_simple_value("selectedGroupName", grt::StringRef(val)); // assume a connection group name
    }

    get_command_ui()->activate_command(action, argument_pool);
  }
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::start_plugin(const std::string &title, const std::string &command, const bec::ArgumentPool &defaults,
                               bool force_external) {
  try {
    std::string message_title = base::strfmt(_("Starting %s"), title.c_str());
    GUILock lock(_wb, message_title, _("Please stand by while the plugin is started..."));
    if (base::hasPrefix(command, "plugin:")) {
      _wb->execute_plugin(command.substr(7, command.length()), defaults);
    } else if (base::hasPrefix(command, "browse:"))
      show_web_page(command.substr(7, command.length()), !force_external);
    else if (base::hasPrefix(command, "http://"))
      show_web_page(command, false);
  } catch (const std::exception &exc) {
    std::string message = base::strfmt(_("Could not open link or plugin. The error message is: %s"), exc.what());
    logError("%s\n", message.c_str());
    mforms::Utilities::show_error(_("Open Plugin Error"), message, _("Close"));
  }
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::handle_home_action(mforms::HomeScreenAction action, const base::any &anyObject) {
  switch (action) {
    case HomeScreenAction::ActionNone:
      break;

    case HomeScreenAction::ActionOpenConnectionFromList: {
      if (_processing_action_open_connection)
        break;
      _processing_action_open_connection = true;
      db_mgmt_ConnectionRef object;
      if (!anyObject.isNull())
        object = getConnectionById(anyObject.as<std::string>());

      if (object.is_valid()) {
        db_mgmt_ConnectionRef connection(db_mgmt_ConnectionRef::cast_from(object));

        _wb->_frontendCallbacks->show_status_text("Opening SQL Editor...");
        _wb->add_new_query_window(connection);
      }
      _processing_action_open_connection = false;
      break;
    }

    case HomeScreenAction::ActionFilesWithConnection: {
      if (_processing_action_open_connection)
        break;

      _processing_action_open_connection = true;
      HomeScreenDropFilesInfo dInfo;
      if (!anyObject.isNull())
        dInfo = anyObject;

      if (dInfo.files.size() != 0) {
        db_mgmt_ConnectionRef connection = getConnectionById(dInfo.connectionId);
        _wb->_frontendCallbacks->show_status_text("Opening files in new SQL Editor ...");
        std::shared_ptr<SqlEditorForm> form = _wb->add_new_query_window(connection, false);
        if (form) {
          for (auto &it : dInfo.files)
            form->open_file(it, true);
          form->update_title();
        }
      }

      _processing_action_open_connection = false;
      break;
    }

    case HomeScreenAction::ActionNewConnection: {
      NewConnectionWizard wizard(_wb, _wb->get_root()->rdbmsMgmt());
      wizard.set_title("Setup New Connection");
      wizard.run();
      refresh_home_connections();
      break;
    }

    case HomeScreenAction::ActionManageConnections: {
      db_mgmt_ConnectionRef object;
      if (!anyObject.isNull())
        object = getConnectionById(anyObject.as<std::string>());

      ServerInstanceEditor editor(_wb->get_root()->rdbmsMgmt());
      _wb->_frontendCallbacks->show_status_text("Connection Manager Opened");
      editor.run(db_mgmt_ConnectionRef::cast_from(object));
      _wb->_frontendCallbacks->show_status_text("");
      refresh_home_connections();
      break;
    }

    case HomeScreenAction::ActionMoveConnection: {
      HomeScreenDropInfo dropInfo = anyObject;

      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

      if (dropInfo.valueIsConnectionId) {
        db_mgmt_ConnectionRef connection = getConnectionById(dropInfo.value);
        move_list_ref_item<db_mgmt_Connection>(connections, connection, dropInfo.to);
        refresh_home_connections(false);
      } else {
        std::string group = dropInfo.value;
        move_list_ref_item<db_mgmt_Connection>(connections, grt::StringRef(group), dropInfo.to);
        refresh_home_connections(false);
      }
      break;
    }

    case HomeScreenAction::ActionMoveConnectionToGroup: {
      HomeScreenDropInfo dropInfo = anyObject;
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
      db_mgmt_ConnectionRef connection = getConnectionById(dropInfo.value);
      if (dropInfo.group != "" && connection.is_valid()) {
        move_item_to_group<db_mgmt_Connection>(dropInfo.group, connections, connection);
        refresh_home_connections(false);
      }
      break;
    }

    case HomeScreenAction::ActionSetupRemoteManagement: {
      db_mgmt_ConnectionRef object = getConnectionById(anyObject.as<std::string>());
      NewServerInstanceWizard wizard(_wb, object);
      _wb->_frontendCallbacks->show_status_text("Started Management Setup Wizard");
      wizard.run_modal();
      _wb->_frontendCallbacks->show_status_text("");
      _wb->save_instances();
      refresh_home_connections();
      break;
    }

    case HomeScreenAction::ActionEditSQLScript: {
      break;
    }

    case HomeScreenAction::ActionOpenEERModel: {
      // Note: wb->open_document has an own GUILock, so we must not set another one here.
      std::string filename = _wb->_frontendCallbacks->show_file_dialog("open", _("Open Workbench Model"), "mwb");
      if (!filename.empty())
        _wb->open_document(filename);
      else
        _wb->_frontendCallbacks->show_status_text("Cancelled");
      break;
    }

    case HomeScreenAction::ActionOpenEERModelFromList: {
      // Note: wb->open_document has an own GUILock, so we must not set another one here.
      if (!anyObject.isNull()) {
        std::string path = anyObject;
        _wb->_frontendCallbacks->show_status_text(strfmt("Opening %s...", path.c_str()));
        _wb->open_document(path);
      }
      break;
    }

    case HomeScreenAction::ActionNewEERModel:
      _wb->new_document();
      break;

    case HomeScreenAction::ActionNewModelFromDB: {
      _wb->new_document();
      if (_wb->get_document().is_valid()) {
        ArgumentPool args;

        // delete the default schema
        if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() > 0)
          _wb->get_document()->physicalModels()[0]->catalog()->schemata().remove(0);

        _wb->update_plugin_arguments_pool(args);
        args.add_entries_for_object("activeCatalog", _wb->get_document()->physicalModels()[0]->catalog(), "db.Catalog");

        _wb->execute_plugin("db.plugin.database.rev_eng", args);

        // if the model is still empty, just close it
        if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() == 0)
          _wb->close_document();
      } else
        _wb->_frontendCallbacks->show_status_text("Error creating document");
      break;
    }

    case HomeScreenAction::ActionNewModelFromScript: {
      _wb->new_document();
      if (_wb->get_document().is_valid()) {
        ArgumentPool args;

        // delete the default schema
        if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() > 0)
          _wb->get_document()->physicalModels()[0]->catalog()->schemata().remove(0);

        _wb->update_plugin_arguments_pool(args);
        args.add_entries_for_object("activeCatalog", _wb->get_document()->physicalModels()[0]->catalog(), "db.Catalog");
        _wb->execute_plugin("db.mysql.plugin.import.sql", args);

        // if the model is still empty, just close it
        if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() == 0)
          _wb->close_document();
      } else
        _wb->_frontendCallbacks->show_status_text("Error creating document");
      break;
    }

    case HomeScreenAction::ActionOpenBlog:
      _command_ui->activate_command("builtin:web_mysql_blog");
      break;

    case HomeScreenAction::ActionOpenDocs:
      _command_ui->activate_command("builtin:web_mysql_docs");
      break;

    case HomeScreenAction::ActionOpenForum:
      _command_ui->activate_command("builtin:web_mysql_forum");
      break;

    case HomeScreenAction::CloseWelcomeMessage:
      _connectionsSection->showWelcomeHeading(false);
      bec::GRTManager::get()->set_app_option("HomeScreen:HeadingMessage", grt::IntegerRef(0));
      break;

    case HomeScreenAction::RescanLocalServers:
      _wb->execute_plugin("wb.tools.createMissingLocalConnections", ArgumentPool());
      break;
    default:
      logError("Unknown Action.\n");
  }
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::refresh_home_connections(bool clear_state) {
  if (!_home_screen)
    return;

  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

  std::map<std::string, std::string> auto_save_files = WBContextSQLIDE::auto_save_sessions();

  _connectionsSection->clear_connections(clear_state);

  // If there are no connections defined yet then create entries for all currently installed
  // local servers (only if this is the first run, after application start).
  if (_initializing_home_screen && (connections->count() == 0)) {
    grt::Module *module = grt::GRT::get()->get_module("Workbench");
    if (module == NULL)
      throw std::logic_error("Internal error: can't find Workbench module.");

    grt::StringListRef arguments(grt::Initialized);
    module->call_function("createInstancesFromLocalServers", arguments);
  }

  std::vector<db_mgmt_ConnectionRef> invalid_connections;
  std::map<std::string, std::string> invalid_connection_ids;

  std::vector<db_mgmt_ConnectionRef> oldAuthList;
  for (grt::ListRef<db_mgmt_Connection>::const_iterator end = connections.end(), inst = connections.begin();
       inst != end; ++inst) {
    // Any connection with NULL driver will be considered invalid and so deleted
    if (!(*inst)->driver().is_valid()) {
      invalid_connections.push_back(*inst);
      invalid_connection_ids.insert(std::pair<std::string, std::string>((*inst)->id(), ""));
    } else {
      grt::DictRef dict((*inst)->parameterValues());
      if (dict.has_key("useLegacyAuth")) {
        if ((size_t)dict.get_int("useLegacyAuth", 0) == 0)
          (*inst)->parameterValues().remove("useLegacyAuth"); // If it's not used (old val), we silently remove it.
        else
          oldAuthList.push_back(*inst);
      }

      {
        std::string host_entry;
        if ((*inst)->driver().is_valid() && (*inst)->driver()->name() == "MysqlNativeSSH")
          host_entry = dict.get_string("sshUserName") + "@" + dict.get_string("sshHost");
        else {
          if ((*inst)->driver()->name() == "MySQLX") {
            std::string editor = "";
            if (dict.get_int("editorLanguage") == 0)
              editor = "sql";
            else if (dict.get_int("editorLanguage") == 1)
              editor = "js";
            else if (dict.get_int("editorLanguage") == 2)
              editor = "py";
            host_entry = strfmt("%s:///%s:%i", editor.c_str(), dict.get_string("hostName").c_str(),
                                (int)dict.get_int("port", 3306));
          } else
            host_entry = strfmt("%s:%i", dict.get_string("hostName").c_str(), (int)dict.get_int("port", 3306));
          if ((*inst)->driver().is_valid() && (*inst)->driver()->name() == "MysqlNativeSocket") {
            // TODO: what about the default for sockets (only have a default for the pipe name)?
            std::string socket = dict.get_string("socket", "MySQL"); // socket or pipe
            host_entry = "Localhost via pipe " + socket;
          }
        }

        std::string title = *(*inst)->name();
        if (auto_save_files.find((*inst)->id()) != auto_save_files.end())
          title += " (auto saved)";

        _connectionsSection->addConnection((*inst).id(), title, host_entry, dict.get_string("userName"),
                                           dict.get_string("schema"));
      }
    }
  }

  _connectionsSection->updateFocusableAreas();

  _oldAuthList = oldAuthList;

  // Deletes invalid connections
  for (std::vector<db_mgmt_ConnectionRef>::const_iterator iterator = invalid_connections.begin();
       iterator != invalid_connections.end(); ++iterator) {
    logWarning("Invalid connection detected %s, deleting it\n", (*iterator)->name().c_str());
    remove_connection(*iterator);
  }
  // XXXX TEST PROBLEM
  // _wb->save_connections();
  _wb->save_instances();
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::refresh_home_documents() {
  if (!_home_screen)
    return;

  _documentsSection->clear_documents();

  std::map<std::string, std::string> auto_save_models(WBContextModel::auto_save_files());

  // find list of auto-saved models that are not in the list of recent files
  for (std::map<std::string, std::string>::const_iterator iter = auto_save_models.begin();
       iter != auto_save_models.end(); ++iter) {
    bool found = false;
    for (grt::StringListRef::const_iterator end = _wb->get_root()->options()->recentFiles().end(),
                                            f = _wb->get_root()->options()->recentFiles().begin();
         f != end; ++f) {
      if (*f == iter->first || strcmp(base::basename((*f)).c_str(), iter->first.c_str()) == 0) {
        found = true;
        break;
      }
    }

    if (!found)
      _documentsSection->add_document(iter->second, 0, wb::ModelFile::read_comment(iter->first.c_str()), 0);
  }

  // Create list entries for each recently opened file.
  // First count how many there are so we can add examples if the list is empty.
  int model_count = 0;
  grt::StringListRef recentFiles(_wb->get_root()->options()->recentFiles());
  for (grt::StringListRef::const_iterator end = recentFiles.end(), f = recentFiles.begin(); f != end; ++f) {
    if (g_str_has_suffix((*f).c_str(), ".mwb"))
      model_count++;
  }

  int sample_models_already_shown = _wb->read_state("WBSampleModelFilesShown", "home", 0);
  if (sample_models_already_shown == 0) {
    // If there is no entry in the MRU list then scan the examples folder for the Sakila model
    // file and add this as initial entry, so that a new user has an easy start.
    if (model_count == 0) {
      std::list<std::string> examples_paths;
#ifdef _MSC_VER
      examples_paths.push_back(mforms::Utilities::get_special_folder(mforms::WinProgramFilesX86) +
                               "\\MySQL\\Samples and Examples 5.5");
      examples_paths.push_back(mforms::Utilities::get_special_folder(mforms::WinProgramFiles) +
                               "\\MySQL\\Samples and Examples 5.5");
      examples_paths.push_back(bec::GRTManager::get()->get_basedir() + "/extras");
#elif defined(__APPLE__)
      examples_paths.push_back(bec::GRTManager::get()->get_basedir() + "/../SharedSupport");
#else
      examples_paths.push_back(bec::GRTManager::get()->get_basedir() + "/extras");
#endif

      for (std::list<std::string>::const_iterator path_iterator = examples_paths.begin();
           path_iterator != examples_paths.end(); path_iterator++) {
        if (g_file_test(path_iterator->c_str(), G_FILE_TEST_IS_DIR)) {
          std::string pattern = base::makePath(*path_iterator, "*.mwb");
          std::list<std::string> sample_model_files = base::scan_for_files_matching(pattern, true);
          for (std::list<std::string>::const_iterator iterator = sample_model_files.begin();
               iterator != sample_model_files.end(); iterator++)
            recentFiles->insert_checked(grt::StringRef(*iterator));
        }
      }
    }
    _wb->save_state("WBSampleModelFilesShown", "home", 1);
  }

  for (grt::StringListRef::const_iterator end = recentFiles.end(), f = recentFiles.begin(); f != end; ++f) {
#ifdef _MSC_VER
    struct _stat stbuf;
#else
    struct stat stbuf;
#endif
    if (base_stat((*f).c_str(), &stbuf) < 0)
      _documentsSection->add_document(*f, 0, "", 0);
    else
      _documentsSection->add_document(*f, stbuf.st_mtime, wb::ModelFile::read_comment(*f), stbuf.st_size);
  }
}

//--------------------------------------------------------------------------------------------------
