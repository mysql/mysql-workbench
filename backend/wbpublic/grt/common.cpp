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
                log_error("Child item %li of list %s::%s is NULL\n", (long int)i, GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str());
              }
              else
              {
                if (!child->owner().is_valid())
                {
                  log_error("owner of %s::%s[%li] %s is NULL (expected %s <%s>)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i, child->id().c_str(),
                            object->id().c_str(), object->class_name().c_str());
                  failed = true;
                }
                else if (child->owner() != object)
                {
                  log_error("owner of %s::%s[%li] %s is wrong (expected %s <%s>, is %s <%s>)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i, child->id().c_str(),
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
                log_error("Child item %li of list %s::%s is NULL\n", (long int)i, GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str());
                failed = true;
              }
              else
              {
                if (!child->owner().is_valid())
                {
                  log_error("owner of %s::%s[%li] %s is NULL (expected %s <%s>)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i, child->id().c_str(),
                            object->id().c_str(), object->class_name().c_str());
                  failed = true;
                }
                else if (child->owner() == object)
                {
                  log_error("owner of %s::%s[%li] is wrong (not supposed to be %s)\n", GrtNamedObjectRef::cast_from(object)->name().c_str(), member->name.c_str(), (long int)i,
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

    grt::Ref<T> item, target_item;
    size_t ui_item_index = grt::BaseListRef::npos;
    size_t item_index = grt::BaseListRef::npos;
    size_t target_index = grt::BaseListRef::npos;


    // Obtains data from the elements lists like:
    // - Top level elements
    // - First and Last positions of the grouped items
    // - Grouped elements
    bool fabric;
    std::vector<std::string> names_list;
    std::map<std::string, size_t> name_positions;
    std::map<std::string, int> initial_positions;
    std::map<std::string, int> final_positions;
    std::map<std::string, grt::ListRef<T> >groups;
    std::map<std::string, bool> fabric_names;

    // Collect names of all ungrouped items and groups in an own list for lookup.
    int item_count = 0;
    for (grt::TypedListConstIterator<T> iterator = items.begin(); iterator != items.end(); ++iterator)
    {
      grt::ListRef<T> target_group;

      fabric = false;
      std::string item_name = (*iterator)->name();
      size_t position = item_name.find("/");

      if (position != std::string::npos)
      {
        std::string group_name = item_name.substr(0, position + 1);
        if (std::find(names_list.begin(), names_list.end(), group_name) == names_list.end())
        {
          item_name = group_name;
          target_group = grt::ListRef<T>(items.get_grt());
          groups[item_name] = target_group;
        }
        else
        {
          final_positions[group_name] = item_count++;
          target_group = groups[group_name];
          item_name = "";
        }
      }
      // On a fabric node the first tile will always be the parent node's name which does
      // not contain the /
      // It needs to be added to the child connections match it
      else if ((*iterator)->driver().is_valid() && (*iterator)->driver()->name() == "MySQLFabric")
      {
        item_name += "/";
        fabric = true;
        target_group = grt::ListRef<T>(items.get_grt());
        groups[item_name] = target_group;
      }

      // Updates data with item's element
      if (item_name.length())
      {
        name_positions[item_name] = names_list.size();
        names_list.push_back(item_name);
        fabric_names[item_name] = fabric;
        initial_positions[item_name] = item_count;
        final_positions[item_name] = item_count;

        item_count++;
      }

      if (target_group.is_valid())
        target_group->insert_unchecked(*iterator);

    }

    // Now gets the moved element start position
    bool grouped = false;
    if (object_type == grt::ObjectType)
    {
      item = grt::Ref<T>::cast_from(object);
      item_name = item->name();
      group_indicator_position = item_name.find("/");

      // If this is a grouped item it is moved within its group.
      if (group_indicator_position != std::string::npos)
      {
        group_name = item_name.substr(0, group_indicator_position + 1);
        grouped = true;
      }
    }
    else
      item_name = object.repr();


    if (grouped)
    {
      grt::ListRef<T> target_group = groups[group_name];
      target_group->get_index(object);

      ui_item_index = target_group->get_index(object);
    }
    else
      ui_item_index = name_positions[item_name];



    // Pre-processes the to value, to see if it is one of the move to commands
    if (to < 0)
    {
      switch (to)
      {
        case MoveUp: to = ui_item_index - 1; break; 
        case MoveDown: to = ui_item_index + 2; break;
        case MoveTop: 
          to = (grouped && fabric_names[group_name]) ? 1 : 0;
          break;
      }
    }


    if (grouped)
    {
      grt::ListRef<T> target_group = groups[group_name];
      item_index = items->get_index(item);

      // In case o should point to the last element on the group
      if (to == MoveBottom)
        to = target_group->count() - 1;
      else 
      {
        // This adjustment is needed because of the way reorder works
        if ((int)ui_item_index < to)
          to--;
      }

      target_item = grt::Ref<T>::cast_from(target_group->get(to));
      target_index = items->get_index(target_item);

      items.reorder(item_index, target_index);
    }
    else
    {
      // In case o should point to the last element on the entire list
      if (to == MoveBottom)
        to = names_list.size() - 1;
      // This adjustment is needed because of the way reorder works
      else if ((int)ui_item_index < to)
        to--;

      target_index = initial_positions[names_list[to]];
      item_index = initial_positions[item_name];

      if (groups.count(item_name))
      {
        grt::ListRef<T> target_group = groups[item_name];
        // When reordering items we need to consider the position of the reordered
        // items in relation with the target index.
        
        bool before = true;
        if (item_index > target_index)
          before = false;;

        for (grt::TypedListConstIterator<T> iterator = target_group.begin();
          iterator != target_group.end(); ++iterator)
        {
          item_index = items.get_index(*iterator);

          // Items before the target are positioned on the target index
          if (before)
          {
            // This identifies the first item AFTER the target index
            if (item_index > target_index)
            {
              target_index++;
              before = false;
            }
            else
              items.reorder(item_index, target_index);
          }

          // On Items after the original target index will be inserted
          // after each other, this is to avoid reversing the connections
          // position which could fake the fabric structure
          if (!before)
            items.reorder(item_index, target_index++);
        }
      }
      else
        items.reorder(item_index, target_index);
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
