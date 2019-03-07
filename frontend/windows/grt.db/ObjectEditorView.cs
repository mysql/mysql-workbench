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
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Grt;

namespace MySQL.GUI.Workbench.Plugins
{
  public partial class ObjectEditorView : DockablePlugin
  {
    private ObjectEditorPlugin editorPlugin;
    public ObjectEditorPlugin EditorPlugin
    {
      get { return editorPlugin; }
    }

    protected ObjectEditorView()
      : base()
		{
		}
    
    public ObjectEditorView(ObjectEditorPlugin plugin)
      : base(plugin.GrtManager)
    {
      InitializeComponent();

      plugin.ContainerForm = this;
      Text = plugin.Text;
      TabText = plugin.TabText;
      SetEditorPlugin(plugin);
    }

    private void Destroy()
    {
      if (editorPlugin != null)
      {
        editorPlugin.Dispose();
        editorPlugin = null;
      }
    }

    protected void SetEditorPlugin(ObjectEditorPlugin value)
    {
      editorPlugin = value;
      Controls.Clear();

      if (editorPlugin != null)
      {
        // That's quite an ugly hack. Relocate all controls from one DockablePlugin to the other.
        while (editorPlugin.Controls.Count > 0)
          editorPlugin.Controls[0].Parent = this;
      }
    }

    public override bool CanCloseDocument()
    {
      return EditorPlugin.CanCloseDocument();
    }

    public override void CloseDocument()
    {
      EditorPlugin.CloseDocument();
    }

    override public void Show()
    {
      base.Show();

      editorPlugin.Show();
    }


    #region IWorkbenchDocument implementation

    public override UIForm BackendForm
    {
      get { return editorPlugin.BackendForm; }
    }

    public override DockablePlugin FindPluginOfType(Type type)
    {
      if (EditorPlugin.GetType() == type)
        return this;
      return null;
    }

    public override bool ClosePluginOfType(Type type)
    {
      if (EditorPlugin.GetType() == type)
      {
        Close();
        return true;
      }
      return false;
    }

    #endregion

    public override IntPtr GetFixedPtr()
    {
      return editorPlugin.GetFixedPtr();
    }

  }
}
