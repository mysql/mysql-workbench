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

using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
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

    [DllImport("user32.dll")]
    public static extern int ToUnicode(uint virtualKeyCode, uint scanCode,
                                       byte[] keyboardState,
                                       [Out, MarshalAs(UnmanagedType.LPWStr, SizeConst = 64)] System.Text.StringBuilder receivingBuffer,
                                       int bufferSize, uint flags);


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
              Keys key = (Keys)Enum.Parse(typeof(Keys), "Oem2", true);

              var mappedString = new System.Text.StringBuilder(256);
              ToUnicode((uint)key, 0, new byte[256], mappedString, 256, 0);

              if (mappedString.ToString() == "/") {
                result |= key;
                return result;
              }
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
              smitem.Name = subitem.getInternalName();
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
              smitem.Name = subitem.getInternalName();
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
              smitem.Name = subitem.getInternalName();
              smitem.Text = subitem.get_caption();
              smitem.ShortcutKeys = shortcut;
              smitem.Enabled = subitem.get_enabled();
              buildSubmenu(smitem, subitem.getInternalName(), subitem.get_subitems(), handler);
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
