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
using System.Collections;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Windows.Forms;

using MySQL.Base;
using MySQL.Controls;
using MySQL.Grt;
using MySQL.GUI.Workbench.Plugins;
using MySQL.GUI.Workbench.Properties;
using MySQL.Utilities;
using MySQL.Utilities.SysUtils;
using MySQL.Workbench;

namespace MySQL.GUI.Workbench
{
  public partial class ModelOverviewForm : TabDocument, IWorkbenchDocument
  {
    #region Member Variables
    
    // Flags to avoid recursive refreshes.
    private enum RefreshState
    {
      Attachments,
      Diagrams,
      Privileges,
      Schema
    }

    private class Identifier
    {
      public NodeIdWrapper id;
      public String objectId;

      public Identifier(NodeIdWrapper id)
      {
        this.id = id;
      }
    }

    private Hashtable states = new Hashtable();

    // Keeps states of columns for each section listview we have.
    // Currently only column widths are stored.
    private Hashtable columnStates = new Hashtable();

    // The Workbench context
    private WbContext wbContext;
    // The Workbench overview
    private Overview wbOverview;

    private WorkbenchMenuManager workbenchMenuManager;

    private List<CollapsingPanel> panelList = new List<CollapsingPanel>();

    private Overview.DisplayMode currentOverviewDisplayMode = Overview.DisplayMode.SmallIcon;

    private Dictionary<String, CollapsingPanel> panelsByNode = new Dictionary<string, CollapsingPanel>();
    private Dictionary<String, ListView> listsByNode = new Dictionary<String, ListView>();

    // Mapper between element icon ids and image list indices, depending on 
    // the view mode of the listview the item is in.
    private Dictionary<int, int> imageIndexMapper = new Dictionary<int, int>();

    private bool overviewInvalid = false;

    private String lastSearchText = "";
    private NodeIdWrapper lastFoundNode = null;

    private ModelObjectDescriptionForm modelObjectDescriptionForm;
    private UserDatatypesForm userDatatypesForm;
    private UndoHistoryForm historyForm;
    private IMessageFilter wheelMessageFilter;

    private bool splitterMovePending = false;

    public bool skipHeader = false;

    #endregion

    #region Constructors

    public ModelOverviewForm(WbContext WbContext, Overview be)
    {
      InitializeComponent();

      wbContext = WbContext;
      wbOverview = be;

      UpdateTabText();
      workbenchMenuManager = new WorkbenchMenuManager(wbContext);
      userDatatypesForm = new UserDatatypesForm(wbContext);
      historyForm = new UndoHistoryForm(wbContext);
      modelObjectDescriptionForm = new ModelObjectDescriptionForm(wbContext);

      wheelMessageFilter = new WheelMessageFilter(this);
      Application.AddMessageFilter(wheelMessageFilter);

      SetupSideBar();
      UpdateColors();
    }

    private void UpdateTabText()
    {
      Text = wbOverview.get_title();
      String newTabText = wbOverview.get_title();

      String filename = wbContext.get_filename();
      if (filename.Length > 0)
        newTabText += " (" + Path.GetFileName(filename) + ")";

      TabText = newTabText;
    }

    #endregion

    #region Internal classes

    /// <summary>
    /// Internal class used to forward mouse wheel messages to the overview page to be able to scroll
    /// all panels using the mouse wheel.
    /// </summary>
    internal class WheelMessageFilter : IMessageFilter
    {
      private ModelOverviewForm form;

      public WheelMessageFilter(ModelOverviewForm form)
      {
        this.form = form;
      }

      public bool PreFilterMessage(ref Message m)
      {
        if ((WM)m.Msg == WM.MOUSEWHEEL)
        {
          // Only handle messages if the control belongs to our form.
          Win32.POINT cursorPoint = new Win32.POINT();
          Win32.GetCursorPos(ref cursorPoint);
          IntPtr targetWindow = Win32.WindowFromPoint(cursorPoint);
          Control control = Control.FromChildHandle(targetWindow);
          if (control != null && ControlUtilities.ContainsControl(form.contentHeaderPanel, control))
          {
            // Forward the message to the control or the first of its parents which can have scrollbars.
            while (control != null && !(control is ScrollableControl))
              control = control.Parent;
            if (control != null)
            {
              Win32.SendMessage(control.Handle, m.Msg, m.WParam, m.LParam);
              return true;
            }
          }
        }

        return false;
      }
    }

    #endregion

    #region IWorkbenchDocument Interface

    public UIForm BackendForm
    {
      get { return wbOverview.get_uiform(); }
    }

    public void RefreshGUI(RefreshType refresh, String str, IntPtr ptr)
    {
      switch (refresh)
      {
        case RefreshType.RefreshSelection:
          if (ptr != null && ptr.ToInt64() != 0)
          {
            UIForm form = UIForm.GetFromFixedId(ptr);
            modelObjectDescriptionForm.UpdateForView(form);
          }
          else
            modelObjectDescriptionForm.UpdateForView(null);
          break;

        case RefreshType.RefreshDocument:
          UpdateTabText();
          break;

        case RefreshType.RefreshOverviewNodeChildren:
          RefreshNodeChildren(new NodeIdWrapper(str));
          break;

        case RefreshType.RefreshOverviewNodeInfo:
          RefreshNodeInfo(new NodeIdWrapper(str));
          break;

        case RefreshType.RefreshCloseEditor:
          CloseEditorsForObject(str);
          break;
      }
    }

    public void PerformCommand(String command)
    {
      switch (command)
      {
        case "view_user_datatypes":
          ShowUserDatatypes();
          break;
        case "view_object_description":
          ShowDescriptions();
          break;
        case "view_undo_history":
          ShowUndoHistory();
          break;
        case "wb.toggleSidebar":
          mainSplitContainer.Panel1Collapsed = !mainSplitContainer.Panel1Collapsed;
          break;
        case "wb.toggleSecondarySidebar":
          mainContentSplitContainer.Panel2Collapsed = !mainContentSplitContainer.Panel2Collapsed;
          break;
      }
    }

    public void UpdateColors()
    {
      ApplyColors(this);
      modelObjectDescriptionForm.UpdateColors();

      if (Controls.Count > 0 && Controls[0] is DrawablePanel)
        Controls[0].BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      else
        BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);

      mainSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      contentSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      sideSplitContainer.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
      scrollPanel.BackColor = Conversions.GetApplicationColor(ApplicationColor.AppColorMainBackground, false);
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

    #endregion

    #region Event Handling

    private void bottomTabControl_TabClosing(object sender, TabClosingEventArgs e)
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
      if (document is IWorkbenchDocument)
        (document as IWorkbenchDocument).CloseDocument();
      else
        if (document is MySQL.Forms.AppViewDockContent)
        {
          MySQL.Forms.AppViewDockContent content = document as MySQL.Forms.AppViewDockContent;
          content.CloseDocument();
        }

      e.page.Controls.Clear();
      e.page.Dispose();
      GC.Collect();
    }

    #endregion

    #region Public functions

    public void SearchAndFocusNode(String text)
    {
      if ((lastSearchText == null) || !text.StartsWith(lastSearchText))
        lastFoundNode = null;

      lastSearchText = text;

      lastFoundNode = wbOverview.search_child_item_node_matching(null, lastFoundNode, text);

      if (lastFoundNode != null)
        FocusNode(lastFoundNode);
    }

    public void RefreshNodeInfo(NodeIdWrapper node)
    {
      int nodeType;

      // if we're refreshing a node, first check what type is it, then update it
      // through its container or the item itself
      if (wbOverview.get_field(node, (int)Overview.Columns.NodeType, out nodeType))
      {
        wbOverview.refresh_node(node, false);

        switch ((Overview.NodeType)nodeType)
        {
          case Overview.NodeType.Root:
            // Do nothing. We refresh the entire content with the model refresh notification.
            break;

          case Overview.NodeType.Item:
            // find the object in its container
            NodeIdWrapper parent = wbOverview.get_parent(node);
            if (listsByNode.ContainsKey(parent.toString()))
            {
              ListView list = listsByNode[parent.toString()];
              UpdateListViewNode(node, list);
            }
            break;

            
          case Overview.NodeType.Section:
            // a CollapsingPanel
            break;

          case Overview.NodeType.Group:
          {
            // schema tabs
            if (panelsByNode.ContainsKey(wbOverview.get_parent(node).toString()))
            {
              CollapsingPanel panel = panelsByNode[wbOverview.get_parent(node).toString()];

              panel.Refresh();
              panel.Update();
            }
            break;
          }

          default:
            throw new Exception("Model overview: invalid node type found");
        }
      }
    }

    public void RefreshNodeChildren(NodeIdWrapper node)
    {
      int nodeType;

      if ((null == node) || !node.is_valid())
      {
        RebuildModelContents();
        return;
      }

      if (overviewInvalid)
        return;

      // find the container and refresh it
      if (wbOverview.get_field(node, (int)Overview.Columns.NodeType, out nodeType))
      {
        wbOverview.refresh_node(node, true);

        if (nodeType == (int)Overview.NodeType.Section)
        {
          RefreshItemList(node, (Overview.NodeType)nodeType);
        }
        else if (nodeType == (int)Overview.NodeType.Group)
        {
          CollapsingPanel panel = panelsByNode[node.toString()];
        }
        else if (nodeType == (int)Overview.NodeType.Division)
        {
          int childNodeType;
          wbOverview.get_field(node, (int)Overview.Columns.ChildNodeType, out childNodeType);

          switch ((Overview.NodeType)childNodeType)
          {
            case Overview.NodeType.Group:
              // Group as children of Division means a tabview
              RefreshGroupTabs(node);
              break;

            case Overview.NodeType.Section:
              break;

            case Overview.NodeType.Item:
              RefreshItemList(node, (Overview.NodeType)nodeType);
              break;
          }
        }
      }
    }


    private void RefreshGroupTabs(NodeIdWrapper node)
    {
      if (panelsByNode.ContainsKey(node.toString()))
      {
        CollapsingPanel panel = panelsByNode[node.toString()];

        panel.SuspendLayout();

        panel.Controls.Clear();
        PopulateSections(panel, currentOverviewDisplayMode, false);

        Refresh();

        panel.ResumeLayout();
      }
    }

    private void RefreshItemList(NodeIdWrapper node, Overview.NodeType nodeType)
    {
      if (listsByNode.ContainsKey(node.toString()))
      {
        ListView list = listsByNode[node.toString()];

        if (nodeType == Overview.NodeType.Section)
        {
          int childCount = wbOverview.count_children(node) - 1;
          String info = "(" + childCount + ((childCount == 1) ? " item" : " items") + ")";
          SetInfoLabelForList(list, info);
        }

        if (list.View == View.Details)
          SaveColumnStates(list);

        PopulateListView(node, list);

        if (list.View == View.Details)
          RestoreColumnStates(list);
      }
    }

    public void BeginRenameSelection()
    {
      ListView list = ActiveControl as ListView;
      if (list != null)
      {
        if (list.SelectedItems.Count > 0)
        {
          NodeIdWrapper itemNodeId = (list.SelectedItems[0].Tag as Identifier).id;
          if (itemNodeId != null && wbOverview.is_editable(itemNodeId))
            list.SelectedItems[0].BeginEdit();
        }
      }
    }

    public void RebuildModelContents()
    {
      overviewInvalid = true;

      // Don't reload the entire page if we are just about to refresh it anyway.
      if (!states.ContainsKey(RefreshState.Schema))
      {
        scrollPanel.SuspendLayout();
        NodeIdWrapper rootNodeId = wbOverview.get_root();

        ResetDocument(true);

        int count = wbOverview.count_children(rootNodeId);
        for (int i = 0; i < count; i++)
        {
          NodeIdWrapper panelNodeId = wbOverview.get_child(rootNodeId, i);
          string caption;
          int expanded;
          Overview.DisplayMode displayMode;
          int temp;

          if (!wbOverview.get_field(panelNodeId, (int)Overview.Columns.Label, out caption))
            throw new Exception("Invalid node");
          wbOverview.get_field(panelNodeId, (int)Overview.Columns.Expanded, out expanded);
          //wbOverview.get_field(panelNodeId, (int)Overview.Columns.Height, out height);
          wbOverview.get_field(panelNodeId, (int)Overview.Columns.DisplayMode, out temp);
          displayMode = (Overview.DisplayMode)temp;
          bool isExpansionDisabled = wbOverview.is_expansion_disabled();
          bool initialExpandState = !isExpansionDisabled;

          CollapsingPanel overviewPanel = new CollapsingPanel(caption, CollapsingPanelDisplayMode.Normal,
            CollapsingPanelStyle.Flat, initialExpandState, 0);
          overviewPanel.DisableExpansionIcon = isExpansionDisabled;
          overviewPanel.HeaderFont = new Font("Tahoma", 9.75f, FontStyle.Bold);
          overviewPanel.Padding = new Padding(12, 4, 0, 0);
          overviewPanel.BackColor = Color.White;

          panelsByNode[panelNodeId.toString()] = overviewPanel;

          overviewPanel.Tag = CreateIdentifier(panelNodeId);

          // Add the collapsing panel to the form. Move every panel explicitly to the top position
          // (moving all others down one step) to make stacking working properly.
          scrollPanel.Controls.Add(overviewPanel);
          scrollPanel.Controls.SetChildIndex(overviewPanel, 0);
          overviewPanel.Dock = DockStyle.Top;

          // Add panel to the list of panels for this model type so it can be deleted later.
          panelList.Add(overviewPanel);

          // If this is the first panel then remove its header.
          // The parent header panel will provide one.
          if (panelList.Count == 1)
            overviewPanel.DisplayMode = CollapsingPanelDisplayMode.NoHeader;

          // Check if there are child items
          FillPanelContent(panelNodeId, displayMode, overviewPanel);
          overviewPanel.Expanded = expanded == 1;

          if (skipHeader)
          {
            overviewPanel.DisplayTabActionButtons = false;
            overviewPanel.DisplayCustomButtons = false;
            overviewPanel.DisplayMode = CollapsingPanelDisplayMode.TabsOnly;
          }
        }
        scrollPanel.ResumeLayout(true);
      }

      // Update immediately so we have a complete overview page while loading other content.
      Update();
    }

    #endregion

    #region Helper functions

    /// <summary>
    /// Creates a key that can be used for later lookup in a dictionary consisting of
    /// a GRT icon id and a view mode (large icon, small icon).
    /// </summary>
    /// <param name="id"></param>
    /// <param name="view"></param>
    /// <returns></returns>
    private int CreateIconKey(int id, View view)
    {
      switch (view)
      {
        case View.LargeIcon:
          return 3 * id;
        case View.Tile:
          return 3 * id + 1;
        case View.SmallIcon:
        case View.Details:
          return 3 * id + 2;
      }
      return 0;
    }

    /// <summary>
    /// Returns the image index for a given node and view mode as cached in the mapper.
    /// If such a mapping does not exist yet then one is created.
    /// 
    /// Note: wbOverview must be assigned already to make this work.
    /// </summary>
    /// <returns>
    /// The index of the image within the associated image list.
    /// </returns>
    private int FindImageIndex(NodeIdWrapper id, View view)
    {
      IconSize size = IconSize.Icon16;
      if (view == View.LargeIcon)
        size = IconSize.Icon48;
      int iconId = wbOverview.get_field_icon(id, (int)Overview.Columns.Label, size);
      int iconKey = CreateIconKey(iconId, view);

      if (!imageIndexMapper.ContainsKey(iconKey))
      {
        // There is no image index stored yet for this node.
        // Add a new mapping for it.
        // This is necessary as we cannot guarantee that both image indices are the same for
        // small and large icons (which would be necessary if we want to use the same index for
        // both, large and small icon view). So we have to set the index depending on the view mode.
        int listIndex = IconManagerWrapper.get_instance().add_icon_to_imagelist(iconId);
        imageIndexMapper.Add(iconKey, listIndex);
      }

      return imageIndexMapper[iconKey];
    }

    private void SetViewMode(CollapsingPanel panel, View mode)
    {
      foreach (Control control in panel.Controls)
        if (control is ListView)
          SetViewMode(control as ListView, mode);

      panel.PerformLayout();
    }

    private void SetViewMode(CollapsingPanel panel, Overview.DisplayMode mode)
    {
      foreach (Control control in panel.Controls)
        if (control is ListView)
          SetViewMode(control as ListView, mode);

      panel.PerformLayout();
    }

    private View[] displayMode2ViewMode = { View.Details, View.LargeIcon, View.Tile, View.Details };

    private void SetViewMode(ListView view, Overview.DisplayMode mode)
    {
      View newViewMode = displayMode2ViewMode[(int)mode];
      SetViewMode(view, newViewMode);
    }

    private void SetViewMode(ListView view, View newViewMode)
    {
      // Keep the old column states if we go away from details mode.
      // They will be restored later when we go back to details view and have 
      // added the columns again.
      if (view.View == View.Details)
        SaveColumnStates(view);

      if (view.View != newViewMode)
      {
        switch (newViewMode)
        {
          case View.Tile:
            view.View = newViewMode;
            view.TileSize = new Size(140, 17);
            view.LargeImageList = IconManagerWrapper.ImageList16;
            break;
          case View.LargeIcon:
            view.View = newViewMode;
            view.LargeImageList = IconManagerWrapper.ImageList48;
            break;
          case View.Details:
            // This call will make the scrollbars visible for a moment but works around a more serious problem.
            // When the listview is switched to report mode without scrollbars being visible then it will not adjust
            // its client area for the header and parts of the first items are hidden behind the header.
            view.Scrollable = true;
            view.View = newViewMode;
            view.Scrollable = false;
            break;
        }

        // Adjust item image indices, as they might not be the same for large and small icons.
        for (int i = 0; i < view.Items.Count; i++)
        {
          NodeIdWrapper id = (view.Items[i].Tag as Identifier).id;
          view.Items[i].ImageIndex = FindImageIndex(id, view.View);
        }
        view.PerformLayout();
      }
    }


    private void FocusNode(NodeIdWrapper node)
    {
      NodeIdWrapper parent = wbOverview.get_parent(node);
      int index = node.end();

      if (listsByNode.ContainsKey(parent.toString()))
      {
        ListView list = listsByNode[parent.toString()];

        list.Focus();
        list.SelectedIndices.Clear();
        list.Items[index].Selected = true;
      }
    }

    #endregion

    #region Form implementation

    private int modelTypeCollapsingPanel_TabCountNeeded(object sender)
    {
      if (wbOverview != null)
      {
        NodeIdWrapper rootNode = wbOverview.get_root();
        if (rootNode != null)
          return wbOverview.count_children(rootNode);
      }

      return 0;
    }

    private bool modelTypeCollapsingPanel_TabInfoNeeded(object sender, int index, out int iconId, out string caption, out string description)
    {
      iconId = 0;
      caption = "";
      description = "";

      NodeIdWrapper nodeId = wbOverview.get_root();
      if (nodeId != null)
      {
        wbOverview.get_field(nodeId, (int)Overview.Columns.Label, out caption);
        return true;
      }
      else
        return false;
    }

    private Identifier CreateIdentifier(NodeIdWrapper nodeId)
    {
      Identifier identifier = new Identifier(nodeId);
      identifier.objectId = wbOverview.get_node_unique_id(nodeId);
      return identifier;
    }

    private void FillPanelContent(NodeIdWrapper panelNodeId, Overview.DisplayMode displayMode, CollapsingPanel overviewPanel)
    {
      // Check the node type of the child items
      int itemNodeType;
      wbOverview.get_field(panelNodeId, (int)Overview.Columns.ChildNodeType, out itemNodeType);

      switch ((Overview.NodeType)itemNodeType)
      {
        case Overview.NodeType.Group:
          {
            // If the child items are of type Group, add them as tabs.
            EnableTabViewForPanel(displayMode, overviewPanel);

            // Fill child items
            if (wbOverview.count_children(panelNodeId) > 0)
              FillPanelContent(wbOverview.get_child(panelNodeId, 0), currentOverviewDisplayMode, overviewPanel);
          }
          break;
        case Overview.NodeType.Section:
          // If they are of item type Section populate the list view with sections and items.
          PopulateSections(overviewPanel, displayMode, true);
          break;
        case Overview.NodeType.Item:
          {
            // If the child entry is an item thean we don't have any intermediate structure.
            ListView sectionListview = GetSectionListview(panelNodeId, overviewPanel, false, displayMode, "", "");
            cacheListview(panelNodeId.toString(), sectionListview);
            PopulateListView(panelNodeId, sectionListview);
            break;
          }
      }
    }

    /// <summary>
    /// Helper class to make sorting in our list views case insensitive.
    /// </summary>
    private class InsensitiveListviewComparer : IComparer
    {
      #region IComparer Members

      private CaseInsensitiveComparer comparer = new CaseInsensitiveComparer();

      public int column; // The column to sort on.
      public SortOrder order = SortOrder.None;

      public InsensitiveListviewComparer(int column, SortOrder order)
      {
        this.column = column;
        this.order = order;
      }

      public int Compare(object x, object y)
      {
        if (order == SortOrder.None)
          return 0;

        // Compare text case-insensitively, but make the default entry always the first one.
        if (((ListViewItem)x).Index == 0)
          return -1;
        else
          if (((ListViewItem)y).Index == 0)
            return 1;
          else
          {
            if (order == SortOrder.Ascending)
              return comparer.Compare(
                (x as ListViewItem).SubItems[column].Text,
                (y as ListViewItem).SubItems[column].Text);
            else
              return comparer.Compare(
                (y as ListViewItem).SubItems[column].Text,
                (x as ListViewItem).SubItems[column].Text);
          }
      }

      #endregion
    }

    private void SetInfoLabelForList(ListView list, String info)
    {
      CollapsingPanel overviewPanel = list.Parent as CollapsingPanel;
      if (null == overviewPanel)
        return;

      Panel panel = null;
      foreach (Control control in overviewPanel.Controls)
      {
        if (control == list)
          break;
        panel = control as Panel;
      }

      if (panel != null)
      {
        Label label = panel.Controls[0] as Label;
        if (label != null)
          label.Text = info;
      }
    }

    /// <summary>
    /// Returns the listview for a given section. If it does not exist yet it gets created.
    /// Optionally a simple panel is added as header for this section.
    /// </summary>
    private ListView GetSectionListview(NodeIdWrapper controlNodeId, CollapsingPanel overviewPanel, bool addSectionHeader,
      Overview.DisplayMode displayMode, String caption, String info)
    {
      // Iterate over all sections we have already and find the one with the given control id.
      String objectId = wbOverview.get_node_unique_id(controlNodeId);
      foreach (Control control in overviewPanel.Controls)
      {
        if (control.Tag != null)
        {
          String currentObjectId = (control.Tag as Identifier).objectId;
          if (currentObjectId != null && currentObjectId == objectId)
          {
            ListView listview = control as ListView;
            SetViewMode(listview, displayMode);
            return listview;
          }
        }
      }

      // If we come here then the section does not exist yet, so add it. Start with a header.
      if (addSectionHeader)
      {
        Panel panel = new Panel();
        panel.BorderStyle = BorderStyle.None;
        panel.Padding = new Padding(5, 2, 5, 0);
        panel.BackgroundImage = Resources.header_bar_blue;
        panel.BackgroundImageLayout = ImageLayout.None;

        panel.Height = 24;

        // Insert client controls in reverse order.
        // Info label.
        Label infoLabel = new Label();
        infoLabel.Text = info;
        infoLabel.ForeColor = Color.Gray;
        infoLabel.Font = overviewPanel.Font;
        infoLabel.AutoSize = true;
        infoLabel.Margin = new Padding(10, 0, 0, 0);
        infoLabel.Dock = DockStyle.Left;
        panel.Controls.Add(infoLabel);

        overviewPanel.Controls.Add(panel);

        // Caption label.
        Label captionLabel = new Label();
        captionLabel.Text = caption;
        //captionLabel.Font = new Font(overviewPanel.Font, FontStyle.Bold);
        captionLabel.AutoSize = true;
        captionLabel.Dock = DockStyle.Left;
        captionLabel.ForeColor = Color.Black;
        panel.Controls.Add(captionLabel);

        overviewPanel.Controls.Add(panel);
      }

      // Then the content view.
      ListView sectionListview = new ListView();
      sectionListview.BorderStyle = BorderStyle.None;
      sectionListview.AllowColumnReorder = true;
      sectionListview.ShowItemToolTips = true;

      // Undocumented feature: enabling AutoSize will indeed make the control automatically resize
      // (the docs say it wouldn't) though without considering long captions. For them to work 
      // additionally Scrollable can be set to true to show a scrollbar, but only in this special case.
      sectionListview.AutoSize = true;
      sectionListview.MultiSelect = false;
      sectionListview.Scrollable = true;
      sectionListview.Visible = true;
      sectionListview.Font = overviewPanel.Font;
      sectionListview.Tag = CreateIdentifier(controlNodeId);
      sectionListview.Sorting = SortOrder.None; // We do custom sort.

      sectionListview.DoubleClick += new EventHandler(listViewDoubleClick);
      sectionListview.ItemDrag += new ItemDragEventHandler(listViewItemDrag);
      sectionListview.SelectedIndexChanged += new EventHandler(listViewSelectedIndexChanged);
      sectionListview.KeyDown += new KeyEventHandler(listViewKeyDown);
      sectionListview.MouseUp += new MouseEventHandler(listViewMouseUp);
      sectionListview.ColumnClick += new ColumnClickEventHandler(ListviewColumnClick);
      sectionListview.Enter += new EventHandler(ListViewEnter);

      // Renaming of overview nodes
      sectionListview.LabelEdit = true;
      sectionListview.AfterLabelEdit += new LabelEditEventHandler(listViewAfterLabelEdit);
      sectionListview.BeforeLabelEdit += new LabelEditEventHandler(listViewBeforeLabelEdit);

      // Add it to the panel. Usually the layout engine of the panel should take care for
      // the layout of the controls, but just to be sure we set here a dock style.
      overviewPanel.Controls.Add(sectionListview);
      sectionListview.Dock = DockStyle.Top;

      // Set Image Lists
      sectionListview.SmallImageList = IconManagerWrapper.ImageList16;
      sectionListview.LargeImageList = IconManagerWrapper.ImageList48;

      SetViewMode(sectionListview, displayMode);

      sectionListview.Update();
      return sectionListview;
    }

    /// <summary>
    /// When entering a list view the surrounding scroll panel scrolls it into view such that its
    /// left upper corner is at the left upper position. This happens only if the listview is larger
    /// than the visible scroll panel area. Since clicking on the listview requires it to be already
    /// in view there's no sense in scrolling.
    /// We try to counter the scroll event by scrolling back to where we were before, even though
    /// this means slight flickering.
    /// </summary>
    void ListViewEnter(object sender, EventArgs e)
    {
      Action<Point> d = (Point p) =>
      {
        scrollPanel.AutoScrollPosition = new Point(Math.Abs(p.X), Math.Abs(p.Y));
      };
      BeginInvoke(d, scrollPanel.AutoScrollPosition);
    }

    void ListviewColumnClick(object sender, ColumnClickEventArgs e)
    {
      ListView listview = (sender as ListView);
      SortOrder newOrder = SortOrder.Ascending;
      if (!(listview.ListViewItemSorter is InsensitiveListviewComparer))
        listview.ListViewItemSorter = new InsensitiveListviewComparer(e.Column, newOrder);
      else
      {
        // Adjust sort order depending on which column was clicked on.
        // A previously not selected column always sorts ascending.
        if ((listview.ListViewItemSorter as InsensitiveListviewComparer).column == e.Column)
        {
          // Another click on the same column. Reverse the sort order.
          if ((listview.ListViewItemSorter as InsensitiveListviewComparer).order == SortOrder.Ascending)
            newOrder = SortOrder.Descending;
        }
        listview.ListViewItemSorter = new InsensitiveListviewComparer(e.Column, newOrder);
      }
      Win32.SetSortIcon(listview, e.Column, newOrder);
    }

    /// <summary>
    /// Sets the given panel up so that it shows tabs on top that can be used to switch content.
    /// </summary>
    /// <param name="displayMode"></param>
    /// <param name="overviewPanel"></param>
    private void EnableTabViewForPanel(Overview.DisplayMode displayMode, CollapsingPanel overviewPanel)
    {
      overviewPanel.TabHeaderImageList = new ImageList();
      overviewPanel.TabHeaderImageList.ColorDepth = ColorDepth.Depth32Bit;
      overviewPanel.TabHeaderImageList.ImageSize = new Size(32, 32);

      overviewPanel.TabHeaderImageList = IconManagerWrapper.ImageList32;

      overviewPanel.DisplayMode = CollapsingPanelDisplayMode.HeaderAndTab;

      overviewPanel.TabCountNeeded += new TabCountHandler(overviewPanel_TabCountNeeded);
      overviewPanel.TabInfoNeeded += new TabInfoHandler(overviewPanel_TabInfoNeeded);
      overviewPanel.TabChanged += new TabChangedHandler(overviewPanel_TabChanged);
      overviewPanel.TabDoubleClicked += new MouseEventHandler(overviewPanel_TabDoubleClicked);
      overviewPanel.TabHeaderMouseUp += new MouseEventHandler(overviewPanel_TabHeaderMouseUp);

      // Select the first tab to populate the ListView
      overviewPanel.SelectedTabIndex = 0;

      overviewPanel.DisplayTabActionButtons = true;
      overviewPanel.TabAddButtonClicked += new EventHandler(overviewPanel_TabAddButtonClicked);
      overviewPanel.TabDelButtonClicked += new EventHandler(overviewPanel_TabDelButtonClicked);

      // Buttons for display mode.
      overviewPanel.DisplayCustomButtons = true;
      overviewPanel.CustomButtonClicked += new CustomButtonEvent(overviewPanel_CustomButtonClicked);

      overviewPanel.AddCustomButton(26, 23, "collapsing_panel_grid_large_icons",
        displayMode == Overview.DisplayMode.LargeIcon);
      overviewPanel.AddCustomButton(26, 23, "collapsing_panel_grid_small_icons",
        displayMode == Overview.DisplayMode.SmallIcon);
      overviewPanel.AddCustomButton(26, 23, "collapsing_panel_grid_details",
        displayMode == Overview.DisplayMode.List);
    }

    /// <summary>
    /// Save the state of the current columns in the given list view so it can be restored later.
    /// </summary>
    /// <param name="listview"></param>
    private void SaveColumnStates(ListView listview)
    {
      String id = (listview.Tag as Identifier).objectId;
      if (id != null)
      {
        List<int> widths = new List<int>();
        for (int i = 0; i < listview.Columns.Count; i++)
          widths.Add(listview.Columns[i].Width);
        columnStates[id] = widths;
      }
    }

    /// <summary>
    /// Restores a saved column state of the given listview (if it exists).
    /// </summary>
    /// <param name="listview"></param>
    private void RestoreColumnStates(ListView listview)
    {
      String id = (listview.Tag as Identifier).objectId;
      if (id != null && columnStates.ContainsKey(id))
      {
        List<int> widths = (List<int>)columnStates[id];
        for (int i = 0; i < widths.Count && i < listview.Columns.Count; i++)
          listview.Columns[i].Width = widths[i];
      }
    }

    /// <summary>
    /// Fills the given panel with a number of sections (simple panel + listview pair) per section
    /// entry in wbOverview.
    /// </summary>
    private void PopulateSections(CollapsingPanel overviewPanel, Overview.DisplayMode displayMode, bool setDefaultTabPage)
    {
      if (overviewPanel != null && overviewPanel.Tag != null)
      {
        overviewPanel.SuspendLayout();
        try
        {
          NodeIdWrapper parentNodeId = null;

          // If this panel has tabs, choose the NodeId of the selected tab as parent.
          if (overviewPanel.DisplayMode == CollapsingPanelDisplayMode.HeaderAndTab ||
            overviewPanel.DisplayMode == CollapsingPanelDisplayMode.TabsOnly)
          {
            // make sure the selected index is still valid
            int count = wbOverview.count_children((overviewPanel.Tag as Identifier).id);
            if (count > 0)
            {
              int default_tab_page_index = wbOverview.get_default_tab_page_index();
              if (setDefaultTabPage && default_tab_page_index != -1)
              {
                overviewPanel.SelectedTabIndex = default_tab_page_index;
              }
              else if (overviewPanel.SelectedTabIndex > count - 1)
              {
                if (count > 0)
                  overviewPanel.SelectedTabIndex = count - 1;
                else
                  overviewPanel.SelectedTabIndex = 0;
              }

              parentNodeId = wbOverview.get_child((overviewPanel.Tag as Identifier).id,
                overviewPanel.SelectedTabIndex);

              // reselect the tab
              wbOverview.focus_node(parentNodeId);
            }
            else
            {
              overviewPanel.SelectedTabIndex = 0;
              parentNodeId = null;
            }
          }
          else
            // If this panel has no tabs, choose the NodeId of the panel itself as parent
            if (overviewPanel.DisplayMode == CollapsingPanelDisplayMode.Normal)
              parentNodeId = (overviewPanel.Tag as Identifier).id;

          if (parentNodeId != null)
          {
            int itemCount = wbOverview.count_children(parentNodeId);
            if (itemCount > 0)
            {
              NodeIdWrapper itemNodeId = wbOverview.get_child(parentNodeId, 0);

              // Check the node type of the child items
              int nodeType;
              wbOverview.get_field(itemNodeId, (int)Overview.Columns.NodeType, out nodeType);

              // If the node type is secion, loop over all sections and add them
              if (nodeType == (int)Overview.NodeType.Section)
              {
                for (int i = 0; i < itemCount; i++)
                {
                  NodeIdWrapper sectionNodeId = wbOverview.get_child(parentNodeId, i);

                  string caption;
                  wbOverview.get_field(sectionNodeId, (int)Overview.Columns.Label, out caption);

                  // Determine child count for info display but don't consider the default entry.
                  int childCount = wbOverview.count_children(sectionNodeId) - 1;
                  String info = "(" + childCount + ((childCount == 1) ? " item" : " items") + ")";
                  ListView sectionListview = GetSectionListview(sectionNodeId, overviewPanel, true, displayMode, caption, info);

                  cacheListview(sectionNodeId.toString(), sectionListview);

                  PopulateListViewColumns(sectionNodeId, sectionListview, displayMode);

                  PopulateListView(sectionNodeId, sectionListview);

                  if (sectionListview.View == View.Details)
                    RestoreColumnStates(sectionListview);
                }
              }
              else
              {
                // If there are no sections, simple add the items.
                ListView sectionListview = GetSectionListview(parentNodeId, overviewPanel, false, displayMode, "", "");
                cacheListview(parentNodeId.toString(), sectionListview);
                PopulateListView(parentNodeId, sectionListview);
              }
            }
          }
        }
        finally
        {
          overviewPanel.ResumeLayout();
        }
      }
    }

    private void PopulateListViewColumns(NodeIdWrapper sectionNodeId, ListView sectionListview, Overview.DisplayMode displayMode)
    {
      // Refresh columns for this view.
      sectionListview.Columns.Clear();
      sectionListview.Columns.Add("Name", 200);

      // Only add more columns if we are not in tile view mode.
      // We add columns also for large icon view, even though we don't see them, as we can then
      // quicker switch between large icon and tile view (no need to re-populate the view then).
      if (displayMode == Overview.DisplayMode.List)
      {
        int columnCount = wbOverview.get_details_field_count(sectionNodeId);
        for (int counter = 0; counter < columnCount; counter++)
        {
          String columnCaption = wbOverview.get_field_name(sectionNodeId,
            (int)Overview.Columns.FirstDetailField + counter);
          sectionListview.Columns.Add(columnCaption, 100);
        }
      }
    }


    private void PopulateListView(NodeIdWrapper parentNodeId, ListView listView)
    {
      listView.BeginUpdate();
      try
      {
        int itemCount = wbOverview.count_children(parentNodeId);
        ListViewItem[] items = new ListViewItem[itemCount];

        int detail_fields_count = wbOverview.get_details_field_count(parentNodeId);

        for (int i = 0; i < itemCount; i++)
        {
          NodeIdWrapper itemNodeId = wbOverview.get_child(parentNodeId, i);
          string caption;

          wbOverview.get_field(itemNodeId, (int)Overview.Columns.Label, out caption);
          ListViewItem item = new ListViewItem(caption, FindImageIndex(itemNodeId, listView.View));
          item.Tag = CreateIdentifier(itemNodeId);
          item.ToolTipText = caption;

          // Add details for that item, if available.
          for (int counter = 0; counter < detail_fields_count; counter++)
          {
            String detailText;
            wbOverview.get_field(itemNodeId, (int)Overview.Columns.FirstDetailField + counter, out detailText);
            item.SubItems.Add(detailText.Replace(Environment.NewLine, " / "));
          }

          items[i] = item;
        }
        listView.Items.Clear();
        listView.Items.AddRange(items);

        // Setting the sorter also starts sorting.
        listView.ListViewItemSorter = new InsensitiveListviewComparer(0, SortOrder.Ascending);
        Win32.SetSortIcon(listView, 0, SortOrder.Ascending);

        // select stuff
        List<int> selected = wbOverview.get_selected_children(parentNodeId);
        foreach (int i in selected)
          listView.SelectedIndices.Add(i);
      }
      finally
      {
        listView.EndUpdate();
        listView.PerformLayout();
      };
    }

    /// <summary>
    /// Updates all fields of the given node.
    /// </summary>
    private void UpdateListViewNode(NodeIdWrapper nodeId, ListView listView)
    {
      // Find the listview item that corresponds to the given node id and update it.
      // Don't wrap this with BeginUpdate/EndUpdate. That will produce more flicker than
      // just updating the item.
      String objectId = wbOverview.get_node_unique_id(nodeId);
      foreach (ListViewItem item in listView.Items)
      {
        String currentObjectId = (item.Tag as Identifier).objectId;
        if (currentObjectId != null && currentObjectId == objectId)
        {
          string caption;

          wbOverview.get_field(nodeId, (int)Overview.Columns.Label, out caption);
          item.Text = caption;
          item.ToolTipText = caption;

          // Note: the first sub item is the same as the main item.
          int offset = -1;
          foreach (ListViewItem.ListViewSubItem subItem in item.SubItems)
          {
            if (offset > -1)
            {
              String detailText;
              wbOverview.get_field(nodeId, (int)Overview.Columns.FirstDetailField + offset, out detailText);
              subItem.Text = detailText.Replace(Environment.NewLine, " / ");
            }
            offset++;
          }
          break;
        }
      }
    }

    void PopupActionHandler(object sender, EventArgs e)
    {
      ToolStripItem item = sender as ToolStripItem;

      if (item != null)
      {
        String name = item.Name;

        if (name == "builtin:rename")
        {
          if (item.Owner.Tag is CollapsingPanel)
          {
            CollapsingPanel panel = item.Owner.Tag as CollapsingPanel;
          }
          else if (item.Owner.Tag is ListView)
          {
            ListView list = item.Owner.Tag as ListView;

            if (list.SelectedItems.Count > 0)
              list.SelectedItems[0].BeginEdit();
          }
        }
        else if (name == "drop")
        {
          List <NodeIdWrapper> node_ids = new List<NodeIdWrapper>();

          if (item.Owner.Tag is ListView)
          {
            ListView list = item.Owner.Tag as ListView;
            foreach (ListViewItem list_item in list.SelectedItems)
              node_ids.Add((list_item.Tag as Identifier).id);
          }
          else
            node_ids.Add((item.Tag as Identifier).id);

          wbOverview.activate_popup_item_for_nodes(name, node_ids);
        }
        else
          wbOverview.activate_popup_item_for_nodes(name, new List<NodeIdWrapper>() { (item.Tag as Identifier).id });
      }
    }

    void overviewPanel_TabHeaderMouseUp(object sender, MouseEventArgs e)
    {
      CollapsingPanel panel = sender as CollapsingPanel;

      if (panel != null)
      {
        if (panel != null && panel.Tag != null)
        {
          List<MySQL.Base.MenuItem> items;
          List<NodeIdWrapper> nodes = new List<NodeIdWrapper>();

          NodeIdWrapper itemNodeId= new NodeIdWrapper();
          int tab = panel.GetTabAtPosition(e.X, e.Y);
          if (tab >= 0)
          {
            itemNodeId = wbOverview.get_child((panel.Tag as Identifier).id, tab);
            nodes.Add(itemNodeId);
          }

          items = wbOverview.get_popup_items_for_nodes(nodes);

          if (items != null && items.Count > 0)
          {
            System.Windows.Forms.ContextMenuStrip menu;

            menu = workbenchMenuManager.ShowContextMenu(panel, items, e.X, e.Y, new EventHandler(PopupActionHandler));
            menu.Tag = panel;

            SetMenuItemsTag(menu.Items, CreateIdentifier(itemNodeId));
          }
        }
      }
    }

    void SetMenuItemsTag(ToolStripItemCollection menuItems, object tagObject)
    {
      foreach (ToolStripItem tsitem in menuItems)
      {
        if (tsitem is ToolStripMenuItem)
        {
          ToolStripMenuItem item = tsitem as ToolStripMenuItem;
          if (item.Tag != null)
            throw new Exception("item.Tag was expected to be unset");
          if ((item.DropDownItems != null) && (item.DropDownItems.Count > 0))
            SetMenuItemsTag(item.DropDownItems, tagObject);
          else
            item.Tag = tagObject;
        }
      }
    }

    void listViewKeyDown(object sender, KeyEventArgs e)
    {
      ListView listView = sender as ListView;

      if (e.Modifiers == Keys.None && e.KeyCode == Keys.F2)
      {
        if (listView.SelectedItems.Count > 0)
          listView.SelectedItems[0].BeginEdit();
      }
    }

    void listViewMouseUp(object sender, MouseEventArgs e)
    {
      ListView listView = sender as ListView;

      if (listView != null && e.Button == MouseButtons.Right)
      {
        List<MySQL.Base.MenuItem> items = new List<MySQL.Base.MenuItem>();
        List<NodeIdWrapper> item_nodes = new List<NodeIdWrapper>();

        if (listView.SelectedItems.Count == 0)
        {
          item_nodes.Add((listView.Tag as Identifier).id); // nodeid for the group represented by the listview
        }
        else
        {
          foreach (ListViewItem item in listView.SelectedItems)
            item_nodes.Add((item.Tag as Identifier).id);
        }

        if (item_nodes.Count != 0)
        {
          items = wbOverview.get_popup_items_for_nodes(item_nodes);
          if (items.Count > 0)
          {
            System.Windows.Forms.ContextMenuStrip menu;

            menu = workbenchMenuManager.ShowContextMenu(listView, items, e.X, e.Y, new EventHandler(PopupActionHandler));
            menu.Tag = listView;

            SetMenuItemsTag(menu.Items, CreateIdentifier(item_nodes[0]));
          }
        }
      }
    }

    void listViewSelectedIndexChanged(object sender, EventArgs e)
    {
      ListView listView = sender as ListView;

      // If the parent of the listview is null then is it currently being destroyed.
      // Strangely enough, listView.Disposing is still false.
      if (listView != null && listView.Parent != null)
      {
        wbOverview.begin_selection_marking();

        foreach (ListViewItem item in listView.SelectedItems)
        {
          NodeIdWrapper itemNodeId = (item.Tag as Identifier).id;
          try
          {
            wbOverview.select_node(itemNodeId);
          }
          catch
          {
          }
        }

        wbOverview.end_selection_marking();
      }
    }

    void listViewBeforeLabelEdit(object sender, LabelEditEventArgs e)
    {
      ListView listView = sender as ListView;
      ListViewItem item = listView.Items[e.Item];
      if (item == null)
        return;

      if (item.Tag == null)
        e.CancelEdit = true;
      else
      {
        NodeIdWrapper itemNodeId = (item.Tag as Identifier).id;
        if (itemNodeId == null || !wbOverview.is_editable(itemNodeId))
          e.CancelEdit = true;
      }
    }

    void listViewAfterLabelEdit(object sender, LabelEditEventArgs e)
    {
      ListView listView = sender as ListView;

      if (listView != null && e.Item >= 0 && e.Label != null && !e.Label.Equals(""))
      {
        ListViewItem item = listView.Items[e.Item];
        if (item != null && item.Tag != null)
        {
          NodeIdWrapper itemNodeId = (item.Tag as Identifier).id;
          if (itemNodeId != null)
          {
            if (!wbOverview.set_field(itemNodeId, (int)Overview.Columns.Label, e.Label))
              e.CancelEdit = true;
          }
        }
      }
      else
        if (e.Label != null)
          e.CancelEdit = true;
    }

    void listViewItemDrag(object sender, ItemDragEventArgs e)
    {
      ListView listView = sender as ListView;

      if (listView != null)
      {
        ListView.SelectedListViewItemCollection selItems = listView.SelectedItems;
        List<GrtValue> selObjects = new List<GrtValue>();

        foreach (ListViewItem item in selItems)
        {
          if (item.Tag != null)
          {
            NodeIdWrapper itemNodeId = (item.Tag as Identifier).id;
            GrtValue val = wbOverview.get_grt_value(itemNodeId, 0);
            if (itemNodeId != null)
              selObjects.Add(val);
          }
        }

        if (selObjects.Count > 0)
          DoDragDrop(selObjects, DragDropEffects.Copy);
      }
    }

    void listViewDoubleClick(object sender, EventArgs e)
    {
      ListView view = sender as ListView;
      if (view != null && view.SelectedItems.Count > 0 && view.SelectedItems[0].Tag != null)
      {
        NodeIdWrapper nodeId = (view.SelectedItems[0].Tag as Identifier).id;
        if (nodeId != null)
          wbOverview.activate_node(nodeId);
      }
    }

    private void overviewPanel_TabDoubleClicked(object sender, MouseEventArgs e)
    {
      CollapsingPanel panel = sender as CollapsingPanel;

      if (panel != null && panel.Tag != null)
        wbOverview.activate_node(
          wbOverview.get_child((panel.Tag as Identifier).id, panel.SelectedTabIndex));
    }

    private void overviewPanel_TabChanged(object sender, int index)
    {
      CollapsingPanel panel = sender as CollapsingPanel;

      if (panel != null && panel.Tag != null && index >= 0)
        wbOverview.select_node(wbOverview.get_child((panel.Tag as Identifier).id, panel.SelectedTabIndex));

      // Delete all sections in the panel and re-populate it.
      SuspendLayout();
      try
      {
        foreach (Control control in panel.Controls)
          if (control is ListView)
          {
            ListView view = control as ListView;
            if (view.View == View.Details)
              SaveColumnStates(view);
          }
        panel.Controls.Clear();
        PopulateSections(panel, currentOverviewDisplayMode, false);
      }
      finally
      {
        ResumeLayout();
      }
    }

    private int overviewPanel_TabCountNeeded(object sender)
    {
      CollapsingPanel panel = sender as CollapsingPanel;

      if (panel != null && panel.Tag != null)
        return wbOverview.count_children((panel.Tag as Identifier).id);
      else
        return 0;
    }

    private bool overviewPanel_TabInfoNeeded(object sender, int index, out int iconId, out string caption, out string description)
    {
      iconId = 0;
      description = "";
      caption = "";

      CollapsingPanel panel = sender as CollapsingPanel;
      if (panel != null && panel.Tag != null)
      {
        NodeIdWrapper tabNodeId = wbOverview.get_child((panel.Tag as Identifier).id, index);
        if (tabNodeId != null)
        {
          wbOverview.get_field(tabNodeId, (int)Overview.Columns.Label, out caption);
          description = wbOverview.get_field_description(tabNodeId, 0);
          int grtIconId = wbOverview.get_field_icon(tabNodeId, (int)Overview.Columns.Label, IconSize.Icon32);
          iconId = IconManagerWrapper.get_instance().add_icon_to_imagelist(grtIconId);
        }
      }

      return true;
    }

    void overviewPanel_TabDelButtonClicked(object sender, EventArgs e)
    {
      CollapsingPanel panel = sender as CollapsingPanel;

      if (panel != null && panel.Tag != null)
      {
        NodeIdWrapper groupNodeId = (panel.Tag as Identifier).id;

        // Delete section object.
        wbOverview.request_delete_object(wbOverview.get_focused_child(groupNodeId));
      }
    }

    void overviewPanel_TabAddButtonClicked(object sender, EventArgs e)
    {
      CollapsingPanel panel = sender as CollapsingPanel;

      if (panel != null && panel.Tag != null)
      {
        NodeIdWrapper groupNodeId = (panel.Tag as Identifier).id;

        // Add section object.
        wbOverview.request_add_object(groupNodeId);
        wbOverview.refresh_node(groupNodeId, true);
      }
    }

    void overviewPanel_CustomButtonClicked(object sender, int index)
    {
      CollapsingPanel panel = sender as CollapsingPanel;
      Overview.DisplayMode newMode = (Overview.DisplayMode)(index + 1);
      if (newMode != currentOverviewDisplayMode)
      {
        bool fullRefresh = (newMode == Overview.DisplayMode.SmallIcon) ||
          (currentOverviewDisplayMode == Overview.DisplayMode.SmallIcon);
        currentOverviewDisplayMode = newMode;

        // Unfortunately, we cannot just switch the view mode here as this would leave all columns
        // intact, which in turn means we have unwanted info in tile mode.
        // So we have to fully re-populate the listviews in cases where we switch from or to tile mode.
        if (fullRefresh)
          PopulateSections(panel, currentOverviewDisplayMode, false);
        else
          SetViewMode(panel, currentOverviewDisplayMode);
      }
    }

    private void ModelOverviewForm_Shown(object sender, EventArgs e)
    {
      LoadFormState();
    }

    internal void ResetDocument(bool removePanels)
    {
      overviewInvalid = false;

      SuspendLayout();

      try
      {
        foreach (CollapsingPanel panel in panelList)
        {
          panel.Controls.Clear();
          if (removePanels && panel.Parent != null)
            panel.Parent.Controls.Remove(panel);
        }

        if (removePanels)
          panelList.Clear();
      }
      finally
      {
        ResumeLayout();
      }
    }

    /// <summary>
    /// Returns the current view mode of the listview that represents the content of the given panel.
    /// </summary>
    /// <param name="panel"></param>
    /// <returns></returns>
    private View GetViewMode(CollapsingPanel panel)
    {
      foreach (Control control in panel.Controls)
        if (control is ListView)
          return (control as ListView).View;

      return View.LargeIcon;
    }

    #endregion

    #region Other Implementation

    private void SetupSideBar()
    {
      mainSplitContainer.SuspendLayout();
      sideSplitContainer.SuspendLayout();
      try
      {
        // Rebuild side bar.
        sideTopTabControl.TabPages.Clear();
        DockSideDocument(modelObjectDescriptionForm, true, true);

        sideBottomTabControl.TabPages.Clear();
        DockSideDocument(userDatatypesForm, false, true);
        DockSideDocument(historyForm, false, false);

        contentSplitContainer.Panel2Collapsed = true;
        contentSplitContainer.SplitterWidth = 6; 
        contentSplitContainer.Panel2MinSize = 370;
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
        // value for panel2. The document's min height will kick in and does the right job.
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

    /// <summary>
    /// Determines if this document can be closed, i.e. all sub documents are queried for closing
    /// as well. Returns false if this form cannot be closed.
    /// </summary>
    /// <returns></returns>
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
        if (document is IWorkbenchDocument)
          (document as IWorkbenchDocument).CloseDocument();
      BackendForm.close();
    }

    private void ShowUserDatatypes()
    {
      mainSplitContainer.Panel2Collapsed = false;
      userDatatypesForm.Activate();
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
      int sidebarWidth = wbContext.read_state("sidebar_width", "model_overview", 200);
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

      NodeIdWrapper nodeId = wbOverview.get_root();
      String caption;
      wbOverview.get_field(nodeId, (int)Overview.Columns.Label, out caption);
      String domain = caption + "-overview";
      foreach (CollapsingPanel panel in panelList)
      {
        panel.Expanded = wbContext.read_state(panel.HeaderCaption + "-expanded", domain, panel.Expanded);
        View viewMode = (View)wbContext.read_state(panel.HeaderCaption + "-mode", domain, (int)GetViewMode(panel));
        SetViewMode(panel, viewMode);

        switch (viewMode)
        {
          case View.LargeIcon:
            panel.ActiveCustomButton = 0;
            break;
          case View.Tile:
            panel.ActiveCustomButton = 1;
            break;
          case View.Details:
            panel.ActiveCustomButton = 2;
            break;
        }
      }
    }

    private void SaveFormState()
    {
      NodeIdWrapper nodeId = wbOverview.get_root();
      String caption;
      wbOverview.get_field(nodeId, (int)Overview.Columns.Label, out caption);
      String domain = caption + "-overview";
      foreach (CollapsingPanel panel in panelList)
      {
        wbContext.save_state(panel.HeaderCaption + "-expanded", domain, panel.Expanded);
        wbContext.save_state(panel.HeaderCaption + "-mode", domain, (int)GetViewMode(panel));
      }
    }

    private void mainSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (splitterMovePending)
      {
        splitterMovePending = false;
        wbContext.save_state("sidebar_width", "model_overview", mainSplitContainer.SplitterDistance);
      }
    }

    private void mainContentSplitContainer_SplitterMoved(object sender, SplitterEventArgs e)
    {
      if (splitterMovePending)
      {
        splitterMovePending = false;
        wbContext.save_state("secondary_sidebar_width", "model_overview", mainSplitContainer.SplitterDistance);
      }
    }

    private void SplitterMoving(object sender, SplitterCancelEventArgs e)
    {
      // This event only comes up when the splitter is moved with the mouse (i.e. by the user).
      // We can use it to differentiate between user initiated and programmatic splitter changes.
      splitterMovePending = true;
    }

    private void ModelOverviewForm_FormClosing(object sender, FormClosingEventArgs e)
    {
      SaveFormState();
    }

    /// <summary>
    /// Keeps an association between a node (id) and a listview for later quick access.
    /// Frees any resource links to the backend if a listview is removed.
    /// </summary>
    /// <param name="id"></param>
    private void cacheListview(String id, ListView listview)
    {
      if (listsByNode.ContainsKey(id))
      {
        if (listsByNode[id] != listview)
        {
          listsByNode[id].SmallImageList = null;
          listsByNode[id].LargeImageList = null;
          listsByNode[id].Dispose();
          listsByNode[id] = listview;
        }
      }
      else
        listsByNode[id] = listview;
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
