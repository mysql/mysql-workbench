/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TRACKABLE_H
#define _TRACKABLE_H
#include <list>
#include <boost/signals2/connection.hpp>
#include <map>
#include <functional>
#include "common.h"

namespace base {

  namespace trackable_checks {
    template <typename T>
    inline std::string is_valid_slot(const T&) {
      return std::string();
    }

    template <class R, class... Args>
    inline std::string is_valid_slot(const std::function<R(Args...)>& f) {
      return !f ? "Attempted to connect empty std::func" : std::string();
    }
  }

  class BASELIBRARY_PUBLIC_FUNC trackable {
  public:
    typedef std::function<void*(void*)> destroy_func;
    void remove_destroy_notify_callback(void* data) {
      _destroy_functions.erase(data);
    }

    void add_destroy_notify_callback(void* data, const destroy_func& func) {
      _destroy_functions[data] = func;
    }

    ~trackable() {
      for (std::map<void*, destroy_func>::iterator It = _destroy_functions.begin(); It != _destroy_functions.end();
           ++It)
        It->second(It->first);
    }

    void disconnect_scoped_connects() {
      _connections.clear();
    }

    template <typename TSignal, typename TSlot>
    void scoped_connect(TSignal* signal, TSlot slot) {
      if (!trackable_checks::is_valid_slot(slot).empty())
        throw std::logic_error(trackable_checks::is_valid_slot(slot));
      std::shared_ptr<boost::signals2::scoped_connection> conn(
        new boost::signals2::scoped_connection(signal->connect(slot)));
      _connections.push_back(conn);
    }

    void track_connection(const boost::signals2::connection& conn) {
      _connections.push_back(
        std::shared_ptr<boost::signals2::scoped_connection>(new boost::signals2::scoped_connection(conn)));
    }

  private:
    std::list<std::shared_ptr<boost::signals2::scoped_connection> > _connections;
    std::map<void*, destroy_func> _destroy_functions;
  };

  template <typename TRetval>
  TRetval run_and_return_value(const std::function<void()>& f) {
    f();
    return TRetval();
  }

  template <bool ret>
  bool run_and_return_value(const std::function<void()>& f) {
    f();
    return ret;
  }
}
#endif // #ifndef _TRACKABLE_H
