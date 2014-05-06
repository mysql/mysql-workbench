/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;

using MySQL.Controls;
using MySQL.Forms;
using MySQL.Base;
using MySQL.Grt.Db;
using MySQL.GUI.Workbench.Plugins;
using MySQL.Utilities;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class SqlIdeForm : Plugins.DockablePlugin, IWorkbenchObserver
  {
    private List<RecordsetView> pendingRelayouts = new List<RecordsetView>();

    private SqlEditorFormWrapper dbSqlEditorBE;
    public SqlEditorFormWrapper Backend { get { return dbSqlEditorBE; } }

    private SqlEditorWrapper runningEditor;

    protected WbContext wbContext;

    private bool canTrackChanges = false; // True when initialization is done and UI changes can be tracked.

    private bool outputVisibilySetInAdmin = false;
    private bool outputIsVisible = true;
    private bool restoreOutputArea = false;
    private bool secondarySidebarVisibilySetInAdmin = false;
    private bool secondarySidebarIsVisible = true;
    private bool restoreSeconarySidebar = false;

    #region Initialization

    public SqlIdeForm(WbContext wbContext, UIForm uiForm)
      : base((uiForm as SqlEditorFormWrapper).grt_manager())
    {
      this.wbContext = wbContext;
      dbSqlEditorBE = uiForm as SqlEditorFormWrapper;
      Initialize();
      UpdateColors();
    }

    protected void Initialize()
    {
      Logger.LogInfo("WQE.net", "Launching SQL IDE\n");

      InitializeComponent();

      Logger.LogDebug("WQE.net", 1, "Setup of generated UI done\n");
      outputSelector.SelectedIndex = 0;

      TabText = dbSqlEditorBE.get_title();

      dbSqlEditorBE.sql_editor_new_ui_cb(OnSqlEditorNew);

      Logger.LogDebug("WQE.net", 2, "Creating Log View\n");
      logView = new GridView(dbSqlEditorBE.log());
      logView.AutoScroll = true;
      logView.RowHeadersVisible = false;
      logView.Parent = actionPanel;
      logView.AllowAutoResizeColumns = false;

      // Some visual setup of the header.
      logView.ColumnHeadersBorderStyle = DataGridViewHeaderBorderStyle.Single;
      logView.ColumnHeadersDefaultCellStyle.Font = ControlUtilities.GetFont("Segoe UI", 7.5f);
      logView.ColumnHeadersHeightSizeMode = DataGridViewColumnHeadersHeightSizeMode.AutoSize;

      dbSqlEditorBE.log().refresh_ui_cb(logView.ProcessModelRowsChange);
      logView.ProcessModelChange();
      logView.Columns[0].DefaultCellStyle.Alignment = DataGridViewContentAlignment.MiddleCenter;
      logView.Columns[0].AutoSizeMode = DataGridViewAutoSizeColumnMode.DisplayedCells;
      logView.Columns[1].AutoSizeMode = DataGridViewAutoSizeColumnMode.DisplayedCells;
      logView.Columns[2].AutoSizeMode = DataGridViewAutoSizeColumnMode.DisplayedCells;
      logView.Columns[3].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
      logView.Columns[4].AutoSizeMode = DataGridViewAutoSizeColumnMode.Fill;
      logView.Columns[5].AutoSizeMode = DataGridViewAutoSizeColumnMode.DisplayedCells;
      logView.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
      logView.ForeColor = Color.Black;
      logView.CellContextMenuStripNeeded += new DataGridViewCellContextMenuStripNeededEventHandler(logView_CellContextMenuStripNeeded);

      Logger.LogDebug("WQE.net", 1, "Creating History View\n");
      historyEntriesView = new GridView(dbSqlEditorBE.history().entries_model());
      historyEntriesView.AutoScroll = true;
      historyEntriesView.ForeColor = Color.Black;
      historyEntriesView.MultiSelect = false;
      historyEntriesView.RowHeadersVisible = false;
      historyEntriesView.Parent = historySplitContainer.Panel1;
      dbSqlEditorBE.history().entries_model().refresh_ui_cb(ProcessModelHistoryEntryRowsChange);
      historyEntriesView.ProcessModelChange();
      historyEntriesView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
      historyEntriesView.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
      historyEntriesView.RowEnter += new DataGridViewCellEventHandler(historyEntriesView_RowEnter);
      historyEntriesView.CellContextMenuStripNeeded += historyEntriesView_CellContextMenuStripNeeded;
      historyEntriesView.SetRowSelected(0);
      {
        Logger.LogDebug("WQE.net", 1, "Setting up menu for History View\n");

        // TODO: replace by mforms backed menu. Then remove SqlIdeMenuManager.
        // See if WorkbenchToolbarManager.cs can go entirely when done.
        SqlIdeMenuManager.MenuContext popupMenuContext = new SqlIdeMenuManager.MenuContext();
        popupMenuContext.GetNodesMenuItems = historyEntriesView.Model.get_popup_items_for_nodes;
        popupMenuContext.GetSelectedNodes = historyEntriesView.SelectedNodes;
        popupMenuContext.TriggerNodesAction = historyEntriesView.Model.activate_popup_item_for_nodes;
        SqlIdeMenuManager.InitMenu(historyListMenuStrip, popupMenuContext);
      }

      Logger.LogDebug("WQE.net", 1, "Setting up History Details View\n");
      historyDetailsView = new GridView(dbSqlEditorBE.history().details_model());
      historyDetailsView.AutoScroll = true;
      historyDetailsView.ForeColor = Color.Black;
      historyDetailsView.RowHeadersVisible = false;
      historyDetailsView.Parent = historySplitContainer.Panel2;
      historyDetailsView.CellContextMenuStripNeeded += new DataGridViewCellContextMenuStripNeededEventHandler(historyDetailsView_CellContextMenuStripNeeded);
      dbSqlEditorBE.history().details_model().refresh_ui_cb(historyDetailsView.ProcessModelRowsChange);
      historyDetailsView.ProcessModelChange();
      historyDetailsView.AutoSizeColumnsMode = DataGridViewAutoSizeColumnsMode.Fill;
      historyDetailsView.Columns[0].AutoSizeMode = DataGridViewAutoSizeColumnMode.DisplayedCells;
      historyDetailsView.SelectionMode = DataGridViewSelectionMode.FullRowSelect;
      historyDetailsView.CellDoubleClick += new DataGridViewCellEventHandler(historyDetailsView_CellDoubleClick);

      Logger.LogDebug("WQE.net", 1, "Setting callbacks to backend\n");
      dbSqlEditorBE.exec_sql_task.finish_cb(AfterExecSql);
      dbSqlEditorBE.exec_sql_task.progress_cb(OnExecSqlProgress);
      dbSqlEditorBE.recordset_list_changed_cb(RecordsetListChanged);
      dbSqlEditorBE.output_text_ui_cb(RecordsetTextOutput);
      resultSetTextBox.Font = GrtManager.get_font_option("workbench.general.Editor:Font");

      dbSqlEditorBE.refresh_ui().set_partial_refresh_slot(OnPartialRefresh);

      Logger.LogDebug("WQE.net", 1, "Setting up side bar\n");
      Control sidebar = dbSqlEditorBE.get_sidebar_control();
      sideArea.Controls.Add(sidebar);
      sidebar.Dock = DockStyle.Fill;

      // Help/snippets palette.
      Logger.LogDebug("WQE.net", 1, "Setting up secondary sidebar\n");
      Control palette = dbSqlEditorBE.get_palette_control();
      rightSplitContainer.Panel1.Controls.Add(palette);
      palette.Dock = DockStyle.Fill;
      palette.Show();

      UpdateColors();

      dbSqlEditorBE.set_docking_delegate(new MainPaneDockDelegate(this));

      ManagedNotificationCenter.AddObserver(this, "GNFormTitleDidChange");

      Logger.LogDebug("WQE.net", 1, "Initialization done\n");
    }

    #endregion

    #region Finalization

    private void Destroy()
    {
      Logger.LogInfo("WQE.net", "Shutting down SQL editor (" + TabText + ")\n");

      ManagedNotificationCenter.RemoveObserver(this, "GNFormTitleDidChange");

      // Make a copy of the keys first as the dictionary is modified while closing the recordsets.
      long[] keys = new long[recordset2placeholder.Count];
      recordset2placeholder.Keys.CopyTo(keys, 0);
      foreach (long key in keys)
        CloseRecordset(key);

      // Close all attached editor tabs.
      foreach (TabPage page in mainContentTabControl.TabPages)
      {
        int editorIndex = EditorIndexFromTabPage(page);
        if (editorIndex >= 0)
        {
          // Close also all docked AppViews from the result tab control
          // (result sets are closed already above).
          SqlEditorWrapper editor = sqlEditors[editorIndex];
          FlatTabControl resultTabControl = GetResultTabControlForEditor(editor);
          if (resultTabControl != null)
          {
            foreach (TabPage resultTabPage in resultTabControl.TabPages)
            {
              if (resultTabPage.Tag is AppViewDockContent)
                RemoveViewFromResultTabview(editor, resultTabPage.Tag as AppViewDockContent);
            }
          }

          // Free only our managed resources attached to that editor but leave all
          // the backend structures intact (and hence our maps too), so the backend
          // can properly process the editors on close (e.g. auto save).
          // The backend resources will then be freed by the backend.
          editor.Dispose();
          continue;
        }

        // AppView still does not support can_close/close duality.
        if (page.Tag is AppViewDockContent)
          (page.Tag as AppViewDockContent).DocumentClosing();
        else
          closeEditorTab(page); // Non-editor tabs.
      }

      historyDetailsView.Dispose();
      historyEntriesView.Dispose();

      dbSqlEditorBE.Dispose();
    }

    #endregion

    #region Refresh Messages

    protected void OnPartialRefresh(int what)
    {
      SqlEditorFormWrapper.PartialRefreshType refresh_type = (SqlEditorFormWrapper.PartialRefreshType)what;
      switch (refresh_type)
      {
        case SqlEditorFormWrapper.PartialRefreshType.RefreshEditorTitle:
          Logger.LogDebug("WQE.net", 1, "Handling partial refresh RefreshEditorTitle\n");
          OnSqlEditorTitleChange();
          break;
        
        case SqlEditorFormWrapper.PartialRefreshType.RefreshRecordsetTitle:
          Logger.LogDebug("WQE.net", 1, "Handling partial refresh RefreshRecordsetTitle\n");
          OnRecordsetCaptionChange();
          break;

        case SqlEditorFormWrapper.PartialRefreshType.QueryExecutionStarted:
          Logger.LogDebug("WQE.net", 1, "Handling partial refresh QueryExecutionStart\n");

          // This handling works properly only for single query execution since we don't get an
          // indicator which editor is actually involved (the backend can actually start multiple
          // editor actions at the same time and in random order).
          // TODO: find a better way to report start and stop of editor actions in the mid term.
          runningEditor = ActiveEditor;
          if (runningEditor != null)
            mainContentTabControl.SetBusy(TabIndexFromEditor(runningEditor), true);
          break;

        default:
          Logger.LogDebug("WQE.net", 1, "Unhandled partial refresh message\n");
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
      if (name == "GNFormTitleDidChange" && dbSqlEditorBE.form_id() == info["form"])
        TabText = dbSqlEditorBE.get_title();
    }

    public override void PerformCommand(string command)
    {
      Logger.LogDebug("WQE.net", 1, "performing command: + " + command + "\n");
      switch (command)
      {
        case "wb.toggleSidebar":
          mainSplitContainer.Panel1Collapsed = !mainSplitContainer.Panel1Collapsed;
          wbContext.save_state("sidebar_visible", "query_editor", !mainSplitContainer.Panel1Collapsed);
          break;
        case "wb.toggleOutputArea":
          outputIsVisible = !outputIsVisible;
          if (IsAdminActive())
            outputVisibilySetInAdmin = true;
          contentSplitContainer.Panel2Collapsed = !contentSplitContainer.Panel2Collapsed;
          wbContext.save_state("output_visible", "query_editor", !contentSplitContainer.Panel2Collapsed);
          break;
        case "wb.toggleSecondarySidebar":
          secondarySidebarIsVisible = !secondarySidebarIsVisible;
          if (IsAdminActive())
            secondarySidebarVisibilySetInAdmin = true;
          mainContentSplitContainer.Panel2Collapsed = !mainContentSplitContainer.Panel2Collapsed;
          wbContext.save_state("support_sidebar_visible", "query_editor", !mainContentSplitContainer.Panel2Collapsed);
          break;
        case "close_editor":
          mainContentTabControl.CloseTabPage(mainContentTabControl.SelectedTab);
          break;
      }
    }

    private List<String> commands = new List<string>();

    #endregion

    #region SQL Script Execution

    private int OnExecSqlProgress(float progress, String msg)
    {
      logView.ProcessModelRowsChange();
      return 0;
    }

    private int AfterExecSql()
    {
      Logger.LogDebug("WQE.net", 1, "Post processing sql execution\n");

      // For normal sql execution we have an editor (which is not necessarily the active editor).
      // If there is no running editor then we probably have an EXPLAIN call that just finished
      // which is always for the active editor.
      SqlEditorWrapper editor = runningEditor;
      if (editor == null)
        editor = ActiveEditor;
      if (editor == null)
        return 0;

      runningEditor = null; // Just like noted when setting this member. Handling needs improvement.

      int tabIndex = TabIndexFromEditor(editor);
      int editorIndex = EditorIndexFromTabIndex(tabIndex);

      if (dbSqlEditorBE.exec_sql_error_count() > 0)
      {
        outputSelector.SelectedIndex = 0;
        dbSqlEditorBE.show_output_area();
      }

      UpdateApplyCancel();

      logView.ProcessModelRowsChange();
      logView.AutoResizeColumn(1);
      logView.AutoResizeColumn(2);
      logView.AutoResizeColumn(5);

      if (logView.Rows.Count > 0)
        logView.SetRowSelected(logView.Rows.Count - 1);

      mainContentTabControl.SelectedIndex = tabIndex;
      mainContentTabControl.SetBusy(tabIndex, false);

      ActiveControl = editor.get_editor_control(); // Refocus the editor control.

      return 0;
    }

    void onSizeChanged(object sender, EventArgs e)
    {
      foreach (RecordsetView view in pendingRelayouts)
      {
        // Workaround for data grids added while the form was minimized.
        // They refuse to compute their own layout until a new resize event occurs.
        view.GridView.Dock = DockStyle.None;
        view.GridView.Dock = DockStyle.Fill;
      }
      pendingRelayouts.Clear();
    }
    #endregion

    #region SQL Editors

    private List<SqlEditorWrapper> sqlEditors = new List<SqlEditorWrapper>();
    private Dictionary<SqlEditorWrapper, TabPage> sqlEditor2page = new Dictionary<SqlEditorWrapper, TabPage>();
    private Dictionary<TabPage, SqlEditorWrapper> page2SqlEditor = new Dictionary<TabPage, SqlEditorWrapper>();

    private SqlEditorWrapper ActiveEditor
    {
      get
      {
        int index = dbSqlEditorBE.active_sql_editor_index();
        if (index < 0 || index > sqlEditors.Count - 1)
          return null;
        return sqlEditors[index];
      }
    }

    /// <summary>
    /// Returns the index of the tab page that hosts the given editor.
    /// There is no 1:1 relationship between the sql editor collections and the tab collection,
    /// because we can have other types of forms docked (e.g. object editors).
    /// </summary>
    private int TabIndexFromEditor(SqlEditorWrapper editor)
    {
      TabPage page = sqlEditor2page[editor];
      if (page == null)
        return -1;
      return mainContentTabControl.TabPages.IndexOf(page);
    }

    /// <summary>
    /// The reverse operation to the previous function, without traveling the layout hierarchy.
    /// This way we could change our UI without affecting the mapping.
    /// </summary>
    private int EditorIndexFromTabIndex(int tabIndex)
    {
      if (tabIndex < 0 || tabIndex > mainContentTabControl.TabCount - 1)
        return -1;

      return EditorIndexFromTabPage(mainContentTabControl.TabPages[tabIndex]);
    }

    private int EditorIndexFromTabPage(TabPage page)
    {
      if (!page2SqlEditor.ContainsKey(page))
        return -1;

      return dbSqlEditorBE.sql_editor_index(page2SqlEditor[page]);
    }

    private int OnSqlEditorNew(int index)
    {
      AddSqlEditor(index);
      return 0;
    }

    private void OnSqlEditorTitleChange()
    {
      int editorIndex = dbSqlEditorBE.active_sql_editor_index();
      if (editorIndex >= 0 && editorIndex < sqlEditors.Count)
      {
        SqlEditorWrapper editor = sqlEditors[editorIndex];
        sqlEditor2page[editor].Text = dbSqlEditorBE.sql_editor_caption(editorIndex);

        // Also update the tooltip text on the way.
        if (!dbSqlEditorBE.sql_editor_is_scratch(editorIndex))
          sqlEditor2page[editor].ToolTipText = dbSqlEditorBE.sql_editor_path(editorIndex);
      }
    }

    private SqlEditorWrapper AddSqlEditor(int index)
    {
      Logger.LogDebug("WQE.net", 1, "Adding new SQL editor (backend index: " + index + ")\n");

      SqlEditorWrapper sqlEditor = dbSqlEditorBE.sql_editor(index);
      sqlEditor.set_result_docking_delegate(new ResultPaneDockDelegate(this, sqlEditor));
      Control editorContainer = sqlEditor.get_editor_container();
      String caption = dbSqlEditorBE.sql_editor_caption(index);

      TabPage page = new TabPage(caption);
      page.Tag = sqlEditor;

      // Create control hierarchy.
      SplitContainer editorSplitContainer = new SplitContainer();
      editorSplitContainer.Orientation = Orientation.Horizontal;
      editorSplitContainer.Dock = DockStyle.Fill;
      editorSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      editorSplitContainer.SplitterWidth = 6;

      // Upper part (editor + toolbar).
      editorSplitContainer.Panel1.Controls.Add(editorContainer);
      editorContainer.Dock = DockStyle.Fill;
      ToolStrip toolStrip = dbSqlEditorBE.get_editor_toolbar(index);
      editorSplitContainer.Panel1.Controls.Add(toolStrip);
      toolStrip.Dock = DockStyle.Top;

      // Lower part (record set views).
      FlatTabControl resultTabControl = new FlatTabControl();
      resultTabControl.CanCloseLastTab = true;
      resultTabControl.ContentPadding = new Padding(0);
      editorSplitContainer.Panel2.Controls.Add(resultTabControl);
      resultTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      resultTabControl.HideWhenEmpty = false;
      resultTabControl.ItemPadding = new Padding(6, 0, 6, 0);
      resultTabControl.ItemSize = new Size(75, 19);
      resultTabControl.Margin = new Padding(0);
      resultTabControl.Padding = new Point(0, 0);
      resultTabControl.MaxTabSize = 200;
      resultTabControl.Name = "resultTabControl";
      resultTabControl.ShowCloseButton = true;
      resultTabControl.ShowFocusState = true;
      resultTabControl.ShowToolTips = true;
      resultTabControl.CanReorderTabs = true;
      resultTabControl.AllowDrop = true;
      resultTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      resultTabControl.TabClosing += new System.EventHandler<MySQL.Controls.TabClosingEventArgs>(ResultTabClosing);
      resultTabControl.TabClosed += new System.EventHandler<MySQL.Controls.TabClosedEventArgs>(ResultTabClosed);
      resultTabControl.MouseClick += new MouseEventHandler(recordsetTabControlMouseClick);
      resultTabControl.SelectedIndexChanged += new EventHandler(resultTabControl_SelectedIndexChanged);

      resultTabControl.UpdateColors();
      resultTabControl.BackgroundColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);

      // Add the new editor first to our internal mappers before adding it to the tab control, as
      // this triggers further actions that need the mapping.
      sqlEditors.Add(sqlEditor);
      sqlEditor2page[sqlEditor] = page;
      page2SqlEditor[page] = sqlEditor;

      page.Controls.Add(editorSplitContainer);
      mainContentTabControl.TabPages.Add(page);

      // Set an initial height for the result set panel depending on what the user last had.
      int storedRecordsetHeight = wbContext.read_state("recordset_height", "query_editor", editorSplitContainer.Height / 2);
      SafeAssignSplitterDistance(editorSplitContainer, editorSplitContainer.Height - editorSplitContainer.SplitterWidth - storedRecordsetHeight);

      editorSplitContainer.SplitterMoved += new SplitterEventHandler(editorSplitContainer_SplitterMoved);
      editorSplitContainer.Panel2Collapsed = true;

      // Select tab last so that callbacks will work on attached recordset.
      mainContentTabControl.SelectedIndex = mainContentTabControl.TabCount - 1;

      Logger.LogDebug("WQE.net", 1, "Done adding SQL editor\n");
      return sqlEditor;
    }

    private void RemoveSqlEditor(int index)
    {
      Logger.LogDebug("WQE.net", 1, "Removing SQL editor at index: " +  index + "\n");
      if (index == -1)
      {
        index = dbSqlEditorBE.active_sql_editor_index();
        Logger.LogDebug("WQE.net", 1, "Using active SQL editor at index: " + index + "\n");
      }

      // This call implicitly deletes all recordsets too.
      SqlEditorWrapper sqlEditor = sqlEditors[index];
      sqlEditors.RemoveAt(index);
      TabPage page = sqlEditor2page[sqlEditor];
      sqlEditor2page.Remove(sqlEditor);
      page2SqlEditor.Remove(page);

      // It is essential that we dispose of the editor here to make it close the backend properly,
      // which in turn has to stop pending tasks, which access UI elements.
      sqlEditor.Dispose();

      dbSqlEditorBE.remove_sql_editor(index);
      Logger.LogDebug("WQE.net", 1, "Done removing SQL editor\n");
    }

    #endregion

    #region Event handling

    void mainContentTabControl_SelectedIndexChanged(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 2, "Switching main content tab\n");

      // This could set -1 as active editor index in the backend if a page was selected
      // that does not hold an SQL editor (e.g. an object editor page).
      dbSqlEditorBE.active_sql_editor_index(EditorIndexFromTabIndex(mainContentTabControl.SelectedIndex));
      UpdateActiveRecordsetInBackend();

      UpdateApplyCancel();

      // Switch off output and secondary sidebar if we are switching to an admin page and
      // those were not explicitly switched on previously. Restore them when switching away.
      if (IsAdminActive())
      {
        if (!outputVisibilySetInAdmin && outputIsVisible)
        {
          restoreOutputArea = true;
          outputIsVisible = false;
          contentSplitContainer.Panel2Collapsed = true;
          dbSqlEditorBE.set_tool_item_checked("wb.toggleOutputArea", false);

          // Do not save this temporary state.
        }
        if (!secondarySidebarVisibilySetInAdmin && secondarySidebarIsVisible)
        {
          restoreSeconarySidebar = true;
          secondarySidebarIsVisible = false;
          mainContentSplitContainer.Panel2Collapsed = true;
          dbSqlEditorBE.set_tool_item_checked("wb.toggleSecondarySidebar", false);
        }
      }
      else
      {
        if (restoreOutputArea) // Only set if not manually adjusted in an admin page and it was visible previously.
        {
          restoreOutputArea = false;
          outputIsVisible = true;
          contentSplitContainer.Panel2Collapsed = false;
          dbSqlEditorBE.set_tool_item_checked("wb.toggleOutputArea", true);
        }
        if (restoreSeconarySidebar) // Ditto.
        {
          restoreSeconarySidebar = false;
          secondarySidebarIsVisible = true;
          mainContentSplitContainer.Panel2Collapsed = false;
          dbSqlEditorBE.set_tool_item_checked("wb.toggleSecondarySidebar", true);
        }
      }
      Logger.LogDebug("WQE.net", 2, "Done switching main content tab\n");
    }

    void editorTabControl_TabClosing(object sender, MySQL.Controls.TabClosingEventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "About to close tab: " + e.page.Text + "\n");
      int editorIndex = EditorIndexFromTabPage(e.page);
      if (editorIndex >= 0)
        e.canClose = dbSqlEditorBE.sql_editor_will_close(editorIndex);
      else
      {
        // Any docked app view (plugin)?
        if (e.page.Tag is AppViewDockContent)
          e.canClose = (e.page.Tag as AppViewDockContent).DocumentClosing();
        else
        {
          // One of the object editors.
          ITabDocument tabDocument = mainContentTabControl.ActiveDocument;
          if (tabDocument is IWorkbenchDocument)
            e.canClose = (tabDocument as IWorkbenchDocument).CanCloseDocument();
        }
      }
      Logger.LogDebug("WQE.net", 1, "Can close tab: " + e.canClose + "\n");
    }

    /// <summary>
    /// Removes a tab page docked to the editor tab control (code editors, object editors etc.)
    /// This routine must not called for code editors on dispose.
    /// </summary>
    /// <param name="page"></param>
    private void closeEditorTab(TabPage page)
    {
      Logger.LogDebug("WQE.net", 1, "Closing tab: " + page.Text + "\n");
      int editorIndex = EditorIndexFromTabPage(page);
      if (editorIndex >= 0)
      {
        RemoveSqlEditor(editorIndex);

        if (sqlEditors.Count > 0)
        {
          editorIndex = EditorIndexFromTabIndex(mainContentTabControl.SelectedIndex);
          if (editorIndex > -1)
          {
            Logger.LogDebug("WQE.net", 2, "Activating editor at index: " + editorIndex + "\n");
            dbSqlEditorBE.active_sql_editor_index(editorIndex);
          }
        }
        else
        {
          Logger.LogDebug("WQE.net", 2, "All editors closed. Create a new script tab\n");
          dbSqlEditorBE.new_sql_script_file(); // This will call our callback for setting up the editor.
          dbSqlEditorBE.active_sql_editor_index(0);
          SqlEditorWrapper sqlEditor = ActiveEditor;
          if (sqlEditor != null)
            ActiveControl = sqlEditor.get_editor_container();
        }
      }
      else
      {
        ITabDocument tabDocument = mainContentTabControl.DocumentFromPage(page);
        Logger.LogDebug("WQE.net", 2, "Close tab document: " + tabDocument.TabText + "\n");
        if (tabDocument is IWorkbenchDocument)
          (tabDocument as IWorkbenchDocument).CloseDocument();
      }
      // Nothing to do for AppView, except we should finally support OnClosing/OnClose duality there too.
      Logger.LogDebug("WQE.net", 1, "Done closing tab: " + page.Text + "\n");
    }

    void editorTabControl_TabClosed(object sender, MySQL.Controls.TabClosedEventArgs e)
    {
      closeEditorTab(e.page);
    }

    void resultTabControl_SelectedIndexChanged(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Result tabview index changed\n");
      UpdateActiveRecordsetInBackend();
      UpdateApplyCancel();
    }

    private void editorTabControl_TabMoving(object sender, TabMovingEventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Moving editor tab\n");

      // When moving tab pages we have to consider the sql editor indexing, which
      // might be different from that of the tab control (e.g. because of object editors).
      int from = EditorIndexFromTabIndex(e.FromIndex);
      if (from > -1)
      {
        // We are indeed moving a tab with an sql editor. Determine the target sql editor
        // index by iterating from the target index to the source index until another sql editor
        // is found. This is then the new sql editor target index.
        int offset = (e.FromIndex < e.ToIndex) ? -1 : 1;
        for (int index = e.ToIndex; index != e.FromIndex; index += offset)
        {
          int to = EditorIndexFromTabIndex(index);
          if (to > -1)
          {
            dbSqlEditorBE.sql_editor_reorder(from, to);

            // Also update our sqlEditor cache.
            SqlEditorWrapper editor = sqlEditors[from];
            sqlEditors[from] = sqlEditors[to];
            sqlEditors[to] = editor;
            break;
          }
        }
      }
    }

    private void applyButton_Click(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Applying changes to recordset\n");
      RecordsetView view = ActiveRecordsetView;
      if (view != null)
        view.SaveChanges();
      UpdateApplyCancel();
    }

    private void cancelButton_Click(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Resetting changes to recordset\n");
      RecordsetView view = ActiveRecordsetView;
      if (view != null)
        view.DiscardChanges();
      UpdateApplyCancel();
    }

    #endregion

    #region IWorkbenchDocument implementation

    public override UIForm BackendForm
    {
      get { return dbSqlEditorBE; }
    }

    #endregion

    #region Recordsets

    // Simple mapper from a record set to its tab page/view or vice versa.
    // No information is stored to which editor they belong.
    private Dictionary<long, TabPage> recordset2page = new Dictionary<long, TabPage>();
    private Dictionary<long, RecordsetView> recordset2placeholder = new Dictionary<long, RecordsetView>();
    private Dictionary<TabPage, MySQL.Grt.Db.RecordsetWrapper> page2recordset = new Dictionary<TabPage, MySQL.Grt.Db.RecordsetWrapper>();

    public MySQL.Grt.Db.RecordsetWrapper ActiveRecordset
    {
      get
      {
        if (mainContentTabControl.SelectedTab == null)
          return null;

        FlatTabControl resultTabControl = GetResultTabControlForEditor(ActiveEditor);
        if (resultTabControl == null || resultTabControl.SelectedTab == null)
          return null;
        
        return page2recordset.ContainsKey(resultTabControl.SelectedTab) ?
          page2recordset[resultTabControl.SelectedTab] : null;
      }
    }

    public RecordsetView ActiveRecordsetView
    {
      get
      {
        MySQL.Grt.Db.RecordsetWrapper recordset = ActiveRecordset;
        if (recordset == null)
          return null;

        return recordset2placeholder[recordset.key()];
      }
    }

    private void UpdateActiveRecordsetInBackend()
    {
      Logger.LogDebug("WQE.net", 1, "Updating backend recordset\n");
      if (mainContentTabControl.SelectedTab == null)
        return;

      int index = dbSqlEditorBE.active_sql_editor_index();
      MySQL.Grt.Db.RecordsetWrapper activeRecordset = ActiveRecordset;
      if (activeRecordset == null)
        dbSqlEditorBE.active_recordset(index, null);
      else
          dbSqlEditorBE.active_recordset(index, activeRecordset);
    }

    private void OnRecordsetCaptionChange()
    {
      Logger.LogDebug("WQE.net", 1, "Changing recordset caption\n");
      if (mainContentTabControl.SelectedTab == null)
        return;

      FlatTabControl resultTabControl = GetResultTabControlForEditor(ActiveEditor);
      if (resultTabControl == null || resultTabControl.SelectedTab == null)
        return;

      TabPage page = resultTabControl.SelectedTab;
      if (page.Tag is AppViewDockContent)
        page.Text = (page.Tag as AppViewDockContent).GetTitle();
      else
      {
        MySQL.Grt.Db.RecordsetWrapper rs = page2recordset[resultTabControl.SelectedTab];
        page.Text = rs.caption();

        UpdateApplyCancel();

        // Trigger also a menu validation as the dirty state change might also change menu items.
        wbContext.validate_menu_for_form(dbSqlEditorBE);
      }
    }

    delegate void DelegateFunc();
    private void RecordsetListChanged(Int32 editor_index, Int32 key, bool added)
    {
      if (added)
      {
        if (InvokeRequired)
          Invoke((Action)(() => AddRecordsetToResultTabview(editor_index, key)));
        else
          AddRecordsetToResultTabview(editor_index, key);
      }
      else
      {
        if (InvokeRequired)
          Invoke((Action)(() => CloseRecordset(key)));
        else
          CloseRecordset(key);
      }
    }

    private void AddRecordsetToResultTabview(Int32 editorIndex, Int32 key)
    {
      if (recordset2page.ContainsKey(key))
        return; // Shouldn't happen. Just in case...

      FlatTabControl resultTabControl = GetResultTabControlForEditor(sqlEditors[editorIndex]);
      if (resultTabControl == null)
        return;

      Logger.LogDebug("WQE.net", 1, "Adding recordset to result tabview\n");

      // Due to the way start and stop of editor actions are reported by the backend we cannot
      // easily correlate between the messages sent and the editors they are meant for.
      // However, when results arrive we know for sure the sql operation is over and we can safely
      // remove the busy animation for that (here known) editor.
      mainContentTabControl.SetBusy(TabIndexFromEditor(sqlEditors[editorIndex]), false);

      MySQL.Grt.Db.RecordsetWrapper rs = dbSqlEditorBE.recordset_for_key(editorIndex, (int)key);
      TabPage page = new TabPage(rs.caption());

      RecordsetView recordsetView = new RecordsetView();
      recordsetView.SetupRecordset(rs, false);
      page.Tag = recordsetView;

      // recordset view is embedded in the result panel and the result panel is added to the resultTabControl
      dbSqlEditorBE.dock_grid_to_result_panel(recordsetView.GridView, rs);

      Form mainForm = ParentForm;
      if (!mainForm.Visible)
        pendingRelayouts.Add(recordsetView);

      // dock result panel in the tab
      Control panel = dbSqlEditorBE.get_result_panel_for(rs);
      panel.Parent = page;
      panel.Dock = DockStyle.Fill;
      page.Controls.Add(panel);

      resultTabControl.TabPages.Add(page);
      int newIndex = resultTabControl.TabPages.Count - 1;
      resultTabControl.SetCloseButtonVisibility(newIndex, FlatTabControl.CloseButtonVisiblity.ShowButton);
      recordset2page.Add(rs.key(), page);
      recordset2placeholder.Add(rs.key(), recordsetView);
      page2recordset.Add(page, rs);
      recordsetView.ProcessModelChange();

      resultTabControl.SelectedIndex = newIndex;

      SplitContainer editorSplitContainer = resultTabControl.Parent.Parent as SplitContainer;

      // Hide SQL text area if we started collapsed (means: in edit mode for a table).
      // If there is no result set, however, show the editor.
      canTrackChanges = false; // Collapsing the panel changes the stored height.
      editorSplitContainer.Panel2Collapsed = resultTabControl.TabCount == 0;
      if (resultTabControl.TabCount > 0)
      {
        if (dbSqlEditorBE.sql_editor_start_collapsed(editorIndex))
          editorSplitContainer.SplitterDistance = 100;
        else
        {
          int storedRecordsetHeight = wbContext.read_state("recordset_height", "query_editor", editorSplitContainer.Height / 2);
          SafeAssignSplitterDistance(editorSplitContainer, editorSplitContainer.Height - editorSplitContainer.SplitterWidth - storedRecordsetHeight);
        }
      }
      canTrackChanges = true;

      Logger.LogDebug("WQE.net", 1, "Done adding recordset to result tabview\n");
    }

    private void CloseRecordset(long key)
    {
      Logger.LogDebug("WQE.net", 1, "Closing recordset\n");
      if (recordset2page.ContainsKey(key))
      {
        TabPage page = recordset2page[key];
        if (page.Parent != null)
        {
          // If the page's parent is null then we closed it in the TabClosed event where this
          // collapsing is also handled.
          FlatTabControl resultTabControl = page.Parent as FlatTabControl;
          SplitContainer editorSplitContainer = resultTabControl.Parent.Parent as SplitContainer;
          editorSplitContainer.Panel2Collapsed = resultTabControl.TabCount == 1;
        }

        recordset2page.Remove(key);

        RecordsetView recordsetView = recordset2placeholder[key];
        page.Tag = null;
        recordset2placeholder.Remove(key);
        recordsetView.Dispose();

        page2recordset.Remove(page);
        page.Dispose();
      }
      Logger.LogDebug("WQE.net", 1, "Done closing recordset\n");
    }

    private delegate void RecordsetTextOutputDelegate(String text, bool bringToFront);

    private void RecordsetTextOutput(String text, bool bringToFront)
    {
      Logger.LogDebug("WQE.net", 1, "Writing recordset data to text output\n");
      if (InvokeRequired)
      {
        RecordsetTextOutputDelegate f = new RecordsetTextOutputDelegate(RecordsetTextOutput);
        Invoke(f, new Object[] {text, bringToFront} );
      }
      else
      {
        resultSetTextBox.AppendText(text);
        if (bringToFront)
          outputSelector.SelectedIndex = 1;
      }
    }

    private void ResultTabClosing(object sender, TabClosingEventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "About closing recordset tab\n");
      e.canClose = false;
      MySQL.Grt.Db.RecordsetWrapper rs = page2recordset.ContainsKey(e.page) ? page2recordset[e.page] : null;
      if (rs != null)
        e.canClose = rs.can_close();
      else
      {
        // Any docked app view (plugin)?
        if (e.page.Tag is AppViewDockContent)
          e.canClose = (e.page.Tag as AppViewDockContent).DocumentClosing();
      }
      Logger.LogDebug("WQE.net", 1, "Can close: " + e.canClose + "\n");
    }

    private void ResultTabClosed(object sender, TabClosedEventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Closing recordset\n");

      // Collapse the result set pane if this was the last tab that was closed.
      FlatTabControl resultTabControl = sender as FlatTabControl;
      if (resultTabControl.TabCount == 0)
        (resultTabControl.Parent.Parent as SplitContainer).Panel2Collapsed = true;

      MySQL.Grt.Db.RecordsetWrapper rs = page2recordset.ContainsKey(e.page) ? page2recordset[e.page] : null;
      if (rs != null)
        rs.close(); // Will trigger the notification chain via RecordListChanged.
      // No need for handling of AppViewDockContent, as this is already closed in ResultTabClosing.
    }

    private void recordsetTabContextMenuClick(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Invoking recordset menu command\n");
      FlatTabControl resultTabControl = GetResultTabControlForEditor(ActiveEditor);
      if (resultTabControl == null)
        return;

      TabPage page = resultTabControl.SelectedTab;
      if (page == null)
        return;
      if (!page2recordset.ContainsKey(page))
        return;

      MySQL.Grt.Db.RecordsetWrapper rs = page2recordset[page];

      ToolStripMenuItem item = sender as ToolStripMenuItem;
      int clickedTab = (int)item.Owner.Tag;
      switch (item.Tag as string)
      {
        case "0": // Rename editor and tab. (currently not active)
          String newName = page.Text;
          if (StringInputForm.ShowModal("Rename a page", "Enter a new name for the page", "Name", ref newName) == DialogResult.OK)
          {
            rs.caption(newName);
            page.Text = rs.caption();
          }
          break;
        case "1": // Close recordset.
          resultTabControl.CloseTabPage(page);
          break;
        case "2": // Close all recordsets.
          while (resultTabControl.TabCount > 0)
            resultTabControl.CloseTabPage(resultTabControl.TabPages[0]);
          break;
        case "3": // Close all editors but the clicked one.
          while (resultTabControl.TabCount > 1)
          {
            if (resultTabControl.TabPages[0] == page)
              resultTabControl.CloseTabPage(resultTabControl.TabPages[1]);
            else
              resultTabControl.CloseTabPage(resultTabControl.TabPages[0]);
          }
          break;
      }
      Logger.LogDebug("WQE.net", 1, "Done invoking recordset menu command\n");
    }

    private void editorTabControlMouseClick(object sender, MouseEventArgs e)
    {
      switch (e.Button)
      {
        case MouseButtons.Right:
          {
            int clickedIndex = mainContentTabControl.TabIndexFromPosition(e.Location);
            if (clickedIndex < 0)
              return;

            // Keep the found index for later handling in the item click handler.
            editorTabsContextMenuStrip.Tag = clickedIndex;

            string path = "";
            int editorIndex = EditorIndexFromTabIndex(clickedIndex);
            if (editorIndex > -1)
              path = dbSqlEditorBE.sql_editor_path(editorIndex);
            copyFullPathToClipboardToolStripMenuItem.Enabled = path.Length > 0;
            Point p = mainContentTabControl.PointToScreen(e.Location);
            editorTabsContextMenuStrip.Show(p);
          }
          break;
      }
    }

    private void recordsetTabControlMouseClick(object sender, MouseEventArgs e)
    {
      switch (e.Button)
      {
        case MouseButtons.Right:
          {
            FlatTabControl resultTabControl = GetResultTabControlForEditor(ActiveEditor);
            if (resultTabControl == null)
              return;

            int clickedIndex = resultTabControl.TabIndexFromPosition(e.Location);
            if (clickedIndex < 0)
              return;

            // Keep the found index for later handling in the item click handler.
            recordsetTabsContextMenuStrip.Tag = clickedIndex;

            TabPage page = resultTabControl.TabPages[clickedIndex];
            renamePage_ToolStripMenuItem.Enabled = page2recordset.ContainsKey(page);
            Point p = resultTabControl.PointToScreen((Point)e.Location);
            recordsetTabsContextMenuStrip.Show(p);
          }
          break;
      }
    }

    private void editorTabContextMenuItemClick(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Invoking editor menu command\n");
      ToolStripMenuItem item = sender as ToolStripMenuItem;
      int clickedTab = (int)item.Owner.Tag;
      TabPage page = mainContentTabControl.TabPages[clickedTab];
      switch (item.Tag as string)
      {
        case "0": // Rename editor and tab. (currently not active)
          String newName = page.Text;
          if (StringInputForm.ShowModal("Rename a page", "Enter a new name for the page", "Name", ref newName) == DialogResult.OK)
          {
            dbSqlEditorBE.sql_editor_caption(clickedTab, newName);
            page.Text = newName;
          }
          break;
        case "1": // Close editor.
          mainContentTabControl.CloseTabPage(page);
          break;
        case "2": // Close all editors.
          // Close them backwards and only exactly the number of editors that are there when we started.
          // Because on close of the last one a new empty one is created which we do not want to close again.
          // Additionally, some tabs might not be closable due to changed content and the user refusing
          // to close them.
          for (int i = mainContentTabControl.TabCount - 1; i >= 0; i--)
            mainContentTabControl.CloseTabPage(mainContentTabControl.TabPages[i]);
          break;
        case "3": // Close all editors but the clicked one.
          // Similar to case 2, but we don't need to care for a newly created editor, as we always
          // keep one open.
          List<TabPage> open_tabs = new List<TabPage>();
          
          for(int index = 0; index < mainContentTabControl.TabCount; index++)
          {
            if(mainContentTabControl.TabPages[index] != page)
              open_tabs.Add(mainContentTabControl.TabPages[index]);
          }

          foreach(TabPage open_page in open_tabs)
            mainContentTabControl.CloseTabPage(open_page);

          break;

        case "4": // Copy full path to clipboard (only valid for SQL editors).
          System.Windows.Forms.Clipboard.SetText(dbSqlEditorBE.sql_editor_path(clickedTab));
          break;

        case "5": // Close all of the same type.
          ITabDocument document = mainContentTabControl.DocumentFromIndex(clickedTab);
          bool closeDocument = document != null; // Only two types of pages: documents (table editors) or sql editors.
          for (int i = mainContentTabControl.TabCount - 1; i >= 0; i--)
          {
            if (closeDocument ^ mainContentTabControl.DocumentFromIndex(i) == null)
              mainContentTabControl.CloseTabPage(mainContentTabControl.TabPages[i]);
          }
          break;
      }
      Logger.LogDebug("WQE.net", 1, "Done invoking editor menu command\n");
    }

    #endregion

    private GridView logView;

    #region History Panel

    private GridView historyEntriesView;
    private GridView historyDetailsView;

    ContextMenuStrip logViewMenuStrip = null;
    ContextMenuStrip historyListMenuStrip = new ContextMenuStrip();
    ContextMenuStrip historyDetailsMenuStrip = null;

    private void historyEntriesView_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
      e.ContextMenuStrip = historyListMenuStrip;
    }

    void historyDetailsView_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
      if (historyDetailsMenuStrip == null)
      {
        // Prepare context menu strip on first use. Since this is actually an mforms managed
        // menu we don't get item clicks from there. So we hook into the click events here too.
        // TODO: this entire handling needs a general review.
        historyDetailsMenuStrip = dbSqlEditorBE.history().get_details_context_menu();
        foreach (ToolStripItem item in historyDetailsMenuStrip.Items)
          item.Click += new EventHandler(historyDetailsMenuItemClick);
      }

      if (historyDetailsView.SelectedRows.Count > 0)
        e.ContextMenuStrip = historyDetailsMenuStrip;
    }

    private void logView_CellContextMenuStripNeeded(object sender, DataGridViewCellContextMenuStripNeededEventArgs e)
    {
      if (logViewMenuStrip == null)
      {
        // Prepare context menu strip on first use.
        logViewMenuStrip = dbSqlEditorBE.get_log_context_menu();
      }

      {
        List<DataGridViewRow> rows = logView.GetSelectedRows();
        List<int> indexes = new List<int>(rows.Count);
        foreach (DataGridViewRow row in rows)
        {
          indexes.Add(row.Index);
        }
        dbSqlEditorBE.set_log_selection(indexes);
      }

      e.ContextMenuStrip = logViewMenuStrip;
    }

    enum HistoryAction { HistoryAppendToEditor, HistoryReplaceEditor, HistoryCopyToClipboard };

    void historyDetailsMenuItemClick(object sender, EventArgs e)
    {
      Logger.LogDebug("WQE.net", 1, "Invoking history details menu command\n");
      ToolStripItem item = sender as ToolStripItem;
      switch (item.Tag.ToString())
      {
        case "copy_row":
          loadSelectedHistoryItems(HistoryAction.HistoryCopyToClipboard);
          break;
        case "append_selected_items":
          loadSelectedHistoryItems(HistoryAction.HistoryAppendToEditor);
          break;
        case "replace_sql_script":
          loadSelectedHistoryItems(HistoryAction.HistoryReplaceEditor);
          break;
      }
      Logger.LogDebug("WQE.net", 1, "Done invoking history details menu command\n");
    }

    void historyEntriesView_RowEnter(Object sender, DataGridViewCellEventArgs e)
    {
      dbSqlEditorBE.history().current_entry(e.RowIndex);
    }

    void loadSelectedHistoryItems(HistoryAction action)
    {
      Logger.LogDebug("WQE.net", 1, "Loading selected history items\n");
      if (null == historyEntriesView.CurrentRow)
        return;

      List<int> sel_indexes = new List<int>();
      foreach (DataGridViewRow row in historyDetailsView.SelectedRows)
        sel_indexes.Add(row.Index);
      sel_indexes.Sort();
      String sql = dbSqlEditorBE.restore_sql_from_history(historyEntriesView.CurrentRow.Index, sel_indexes);

      SqlEditorWrapper sqlEditor = ActiveEditor;
      switch (action)
      {
        case HistoryAction.HistoryAppendToEditor:
          if (sqlEditor != null)
            sqlEditor.append_text(sql);
          break;
        case HistoryAction.HistoryReplaceEditor:
          if (sqlEditor != null)
            sqlEditor.set_text(sql);
          break;
        case HistoryAction.HistoryCopyToClipboard:
          System.Windows.Forms.Clipboard.SetText(sql);
          break;
      }
      Logger.LogDebug("WQE.net", 1, "Done loading selected history items\n");
    }

    void historyDetailsView_CellDoubleClick(Object sender, DataGridViewCellEventArgs e)
    {
      loadSelectedHistoryItems(HistoryAction.HistoryAppendToEditor);
    }

    private void ProcessModelHistoryEntryRowsChange()
    {
      Logger.LogDebug("WQE.net", 1, "Processing history entry changes\n");
      historyEntriesView.ProcessModelRowsChange();
      int selected_entry = dbSqlEditorBE.history().current_entry();

      if (selected_entry != -1)
        historyEntriesView.SetRowSelected(selected_entry);
    }

    #endregion

    #region Event Handling

    private void DbSqlEditor_Shown(object sender, EventArgs e)
    {
      LoadFormState();
      canTrackChanges = true;

      Logger.LogInfo("WQE.net", "SQL IDE UI is ready\n");
    }

    private void mainSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (!canTrackChanges || IsDisposed || Disposing)
        return;

      wbContext.save_state("sidebar_width", "query_editor", mainSplitContainer.SplitterDistance);
    }

    private void contentSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (!canTrackChanges || IsDisposed || Disposing)
        return;

      int splitterDistance = contentSplitContainer.Height - contentSplitContainer.SplitterWidth - contentSplitContainer.SplitterDistance;
      wbContext.save_state("output_height", "query_editor", splitterDistance);
    }

    private void mainContentSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (!canTrackChanges || IsDisposed || Disposing)
        return;

      int splitterDistance = mainContentSplitContainer.Width - mainContentSplitContainer.SplitterWidth - mainContentSplitContainer.SplitterDistance;
      wbContext.save_state("support_sidebar_width", "query_editor", splitterDistance);
    }

    private void editorSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (!canTrackChanges || IsDisposed || Disposing)
        return;

      SplitContainer container = sender as SplitContainer;
      int splitterDistance = container.Height - container.SplitterWidth - container.SplitterDistance;
      wbContext.save_state("recordset_height", "query_editor", splitterDistance);
    }

    void outputPaneIndexChanged(object sender, System.EventArgs e)
    {
      ToolStripComboBox selector = sender as ToolStripComboBox;
      if (outputPageContent.Controls.Count > 0)
        outputPageContent.Controls.RemoveAt(0);
      switch (selector.SelectedIndex)
      {
        case 0:
          outputPageContent.Controls.Add(actionPanel);
          actionPanel.Dock = DockStyle.Fill;
          break;
        case 1:
          outputPageContent.Controls.Add(textOutputPage);
          textOutputPage.Dock = DockStyle.Fill;
          break;
        case 2:
          outputPageContent.Controls.Add(historyPage);
          historyPage.Dock = DockStyle.Fill;
          break;
      }
    }

    #endregion

    #region Form Layout

    private void SafeAssignSplitterDistance(SplitContainer container, int distance)
    {
      // If the splitter is within any of the min size areas of the two panels set it to the center
      // between both. If that still fails it can only mean the container is smaller than the sum of
      // the min sizes. In that case don't assign a splitter position at all.
      // Note: the orientation gives the orientation of the splitter, not the layout of the container.
      int size = container.Orientation == Orientation.Horizontal ? container.Height : container.Width;
      if (distance < container.Panel1MinSize ||
        distance > size - container.Panel2MinSize)
        distance = (size - container.Panel2MinSize - container.Panel1MinSize) / 2;

      try
      {
      if (distance > 0)
        container.SplitterDistance = distance;
      }
      catch (Exception)
      {
      	// Ignore stupid splitter distance errors. The SplitContainer really should adjust values
        // outside the valid range to a meaningful position.
      }
    }

    private void LoadFormState()
    {
      Logger.LogDebug("WQE.net", 1, "Loading form state\n");

      // Object side bar.
      int splitterDistance = wbContext.read_state("sidebar_width", "query_editor", 200);
      if (splitterDistance < 0)
        splitterDistance = 200;

      SafeAssignSplitterDistance(mainSplitContainer, splitterDistance);

      // Output tab. Distance measured from bottom (for easy default value).
      int storedSize = wbContext.read_state("output_height", "query_editor", 200);
      SafeAssignSplitterDistance(contentSplitContainer,
        contentSplitContainer.Height - contentSplitContainer.SplitterWidth - storedSize);

      // Support side bar. Distance measured from right (for easy default value).
      storedSize = wbContext.read_state("support_sidebar_width", "query_editor", 200);
      SafeAssignSplitterDistance(mainContentSplitContainer,
        mainContentSplitContainer.Width - mainContentSplitContainer.SplitterWidth - storedSize);

      // Visibility of the sidebar/output areas.
      bool visible = wbContext.read_state("sidebar_visible", "query_editor", true);
      dbSqlEditorBE.set_tool_item_checked("wb.toggleSidebar", visible);
      mainSplitContainer.Panel1Collapsed = !visible;

      outputIsVisible = wbContext.read_state("output_visible", "query_editor", true);
      dbSqlEditorBE.set_tool_item_checked("wb.toggleOutputArea", outputIsVisible);
      contentSplitContainer.Panel2Collapsed = !outputIsVisible;

      secondarySidebarIsVisible = wbContext.read_state("support_sidebar_visible", "query_editor", true);
      dbSqlEditorBE.set_tool_item_checked("wb.toggleSecondarySidebar", secondarySidebarIsVisible);
      mainContentSplitContainer.Panel2Collapsed = !secondarySidebarIsVisible;
    }
    
    /// <summary>
    /// Docks the given document to the editor tab control (object editors and plugins).
    /// </summary>
    public TabPage DockDocument(ITabDocument document, bool activate)
    {
      Logger.LogDebug("WQE.net", 1, "Docking new document (activate it: " + activate + ")\n");
      if (document is DockablePlugin)
        (document as DockablePlugin).UpdateColors();

      if (!mainContentTabControl.HasDocument(document))
        mainContentTabControl.AddDocument(document);
      if (activate)
        document.Activate();

      return mainContentTabControl.SelectedTab;
    }

    public void UndockDocument(ITabDocument document)
    {
      mainContentTabControl.RemoveDocument(document);
    }

    #endregion

    #region Miscellaneous

    /// <summary>
    /// Helper method to ease access to the result set tab control, stored in our
    /// nested tab hierarchy.
    /// </summary>
    private FlatTabControl GetResultTabControlForEditor(SqlEditorWrapper editor)
    {
      SplitContainer editorSplitContainer = GetSplitContainerForEditor(editor);
      if (editorSplitContainer == null || editorSplitContainer.Panel2.Controls.Count == 0)
        return null;

      return editorSplitContainer.Panel2.Controls[0] as FlatTabControl;
    }

    /// <summary>
    /// Helper method to ease access to the result set tab control, stored in our
    /// nested tab hierarchy.
    /// </summary>
    private SplitContainer GetSplitContainerForEditor(SqlEditorWrapper editor)
    {
      if (editor == null)
        return null;

      int index = TabIndexFromEditor(editor);
      if (index < 0 || index > mainContentTabControl.TabCount - 1)
        return null;

      TabPage editorTabPage = mainContentTabControl.TabPages[index];
      if (editorTabPage.Controls.Count == 0)
        return null;

      return editorTabPage.Controls[0] as SplitContainer;
    }

    /// <summary>
    /// Update visibility and enabled state of apply and cancel button.
    /// </summary>
    private void UpdateApplyCancel()
    {
      Logger.LogDebug("WQE.net", 2, "Updating apply/cancel buttons\n");
      RecordsetView view = ActiveRecordsetView;
      if (view == null)
      {
        applyButton.Visible = false;
        cancelButton.Visible = false;
        readOnlyLabel.Visible = false;
        readOnlyPictureBox.Visible = false;
        labelsTooltip.RemoveAll();
      }
      else
      {
        if (view.GridView.ReadOnly)
        {
          readOnlyLabel.Visible = true;
          readOnlyPictureBox.Visible = true;
          labelsTooltip.SetToolTip(readOnlyLabel, view.GridView.Model.readonly_reason());
          labelsTooltip.SetToolTip(readOnlyPictureBox, view.GridView.Model.readonly_reason());
          applyButton.Visible = false;
          cancelButton.Visible = false;
        }
        else
        {
          readOnlyLabel.Visible = false;
          readOnlyPictureBox.Visible = false;
          labelsTooltip.RemoveAll();
          applyButton.Visible = true;
          cancelButton.Visible = true;
          applyButton.Enabled = view.Dirty;
          cancelButton.Enabled = view.Dirty;
        }
      }
    }

    public override void UpdateColors()
    {
      base.UpdateColors();

      Logger.LogDebug("WQE.net", 1, "Updating display colors\n");
      outputPageToolStrip.Renderer = new FlatSubToolStripRenderer();

      readOnlyLabel.ForeColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTabUnselected, true);
      readOnlyLabel.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTabUnselected, false);
      readOnlyPictureBox.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorTabUnselected, false);
      mainSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      contentSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      mainContentSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      rightSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      historySplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);

      // Adjust all result split containers.
      foreach (TabPage page in mainContentTabControl.TabPages)
        if (page.Controls.Count > 0)
          page.Controls[0].BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);

      // The side area has the sidebar which is rooted on a split container.
      if (sideArea.Controls.Count > 0)
        sideArea.Controls[0].BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
    }

    public override bool CanCloseDocument()
    {
      Logger.LogDebug("WQE.net", 1, "Processing CanCloseDocument()\n");

      // The backend will take care for all SQL editors. We only check object editors here.
      if (!Backend.can_close())
        return false;

      foreach (ITabDocument document in mainContentTabControl.Documents)
      {
        if (document is IWorkbenchDocument)
        {
          UIForm form = (document as IWorkbenchDocument).BackendForm;
          if (!form.can_close())
            return false;
        }
        else if (document is MySQL.Forms.AppViewDockContent)
        {
          if (!(document as MySQL.Forms.AppViewDockContent).DocumentClosing())
            return false;
        }
      }
      return true;
    }

    /// <summary>
    /// Adds an arbitrary view object to the result pane for the given editor.
    /// The view is usually the visual representation of a plugin.
    /// </summary>
    /// <param name="editor">The editor whose result tabview is used.</param>
    /// <param name="view">The view to dock.</param>
    public void AddViewToResultTabview(SqlEditorWrapper editor, AppViewDockContent view)
    {
      Logger.LogDebug("WQE.net", 1, "Adding new view to result tabview\n");
      FlatTabControl resultTabControl = GetResultTabControlForEditor(editor);
      if (resultTabControl == null)
        return;

      int newIndex = resultTabControl.AddDocument(view);
      resultTabControl.TabPages[newIndex].Tag = view;
      resultTabControl.SetCloseButtonVisibility(newIndex, FlatTabControl.CloseButtonVisiblity.ShowButton);
      resultTabControl.SelectedIndex = newIndex;

      SplitContainer editorSplitContainer = resultTabControl.Parent.Parent as SplitContainer;
      editorSplitContainer.Panel2Collapsed = false;
    }

    /// <summary>
    /// Removes a previously added view from the result tabview of the given editor.
    /// </summary>
    /// <param name="editor">The editor whose result tabview is used.</param>
    /// <param name="view">The view to dock.</param>
    public void RemoveViewFromResultTabview(SqlEditorWrapper editor, AppViewDockContent view)
    {
      Logger.LogDebug("WQE.net", 1, "Removing view from result tabview\n");
      FlatTabControl resultTabControl = GetResultTabControlForEditor(editor);
      if (resultTabControl == null)
        return;

      TabPage page = view.Parent as TabPage;
      if (page == null)
        return;

      SplitContainer editorSplitContainer = resultTabControl.Parent.Parent as SplitContainer;
      editorSplitContainer.Panel2Collapsed = resultTabControl.TabCount == 1;

      page.Dispose();
    }

    public Size GetResultTabViewSize(SqlEditorWrapper editor)
    {
      FlatTabControl resultTabControl = GetResultTabControlForEditor(editor);
      if (resultTabControl == null)
        return new Size(0, 0);

      return resultTabControl.ClientSize;
    }

    /// <summary>
    /// Returns true if the currently active page in the content tab control is an admin page.
    /// </summary>
    /// <returns></returns>
    private bool IsAdminActive()
    {
      ITabDocument document = mainContentTabControl.ActiveDocument;
      if (document is AppViewDockContent)
      {
        if ((document as AppViewDockContent).GetContextName() == "administrator")
          return true;
      }
      return false;
    }

    #endregion

  }

  #region DockingPoint delegate implementation

  public class ResultPaneDockDelegate : ManagedDockDelegate
  {
    private SqlIdeForm owner = null;

    public ResultPaneDockDelegate(SqlIdeForm owner, SqlEditorWrapper editorWrapper)
      : base(editorWrapper)
    {
      this.owner = owner;
    }

    protected override void Dispose(bool disposing)
    {
      base.Dispose(disposing);
    }

    public override String get_type(Object representedObject)
    {
      return "db.Query.QueryEditor:result"; // Same as defined in wb_sql_editor_form.h
    }

    public override void dock_view(Object representedObject, AppViewDockContent view, String arg1, int arg2)
    {
      SqlEditorWrapper wrapper = representedObject as SqlEditorWrapper;
      owner.AddViewToResultTabview(wrapper, view);
    }

    public override void undock_view(Object representedObject, AppViewDockContent view)
    {
      SqlEditorWrapper wrapper = representedObject as SqlEditorWrapper;
      owner.RemoveViewFromResultTabview(wrapper, view);
    }

    public override bool select_view(Object representedObject, AppViewDockContent view)
    {
      view.Activate();
      return true;
    }

    public override void set_view_title(Object representedObject, AppViewDockContent view, String str)
    {
      view.TabText = str;
    }

    /// <summary>
    /// Meant to return the size of the host of the view, not the view itself.
    /// </summary>
    public override Size get_size(Object representedObject)
    {
      SqlEditorWrapper wrapper = representedObject as SqlEditorWrapper;
      return owner.GetResultTabViewSize(wrapper);
    }
  };

  public class MainPaneDockDelegate : ManagedDockDelegate
  {
    private SqlIdeForm owner = null;

    public MainPaneDockDelegate(SqlIdeForm owner)
      : base(null)
    {
      this.owner = owner;
    }

    protected override void Dispose(bool disposing)
    {
      base.Dispose(disposing);
    }

    public override String get_type(Object representedObject)
    {
      return "db.query.Editor:main"; // Same as defined in wb_sql_editor_form.h
    }

    public override void dock_view(Object representedObject, AppViewDockContent view, String arg1, int arg2)
    {
      TabPage page = owner.DockDocument(view, true);
      page.Tag = view;
    }

    public override void undock_view(Object representedObject, AppViewDockContent view)
    {
      owner.UndockDocument(view);
    }

    public override bool select_view(Object representedObject, AppViewDockContent view)
    {
      view.Activate();
      return true;
    }

    public override void set_view_title(Object representedObject, AppViewDockContent view, String str)
    {
      view.TabText = str;
    }

    /// <summary>
    /// Meant to return the size of the host of the view, not the view itself.
    /// </summary>
    public override Size get_size(Object representedObject)
    {
      return new Size(0, 0);
    }
  };

  #endregion

}
