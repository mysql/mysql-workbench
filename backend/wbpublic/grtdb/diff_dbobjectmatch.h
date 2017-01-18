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
