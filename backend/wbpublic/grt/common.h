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
#ifndef _BE_COMMON_H_
#define _BE_COMMON_H_

#include <cstring>
#include <vector>
#include <string>
#include <glib.h>
#include <algorithm>

#ifndef _WIN32
#include <stdexcept>
#include <stdarg.h>
#endif

#include <string.h>
#include "grtpp.h"

#include "wbpublic_public_interface.h"

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif



namespace bec {
  
  enum MatchType
  {
    MatchAny,
    MatchBefore,
    MatchAfter,
    MatchLast
  };
  
  enum MoveType
  {
    MoveTop,
    MoveUp,
    MoveDown,
    MoveBottom
  };

  enum FindType
  {
    FindPrefix,
    FindFull
  };

  WBPUBLICBACKEND_PUBLIC_FUNC bool validate_tree_structure(const grt::ObjectRef &object);

  std::string WBPUBLICBACKEND_PUBLIC_FUNC replace_string(const std::string &s,
                            const std::string &from,
                            const std::string &to);
  void WBPUBLICBACKEND_PUBLIC_FUNC replace_string_inplace(std::string &text,
                            const std::string &from,
                            const std::string &to);
  
  // replaces a variable from a string in format %variable%
  // a filter can be passed to the variable as in %variable|filter%
  // supported filters are upper, lower and capitalize
  std::string WBPUBLICBACKEND_PUBLIC_FUNC replace_variable(const std::string &format, const std::string &variable, const std::string &value);
  
  std::string WBPUBLICBACKEND_PUBLIC_FUNC append_extension_if_needed(const std::string &path,
                                                              const std::string &ext);

  inline bool has_prefix(const std::string &str, const std::string &pref)
  {
    if (strncmp(str.c_str(), pref.c_str(), pref.length())==0)
      return true;
    return false;
  }


  inline bool has_suffix(const std::string &str, const std::string &suf)
  {
    if (suf.length() < str.length() && strncmp(str.c_str()+str.length()-suf.length(), suf.c_str(), suf.length())==0)
      return true;
    return false;
  }


  std::string WBPUBLICBACKEND_PUBLIC_FUNC make_path(const std::string &prefix, const std::string &file);

  inline std::string pathlist_append(const std::string &l, const std::string &s)
  {
    if (l.empty())
      return s;
    return l+G_SEARCHPATH_SEPARATOR+s;
  }

  inline std::string pathlist_prepend(const std::string &l, const std::string &s)
  {
    if (l.empty())
      return s;
    return s+G_SEARCHPATH_SEPARATOR+l;
  }
  
  template <class T>
  size_t find_list_ref_item_position(grt::ListRef<T> &item_data, std::string& name, MatchType match = MatchAny, grt::Ref<T>* reference = NULL, FindType find_mode = FindPrefix)
  {

    if ((match == MatchBefore || match == MatchAfter) && !reference)
        throw std::invalid_argument("A reference must be specified for MatchBefore and MatchAfter");

    bool search_enabled = match != MatchAfter;
    bool exit = false;

    size_t index = grt::BaseListRef::npos;
    
    for ( grt::TypedListConstIterator<T> end = item_data.end(),
         inst = item_data.begin(); inst != end && !exit; ++inst)
    {
      // If skip is defined will omit the entries until the 'skip' element is found
      if (search_enabled)
      {
        // For MatchBefore the search ends when the reference item is found
        if (match == MatchBefore && (*reference) == (*inst))
          exit = true;
        else
        {
          std::string item_name = (*inst)->name();
          
          int compare_result = (find_mode == FindPrefix) ? item_name.compare(0, name.length(), name) : item_name.compare(name);

          // index will contain always the position of the last matched entry
          if (compare_result == 0)
          {
            index = item_data.get_index(*inst);
            
            // MatchBefore needs to search until the reference is found
            // MatchLast needs to search until the whole list has been searched to get the last match
            // MatchAfter and MatchAny are done as soon as a match is found
            if (match != MatchBefore && match != MatchLast)
              exit = true;
          }
        }
      }
      
      // For MatchAfter the search starts once the reference item has been found
      else if ((*reference) == (*inst))
        search_enabled = true;
    }  
    
    return index;
  }

  template <class T>
  WBPUBLICBACKEND_PUBLIC_FUNC void move_list_ref_item(MoveType move_type, grt::ListRef<T> items, const grt::ValueRef &object);

  template <class T>
  WBPUBLICBACKEND_PUBLIC_FUNC void move_list_ref_item(grt::ListRef<T> items, const grt::ValueRef &object, size_t to);

  class WBPUBLICBACKEND_PUBLIC_FUNC TimerActionThread
  {
  public:
    typedef boost::function<void ()> Action;
    static TimerActionThread * create(const Action &action, gulong milliseconds);
    void stop(bool clear_exit_signal);
    boost::signals2::signal<void ()> on_exit;
  private:
    base::Mutex _action_mutex;
    Action _action;
    gulong _microseconds;
    GThread *_thread;
    TimerActionThread(const Action &action, gulong milliseconds);
    static gpointer start(gpointer data);
    void main_loop();
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC ScopeExitTrigger
  {
  public:
    typedef boost::function<void ()> Slot;
    ScopeExitTrigger();
    ScopeExitTrigger(const Slot &cb);
    ~ScopeExitTrigger();
    ScopeExitTrigger & operator=(const Slot &cb);
    Slot slot;
  };
  
};

#endif /* _BE_COMMON_H_ */
