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

        var charsetLlist = schemaEditorWrapper.get_charset_list();
        optCharset.Items.Clear();
        optCharset.Items.AddRange(charsetLlist.ToArray());

        var charset = schemaEditorWrapper.get_schema_option_by_name("CHARACTER SET");
        var idx = optCharset.FindString(charset);
        if (idx < 0)
          idx = 0;
        optCharset.SelectedIndex = idx;

        var selectedValue = "Default Charset";
        if (optCharset.SelectedItem != null)
          selectedValue = optCharset.SelectedItem.ToString();
        var collation = schemaEditorWrapper.get_charset_collation_list(selectedValue);
        optCollation.Items.Clear();
        optCollation.Items.AddRange(collation.ToArray());

        var collate = schemaEditorWrapper.get_schema_option_by_name("COLLATE");
        idx = optCollation.FindString(charset);
        if (idx < 0)
          idx = 0;
        optCollation.SelectedIndex = idx;

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
        if (optCharset.Text == "Default Charset")
          schemaEditorWrapper.set_schema_option_by_name("CHARACTER SET", "");
        else
          schemaEditorWrapper.set_schema_option_by_name("CHARACTER SET", optCharset.Text);

        if(optCollation.Text == "Default Collation")
          schemaEditorWrapper.set_schema_option_by_name("COLLATE", "");
        else
          schemaEditorWrapper.set_schema_option_by_name("COLLATE", optCollation.Text);

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

    private void optCharset_SelectedIndexChanged(object sender, EventArgs e) {
      var cbCharset = (ComboBox)sender;
      if (cbCharset != null && cbCharset.SelectedItem != null) {
        var collation = schemaEditorWrapper.get_charset_collation_list(cbCharset.SelectedItem.ToString());
        optCollation.Items.Clear();
        optCollation.Items.AddRange(collation.ToArray());
        optCollation.SelectedIndex = 0;
        setSchemaOpt();
      }
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
