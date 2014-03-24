/*
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

using MySQL.Grt;
using MySQL.Utilities;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  class WorkbenchMenuManager : MenuManager
  {
    #region Member Variables
    
    // The Workbench context
    private WbContext wbContext;

    #endregion

    #region Constructors

    public WorkbenchMenuManager(WbContext WbContext)
    {
      wbContext = WbContext;
    }

    #endregion


    void tsitem_DropDownOpening(object sender, EventArgs e)
    {
      System.Windows.Forms.ToolStripMenuItem tsitem = sender as ToolStripMenuItem;

      buildSubmenu(tsitem, tsitem.Name, null, new EventHandler(menuItem_Click));
    }

    public System.Windows.Forms.ContextMenuStrip ShowContextMenu(System.Windows.Forms.Control parent, List<MySQL.Base.MenuItem> items, int x, int y)
    {
      System.Windows.Forms.ContextMenuStrip menu = new System.Windows.Forms.ContextMenuStrip();
      System.Windows.Forms.ToolStripItem[] itemList;

      itemList = buildMenu(items, new EventHandler(menuItem_Click));
      menu.Items.AddRange(itemList);

      menu.Show(parent, new System.Drawing.Point(x, y), ToolStripDropDownDirection.BelowRight);

      return menu;
    }

    void menuItem_Click(object sender, EventArgs e)
    {
      System.Windows.Forms.ToolStripMenuItem item = sender as System.Windows.Forms.ToolStripMenuItem;

      if (item != null)
      {
        wbContext.activate_command(item.Name);
      }
    }
  }
}
