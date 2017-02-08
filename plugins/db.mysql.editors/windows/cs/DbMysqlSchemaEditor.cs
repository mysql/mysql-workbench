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
using System.ComponentModel;
using System.Windows.Forms;

using MySQL.Grt;
using MySQL.Grt.Db;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class DbMysqlSchemaEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private MySQLSchemaEditorWrapper schemaEditorWrapper { get { return Backend as MySQLSchemaEditorWrapper; } } 
    protected bool settingOptions = false;

    #endregion

    #region Constructors

    public DbMysqlSchemaEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);

      if (IsEditingLiveObject)
        AdjustEditModeControls(mainTabControl);
    }

    #endregion

    #region Form implemenation

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;
      SuspendLayout();

      try
      {
        Backend = new MySQLSchemaEditorWrapper(value);

        RefreshFormData();
      }
      finally
      {
        ResumeLayout();
        InitializingControls = false;
      }

      Invalidate();

      return true;
    }

    protected override void RefreshFormData()
    {
      if (settingOptions)
        return;

      if (schemaEditorWrapper.is_editing_live_object() && !schemaEditorWrapper.is_new_object())
        nameTextBox.Enabled = false;

      optComments.Visible = label7.Visible = !IsEditingLiveObject;

      try
      {
        InitializingControls = true;
        nameTextBox.Text = schemaEditorWrapper.get_name();

        System.Collections.Generic.List<String> collations_list = schemaEditorWrapper.get_charset_collation_list();

        optCollation.Items.Clear();
        optCollation.Items.Add("Server Default");
        optCollation.Items.AddRange(collations_list.ToArray());

        int idx = 0;
        String cscoll = schemaEditorWrapper.get_schema_option_by_name("CHARACTER SET - COLLATE");
        bool found = false;
        foreach (String next_cscoll in optCollation.Items)
        {
          if (next_cscoll == cscoll)
          {
            found = true;
            optCollation.SelectedIndex = idx;
            break;
          }
          idx++;
        }
        if (!found)
          optCollation.SelectedIndex = 0;

        TabText = schemaEditorWrapper.get_title();

        optComments.Text = schemaEditorWrapper.get_comment();

        refactorButton.Enabled = schemaEditorWrapper.refactor_possible();
      }
      finally
      {
        InitializingControls = false;
      }
    }

    private void setSchemaOpt()
    {
      // When a UI value changes, update the schema options
      if (InitializingControls)
        return;

      settingOptions = true;
      try
      {
        // set charset/collation
        //If there is no "-" in optCollation.Text like in case of "Server default" collation and charset will be reset to ""
        schemaEditorWrapper.set_schema_option_by_name("CHARACTER SET - COLLATE", optCollation.Text);

        if (!optComments.Text.Equals(schemaEditorWrapper.get_comment()))
          schemaEditorWrapper.set_comment(optComments.Text);

      }
      finally
      {
        settingOptions = false;
      }
    }

    private void DbMysqlSchemaEditor_Shown(object sender, EventArgs e)
    {
      if (nameTextBox.Enabled)
        nameTextBox.Focus();
    }

    #endregion

    #region Event handling

    private void nameTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && !nameTextBox.Text.Equals(schemaEditorWrapper.get_name()))
        schemaEditorWrapper.set_name(nameTextBox.Text);

      TabText = schemaEditorWrapper.get_title();
    }

    private void optCollation_SelectedIndexChanged(object sender, EventArgs e)
    {
      setSchemaOpt();
    }

    private void optComments_TextChanged(object sender, EventArgs e)
    {
      setSchemaOpt();
    }

    private void refactorButton_Click(object sender, EventArgs e)
    {
      schemaEditorWrapper.refactor_catalog();
    }

    #endregion

  }
}
