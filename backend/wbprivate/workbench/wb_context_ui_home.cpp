/*
 * Copyright (c) 2008, 2014, Oracle and/or its affiliates. All rights reserved.
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

#include <glib/gstdio.h>

#include "base/file_functions.h"
#include "base/file_utilities.h"
#include "base/string_utilities.h"
#include "base/log.h"
#include "base/ui_form.h"

#include "mforms/menu.h"

#include "grtpp.h"

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

#include "home_screen.h"
#include <zip.h>

DEFAULT_LOG_DOMAIN(DOMAIN_WB_CONTEXT_UI);

using namespace bec;
using namespace wb;
using namespace base;

/**
 * Helper method to construct a human-readable server description.
 */
std::string get_server_info(db_mgmt_ServerInstanceRef instance)
{
  std::string text;
  std::string system = instance->serverInfo().get_string("sys.system");

  if (instance->serverInfo().get_int("remoteAdmin"))
    text = strfmt("Host: %s  Type: %s", instance->loginInfo().get_string("ssh.hostName").c_str(),
      system.c_str());
  else
  {
    if (instance->serverInfo().get_int("windowsAdmin")/* || system == "Windows"*/)
    {
      std::string host = instance->loginInfo().get_string("wmi.hostName");
      if (host == "localhost" || host.empty() || host == "127.0.0.1")
      {
        text = "Local  Type: Windows";
      }
      else
        text = "Host: " + host + "  Type: Windows";
    }
    else
    {
      std::string host = instance->connection().is_valid() ?
        instance->connection()->parameterValues().get_string("hostName") : "Invalid";

      if (host == "localhost" || host.empty() || host == "127.0.0.1")
      {
        text = strfmt("Local  Type: %s", system.c_str());
      }
      else
        text = strfmt("Host: %s  Type: DB Only", host.c_str());
    }
  }

  return text;
}

//--------------------------------------------------------------------------------------------------

  template <class T>
  void get_groups_for_movement(grt::ListRef<T> items, const grt::ValueRef &object, std::vector<std::string> &groups)
  {
    grt::Ref<T> item = grt::Ref<T>::cast_from(object);

    std::string item_name = item->name();
    size_t current_group_separator_position = item_name.find("/");

    std::string current_group = "";
    
    if (current_group_separator_position != std::string::npos)
    {
      current_group = item_name.substr(0, current_group_separator_position);
      groups.push_back("*Ungrouped*");
    }

    for (typename grt::ListRef<T>::const_iterator end = items.end(),
          inst = items.begin(); inst != end; ++inst)
    {
      std::string name = (*inst)->name();
      size_t group_separator_position = name.find("/");

      if (group_separator_position != std::string::npos)
      {
        std::string group_name = name.substr(0, group_separator_position);

        bool found = false;
        for (std::vector<std::string>::iterator end = groups.end(),
            index = groups.begin(); index != end && !found; index++)
        {
          found = (*index).compare(0, group_separator_position, group_name) == 0;
        }

        if (!found && group_name != current_group)
          groups.push_back(group_name);
      }
    }
  }

  template <class T>
  bool validate_group_for_movement(grt::ListRef<T> items, const grt::ValueRef &object, std::string group)
  {
    bool ret_val = false;

    // Validates the group is valid...
    size_t group_position = group.find("/");

    if( group.length() == 0 )
      Utilities::show_warning(_("Move To Group"), _("You must select the target group from the list or type a new group."),  _("Ok"));
    else if (group_position != std::string::npos)
      Utilities::show_warning(_("Move To Group"), _("The selected group is invalid, should not contain the \"/\" character."),  _("Ok"));
    else
    {
      // Gets the connection and connection list...
      grt::Ref<T> item = grt::Ref<T>::cast_from(object);

      // Creates the new connection name...
      std::string item_name = item->name();
      std::string new_item_name = "";

      group_position = item_name.find("/");

      if (group == "*Ungrouped*")
        new_item_name = item_name.substr(group_position + 1);
      else
      {
        if (group_position != std::string::npos)
          new_item_name = group + "/" + item_name.substr(group_position + 1);
        else
          new_item_name = group + "/" + item_name;
      }

      size_t item_index = bec::find_list_ref_item_position<T>(items, new_item_name, MatchAny, NULL, FindFull);

      if (item_index != grt::BaseListRef::npos)
        Utilities::show_warning(_("Move To Group"), _("Unable to perform the movement as there's an entry with the same name in the target group"),  _("Ok"));
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
    else
    {
      if (current_group_separator_position != std::string::npos)
        new_item_name = group + "/" + item_name.substr(current_group_separator_position + 1);
      else
        new_item_name = group + "/" + item_name;
    }

    item->name(new_item_name);
  }

  template <class T>
  void move_item_to_group(std::string group, grt::ListRef<T> items, const grt::ValueRef &object)
  {
    size_t current_item_index, current_group_index, sibling_index, target_index;
    bool sibling_move = false;
    bool item_move = false;
    
    // To ensure the grouping order is maintained after a move to group operation it is needed to consider
    // - If the item being moved is in a group or not
    // - If the item is being moved to a group or not
    // - Removing the first item from a group may affect the group ordering, to prevent that, the next item in the group 
    //   is being moved to the position of the first item
    // - When moved to a group, the item will be placed after the last element in the group
    // - Moving to a new group or ungrouping the item doesn't affect it's position on the list

    //grt::Type object_type = object.type();
    
    grt::Ref<T> item = grt::Ref<T>::cast_from(object);
    std::string item_name = item->name();

    std::string item_group = "";
    size_t group_indicator_position = item_name.find("/");

    // Gets the position of the item being moved...
    current_item_index = find_list_ref_item_position<T>(items, item_name);
    if (group_indicator_position != std::string::npos)
    {
      item_group = item_name.substr(0, group_indicator_position + 1);

      // Gets the position of the first element on the group
      current_group_index = find_list_ref_item_position<T>(items, item_group);

      if (current_item_index == current_group_index)
      {
        sibling_index = find_list_ref_item_position<T>(items, item_group, MatchAfter, &item);

        if (sibling_index != grt::BaseListRef::npos)
          sibling_move = true;
      }
    }

    if (group != "*Ungrouped*")
    {
      // Gets the position of the last element on the target group if exists ( happily the function does that when passing MatchBefore without a reference )
      std::string tmp_group = group + "/";
      target_index = find_list_ref_item_position<T>(items, tmp_group , MatchLast);

      if (target_index != grt::BaseListRef::npos)
      {
        item_move = true;

        if (target_index < current_item_index)
          target_index++;
      }
    }

    if (sibling_move)
    {
      items.reorder(sibling_index, current_item_index);

      if (sibling_index > current_item_index)
        current_item_index++;
    }

    if (item_move)
      items.reorder(current_item_index, target_index);

    update_item_group<T>(object, group);
  }  

//--------------------------------------------------------------------------------------------------

void WBContextUI::show_about()
{
  AboutBox::show_about(_wb->get_root()->info()->edition());
}

//--------------------------------------------------------------------------------------------------

/**
 * Creates the main home screen (Workbench Central, Workspace) if not yet done and docks it to 
 * the main application window.
 */
void WBContextUI::show_home_screen()
{
  if (_home_screen != NULL)
  {
    mforms::App::get()->select_view(_home_screen);
    return;
  }

  _initializing_home_screen = (_home_screen == NULL);
  if (_home_screen == NULL)
  {    
    _home_screen = mforms::manage(new HomeScreen(_command_ui, _wb->get_root()->rdbmsMgmt()));
    _home_screen->set_on_close(boost::bind(&WBContextUI::home_screen_closing, this));
    _home_screen->set_callback((home_screen_action_callback)&WBContextUI::home_action_callback, this);
    _home_screen->handle_context_menu = boost::bind(&WBContextUI::handle_home_context_menu, this, _1, _2);

    // Setup context menus.
    mforms::Menu *menu;

    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      groups.push_back("Menu/Home/Connections");
      bec::MenuItemList pitems = _wb->get_grt_manager()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty())
      {
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
      argument_pool.add_entries_for_object("selectedConnection", db_mgmt_ConnectionRef(_wb->get_grt()), "db.mgmt.Connection");
      groups.push_back("Menu/Home/Connections");
      bec::MenuItemList pitems = _wb->get_grt_manager()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty())
      {
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

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Move To Top"), "move_connection_to_top");
    menu->add_item(_("Move Up"), "move_connection_up");
    menu->add_item(_("Move Down"), "move_connection_down");
    menu->add_item(_("Move To End"), "move_connection_to_end");
    menu->add_item(_("Delete Group..."), "delete_connection_group");

    _home_screen->set_menu(menu, HomeMenuConnectionGroup);

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Move To Top"), "move_connection_to_top");
    menu->add_item(_("Move Up"), "move_connection_up");
    menu->add_item(_("Move Down"), "move_connection_down");
    menu->add_item(_("Move To End"), "move_connection_to_end");

    menu->add_separator();
    menu->add_item(_("Edit Connection..."), "edit_connection");
    menu->add_item(_("Delete Connection..."), "delete_fabric_connection");

    menu->add_separator();
    menu->add_item(_("Delete Connections to Managed Servers..."), "delete_fabric_connection_servers");

    _home_screen->set_menu(menu, HomeMenuConnectionFabric);
    
    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Open Model"), "open_model_from_list");
    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      argument_pool.add_simple_value("selectedModelFile", grt::ValueRef());
      groups.push_back("Menu/Home/ModelFiles");
      bec::MenuItemList pitems = _wb->get_grt_manager()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty())
      {
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

    menu = mforms::manage(new mforms::Menu());
    menu->add_item(_("Create EER Model from Database"), "model_from_schema");
    menu->add_item(_("Create EER Model from Script"), "model_from_script");
    {
      std::list<std::string> groups;
      bec::ArgumentPool argument_pool;
      _wb->update_plugin_arguments_pool(argument_pool);
      argument_pool.add_simple_value("selectedModelFile", grt::ValueRef());
      groups.push_back("Menu/Home/ModelFiles"); // TODO: do we need a different group for the action menu?
      bec::MenuItemList pitems = _wb->get_grt_manager()->get_plugin_context_menu_items(groups, argument_pool);
      if (!pitems.empty())
      {
        menu->add_separator();
        for (bec::MenuItemList::const_iterator iterator = pitems.begin(); iterator != pitems.end(); iterator++)
          menu->add_items_from_list(pitems);
      }      
    }

    _home_screen->set_menu(menu, HomeMenuDocumentModelAction);
  }

  mforms::App::get()->dock_view(_home_screen, "maintab");
  
  std::string error;
  try
  {
    refresh_home_documents();
    refresh_home_connections(true);
    refresh_home_starters();
  }
  catch (const std::exception *exc)
  { error = exc->what(); }
  catch (const std::exception &exc)
  { error = exc.what(); }
  catch (...)
  { error = "(unknown)"; }

  if (!error.empty())
  {
    std::string message = base::strfmt(_("Error while setting up home screen. The error message is: %s"), error.c_str());
    log_error("%s\n", message.c_str());
    mforms::Utilities::show_error(_("Home Screen Error"), message, _("Close"));
  }

  _home_screen->setup_done();
  _initializing_home_screen = false;
}

//--------------------------------------------------------------------------------------------------

/**
 * Called when the home screen is closed by the UI. We have to clear our reference then.
 */
bool WBContextUI::home_screen_closing()
{  
  if (_home_screen)
    _home_screen->release();
  _home_screen = NULL;
  return true;
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::home_action_callback(HomeScreenAction action, const grt::ValueRef &object, WBContextUI *self)
{
  try
  {
    self->handle_home_action(action, object);
  }
  catch (const std::exception &exc)
  {
    std::string message = base::strfmt("Exception caught while processing action from home screen: %s", exc.what());
    log_error("%s\n", message.c_str());
    mforms::Utilities::show_error("Internal Error", message, _("Close"));
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Removes a connection from the stored connections list along with all associated data
 * (including its server instance entry).
 */
void WBContextUI::remove_connection(const db_mgmt_ConnectionRef &connection)
{
  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
  grt::ListRef<db_mgmt_ServerInstance> instances = _wb->get_root()->rdbmsMgmt()->storedInstances();

  // Remove all associated server instances.
  for (ssize_t i = instances.count() - 1; i >= 0; --i)
  {
    db_mgmt_ServerInstanceRef instance(instances[i]);
    if (instance->connection() == connection)
      instances->remove(i);
  }

  // Remove password associated with this connection (if stored in keychain/vault). Check first
  // this service/username combination isn't used anymore by other connections.
  bool credentials_still_used = false;
  grt::DictRef parameter_values = connection->parameterValues();
  std::string host = connection->hostIdentifier();
  std::string user = parameter_values.get_string("userName");
  for (grt::ListRef<db_mgmt_Connection>::const_iterator i = connections.begin();
    i != connections.end(); ++i)
  {
    if (*i != connection)
    {
      grt::DictRef current_parameters = (*i)->parameterValues();
      if (host == *(*i)->hostIdentifier() && user == current_parameters.get_string("userName"))
      {
        credentials_still_used = true;
        break;
      }
    }
  }
  if (!credentials_still_used)
    mforms::Utilities::forget_password(host, user);

  connections->remove(connection);
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::handle_home_context_menu(const grt::ValueRef &object, const std::string &action)
{
  if (action == "open_connection")
  {
    handle_home_action(ActionOpenConnectionFromList, object);
  }
  else if (action == "delete_connection" || action == "delete_fabric_connection" || action == "delete_fabric_connection_servers")
  {
    db_mgmt_ConnectionRef connection(db_mgmt_ConnectionRef::cast_from(object));
    
    std::string name = connection->name();
    std::string title;
    std::string warning;

    if (action == "delete_fabric_connection_servers")
    {
      title = _("Delete Managed Server Connections");
      warning = strfmt(_("Do you really want to delete managed server connections on this fabric node: %s?"), name.c_str());
    }
    else
    {
      title = _("Delete Connection");
      warning = strfmt(_("Do you want to delete connection %s?"), name.c_str());
    }

    int answer = Utilities::show_warning(title, warning,  _("Delete"), _("Cancel"));
    if (answer == mforms::ResultOk)
    {
      // In case it is not a single connection, implies it is a fabric connection and we need to
      // delete the connections to the managed servers
      if (action != "delete_connection")
      {
        handle_home_context_menu(grt::StringRef(name), "internal_delete_connection_group");
        connection->parameterValues().set("connections_created", grt::IntegerRef(0));
      }

      // Connection is not deleted when we are just deleting the children of a fabric connection
      if (action != "delete_fabric_connection_servers")
        remove_connection(connection);

      refresh_home_connections();
    }
  }
  else if (action == "manage_connections" || action == "edit_connection")
  {
    handle_home_action(ActionManageConnections, object);
  }
  else if (action == "move_connection_to_top")
  {
    // We enter here for both: groups and connections. The object for groups must be a StringRef
    // with the name of the group. For connections it's the connection ref.
    // Similar for the other move_* actions.
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(MoveTop, connections, object);
    refresh_home_connections();
  }
  else if (action == "move_connection_up")
  {
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(MoveUp, connections, object);
    refresh_home_connections();
  }
  else if (action == "move_connection_down")
  {
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(MoveDown, connections, object);

    refresh_home_connections();
  }
  else if (action == "move_connection_to_end")
  {
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
    bec::move_list_ref_item<db_mgmt_Connection>(MoveBottom, connections, object);

    refresh_home_connections();
  }
  else if (action == "move_connection_to_group")
  {
    grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

    // Create the available groups for the movement...
    std::vector<std::string> groups;
    get_groups_for_movement<db_mgmt_Connection>(connections, object, groups);

    SelectOptionDialog dialog(_("Move To Group"), _("Pick a group to move the selected connection "
      "to\nor type a name to move it to a new one."), groups);
    dialog.set_validation_function(boost::bind(&validate_group_for_movement<db_mgmt_Connection>, connections, object, _1));
    std::string result = dialog.run();

    // At this point the movement is considered valid so we just do it.
    if( result != "")
    {
      move_item_to_group<db_mgmt_Connection>(result, connections, object);
      refresh_home_connections();
    }
  }
  else if (action == "delete_connection_group" || action == "internal_delete_connection_group")
  {
    std::string group = object.repr();
    int answer = mforms::ResultOk;
    

    // Internal deletion does not require the prompt
    if (action == "delete_connection_group")
    {
      std::string text= strfmt(_("Do you really want to delete all the connections in group: %s?"), group.c_str());
      answer = Utilities::show_warning(_("Delete Connection Group"), text,  _("Delete"), _("Cancel"));
    }
    
    if (answer == mforms::ResultOk)
    {
      group += "/";
      size_t group_length = group.length();

      std::vector<db_mgmt_ConnectionRef> candidates;
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

      ssize_t index = connections.count() - 1;
      while (index >= 0)
      {
        std::string name = connections[index]->name();

        if (name.compare(0, group_length, group) == 0)
          candidates.push_back(connections[index]);

        index--;
      }

      for (std::vector<db_mgmt_ConnectionRef>::const_iterator iterator = candidates.begin();
        iterator != candidates.end(); ++iterator)
        remove_connection(*iterator);

      // Internal deletion does not require the UI update
      if (action == "delete_connection_group")
        refresh_home_connections();
    }
  }
  else if (action == "delete_connection_all")
  {
    std::string text= _("Do you really want to delete all defined connections?");
    int answer= Utilities::show_warning(_("Delete All Connections"), text,  _("Delete"), _("Cancel"));
    if (answer == mforms::ResultOk)
    {
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
      while (connections->count() > 0)
        remove_connection(connections[0]);
      refresh_home_connections();
    }
  }
  else if (action == "open_model_from_list")
  {
    handle_home_action(ActionOpenEERModelFromList, object);
  }
  else if (action == "model_from_schema")
  {
    handle_home_action(ActionNewModelFromDB, object);
  }
  else if (action == "model_from_script")
  {
    handle_home_action(ActionNewModelFromScript, object);
  }
  else if (action == "show_model")
  {
    std::string file = grt::StringRef::cast_from(object);
    mforms::Utilities::reveal_file(file);
  }
  else if (action == "remove_model")
  {
    _wb->get_root()->options()->recentFiles()->remove(object);

    bool remove_auto_save = false;
    std::string file = grt::StringRef::cast_from(object);
    if (file.size() > 5 && file.substr(file.size()-5) == ".mwbd")
      remove_auto_save = true;
    else
    {
      std::map<std::string, std::string> auto_save_models(WBContextModel::auto_save_files());

      if (auto_save_models.find(file) != auto_save_models.end())
      {
        file = auto_save_models[file];
        remove_auto_save = true;
      }
      else if (auto_save_models.find(base::basename(file)) != auto_save_models.end())
      {
        file = auto_save_models[base::basename(file)];
        remove_auto_save = true;
      }
    }
    
    if (remove_auto_save)
    {
      try
      {
        base::remove_recursive(file);

        // Refreshes the list of autosaved files...
        WBContextModel::detect_auto_save_files(_wb->get_auto_save_dir());
      }
      catch (std::exception &exc)
      {
        log_error("Error removing model file %s: %s\n", file.c_str(), exc.what());
      }
    }
    refresh_home_documents();
  }
  else if (action == "remove_model_all")
  {
    std::string text= _("Do you want to remove all entries from the model list?");
    int answer= Utilities::show_warning(_("Clear Model Entry List"), text,  _("Delete"), _("Cancel"));
    if (answer == mforms::ResultOk)
    {
      grt::StringListRef file_names(_wb->get_root()->options()->recentFiles());
      for (ssize_t index = file_names->count() - 1; index >= 0; index--)
      {
        if (g_str_has_suffix(file_names[index].c_str(), ".mwb"))
          file_names->remove(index);
      }
      refresh_home_documents();
    }
  }
  else 
  {
    bec::ArgumentPool argument_pool;
    _wb->update_plugin_arguments_pool(argument_pool);

    if (db_mgmt_ConnectionRef::can_wrap(object))
    {
      argument_pool.add_entries_for_object("selectedConnection", db_mgmt_ConnectionRef::cast_from(object));
      get_command_ui()->activate_command(action, argument_pool);
    }
    else
      if (grt::StringRef::can_wrap(object))
      {
        argument_pool.add_simple_value("selectedModelFile", grt::StringRef::cast_from(object));
        get_command_ui()->activate_command(action, argument_pool);
      }
      else // Any other command.
        get_command_ui()->activate_command(action, argument_pool);
  }
}


//--------------------------------------------------------------------------------------------------

void WBContextUI::start_plugin(const std::string& title, const std::string& command, bool force_external)
{
  try
  {
    std::string message_title= base::strfmt(_("Starting %s"), title.c_str());
    GUILock lock(_wb, message_title, _("Please stand by while the plugin is started..."));
    if (base::starts_with(command, "plugin:"))
    {
      _wb->execute_plugin(command.substr(7, command.length()));
    }
    else
      if (base::starts_with(command, "browse:"))
        show_web_page(command.substr(7, command.length()), !force_external);
      else
        if (base::starts_with(command, "http://"))
          show_web_page(command, false);
  }
  catch (const std::exception &exc)
  {
    std::string message = base::strfmt(_("Could not open link or plugin. The error message is: %s"), exc.what());
    log_error("%s\n", message.c_str());
    mforms::Utilities::show_error(_("Open Plugin Error"), message, _("Close"));
  }
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::handle_home_action(HomeScreenAction action, const grt::ValueRef &object)
{
  switch (action)
  {
    case ActionNone:
      break;

    case ActionShortcut:
    {
      app_StarterRef starter = app_StarterRef::cast_from(object);
      start_plugin(starter->title(), starter->command());
    }
    
    break;
      
    case ActionRemoveShortcut:
    {
      app_StarterRef starter = app_StarterRef::cast_from(object);
      _wb->get_root()->starters()->displayList()->remove(starter);
      _wb->save_starters();
      refresh_home_starters();
    }
      break;
      
    case ActionOpenConnectionFromList:
    {
      if (_processing_action_open_connection)
        break;
      _processing_action_open_connection = true;
      if (object.is_valid())
      {
        db_mgmt_ConnectionRef connection(db_mgmt_ConnectionRef::cast_from(object));

        _wb->show_status_text("Opening SQL Editor...");
        _wb->add_new_query_window(connection);
      }
      _processing_action_open_connection = false;
      break;
    }
      
    case ActionFilesWithConnection:
      {
        if (_processing_action_open_connection)
          break;

        _processing_action_open_connection = true;
        if (object.is_valid())
        {
          grt::DictRef dict = grt::DictRef::cast_from(object);
          db_mgmt_ConnectionRef connection = db_mgmt_ConnectionRef::cast_from(dict["connection"]);
          grt::StringListRef names = grt::StringListRef::cast_from(dict["files"]);
          _wb->show_status_text("Opening files in new SQL Editor ...");          
          boost::shared_ptr<SqlEditorForm> form = _wb->add_new_query_window(connection, false);
          if (form)
          {
            for (size_t i = 0; i < names->count(); ++i)
              form->open_file(names[i], true);
            form->update_title();
          }
        }

        _processing_action_open_connection = false;
        break;
      }

    case ActionNewConnection:
    {
      NewConnectionWizard wizard(_wb, _wb->get_root()->rdbmsMgmt());
      wizard.set_title("Setup New Connection");
      wizard.run();
      refresh_home_connections();
      break;
    }

    case ActionManageConnections:
    {
      ServerInstanceEditor editor(_wb->get_grt_manager(), _wb->get_root()->rdbmsMgmt());
      _wb->show_status_text("Connection Manager Opened");
      editor.run(db_mgmt_ConnectionRef::cast_from(object));
      _wb->show_status_text("");
      refresh_home_connections();
      break;
    }

    case ActionMoveConnection:
    {
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

      grt::DictRef dict = grt::DictRef::cast_from(object);
      ssize_t to = grt::IntegerRef::cast_from(dict["to"]);
      if (db_mgmt_ConnectionRef::can_wrap(dict["object"]))
      {
        db_mgmt_ConnectionRef connection = db_mgmt_ConnectionRef::cast_from(dict["object"]);
        move_list_ref_item<db_mgmt_Connection>(connections, connection, to);
        refresh_home_connections();
      }
      else
      {
        if (grt::StringRef::can_wrap(dict["object"]))
        {
          grt::StringRef group = grt::StringRef::cast_from(dict["object"]);
          move_list_ref_item<db_mgmt_Connection>(connections, group, to);
          refresh_home_connections();
        }

      }
      break;
    }

    case ActionMoveConnectionToGroup:
    {
      grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());
      db_mgmt_ConnectionRef connection;

      grt::DictRef dict = grt::DictRef::cast_from(object);
      std::string group = grt::StringRef::cast_from(dict["group"]);
      connection = db_mgmt_ConnectionRef::cast_from(dict["object"]);
      if (group != "" && connection.is_valid())
      {
        move_item_to_group<db_mgmt_Connection>(group, connections, connection);
        refresh_home_connections();
      }
      break;
    }
    
    case ActionCreateFabricConnections:
    {
      // Creates the fabric connections only if they have not been already created
      // since the last connection refresh
      GUILock lock(_wb, _("Connecting to MySQL Fabric Management Node"), _("The connections to the managed MySQL Servers will be created in a moment.\n"
                                                                     "\nPlease stand by..."));
      _wb->show_status_text(_("Connecting to MySQL Fabric Management Node..."));
      db_mgmt_ConnectionRef connection(db_mgmt_ConnectionRef::cast_from(object));
      grt::GRT *grt = connection->get_grt();
      grt::BaseListRef args(grt);
      args->insert_unchecked(connection);
      
      grt::ValueRef result = grt->call_module_function("WBFabric", "createConnections", args);
      std::string error = grt::StringRef::extract_from(result);
      
      if (error.length())
      {
        mforms::Utilities::show_error(_("MySQL Fabric Connection Error"), error, "OK");
        _wb->show_status_text(_("Failed creating connections to Managed MySQL Servers..."));
      }
      else
      {
        // Sets the flag to indicate the connections have been crated for this fabric node
        connection->parameterValues().set("connections_created", grt::IntegerRef(1));
        _wb->show_status_text(_("Created connections to Managed MySQL Servers..."));
      }
      
      break;
    }
      
    case ActionSetupRemoteManagement:
    {
      NewServerInstanceWizard wizard(_wb, db_mgmt_ConnectionRef::cast_from(object));
      _wb->show_status_text("Started Management Setup Wizard");
      wizard.run_modal();
      _wb->show_status_text("");
      _wb->save_instances();
      refresh_home_connections();
      break;
    }

    case ActionEditSQLScript:
    {
      break;
    }

    case ActionOpenEERModel:
      {
        // Note: wb->open_document has an own GUILock, so we must not set another one here.
        std::string filename = _wb->show_file_dialog("open", _("Open Workbench Model"), "mwb");
        if (!filename.empty())
          _wb->open_document(filename);
        else
          _wb->show_status_text("Cancelled");
        break;
      }

    case ActionOpenEERModelFromList:
    {
      // Note: wb->open_document has an own GUILock, so we must not set another one here.
      if (object.is_valid())
      {
        std::string path = *grt::StringRef::cast_from(object);
        _wb->show_status_text(strfmt("Opening %s...", path.c_str()));
        _wb->open_document(path);
      }
      break;
    }

    case ActionNewEERModel:
      _wb->new_document();
      break;

    case ActionNewModelFromDB:
      {
        _wb->new_document();          
        if (_wb->get_document().is_valid())
        {
          ArgumentPool args;

          // delete the default schema
          if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() > 0)
            _wb->get_document()->physicalModels()[0]->catalog()->schemata().remove(0);

          _wb->update_plugin_arguments_pool(args);
          args.add_entries_for_object("activeCatalog", _wb->get_document()->physicalModels()[0]->catalog(),
            "db.Catalog");

          _wb->execute_plugin("db.plugin.database.rev_eng", args);

          // if the model is still empty, just close it
          if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() == 0)
            _wb->close_document();
        }
        else
          _wb->show_status_text("Error creating document");
        break;
      }

    case ActionNewModelFromScript:
      {
        _wb->new_document();
        if (_wb->get_document().is_valid())
        {
          ArgumentPool args;

          // delete the default schema
          if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() > 0)
            _wb->get_document()->physicalModels()[0]->catalog()->schemata().remove(0);

          _wb->update_plugin_arguments_pool(args);
          args.add_entries_for_object("activeCatalog", _wb->get_document()->physicalModels()[0]->catalog(),
            "db.Catalog");
          _wb->execute_plugin("db.mysql.plugin.import.sql", args);

          // if the model is still empty, just close it
          if (_wb->get_document()->physicalModels()[0]->catalog()->schemata().count() == 0)
            _wb->close_document();
        }
        else
          _wb->show_status_text("Error creating document");
        break;
      }
  }
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::refresh_home_connections(bool initial_load)
{
  if (!_home_screen)
    return;

  grt::ListRef<db_mgmt_Connection> connections(_wb->get_root()->rdbmsMgmt()->storedConns());

  std::map<std::string, std::string> auto_save_files = WBContextSQLIDE::auto_save_sessions();
  
  _home_screen->clear_connections();

  // If there are no connections defined yet then create entries for all currently installed
  // local servers (only if this is the first run, after application start).
  if (_initializing_home_screen && (connections->count() == 0))
  {
    grt::GRT* grt = _wb->get_grt();
    grt::Module* module = grt->get_module("Workbench");
    if (module == NULL)
      throw std::logic_error("Internal error: can't find Workbench module.");
    
    grt::StringListRef arguments(grt);
    module->call_function("createInstancesFromLocalServers", arguments);
  }


  std::vector<db_mgmt_ConnectionRef> fabric_managed;

  for (grt::ListRef<db_mgmt_Connection>::const_iterator end = connections.end(),
      inst = connections.begin(); inst != end; ++inst)
  {
    grt::DictRef dict((*inst)->parameterValues());

    // Fabric managed connections will be deleted the initial loading
    if (initial_load && dict.has_key("fabric_managed"))
      fabric_managed.push_back(*inst);
    else
    {
      std::string host_entry;
      if ((*inst)->driver().is_valid() && (*inst)->driver()->name() == "MysqlNativeSSH")
        host_entry = dict.get_string("sshUserName") + "@" + dict.get_string("sshHost");
      else
      {
        // Fabric connectoins for which the managed server connections were created need to have the
        // flag reset on the initial loading
        if (initial_load && dict.has_key("connections_created"))
          dict.set("connections_created", grt::IntegerRef(0));

        host_entry = strfmt("%s:%i", dict.get_string("hostName").c_str(), (int) dict.get_int("port", 3306));
        if ((*inst)->driver().is_valid() && (*inst)->driver()->name() == "MysqlNativeSocket")
        {
          // TODO: what about the default for sockets (only have a default for the pipe name)?
          std::string socket= dict.get_string("socket", "MySQL"); // socket or pipe
          host_entry= "Localhost via pipe " + socket;
        }
      }

      std::string title = *(*inst)->name();
      if (auto_save_files.find((*inst)->id()) != auto_save_files.end())
        title += " (auto saved)";

      _home_screen->add_connection(*inst, title, host_entry,
        dict.get_string("userName"), dict.get_string("schema"));
    }
  }

  // Deletes the fabric managed connections
  for (std::vector<db_mgmt_ConnectionRef>::const_iterator iterator = fabric_managed.begin();
    iterator != fabric_managed.end(); ++iterator)
    remove_connection(*iterator);
  
  _wb->save_connections();
  _wb->save_instances();
}

//--------------------------------------------------------------------------------------------------

void WBContextUI::refresh_home_documents()
{
  if (!_home_screen)
    return;

  _home_screen->clear_documents();
  
  std::map<std::string, std::string> auto_save_models(WBContextModel::auto_save_files());

  // find list of auto-saved models that are not in the list of recent files
  for (std::map<std::string, std::string>::const_iterator iter = auto_save_models.begin();
       iter != auto_save_models.end(); ++iter)
  {
    bool found = false;
    for (grt::StringListRef::const_iterator end = _wb->get_root()->options()->recentFiles().end(),
         f = _wb->get_root()->options()->recentFiles().begin(); f != end; ++f)
    {
      if (*f == iter->first || strcmp(base::basename((*f)).c_str(), iter->first.c_str()) == 0)
      {
        found = true;
        break;
      }
    }

    if (!found)
      _home_screen->add_document(iter->second, 0, wb::ModelFile::read_comment(iter->first.c_str()), 0);
  }
  
  // Create list entries for each recently opened file.
  // First count how many there are so we can add examples if the list is empty.
  int model_count = 0;
  grt::StringListRef recentFiles(_wb->get_root()->options()->recentFiles());
  for (grt::StringListRef::const_iterator end = recentFiles.end(), f = recentFiles.begin();
    f != end; ++f)
  {
    if (g_str_has_suffix((*f).c_str(), ".mwb"))
      model_count++;
  }

  
  int sample_models_already_shown = _wb->read_state("WBSampleModelFilesShown", "home", 0);
  if (sample_models_already_shown == 0)
  {
    // If there is no entry in the MRU list then scan the examples folder for the Sakila model
    // file and add this as initial entry, so that a new user has an easy start.
    if (model_count == 0)
    {
      std::list<std::string> examples_paths;
#ifdef _WIN32
      examples_paths.push_back(mforms::Utilities::get_special_folder(mforms::WinProgramFilesX86) +
        "\\MySQL\\Samples and Examples 5.5");
      examples_paths.push_back(mforms::Utilities::get_special_folder(mforms::WinProgramFiles) +
        "\\MySQL\\Samples and Examples 5.5");
      examples_paths.push_back(_wb->get_grt_manager()->get_basedir()+"/extras");
#elif defined(__APPLE__)
      examples_paths.push_back(_wb->get_grt_manager()->get_basedir()+"/../SharedSupport");
#else
      examples_paths.push_back(_wb->get_grt_manager()->get_basedir()+"/extras");
#endif

      for (std::list<std::string>::const_iterator path_iterator = examples_paths.begin();
        path_iterator != examples_paths.end(); path_iterator++)
      {
        if (g_file_test(path_iterator->c_str(), G_FILE_TEST_IS_DIR))
        {
          std::string pattern = make_path(*path_iterator, "*.mwb");
          std::list<std::string> sample_model_files = base::scan_for_files_matching(pattern, true);
          for (std::list<std::string>::const_iterator iterator = sample_model_files.begin();
            iterator != sample_model_files.end(); iterator++)
            recentFiles->insert_checked(grt::StringRef(*iterator));
        }
      }
    }
    _wb->save_state("WBSampleModelFilesShown", "home", 1);
  }

  for (grt::StringListRef::const_iterator end = recentFiles.end(), f = recentFiles.begin();
    f != end; ++f)
  {
    
#ifdef _WIN32
    struct _stat stbuf;
#else
    struct stat stbuf;
#endif
    if (base_stat((*f).c_str(), &stbuf) < 0)
      _home_screen->add_document(*f, 0, "", 0);
    else
      _home_screen->add_document(*f, stbuf.st_mtime, wb::ModelFile::read_comment(*f), stbuf.st_size);
  }

}

//--------------------------------------------------------------------------------------------------

void WBContextUI::refresh_home_starters()
{
  if (!_home_screen)
    return;

  _home_screen->clear_shortcuts();
  
  grt::ListRef<app_Starter> starters= _wb->get_root()->starters()->displayList();
  for (grt::ListRef<app_Starter>::const_iterator iterator= starters.begin(); iterator != starters.end(); 
       iterator++)
  {
    _home_screen->add_shortcut(*iterator, (*iterator)->smallIcon());
  }
  _home_screen->set_needs_repaint();
}

//--------------------------------------------------------------------------------------------------
