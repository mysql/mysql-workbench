/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Grt;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public class SqlIdeMenuManager
  {
    public class MenuContext
    {
      public MySQL.Grt.ActionList ActionList;

      public delegate List<NodeIdWrapper> GetSelectedNodes_Delegate();
      public GetSelectedNodes_Delegate GetSelectedNodes;
      public List<NodeIdWrapper> nodes;
      public delegate List<MySQL.Base.MenuItem> GetNodesMenuItems_Delegate(List<NodeIdWrapper> nodes);
      public GetNodesMenuItems_Delegate GetNodesMenuItems;
      public delegate bool TriggerNodesAction_Delegate(String action, List<NodeIdWrapper> nodes);
      public TriggerNodesAction_Delegate TriggerNodesAction;

      public delegate List<int> GetSelectedRowsCol_Delegate(ref int column);
      public GetSelectedRowsCol_Delegate GetSelectedRowsCol;
      public List<int> rows;
      public int column;
      public delegate List<MySQL.Base.MenuItem> GetRowsColMenuItems_Delegate(List<int> rows, int column);
      public GetRowsColMenuItems_Delegate GetRowsColMenuItems;
      public delegate bool TriggerRowsColAction_Delegate(String action, List<int> rows, int column);
      public TriggerRowsColAction_Delegate TriggerRowsColAction;
    }

    static public void InitMenu(ContextMenuStrip menu, MenuContext menuContext)
    {
      MenuContext prevMenuContext = menu.Tag as MenuContext;
      if (null == prevMenuContext)
        menu.Opening += OnMenuOpening;
      menu.Tag = menuContext;
    }

    static private void OnMenuOpening(object sender, CancelEventArgs e)
    {
      ContextMenuStrip menu = sender as ContextMenuStrip;
      MenuContext menuContext = menu.Tag as MenuContext;
      List<MySQL.Base.MenuItem> itemsBE = null;
      menuContext.nodes = null;
      menuContext.rows = null;
      menuContext.column = -1;
      if (null != menuContext.GetSelectedNodes)
      {
        menuContext.nodes = menuContext.GetSelectedNodes();
        if (null == menuContext.nodes)
          menuContext.nodes = new List<NodeIdWrapper>();
        itemsBE = menuContext.GetNodesMenuItems(menuContext.nodes);
      }
      else if (null != menuContext.GetSelectedRowsCol)
      {
        menuContext.rows = menuContext.GetSelectedRowsCol(ref menuContext.column);
        if (null == menuContext.rows)
          menuContext.rows = new List<int>();
        itemsBE = menuContext.GetRowsColMenuItems(menuContext.rows, menuContext.column);
      }
      menu.Items.Clear();
      if (null == itemsBE)
      {
        e.Cancel = true;
      }
      else
      {
        FillMenuItems(itemsBE, menu.Items);
        e.Cancel = false;
      }
    }

    static private void FillMenuItems(List<MySQL.Base.MenuItem> itemsBE, ToolStripItemCollection itemsFE)
    {
      foreach (MySQL.Base.MenuItem itemBE in itemsBE)
      {
        switch (itemBE.get_type())
        {
          case MySQL.Base.MenuItemType.MenuSeparator:
            {
              itemsFE.Add(new ToolStripSeparator());
            }
            break;
          default:
            {
              ToolStripMenuItem itemFE = new ToolStripMenuItem();
              itemFE.Tag = itemBE.getInternalName();
              itemFE.Text = itemBE.get_caption();
              itemFE.Enabled = itemBE.get_enabled();
              if (MySQL.Base.MenuItemType.MenuCascade == itemBE.get_type())
              {
                FillMenuItems(itemBE.get_subitems(), itemFE.DropDownItems);
              }
              else
              {
                itemFE.Click += new EventHandler(OnMenuItemClick);
              }
              itemsFE.Add(itemFE);
            }
            break;
        }
      }
    }

    static private void OnMenuItemClick(object sender, EventArgs e)
    {
      ToolStripMenuItem menuItem = sender as ToolStripMenuItem;
      MenuContext menuContext = GetMenuContext(menuItem.Owner);
      if (null == menuContext)
        return;
      string action = menuItem.Tag as string;
      bool res = false;
      if (null != menuContext.nodes)
      {
        if (!res && (null != menuContext.ActionList))
          res = menuContext.ActionList.trigger_action(action, menuContext.nodes);
        if (!res && (null != menuContext.TriggerNodesAction) && (null != menuContext.nodes))
          res = menuContext.TriggerNodesAction(action, menuContext.nodes);
      }
      else if (null != menuContext.rows)
      {
        if (!res && (null != menuContext.ActionList))
          res = menuContext.ActionList.trigger_action(action, menuContext.rows, menuContext.column);
        if (!res && (null != menuContext.TriggerRowsColAction))
          res = menuContext.TriggerRowsColAction(action, menuContext.rows, menuContext.column);
      }
    }

    static private MenuContext GetMenuContext(ToolStrip toolStrip)
    {
      if (null == toolStrip)
        return null;
      MenuContext menuContext = toolStrip.Tag as MenuContext;
      if (null == menuContext)
      {
        ToolStripDropDownMenu dropDownMenu = toolStrip as ToolStripDropDownMenu;
        if (null == dropDownMenu)
          return null;
        ToolStripItem ownerItem = dropDownMenu.OwnerItem;
        return GetMenuContext(ownerItem.Owner);
      }
      else
        return menuContext;
    }
  }
}
