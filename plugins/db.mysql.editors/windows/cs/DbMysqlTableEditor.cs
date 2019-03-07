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
using System.Collections.Generic;
using System.Windows.Forms;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

using MySQL.Forms;
using MySQL.Grt;
using MySQL.Grt.Db;
using MySQL.GUI.Workbench.Plugins.Properties;
using MySQL.Utilities;

namespace MySQL.GUI.Workbench.Plugins
{
  // TODO: remove all stored NodeIds . This is just nonsense.
  //       NodeIds should be created on demand (they are temporary anyway) from tree node indices.
  public partial class DbMysqlTableEditor : ObjectEditorPlugin, IModelChangeListener
  {
    #region Member Variables

    private MySQLTableEditorWrapper tableEditorBE { get { return Backend as MySQLTableEditorWrapper; } } 
    private bool dragInProgress = false;

    private DbMysqlTableColumnsListModel columnsListModel;
    private DbMysqlTableIndicesListModel indicesListModel;
    private DbMysqlTableIndexColumnsListModel indexColumnsListModel;
    private DbMysqlTableFkListModel fkListModel;
    private DbMysqlTableFkColumnListModel fkColumnsListModel;

    private DbObjectEditorPages dbObjectEditorPages = null;

    private SimpleGrtTreeModel partitionTreeModel;
    private MySQLTablePartitionTreeWrapper partitionTreeBE;

    private bool headingCollapsed = false;
    private bool privPageWasActive = false; // Set when doing re-init.
    private bool updatingTable = false;
    private bool updatingFkValues = false;

    #endregion

    #region Constructors

    public DbMysqlTableEditor()
        : base()
    {
    }

    public DbMysqlTableEditor(GrtManager manager, GrtValue value)
      : base(manager)
    {
      InitializeComponent();
      ReinitWithArguments(value);

      if (IsEditingLiveObject)
        AdjustEditModeControls(mainTabControl);
      else
        collapsePictureBox_Click(null, null); // Start with a collapsed header section.
      //  AdjustToSmallerLayout(); TODO: can go

      topPanel.Parent = mainTabControl.Parent;
    }

    #endregion

    #region ObjectEditorPlugin Overrides

    public override bool ReinitWithArguments(GrtValue value)
    {
      if (Backend != null && !Backend.can_close())
        return false;  // Will open the plugin in a new editor window instead.

      InitializingControls = true;
      SuspendLayout();

      try
      {
        // We have to remove the privileges tab here otherwise we leave it with a
        // dangling backend reference, which later crashes.
        // This will certainly flicker but due to the design (priv tab in a different form)
        // we have no other choice.
        if (dbObjectEditorPages != null)
        {
          privPageWasActive = mainTabControl.SelectedTab == dbObjectEditorPages.PrivilegesTabPage;
          mainTabControl.TabPages.Remove(dbObjectEditorPages.PrivilegesTabPage);
        }

        if (insertsTabPage.Controls.Count > 0)
          insertsTabPage.Controls.Clear();

        Backend = new MySQLTableEditorWrapper(value);

        Control panel = tableEditorBE.get_trigger_panel();
        triggersTabPage.Controls.Add(panel);
        panel.Dock = DockStyle.Fill;

        dbObjectEditorPages = new DbObjectEditorPages(GrtManager, tableEditorBE);

        InitFormData();
        RefreshFormData();

        {
          panel = tableEditorBE.get_inserts_panel();
          insertsTabPage.Controls.Add(panel);
          panel.Parent = insertsTabPage;
          panel.Dock = DockStyle.Fill;
        }
        Backend.reset_editor_undo_stack();
      }
      finally
      {
        ResumeLayout(true);

        InitializingControls = false;
      }

      return true;
    }

    #endregion

    #region Form implemenation

    protected void InitFormData()
    {
      int current_page = mainTabControl.SelectedIndex;

      schemaLabel.Text = tableEditorBE.get_schema_name();

      // Init Columns TreeView
      columnsTreeView.SelectedNode = null;
      if (columnsTreeView.Model != null && columnsTreeView.Model is DbMysqlTableColumnsListModel)
        (columnsTreeView.Model as DbMysqlTableColumnsListModel).DetachEvents();

      columnsListModel = new DbMysqlTableColumnsListModel(this, columnsTreeView, tableEditorBE.get_columns(),
        columnIconNodeControl, nameNodeControl, datatypeComboBoxNodeControl, pkNodeControl,
        nnNodeControl, uqNodeControl, binNodeControl, unNodeControl, zfNodeControl, aiNodeControl,
        gNodeControl, defaultNodeControl, tableEditorBE);
      
      nameNodeControl.IsEditEnabledValueNeeded += new EventHandler<Aga.Controls.Tree.NodeControls.NodeControlValueEventArgs>(canEdit);
      datatypeComboBoxNodeControl.IsEditEnabledValueNeeded += new EventHandler<Aga.Controls.Tree.NodeControls.NodeControlValueEventArgs>(canEdit);
      defaultNodeControl.IsEditEnabledValueNeeded += new EventHandler<Aga.Controls.Tree.NodeControls.NodeControlValueEventArgs>(canEdit);

      columnCommentTextBox.Text = "";
      columnCommentTextBox.Enabled = false;

      columnsTreeView.Model = columnsListModel;
      columnsTreeView.Refresh();

      // Init Index TreeView
      indicesTreeView.SelectedNode = null;
      if (indicesTreeView.Model != null && indicesTreeView.Model is DbMysqlTableIndicesListModel)
        (indicesTreeView.Model as DbMysqlTableIndicesListModel).DetachEvents();

      indicesListModel = new DbMysqlTableIndicesListModel(indicesTreeView, tableEditorBE.get_indexes(),
        indexNameNodeControl, indexTypeNodeControl, indexColumnNameNodeControl, tableEditorBE);
      indicesTreeView.Model = indicesListModel;
      indicesTreeView.Refresh();

      // Init FK TreeView
      fkTreeView.SelectedNode = null;
      if (fkTreeView.Model != null && fkTreeView.Model is DbMysqlTableFkListModel)
        (fkTreeView.Model as DbMysqlTableFkListModel).DetachEvents();

      fkListModel = new DbMysqlTableFkListModel(fkTreeView, tableEditorBE.get_fks(),
        nameFkNodeControl, targetFkNodeControl, tableEditorBE, columnEnabledFkNodeControl);
      fkTreeView.Model = fkListModel;
      fkTreeView.Refresh();

      // fk actions
      onDeleteActionComboBox.Enabled = true;
      onDeleteActionComboBox.Items.Clear();
      onUpdateActionComboBox.Enabled = true;
      onUpdateActionComboBox.Items.Clear();

      onDeleteActionComboBox.BeginUpdate();
      onUpdateActionComboBox.BeginUpdate();

      try
      {
        System.Collections.Generic.List<String> fk_action_options_list = tableEditorBE.get_fk_action_options();
        foreach (String fk_action in fk_action_options_list)
        {
          onDeleteActionComboBox.Items.Add(fk_action);
          onUpdateActionComboBox.Items.Add(fk_action);
        }
      }
      finally
      {
        onDeleteActionComboBox.EndUpdate();
        onUpdateActionComboBox.EndUpdate();
      }
      // index storage types
      indexStorageTypeComboBox.Enabled = true;
      indexStorageTypeComboBox.Items.Clear();
      System.Collections.Generic.List<String> index_storage_types_list = tableEditorBE.get_index_storage_types();
      foreach (String storage_type in index_storage_types_list)
      {
        indexStorageTypeComboBox.Items.Add(storage_type);
      }

      // engines
      optEngine.Enabled = true;
      optEngine.Items.Clear();
      System.Collections.Generic.List<String> engines_list = tableEditorBE.get_engines_list();
      foreach (String engine in engines_list)
      {
        optEngine.Items.Add(engine);
      }

      System.Collections.Generic.List<String> collations_list = tableEditorBE.get_charset_collation_list();

      var charsetLlist = tableEditorBE.get_charset_list();
      optCharset.Items.Clear();
      optCharset.Items.AddRange(charsetLlist.ToArray());

      optCollation.Items.Clear();
      optCollation.Items.Add("Default Collation");

      columnCollationComboBox.Enabled = false;
      columnCollationComboBox.Items.Clear();
      columnCollationComboBox.Items.Add("Default Collation");

      columnCharsetComboBox.Enabled = false;
      columnCharsetComboBox.Items.Clear();
      columnCharsetComboBox.Items.AddRange(charsetLlist.ToArray());


      // This validation was added to avoid removing the page if not needed
      // as it causes flickering
      if (mainTabControl.TabPages.Contains(insertsTabPage))
      {
        if (IsEditingLiveObject)
          mainTabControl.TabPages.Remove(insertsTabPage);
      }
      else
      {
        if (!IsEditingLiveObject)
          mainTabControl.TabPages.Add(insertsTabPage);
      }

      // We always have to remove the priv tab because the owning form + backend are recreated.
      // This means we only need to add it here if we are not editing a live object.
      // Note: we cannot reuse the priv tab page because its owning form + backend are gone
      //       when re-initializing the table editor for a new object.
      if (!IsEditingLiveObject)
      {
        mainTabControl.TabPages.Add(dbObjectEditorPages.PrivilegesTabPage);
        if (privPageWasActive)
          mainTabControl.SelectedTab = dbObjectEditorPages.PrivilegesTabPage;
      }

      // Partitioning stuff
      if (partitionTreeView.Model != null && partitionTreeView.Model is SimpleGrtTreeModel)
        (partitionTreeView.Model as SimpleGrtTreeModel).DetachEvents();

      partitionTreeBE = tableEditorBE.get_partitions();
      partitionTreeModel = new SimpleGrtTreeModel(partitionTreeView, partitionTreeBE, partNodeStateIcon, false);
      partitionTreeModel.AddColumn(partNameNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.Name, true);
      partitionTreeModel.AddColumn(partValuesNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.Value, true);
      partitionTreeModel.AddColumn(partDataDirNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.DataDirectory, true);
      partitionTreeModel.AddColumn(partIndexDirNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.IndexDirectory, true);
      partitionTreeModel.AddColumn(partMaxRowsNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.MaxRows, true);
      partitionTreeModel.AddColumn(partMinRowsNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.MinRows, true);
      partitionTreeModel.AddColumn(partCommentNodeControl, (int)MySQLTablePartitionTreeWrapper.Columns.Comment, true);
      partitionTreeView.Model = partitionTreeModel;
      partitionTreeView.Refresh();

      tableEditorBE.load_trigger_sql();
    }

    /// <summary>
    /// The current layout is too big for the small room we have for the table editor
    /// in the modeling section, so we switch to a smaller layout that uses less vertical space.
    /// </summary>
    void AdjustToSmallerLayout()
    {
      // Start with a collapsed header section.
      collapsePictureBox_Click(null, null);

      // Make the right part of the columns split container visible and dock collation and
      // comment controls to it.
      columnListSplitContainer.Panel2Collapsed = false;
      label9.Parent = oldTableLayoutPanel;
      oldTableLayoutPanel.SetCellPosition(label9, new TableLayoutPanelCellPosition(0, 0));
      columnCollationComboBox.Parent = oldTableLayoutPanel;
      oldTableLayoutPanel.SetCellPosition(columnCollationComboBox, new TableLayoutPanelCellPosition(1, 0));
      label12.Parent = oldTableLayoutPanel;
      label12.Anchor = AnchorStyles.Left | AnchorStyles.Top | AnchorStyles.Right;
      oldTableLayoutPanel.SetCellPosition(label12, new TableLayoutPanelCellPosition(0, 1));
      columnCommentTextBox.Parent = oldTableLayoutPanel;
      oldTableLayoutPanel.SetCellPosition(columnCommentTextBox, new TableLayoutPanelCellPosition(1, 1));
      columnCommentTextBox.AcceptsReturn = true;

      // Finally hide the entire lower part (column details).
      tableLayoutPanel1.Hide();
    }

    void canEdit(object sender, Aga.Controls.Tree.NodeControls.NodeControlValueEventArgs e)
    {
      if (dragInProgress)
        e.Value = false;
    }

    protected override void RefreshPartialFormData(int what)
    {
      base.RefreshPartialFormData(what);

      switch ((TableEditorWrapper.PartialRefreshes)what)
      {
        case TableEditorWrapper.PartialRefreshes.RefreshColumnList:
          columnsListModel.RefreshModel();
          break;

        case TableEditorWrapper.PartialRefreshes.RefreshColumnCollation:
          columnsTreeView_SelectionChanged(columnsTreeView, null);
          break;

        case TableEditorWrapper.PartialRefreshes.RefreshColumnMoveUp:
          {
            int index = columnsListModel.TreeView.SelectedNode.Index;
            columnsListModel.RefreshModel();
            columnsListModel.TreeView.SelectedNode = columnsListModel.TreeView.Root.Children[index - 1];
            break;
          }

        case TableEditorWrapper.PartialRefreshes.RefreshColumnMoveDown:
          {
            int index = columnsListModel.TreeView.SelectedNode.Index;
            columnsListModel.RefreshModel();
            columnsListModel.TreeView.SelectedNode = columnsListModel.TreeView.Root.Children[index + 1];
            break;
          }
      }
    }

    protected override void RefreshFormData()
    {
      if (updatingTable)
        return;

      updatingTable = true;
      try
      {
        base.RefreshFormData();

        nameTextBox.Text = tableEditorBE.get_name();
        optComments.Text = tableEditorBE.get_comment();

        switch (tableEditorBE.get_table_option_by_name("PACK_KEYS"))
        {
          case "DEFAULT":
            optPackKeys.SelectedIndex = 1;
            break;
          case "0":
            optPackKeys.SelectedIndex = 2;
            break;
          case "1":
            optPackKeys.SelectedIndex = 3;
            break;
          default:
            optPackKeys.SelectedIndex = 0;
            break;
        }

        optTablePassword.Text = tableEditorBE.get_table_option_by_name("PASSWORD");
        optAutoIncrement.Text = tableEditorBE.get_table_option_by_name("AUTO_INCREMENT");

        switch (tableEditorBE.get_table_option_by_name("DELAY_KEY_WRITE"))
        {
          case "0":
            optDelayKeyUpdates.Checked = false;
            break;
          case "1":
            optDelayKeyUpdates.Checked = true;
            break;
        }

        switch (tableEditorBE.get_table_option_by_name("ROW_FORMAT"))
        {
          case "DEFAULT":
            optRowFormat.SelectedIndex = 1;
            break;
          case "DYNAMIC":
            optRowFormat.SelectedIndex = 2;
            break;
          case "FIXED":
            optRowFormat.SelectedIndex = 3;
            break;
          case "COMPRESSED":
            optRowFormat.SelectedIndex = 4;
            break;
          case "REDUNDANT":
            optRowFormat.SelectedIndex = 5;
            break;
          case "COMPACT":
            optRowFormat.SelectedIndex = 6;
            break;
          default:
            optRowFormat.SelectedIndex = 0;
            break;
        }

        switch (tableEditorBE.get_table_option_by_name("KEY_BLOCK_SIZE"))
        {
          case "1":
            optKeyBlockSize.SelectedIndex = 1;
            break;
          case "2":
            optKeyBlockSize.SelectedIndex = 2;
            break;
          case "4":
            optKeyBlockSize.SelectedIndex = 3;
            break;
          case "8":
            optKeyBlockSize.SelectedIndex = 4;
            break;
          case "16":
            optKeyBlockSize.SelectedIndex = 5;
            break;
          default:
            optKeyBlockSize.SelectedIndex = 0;
            break;
        }

        optAvgRowLength.Text = tableEditorBE.get_table_option_by_name("AVG_ROW_LENGTH");
        optMaxRows.Text = tableEditorBE.get_table_option_by_name("MAX_ROWS");
        optMinRows.Text = tableEditorBE.get_table_option_by_name("MIN_ROWS");

        switch (tableEditorBE.get_table_option_by_name("CHECKSUM"))
        {
          case "0":
            optUseChecksum.Checked = false;
            break;
          case "1":
            optUseChecksum.Checked = true;
            break;
        }

        optDataDirectory.Text = tableEditorBE.get_table_option_by_name("DATA DIRECTORY");
        optIndexDirectory.Text = tableEditorBE.get_table_option_by_name("INDEX DIRECTORY");
        optUnionTables.Text = tableEditorBE.get_table_option_by_name("UNION");

        switch (tableEditorBE.get_table_option_by_name("INSERT_METHOD"))
        {
          case "FIRST":
            optMergeMethod.SelectedIndex = 1;
            break;
          case "LAST":
            optMergeMethod.SelectedIndex = 2;
            break;
          default:
            optMergeMethod.SelectedIndex = 0;
            break;
        }

        String eng = tableEditorBE.get_table_option_by_name("ENGINE");
        if (eng == String.Empty)
          optEngine.SelectedIndex = 0;
        else
          optEngine.Text = eng;


        var charset = tableEditorBE.get_table_option_by_name("CHARACTER SET");
        var idx = optCharset.FindString(charset);
        if (idx < 0)
          idx = 0;
        optCharset.SelectedIndex = idx;

        var selectedValue = "Default Charset";
        if (optCharset.SelectedItem != null)
          selectedValue = optCharset.SelectedItem.ToString();
        var collation = tableEditorBE.get_charset_collation_list(selectedValue);
        optCollation.Items.Clear();
        optCollation.Items.AddRange(collation.ToArray());

        var collate = tableEditorBE.get_table_option_by_name("COLLATE");
        idx = optCollation.FindString(charset);
        if (idx < 0)
          idx = 0;
        optCollation.SelectedIndex = idx;

        TabText = tableEditorBE.get_title();

        columnsListModel.RefreshModel();
        indicesListModel.RefreshModel();
        fkListModel.RefreshModel();
        tableEditorBE.load_trigger_sql();

        // partitioning
        if (tableEditorBE.get_partition_type() == null || tableEditorBE.get_partition_type() == "")
          partEnable.Checked = false;
        else
        {
          partEnable.Checked = true;
          partFunction.SelectedItem = tableEditorBE.get_partition_type();

          partParams.Text = tableEditorBE.get_partition_expression();
          partManual.Checked = tableEditorBE.get_explicit_partitions();
          partCount.Text = tableEditorBE.get_partition_count().ToString();

          subpartFunction.SelectedItem = tableEditorBE.get_subpartition_type();

          subpartParams.Text = tableEditorBE.get_subpartition_expression();
          subpartManual.Checked = tableEditorBE.get_explicit_subpartitions();
          subpartCount.Text = tableEditorBE.get_subpartition_count().ToString();
        }

        refreshPartitioningList();

        columnsTreeView_SelectionChanged(null, null);
      }
      finally
      {
        updatingTable = false;
      }
    }

    private void nameTextBox_KeyPress(object sender, KeyPressEventArgs e)
    {
      if (e.KeyChar == '\r')
      {
        if (!nameTextBox.Text.Equals(tableEditorBE.get_name()))
        {
          tableEditorBE.set_name(nameTextBox.Text);
          TabText = tableEditorBE.get_title();
        }

        e.Handled = true;
      }
    }

    private void nameTextBox_Leave(object sender, EventArgs e)
    {
      if (!nameTextBox.Text.Equals(tableEditorBE.get_name()))
      {
        tableEditorBE.set_name(nameTextBox.Text);
        TabText = tableEditorBE.get_title();
      }
    }

    private void commentsTextBox_TextChanged(object sender, EventArgs e)
    {
      if (!InitializingControls && !optComments.Text.Equals(tableEditorBE.get_comment()))
        tableEditorBE.set_comment(optComments.Text);
    }

    private void DbMysqlTableEditor_Load(object sender, EventArgs e)
    {
      ActiveControl = nameTextBox;
    }

    private void mainTabControl_SelectedIndexChanged(object sender, EventArgs e)
    {
      if (mainTabControl.SelectedTab == indicesTabPage)
      {
        indicesListModel.RefreshModel();

        // if the index tab is selected, auto-edit an index
        if (indicesTreeView.Root.Children.Count >= 1)
        {
          ActiveControl = indicesTreeView;
          indicesTreeView.SelectedNode = indicesTreeView.Root.Children[indicesTreeView.Root.Children.Count - 1];
          indexNameNodeControl.BeginEdit();
        }
      }
      else if (mainTabControl.SelectedTab == foreignKeysTabPage)
      {
        fkListModel.RefreshModel();

        if (optEngine.SelectedIndex == 0 || // Index 0 is the server default.
          tableEditorBE.engine_supports_foreign_keys())
        {
          foreignKeyWarningPanel.Visible = false;

          // if the fk tab is selected, auto-edit a fk
          if (fkTreeView.Root.Children.Count >= 1)
          {
            ActiveControl = fkTreeView;
            fkTreeView.SelectedNode = fkTreeView.Root.Children[fkTreeView.Root.Children.Count - 1];
            nameFkNodeControl.BeginEdit();
          }
        }
        else
          foreignKeyWarningPanel.Visible = true;
      }
    }

    private void mainTabControl_PreviewKeyDown(object sender, PreviewKeyDownEventArgs e)
    {
      if (e.Control)
      {
        if (e.KeyCode == Keys.Left)
        {
          if (mainTabControl.SelectedIndex < mainTabControl.TabCount - 1)
            mainTabControl.SelectedIndex++;
        }
        else if (e.KeyCode == Keys.Right)
        {
          if (mainTabControl.SelectedIndex > 0)
            mainTabControl.SelectedIndex--;
        }
      }
    }

    #endregion

    #region Columns


    private void columnsTreeView_NodeMouseDoubleClick(object sender, TreeNodeAdvMouseEventArgs e)
    {
      if (e.Node != null && e.Node.Tag != null)
      {
        GrtListNode node = e.Node.Tag as GrtListNode;

        if (node != null && e.Control == columnIconNodeControl)
        {
          int isPk;

          columnsListModel.GrtList.get_field(node.NodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK, out isPk);

          columnsListModel.GrtList.set_field(node.NodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK, (isPk + 1) % 2);
        }
        else if (e.Control is AdvNodeTextBox)
        {
          AdvNodeTextBox tbox = e.Control as AdvNodeTextBox;
          tbox.BeginEdit();
        }
        else if (e.Control is AdvNodeComboBox)
        {
          AdvNodeComboBox tbox = e.Control as AdvNodeComboBox;
          tbox.BeginEdit();
        }
      }
    }

    private void columnsTreeView_SelectionChanged(object sender, EventArgs e)
    {
      if (InitializingControls)
        return;

      if (columnsTreeView.SelectedNode != null)
      {
        NodeIdWrapper nodeId = (columnsTreeView.SelectedNode.Tag as GrtListNode).NodeId;

        string stringValue;
        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Comment, out stringValue))
        {
          columnCommentTextBox.Text = stringValue;
          columnCommentTextBox.Enabled = true;
        }
        else
        {
          columnCommentTextBox.Text = "";
          columnCommentTextBox.Enabled = false;
        }

        // charset/collation
        String hasCharset = null;
        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.HasCharset, out hasCharset) &&
          (hasCharset == "1"))
        {

          var charset = "";
          columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Charset, out charset);

          columnCharsetComboBox.Enabled = true;
          columnCollationComboBox.Enabled = true;
          var idx = columnCharsetComboBox.FindString(charset);
          if (idx < 0)
            idx = 0;
          columnCharsetComboBox.SelectedIndex = idx;
        }
        else
        {
          columnCharsetComboBox.SelectedIndex = 0;
          columnCharsetComboBox.Enabled = false;
          columnCollationComboBox.SelectedIndex = 0;
          columnCollationComboBox.Enabled = false;

        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name, out stringValue))
        {
          columnNameTextBox.Text = stringValue;
          columnNameTextBox.Enabled = true;
        }
        else
        {
          columnNameTextBox.Text = "";
          columnNameTextBox.Enabled = false;
        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Type, out stringValue))
        {
          columnDataTypeTextBox.Text = stringValue;
          columnDataTypeTextBox.Enabled = true;
        }
        else
        {
          columnDataTypeTextBox.Text = "";
          columnDataTypeTextBox.Enabled = false;
        }

        // Until this is refactored we use the default text field also for the expression
        // in generated columns.
        int isGenerated;
        bool enableGenerated = columnsListModel.GrtList.get_field(nodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsGenerated, out isGenerated);
        bool enableDefault = false;

        if (isGenerated == 0)
        {
          defaultLabel.Text = "Default:";
          if (columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Default, out stringValue))
          {
            columnDefaultTextBox.Text = stringValue;
            enableDefault = true;
          }
          else
            columnDefaultTextBox.Text = "";
        }

        int flag;
        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK, out flag))
        {
          pkCheckBox.Checked = flag != 0;
          pkCheckBox.Enabled = true;
        }
        else
        {
          pkCheckBox.Checked = false;
          pkCheckBox.Enabled = false;
        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsNotNull, out flag))
        {
          nnCheckBox.Checked = flag != 0;
          nnCheckBox.Enabled = true;
        }
        else
        {
          nnCheckBox.Checked = false;
          nnCheckBox.Enabled = false;
        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnique, out flag))
        {
          uniqueCheckBox.Checked = flag != 0;
          uniqueCheckBox.Enabled = true;
        }
        else
        {
          uniqueCheckBox.Checked = false;
          uniqueCheckBox.Enabled = false;
        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsBinary, out flag))
        {
          binaryCheckBox.Checked = flag != 0;
          binaryCheckBox.Enabled = true;
        }
        else
        {
          binaryCheckBox.Checked = false;
          binaryCheckBox.Enabled = false;
        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnsigned, out flag))
        {
          unsignedCheckBox.Checked = flag != 0;
          unsignedCheckBox.Enabled = true;
        }
        else
        {
          unsignedCheckBox.Checked = false;
          unsignedCheckBox.Enabled = false;
        }

        if (columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsZerofill, out flag))
        {
          zeroFillCheckBox.Checked = flag != 0;
          zeroFillCheckBox.Enabled = true;
        }
        else
        {
          zeroFillCheckBox.Checked = false;
          zeroFillCheckBox.Enabled = false;
        }

        int canAutoIncrement;
        if (!columnsListModel.GrtList.get_field(nodeId,
          (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsAutoIncrement, out canAutoIncrement))
          canAutoIncrement = 0;

        if (canAutoIncrement != 0)
        {
          if (columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsAutoIncrement, out flag))
          {
            aiCheckBox.Checked = flag != 0;
            aiCheckBox.Enabled = true;
          }
          else
          {
            aiCheckBox.Checked = false;
            aiCheckBox.Enabled = false;
          }
        }
        else
        {
          aiCheckBox.Checked = false;
          aiCheckBox.Enabled = false;
        }

        generatedCheckbox.Enabled = enableGenerated;
        virtualRadioButton.Enabled = enableGenerated && (isGenerated != 0);
        storedRadioButton.Enabled = enableGenerated && (isGenerated != 0);
        columnDefaultTextBox.Enabled = enableDefault || enableGenerated;
        generatedCheckbox.Checked = isGenerated != 0;
        if (isGenerated != 0)
        {
          defaultLabel.Text = "Expression:";
          
          if (columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.GeneratedStorageType, out stringValue))
          {
            if (stringValue.ToLower() == "stored")
              storedRadioButton.Checked = true;
            else
              virtualRadioButton.Checked = true; // Auto switches off the other button.
          }
          else
          {
            virtualRadioButton.Checked = false;
            virtualRadioButton.Enabled = false;
            storedRadioButton.Checked = false;
            storedRadioButton.Enabled = false;
          }

          if (columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.GeneratedExpression, out stringValue))
          {
            columnDefaultTextBox.Text = stringValue;
          }
          else
            columnDefaultTextBox.Text = "";
        }
      }
      else
      {
        columnCommentTextBox.Enabled = false;
        columnCommentTextBox.Text = "";
        columnCollationComboBox.Enabled = false;
        columnCharsetComboBox.Enabled = false;
        columnNameTextBox.Text = "";
        columnNameTextBox.Enabled = false;
        columnDataTypeTextBox.Text = "";
        columnDataTypeTextBox.Enabled = false;
        columnDefaultTextBox.Text = "";
        columnDefaultTextBox.Enabled = false;
        pkCheckBox.Checked = false;
        pkCheckBox.Enabled = false;
        nnCheckBox.Checked = false;
        nnCheckBox.Enabled = false;
        uniqueCheckBox.Checked = false;
        uniqueCheckBox.Enabled = false;
        binaryCheckBox.Checked = false;
        binaryCheckBox.Enabled = false;
        unsignedCheckBox.Checked = false;
        unsignedCheckBox.Enabled = false;
        zeroFillCheckBox.Checked = false;
        zeroFillCheckBox.Enabled = false;
        aiCheckBox.Checked = false;
        aiCheckBox.Enabled = false;

        generatedCheckbox.Checked = false;
        generatedCheckbox.Enabled = false;
        virtualRadioButton.Checked = false;
        virtualRadioButton.Enabled = false;
        storedRadioButton.Checked = false;
        storedRadioButton.Enabled = false;
      }
    }

    private void columnCollationComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      // set charset/collation
      if (columnsTreeView.SelectedNode != null)
      {
        NodeIdWrapper nodeId = new NodeIdWrapper(columnsTreeView.SelectedNode.Index);
        if (columnCollationComboBox.SelectedIndex == 0)
        {
          columnsListModel.GrtList.set_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Collation, "");
        }
        else
        {
          columnsListModel.GrtList.set_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Collation,
              columnCollationComboBox.Text);
        }
      }
    }

    private void columnCharsetComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      // set charset
      if (columnsTreeView.SelectedNode != null)
      {
        NodeIdWrapper nodeId = new NodeIdWrapper(columnsTreeView.SelectedNode.Index);
        if (columnCharsetComboBox.SelectedIndex == 0)
        {
          columnsListModel.GrtList.set_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Charset, "");
          columnCollationComboBox.Enabled = false;
          columnCollationComboBox.SelectedIndex = 0;
        }
        else
        {
          columnsListModel.GrtList.set_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Charset,
              columnCharsetComboBox.Text);
          var collation = tableEditorBE.get_charset_collation_list(columnCharsetComboBox.Text);
          columnCollationComboBox.Items.Clear();
          columnCollationComboBox.Items.AddRange(collation.ToArray());
          var collationItem = "";
          columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Collation, out collationItem);
          int idx = columnCollationComboBox.FindString(collationItem);
          if (idx < 0) {
            columnsListModel.GrtList.set_field(nodeId,
              (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Collation, "");
            idx = 0;
          }
          columnCollationComboBox.Enabled = true;
          columnCollationComboBox.SelectedIndex = idx;
        }
      }
    }

    private void updateColumnTextField(MySQLTableColumnsListWrapper.MySQLColumnListColumns column, TextBox box)
    {
      if (columnsTreeView.SelectedNode != null)
      {
        // Change an attribute triggers a whole chain of events which ultimately cause the
        // table editor to refresh completely (including all nodes in the columns tree etc.).
        // So we have to take measure to restore what was active when the user changed the value.
        NodeIdWrapper nodeId = new NodeIdWrapper(columnsTreeView.SelectedNode.Index);
        if (!columnsListModel.GrtList.set_field(nodeId, (int)column, box.Text))
        {
          CustomMessageBox.Show(MessageType.MessageError, "Could not set new value",
            "The entered value contains errors and cannot be accepted. The previous value is kept instead.",
            "Close");

          string originalText;
          columnsListModel.GrtList.get_field(nodeId, (int)column, out originalText);
          box.Text = originalText;
        }

        columnsTreeView.SelectedNode = columnsTreeView.Root.Children[nodeId.get_by_index(0)];
      }
    }

    private void applyColumnText(TextBox box)
    {
      MySQLTableColumnsListWrapper.MySQLColumnListColumns column = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK;
      switch (box.Tag.ToString())
      {
        case "0":
          column = MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name;
          break;
        case "1":
          column = MySQLTableColumnsListWrapper.MySQLColumnListColumns.Comment;
          break;
        case "2":
          column = MySQLTableColumnsListWrapper.MySQLColumnListColumns.Type;
          break;
        case "3":
          column = MySQLTableColumnsListWrapper.MySQLColumnListColumns.Default;
          break;
      }
      if (column != MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK)
      {
        updateColumnTextField(column, box);
        box.SelectAll();
      }
    }

    private void columnTextBox_Leave(object sender, EventArgs e)
    {
      if (columnsTreeView.SelectedNode != null && !IsDisposed)
        applyColumnText(sender as TextBox);
    }

    private void columnFlagsChanged(object sender, EventArgs e)
    {
      if (columnsTreeView.SelectedNode == null)
        return;

      NodeIdWrapper nodeId = new NodeIdWrapper(columnsTreeView.SelectedNode.Index);
      CheckBox box = sender as CheckBox;
      int value = box.Checked ? 1 : 0;
      MySQLTableColumnsListWrapper.MySQLColumnListColumns columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name;
      switch (box.Tag.ToString())
      {
        case "0":
          columnValue =  MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsPK;
          break;
        case "1":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsNotNull;
          break;
        case "2":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnique;
          break;
        case "3":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsBinary;
          break;
        case "4":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsUnsigned;
          break;
        case "5":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsZerofill;
          break;
        case "6":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsAutoIncrement;
          break;
        case "7":
          columnValue = MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsGenerated;
          break;
      }

      if (columnValue != MySQLTableColumnsListWrapper.MySQLColumnListColumns.Name)
      {
        if (!columnsListModel.GrtList.set_field(nodeId, (int)columnValue, value))
        {
          // Value not accepted. Restore previous one.
          columnsListModel.GrtList.get_field(nodeId, (int)columnValue, out value);
          box.Checked = value != 0;
        }
      }

      if (columnValue == MySQLTableColumnsListWrapper.MySQLColumnListColumns.IsGenerated)
      {
        virtualRadioButton.Enabled = box.Checked;
        storedRadioButton.Enabled = box.Checked;
        String stringValue;
        if (box.Checked)
        {
          if (!virtualRadioButton.Checked && !storedRadioButton.Checked)
            virtualRadioButton.Checked = true; // Default value.

          defaultLabel.Text = "Expression:";
          if (columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.GeneratedExpression, out stringValue))
          {
            columnDefaultTextBox.Text = stringValue;
          }
          else
            columnDefaultTextBox.Text = "";
        }
        else
        {
          defaultLabel.Text = "Default:";
          if (columnsListModel.GrtList.get_field(nodeId,
            (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.Default, out stringValue))
          {
            columnDefaultTextBox.Text = stringValue;
          }
          else
            columnDefaultTextBox.Text = "";
        }
      } 
      
      columnsListModel.RefreshModel();
    }

    private void storageRadioButton_CheckedChanged(object sender, EventArgs e)
    {
      if (columnsTreeView.SelectedNode == null)
        return;

      NodeIdWrapper nodeId = new NodeIdWrapper(columnsTreeView.SelectedNode.Index);
      RadioButton button = sender as RadioButton;
      if (button.Checked)
      {
        columnsListModel.GrtList.set_field(nodeId, (int)MySQLTableColumnsListWrapper.MySQLColumnListColumns.GeneratedStorageType,
          sender == virtualRadioButton ? "VIRTUAL" : "STORED");
      }
    }

    #endregion

    #region Index

    private void indicesTreeView_SelectionChanged(object sender, EventArgs e)
    {
      // remove old virtual value events
      if (indexColumnsTreeView.Model != null)
        indexColumnsListModel.DetachEvents();

      // if a new node was selected, create an inspector for it
      if (indicesTreeView.SelectedNode != null && indicesTreeView.SelectedNode.NextNode != null)
      {
        GrtListNode node = indicesTreeView.SelectedNode.Tag as GrtListNode;

        if (node != null)
        {
          indexCommentText.Enabled = tableEditorBE.is_server_version_at_least(5, 5, -1);
          visibleCheckBox.Enabled = false;
          indexStorageTypeComboBox.Enabled = true;
          indexRowBlockSizeText.Enabled = true;
          indexParserText.Enabled = true;

          // select the index
          tableEditorBE.get_indexes().select_index(node.NodeId);

          // create new inspector model for the selected value, pass valueNodeTextBox so virtual value events can be attached
          indexColumnsListModel = new DbMysqlTableIndexColumnsListModel(indexColumnsTreeView,
            tableEditorBE.get_indexes().get_columns(), tableEditorBE.get_columns(),
            indexColumnEnabledNodeControl,
            indexColumnNameNodeControl,
            indexColumnOrderNodeControl,
            indexColumnStorageNodeControl,
            indexColumnLengthNodeControl);

          // assign model to treeview
          indexColumnsTreeView.Model = indexColumnsListModel;

          MySQL.Grt.Db.IndexListWrapper ilist = tableEditorBE.get_indexes();

          String value = null;

          ilist.get_field(node.NodeId, (int)MySQL.Grt.Db.IndexListWrapper.IndexListColumns.Comment, out value);
          indexCommentText.Text = value;
          ilist.get_field(node.NodeId, (int)MySQLIndexListWrapper.Columns.StorageType, out value);
          indexStorageTypeComboBox.SelectedItem = value != "" ? value : null;
          ilist.get_field(node.NodeId, (int)MySQLIndexListWrapper.Columns.RowBlockSize, out value);
          indexRowBlockSizeText.Text = value;
          ilist.get_field(node.NodeId, (int)MySQLIndexListWrapper.Columns.Parser, out value);
          indexParserText.Text = value;

          if (tableEditorBE.is_server_version_at_least(8, 0, 0))
          {
            String type = "";
            ilist.get_field(node.NodeId, (int)MySQLIndexListWrapper.Columns.Type, out type);
            int visible = 1;
            if (type != "PRIMARY")
              ilist.get_field(node.NodeId, (int)MySQLIndexListWrapper.Columns.Visible, out visible);
            visibleCheckBox.Checked = visible == 1;
            if (type == "PRIMARY" || (type == "UNIQUE" && ilist.count() == 2))
              visibleCheckBox.Enabled = false;
            else
              visibleCheckBox.Enabled = true;
          }
        }
        else
        {
          indexColumnsTreeView.Model = null;
          indexCommentText.Text = "";
          indexStorageTypeComboBox.SelectedItem = null;
          indexRowBlockSizeText.Text = "";
          indexParserText.Text = "";

          indexCommentText.Enabled = false;
          indexStorageTypeComboBox.Enabled = false;
          indexRowBlockSizeText.Enabled = false;
          indexParserText.Enabled = false;
          visibleCheckBox.Enabled = false;
          visibleCheckBox.Checked = false;
        }
      }
      else
      {
        indexColumnsTreeView.Model = null;
        indexCommentText.Text = "";
        indexStorageTypeComboBox.SelectedItem = null;
        indexRowBlockSizeText.Text = "";
        indexParserText.Text = "";

        indexCommentText.Enabled = false;
        indexStorageTypeComboBox.Enabled = false;
        indexRowBlockSizeText.Enabled = false;
        indexParserText.Enabled = false;
        visibleCheckBox.Enabled = false;
        visibleCheckBox.Checked = false;
      }
    }


    void indexCommentText_TextChanged(object sender, System.EventArgs e)
    {
      if (indicesTreeView.SelectedNode != null)
      {
        GrtListNode node = indicesTreeView.SelectedNode.Tag as GrtListNode;

        if (node != null)
        {
          String text = null;
          tableEditorBE.get_indexes().get_field(node.NodeId, 
            (int)MySQL.Grt.Db.IndexListWrapper.IndexListColumns.Comment, out text);
          if (indexCommentText.Text != text)
          {
            tableEditorBE.get_indexes().set_field(node.NodeId, 
              (int)MySQL.Grt.Db.IndexListWrapper.IndexListColumns.Comment, indexCommentText.Text);
          }
        }
      }
    }

    void indexParserText_TextChanged(object sender, System.EventArgs e)
    {
      if (indicesTreeView.SelectedNode != null)
      {
        GrtListNode node = indicesTreeView.SelectedNode.Tag as GrtListNode;

        if (node != null)
        {
          String text = null;
          tableEditorBE.get_indexes().get_field(node.NodeId, 
            (int)MySQLIndexListWrapper.Columns.Parser, out text);
          if (indexParserText.Text != text)
          {
            tableEditorBE.get_indexes().set_field(node.NodeId, 
              (int)MySQLIndexListWrapper.Columns.Parser, indexParserText.Text);
          }
        }
      }
    }

    void visibleChanged(object sender, System.EventArgs e) 
    {
      if (indicesTreeView.SelectedNode != null)
      {
        GrtListNode node = indicesTreeView.SelectedNode.Tag as GrtListNode;
        if (node != null) {
          int value = visibleCheckBox.Checked ? 1 : 0;
          tableEditorBE.get_indexes().set_field(node.NodeId, (int)MySQLIndexListWrapper.Columns.Visible, value);
        }
      }
    }

    void indexRowBlockSizeText_TextChanged(object sender, System.EventArgs e)
    {
      if (indicesTreeView.SelectedNode != null)
      {
        GrtListNode node = indicesTreeView.SelectedNode.Tag as GrtListNode;

        if (node != null)
        {
          String text = null;
          tableEditorBE.get_indexes().get_field(node.NodeId, 
            (int)MySQLIndexListWrapper.Columns.RowBlockSize, out text);
          if (indexRowBlockSizeText.Text != text)
          {
            tableEditorBE.get_indexes().set_field(node.NodeId, 
              (int)MySQLIndexListWrapper.Columns.RowBlockSize, indexRowBlockSizeText.Text);
          }
        }
      }
    }

    void indexStorageTypeComboBox_SelectedIndexChanged(object sender, System.EventArgs e)
    {
      if (indicesTreeView.SelectedNode != null)
      {
        GrtListNode node = indicesTreeView.SelectedNode.Tag as GrtListNode;

        if (node != null)
        {
          String text = null;
          tableEditorBE.get_indexes().get_field(node.NodeId,
            (int)MySQLIndexListWrapper.Columns.StorageType, out text);

          if (indexStorageTypeComboBox.SelectedItem == null)
          {
            tableEditorBE.get_indexes().set_field(node.NodeId,
              (int)MySQLIndexListWrapper.Columns.StorageType, "");
          }
          else if (indexStorageTypeComboBox.SelectedItem.ToString() != text)
          {
            tableEditorBE.get_indexes().set_field(node.NodeId,
              (int)MySQLIndexListWrapper.Columns.StorageType, indexStorageTypeComboBox.SelectedItem.ToString());
          }
        }
      }
    }
    
    private void deleteSelectedIndicesToolStripMenuItem_Click(object sender, EventArgs e)
    {
      // Loop over all selected Nodes and delete them
      if (indicesTreeView.SelectedNodes.Count > 0)
      {
        List<NodeIdWrapper> nodes = new List<NodeIdWrapper>();

        foreach (TreeNodeAdv node in indicesTreeView.SelectedNodes)
        {
          GrtListNode listNode = node.Tag as GrtListNode;
          nodes.Add(listNode.NodeId);
        }
        nodes.Reverse();

        foreach (NodeIdWrapper node in nodes)
          tableEditorBE.remove_index(node);

        indicesListModel.RefreshModel();
      }
    }

    #endregion

    #region Options

    private void setTableOpt(string controlName)
    {
      switch (optPackKeys.SelectedIndex)
      {
        case 0:
          tableEditorBE.set_table_option_by_name("PACK_KEYS", "");
          break;
        case 1:
          tableEditorBE.set_table_option_by_name("PACK_KEYS", "DEFAULT");
          break;
        case 2:
          tableEditorBE.set_table_option_by_name("PACK_KEYS", "0");
          break;
        case 3:
          tableEditorBE.set_table_option_by_name("PACK_KEYS", "1");
          break;
      }

      tableEditorBE.set_table_option_by_name("PASSWORD", optTablePassword.Text);
      tableEditorBE.set_table_option_by_name("AUTO_INCREMENT", optAutoIncrement.Text);

      if (optDelayKeyUpdates.Checked)
        tableEditorBE.set_table_option_by_name("DELAY_KEY_WRITE", "1");
      else
        tableEditorBE.set_table_option_by_name("DELAY_KEY_WRITE", "0");

      switch (optRowFormat.SelectedIndex)
      {
        case 0:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "");
          break;
        case 1:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "DEFAULT");
          break;
        case 2:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "DYNAMIC");
          break;
        case 3:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "FIXED");
          break;
        case 4:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "COMPRESSED");
          break;
        case 5:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "REDUNDANT");
          break;
        case 6:
          tableEditorBE.set_table_option_by_name("ROW_FORMAT", "COMPACT");
          break;
      }

      switch (optKeyBlockSize.SelectedIndex)
      {
        case 0:
          tableEditorBE.set_table_option_by_name("KEY_BLOCK_SIZE", "");
          break;
        case 1:
          tableEditorBE.set_table_option_by_name("KEY_BLOCK_SIZE", "1");
          break;
        case 2:
          tableEditorBE.set_table_option_by_name("KEY_BLOCK_SIZE", "2");
          break;
        case 3:
          tableEditorBE.set_table_option_by_name("KEY_BLOCK_SIZE", "4");
          break;
        case 4:
          tableEditorBE.set_table_option_by_name("KEY_BLOCK_SIZE", "8");
          break;
        case 5:
          tableEditorBE.set_table_option_by_name("KEY_BLOCK_SIZE", "16");
          break;
      }

      tableEditorBE.set_table_option_by_name("AVG_ROW_LENGTH", optAvgRowLength.Text);
      tableEditorBE.set_table_option_by_name("MAX_ROWS", optMaxRows.Text);
      tableEditorBE.set_table_option_by_name("MIN_ROWS", optMinRows.Text);

      if (optUseChecksum.Checked)
        tableEditorBE.set_table_option_by_name("CHECKSUM", "1");
      else
        tableEditorBE.set_table_option_by_name("CHECKSUM", "0");

      tableEditorBE.set_table_option_by_name("DATA DIRECTORY", optDataDirectory.Text);
      tableEditorBE.set_table_option_by_name("INDEX DIRECTORY", optIndexDirectory.Text);
      tableEditorBE.set_table_option_by_name("UNION", optUnionTables.Text);

      switch (optMergeMethod.SelectedIndex)
      {
        case 0:
          tableEditorBE.set_table_option_by_name("INSERT_METHOD", ""); // Don't set NO here or we get into trouble if the table engine changes.
          break;
        case 1:
          tableEditorBE.set_table_option_by_name("INSERT_METHOD", "FIRST");
          break;
        case 2:
          tableEditorBE.set_table_option_by_name("INSERT_METHOD", "LAST");
          break;
      }

      String eng = optEngine.Text;
     // if (eng == optEngine.Items[0].ToString())
     //   eng = "";

      tableEditorBE.set_table_option_by_name("ENGINE", eng);

      if(controlName == "optCharset") { 
        // set charset
        if (optCharset.SelectedIndex == 0) {
          tableEditorBE.set_table_option_by_name("CHARACTER SET", "");
          optCollation.Items.Clear();
          optCollation.Items.Add("Default Collation");
          optCollation.SelectedIndex = 0;
        } else {
          tableEditorBE.set_table_option_by_name("CHARACTER SET", optCharset.Text);
          var collation = tableEditorBE.get_charset_collation_list(optCharset.Text);
          optCollation.Items.Clear();
          optCollation.Items.AddRange(collation.ToArray());
          optCollation.SelectedIndex = 0;
        }
      }

      if (controlName == "optCollation") {
        // set collation
        if (optCollation.SelectedIndex == 0) {
          tableEditorBE.set_table_option_by_name("COLLATE", "");
        } else {
          tableEditorBE.set_table_option_by_name("COLLATE", optCollation.Text);
        }
      }
    }

    private void tableOptChanged(object sender, EventArgs e)
    {
      // When a UI value changes, update the table options in the backend.
      if (!InitializingControls && !updatingTable)
      {
        updatingTable = true;
        try
        {
          var control = (Control)sender;
          var name = control != null ? control.Name : "";
          setTableOpt(name);
        }
        finally
        {
          updatingTable = false;
        }
      }
    }

    private void DbMysqlTableEditor_KeyDown(object sender, KeyEventArgs e)
    {
      if (e.KeyCode == Keys.Right && e.Control && e.Alt)
      {
        if (mainTabControl.SelectedIndex < mainTabControl.TabCount - 1)
          mainTabControl.SelectedIndex = mainTabControl.SelectedIndex + 1;
        else
          mainTabControl.SelectedIndex = 0;

        e.Handled = true;
      }

      if (e.KeyCode == Keys.Left && e.Control && e.Alt)
      {
        if (mainTabControl.SelectedIndex > 0)
          mainTabControl.SelectedIndex = mainTabControl.SelectedIndex - 1;
        else
          mainTabControl.SelectedIndex = mainTabControl.TabCount - 1;

        e.Handled = true;
      }
    }

    private void DbMysqlTableEditor_Shown(object sender, EventArgs e)
    {
      DbMysqlTableEditor editor = (DbMysqlTableEditor)sender;
      if(sender != null) 
      {
        editor.Focus();
        editor.nameTextBox.Focus();
      }
    }

    #endregion

    #region Partitioning


    private void partEnable_CheckedChanged(object sender, EventArgs e)
    {
      bool flag = partEnable.Checked;

      partFunction.Enabled = flag;
      partParams.Enabled = flag;
      partCount.Enabled = flag;
      partManual.Enabled = flag;
      //partPanel.Enabled = flag;

      if (flag)
      {
        if (tableEditorBE.get_partition_type() == "")
        {
          // this will set partition function to HASH only if
          // nothing is selected in the drop-down, otherwise
          // the currect selection will be applied to backend
          tableEditorBE.set_partition_type("HASH");
          partFunction_SelectedIndexChanged(this, null);
        }
      }
      else
        tableEditorBE.set_partition_type("");

      if (partFunction.SelectedItem == null || !(partFunction.SelectedItem.ToString() == "RANGE" || partFunction.SelectedItem.ToString() == "LIST"))
        flag = false;

      subpartFunction.Enabled = flag;
      subpartParams.Enabled = flag;
      subpartCount.Enabled = flag;
      subpartManual.Enabled = flag;
    }

    private void partFunction_SelectedIndexChanged(object sender, EventArgs e)
    {
      if (partFunction.SelectedItem == null)
      {
        partFunction.SelectedItem = tableEditorBE.get_partition_type();
        return;
      }

      if (partFunction.SelectedItem.ToString() != tableEditorBE.get_partition_type())
      {
        if (!tableEditorBE.set_partition_type(partFunction.SelectedItem.ToString()))
        {
          partFunction.SelectedItem = tableEditorBE.get_partition_type();
          return;
        }
      }
      if (partFunction.SelectedItem != null && (partFunction.SelectedItem.ToString() == "RANGE" || partFunction.SelectedItem.ToString() == "LIST"))
      {
        subpartFunction.Enabled = true;
        subpartParams.Enabled = true;
        subpartCount.Enabled = true;
        subpartManual.Enabled = true;
      }
      else
      {
        subpartFunction.Enabled = false;
        subpartParams.Enabled = false;
        subpartCount.Enabled = false;
        subpartManual.Enabled = false;
      }
    }


    void subpartFunction_SelectedIndexChanged(object sender, System.EventArgs e)
    {
      if (subpartFunction.SelectedItem.ToString() != tableEditorBE.get_subpartition_type())
      {
        if (subpartFunction.SelectedItem == null || !tableEditorBE.set_subpartition_type(subpartFunction.SelectedItem.ToString()))
        {
          subpartFunction.SelectedItem = tableEditorBE.get_subpartition_type();
          return;
        }
      }
    }


    private void refreshPartitioningList()
    {
      partitionTreeModel.RefreshModel();

      partitionTreeView.ExpandAll();
    }


    void partParams_TextChanged(object sender, System.EventArgs e)
    {
      tableEditorBE.set_partition_expression(partParams.Text);
    }

    void subpartParams_TextChanged(object sender, System.EventArgs e)
    {
      tableEditorBE.set_subpartition_expression(subpartParams.Text);
    }

    private void partCount_Changed(object sender, EventArgs e)
    {
      try
      {
        if (partCount.Text == "")
          tableEditorBE.set_partition_count(0);
        else
          tableEditorBE.set_partition_count(int.Parse(partCount.Text));
        refreshPartitioningList();
      }
      catch (System.FormatException exc)
      {
        MessageBox.Show(exc.Message, "Invalid Value");
      }
    }

    private void subpartCount_Changed(object sender, EventArgs e)
    {
      try
      {
        if (subpartCount.Text == "")
          tableEditorBE.set_subpartition_count(0);
        else
          tableEditorBE.set_subpartition_count(int.Parse(subpartCount.Text));
        refreshPartitioningList();
      }
      catch (System.FormatException exc)
      {
        MessageBox.Show(exc.Message, "Invalid Value");
      }
    }

    private void partManual_CheckedChanged(object sender, EventArgs e)
    {
      tableEditorBE.set_explicit_partitions(partManual.Checked);

      partCount.Text = "" + tableEditorBE.get_partition_count();

      refreshPartitioningList();
    }

    private void subpartManual_CheckedChanged(object sender, EventArgs e)
    {
      tableEditorBE.set_explicit_subpartitions(subpartManual.Checked);

      subpartCount.Text = "" + tableEditorBE.get_subpartition_count();

      refreshPartitioningList();
    }

    #endregion

    #region Foreign Keys

    private void fkTreeView_SelectionChanged(object sender, EventArgs e)
    {
      updatingFkValues = true;

      try
      {
        // Remove old virtual value events.
        if (fkColumnsTreeView.Model != null)
          fkColumnsListModel.DetachEvents();

        // if a new node was selected, create an inspector for it
        if (fkTreeView.SelectedNode != null && fkTreeView.SelectedNode.NextNode != null)
        {
          GrtListNode node = fkTreeView.SelectedNode.Tag as GrtListNode;

          if (node != null)
          {
            String text;

            fkCommentText.Enabled = true;
            onDeleteActionComboBox.Enabled = true;
            onUpdateActionComboBox.Enabled = true;

            // select the fk
            tableEditorBE.get_fks().select_fk(node.NodeId);

            // create new inspector model for the selected value, pass valueNodeTextBox so virtual value events can be attached
            fkColumnsListModel = new DbMysqlTableFkColumnListModel(fkColumnsTreeView,
              tableEditorBE.get_fks().get_columns(),
              columnEnabledFkNodeControl, columnFkNodeControl,
              targetColumnFkNodeControl, tableEditorBE);

            tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.OnDelete, out text);
            if (String.IsNullOrEmpty(text))
              onDeleteActionComboBox.SelectedIndex = -1;
            else
              onDeleteActionComboBox.SelectedItem = text;

            tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.OnUpdate, out text);
            if (String.IsNullOrEmpty(text))
              onUpdateActionComboBox.SelectedIndex = -1;
            else
              onUpdateActionComboBox.SelectedItem = text;

            tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.Comment, out text);
            fkCommentText.Text = text;

            tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.Index, out text);

            int value = 0;
            tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.ModelOnly, out value);
            fkModelOnlyCheck.Checked = value != 0;
            fkModelOnlyCheck.Enabled = true;

            // assign model to treeview
            fkColumnsTreeView.Model = fkColumnsListModel;
          }
          else
          {
            fkColumnsTreeView.Model = null;
            onDeleteActionComboBox.SelectedIndex = -1;
            onUpdateActionComboBox.SelectedIndex = -1;
            fkCommentText.Text = "";
            fkIndexLabel.Text = "";
            fkModelOnlyCheck.Checked = false;
            fkModelOnlyCheck.Enabled = false;
            fkCommentText.Enabled = false;
            onDeleteActionComboBox.Enabled = false;
            onUpdateActionComboBox.Enabled = false;
          }
        }
        else
        {
          fkColumnsTreeView.Model = null;
          onDeleteActionComboBox.SelectedIndex = -1;
          onUpdateActionComboBox.SelectedIndex = -1;
          fkCommentText.Text = "";
          fkIndexLabel.Text = "";
          fkModelOnlyCheck.Checked = false;
          fkModelOnlyCheck.Enabled = false;
          fkCommentText.Enabled = false;
          onDeleteActionComboBox.Enabled = false;
          onUpdateActionComboBox.Enabled = false;
        }
      }
      finally
      {
        updatingFkValues = false;
      }
    }

    void fkModelOnlyCheck_CheckedChanged(object sender, System.EventArgs e)
    {
      if (onUpdateActionComboBox.SelectedItem != null)
      {
        foreach (TreeNodeAdv node in fkTreeView.SelectedNodes)
        {
          GrtListNode listNode = node.Tag as GrtListNode;

          tableEditorBE.get_fks().set_field(listNode.NodeId,
            (int)FKConstraintListWrapper.FKConstraintListColumns.ModelOnly,
            fkModelOnlyCheck.Checked ? 1 : 0);
        }
      }
    }

    private void fkTreeView_KeyPress(object sender, KeyPressEventArgs e)
    {
      // initially was intended for deleting fk columns using keyboard
    }

    private void deleteSelectedFKsToolStripMenuItem_Click(object sender, EventArgs e)
    {
      // Loop over all selected Nodes and delete them
      if (fkTreeView.SelectedNodes.Count > 0)
      {
        List<NodeIdWrapper> nodes = new List<NodeIdWrapper>();

        foreach (TreeNodeAdv node in fkTreeView.SelectedNodes)
        {
          GrtListNode listNode = node.Tag as GrtListNode;
          nodes.Add(listNode.NodeId);
        }
        nodes.Reverse();

        foreach (NodeIdWrapper node in nodes)
          tableEditorBE.remove_fk(node);

        fkListModel.RefreshModel();
      }
    }

    private void onUpdateActionComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      if (!updatingFkValues && onUpdateActionComboBox.SelectedItem != null)
      {
        // Always assigned, otherwise the combobox would be disabled.
        GrtListNode node = fkTreeView.SelectedNode.Tag as GrtListNode;

        if (!tableEditorBE.get_fks().set_field(node.NodeId,
          (int)FKConstraintListWrapper.FKConstraintListColumns.OnUpdate,
          onUpdateActionComboBox.SelectedItem.ToString()))
        {
          // The change was rejected. Revert the combobox to its original value.
          string text;
          tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.OnUpdate, out text);
          if (String.IsNullOrEmpty(text))
            onUpdateActionComboBox.SelectedIndex = -1;
          else
            onUpdateActionComboBox.SelectedItem = text;
        }
      }
    }

    private void onDeleteActionComboBox_SelectedIndexChanged(object sender, EventArgs e)
    {
      if (!updatingFkValues && onDeleteActionComboBox.SelectedItem != null)
      {
        // Always assigned, otherwise the combobox would be disabled.
        GrtListNode node = fkTreeView.SelectedNode.Tag as GrtListNode;

        if (!tableEditorBE.get_fks().set_field(node.NodeId,
          (int)FKConstraintListWrapper.FKConstraintListColumns.OnDelete,
          onDeleteActionComboBox.SelectedItem.ToString()))
        {
          // The change was rejected. Revert the combobox to its original value.
          string text;
          tableEditorBE.get_fks().get_field(node.NodeId, (int)FKConstraintListWrapper.FKConstraintListColumns.OnDelete, out text);
          if (String.IsNullOrEmpty(text))
            onDeleteActionComboBox.SelectedIndex = -1;
          else
            onDeleteActionComboBox.SelectedItem = text;
        }
      }
    }

    private void fkCommentText_TextChanged(object sender, EventArgs e)
    {
      foreach (TreeNodeAdv node in fkTreeView.SelectedNodes)
      {
        GrtListNode listNode = node.Tag as GrtListNode;

        tableEditorBE.get_fks().set_field(listNode.NodeId,
          (int)FKConstraintListWrapper.FKConstraintListColumns.Comment,
          fkCommentText.Text);
      }
    }

    #endregion

    #region Inserts

    private void insertsTabPage_Enter(object sender, EventArgs e)
    {
      if (insertsTabPage.Controls.Count == 0)
      {
        Control panel = tableEditorBE.get_inserts_panel();
        insertsTabPage.Controls.Add(panel);
        panel.Parent = insertsTabPage;
        panel.Dock = DockStyle.Fill;
      }
    }

    #endregion

    #region Event handling

    void default_NodeMouseDoubleClick(object sender, Aga.Controls.Tree.TreeNodeAdvMouseEventArgs e)
    {
      if (e.Node != null && e.Node.Tag != null)
      {
        if (e.Control is AdvNodeTextBox)
        {
          AdvNodeTextBox tbox = e.Control as AdvNodeTextBox;
          tbox.BeginEdit();
        }
        else if (e.Control is AdvNodeComboBox)
        {
          AdvNodeComboBox tbox = e.Control as AdvNodeComboBox;
          tbox.BeginEdit();
        }
      }
    }

    private void collapsePictureBox_Click(object sender, EventArgs e)
    {
      headingCollapsed = !headingCollapsed;
      if (headingCollapsed)
      {
        collapsePictureBox.Image = Resources.EditorExpand;
        headingLayoutPanel.AutoSize = false;
        headingLayoutPanel.Size = new System.Drawing.Size(headingLayoutPanel.Width, 38);
        headingLayoutPanel.SetRowSpan(this.pictureBox1, 1);
      }
      else
      {
        collapsePictureBox.Image = Resources.EditorCollapse;
        headingLayoutPanel.AutoSize = true;
        headingLayoutPanel.SetRowSpan(this.pictureBox1, 2);
      }
    }

    #endregion

    #region Drag & Drop

    private void columnsTreeView_DragEnter(object sender, DragEventArgs e)
    {
      if (e.Data.GetDataPresent(typeof(GrtValue)))
      {
        GrtValue value= (GrtValue)e.Data.GetData(typeof(GrtValue));

        if (value != null && value.is_object_instance_of("db.UserDatatype"))
        {
          e.Effect = DragDropEffects.Copy;
          return;
        }
      }

      e.Effect = DragDropEffects.Move;
    }

    private void columnsTreeView_DragOver(object sender, DragEventArgs e)
    {
      if (e.Data.GetDataPresent(typeof(GrtValue)))
      {
        GrtValue value = (GrtValue)e.Data.GetData(typeof(GrtValue));

        if (value != null && value.is_object_instance_of("db.UserDatatype"))
        {
          e.Effect = DragDropEffects.Copy;
          return;
        }
      }

      TreeNodeAdv dropNode = columnsTreeView.DropPosition.Node as TreeNodeAdv;
      if (dropNode == null)
      {
        e.Effect = DragDropEffects.None;
        return;
      }

      TreeNodeAdv[] nodes = (TreeNodeAdv[])e.Data.GetData(typeof(TreeNodeAdv[]));
      if (nodes == null) // Probably drag operation from outside.
      {
        e.Effect = DragDropEffects.None;
        return;
      }

      int targetIndex = dropNode.Index;
      if (columnsTreeView.DropPosition.Position == NodePosition.After)
        targetIndex++;

      // Check the actual drop position. A node cannot be dragged onto itself (which would mean
      // insert it right before itself, which is meaningless) or on/before the next node (which
      // would mean insert it at the same position again, which it is already).
      foreach (TreeNodeAdv node in nodes)
        if (node.Index == targetIndex || node.Index + 1 == targetIndex)
        {
          e.Effect = DragDropEffects.None;
          return;
        }
      e.Effect = DragDropEffects.Move;
    }

    private void columnsTreeView_DragDrop(object sender, DragEventArgs e)
    {
      if (e.Data.GetDataPresent(typeof(GrtValue)))
      {
        GrtValue value = (GrtValue)e.Data.GetData(typeof(GrtValue));

        if (value.is_object_instance_of("db.UserDatatype"))
        {
          TreeNodeAdv node = columnsTreeView.DropPosition.Node as TreeNodeAdv;
          if(node != null)
            tableEditorBE.get_columns().set_column_type(((GrtListNode)node.Tag).NodeId, value);
        }
        return;
      }

      TreeNodeAdv dropNode = columnsTreeView.DropPosition.Node as TreeNodeAdv;
      if (dropNode == null)
        return;

      // Each time a node is moved in the backend the model recreates all nodes in the tree.
      // So this list of nodes we get from the data object is invalid right after the first move.
      // To make it still work we keep only the indices to change and update that list as we go.
      TreeNodeAdv[] nodes = (TreeNodeAdv[])e.Data.GetData(typeof(TreeNodeAdv[]));
      List<int> indices = new List<int>(nodes.Length);
      foreach (TreeNodeAdv nextNode in nodes)
        indices.Add(nextNode.Index);

      int targetIndex = dropNode.Index;
      if (columnsTreeView.DropPosition.Position == NodePosition.After)
        targetIndex++;

      tableEditorBE.get_columns().reorder_many(indices, targetIndex);
    }

    private void columnsTreeView_ItemDrag(object sender, ItemDragEventArgs e)
    {
      columnsTreeView.HideEditor(); // In case a node editor just popped up.
      dragInProgress = true; // In case a new edit operation is just about to start.

      // Remove the last node (row) from the selection as this is a virtual entry not meant
      // to be moved or anything other than holding place for a new column entry.
      columnsTreeView.Root.Children[columnsTreeView.Root.Children.Count - 1].IsSelected = false;
      if (columnsTreeView.SelectedNodes.Count > 0)
        columnsTreeView.DoDragDropSelectedNodes(DragDropEffects.Move);
      dragInProgress = false;
    }

    #endregion

    #region IWorkbenchDocument Interface

    public override bool CanCloseDocument()
    {
      return base.CanCloseDocument();
    }
    
    #endregion

    #region IModelChangeListener interface

    /// <summary>
    /// Called by the model if something was changed there. We use this notification to update
    /// our separate input elements (especially the checkboxes). No need to also update the text fields
    /// as they are already adjusted in the selection change event handler.
    /// </summary>
    /// <param name="control">The control that caused the change.</param>
    public void ValueChanged(BindableControl control, Object value)
    {
      if (value == null || !(control is NodeCheckBox))
        return;

      bool boolValue = Convert.ToBoolean(value);
      if (control == pkNodeControl)
        pkCheckBox.Checked = boolValue;
      else
        if (control == nnNodeControl)
          nnCheckBox.Checked = boolValue;
        else
          if (control == uqNodeControl)
            uniqueCheckBox.Checked = boolValue;
          else
            if (control == binNodeControl)
              binaryCheckBox.Checked = boolValue;
            else
              if (control == unNodeControl)
                unsignedCheckBox.Checked = boolValue;
              else
                if (control == zfNodeControl)
                  zeroFillCheckBox.Checked = boolValue;
                else
                  if (control == aiNodeControl)
                    aiCheckBox.Checked = boolValue;
                  else
                    if (control == gNodeControl)
                      generatedCheckbox.Checked = boolValue;
    }

    #endregion

  }
}
