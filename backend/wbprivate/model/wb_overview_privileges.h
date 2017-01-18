/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _WB_OVERVIEW_PRIVILEGES_H_
#define _WB_OVERVIEW_PRIVILEGES_H_

#include "workbench/wb_overview.h"

namespace wb {

  class PhysicalOverviewBE;

  namespace internal {
    class PrivilegeContentListNode;

    class PrivilegeInfoNode : public OverviewBE::ContainerNode {
      bool add_new_user(WBContext *wb);
      bool add_new_role(WBContext *wb);

    public:
      PrivilegeInfoNode(const db_CatalogRef &catalog, PhysicalOverviewBE *owner);

      virtual void paste_object(WBContext *wb, bec::Clipboard *clip);
      virtual bool is_pasteable(bec::Clipboard *clip);

      virtual int get_popup_menu_items(WBContext *wb, bec::MenuItemList &items) {
        return 0;
      }
    };
  };
};

#endif /* _WB_OVERVIEW_PRIVILEGES_H_ */
