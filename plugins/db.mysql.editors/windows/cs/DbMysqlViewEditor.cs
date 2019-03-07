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

using MySQL.Grt;
using MySQL.Grt.Db;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class DbMysqlViewEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private MySQLViewEditorWrapper viewEditorBE { get { return Backend as MySQLViewEditorWrapper; } } 
    private DbObjectEditorPages dbObjectEditorPages;

    #endregion

    #region Constructors

    public DbMysqlViewEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);

      if (IsEditingLiveObject)
        AdjustEditModeControls(mainTabControl);
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;

      try
      {
        Backend = new MySQLViewEditorWrapper(value);
        SetupEditorOnHost(panel1, true);
        viewEditorBE.load_view_sql();

        InitFormData();
        RefreshFormData();

        Backend.reset_editor_undo_stack();
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

    private void DbMysqlViewEditor_Load(object sender, EventArgs e)
    {
      ActivateEditor();
    }

    protected void InitFormData()
    {
      // Add privileges tab sheet
      if (dbObjectEditorPages != null)
        mainTabControl.TabPages.Remove(dbObjectEditorPages.PrivilegesTabPage);
      dbObjectEditorPages = new DbObjectEditorPages(GrtManager, viewEditorBE);
      if (!IsEditingLiveObject)
          mainTabControl.TabPages.Add(dbObjectEditorPages.PrivilegesTabPage);
      else
          mainTabControl.TabPages.Remove(commentsTabpage);
    }

    protected override void RefreshFormData()
    {
      nameTextBox.Text = viewEditorBE.get_name();
      TabText = viewEditorBE.get_title();
      commentTextBox.Text = viewEditorBE.get_comment();
      viewEditorBE.load_view_sql();
    }

    private void nameTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && !nameTextBox.Text.Equals(viewEditorBE.get_name()))
        viewEditorBE.set_name(nameTextBox.Text);

      TabText = viewEditorBE.get_title();
    }

    private void commentTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && !commentTextBox.Text.Equals(viewEditorBE.get_comment()))
        viewEditorBE.set_comment(commentTextBox.Text);
    }

    #endregion

  }
}