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
using System.ComponentModel;
using System.Windows.Forms;

using MySQL.Grt;
using MySQL.Forms;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class StoredNoteEditor : ObjectEditorPlugin
  {
    #region Member Variables

    private MySQL.Grt.StoredNoteEditorWrapper storedNoteEditorBE { get { return Backend as MySQL.Grt.StoredNoteEditorWrapper; } }

    #endregion

    #region Constructors

    public StoredNoteEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      if (Backend != null && Backend.is_editor_dirty())
        return false; // Will open the plugin in a new editor window instead.

      InitializingControls = true;

      try
      {
        Backend = new MySQL.Grt.StoredNoteEditorWrapper(value);
        SetupEditorOnHost(content, storedNoteEditorBE.is_sql_script());
        storedNoteEditorBE.load_text();
        EditorTextChanged();
      }
      finally
      {
        InitializingControls = false;
      }

      Invalidate();

      return true;
    }

    #endregion

    #region Form implemenation

    private void applyButton_Click(object sender, EventArgs e)
    {
      storedNoteEditorBE.commit_changes();
      EditorTextChanged();
      ActivateEditor();
    }

    private void discardButton_Click(object sender, EventArgs e)
    {
      storedNoteEditorBE.load_text();
      EditorTextChanged();
      ActivateEditor();
    }

    protected override void EditorTextChanged()
    {
      base.EditorTextChanged(); // TabText is updated in base.

      applyButton.Enabled = Backend.is_editor_dirty();
      discardButton.Enabled = Backend.is_editor_dirty();
    }

    public override bool CanCloseDocument()
    {
      if (!base.CanCloseDocument())
      {
        CustomMessageBox.Show(MessageType.MessageWarning, "Editor Content Changed", "There are pending changes that " +
          "must be applied first before you can close the editor. Alternatively you can revert to the previous state. ", "OK");
        return false;
      }
      return true;
    }

    #endregion

  }
}