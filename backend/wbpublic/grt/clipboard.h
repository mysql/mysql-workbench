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

#pragma once

#include "grt.h"
#ifndef _WIN32
#include <vector>
#endif

namespace bec {

  class Clipboard {
    std::list<grt::ObjectRef> _contents;
    std::string _description;

    boost::signals2::signal<void()> _changed_signal;

  public:
    Clipboard(){};
    virtual ~Clipboard(){};

    boost::signals2::signal<void()>* signal_changed() {
      return &_changed_signal;
    }
    void changed() {
      _changed_signal();
    }

    virtual void clear() {
      _contents.clear();
    }

    virtual bool empty() {
      return _contents.empty();
    }

    virtual void append_data(const grt::ObjectRef& data) {
      _contents.push_back(data);
    }
    virtual bool is_data() const {
      return !_contents.empty();
    }
    virtual std::list<grt::ObjectRef> get_data() {
      return _contents;
    }

    void set_content_description(const std::string description) {
      _description = description;
    }
    std::string get_content_description() {
      return _description;
    }
  };
};
