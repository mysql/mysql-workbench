/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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
using System.Windows.Forms;

using MySQL.Grt;
using MySQL.Grt.Db;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class DbMysqlRoutineGroupEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private MySQLRoutineGroupEditorWrapper routineGroupEditorBE { get { return Backend as MySQLRoutineGroupEditorWrapper; } } 

    #endregion

    #region Constructors

    public DbMysqlRoutineGroupEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      InitFormData();
      ReinitWithArguments(value);
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;

      try
      {
        Backend = new MySQLRoutineGroupEditorWrapper(value);
        Control editor = SetupEditorOnHost(tabPage2, true);
        /*
         * TODO: The editor is an mforms control, so drag/drop is managed there, but this is still pending.
        editor.AllowDrop = true;
        editor.DragEnter += new DragEventHandler(ddlEditor_DragEnter);
        editor.DragOver += new DragEventHandler(ddlEditor_DragOver);
        editor.DragDrop += new DragEventHandler(ddlEditor_DragDrop);
        */
        routineGroupEditorBE.load_routines_sql();
        RefreshFormData();
      }
      finally
      {
        InitializingControls = false;
      }

      Invalidate();

      return true;
    }

    #endregion

    #region Form implementation

    protected void InitFormData()
    {
      routineNamesListBox.KeyDown += new KeyEventHandler(routineNamesListBox_KeyDown);

      ContextMenu listboxMenu = new ContextMenu();
      System.Windows.Forms.MenuItem openItem = new System.Windows.Forms.MenuItem("Open routine in its own editor");
      openItem.Click += new EventHandler(openItem_Click);
      listboxMenu.MenuItems.Add(openItem);
      System.Windows.Forms.MenuItem removeItem = new System.Windows.Forms.MenuItem("Remove routine from the group");
      removeItem.Click += new EventHandler(removeItem_Click);
      listboxMenu.MenuItems.Add(removeItem);
      listboxMenu.Popup += new EventHandler(listboxMenu_Popup);
      routineNamesListBox.ContextMenu = listboxMenu;
      routineNamesListBox.AllowDrop = true;

      routineNamesListBox.DragEnter += new DragEventHandler(ddlEditor_DragEnter);
      routineNamesListBox.DragOver += new DragEventHandler(ddlEditor_DragOver);
      routineNamesListBox.DragDrop += new DragEventHandler(ddlEditor_DragDrop);
    }

    protected override void RefreshFormData()
    {
      nameTextBox.Text = routineGroupEditorBE.get_name();
      TabText = routineGroupEditorBE.get_title();
      commentTextBox.Text = routineGroupEditorBE.get_comment();
      refresh();
    }

    void listboxMenu_Popup(object sender, EventArgs e)
    {
      ((ContextMenu)sender).MenuItems[0].Enabled = routineNamesListBox.SelectedItem != null;
    }

    void removeItem_Click(object sender, EventArgs e)
    {
      if (routineNamesListBox.SelectedItem != null)
      {
        routineGroupEditorBE.delete_routine_with_name(routineNamesListBox.SelectedItem.ToString());
        routineGroupEditorBE.load_routines_sql();
      }
    }

    void openItem_Click(object sender, EventArgs e)
    {
      if (routineNamesListBox.SelectedIndex > -1)
        routineGroupEditorBE.open_editor_for_routine_at_index((uint)routineNamesListBox.SelectedIndex);
    }

    private void refresh()
    {
      routineNamesListBox.Items.Clear();

      List<String> nameslist = routineGroupEditorBE.get_routines_names();
      foreach (String name in nameslist)
        routineNamesListBox.Items.Add(name);
    }

    void routineNamesListBox_KeyDown(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Delete)
      {
        routineGroupEditorBE.delete_routine_with_name(((ListBox)sender).SelectedItem.ToString());
        refresh();
        routineGroupEditorBE.commit_changes();
      }
    }

    void ddlEditor_DragDrop(object sender, DragEventArgs e)
    {
      List<GrtValue> values = (List<GrtValue>)e.Data.GetData(typeof(List<GrtValue>));

      foreach (GrtValue value in values)
      {
        if (value.is_object_instance_of("db.mysql.Routine"))
        {
          routineGroupEditorBE.append_routine_with_id(value.object_id());
        }
      }
      routineGroupEditorBE.load_routines_sql();
      refresh();
      routineGroupEditorBE.commit_changes();
    }

    void ddlEditor_DragOver(object sender, DragEventArgs e)
    {
      e.Effect = DragDropEffects.None;
      if (e.Data.GetDataPresent(typeof(List<GrtValue>)))
      {
        List<GrtValue> values = (List<GrtValue>)e.Data.GetData(typeof(List<GrtValue>));
        bool has_routines = false;
        foreach (GrtValue value in values)
        {
          if (value.is_object_instance_of("db.mysql.Routine"))
          {
            has_routines = true;
            break;
          }
        }
        if (has_routines)
          e.Effect = DragDropEffects.Copy;
      }
    }

    void ddlEditor_DragEnter(object sender, DragEventArgs e)
    {
      ddlEditor_DragOver(sender, e);
    }

    private void sqlEditorParseLogCallback(List<String> strlist)
    {
      routineNamesListBox.Items.Clear();

      List<String> nameslist = routineGroupEditorBE.get_routines_names();
      foreach (String name in nameslist)
        routineNamesListBox.Items.Add(name);
    }

    private void nameTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && !nameTextBox.Text.Equals(routineGroupEditorBE.get_name()))
        routineGroupEditorBE.set_name(nameTextBox.Text);

      TabText = routineGroupEditorBE.get_title();
    }

    private void commentTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && !commentTextBox.Text.Equals(routineGroupEditorBE.get_comment()))
        routineGroupEditorBE.set_comment(commentTextBox.Text);
    }

    private void DbMysqlRoutineGroupEditor_Load(object sender, EventArgs e)
    {
      ActiveControl = nameTextBox;
    }

    #endregion

    private void routineNamesListBox_MouseDoubleClick(object sender, MouseEventArgs e)
    {
      if (routineNamesListBox.SelectedIndex > -1)
        routineGroupEditorBE.open_editor_for_routine_at_index((uint)routineNamesListBox.SelectedIndex);
    }

  }
}