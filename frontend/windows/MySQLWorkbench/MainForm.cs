/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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
using System.Reflection;
using System.Windows.Forms;

using Aga.Controls.Tree;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Forms;
using MySQL.Grt;
using MySQL.GUI.Mdc;
using MySQL.GUI.Workbench.Plugins;
using MySQL.GUI.Workbench.Properties;
using MySQL.Utilities;
using MySQL.Utilities.SysUtils;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class MainForm : Form, IWorkbenchObserver
  {
    #region Member Variables

    static private String[] resourcePaths = {
      "",
      "images/ui",
      "images/icons",
      "images/grt",
      "images/grt/structs",
      "images/cursors",
      "images/home",
      "images/sql",
      "images/sql/mac",
    };

    // The Workbench context
    protected WbContext wbContext;
    // the GRT manager
    protected GrtManager grtManager;

    // A timer for helping backend timers.
    private System.Windows.Forms.Timer timer = null;

    // Special Forms
    private ModelOverviewForm workbenchPhysicalOverviewForm = null;

    private bool shuttingDown = false;
    private ImageList tabImageList = null;

    public MainPageDockDelegate dockDelegate = null;

    #endregion

    #region Constructors

    /// <summary>
    /// Standard constructor
    /// </summary>
    private MainForm()
    {
      InitializeComponent();
    }

    /// <summary>
    /// Constructor that takes a WbContext and passes it to the sub-forms that get created
    /// </summary>
    /// <param name="WbContext">The WbContext Backend Wrapper</param>
    public MainForm(WbContext WbContext)
      : this()
    {
      wbContext = WbContext;
      grtManager = wbContext.get_grt_manager();

      dockDelegate = new MainPageDockDelegate(this, null);

      tabImageList = new ImageList();
      tabImageList.ColorDepth = ColorDepth.Depth32Bit;
      tabImageList.ImageSize = new Size(18, 16);
      ImageListHelper.Add(ApplicationCommand(AppCommand.AppGetResourcePath, "WB_Home.png"), tabImageList);
      contentTabControl.ImageList = tabImageList;

      // Create a timer to be triggered when the backend needs
      timer = new System.Windows.Forms.Timer();
      timer.Tick += new EventHandler(timer_Tick);

      // Prepare Statusbar
      PictureBox statusStripImg = new PictureBox();
      statusStripImg.SizeMode = PictureBoxSizeMode.CenterImage;
      statusStripImg.Image = Resources.statusbar_separator;
      statusStripImg.BackColor = Color.Transparent;
      ToolStripControlHost host = new ToolStripControlHost(statusStripImg);
      host.Alignment = ToolStripItemAlignment.Right;
      mainStatusStrip.Items.Add(host);

      // output img
      statusStripImg = new PictureBox();
      statusStripImg.Name = "grtShellStripButton";
      statusStripImg.SizeMode = PictureBoxSizeMode.CenterImage;
      statusStripImg.Image = Resources.statusbar_output;
      statusStripImg.BackColor = Color.Transparent;
      mainFormToolTip.SetToolTip(statusStripImg, "Display Output Window");
      host = new ToolStripControlHost(statusStripImg);
      host.Alignment = ToolStripItemAlignment.Right;
      mainStatusStrip.Items.Add(host);

      statusStripImg = new PictureBox();
      statusStripImg.SizeMode = PictureBoxSizeMode.CenterImage;
      statusStripImg.Image = Resources.statusbar_separator;
      statusStripImg.BackColor = Color.Transparent;
      host = new ToolStripControlHost(statusStripImg);
      host.Alignment = ToolStripItemAlignment.Right;
      mainStatusStrip.Items.Add(host);

      // Listen to system color changes.
      Microsoft.Win32.SystemEvents.UserPreferenceChanged += new Microsoft.Win32.UserPreferenceChangedEventHandler(PreferenceChangedHandler);

      ManagedNotificationCenter.AddObserver(this, "GNColorsChanged");
      ManagedNotificationCenter.AddObserver(this, "GNFocusChanged");
    }

    void Destroy()
    {
      ManagedNotificationCenter.RemoveObserver(this, "");
      Microsoft.Win32.SystemEvents.UserPreferenceChanged -= new Microsoft.Win32.UserPreferenceChangedEventHandler(PreferenceChangedHandler);

      // All documents are closed in Form_Closing.
    }

    #endregion

    #region Implement Interfaces

    delegate void DelegateFunc();

    public string StatusBarText
    {
      get { return statusText.Text; }
      set
      {
        if (InvokeRequired)
        {
          DelegateFunc f = delegate
          {
            statusText.Text = value.Replace("\r\n", "\n").Replace("\n", "");
          };

          BeginInvoke(f);
        }
        else
          statusText.Text = value.Replace("\r\n", "\n").Replace("\n", "");
      }
    }

    #endregion

    #region IWorkbenchObserver interface

    public void HandleNotification(string name, IntPtr sender, Dictionary<string, string> info)
    {
      switch (name)
      {
        case "GNColorsChanged":
          UpdateColors();
          break;

        case "GNFocusChanged":
          wbContext.validate_edit_menu();
          break;
      }
    }

    #endregion

    #region Properties

    public ModelOverviewForm WorkbenchPhysicalOverviewForm
    {
      get { return workbenchPhysicalOverviewForm; }
    }

    #endregion

    #region Callbacks

    public void PreferenceChangedHandler(object sender, Microsoft.Win32.UserPreferenceChangedEventArgs e)
    {
      if (e.Category == Microsoft.Win32.UserPreferenceCategory.VisualStyle     // For changing from high contrast to normal style.
        || e.Category == Microsoft.Win32.UserPreferenceCategory.Accessibility) // For changing between high contrast schemes.
      {
        if (SystemInformation.HighContrast)
          Conversions.SetColorScheme(ColorScheme.ColorSchemeHighContrast);
        else
        {
          Conversions.SetColorScheme(ColorScheme.ColorSchemeStandard);

          // Re-establish our glass frame.
          int topAreaHeight = 0;
          if (contentTabControl.ActiveDocument != null)
            topAreaHeight += contentTabControl.ActiveDocument.ToolbarHeight;

          AdjustGlassFrame(topAreaHeight);
        }
      }
    }

    public void ShowStatusText(String message)
    {
      StatusBarText = message;
    }

    public bool ShowProgress(String title, String detail, float progress)
    {
      if (progress < 0)
      {
        if (title != "" && detail != "")
          StatusBarText = title + ": " + detail;
        else if (title != "")
          StatusBarText = title;
        else
          StatusBarText = "Finished";

        statusProgress.Visible = false;
        return false;
      }
      else if (!statusProgress.Visible)
        statusProgress.Visible = true;

      if (title != "" && detail != "")
        StatusBarText = title + ": " + detail;
      else
        StatusBarText = title;

      statusProgress.Value = (int)(progress * 100);

      return false;
    }

    public String ApplicationCommand(AppCommand command, String str)
    {
      if (IsDisposed || Disposing)
        return "";

      String result = "";
      switch (command)
      {
        case AppCommand.AppQuit: // Not yet used.
          Close();
          break;
        case AppCommand.AppGetResourcePath:
          result = GetIconPath(str);
          break;
        case AppCommand.AppSetStatusText:
          StatusBarText = str;
          break;
      }
      return result;
    }

    public void RefreshGUI(RefreshType refresh, String str, IntPtr ptr)
    {
      if (IsDisposed || Disposing)
        return;

      SuspendLayout();

      switch (refresh)
      {
        case RefreshType.RefreshCloseDocument:
          {
            ForwardRefreshToAllDocuments(refresh, str, ptr);
            if (workbenchPhysicalOverviewForm != null)
            {
              workbenchPhysicalOverviewForm.ResetDocument(true);
              workbenchPhysicalOverviewForm.Close();
              workbenchPhysicalOverviewForm = null;
            }

            wbContext.flush_idle_tasks(true);
            wbContext.close_document_finish();

            break;
          }
        case RefreshType.RefreshNewDiagram:
          {
            BaseWindowsCanvasView canvas = BaseWindowsCanvasView.GetFromFixedId(ptr);
            if (canvas != null)
            {
              // Open only diagrams which were open when this model was closed last time.
              ModelDiagramForm modelDiagramForm = canvas.GetOwnerForm() as ModelDiagramForm;
              if (modelDiagramForm != null && !modelDiagramForm.DiagramWrapper.is_closed())
                DockDocument(modelDiagramForm, true, true);
            }
            break;
          }

        case RefreshType.RefreshNewModel: // A new model was loaded or created. Show UI.
          ShowPhysicalOverviewForm(true);

          // Let the backend create what is necessary.
          wbContext.new_model_finish();
          break;

        case RefreshType.RefreshDocument:
        case RefreshType.RefreshCloseEditor:
        case RefreshType.RefreshOverviewNodeInfo:
          ForwardRefreshToAllDocuments(refresh, str, ptr);
          break;

        case RefreshType.RefreshSchemaNoReload:
        case RefreshType.RefreshSelection:
        case RefreshType.RefreshZoom:
          ForwardRefreshToActivDocument(refresh, str, ptr);
          break;

        case RefreshType.RefreshTimer:
          UpdateTimer();
          break;

        case RefreshType.RefreshFinishEdits:
          // Force all ongoing edits to be Commited (like in listview cells)
          try
          {
            Control activeControl = ControlUtilities.GetLeafActiveControl(this);
            if (activeControl != null)
            {
              if (activeControl.Parent is Aga.Controls.Tree.TreeViewAdv)
              {
                ActiveControl = null;
              }
            }
          }
          catch (Exception e)
          {
            Program.HandleException(e);
          }
          break;

        case RefreshType.RefreshOverviewNodeChildren:
          if (workbenchPhysicalOverviewForm != null)
            workbenchPhysicalOverviewForm.RefreshGUI(refresh, str, ptr);
          break;
      }

      ResumeLayout();
    }

    /// <summary>
    /// Creates a new diagram form with a given name and id. UI handling (e.g. docking) is done later.
    /// </summary>
    public BaseWindowsCanvasView CreateNewDiagram(string viewId, string name)
    {
      ModelDiagramForm modelDiagramForm = new ModelDiagramForm(wbContext, viewId);

      modelDiagramForm.Text = name;
      modelDiagramForm.TabText = name;

      return modelDiagramForm.Canvas;
    }

    /// <summary>
    /// Frees a model diagram form once its backend was freed.
    /// </summary>
    public void DestroyView(BaseWindowsCanvasView canvasView)
    {
      ModelDiagramForm modelDiagramForm = canvasView.GetOwnerForm() as ModelDiagramForm;
      if (modelDiagramForm != null)
      {
        modelDiagramForm.Close();
        if (workbenchPhysicalOverviewForm != null)
          workbenchPhysicalOverviewForm.Activate();
      }
    }

    public void SwitchedView(BaseWindowsCanvasView canvasView)
    {
      ModelDiagramForm modelDiagramForm = canvasView.GetOwnerForm() as ModelDiagramForm;
      if (modelDiagramForm != null)
        DockDocument(modelDiagramForm, true, true);
    }

    public void ToolChanged(BaseWindowsCanvasView canvasView)
    {
      ModelDiagramForm modelDiagramForm = canvasView.GetOwnerForm() as ModelDiagramForm;
      if (modelDiagramForm != null)
        modelDiagramForm.OnToolChanged();
    }

    public IntPtr OpenPlugin(GrtManager GrtManager, GrtModule GrtModule, string AssemblyName,
      string ClassName, GrtValue GrtList, GUIPluginFlags flags)
    {
      IntPtr ptr = IntPtr.Zero;

      try
      {
        // Load assembly
        Assembly assembly = Assembly.LoadFrom(System.IO.Path.Combine(
          Application.StartupPath, AssemblyName));

        // Find class
        foreach (Type type in assembly.GetTypes())
        {
          if (type.IsClass == true && type.FullName.EndsWith("." + ClassName))
          {
            // use global grtManager
            Object[] args = { grtManager, GrtList };

            if (typeof(DockablePlugin).IsAssignableFrom(type))
            {
              // If ForceNewWindowFlag is not set and this is an object editor
              if ((GUIPluginFlags.ForceNewWindowFlag & flags) == 0 &&
                (GUIPluginFlags.StandaloneWindowFlag & flags) == 0 &&
                typeof(ObjectEditorPlugin).IsAssignableFrom(type))
              {
                // Check if a plugin of this type is already open. If we can find one on the active
                // document page then reuse that. If not try the other pages and close the first
                // editor of that plugin type we find.
                DockablePlugin plugin = null;
                if (contentTabControl.ActiveDocument is IWorkbenchDocument)
                  plugin = (contentTabControl.ActiveDocument as IWorkbenchDocument).FindPluginOfType(type);

                if (plugin == null)
                {
                  foreach (ITabDocument document in contentTabControl.Documents)
                    if (document is IWorkbenchDocument && document != contentTabControl.ActiveDocument)
                    {
                      if ((document as IWorkbenchDocument).ClosePluginOfType(type))
                        break;
                    }
                }
                else
                {
                  // If so, try to change the current GRT Object and exit
                  if ((plugin as ObjectEditorView).EditorPlugin.ReinitWithArguments(GrtList))
                  {
                    // Unregister plugin from back end.
                    wbContext.close_gui_plugin(plugin.GetFixedPtr());

                    // return old Ptr as Ptr for the new editor
                    return plugin.GetFixedPtr();
                  }
                  else
                    Logger.LogDebug("Plugins .NET", 1, String.Format("Object editor for {0} does not support reuse", ClassName));
                }
              }

              ObjectEditorPlugin objectEditorPlugin = Activator.CreateInstance(type, args) as ObjectEditorPlugin;
              if (objectEditorPlugin != null)
              {
                objectEditorPlugin.Context = wbContext;

                ObjectEditorView pluginForm;
                if ((GUIPluginFlags.StandaloneWindowFlag & flags) != 0)
                  pluginForm = new StandaloneWindowPlugin(objectEditorPlugin);
                else
                  pluginForm = new ObjectEditorView(objectEditorPlugin);

                // Get fixed ptr
                ptr = pluginForm.GetFixedPtr();

                if (ptr == IntPtr.Zero)
                  throw new Exception("Internal error: could not pin memory for plugin.");
              }

              break;
            }
            else if (typeof(Plugin).IsAssignableFrom(type))
            {
              Plugin plugin = Activator.CreateInstance(type, args) as Plugin;

              try
              {
                plugin.Execute();
              }
              catch (Exception e)
              {
                Program.HandleException(e);
              }
            }
          }
        }
      }
      catch (Exception e)
      {
        Program.HandleException(e);
      }

      return ptr;
    }

    public void CreateMainFormView(String ViewName, UIForm viewBE)
    {
      try
      {
        DockablePlugin form = null;
        switch (ViewName)
        {
          case "dbquery":
            form = new SqlIdeForm(wbContext, viewBE);
            break;
        }
        if (form != null)
        {
          form.Context = wbContext;
          DockDocument(form, true, true);
        }
      }
      catch (Exception e)
      {
        Program.HandleException(e);
      }
    }

    public void ShowPlugin(IntPtr ptr)
    {
      if (ptr != IntPtr.Zero)
      {
        TabDocument pluginForm = DockablePlugin.GetFromFixedPtr(ptr);
        if (pluginForm != null)
        {
          if (pluginForm is ObjectEditorPlugin)
          {
            ObjectEditorView objectEditorView = (pluginForm as ObjectEditorPlugin).ContainerForm;
            if (objectEditorView is StandaloneWindowPlugin)
              objectEditorView.Show();
            else if (objectEditorView is ObjectEditorView)
              DockDocument(objectEditorView, false, true);
          }
          else if (pluginForm is DockablePlugin && !(pluginForm is WizardPlugin))
            DockDocument(pluginForm, false, true);
          else
            pluginForm.ShowDialog();
        }
      }
    }

    public void HidePlugin(IntPtr ptr)
    {
      if (ptr != IntPtr.Zero)
      {
        TabDocument editorForm = DockablePlugin.GetFromFixedPtr(ptr);

        if (editorForm is ObjectEditorPlugin)
          editorForm = (editorForm as ObjectEditorPlugin).ContainerForm;

        if (editorForm != null)
          editorForm.Hide();
      }
    }
    #endregion

    #region DockingPointDelegate

    public bool select_view(AppViewDockContent view)
    {
      foreach (ITabDocument document in contentTabControl.Documents)
      {
        if (document is AppViewDockContent)
        {
          AppViewDockContent content = (document as AppViewDockContent);
          if (content == view)
          {
            content.Activate();
            return true;
          }
        }
      }
      return false;
    }

    public void set_view_title(AppViewDockContent view, String str)
    {
      if (contentTabControl.IndexFromDocument(view) > 0)
        view.TabText = str;
    }

    public System.Drawing.Size get_size()
    {
      return contentTabControl.Size;
    }

    #endregion

    #region Edit Menu Handlers

    private void EditUndo()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase)
        (activeControl as TextBoxBase).Undo();
      else
        if (activeControl is ComboBox)
          Win32Api.Undo(activeControl as ComboBox);
        else
          if (activeControl is ScintillaControl)
            (activeControl as ScintillaControl).Undo();
          else
            if (wbContext.edit_can_undo())
              wbContext.edit_undo();
    }

    private bool EditCanUndo()
    {
      // If we are on a model form then edit actions are controlled by the backend.
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_undo();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox)
        return true;
      else
        if (activeControl is ScintillaControl)
          return (activeControl as ScintillaControl).CanUndo;
        else
          return wbContext.edit_can_undo();
    }

    private void EditRedo()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox)
        return;
      else
        if (activeControl is ScintillaControl)
          (activeControl as ScintillaControl).Redo();
        else
          if (wbContext.edit_can_redo())
            wbContext.edit_redo();
    }

    private bool EditCanRedo()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_redo();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox)
        return true;
      else
        if (activeControl is ScintillaControl)
          return (activeControl as ScintillaControl).CanRedo;
        else
          return wbContext.edit_can_redo();
    }

    private void EditCopy()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase)
        (activeControl as TextBoxBase).Copy();
      else
        if (activeControl is ComboBox)
          Win32Api.Copy(activeControl as ComboBox);
        else
          if (activeControl is ScintillaControl)
            (activeControl as ScintillaControl).Copy();
          else
            if (activeControl is GridView)
              (activeControl as GridView).Copy();
            else
              if (wbContext.edit_can_copy())
                wbContext.edit_copy();
    }

    private bool EditCanCopy()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_copy();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox)
        return true;
      else
        if (activeControl is ScintillaControl)
          return (activeControl as ScintillaControl).CanCopy;
        else
          if (activeControl is GridView)
            return (activeControl as GridView).CanCopy;
          else
            return wbContext.edit_can_copy();
    }

    private void EditCut()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase)
        (activeControl as TextBoxBase).Cut();
      else
        if (activeControl is ComboBox)
          Win32Api.Cut(activeControl as ComboBox);
        else
          if (activeControl is ScintillaControl)
            (activeControl as ScintillaControl).Cut();
          else
            if (wbContext.edit_can_copy()) // TODO: check really necessary?
              wbContext.edit_cut();
    }

    private bool EditCanCut()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_cut();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox)
        return false;
      else
        if (activeControl is ScintillaControl)
          return (activeControl as ScintillaControl).CanCut;
        else
          if ((activeControl is TreeViewAdv) || (activeControl != null && activeControl.Parent is TreeViewAdv))
            return false; // Returning false tells the backend not to take over the operation.
          else
            return wbContext.edit_can_cut();
    }

    private void EditPaste()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase)
        (activeControl as TextBoxBase).Paste();
      else
        if (activeControl is ComboBox)
          Win32Api.Paste(activeControl as ComboBox);
        else
          if (activeControl is ScintillaControl)
            (activeControl as ScintillaControl).Paste();
          else
            if (wbContext.edit_can_paste())
              wbContext.edit_paste();
    }

    private bool EditCanPaste()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_paste();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox || activeControl is GridView)
        return false;
      else
        if (activeControl is ScintillaControl)
          return (activeControl as ScintillaControl).CanPaste;
        else
          return wbContext.edit_can_paste();
    }

    private void EditSelectAll()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is ComboBox)
        (activeControl as ComboBox).SelectAll();
      else
        if (activeControl is TextBoxBase)
          (activeControl as TextBoxBase).SelectAll();
        else
          if (activeControl is ScintillaControl)
            (activeControl as ScintillaControl).SelectAll();
          else
            if (activeControl is DataGridView)
              (activeControl as DataGridView).SelectAll();
            else
              if (wbContext.edit_can_select_all())
                wbContext.edit_select_all();
    }

    private bool EditCanSelectAll()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_select_all();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox || activeControl is ScintillaControl ||
        activeControl is DataGridView)
        return true;
      else
        return wbContext.edit_can_select_all();
    }

    private void EditDelete()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase)
        (activeControl as TextBoxBase).SelectedText = "";
      else
        if (activeControl is ScintillaControl)
          (activeControl as ScintillaControl).Delete();
        else
          if (wbContext.edit_can_delete())
            wbContext.edit_delete();
    }

    private bool EditCanDelete()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == workbenchPhysicalOverviewForm)
        return wbContext.edit_can_delete();

      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is TextBoxBase || activeControl is ComboBox)
        return false;
      else
        if (activeControl is ScintillaControl)
          return (activeControl as ScintillaControl).CanDelete;
        else
          return wbContext.edit_can_delete();
    }

    private void EditFind()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is ScintillaControl)
        (activeControl as ScintillaControl).ShowFindPanel(false);
      else
      {
        // Not an editor control. So try focusing the search box in the toolbar (if there's any).
        IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
        if (wbDocument != null)
        {
          wbContext.focus_search_box(wbDocument.BackendForm);

          // and then search
          string searchString = wbContext.get_search_string(wbDocument.BackendForm);
          if (searchString != "")
          {
            if (!wbContext.try_searching_diagram(searchString))
            {
              // if active main tab is Overview, search it
              if (contentTabControl.ActiveDocument == workbenchPhysicalOverviewForm)
                workbenchPhysicalOverviewForm.SearchAndFocusNode(searchString);
            }
          }
        }
      }
    }

    private bool EditCanFind()
    {
      IWorkbenchDocument wbDocument = contentTabControl.ActiveDocument as IWorkbenchDocument;
      if (wbDocument == null && !EditCanFindReplace())
        return false;

      return true;
    }

    private void EditFindReplace()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is ScintillaControl)
        (activeControl as ScintillaControl).ShowFindPanel(true);
    }

    private bool EditCanFindReplace()
    {
      Control activeControl = ControlUtilities.GetLeafActiveControl(this);
      if (activeControl is ScintillaControl)
        return true;
      return false;
    }

    private void UpdateColors()
    {
      contentTabControl.RenderWithGlow = false;
      contentTabControl.UpdateColors();
      contentTabControl.BackgroundColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      mainStatusStrip.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      mainStatusStrip.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorStatusbar, true);
      statusText.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorStatusbar, true);

      foreach (ITabDocument document in contentTabControl.Documents)
      {
        if (document is IWorkbenchDocument)
          (document as IWorkbenchDocument).UpdateColors();
        else
          if (document is AppViewDockContent)
            (document as AppViewDockContent).UpdateColors();
      }
    }

    /// <summary>
    /// Some initialization done after everything else is set up and available.
    /// </summary>
    public void PostInit()
    {
      UpdateColors();

      List<String> commands = new List<string>();

      commands.Add("overview.mysql_model");
      commands.Add("diagram_size");
      commands.Add("view_model_navigator");
      commands.Add("view_catalog");
      commands.Add("view_layers");
      commands.Add("view_user_datatypes");
      commands.Add("view_object_properties");
      commands.Add("view_object_description");
      commands.Add("view_undo_history");
      commands.Add("reset_layout");
      commands.Add("wb.page_setup");
      commands.Add("help_version_check");
      commands.Add("wb.toggleSidebar");
      commands.Add("wb.toggleSecondarySidebar");
      commands.Add("wb.next_tab");
      commands.Add("wb.back_tab");
      commands.Add("closetab");
      commands.Add("close_tab");

      // SQL IDE specific commands.
      commands.Add("wb.toggleOutputArea");
      commands.Add("close_editor");

      wbContext.add_frontend_commands(commands);

      // special command in Windows implemented in wrapper to perform 
      // edit menu actions
      wbContext.set_edit_menu_delegates(
        EditUndo, EditCanUndo,
        EditRedo, EditCanRedo,
        EditCopy, EditCanCopy,
        EditCut, EditCanCut, EditPaste, EditCanPaste,
        EditSelectAll, EditCanSelectAll, EditDelete, EditCanDelete,
        EditFind, EditCanFind,
        EditFindReplace, EditCanFindReplace);
    }

    public void PerformCommand(String command)
    {
      switch (command)
      {
        case "overview.mysql_model":
          ShowPhysicalOverviewForm();
          break;
        case "diagram_size":
          ShowDiagramOptionsForm();
          break;
        case "reset_layout":
          ResetWindowLayout();
          break;
        case "wb.page_setup":
          ShowPageSettingsForm();
          break;
        case "closetab":
          CloseActiveTab();
          break;
        case "close_tab":
          contentTabControl.CloseTabPage(contentTabControl.SelectedTab);
          break;
        case "wb.next_tab":
          if (contentTabControl.SelectedIndex == contentTabControl.TabCount - 1)
            contentTabControl.SelectedIndex = 0;
          else
            contentTabControl.SelectedIndex++;
          break;
        case "wb.back_tab":
          if (contentTabControl.SelectedIndex == 0)
            contentTabControl.SelectedIndex = contentTabControl.TabCount - 1;
          else
            contentTabControl.SelectedIndex--;
          break;
        case "help_version_check":
          Program.CheckForNewVersion();
          break;
        case "wb.toggleSidebar":
        case "wb.toggleSecondarySidebar":
          ForwardCommandToActivDocument(command);
          break;
        default:
          ForwardCommandToActivDocument(command);
          break;
      }
    }

    /// <summary>
    /// Closes the active tab page in tab controls of a specific type. That is, all tabs with entities
    /// which should be closed as a whole (e.g. model + diagram documents, object editors).
    /// </summary>
    private void CloseActiveTab()
    {
      // Start at the bottom of the hierarchy and go up. This will include also tab controls
      // from child forms (e.g. model diagram, model overview, SQL IDE etc.), so we don't need to
      // forward this close call and handle it in different places.
      Control control = ControlUtilities.GetLeafActiveControl(this);
      if (control is FlatTabControl)
        control = (control as FlatTabControl).SelectedTab;

      Control host = null;
      while (host == null)
      {
        while ((control != null) && !(control is TabPage))
          control = control.Parent;

        if (control == null)
          break;

        host = control.Parent;
        if (host is FlatTabControl)
        {
          // Reset host if the tab control is not what we are looking for and start over.
          FlatTabControl.TabStyleType style = (host as FlatTabControl).TabStyle;
          if (style == FlatTabControl.TabStyleType.TopNormal || style == FlatTabControl.TabStyleType.TopTransparent)
            break;
        }
        control = host;
        host = null;
      }

      TabPage page = control as TabPage;
      if (host != null && page.Text.Length > 0) // Don't close the home screen.
        (host as FlatTabControl).CloseTabPage(control as TabPage);
    }

    private void ShowPageSettingsForm()
    {
      PageSettingsForm form = new PageSettingsForm(wbContext);

      form.ShowDialog();
      form.Dispose();
    }

    private void ShowDiagramOptionsForm()
    {
      DiagramOptionsForm form = new DiagramOptionsForm(wbContext);

      form.ShowDialog();
      form.Close();
    }

    public String ShowFileDialog(String type, String title, String extensions)
    {
      FileDialog dialog;

      if (type == "save")
      {
        dialog = new SaveFileDialog();
      }
      else if (type == "open")
      {
        dialog = new OpenFileDialog();
      }
      else
        return "";

      String[] exts = extensions.Split(new char[] { ',' });

      dialog.RestoreDirectory = true;
      dialog.Title = title;
      dialog.DefaultExt = exts[0];
      String filter = "";
      foreach (String ext in exts)
      {
        if (ext.Contains("|"))
          filter = filter + ext + "|";
        else if (ext == "mwb")
          filter = filter + String.Format("{0} (*.mwb)|*.mwb|",
            "MySQL Workbench Models");
        else if (ext == "sql")
          filter = filter + String.Format("{0} (*.sql)|*.sql|",
            "SQL Script Files");
        else
          filter = filter + String.Format("{0} files (*.{0})|*.{0}|", ext);
      }
      filter = filter + String.Format("{0} (*.*)|*.*", "All files");

      dialog.Filter = filter;

      if (dialog.ShowDialog() == DialogResult.OK)
        return dialog.FileName;

      return "";
    }

    public bool QuitApplication()
    {
      if (!shuttingDown)
        Close();

      return true;
    }

    #endregion

    #region Other Application Logic

    protected override void WndProc(ref Message m)
    {
      if (m.Msg == (int)WM.ACTIVATEAPP && m.WParam != IntPtr.Zero)
      {
        ManagedNotificationCenter.Send("GNApplicationActivated", IntPtr.Zero);
      }
      base.WndProc(ref m);
    }

    /// <summary>
    /// Try to find the given file in known locations. If name is empty return the absolute application
    /// path (as common root for all resource subfolders).
    /// </summary>
    /// <param name="name"></param>
    static public string GetIconPath(string name)
    {
      if (name == "")
        return Environment.CurrentDirectory;

      string result = "";
      if (File.Exists(name) || Directory.Exists(name))
        return name;
      foreach (String path in resourcePaths)
        if (File.Exists(path + "/" + name))
        {
          result = path + "/" + name;
          break;
        }
      return result;
    }

    public void ShowPhysicalOverviewForm(bool isNew = false)
    {
      if (workbenchPhysicalOverviewForm == null)
        workbenchPhysicalOverviewForm = new ModelOverviewForm(wbContext, wbContext.get_physical_overview());

      //SuspendLayout();
      //contentTabControl.SuspendLayout();
      DockDocument(workbenchPhysicalOverviewForm, true, true);
      Update();
      if (isNew)
        workbenchPhysicalOverviewForm.RebuildModelContents();
      //contentTabControl.ResumeLayout(true);
      //ResumeLayout();

      workbenchPhysicalOverviewForm.Activate();
    }

    public void LockGUI(bool value)
    {
      if (value)
        Cursor.Current = Cursors.WaitCursor;
      else
        Cursor.Current = null;
    }

    public void ResetWindowLayout()
    {
      // Ignore.
    }

    /// <summary>
    /// Restores the form to a usable state and activates it.
    /// </summary>
    public void Activate(bool restoreIfMinimized)
    {
      if (restoreIfMinimized && WindowState == FormWindowState.Minimized)
        WindowState = FormWindowState.Normal;
      Activate();
    }

    /// <summary>
    /// Make the upper part of the window extend the glass frame (if we are on Aero).
    /// </summary>
    /// <param name="toolbarHeight"></param>
    private void AdjustGlassFrame(int toolbarHeight)
    {
      if (ControlUtilities.IsCompositionEnabled())
      {
        int topAreaHeight = contentTabControl.ItemSize.Height + contentTabControl.Margin.Top +
          toolbarHeight + Margin.Top;

        // For Win8 glass area extension is producing weird results (white frames).
        // So we only extend the top area.
        Win32.MARGINS margins;
        if (ControlUtilities.IsWin8OrAbove())
          margins = new Win32.MARGINS(0, topAreaHeight - 3, 0, 0);
        else
          margins = new Win32.MARGINS(3, topAreaHeight, 3, 2);
        Win32.DwmExtendFrameIntoClientArea(Handle, margins);
      }
    }

    /// <summary>
    /// Prepares the given document to conform to the overall layout. This requires to draw an
    /// intermediate set of controls (panels) and add menu + toolbar.
    /// </summary>
    /// <param name="document">The document to adjust, which comes with the actual content.</param>
    /// <param name="backend">The backend UI form used to get toolbar and menu.</param>
    /// <param name="withPadding">Indicates if we need padding around the content or not.</param>
    private void SetupDockLayout(ITabDocument document, UIForm backend, bool withPadding)
    {
      // Setting up the layout requires some additional nesting of controls.
      // A tab document should have only one container as top level control, which we re-host.
      Control root = document.Content;
      Control content = (root.Controls.Count > 0) ? root.Controls[0] : null;

      // If there's already a drawable panel then this document has already been set up
      // and just needs to be shown. Happens for re-used views (like the Output view).
      if (content is DrawablePanel)
        return;

      root.Controls.Clear();

      // Content area.
      DrawablePanel contentPanel = new DrawablePanel();
      contentPanel.BackColor = MySQL.Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      contentPanel.CustomBackground = false;
      contentPanel.Dock = DockStyle.Fill;
      contentPanel.Padding = withPadding ? new Padding(6) : new Padding(0);
      root.Controls.Add(contentPanel);
      if (content != null)
      {
        contentPanel.Controls.Add(content);
        content.Dock = DockStyle.Fill;
      }

      // Menu + toolbar area. We have two types of tab documents:
      // - appview based (and hence backend-managed)
      // - native Form (with interface via WBContext).
      // Appview based documents have own methods to return their menu and toolbar because
      // retrieval via managed code requires a managed UIForm. However appview classes are
      // already managed via mforms and adding another reference management via UIForm
      // will lead to unpredictable results.
      ToolStrip toolbar;
      MenuStrip menu;
      if (backend != null)
      {
        // Retrieval via WBContext will return a default toolbar if no other is defined.
        toolbar = wbContext.toolbar_for_form(backend);
        menu = wbContext.menu_for_form(backend);
      }
      else
      {
        AppViewDockContent dockContent = document as AppViewDockContent;
        toolbar = dockContent.GetToolBar();
        if (toolbar == null)
          toolbar = wbContext.toolbar_for_form(null); // Currently never returns a toolbar.
        menu = wbContext.menu_for_appview(dockContent);
      }
      DrawablePanel menuPanel = new DrawablePanel();
      if (toolbar != null || menu != null)
      {
        menuPanel.BackColor = Color.Transparent;
        menuPanel.CustomBackground = true;
        menuPanel.Dock = DockStyle.Top;
        menuPanel.AutoSize = true;
        root.Controls.Add(menuPanel);
        if (toolbar != null)
        {
          toolbar.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainTab, false);

          menuPanel.Controls.Add(toolbar);
          toolbar.Dock = DockStyle.Top;
        }

        if (menu != null)
        {
          menu.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainTab, false);

          menuPanel.Controls.Add(menu);
          menu.Dock = DockStyle.Top;
          menu.AutoSize = true;
        }
      }
    }

    #endregion

    #region Document Handling

    private void tabControl_ContentAdded(object sender, ControlEventArgs e)
    {
      if (e.Control is TabPage)
      {
        ITabDocument document = (sender as FlatTabControl).DocumentFromPage(e.Control as TabPage);
        if (document is ModelDiagramForm)
        {
          ModelDiagramForm form = document as ModelDiagramForm;
          ((ModelDiagramFormWrapper)form.BackendForm).set_closed(false);
        }
      }
    }

    private void tabControl_ContentRemoved(object sender, ControlEventArgs e)
    {
      if (e.Control is TabPage)
      {
        ITabDocument document = (sender as FlatTabControl).DocumentFromPage(e.Control as TabPage);
        if (document is ModelDiagramForm)
        {
          ModelDiagramForm form = document as ModelDiagramForm;
          ((ModelDiagramFormWrapper)form.BackendForm).set_closed(true);
        }
      }
    }

    private void SetEnableForMenuItem(ToolStripItem item, bool enabled)
    {
      item.Enabled = enabled;
      if (item is ToolStripMenuItem)
      {
        foreach (ToolStripItem subitem in (item as ToolStripMenuItem).DropDownItems)
          SetEnableForMenuItem(subitem, enabled);
      }
    }

    private void tabControl_SelectedIndexChanged(object sender, EventArgs e)
    {
      // Reset status bar to clean up any previously done action message.
      // Hide the statusbar on the home page, though.
      StatusBarText = "Ready";

      // The home screen tab has no text since we only use an icon there.
      // So we can use this as quick check here.
      mainStatusStrip.Visible = contentTabControl.SelectedIndex > -1 &&
        (contentTabControl.SelectedTab.Text.Length > 0);

      // Disable embedded menus for all inactive pages.
      ITabDocument[] documents = contentTabControl.DocumentsToArray();
      for (int i = 0; i < documents.Length; i++)
      {
        AppViewDockContent content = documents[i] as AppViewDockContent;
        if (content != null)
        {
          MenuStrip menu = content.GetMenuBar();
          if (menu != null)
          {
            menu.Enabled = (contentTabControl.SelectedIndex == i);
            foreach (ToolStripItem item in menu.Items)
              SetEnableForMenuItem(item, menu.Enabled);
          }
        }
        else
        {
          IWorkbenchDocument wbDocument = documents[i] as IWorkbenchDocument;
          if (wbDocument != null)
          {
            MenuStrip menu = wbContext.menu_for_form(wbDocument.BackendForm);
            if (menu != null)
            {
              menu.Enabled = (contentTabControl.SelectedIndex == i);
              foreach (ToolStripItem item in menu.Items)
                SetEnableForMenuItem(item, menu.Enabled);
            }
          }
        }
      }

      // For the glass effect behind the tabs (and the proper content drawing) we need the
      // height of the tabs as well as menu and toolbar of the active tab.
      int topAreaHeight = 0;

      // Suspend dock layout to prevent flicker.
      SuspendLayout();
      contentTabControl.SuspendLayout();
      try
      {
        FlatTabControl tabControl = sender as FlatTabControl;
        if (tabControl.ActiveDocument != null)
        {
          ITabDocument activeDocument = tabControl.ActiveDocument;
          activeDocument.Activate();
          topAreaHeight += activeDocument.ToolbarHeight;

          if (activeDocument is IWorkbenchDocument)
          {
            IWorkbenchDocument wbDoc = activeDocument as IWorkbenchDocument;

            // If the current View is changed, update ModelCatalogForm
            if (activeDocument is ModelDiagramForm)
            {
              // First focus the canvas to make it the active control before the menu items
              // are revalidated (on set_active_form).
              ModelDiagramForm form = activeDocument as ModelDiagramForm;
              form.FocusCanvasControl();
              wbContext.set_active_form(form.BackendForm);
            }
            else
              wbContext.set_active_form(wbDoc.BackendForm);
          }
          else if (activeDocument is Plugins.DockablePlugin)
          {
            Plugins.DockablePlugin form = activeDocument as Plugins.DockablePlugin;
            wbContext.set_active_form(form.BackendForm);
          }
          else if (activeDocument is UIForm)
          {
            wbContext.set_active_form(activeDocument as UIForm);
          }
          else if (activeDocument is MySQL.Forms.AppViewDockContent)
          {
            MySQL.Forms.AppViewDockContent form = activeDocument as MySQL.Forms.AppViewDockContent;

            wbContext.set_active_form_from_appview(form);
          }
          else
            wbContext.set_active_form(null);
        }
        else
          wbContext.set_active_form(null);
      }
      finally
      {
        // Resume suspended dock layout.
        contentTabControl.ResumeLayout(true);
        ResumeLayout();
      }

      // Different pages might have a different height of the menu/toolbar area.
      AdjustGlassFrame(topAreaHeight);
    }

    private void ForwardRefreshToActivDocument(RefreshType refresh, String str, IntPtr ptr)
    {
      if (contentTabControl.ActiveDocument is IWorkbenchDocument)
        (contentTabControl.ActiveDocument as IWorkbenchDocument).RefreshGUI(refresh, str, ptr);
    }

    private void ForwardRefreshToAllDocuments(RefreshType refresh, String str, IntPtr ptr)
    {
      foreach (ITabDocument document in contentTabControl.Documents)
        if (document is IWorkbenchDocument)
          (document as IWorkbenchDocument).RefreshGUI(refresh, str, ptr);
    }

    private void ForwardCommandToActivDocument(String command)
    {
      if (contentTabControl.ActiveDocument is IWorkbenchDocument)
        (contentTabControl.ActiveDocument as IWorkbenchDocument).PerformCommand(command);
    }

    private void ForwardCommandToAllDocuments(String command)
    {
      foreach (ITabDocument document in contentTabControl.Documents)
        if (document is IWorkbenchDocument)
          (document as IWorkbenchDocument).PerformCommand(command);
    }

    /// <summary>
    /// Docks the given document to either the own tab control (as main document) or as
    /// editor window to the currently active main document.
    /// </summary>
    /// <param name="document">The document to dock.</param>
    /// <param name="main">If true then dock that document to our own tab control, otherwise
    /// dock it to the currently active main document.</param>
    public void DockDocument(ITabDocument document, bool main, bool activate)
    {
      if (main)
      {
        int index = -1;
        if (!contentTabControl.HasDocument(document))
        {
          IWorkbenchDocument wbDocument = document as IWorkbenchDocument;
          SetupDockLayout(document, (wbDocument != null) ? wbDocument.BackendForm : null, contentTabControl.TabCount > 0);
          index = contentTabControl.AddDocument(document);
        }

        if (index == 0) // Replace the title by an icon.
        {
          contentTabControl.TabPages[index].ImageIndex = 0;
          contentTabControl.SetCloseButtonVisibility(0, FlatTabControl.CloseButtonVisiblity.HideButton);
        }

        if (activate)
        {
          document.Activate();
          AdjustGlassFrame(document.ToolbarHeight);
        }
      }
      else
      {
        ITabDocument host = contentTabControl.ActiveDocument;
        if (host is ModelDiagramForm)
          (host as ModelDiagramForm).DockDocument(document, activate);
        else
          if (host is ModelOverviewForm)
            (host as ModelOverviewForm).DockDocument(document, activate);
          else
            if (host is SqlIdeForm)
              (host as SqlIdeForm).DockDocument(document, activate);
      }

      // Do the color updates after everything set up as this depends on the structure.
      if (document is IWorkbenchDocument)
        (document as IWorkbenchDocument).UpdateColors();
      else
        if (document is AppViewDockContent)
          (document as AppViewDockContent).UpdateColors();

    }

    /// <summary>
    /// Undocks the given document from our tab control if it is there.
    /// If not forward the call to the currently active document.
    /// </summary>
    /// <param name="document"></param>
    public void UndockDocument(ITabDocument document)
    {
      if (contentTabControl.HasDocument(document))
        contentTabControl.RemoveDocument(document);
      else
      {
        ITabDocument host = contentTabControl.ActiveDocument;
        if (host is ModelDiagramForm)
          (host as ModelDiagramForm).UndockDocument(document);
        else
          if (host is ModelOverviewForm)
            (host as ModelOverviewForm).UndockDocument(document);
          else
            if (host is SqlIdeForm)
              (host as SqlIdeForm).UndockDocument(document);
      }
    }

    public int view_count()
    {
      return contentTabControl.TabCount;
    }

    public AppViewDockContent view_at_index(int i)
    {
      return contentTabControl.TabPages[i].Tag as AppViewDockContent;
    }

    public AppViewDockContent selected_view()
    {
      return contentTabControl.SelectedTab.Tag as AppViewDockContent;
    }

    #endregion

    #region UI Event Handling

    private void spacerPanel_Paint(object sender, PaintEventArgs e)
    {
      Graphics g = e.Graphics;

      g.DrawLine(Pens.White, e.ClipRectangle.Left, 1, e.ClipRectangle.Right - 1, 1);
    }

    private void MainForm_KeyDown(object sender, KeyEventArgs e)
    {
      //wbContext.handle_key_event(true, e);
    }

    private void MainForm_FormClosing(object sender, FormClosingEventArgs e)
    {
      // Temporarily remove the focus from what ever control has it currently.
      // This way controls that save content on leave get their chance.
      Focus();

      shuttingDown = true;

      // Query all documents if they can close in advance, so they stay open if one says nay.
      // For historical reasons diagram and model overview pages are on the same tab level
      // (the main one) even though diagrams are subordinated to model tabs. This means we have
      // to do two rounds to ensure diagram tabs are checked before the model tabs are.
      foreach (ITabDocument document in contentTabControl.Documents)
      {
        if (document is ModelDiagramForm)
        {
          e.Cancel = !(document as IWorkbenchDocument).CanCloseDocument();

          // Make the page active that refused to close.
          if (e.Cancel)
          {
            document.Activate();
            break;
          }
        }
      }

      if (!e.Cancel)
      {
        foreach (ITabDocument document in contentTabControl.Documents)
        {
          if (document is ModelDiagramForm)
            continue;

          if (document is IWorkbenchDocument)
            e.Cancel = !(document as IWorkbenchDocument).CanCloseDocument();
          else
            if (document is MySQL.Forms.AppViewDockContent)
            {
              MySQL.Forms.AppViewDockContent content = document as MySQL.Forms.AppViewDockContent;
              e.Cancel = !content.CanCloseDocument();
            }

          if (e.Cancel)
          {
            // Make the page active that refused to close.
            document.Activate();
            break;
          }
        }
      }

      if (!e.Cancel)
        e.Cancel = !wbContext.request_quit();

      if (e.Cancel)
      {
        // Reset termination flag, in case it was set.
        grtManager.resetTermination();
        shuttingDown = false;
        return;
      }

      // Restore previous clipboard chain.
      SaveFormState();

      Hide();

      // Go through each workbench document and close it, now unconditionally.
      contentTabControl.SuspendLayout();
      try
      {
        ITabDocument[] documents = contentTabControl.DocumentsToArray();

        // Close all documents (editors implement the document interface too, so we get them
        // with that loop as well).
        for (int i = documents.Length - 1; i >= 0; i--)
        {
          if (documents[i] is MySQL.Forms.AppViewDockContent)
          {
            MySQL.Forms.AppViewDockContent content = documents[i] as MySQL.Forms.AppViewDockContent;
            content.CloseDocument();
          }
          else
            documents[i].Close();
        }
        workbenchPhysicalOverviewForm = null;
      }
      finally
      {
        contentTabControl.ResumeLayout();
      }

      wbContext.perform_quit();
    }

    private void LoadFormState()
    {
      int x = wbContext.read_state("left", "mainform", 100);
      int y = wbContext.read_state("top", "mainform", 100);
      int w = wbContext.read_state("width", "mainform", 1200);
      int h = wbContext.read_state("height", "mainform", 900);

      // Sanity checks to avoid restoring the application into a state where it cannot be reached.
      if (x + w < 100)
        x = 100 - w;
      if (y + h < 100)
        y = 100 - h;
      Rectangle workingArea = Screen.GetWorkingArea(new Point(x, y));
      if (x > workingArea.Right - 100)
        x = workingArea.Right - 100;
      if (y > workingArea.Bottom - 100)
        y = workingArea.Bottom - 100;

      // First set the bounds then the state. If the window is maximized this will properly set
      // RestoreBounds so un-maximizing will later do the right job.
      Bounds = new Rectangle(x, y, w, h);
      WindowState = (FormWindowState)wbContext.read_state("windowstate", "mainform", (int)FormWindowState.Normal);

      // Don't restore into minimized state. Confusing for the user and makes trouble for layouting.
      if (WindowState == FormWindowState.Minimized)
        WindowState = FormWindowState.Normal;
    }

    private void SaveFormState()
    {
      wbContext.save_state("windowstate", "mainform", (int)WindowState);

      if (WindowState == FormWindowState.Normal)
      {
        wbContext.save_state("left", "mainform", Left);
        wbContext.save_state("top", "mainform", Top);
        wbContext.save_state("width", "mainform", Width);
        wbContext.save_state("height", "mainform", Height);
      }
      else
      {
        wbContext.save_state("left", "mainform", RestoreBounds.Left);
        wbContext.save_state("top", "mainform", RestoreBounds.Top);
        wbContext.save_state("width", "mainform", RestoreBounds.Width);
        wbContext.save_state("height", "mainform", RestoreBounds.Height);
      }

      bool outputVisible = false;
      foreach (ITabDocument content in contentTabControl.Documents)
      {
        if (content.TabText == "Output")
        {
          outputVisible = true;
          break;
        }
      }
      wbContext.save_state("outputvisible", "mainform", outputVisible ? 1 : 0);
    }

    private void MainForm_Load(object sender, EventArgs e)
    {
      // Switch to high contrast mode if that is active on the system but not set in our settings.
      if (!Conversions.InHighContrastMode() && SystemInformation.HighContrast)
        Conversions.SetColorScheme(ColorScheme.ColorSchemeHighContrast);

      LoadFormState();

      BringToFront();
      Activate();
    }


    void MainForm_Activated(object sender, System.EventArgs e)
    {
      wbContext.mainform_activated();
    }

    void MainForm_Deactivated(object sender, System.EventArgs e)
    {
      wbContext.mainform_deactivated();
    }

    public void UpdateTimer()
    {
      int interval = (int)(1000 * wbContext.delay_for_next_timer());
      if (interval > 0)
      {
        timer.Interval = interval;
        timer.Start();
      }
      else if (interval < 0)
        timer.Stop();
      else
      {
        timer.Interval = 1;
        timer.Start();
      }
    }

    private void timer_Tick(object sender, EventArgs e)
    {
      wbContext.flush_timers();
      UpdateTimer();
    }

    private void tabControl_TabClosing(object sender, TabClosingEventArgs e)
    {
      e.canClose = true;

      ITabDocument document = (sender as FlatTabControl).DocumentFromPage(e.page);
      if (document == null)
        return;

      if (document is IWorkbenchDocument)
        e.canClose = (document as IWorkbenchDocument).CanCloseDocument();
      else
        if (document is MySQL.Forms.AppViewDockContent)
        {
          MySQL.Forms.AppViewDockContent content = document as MySQL.Forms.AppViewDockContent;
          e.canClose = content.CanCloseDocument();
        }
    }

    private void contentTabControl_TabClosed(object sender, TabClosedEventArgs e)
    {
      ITabDocument document = (sender as FlatTabControl).DocumentFromPage(e.page);
      if (document is IWorkbenchDocument)
        (document as IWorkbenchDocument).CloseDocument();
      else
        if (document is MySQL.Forms.AppViewDockContent)
        {
          // Remove the appview from the page, otherwise it gets disposed with the page,
          // what we don't want.
          //e.page.Controls.Clear();
          MySQL.Forms.AppViewDockContent content = document as MySQL.Forms.AppViewDockContent;
          content.CloseDocument();
        }
    }

    private void tabsContextMenuItemClick(object sender, EventArgs e)
    {
      ToolStripMenuItem item = sender as ToolStripMenuItem;
      int clickedTab = (int)item.Owner.Tag;

      if (contentTabControl.TabCount < clickedTab || clickedTab < 0)
        return;
      TabPage page = contentTabControl.TabPages[clickedTab];
      if (page == null)
        return;

      switch (item.Tag as string)
      {
        case "0": // Close page.
          contentTabControl.CloseTabPage(page);
          break;
        case "1": // Close all editors but the clicked one.
          TabControl.TabPageCollection pages = contentTabControl.TabPages;
          for (int i = 1; i < pages.Count; i++)
          {
            if (pages[i] == page)
              continue;
            contentTabControl.CloseTabPage(pages[i]);
            i = 0;
          }
          contentTabControl.SelectedIndex = contentTabControl.TabPages.Count - 1;
          break;
        case "2": // Close tabs of the same type.
          ITabDocument document = contentTabControl.DocumentFromIndex(clickedTab);
          Type classToClose = document.GetType();
          for (int i = contentTabControl.TabCount - 1; i >= 0; i--)
          {
            if (contentTabControl.DocumentFromIndex(i).GetType() == classToClose)
              contentTabControl.CloseTabPage(contentTabControl.TabPages[i]);
          }
          break;
      }
    }

    private void contentTabControl_MouseClick(object sender, MouseEventArgs e)
    {
      switch (e.Button)
      {
        case MouseButtons.Right:
          {
            int clickedIndex = contentTabControl.TabIndexFromPosition(e.Location);
            if (clickedIndex < 0)
              return;

            closeTabMenuItem.Enabled = clickedIndex > 0;
            closeOtherTabsMenuItem.Enabled = contentTabControl.TabCount > 1;

            ITabDocument document = contentTabControl.DocumentFromIndex(clickedIndex);
            closeTabsOfSameTypeItem.Enabled = document is SqlIdeForm || document is ModelDiagramForm
              || document is ModelOverviewForm;

            // Keep the found index for later handling in the item click handler.
            tabsContextMenuStrip.Tag = clickedIndex;

            Point p = contentTabControl.PointToScreen(e.Location);
            tabsContextMenuStrip.Show(p);
          }
          break;
      }
    }

    private void mainStatusStrip_Paint(object sender, PaintEventArgs e)
    {
      using (SolidBrush brush = new SolidBrush(mainStatusStrip.BackColor))
        e.Graphics.FillRectangle(brush, e.ClipRectangle);
    }

    #endregion

    public class MainPageDockDelegate : ManagedDockDelegate
    {
      private MainForm _owner = null;

      public MainPageDockDelegate(MainForm owner, Object representedObject)
        : base(representedObject)
      {
        _owner = owner;
      }

      public override String get_type(Object representedObject)
      {
        return "MainWindow";
      }

      public override void dock_view(Object representedObject, AppViewDockContent view, String arg1, int arg2)
      {
        _owner.DockDocument(view, true, true);
      }

      public override void undock_view(Object representedObject, AppViewDockContent view)
      {
        _owner.UndockDocument(view);
      }

      public override bool select_view(Object representedObject, AppViewDockContent view)
      {
        return _owner.select_view(view);
      }

      public override void set_view_title(Object representedObject, AppViewDockContent view, String str)
      {
        _owner.set_view_title(view, str);
      }

      public override System.Drawing.Size get_size(Object representedObject)
      {
        return _owner.get_size();
      }

      public override AppViewDockContent selected_view()
      {
        return _owner.selected_view();
      }

      public override int view_count()
      {
        return _owner.view_count();
      }

      public override AppViewDockContent view_at_index(int i)
      {
        return _owner.view_at_index(i);
      }
    };

  }
}
