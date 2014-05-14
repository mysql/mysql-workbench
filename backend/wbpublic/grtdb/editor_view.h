/* 
 * Copyright (c) 2007, 2013, Oracle and/or its affiliates. All rights reserved.
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
#ifndef _EDITOR_VIEW_H_
#define _EDITOR_VIEW_H_

#include "grtdb/editor_dbobject.h"
#include "wbpublic_public_interface.h"

namespace bec {

  class WBPUBLICBACKEND_PUBLIC_FUNC ViewEditorBE : public DBObjectEditorBE
  {
  protected:
    db_ViewRef _view;
    bool _has_syntax_error;

  public:
    virtual std::string get_title();

    virtual db_DatabaseObjectRef get_dbobject() { return get_view(); }
    virtual db_ViewRef& get_view() { return _view; }

    virtual std::string get_query();
    virtual void set_query(const std::string &sql, bool sync);
    grt::ValueRef parse_sql(grt::GRT*, grt::StringRef sql);
    bool has_syntax_error() { return _has_syntax_error; }

    ViewEditorBE(GRTManager *grtm, const db_ViewRef &view, const db_mgmt_RdbmsRef &rdbms);
  };

};

#endif /* _EDITOR_VIEW_H_ */
