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
using System.Drawing;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Forms;
using MySQL.Grt;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench.Plugins
{
  /// <summary>
  /// Generic GRT Object Editor
  /// </summary>
  public partial class ObjectEditorPlugin : DockablePlugin
  {
    #region Member Variables

    /// <summary>
    /// Specifies if the controls on the form are currently getting initialized
    /// </summary>
    private bool initializingControls = false;

    private BaseEditorWrapper backend;
    private SqlEditorWrapper editorBackend;

    private SplitContainer mainSplitContainer = null;
    Button revertLiveObjectButton;
    Button applyLiveObjectButton;

    #endregion

    #region Constructors

    public ObjectEditorPlugin()
      : base()
    {
    }

    public ObjectEditorPlugin(GrtManager manager)
      : base(manager)
    {
    }

    public virtual bool ReinitWithArguments(GrtValue value)
    {
      return false;
    }

    #endregion

    #region Properties

    protected bool InitializingControls
    {
      get { return initializingControls; }
      set { initializingControls = value; }
    }

    public new Control ActiveControl
    {
      get
      {
        if (null != ContainerForm)
          return ContainerForm.ActiveControl;
        else
          return ActiveControl;
      }
      set
      {
        if (null != ContainerForm)
          ContainerForm.ActiveControl = value;
        else
          ActiveControl = value;
      }
    }

    public ObjectEditorView ContainerForm { get; set; }

    protected BaseEditorWrapper Backend
    {
      get { return backend; }
      set
      {
        if (backend != null)
          backend.Dispose();
        if (editorBackend != null)
        {
          Control editor = editorBackend.get_editor_container();
          if (editor.Parent != null)
            editor.Parent.Controls.Remove(editor);
          editor.Dispose();
          editorBackend.Dispose();
          editorBackend = null;
        }

        backend = value;
        if (backend != null)
        {
          backend.set_refresh_ui_handler(CallRefreshFormData);
          backend.set_refresh_partial_ui_handler(CallRefreshPartialFormData);
        }
      }
    }

    #endregion

    #region Notifications

    protected virtual void EditorTextChanged()
    {
      TabText = backend.get_title();
    }

    #endregion

    #region IWorkbenchDocument Interface

    public override UIForm BackendForm
    {
      get { return backend; }
    }

    public override void RefreshGUI(MySQL.Workbench.RefreshType refresh, String str, IntPtr ptr)
    {
    }

    public override void PerformCommand(String command)
    {
    }

    public override DockablePlugin FindPluginOfType(Type type)
    {
      if (GetType() == type)
        return this;
      return null;
    }

    public override bool ClosePluginOfType(Type type)
    {
      if (GetType() == type)
      {
        Close();
        return true;
      }
      return false;
    }

    public override bool CanCloseDocument()
    {
      bool result = base.CanCloseDocument();

      if (!result)
        return false;

      result = backend.can_close();
      if (!result)
        return false;

      return true;
    }

    public override void CloseDocument()
    {
      if (backend != null)
        backend.disable_auto_refresh();

      base.CloseDocument();
    }

    #endregion

    #region Form Implementation

    public Control SetupEditorOnHost(Control host, bool coloring)
    {
      if (backend != null && editorBackend == null)
        editorBackend = SqlEditorWrapper.get_sql_editor(backend);

      if (editorBackend != null)
      {
        if (coloring)
        {
          editorBackend.set_sql_check_enabled(true);
          editorBackend.set_language("mysql");
        }
        else
        {
          editorBackend.set_sql_check_enabled(false);
          editorBackend.set_language("");
        }

        Control editor = editorBackend.get_editor_container(); // The host of code editor, toolbar etc.
        host.Controls.Add(editor);
        editor.Dock = DockStyle.Fill;

        return editor;
      }

      return null;
    }

    public bool IsEditingLiveObject
    {
      get { return (null == backend) ? false : backend.is_editing_live_object(); }
    }

    protected void AdjustEditModeControls(Control mainTabControl)
    {
      bool isEditingLiveObject = IsEditingLiveObject;
      bool isEditingLiveObjectUI = ((mainSplitContainer != null) && (mainTabControl.Parent == mainSplitContainer.Panel1));
      if (isEditingLiveObject == isEditingLiveObjectUI)
        return;

      Control mainContainer = (isEditingLiveObjectUI) ? mainSplitContainer.Parent : mainTabControl.Parent;
      mainContainer.SuspendLayout();

      try
      {
        if (isEditingLiveObject)
        {
          if (null == mainSplitContainer)
          {
            mainSplitContainer = new SplitContainer();
            mainSplitContainer.Dock = DockStyle.Fill;
            mainSplitContainer.Orientation = Orientation.Horizontal;
            mainSplitContainer.SplitterWidth = 2;
            mainSplitContainer.FixedPanel = FixedPanel.Panel2;

            Panel liveObjectControlsPanel = new Panel();
            liveObjectControlsPanel.Parent = mainSplitContainer.Panel2;
            liveObjectControlsPanel.Dock = DockStyle.Fill;

            {
              revertLiveObjectButton = new Button();
              revertLiveObjectButton.UseVisualStyleBackColor = true;
              revertLiveObjectButton.Parent = liveObjectControlsPanel;
              revertLiveObjectButton.FlatStyle = FlatStyle.System;
              revertLiveObjectButton.Text = "Revert";
              revertLiveObjectButton.Location = new Point(
                mainSplitContainer.Panel2.ClientSize.Width - revertLiveObjectButton.Width - revertLiveObjectButton.Margin.Right,
                revertLiveObjectButton.Margin.Top);
              revertLiveObjectButton.Click += new EventHandler(revertChangesToLiveObjectButton_Click);
            }
            {
              applyLiveObjectButton = new Button();
              applyLiveObjectButton.UseVisualStyleBackColor = true;
              applyLiveObjectButton.Parent = liveObjectControlsPanel;
              applyLiveObjectButton.FlatStyle = FlatStyle.System;
              applyLiveObjectButton.Text = "Apply";
              applyLiveObjectButton.Location = new Point(
                revertLiveObjectButton.Location.X - revertLiveObjectButton.Margin.Left - applyLiveObjectButton.Width - applyLiveObjectButton.Margin.Right,
                revertLiveObjectButton.Location.Y);
              applyLiveObjectButton.Click += new EventHandler(applyChangesToLiveObjectButton_Click);
            }

            mainSplitContainer.Panel2MinSize = revertLiveObjectButton.Height + revertLiveObjectButton.Margin.Vertical;
            mainSplitContainer.SplitterDistance = mainContainer.ClientSize.Height - mainSplitContainer.Panel2MinSize - mainSplitContainer.SplitterWidth;

            revertLiveObjectButton.Anchor =   AnchorStyles.Right | AnchorStyles.Bottom;
            applyLiveObjectButton.Anchor = AnchorStyles.Right | AnchorStyles.Bottom;
          }

          mainSplitContainer.Parent = mainTabControl.Parent;
          mainTabControl.Parent = mainSplitContainer.Panel1;

       }
        else
        {
          mainSplitContainer.Parent = null;
          mainTabControl.Parent = mainContainer;
        }
      }
      finally
      {
        mainContainer.ResumeLayout();
      }
    }

    private void applyChangesToLiveObjectButton_Click(object sender, EventArgs e)
    {
      backend.apply_changes_to_live_object();
    }

    private void revertChangesToLiveObjectButton_Click(object sender, EventArgs e)
    {
      backend.revert_changes_to_live_object();
    }

    public override string TabText
    {
      set
      {
        base.TabText = value;
        if (null != ContainerForm)
          ContainerForm.TabText = value;
      }
    }

    public bool ShouldCloseOnObjectDelete(String oid)
    {
      return backend.should_close_on_delete_of(oid);
    }

    protected virtual void RefreshFormData() {}

    private void CallRefreshFormData() 
    {
      if (InvokeRequired)
        Invoke((Action)(() => RefreshFormData()));
      else
        RefreshFormData();
    }

    protected virtual void RefreshPartialFormData(int what)
    {
      // Some general handling for all plugins. For handling of other refresh types
      // override this method in the particular plugin.
      switch ((BaseEditorWrapper.PartialRefreshType)what)
      {
        case BaseEditorWrapper.PartialRefreshType.RefreshTextChanged:
          EditorTextChanged();
          break;
      }
    }

    private void CallRefreshPartialFormData(int what) 
    {
      if (InvokeRequired)
        Invoke((Action)(() => RefreshPartialFormData(what)));
      else
        RefreshPartialFormData(what);
    }

    public void ActivateEditor()
    {
      ActiveControl = editorBackend.get_editor_container();
    }

    #endregion


  }
}