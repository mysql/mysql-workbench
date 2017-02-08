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
#include "wbpublic_public_interface.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC Reporter {
  protected:
    // need to be mutable to work in const methods
    mutable int _error_count;
    mutable int _warning_count;
    mutable bool _tracking;

  public:
    Reporter();

    inline bool is_tracking() const;
    inline void start_tracking() const;

    void flush() const;

    void report_warning(const char* format, ...) const;
    void report_error(const char* format, ...) const;
    void report_info(const char* format, ...) const;

    void report_summary(const char* operation_name) const;

    inline int error_count() const;
    inline int warning_count() const;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC SummaryCentry {
    Reporter* _rep;
    std::string _operation;

  public:
    SummaryCentry(Reporter& parent, const std::string& operation);
    ~SummaryCentry();

  private:
    SummaryCentry(SummaryCentry&);
    SummaryCentry& operator=(SummaryCentry&);
  };
};
