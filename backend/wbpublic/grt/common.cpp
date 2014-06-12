/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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
#include "base/threading.h"
#include "base/log.h"

#include "common.h"

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"

#include "mforms/utilities.h"

DEFAULT_LOG_DOMAIN("bec");

#include <cstdlib>

using namespace base;

// XXX: most of the string handling code here is not utf-8 safe, check that.
// TODO: move general code to the base library.
namespace bec {

  std::string replace_string(const std::string &s,
                                    const std::string &from,
                                    const std::string &to)
  {
    std::string::size_type p;
    std::string ss, res;

    ss= s;
    p= ss.find(from);
    while (p != std::string::npos)
    {
      if (p > 0)
        res.append(ss.substr(0, p)).append(to);
      else
        res.append(to);
      ss= ss.substr(p+from.size());
      p= ss.find(from);
    }
    res.append(ss);

    return res;
  }


  void replace_string_inplace(std::string &text, const std::string &from, const std::string &to)
  {
    std::string::size_type from_len= from.length();
    std::string::size_type p= text.find(from);
    while (p != std::string::npos)
    {
      text.replace(p, from_len, to);
      p= text.find(from, p);
    }
  }

  std::string replace_variable(const std::string &format, const std::string &variable, const std::string &value)
  {

    std::string result= format;
    std::string::size_type pos;
  
    for (;;)
    {
      std::string s;
      std::string::size_type end;
      
      pos= result.find(variable.substr(0, variable.size()-1));
      if (pos == std::string::npos)
        break;
      
      end= result.find('%', pos+1);
      if (end == std::string::npos) // bad format
        break;

      s= result.substr(pos+1, end-pos-1);

      std::string::size_type filter_pos= s.find("|");
      std::string filtered_value= value;
      
      if (filter_pos == std::string::npos)
      {
        if (s.length() != variable.length()-2)
          break;
      }
      else if (filter_pos != variable.length()-2)
        break;
      else
      {
        std::string filter= s.substr(filter_pos+1, s.size()-filter_pos);

        if (filter.compare("capitalize") == 0)
        {
          gunichar ch= g_utf8_get_char(value.data());
          
          ch= g_unichar_toupper(ch);
          
          gchar *rest= g_utf8_find_next_char(value.data(), value.data()+value.size());
          char utf8[10];
          utf8[g_unichar_to_utf8(ch, utf8)]= 0;
          filtered_value= std::string(utf8).append(rest);
        }
        else if (filter.compare("uncapitalize") == 0)
        {
          gunichar ch= g_utf8_get_char(value.data());

          ch= g_unichar_tolower(ch);

          gchar *rest= g_utf8_find_next_char(value.data(), value.data()+value.size());
          char utf8[10];
          utf8[g_unichar_to_utf8(ch, utf8)]= 0;
          filtered_value= std::string(utf8).append(rest);
        }
        else if (filter.compare("lower") == 0)
        {
          gchar *l= g_utf8_strdown(value.data(), (gssize)value.size());
          if (l)
            filtered_value= l;
          g_free(l);
        }
        else if (filter.compare("upper") == 0)
        {
          gchar *l= g_utf8_strup(value.data(), (gssize)value.size());
          if (l)
            filtered_value= l;
          g_free(l);
        }
      }
      result= result.substr(0, pos).append(filtered_value).append(result.substr(end+1));
    }
    
    return result;
  }
  
  
  std::string append_extension_if_needed(const std::string &path,
                                         const std::string &ext)
  {
    if (!has_suffix(path, ext))
      return path+ext;
    return path;
  }


  std::string make_path(const std::string &prefix, const std::string &file)
  {
    if (prefix.empty())
      return file;

    if (prefix[prefix.size()-1] == '/' || prefix[prefix.size()-1] == '\\')
      return prefix+file;
    return prefix+G_DIR_SEPARATOR+file;
  }



  static bool validate_member(const grt::ClassMember *member, const GrtObjectRef &object, bool &failed)
  {
    GrtObjectRef child;
    switch (member->type.base.type)
    {
      case grt::DictType:
        break;
      case grt::ListType:
        if (member->owned_object)
        {
          grt::BaseListRef list(grt::BaseListRef::cast_from(object->get_member(member->name)));
          for (size_t i = 0; i < list.count(); i++)
          {
            if (GrtObjectRef::can_wrap(list.get(i)))
            {
              GrtObjectRef child(GrtObjectRef::cast_from(list.get(i)));
              if (!child.is_valid())
              {
                failed = true;
                log_error("Child item %zi of list %s::%s is NULL", i, GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str());
              }
              else
              {
                if (!child->owner().is_valid())
                {
                  log_error("owner of %s::%s[%zi] %s is NULL (expected %s <%s>)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), i, child->id().c_str(),
                            object->id().c_str(), object->class_name().c_str());
                  failed = true;
                }
                else if (child->owner() != object)
                {
                  log_error("owner of %s::%s[%zi] %s is wrong (expected %s <%s>, is %s <%s>)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), i, child->id().c_str(),
                            object->id().c_str(), object->class_name().c_str(), child->owner()->id().c_str(), child->owner()->class_name().c_str());
                  failed = true;
                }
                else
                  failed = validate_tree_structure(child) || failed;
              }
            }
            else
              log_info("Unknown object in list %s\n", member->name.c_str());
          }
        }
        else
        {
          grt::BaseListRef list(grt::BaseListRef::cast_from(object->get_member(member->name)));
          for (size_t i = 0; i < list.count(); i++)
          {
            if (GrtObjectRef::can_wrap(list.get(i)))
            {
              GrtObjectRef child(GrtObjectRef::cast_from(list.get(i)));
              if (!child.is_valid())
              {
                log_error("Child item %zi of list %s::%s is NULL", i, GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str());
                failed = true;
              }
              else
              {
                if (!child->owner().is_valid())
                {
                  log_error("owner of %s::%s[%zi] %s is NULL (expected %s <%s>)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), i, child->id().c_str(),
                            object->id().c_str(), object->class_name().c_str());
                  failed = true;
                }
                else if (child->owner() == object)
                {
                  log_error("owner of %s::%s[%zi] is wrong (not supposed to be %s)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), i,
                            child->owner()->id().c_str());
                  failed = true;
                }
              }
            }
          }
        }
        break;
      case grt::ObjectType:
        child = GrtObjectRef::cast_from(object->get_member(member->name));
        if (child.is_valid())
        {
          if (child->owner() != object && member->owned_object)
          {
            log_error("owner of %s::%s is wrong (expected %s, is %s)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(),
                      object->id().c_str(), child->owner()->id().c_str());
            failed = true;
          }
          if (member->owned_object)
            failed = validate_tree_structure(child) || failed;
        }
        break;
      default:
        break;
    }
    return true;
  }


  /** Validates a GRT object tree for correct owner values
   */
  bool validate_tree_structure(const grt::ObjectRef &object)
  {
    bool failed = false;
    grt::MetaClass *mc = object->get_metaclass();
    mc->foreach_member(boost::bind(validate_member, _1, GrtObjectRef::cast_from(object), failed));
    return !failed;
  }

  template <class T>
  void move_list_ref_item(MoveType move_type, grt::ListRef<T> items, const grt::ValueRef &object)
  {
    grt::Type object_type = object.type();

    std::string group_name;
    std::string item_name;
    std::string search_name = "";
    size_t group_indicator_position = std::string::npos;

    MatchType match = (move_type == MoveUp) ? MatchBefore : MatchAfter;

    grt::Ref<T> item;

    // Gets the relevant index for the selected object.
    size_t item_index = grt::BaseListRef::npos;
    if (object_type == grt::ObjectType)
    {
      // For any object like connections or instances.
      item = grt::Ref<T>::cast_from(object);

      item_index = items.get_index(item);
      item_name = item->name();
      group_indicator_position = item_name.find("/");

      // When a grouped item is selected, the movement will be done across same
      // group items
      if (group_indicator_position != std::string::npos)
        search_name = item_name.substr(0, group_indicator_position + 1);
    }
    else
    {
      // For strings (i.e. group names).
      group_name = object.repr();
      group_name += "/";

      // Searches the index of the initial element of the group.
      item_index = find_list_ref_item_position<T>(items, group_name);
      item = items[item_index];
      item_name = group_name;
    }

    // This is executed whenever the target position depends on the main list.
    // The only case where this is excluded is when the selected item belongs to a group.
    if (group_indicator_position == std::string::npos)
    {
      std::vector<std::string> items_list;

      // Gets the main list items (groups and non grouped items).
      for ( grt::TypedListConstIterator<T> end = items.end(),
           inst = items.begin(); inst != end; ++inst)
      {
        std::string item_name = (*inst)->name();
        size_t position = item_name.find("/");

        if (position != std::string::npos)
        {
          std::string group_name = item_name.substr(0, position + 1);
          if (std::find(items_list.begin(), items_list.end(), group_name) == items_list.end())
            items_list.push_back(group_name);
        }
        else
          items_list.push_back(item_name);
      }

      // Searches the item inside the list of non grouped items/groups, will find it only if it's a non grouped item
      size_t item_list_position = std::find(items_list.begin(), items_list.end(), std::string(item_name)) - items_list.begin();

      if (move_type != MoveTop && move_type != MoveBottom)
      {
        size_t offset = (move_type == MoveUp) ? -1 : 1;

        item_name = items_list.at(item_list_position + offset);

        // If the next item is a group
        group_indicator_position = item_name.find("/");

        if(group_indicator_position != std::string::npos)
        {
          search_name = item_name.substr(0, group_indicator_position + 1);
          if (move_type == MoveUp)
            match = MatchAny;
        }
        else
          search_name = item_name;
      }
    }

    if (move_type == MoveTop)
      items.reorder(item_index, 0);
    else if (move_type == MoveBottom)
      items.reorder(item_index, grt::ListRef<T>::npos);
    else
    {
      // Searches the index of the target position.
      size_t target_index = grt::BaseListRef::npos;
      target_index = find_list_ref_item_position<T>(items, search_name, match, &item);

      if (move_type == MoveDown)
        items.reorder(target_index, item_index);
      else
        items.reorder(item_index, target_index);
    }
  }  

  // Template instantiation to avoid having all this code in the header file.
  template WBPUBLICBACKEND_PUBLIC_FUNC void move_list_ref_item<db_mgmt_Connection>(MoveType move_type, grt::ListRef<db_mgmt_Connection> items, const grt::ValueRef &object);

  /**
   * Moves the given object within the list to the given position.
   * This position must be local to the enclosing group.
   */
  template <class T>
  void move_list_ref_item(grt::ListRef<T> items, const grt::ValueRef &object, size_t to)
  {
    grt::Type object_type = object.type();

    std::string group_name;
    std::string item_name;
    std::string search_name;
    size_t group_indicator_position = std::string::npos;

    grt::Ref<T> item;

    // Determine the index of the given object (for groups that is for the first group member).
    size_t item_index;
    if (object_type == grt::ObjectType)
    {
      // For any object like connections or instances.
      item = grt::Ref<T>::cast_from(object);

      item_index = items.get_index(item);
      item_name = item->name();
      group_indicator_position = item_name.find("/");

      // If this is a grouped item it is moved within its group.
      if (group_indicator_position != std::string::npos)
        search_name = item_name.substr(0, group_indicator_position + 1);
    }
    else
    {
      // For strings (i.e. group names).
      group_name = object.repr() + "/";

      // Searches the index of the initial element of the group.
      item_index = find_list_ref_item_position<T>(items, group_name);
      item = items[item_index];
      item_name = group_name;
    }

    if (search_name.empty())
    {
      // Working on an ungrouped item (can itself be a group, however).
      std::vector<std::string> names_list;

      // Collect names of all ungrouped items and groups in an own list for lookup.
      for (grt::TypedListConstIterator<T> iterator = items.begin(); iterator != items.end(); ++iterator)
      {
        std::string item_name = (*iterator)->name();
        size_t position = item_name.find("/");

        if (position != std::string::npos)
        {
          std::string group_name = item_name.substr(0, position + 1);
          if (std::find(names_list.begin(), names_list.end(), group_name) == names_list.end())
            names_list.push_back(group_name);
        }
        else
          names_list.push_back(item_name);
      }

      item_name = to < names_list.size() ? names_list.at(to) : "";

      // If the next item is a group search for that group. Otherwise use the item name directly.
      group_indicator_position = item_name.find("/");
      if (group_indicator_position != std::string::npos)
        search_name = item_name.substr(0, group_indicator_position + 1);
      else
        search_name = item_name;
    }
    else
    {
      // Working within a group. Collect names of all group members.
      std::vector<std::string> names_list;

      // Collect names of all ungrouped items and groups in an own list for lookup.
      for (grt::TypedListConstIterator<T> iterator = items.begin(); iterator != items.end(); ++iterator)
      {
        std::string item_name = (*iterator)->name();
        if (item_name.find(search_name) == 0) // Same group?
          names_list.push_back(item_name);
      }
      search_name = to < names_list.size() ? names_list[to] : "";
    }

    // Now find the determined search name in the complete list to get the target index.
    // If there's no search name we have to move to the end of the list.
    size_t target_index;
    if (search_name.empty())
      target_index = items->count();
    else
      target_index = find_list_ref_item_position<T>(items, search_name, MatchAny, &item);

    if (group_name.empty())
    {
      // Reorder is incorrectly implemented, so we have to adjust the target index.
      if (item_index < target_index)
        items.reorder(item_index, target_index - 1);
      else
        items.reorder(item_index, target_index);
    }
    else
    {
      // If the group name is set then we are moving an entire group.
      // So we have to move all entries belonging to that group to the target position.
      grt::ListRef<T> group_members(items.get_grt());

      // We iterate through the entire list to collect all members. This allows us
      // to fix lists with members not following directly each other.
      for (grt::TypedListConstIterator<T> iterator = items.begin(); iterator != items.end(); ++iterator)
      {
        std::string item_name = (*iterator)->name();
        if (item_name.find(group_name) == 0) // Same group?
          group_members.insert(*iterator);
      }

      for (grt::TypedListConstReverseIterator<T> iterator = group_members.rbegin();
           iterator != group_members.rend(); ++iterator)
      {
        item_index = items.get_index(*iterator);
        if (item_index < target_index)
          items.reorder(item_index, target_index - 1);
        else
          items.reorder(item_index, target_index);
      }

    }
  }

  // Template instantiation to avoid having all this code in the header file.
  template WBPUBLICBACKEND_PUBLIC_FUNC void move_list_ref_item<db_mgmt_Connection>(grt::ListRef<db_mgmt_Connection> items,
                                                       const grt::ValueRef &object, size_t to);
  
  //------------------------------------------------------------------------------------------------

  TimerActionThread::TimerActionThread(const Action &action, gulong milliseconds) : _action(action), _microseconds(milliseconds * 1000)
  {
    _thread= base::create_thread(start, this);
  }

  TimerActionThread * TimerActionThread::create(const Action &action, gulong milliseconds)
  {
    return new TimerActionThread(action, milliseconds);
  }

  gpointer TimerActionThread::start(gpointer data)
  {
    mforms::Utilities::set_thread_name("timer");
    TimerActionThread *thread= static_cast<TimerActionThread*>(data);
    thread->main_loop();
    return NULL;
  }

  void TimerActionThread::stop(bool clear_exit_signal)
  {
    MutexLock action_mutex(_action_mutex);
    _action= Action();
    if (clear_exit_signal)
      on_exit.disconnect_all_slots();
  }

  void TimerActionThread::main_loop()
  {
    const int poll_interval= 1000000; // check every 1 sec if thread was stopped
    for (;;)
    {
      std::div_t d= std::div(_microseconds, poll_interval);
      for (int n= 0; n < d.quot; ++n)
      {
        g_usleep(poll_interval);
        {
          MutexLock action_mutex(_action_mutex);
          if (_action.empty())
            goto exit;
        }
      }
      g_usleep(d.rem);
      {
        MutexLock action_mutex(_action_mutex);
        if (_action.empty())
          goto exit;
        if (_microseconds != 0)
          _action();
        else
          g_usleep(poll_interval);
      }
    }

exit:
    on_exit();
    delete this;
  }
  ScopeExitTrigger::ScopeExitTrigger()
  { }

  ScopeExitTrigger::ScopeExitTrigger(const Slot &cb) : slot(cb)
  { }

  ScopeExitTrigger::~ScopeExitTrigger()
  {
    if (slot)
      slot();
  }

  ScopeExitTrigger& ScopeExitTrigger::operator=(const Slot &cb)
  {
    slot= cb; return *this;
  }


};
