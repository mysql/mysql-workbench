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
using System.IO;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Grt;
using MySQL.GUI.Mdc;
using MySQL.GUI.Workbench.Plugins;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class ModelDiagramForm : TabDocument, IWorkbenchDocument, IWorkbenchObserver
  {
    #region Member Variables
    
    private WbContext wbContext;
    private SortedDictionary<String, Cursor> cursors = new SortedDictionary<String, Cursor>();
    private ModelDiagramFormWrapper formBE = null;

    private ModelCatalogForm modelCatalogForm;
    private ModelLayerForm modelLayerForm;
    private ModelNavigatorForm modelNavigator;
    private ModelPropertiesForm modelPropertiesForm;
    private ModelObjectDescriptionForm modelObjectDescriptionForm;
    private UserDatatypesForm userDatatypesForm;
    private UndoHistoryForm historyForm;

    private bool splitterMovePending = false;
    private ToolStrip optionsToolStrip = null;
    private ToolStrip toolsToolStrip = null;

    #endregion

    #region Constructors

    public ModelDiagramForm(WbContext context, String id)
    {
      InitializeComponent();
      
      wbContext = context;

      CreateCanvas(id); // Sets formBE.

      canvasViewer.CanvasPanel.MouseMove += new MouseEventHandler(CanvasPanel_MouseMove);
      canvasViewer.CanvasPanel.MouseDown += new MouseEventHandler(CanvasPanel_MouseDown);
      canvasViewer.CanvasPanel.MouseUp += new MouseEventHandler(CanvasPanel_MouseUp);
      canvasViewer.CanvasPanel.MouseDoubleClick += new MouseEventHandler(CanvasPanel_MouseDoubleClick);
      canvasViewer.CanvasPanel.KeyDown += new KeyEventHandler(CanvasPanel_KeyDown);
      canvasViewer.CanvasPanel.KeyUp += new KeyEventHandler(CanvasPanel_KeyUp);
      canvasViewer.CanvasPanel.MouseLeave += new EventHandler(CanvasPanel_MouseLeave);

      // Sidebar windows.
      modelNavigator = new ModelNavigatorForm(this);
      userDatatypesForm = new UserDatatypesForm(wbContext);
      modelLayerForm = new ModelLayerForm(this);
      modelCatalogForm = new ModelCatalogForm(formBE);
      historyForm = new UndoHistoryForm(wbContext);
      modelPropertiesForm = new ModelPropertiesForm(wbContext);
      modelObjectDescriptionForm = new ModelObjectDescriptionForm(wbContext);

      SetupSideBars();

      toolsToolStrip = formBE.get_tools_toolbar();
      toolsToolStrip.Dock = DockStyle.Left;
      diagramPanel.Controls.Add(toolsToolStrip);

      optionsToolStrip = formBE.get_options_toolbar();
      optionsToolStrip.Padding = new Padding(2);
      optionsToolStrip.Dock = DockStyle.None;
      optionsToolStrip.AutoSize = false;
      diagramPanel.Controls.Add(optionsToolStrip);
      diagramPanel.Controls.SetChildIndex(optionsToolStrip, 0);
      optionsToolStrip.Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right;
      optionsToolStrip.Hide();

      UpdateColors();

      ManagedNotificationCenter.AddObserver(this, "GNFormTitleDidChange");
    }

    private void Destroy()
    {
      formBE.Dispose();

      ManagedNotificationCenter.RemoveObserver(this, "GNFormTitleDidChange");

      // No need to again close all docked editors here. This happened already in CloseDocument().

      canvasViewer.FinalizeCanvas();
    }

    #endregion
    
    #region IWorkbenchDocument Interface

    public UIForm BackendForm
    {
      get { return formBE; }
    }

    public ModelDiagramFormWrapper DiagramWrapper
    {
        get { return formBE; }
    }

    public void RefreshGUI(RefreshType refresh, String str, IntPtr ptr)
    {
      if (ShuttingDown)
        return;

      switch (refresh)
      {
        case RefreshType.RefreshCloseDocument:
          ShuttingDown = true;
          break;

        case RefreshType.RefreshSelection:
          if (ptr != null && ptr.ToInt64() != 0)
          {
            UIForm form = UIForm.GetFromFixedId(ptr);

            modelPropertiesForm.UpdateForForm(form);
            modelObjectDescriptionForm.UpdateForView(form);
          }
          else
          {
            modelPropertiesForm.UpdateForForm(formBE);
            modelObjectDescriptionForm.UpdateForView(formBE);
          }
          break;

        case RefreshType.RefreshDocument:
          TabText = formBE.get_title();

          break;

        case RefreshType.RefreshZoom:
          modelNavigator.UpdateDisplay();
          break;

        case RefreshType.RefreshCloseEditor:
          CloseEditorsForObject(str);
          break;

      }
    }

    /// <summary>
    /// Notifications coming in from the notification center.
    /// </summary>
    /// <param name="name">The name of the notification. We only get called for notifications we want.</param>
    /// <param name="sender">The object that caused the notification to be sent.</param>
    /// <param name="info">Name/Value string pairs for data to be passed.</param>
    public void HandleNotification(string name, IntPtr sender, Dictionary<string, string> info)
    {
      if (name == "GNFormTitleDidChange" && info["form"] == formBE.form_id())
        TabText = info["title"];
    }

    public void PerformCommand(String command)
    {
      switch (command)
      {
        case "view_model_navigator":
          ShowModelNavigator();
          break;
        case "view_catalog":
          ShowCatalog();
          break;
        case "view_layers":
          ShowLayers();
          break;
        case "view_user_datatypes":
          ShowUserDatatypes();
          break;
        case "view_object_properties":
          ShowProperties();
          break;
        case "view_object_description":
          ShowDescriptions();
          break;
        case "view_undo_history":
          ShowUndoHistory();
          break;
        case "wb.toggleSidebar":
          mainSplitContainer.Panel1Collapsed = !mainSplitContainer.Panel1Collapsed;
          diagramPanel.Invalidate();
          break;
        case "wb.toggleSecondarySidebar":
          mainContentSplitContainer.Panel2Collapsed = !mainContentSplitContainer.Panel2Collapsed;
          break;
      }
    }

    public void UpdateColors()
    {
      ApplyColors(this);
      modelCatalogForm.UpdateColors();
      modelNavigator.UpdateColors();
      modelObjectDescriptionForm.UpdateColors();

      if (Controls.Count > 0 && Controls[0] is DrawablePanel)
        Controls[0].BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      else
        BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);

      mainSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      contentSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      sideSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
    }

    public DockablePlugin FindPluginOfType(Type type)
    {
      foreach (ITabDocument content in bottomTabControl.Documents)
        if (content is ObjectEditorView && (content as ObjectEditorView).EditorPlugin.GetType() == type)
          return content as DockablePlugin;
      return null;
    }

    public bool ClosePluginOfType(Type type)
    {
      foreach (ITabDocument content in bottomTabControl.Documents)
        if (content is ObjectEditorView && (content as ObjectEditorView).EditorPlugin.GetType() == type)
        {
          // Unregister plugin from back end.
          wbContext.close_gui_plugin((content as ObjectEditorView).EditorPlugin.GetFixedPtr());

          content.Close();
          if (bottomTabControl.TabCount == 0)
            contentSplitContainer.Panel2Collapsed = true;
          return true;
        }
          
      return false;
    }

    public bool CanCloseDocument()
    {
      foreach (ITabDocument document in bottomTabControl.Documents)
        if (document is IWorkbenchDocument)
          if (!(document as IWorkbenchDocument).CanCloseDocument())
            return false;

      if (!BackendForm.can_close())
        return false;

      return true;
    }

    public void CloseDocument()
    {
      foreach (ITabDocument document in bottomTabControl.Documents)
        CloseTabDocument(document);
      BackendForm.close();
    }

    #endregion

    #region Properties

    public BaseWindowsCanvasView Canvas
    {
      get { return (BaseWindowsCanvasView)canvasViewer.Canvas; }
    }

    public double Zoom
    {
      get { return formBE.get_zoom(); }
      set { formBE.set_zoom(value); }
    }

    public bool ShuttingDown { get; set; }

    #endregion

    #region Event Handling

    private void canvasViewer_DragEnter(object sender, DragEventArgs e)
    {
      Point p = (sender as Control).PointToClient(new Point(e.X, e.Y));
      if (formBE.accepts_drop(p.X, p.Y, e.Data))
        e.Effect = DragDropEffects.Copy;
    }
    
    private void canvasViewer_DragDrop(object sender, DragEventArgs e)
    {
      Point p = (sender as Control).PointToClient(new Point(e.X, e.Y));
      formBE.perform_drop(p.X, p.Y, e.Data);
    }

    void CanvasPanel_MouseUp(object sender, MouseEventArgs e)
    {
      Point p = e.Location;
      formBE.OnMouseUp(e, p.X, p.Y, ModifierKeys, e.Button);
    }

    void CanvasPanel_MouseDown(object sender, MouseEventArgs e)
    {
      Point p = e.Location;
      formBE.OnMouseDown(e, p.X, p.Y, ModifierKeys, e.Button);
    }

    void CanvasPanel_MouseDoubleClick(object sender, MouseEventArgs e)
    {
      Point p = e.Location;
      formBE.OnMouseDoubleClick(e, p.X, p.Y, ModifierKeys, e.Button);
    }

    void CanvasPanel_MouseMove(object sender, MouseEventArgs e)
    {
      Point p = e.Location;
      formBE.OnMouseMove(e, p.X, p.Y, ModifierKeys, e.Button);
    }

    void CanvasPanel_MouseLeave(object sender, EventArgs e)
    {
      if (!IsDisposed && !Disposing)
        formBE.OnMouseMove(new MouseEventArgs(0, 0, -1, -1, 0), -1, -1, ModifierKeys, MouseButtons);
    }

    void CanvasPanel_KeyUp(object sender, KeyEventArgs e)
    {
      formBE.OnKeyUp(e, ModifierKeys);
    }

    void CanvasPanel_KeyDown(object sender, KeyEventArgs e)
    {
      formBE.OnKeyDown(e, ModifierKeys);
    }

    private void tabControl_TabClosing(object sender, TabClosingEventArgs e)
    {
      ITabDocument document = (sender as FlatTabControl).DocumentFromPage(e.page);
      if (document is IWorkbenchDocument)
        e.canClose = (document as IWorkbenchDocument).CanCloseDocument();
      else
        if (document is MySQL.Forms.AppViewDockContent)
        {
          MySQL.Forms.AppViewDockContent content = document as MySQL.Forms.AppViewDockContent;
          e.canClose = content.CanCloseDocument();
        }
    }

    private void bottomTabControl_TabClosed(object sender, MySQL.Controls.TabClosedEventArgs e)
    {
      if (bottomTabControl.TabCount == 0)
        contentSplitContainer.Panel2Collapsed = true;

      ITabDocument document = (sender as FlatTabControl).DocumentFromPage(e.page);
      CloseTabDocument(document);
    }

    private void CloseTabDocument(ITabDocument document)
    {
      if (document is IWorkbenchDocument)
        (document as IWorkbenchDocument).CloseDocument();
      else
        if (document is MySQL.Forms.AppViewDockContent)
        {
          MySQL.Forms.AppViewDockContent content = document as MySQL.Forms.AppViewDockContent;
          content.CloseDocument();
        }
    }
    private void ModelDiagramForm_Shown(object sender, EventArgs e)
    {
      LoadFormState();
    }

    #endregion

    #region Other Implementation

    private void SetupSideBars()
    {
      mainSplitContainer.SuspendLayout();
      sideSplitContainer.SuspendLayout();
      try
      {
        // Rebuild side bar.
        modelNavigator.TopLevel = false;
        navigatorHost.Controls.Add(modelNavigator);
        modelNavigator.Dock = DockStyle.Fill;
        modelNavigator.Show();

        sideTopTabControl.TabPages.Clear();

        DockSideDocument(modelCatalogForm, true, true);
        DockSideDocument(modelLayerForm, true, false);
        DockSideDocument(userDatatypesForm, true, false);

        DockSideDocument(modelObjectDescriptionForm, false, true);
        DockSideDocument(modelPropertiesForm, false, false);
        DockSideDocument(historyForm, false, false);

        contentSplitContainer.Panel2MinSize = 370;
        mainContentSplitContainer.Panel2MinSize = 200;
      }
      finally
      {
        mainSplitContainer.ResumeLayout(true);
        sideSplitContainer.ResumeLayout(true);
      }
    }

    private void DockSideDocument(ITabDocument document, bool top, bool activate)
    {
      if (top)
      {
        if (!sideTopTabControl.HasDocument(document))
        {
          int index = sideTopTabControl.AddDocument(document);
          sideTopTabControl.TabPages[index].BackColor = Color.White;
        }
      }
      else
      {
        if (!sideBottomTabControl.HasDocument(document))
        {
          int index = sideBottomTabControl.AddDocument(document);
          sideBottomTabControl.TabPages[index].BackColor = Color.White;
        }
      }
      if (activate)
        document.Activate();

      if (sideBottomTabControl.TabCount == 0)
        sideSplitContainer.Panel2Collapsed = true; // This will implicitly expand panel1.
      else
        sideSplitContainer.Panel1Collapsed = false;
      if (sideTopTabControl.TabCount == 0)
        sideSplitContainer.Panel1Collapsed = true; // This will implicitly expand panel2.
      else
        sideSplitContainer.Panel2Collapsed = false;
    }

    /// <summary>
    /// Docks the given document to the bottom tab control (like object editors etc.).
    /// </summary>
    /// <param name="document"></param>
    public void DockDocument(ITabDocument document, bool activate)
    {
      if (!bottomTabControl.HasDocument(document))
        bottomTabControl.AddDocument(document);
      if (activate)
        document.Activate();

      if (contentSplitContainer.Panel2Collapsed)
      {
        contentSplitContainer.Panel2Collapsed = false;

        // Set a splitter distance or we end up at almost full display. Use a relatively small
        // value for panel2. The document's minheight will kick in and does the right job.
        contentSplitContainer.SplitterDistance = contentSplitContainer.Height - 100;
      }
    }

    public void UndockDocument(ITabDocument document)
    {
      if (bottomTabControl.HasDocument(document))
        bottomTabControl.RemoveDocument(document);
    }

    public void CloseEditorsForObject(string oid)
    {
      ITabDocument[] documents = bottomTabControl.DocumentsToArray();

      // loop over all documents
      for (int i = documents.Length - 1; i >= 0; i--)
      {
        if (documents[i] is ObjectEditorView)
        {
          ObjectEditorView editor = documents[i] as ObjectEditorView;
          if (oid == "" || editor.EditorPlugin.ShouldCloseOnObjectDelete(oid))
            bottomTabControl.CloseDocument(editor);
        }
      }
    }

    public BaseWindowsCanvasView CreateCanvas(String id)
    {
      BaseWindowsCanvasView canvas = canvasViewer.CreateCanvasView(this, false,
        wbContext.software_rendering_enforced(), wbContext.opengl_rendering_enforced());
      formBE = wbContext.get_diagram_form_for_diagram(id);
      return canvas;
    }

    public void OnToolChanged()
    {
      formBE.update_options_toolbar();
      optionsToolStrip.Bounds = new Rectangle(toolsToolStrip.Width, diagramPanel.Padding.Top,
        canvasViewer.ClientSize.Width - toolsToolStrip.Width, 25);
      optionsToolStrip.Visible = optionsToolStrip.Items.Count > 0;
      SetCursor(formBE.get_tool_cursor());
    }

    private void SetCursor(string CursorFileName)
    {
      // if no cursor is specified, use default arrow
      if (CursorFileName.Equals(""))
      {
        canvasViewer.Cursor = System.Windows.Forms.Cursors.Default;
        return;
      }

      // Check if cursor already cached
      if (cursors.ContainsKey(CursorFileName))
        canvasViewer.Cursor = cursors[CursorFileName];
      else
      {
        // Load cursor
        Cursor c = null;
        string fullPath = Path.Combine("./images/cursors/", CursorFileName + ".cur");

        if (System.IO.File.Exists(fullPath))
        {
          c = new Cursor(fullPath);
          if (c != null)
          {
            // Add cursor to cache
            cursors.Add(CursorFileName, c);
            canvasViewer.Cursor = c;
          }
        }

        if (c == null)
          canvasViewer.Cursor = System.Windows.Forms.Cursors.Default;
      }
    }

    public void FocusCanvasControl()
    {
      ActiveControl = canvasViewer.CanvasPanel;
    }

    private void ShowModelNavigator()
    {
      mainSplitContainer.Panel2Collapsed = false;
      modelNavigator.Visible = true;
    }

    private void ShowCatalog()
    {
      mainSplitContainer.Panel2Collapsed = false;
      modelCatalogForm.Activate();
    }

    private void ShowLayers()
    {
      mainSplitContainer.Panel2Collapsed = false;
      modelLayerForm.Activate();
    }

    private void ShowUserDatatypes()
    {
      mainSplitContainer.Panel2Collapsed = false;
      userDatatypesForm.Activate();
    }

    private void ShowProperties()
    {
      mainSplitContainer.Panel2Collapsed = false;
      modelPropertiesForm.Activate();
    }

    private void ShowDescriptions()
    {
      mainSplitContainer.Panel2Collapsed = false;
      modelObjectDescriptionForm.Activate();
    }

    private void ShowUndoHistory()
    {
      mainSplitContainer.Panel2Collapsed = false;
      historyForm.Activate();
    }

    private void LoadFormState()
    {
      // Bounds are empty when the application is minimized while the form is constructed.
      if (Bounds.IsEmpty)
        return;

      int sidebarWidth = wbContext.read_state("sidebar_width", "model_diagram", 200);
      if (sidebarWidth < 0)
        sidebarWidth = 200;
      if (sidebarWidth > mainSplitContainer.Width)
        sidebarWidth = mainSplitContainer.Width / 2;

      mainSplitContainer.SplitterDistance = sidebarWidth;

      sidebarWidth = wbContext.read_state("secondary_sidebar_width", "model_diagram", mainContentSplitContainer.Width - 200);
      if (mainContentSplitContainer.Width - sidebarWidth < 200)
        sidebarWidth = mainContentSplitContainer.Width - 200;
      if (sidebarWidth > mainContentSplitContainer.Width)
        sidebarWidth = mainContentSplitContainer.Width / 2;

      mainContentSplitContainer.SplitterDistance = sidebarWidth;
    }

    private void mainSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (splitterMovePending)
      {
        splitterMovePending = false;
        wbContext.save_state("sidebar_width", "model_diagram", mainSplitContainer.SplitterDistance);
      }
    }

    private void mainContentSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (splitterMovePending)
      {
        splitterMovePending = false;
        wbContext.save_state("secondary_sidebar_width", "model_diagram", mainContentSplitContainer.SplitterDistance);
      }
    }

    private void splitterMoving(object sender, SplitterCancelEventArgs e)
    {
      // This event only comes up when the splitter is moved with the mouse (i.e. by the user).
      // We can use it to differentiate between user initiated and programmatic splitter changes.
      splitterMovePending = true;
    }

    private void ApplyColors(Control parent)
    {
      foreach (Control control in parent.Controls)
      {
        if (control is FlatTabControl)
        {
          FlatTabControl tabView = control as FlatTabControl;
          tabView.UpdateColors();
          tabView.BackgroundColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
        }
        else
          if (control is HeaderPanel)
          {
            HeaderPanel panel = control as HeaderPanel;
            panel.HeaderColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeader, false);
            panel.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeader, true);
            panel.HeaderColorFocused = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeaderFocused, false);
            panel.ForeColorFocused = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelHeaderFocused, true);
          }
          else
            if (control is ToolStrip)
            {
              ToolStrip toolStrip = control as ToolStrip;
              toolStrip.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, false);
              toolStrip.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelToolbar, true);
            }
            else
              if (control is TabPage)
              {
                TabPage page = control as TabPage;
                if (page.Parent is FlatTabControl)
                {
                  FlatTabControl view = page.Parent as FlatTabControl;
                  if (view.TabStyle == FlatTabControl.TabStyleType.BottomNormal)
                    page.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorPanelContentArea, false);
                }
              }

        ApplyColors(control);
      }
    }

    public override void Activate()
    {
      base.Activate();

      if (secondarySidebarPanel.Controls.Count == 0)
      {
        Control secondarySidebar = wbContext.shared_secondary_sidebar();
        secondarySidebarPanel.Controls.Add(secondarySidebar);
        secondarySidebar.Dock = DockStyle.Fill;
      }
    }

    #endregion

  }
}
