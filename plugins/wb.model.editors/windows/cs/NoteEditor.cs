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

using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class NoteEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private MySQL.Grt.NoteEditorWrapper noteEditorBE { get { return Backend as MySQL.Grt.NoteEditorWrapper; } }

    #endregion

    #region Constructors

    public NoteEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);
    }

    #endregion


    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      InitializingControls = true;
      SuspendLayout();

      try
      {
        Backend = new MySQL.Grt.NoteEditorWrapper(value);

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

    #endregion

    #region Form implemenation

    protected override void RefreshFormData()
    {
      sqlTextBox.Text = noteEditorBE.get_text();
      nameTextBox.Text = noteEditorBE.get_name();

      TabText = noteEditorBE.get_title();
    }

    #endregion

    private void sqlTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && sqlTextBox.Text != noteEditorBE.get_text())
        noteEditorBE.set_text(sqlTextBox.Text);
    }

    private void nameTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && nameTextBox.Text != noteEditorBE.get_name())
      {
        noteEditorBE.set_name(nameTextBox.Text);
        TabText = noteEditorBE.get_title();
      }
    }

  }
}