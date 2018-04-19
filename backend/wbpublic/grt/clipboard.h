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

#pragma once

#include "grt.h"
#ifndef _MSC_VER
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
