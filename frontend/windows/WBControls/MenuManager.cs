/* 
 * Copyright (c) 2007, 2014, Oracle and/or its affiliates. All rights reserved.
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
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls.Properties;

namespace MySQL.Utilities
{
  public class MenuManager
  {
    public System.Windows.Forms.ContextMenuStrip ShowContextMenu(System.Windows.Forms.Control parent, List<MySQL.Base.MenuItem> items,
      int x, int y, EventHandler handler)
    {
      System.Windows.Forms.ContextMenuStrip menu = new System.Windows.Forms.ContextMenuStrip();
      System.Windows.Forms.ToolStripItem[] itemList;

      itemList = buildMenu(items, handler);
      menu.Items.AddRange(itemList);

      menu.Show(parent, new System.Drawing.Point(x, y), ToolStripDropDownDirection.BelowRight);
      return menu;
    }

    public static Keys convertShortcut(String shortcut)
    {
      Keys result = Keys.None;

      if (shortcut != "")
      {
        foreach (String k in shortcut.Split(new char[] { '+' }))
        {
          int x;

          // Convert the platform neutral keywords to such used on Windows.
          String shortcutString;
          switch (k)
          {
            case "Modifier":
              shortcutString = "Control";
              break;
            case "Command": // We cannot use LWin as shortcut key, so map "command" to alt too.
            case "Alternate":
              shortcutString = "Alt";
              break;
            case "Plus":
              shortcutString = "Oemplus";
              break;
            case "Minus":
              shortcutString = "OemMinus";
              break;
            case "Slash":
              shortcutString = "Divide";
              break;
            default:
              shortcutString = k;
              break;
          }
          if (k.Length == 1 && int.TryParse(k, out x))
            result |= (Keys) Enum.Parse(typeof(Keys), "D" + shortcutString);
          else
            result |= (Keys) Enum.Parse(typeof(Keys), shortcutString, true);
        }
      }
      return result;
    }

    public virtual void buildSubmenu(ToolStripMenuItem parentItem, String name, List<MySQL.Base.MenuItem> subitems,
      EventHandler handler)
    {
      ToolStripItem[] itemlist = buildMenu(subitems, handler);

      parentItem.DropDownItems.Clear();
      parentItem.DropDownItems.AddRange(itemlist);
    }

    public virtual System.Windows.Forms.ToolStripItem[] buildMenu(List<MySQL.Base.MenuItem> menuItems, System.EventHandler handler)
    {
      System.Windows.Forms.ToolStripItem[] itemlist = new System.Windows.Forms.ToolStripItem[menuItems.Count];
      int i = 0;

      // rebuild the menu
      foreach (MySQL.Base.MenuItem subitem in menuItems)
      {
        Keys shortcut = convertShortcut(subitem.get_shortcut());

        switch (subitem.get_type())
        {
          case MenuItemType.MenuAction:
          case MenuItemType.MenuUnavailable:
            {
              System.Windows.Forms.ToolStripMenuItem smitem;

              smitem = new System.Windows.Forms.ToolStripMenuItem();
              smitem.Name = subitem.get_name();
              smitem.Text = subitem.get_caption();
              smitem.ShortcutKeys = shortcut;
              smitem.Enabled = subitem.get_enabled();
              if (subitem.get_type() == MenuItemType.MenuUnavailable)
              {
                smitem.Image = Resources.menu_se;
                smitem.Enabled = false;
              }
              smitem.Click += handler;
              itemlist[i++] = smitem;
              break;
            }
            
          case MenuItemType.MenuCheck:
          case MenuItemType.MenuRadio:
            {
              System.Windows.Forms.ToolStripMenuItem smitem;

              smitem = new System.Windows.Forms.ToolStripMenuItem();
              smitem.Name = subitem.get_name();
              smitem.Text = subitem.get_caption();
              smitem.ShortcutKeys = shortcut;
              smitem.Enabled = subitem.get_enabled();
              smitem.Checked = subitem.get_checked();
              smitem.Click += handler;
              itemlist[i++] = smitem;
              break;
            }

          case MenuItemType.MenuSeparator:
            itemlist[i++] = new System.Windows.Forms.ToolStripSeparator();
            break;

          case MenuItemType.MenuCascade:
            {
              System.Windows.Forms.ToolStripMenuItem smitem;

              smitem = new System.Windows.Forms.ToolStripMenuItem();
              smitem.Name = subitem.get_name();
              smitem.Text = subitem.get_caption();
              smitem.ShortcutKeys = shortcut;
              smitem.Enabled = subitem.get_enabled();
              buildSubmenu(smitem, subitem.get_name(), subitem.get_subitems(), handler);
              itemlist[i++] = smitem;
              break;
            }

          default:
            throw new Exception("bad item type");
        }
      }
      return itemlist;
    }

  }
}
