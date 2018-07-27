/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/log.h"

#include "validation_manager.h"
#include "grt/grt_manager.h"

#include <algorithm>

DEFAULT_LOG_DOMAIN("validation")

//--------------------------------------------------------------------------------------------------

bec::ValidationMessagesBE::ValidationMessagesBE() {
  _error_icon = IconManager::get_instance()->get_icon_id("mini_error.png");
  _warning_icon = IconManager::get_instance()->get_icon_id("mini_warning.png");
  _info_icon = IconManager::get_instance()->get_icon_id("mini_notice.png");

  scoped_connect(bec::ValidationManager::signal_notify(),
                 std::bind(&bec::ValidationMessagesBE::validation_message, this, std::placeholders::_1,
                           std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationMessagesBE::clear() {
  _errors.clear();
  _warnings.clear();
}

//--------------------------------------------------------------------------------------------------

bool bec::ValidationMessagesBE::get_field(const bec::NodeId& node, ColumnId column, std::string& value) {
  bool ret = false;
  if (column == bec::ValidationMessagesBE::Description) {
    const MessageList::size_type idx = node.end();

    if (idx < _errors.size())
      value = _errors[idx].msg;
    else
      value = _warnings[idx].msg;

    ret = true;
  }

  return ret;
}

//--------------------------------------------------------------------------------------------------

bec::IconId bec::ValidationMessagesBE::get_field_icon(const bec::NodeId& node, ColumnId column, IconSize) {
  bec::IconId icon_id = _info_icon;

  if (column == bec::ValidationMessagesBE::Description) {
    const MessageList::size_type idx = node.end();

    if (idx < _errors.size())
      icon_id = _error_icon;
    else
      icon_id = _warning_icon;
  }

  return icon_id;
}

//--------------------------------------------------------------------------------------------------

size_t bec::ValidationMessagesBE::count() {
  return (_errors.size() + _warnings.size());
}

//--------------------------------------------------------------------------------------------------

int bec::ValidationMessagesBE::get_node_popup_items(const bec::NodeId& node, bec::MenuItemList& menu) {
  return 0;
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationMessagesBE::activate_node_popup_item(const bec::NodeId& node, const std::string& name) {
}

//--------------------------------------------------------------------------------------------------

bool bec::ValidationMessagesBE::match_message(const bec::ValidationMessagesBE::Message& m, const grt::ObjectRef& obj,
                                              const grt::Validator::Tag& tag) {
  return (obj == m.obj && tag == m.tag);
};

//--------------------------------------------------------------------------------------------------

void bec::ValidationMessagesBE::remove_messages(bec::ValidationMessagesBE::MessageList* ml, const grt::ObjectRef& obj,
                                                const grt::Validator::Tag& tag) {
  bec::ValidationMessagesBE::MessageList::iterator it = ml->end();
  bool was_remove = true;

  while (was_remove) {
    it = std::remove_if(ml->begin(), ml->end(),
                        std::bind(&bec::ValidationMessagesBE::match_message, std::placeholders::_1, obj, tag));
    if (it != ml->end()) {
      was_remove = true;
      ml->erase(it);
    } else
      was_remove = false;
  }
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationMessagesBE::validation_message(const grt::Validator::Tag& tag, const grt::ObjectRef& obj,
                                                   const std::string& msg, const int type) {
  switch (type) {
    case grt::NoErrorMsg: {
      if ("*" != tag) {
        // Clear all types with obj and tag. Argument @msg in this case holds tag value
        remove_messages(&_errors, obj, tag);
        remove_messages(&_warnings, obj, tag);
      } else
        clear();
      break;
    }
    case grt::ErrorMsg: {
      _errors.push_back(Message(msg, obj, tag));
      break;
    }
    case grt::WarningMsg: {
      _warnings.push_back(Message(msg, obj, tag));
      break;
    }
    default: { logWarning("Unhandled type in validation_message\n"); }
  }

  tree_changed();
}

bec::ValidationManager::MessageSignal* bec::ValidationManager::_signal_notify = 0;

//--------------------------------------------------------------------------------------------------

bool bec::ValidationManager::is_validation_plugin(const app_PluginRef& plugin) {
  return plugin->attributes().has_key("ValidationRT");
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationManager::register_validator(const std::string& type, grt::Validator* v) {
  grt::MetaClass* mc = grt::GRT::get()->get_metaclass(type);
  if (mc)
    mc->add_validator(v);
  else
    logWarning("Specified metaclass '%s' is not known.\n", type.c_str());
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationManager::scan() {
  const std::vector<app_PluginRef> plugins = bec::GRTManager::get()->get_plugin_manager()->get_plugins_for_group("");

  for (size_t i = 0; i < plugins.size(); ++i) {
    if (bec::ValidationManager::is_validation_plugin(plugins[i])) {
      grt::Module* module = grt::GRT::get()->get_module(plugins[i]->moduleName());
      grt::CPPModule* cpp_module = dynamic_cast<grt::CPPModule*>(module);
      if (cpp_module) {
        // Handle plugin directly
        // 1. Fetch slot
        logDebug2("ValidationManager: %s", plugins[i]->caption().c_str());
      } else {
        throw std::logic_error(std::string("Handling of non-C++ validation plugins is not implemented. ") + __FILE__);
      }
    }
  }
}

//--------------------------------------------------------------------------------------------------

bool bec::ValidationManager::validate_instance(const grt::ObjectRef& obj, const grt::Validator::Tag& tag) {
  bool ret = true;

  // Clear messages with corresponding tag from the object.
  (*signal_notify())(tag, obj, tag, grt::NoErrorMsg);

  static const grt::MetaClass* mc_to_break_checks = grt::GRT::get()->get_metaclass("db.DatabaseObject");
  grt::MetaClass* mc = obj->get_metaclass();

  while (mc && mc != mc_to_break_checks) {
    if (!mc->foreach_validator(obj, tag))
      ret = false;
    mc = mc->parent();
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationManager::message(const grt::Validator::Tag& tag, const grt::ObjectRef& o, const std::string& m,
                                     const int level) {
  // Add message to the Object
  (*signal_notify())(tag, o, m, level);
}

//--------------------------------------------------------------------------------------------------

void bec::ValidationManager::clear() {
  // Clear messages from listeners
  (*signal_notify())("*", grt::ObjectRef(), "", grt::NoErrorMsg);
}

//--------------------------------------------------------------------------------------------------
