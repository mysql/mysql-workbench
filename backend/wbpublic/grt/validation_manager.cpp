/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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

#include "stdafx.h"

#include "validation_manager.h"
#include "grt/grt_manager.h"

#include <algorithm>

//------------------------------------------------------------------------------
bec::ValidationMessagesBE::ValidationMessagesBE()
{
  _error_icon   = IconManager::get_instance()->get_icon_id("mini_error.png");
  _warning_icon = IconManager::get_instance()->get_icon_id("mini_warning.png");
  _info_icon    = IconManager::get_instance()->get_icon_id("mini_notice.png");

  scoped_connect(bec::ValidationManager::signal_notify(),boost::bind(&bec::ValidationMessagesBE::validation_message, this, _1, _2, _3, _4));
}

//------------------------------------------------------------------------------
void bec::ValidationMessagesBE::clear()
{
  _errors.clear();
  _warnings.clear();
}

//------------------------------------------------------------------------------
bool bec::ValidationMessagesBE::get_field(const bec::NodeId &node, int column, std::string &value)
{
  bool ret = false;
  if (column == bec::ValidationMessagesBE::Description)
  {
    const MessageList::size_type idx = node.end();
  
    if (idx < _errors.size())
      value = _errors[idx].msg;
    else
      value = _warnings[idx].msg;

    ret = true;
  }
  
  return ret;
}

//------------------------------------------------------------------------------
bec::IconId bec::ValidationMessagesBE::get_field_icon(const bec::NodeId &node, int column, IconSize)
{
  bec::IconId icon_id = _info_icon;
  
  if (column == bec::ValidationMessagesBE::Description)
  {
    const MessageList::size_type idx = node.end();

    if (idx < _errors.size())
      icon_id = _error_icon;
    else
      icon_id = _warning_icon;
  }
  
  return icon_id;
}

//------------------------------------------------------------------------------
int bec::ValidationMessagesBE::count()
{
  return _errors.size() + _warnings.size();
}

//------------------------------------------------------------------------------
int bec::ValidationMessagesBE::get_node_popup_items(const bec::NodeId& node, bec::MenuItemList& menu)
{
  return 0;
}

//------------------------------------------------------------------------------
void bec::ValidationMessagesBE::activate_node_popup_item(const bec::NodeId &node, const std::string &name)
{}

//------------------------------------------------------------------------------
bool bec::ValidationMessagesBE::match_message(const bec::ValidationMessagesBE::Message& m, const grt::ObjectRef& obj, const grt::Validator::Tag& tag)
{
  //g_message("obj == m.obj %i, tag '%s' == m.tag '%s'", obj == m.obj, tag.c_str(), m.tag.c_str());
  return (obj == m.obj && tag == m.tag);
};

//------------------------------------------------------------------------------
void bec::ValidationMessagesBE::remove_messages(bec::ValidationMessagesBE::MessageList* ml, const grt::ObjectRef& obj, const grt::Validator::Tag& tag)
{
  bec::ValidationMessagesBE::MessageList::iterator it = ml->end();
  bool was_remove = true;

  while (was_remove)
  {
    it = std::remove_if(ml->begin(), ml->end(),  boost::bind(&bec::ValidationMessagesBE::match_message, _1, obj, tag));
    if (it != ml->end())
    {
      was_remove = true;
      ml->erase(it);
    }
    else
      was_remove = false;
  }
}

//------------------------------------------------------------------------------
void bec::ValidationMessagesBE::validation_message(const grt::Validator::Tag& tag, const grt::ObjectRef& obj, const std::string& msg, const int type)
{  
  switch (type)
  {
    case grt::NoErrorMsg:
    {
      if ("*" != tag)
      {
        // Clear all types with obj and tag. Argument @msg in this case holds tag value
        remove_messages(&_errors, obj, tag);
        remove_messages(&_warnings, obj, tag);
      }
      else
        clear();
      break;
    }
    case grt::ErrorMsg:
    {
      _errors.push_back(Message(msg, obj, tag));
      break;
    }
    case grt::WarningMsg:
    {
      _warnings.push_back(Message(msg, obj, tag));
      break;
    }
    default:
    {
      g_message("Unhandled type in validation_message");
    }
  }

  tree_changed();
}


bec::ValidationManager::MessageSignal* bec::ValidationManager::_signal_notify = 0;

//------------------------------------------------------------------------------
bool bec::ValidationManager::is_validation_plugin(const app_PluginRef& plugin)
{
  return plugin->attributes().has_key("ValidationRT");
}

//------------------------------------------------------------------------------
void bec::ValidationManager::register_validator(grt::GRT* grt, const std::string& type, grt::Validator* v)
{
  grt::MetaClass* mc = grt->get_metaclass(type);
  if (mc)
    mc->add_validator(v);
  else
    g_warning("Specified metaclass '%s' is not known.", type.c_str());
}

//------------------------------------------------------------------------------
void bec::ValidationManager::scan(GRTManager* grtm)
{
  const std::vector<app_PluginRef> plugins = grtm->get_plugin_manager()->get_plugins_for_group("");
  const int size = plugins.size();
  
  for (int i = 0; i < size; ++i)
  {
    if (bec::ValidationManager::is_validation_plugin(plugins[i]))
    {
      grt::Module* module = plugins[i]->get_grt()->get_module(plugins[i]->moduleName());
      grt::CPPModule* cpp_module = dynamic_cast<grt::CPPModule*>(module);
      if (cpp_module)
      { 
        // Handle plugin directly
        //1. Fetch slot
        g_message("ValidationManager: %s", plugins[i]->caption().c_str());
      }
      else
      {
        throw std::logic_error(std::string("Handling of non-C++ validation plugins is not implemented. ") + __FILE__);
      }
    }
  }
}

//------------------------------------------------------------------------------
bool bec::ValidationManager::validate_instance(const grt::ObjectRef& obj, const grt::Validator::Tag& tag)
{
  bool ret = true;
  // Clear messages with correspoding tag from the object
  (*signal_notify())(tag, obj, tag, grt::NoErrorMsg);

  static const grt::MetaClass *mc_to_break_checks = obj->get_grt()->get_metaclass("db.DatabaseObject");
  grt::MetaClass* mc = obj->get_metaclass();
  
  while (mc && mc != mc_to_break_checks)
  {
    if (!mc->foreach_validator(obj, tag))
      ret = false;
    mc = mc->parent();
  }
  return ret;
}

//------------------------------------------------------------------------------
void bec::ValidationManager::message(const grt::Validator::Tag& tag, const grt::ObjectRef& o, const std::string& m, const int level)
{
  // Add message to the Object
  (*signal_notify())(tag, o, m, level);
}

//------------------------------------------------------------------------------
void bec::ValidationManager::clear()
{
  // Clear messages from listeners
  (*signal_notify())("*", grt::ObjectRef(), "", grt::NoErrorMsg);
}
