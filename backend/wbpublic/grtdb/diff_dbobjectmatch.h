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

#include "grtpp_util.h"
#include "wbpublic_public_interface.h"

#include "base/string_utilities.h"
#include "grt/common.h"
#include "grtdb/charset_utils.h"
#include "grtsqlparser/sql_facade.h"
#include "db_object_helpers.h"

namespace sql {
  class DatabaseMetaData;
}

namespace grt {

  struct WBPUBLICBACKEND_PUBLIC_FUNC DbObjectMatchAlterOmf : public Omf {
    virtual bool less(const ValueRef&, const ValueRef&) const;
    virtual bool equal(const ValueRef&, const ValueRef&) const;
  };

  typedef std::function<bool(const ValueRef obj1, const ValueRef obj2, const std::string name)> comparison_rule;
  class WBPUBLICBACKEND_PUBLIC_FUNC NormalizedComparer {
  protected:
    bool comment_compare(const ValueRef obj1, const ValueRef obj2, const std::string& name) const;
    std::map<std::string, std::list<comparison_rule> > rules;
    int _maxTableCommentLength;
    int _maxIndexCommentLength;
    int _maxColumnCommentLength;

    bool _case_sensitive;
    bool _skip_routine_definer;
    void load_rules();

  public:
    void init_omf(Omf* omf);
    void load_db_options(sql::DatabaseMetaData* dbc_meta);
    NormalizedComparer(const grt::DictRef options = grt::DictRef());
    void add_comparison_rule(const std::string& name, comparison_rule rule) {
      rules[name].push_back(rule);
    };
    bool normalizedComparison(const ValueRef obj1, const ValueRef obj2, const std::string name);
    grt::DictRef get_options_dict() const;
    bool is_case_sensitive() const {
      return _case_sensitive;
    };
    bool skip_routine_definer() const {
      return _skip_routine_definer;
    };
  };

} // namespace grt
