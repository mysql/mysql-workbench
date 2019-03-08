/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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
    virtual ~Reporter() {};

    virtual bool is_tracking() const;
    virtual void start_tracking() const;

    virtual void flush() const;

    virtual void report_warning(const char* format, ...) const;
    virtual void report_error(const char* format, ...) const;
    virtual void report_info(const char* format, ...) const;
    virtual void report_heading(const char* format, ...) const;

    virtual void report_summary(const char* operation_name) const;

    virtual int error_count() const;
    virtual int warning_count() const;
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
