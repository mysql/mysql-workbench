namespace MySQL.GUI.Workbench.Plugins
{
	partial class DbMysqlTableEditor
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
      this.components = new System.ComponentModel.Container();
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlTableEditor));
      this.columnNameTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.datatypeTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.nnTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.aiTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.defaultTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.nnNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.aiNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.gNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.gTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.indexNameColumn = new Aga.Controls.Tree.TreeColumn();
      this.indexTypeColumn = new Aga.Controls.Tree.TreeColumn();
      this.indexColumnNameTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.indexColumnStorageTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.indexColumnLengthTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.columnIconNodeControl = new Aga.Controls.Tree.NodeControls.NodeIcon();
      this.nameNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.datatypeComboBoxNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.defaultNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.indexContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.deleteSelectedIndicesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.indexNameNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.indexTypeNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.indexColumnOrderTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.indexColumnEnabledNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.indexColumnNameNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.indexColumnOrderNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.indexColumnStorageNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.indexColumnLengthNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.nameFkColumn = new Aga.Controls.Tree.TreeColumn();
      this.targetFkColumn = new Aga.Controls.Tree.TreeColumn();
      this.fksContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.deleteSelectedFKsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.nameFkNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.targetFkNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.columnFkColumn = new Aga.Controls.Tree.TreeColumn();
      this.targetColumnFkColumn = new Aga.Controls.Tree.TreeColumn();
      this.columnEnabledFkNodeControl = new MySQL.Utilities.AdvNodeCheckBox();
      this.columnFkNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.targetColumnFkNodeControl = new MySQL.Utilities.AdvNodeComboBox();
      this.partNodeStateColumn = new Aga.Controls.Tree.TreeColumn();
      this.partNameTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partValuesTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partDataDirTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partIndexDirTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partMinRowsTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partMaxRowsTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partCommentTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partEngineTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partTablespaceTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partNodegroupTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.partNodeStateIcon = new Aga.Controls.Tree.NodeControls.NodeStateIcon();
      this.partNameNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partValuesNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partDataDirNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partIndexDirNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partMinRowsNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partMaxRowsNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partCommentNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partEngineNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partTablespaceNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.partNodegroupNodeControl = new MySQL.Utilities.AdvNodeTextBox();
      this.toolTip = new System.Windows.Forms.ToolTip(this.components);
      this.subpartFunction = new System.Windows.Forms.ComboBox();
      this.partParams = new System.Windows.Forms.TextBox();
      this.subpartParams = new System.Windows.Forms.TextBox();
      this.partFunction = new System.Windows.Forms.ComboBox();
      this.subpartManual = new System.Windows.Forms.CheckBox();
      this.partManual = new System.Windows.Forms.CheckBox();
      this.fkModelOnlyCheck = new System.Windows.Forms.CheckBox();
      this.indicesTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.collapsePictureBox = new System.Windows.Forms.PictureBox();
      this.virtualRadioButton = new System.Windows.Forms.RadioButton();
      this.storedRadioButton = new System.Windows.Forms.RadioButton();
      this.partitionTreeMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.showClusterSettingsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
      this.createSubpartitionToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.deleteRowToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.insertsTabPage = new System.Windows.Forms.TabPage();
      this.optionsTabPage = new System.Windows.Forms.TabPage();
      this.panel8 = new System.Windows.Forms.Panel();
      this.optMergeMethod = new System.Windows.Forms.ComboBox();
      this.label48 = new System.Windows.Forms.Label();
      this.label49 = new System.Windows.Forms.Label();
      this.label46 = new System.Windows.Forms.Label();
      this.label47 = new System.Windows.Forms.Label();
      this.optUnionTables = new System.Windows.Forms.TextBox();
      this.label45 = new System.Windows.Forms.Label();
      this.bevel11 = new MySQL.Utilities.Bevel();
      this.panel7 = new System.Windows.Forms.Panel();
      this.label41 = new System.Windows.Forms.Label();
      this.optIndexDirectory = new System.Windows.Forms.TextBox();
      this.label42 = new System.Windows.Forms.Label();
      this.label43 = new System.Windows.Forms.Label();
      this.label44 = new System.Windows.Forms.Label();
      this.optDataDirectory = new System.Windows.Forms.TextBox();
      this.label40 = new System.Windows.Forms.Label();
      this.bevel10 = new MySQL.Utilities.Bevel();
      this.panel6 = new System.Windows.Forms.Panel();
      this.optKeyBlockSize = new System.Windows.Forms.ComboBox();
      this.label5 = new System.Windows.Forms.Label();
      this.label56 = new System.Windows.Forms.Label();
      this.label29 = new System.Windows.Forms.Label();
      this.label39 = new System.Windows.Forms.Label();
      this.optUseChecksum = new System.Windows.Forms.CheckBox();
      this.bevel9 = new MySQL.Utilities.Bevel();
      this.label37 = new System.Windows.Forms.Label();
      this.label33 = new System.Windows.Forms.Label();
      this.label30 = new System.Windows.Forms.Label();
      this.optRowFormat = new System.Windows.Forms.ComboBox();
      this.optMaxRows = new System.Windows.Forms.TextBox();
      this.label35 = new System.Windows.Forms.Label();
      this.label36 = new System.Windows.Forms.Label();
      this.label34 = new System.Windows.Forms.Label();
      this.optMinRows = new System.Windows.Forms.TextBox();
      this.optAvgRowLength = new System.Windows.Forms.TextBox();
      this.label31 = new System.Windows.Forms.Label();
      this.label32 = new System.Windows.Forms.Label();
      this.panel5 = new System.Windows.Forms.Panel();
      this.label28 = new System.Windows.Forms.Label();
      this.label38 = new System.Windows.Forms.Label();
      this.optDelayKeyUpdates = new System.Windows.Forms.CheckBox();
      this.bevel8 = new MySQL.Utilities.Bevel();
      this.label26 = new System.Windows.Forms.Label();
      this.label23 = new System.Windows.Forms.Label();
      this.optAutoIncrement = new System.Windows.Forms.TextBox();
      this.optPackKeys = new System.Windows.Forms.ComboBox();
      this.label27 = new System.Windows.Forms.Label();
      this.label22 = new System.Windows.Forms.Label();
      this.label24 = new System.Windows.Forms.Label();
      this.label25 = new System.Windows.Forms.Label();
      this.optTablePassword = new System.Windows.Forms.TextBox();
      this.partitioningTabPage = new System.Windows.Forms.TabPage();
      this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
      this.panel4 = new System.Windows.Forms.Panel();
      this.partEnable = new System.Windows.Forms.CheckBox();
      this.bevel12 = new MySQL.Utilities.Bevel();
      this.label50 = new System.Windows.Forms.Label();
      this.label54 = new System.Windows.Forms.Label();
      this.label55 = new System.Windows.Forms.Label();
      this.partCount = new System.Windows.Forms.TextBox();
      this.label52 = new System.Windows.Forms.Label();
      this.label51 = new System.Windows.Forms.Label();
      this.label53 = new System.Windows.Forms.Label();
      this.subpartCount = new System.Windows.Forms.TextBox();
      this.partitionTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.triggersTabPage = new System.Windows.Forms.TabPage();
      this.foreignKeysTabPage = new System.Windows.Forms.TabPage();
      this.foreignKeyWarningPanel = new System.Windows.Forms.Panel();
      this.label11 = new System.Windows.Forms.Label();
      this.foreignKeyPageSplitContainer = new System.Windows.Forms.SplitContainer();
      this.fkTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.fkColumnsTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.panel3 = new System.Windows.Forms.Panel();
      this.splitContainer6 = new System.Windows.Forms.SplitContainer();
      this.label21 = new System.Windows.Forms.Label();
      this.label10 = new System.Windows.Forms.Label();
      this.bevel7 = new MySQL.Utilities.Bevel();
      this.label20 = new System.Windows.Forms.Label();
      this.label18 = new System.Windows.Forms.Label();
      this.onDeleteActionComboBox = new System.Windows.Forms.ComboBox();
      this.onUpdateActionComboBox = new System.Windows.Forms.ComboBox();
      this.label19 = new System.Windows.Forms.Label();
      this.bevel6 = new MySQL.Utilities.Bevel();
      this.fkCommentText = new System.Windows.Forms.TextBox();
      this.fkIndexLabel = new System.Windows.Forms.Label();
      this.indicesTabPage = new System.Windows.Forms.TabPage();
      this.splitContainer1 = new System.Windows.Forms.SplitContainer();
      this.splitContainer3 = new System.Windows.Forms.SplitContainer();
      this.label8 = new System.Windows.Forms.Label();
      this.bevel3 = new MySQL.Utilities.Bevel();
      this.indexColumnsTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.panel2 = new System.Windows.Forms.Panel();
      this.splitContainer5 = new System.Windows.Forms.SplitContainer();
      this.indexParserText = new System.Windows.Forms.TextBox();
      this.label17 = new System.Windows.Forms.Label();
      this.label15 = new System.Windows.Forms.Label();
      this.indexRowBlockSizeText = new System.Windows.Forms.TextBox();
      this.label13 = new System.Windows.Forms.Label();
      this.bevel4 = new MySQL.Utilities.Bevel();
      this.label14 = new System.Windows.Forms.Label();
      this.indexStorageTypeComboBox = new System.Windows.Forms.ComboBox();
      this.bevel5 = new MySQL.Utilities.Bevel();
      this.label16 = new System.Windows.Forms.Label();
      this.indexCommentText = new System.Windows.Forms.TextBox();
      this.columnsTabPage = new System.Windows.Forms.TabPage();
      this.columnListSplitContainer = new System.Windows.Forms.SplitContainer();
      this.columnsTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.pkTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.uqTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.binTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.unTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.zfTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.pkNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.binNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.unNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.uqNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.zfNodeControl = new Aga.Controls.Tree.NodeControls.NodeCheckBox();
      this.oldTableLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
      this.bevel2 = new MySQL.Utilities.Bevel();
      this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
      this.columnCollationComboBox = new System.Windows.Forms.ComboBox();
      this.label12 = new System.Windows.Forms.Label();
      this.label9 = new System.Windows.Forms.Label();
      this.label6 = new System.Windows.Forms.Label();
      this.label57 = new System.Windows.Forms.Label();
      this.defaultLabel = new System.Windows.Forms.Label();
      this.columnNameTextBox = new System.Windows.Forms.TextBox();
      this.columnDataTypeTextBox = new System.Windows.Forms.TextBox();
      this.columnDefaultTextBox = new System.Windows.Forms.TextBox();
      this.pkCheckBox = new System.Windows.Forms.CheckBox();
      this.nnCheckBox = new System.Windows.Forms.CheckBox();
      this.uniqueCheckBox = new System.Windows.Forms.CheckBox();
      this.binaryCheckBox = new System.Windows.Forms.CheckBox();
      this.unsignedCheckBox = new System.Windows.Forms.CheckBox();
      this.zeroFillCheckBox = new System.Windows.Forms.CheckBox();
      this.aiCheckBox = new System.Windows.Forms.CheckBox();
      this.columnCommentTextBox = new System.Windows.Forms.TextBox();
      this.storageLabel = new System.Windows.Forms.Label();
      this.generatedCheckbox = new System.Windows.Forms.CheckBox();
      this.headingLayoutPanel = new System.Windows.Forms.TableLayoutPanel();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.optEngine = new System.Windows.Forms.ComboBox();
      this.label3 = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.schemaLabel = new System.Windows.Forms.Label();
      this.optCollation = new System.Windows.Forms.ComboBox();
      this.label2 = new System.Windows.Forms.Label();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.label1 = new System.Windows.Forms.Label();
      this.label7 = new System.Windows.Forms.Label();
      this.optComments = new System.Windows.Forms.TextBox();
      this.mainTabControl = new MySQL.Controls.FlatTabControl();
      this.topPanel = new System.Windows.Forms.Panel();
      this.indexContextMenuStrip.SuspendLayout();
      this.fksContextMenuStrip.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.collapsePictureBox)).BeginInit();
      this.partitionTreeMenuStrip.SuspendLayout();
      this.optionsTabPage.SuspendLayout();
      this.panel8.SuspendLayout();
      this.panel7.SuspendLayout();
      this.panel6.SuspendLayout();
      this.panel5.SuspendLayout();
      this.partitioningTabPage.SuspendLayout();
      this.tableLayoutPanel2.SuspendLayout();
      this.panel4.SuspendLayout();
      this.foreignKeysTabPage.SuspendLayout();
      this.foreignKeyWarningPanel.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.foreignKeyPageSplitContainer)).BeginInit();
      this.foreignKeyPageSplitContainer.Panel1.SuspendLayout();
      this.foreignKeyPageSplitContainer.Panel2.SuspendLayout();
      this.foreignKeyPageSplitContainer.SuspendLayout();
      this.panel3.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer6)).BeginInit();
      this.splitContainer6.Panel1.SuspendLayout();
      this.splitContainer6.Panel2.SuspendLayout();
      this.splitContainer6.SuspendLayout();
      this.indicesTabPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
      this.splitContainer1.Panel1.SuspendLayout();
      this.splitContainer1.Panel2.SuspendLayout();
      this.splitContainer1.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer3)).BeginInit();
      this.splitContainer3.Panel1.SuspendLayout();
      this.splitContainer3.Panel2.SuspendLayout();
      this.splitContainer3.SuspendLayout();
      this.panel2.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer5)).BeginInit();
      this.splitContainer5.Panel1.SuspendLayout();
      this.splitContainer5.Panel2.SuspendLayout();
      this.splitContainer5.SuspendLayout();
      this.columnsTabPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.columnListSplitContainer)).BeginInit();
      this.columnListSplitContainer.Panel1.SuspendLayout();
      this.columnListSplitContainer.Panel2.SuspendLayout();
      this.columnListSplitContainer.SuspendLayout();
      this.tableLayoutPanel1.SuspendLayout();
      this.headingLayoutPanel.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.mainTabControl.SuspendLayout();
      this.topPanel.SuspendLayout();
      this.SuspendLayout();
      // 
      // columnNameTreeColumn
      // 
      this.columnNameTreeColumn.Header = "Column Name";
      this.columnNameTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.columnNameTreeColumn.TooltipText = null;
      this.columnNameTreeColumn.Width = 180;
      // 
      // datatypeTreeColumn
      // 
      this.datatypeTreeColumn.Header = "Datatype";
      this.datatypeTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.datatypeTreeColumn.TooltipText = null;
      this.datatypeTreeColumn.Width = 120;
      // 
      // nnTreeColumn
      // 
      this.nnTreeColumn.Header = "NN";
      this.nnTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.nnTreeColumn.TooltipText = "Not Null";
      this.nnTreeColumn.Width = 30;
      // 
      // aiTreeColumn
      // 
      this.aiTreeColumn.Header = "AI";
      this.aiTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.aiTreeColumn.TooltipText = "Auto Incremental";
      this.aiTreeColumn.Width = 30;
      // 
      // defaultTreeColumn
      // 
      this.defaultTreeColumn.Header = "Default/Expression";
      this.defaultTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.defaultTreeColumn.TooltipText = "The expression for generated columns, otherwise the default value";
      this.defaultTreeColumn.Width = 150;
      // 
      // nnNodeControl
      // 
      this.nnNodeControl.LeftMargin = 3;
      this.nnNodeControl.ParentColumn = this.nnTreeColumn;
      this.nnNodeControl.VirtualMode = true;
      // 
      // aiNodeControl
      // 
      this.aiNodeControl.LeftMargin = 3;
      this.aiNodeControl.ParentColumn = this.aiTreeColumn;
      this.aiNodeControl.VirtualMode = true;
      // 
      // gNodeControl
      // 
      this.gNodeControl.LeftMargin = 3;
      this.gNodeControl.ParentColumn = this.gTreeColumn;
      this.gNodeControl.VirtualMode = true;
      // 
      // gTreeColumn
      // 
      this.gTreeColumn.Header = "G";
      this.gTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.gTreeColumn.TooltipText = "Generated Column";
      this.gTreeColumn.Width = 30;
      // 
      // indexNameColumn
      // 
      this.indexNameColumn.Header = "Index Name";
      this.indexNameColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.indexNameColumn.TooltipText = null;
      this.indexNameColumn.Width = 130;
      // 
      // indexTypeColumn
      // 
      this.indexTypeColumn.Header = "Type";
      this.indexTypeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.indexTypeColumn.TooltipText = null;
      this.indexTypeColumn.Width = 100;
      // 
      // indexColumnNameTreeColumn
      // 
      this.indexColumnNameTreeColumn.Header = "Column";
      this.indexColumnNameTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.indexColumnNameTreeColumn.TooltipText = null;
      this.indexColumnNameTreeColumn.Width = 130;
      // 
      // indexColumnStorageTreeColumn
      // 
      this.indexColumnStorageTreeColumn.Header = "Order";
      this.indexColumnStorageTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.indexColumnStorageTreeColumn.TooltipText = null;
      // 
      // indexColumnLengthTreeColumn
      // 
      this.indexColumnLengthTreeColumn.Header = "Length";
      this.indexColumnLengthTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.indexColumnLengthTreeColumn.TooltipText = null;
      // 
      // columnIconNodeControl
      // 
      this.columnIconNodeControl.LeftMargin = 1;
      this.columnIconNodeControl.ParentColumn = this.columnNameTreeColumn;
      this.columnIconNodeControl.VirtualMode = true;
      // 
      // nameNodeControl
      // 
      this.nameNodeControl.IncrementalSearchEnabled = true;
      this.nameNodeControl.LeftMargin = 3;
      this.nameNodeControl.ParentColumn = this.columnNameTreeColumn;
      this.nameNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.nameNodeControl.VirtualMode = true;
      // 
      // datatypeComboBoxNodeControl
      // 
      this.datatypeComboBoxNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDown;
      this.datatypeComboBoxNodeControl.IncrementalSearchEnabled = true;
      this.datatypeComboBoxNodeControl.LeftMargin = 0;
      this.datatypeComboBoxNodeControl.ParentColumn = this.datatypeTreeColumn;
      this.datatypeComboBoxNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.datatypeComboBoxNodeControl.VirtualMode = true;
      // 
      // defaultNodeControl
      // 
      this.defaultNodeControl.IncrementalSearchEnabled = true;
      this.defaultNodeControl.LeftMargin = 0;
      this.defaultNodeControl.ParentColumn = this.defaultTreeColumn;
      this.defaultNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.defaultNodeControl.VirtualMode = true;
      // 
      // indexContextMenuStrip
      // 
      this.indexContextMenuStrip.ImageScalingSize = new System.Drawing.Size(20, 20);
      this.indexContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.deleteSelectedIndicesToolStripMenuItem});
      this.indexContextMenuStrip.Name = "indexContextMenuStrip";
      this.indexContextMenuStrip.Size = new System.Drawing.Size(238, 28);
      // 
      // deleteSelectedIndicesToolStripMenuItem
      // 
      this.deleteSelectedIndicesToolStripMenuItem.Name = "deleteSelectedIndicesToolStripMenuItem";
      this.deleteSelectedIndicesToolStripMenuItem.Size = new System.Drawing.Size(237, 24);
      this.deleteSelectedIndicesToolStripMenuItem.Text = "Delete Selected Indexes";
      this.deleteSelectedIndicesToolStripMenuItem.Click += new System.EventHandler(this.deleteSelectedIndicesToolStripMenuItem_Click);
      // 
      // indexNameNodeControl
      // 
      this.indexNameNodeControl.IncrementalSearchEnabled = true;
      this.indexNameNodeControl.LeftMargin = 0;
      this.indexNameNodeControl.ParentColumn = this.indexNameColumn;
      this.indexNameNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.indexNameNodeControl.VirtualMode = true;
      // 
      // indexTypeNodeControl
      // 
      this.indexTypeNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.indexTypeNodeControl.IncrementalSearchEnabled = true;
      this.indexTypeNodeControl.LeftMargin = 0;
      this.indexTypeNodeControl.ParentColumn = this.indexTypeColumn;
      this.indexTypeNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.indexTypeNodeControl.VirtualMode = true;
      // 
      // indexColumnOrderTreeColumn
      // 
      this.indexColumnOrderTreeColumn.Header = "#";
      this.indexColumnOrderTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.indexColumnOrderTreeColumn.TooltipText = null;
      this.indexColumnOrderTreeColumn.Width = 40;
      // 
      // indexColumnEnabledNodeControl
      // 
      this.indexColumnEnabledNodeControl.LeftMargin = 0;
      this.indexColumnEnabledNodeControl.ParentColumn = this.indexColumnNameTreeColumn;
      this.indexColumnEnabledNodeControl.VirtualMode = true;
      // 
      // indexColumnNameNodeControl
      // 
      this.indexColumnNameNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.indexColumnNameNodeControl.EditEnabled = false;
      this.indexColumnNameNodeControl.IncrementalSearchEnabled = true;
      this.indexColumnNameNodeControl.LeftMargin = 3;
      this.indexColumnNameNodeControl.ParentColumn = this.indexColumnNameTreeColumn;
      this.indexColumnNameNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.indexColumnNameNodeControl.VirtualMode = true;
      // 
      // indexColumnOrderNodeControl
      // 
      this.indexColumnOrderNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.indexColumnOrderNodeControl.IncrementalSearchEnabled = true;
      this.indexColumnOrderNodeControl.LeftMargin = 3;
      this.indexColumnOrderNodeControl.ParentColumn = this.indexColumnOrderTreeColumn;
      this.indexColumnOrderNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.indexColumnOrderNodeControl.VirtualMode = true;
      // 
      // indexColumnStorageNodeControl
      // 
      this.indexColumnStorageNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.indexColumnStorageNodeControl.IncrementalSearchEnabled = true;
      this.indexColumnStorageNodeControl.LeftMargin = 3;
      this.indexColumnStorageNodeControl.ParentColumn = this.indexColumnStorageTreeColumn;
      this.indexColumnStorageNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.indexColumnStorageNodeControl.VirtualMode = true;
      // 
      // indexColumnLengthNodeControl
      // 
      this.indexColumnLengthNodeControl.IncrementalSearchEnabled = true;
      this.indexColumnLengthNodeControl.LeftMargin = 3;
      this.indexColumnLengthNodeControl.ParentColumn = this.indexColumnLengthTreeColumn;
      this.indexColumnLengthNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.indexColumnLengthNodeControl.VirtualMode = true;
      // 
      // nameFkColumn
      // 
      this.nameFkColumn.Header = "Foreign Key Name";
      this.nameFkColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.nameFkColumn.TooltipText = null;
      this.nameFkColumn.Width = 150;
      // 
      // targetFkColumn
      // 
      this.targetFkColumn.Header = "Referenced Table";
      this.targetFkColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.targetFkColumn.TooltipText = null;
      this.targetFkColumn.Width = 250;
      // 
      // fksContextMenuStrip
      // 
      this.fksContextMenuStrip.ImageScalingSize = new System.Drawing.Size(20, 20);
      this.fksContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.deleteSelectedFKsToolStripMenuItem});
      this.fksContextMenuStrip.Name = "fksContextMenuStrip";
      this.fksContextMenuStrip.Size = new System.Drawing.Size(210, 28);
      // 
      // deleteSelectedFKsToolStripMenuItem
      // 
      this.deleteSelectedFKsToolStripMenuItem.Name = "deleteSelectedFKsToolStripMenuItem";
      this.deleteSelectedFKsToolStripMenuItem.Size = new System.Drawing.Size(209, 24);
      this.deleteSelectedFKsToolStripMenuItem.Text = "Delete Selected FKs";
      this.deleteSelectedFKsToolStripMenuItem.Click += new System.EventHandler(this.deleteSelectedFKsToolStripMenuItem_Click);
      // 
      // nameFkNodeControl
      // 
      this.nameFkNodeControl.IncrementalSearchEnabled = true;
      this.nameFkNodeControl.LeftMargin = 3;
      this.nameFkNodeControl.ParentColumn = this.nameFkColumn;
      this.nameFkNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.nameFkNodeControl.VirtualMode = true;
      // 
      // targetFkNodeControl
      // 
      this.targetFkNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.targetFkNodeControl.EditorWidth = 300;
      this.targetFkNodeControl.IncrementalSearchEnabled = true;
      this.targetFkNodeControl.LeftMargin = 0;
      this.targetFkNodeControl.ParentColumn = this.targetFkColumn;
      this.targetFkNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.targetFkNodeControl.VerticalAlign = Aga.Controls.Tree.VerticalAlignment.Center;
      this.targetFkNodeControl.VirtualMode = true;
      // 
      // columnFkColumn
      // 
      this.columnFkColumn.Header = "Column";
      this.columnFkColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.columnFkColumn.TooltipText = null;
      this.columnFkColumn.Width = 120;
      // 
      // targetColumnFkColumn
      // 
      this.targetColumnFkColumn.Header = "Referenced Column";
      this.targetColumnFkColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.targetColumnFkColumn.TooltipText = null;
      this.targetColumnFkColumn.Width = 250;
      // 
      // columnEnabledFkNodeControl
      // 
      this.columnEnabledFkNodeControl.LeftMargin = 0;
      this.columnEnabledFkNodeControl.ParentColumn = this.columnFkColumn;
      this.columnEnabledFkNodeControl.VirtualMode = true;
      // 
      // columnFkNodeControl
      // 
      this.columnFkNodeControl.EditEnabled = false;
      this.columnFkNodeControl.IncrementalSearchEnabled = true;
      this.columnFkNodeControl.LeftMargin = 3;
      this.columnFkNodeControl.ParentColumn = this.columnFkColumn;
      this.columnFkNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.columnFkNodeControl.VirtualMode = true;
      // 
      // targetColumnFkNodeControl
      // 
      this.targetColumnFkNodeControl.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.targetColumnFkNodeControl.IncrementalSearchEnabled = true;
      this.targetColumnFkNodeControl.LeftMargin = 3;
      this.targetColumnFkNodeControl.ParentColumn = this.targetColumnFkColumn;
      this.targetColumnFkNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.targetColumnFkNodeControl.VirtualMode = true;
      // 
      // partNodeStateColumn
      // 
      this.partNodeStateColumn.Header = "";
      this.partNodeStateColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partNodeStateColumn.TooltipText = null;
      this.partNodeStateColumn.Width = 25;
      // 
      // partNameTreeColumn
      // 
      this.partNameTreeColumn.Header = "Partition";
      this.partNameTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partNameTreeColumn.TooltipText = null;
      this.partNameTreeColumn.Width = 90;
      // 
      // partValuesTreeColumn
      // 
      this.partValuesTreeColumn.Header = "Values";
      this.partValuesTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partValuesTreeColumn.TooltipText = null;
      this.partValuesTreeColumn.Width = 140;
      // 
      // partDataDirTreeColumn
      // 
      this.partDataDirTreeColumn.Header = "Data Directory";
      this.partDataDirTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partDataDirTreeColumn.TooltipText = null;
      this.partDataDirTreeColumn.Width = 150;
      // 
      // partIndexDirTreeColumn
      // 
      this.partIndexDirTreeColumn.Header = "Index Directory";
      this.partIndexDirTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partIndexDirTreeColumn.TooltipText = null;
      this.partIndexDirTreeColumn.Width = 150;
      // 
      // partMinRowsTreeColumn
      // 
      this.partMinRowsTreeColumn.Header = "Min Rows";
      this.partMinRowsTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partMinRowsTreeColumn.TooltipText = null;
      this.partMinRowsTreeColumn.Width = 60;
      // 
      // partMaxRowsTreeColumn
      // 
      this.partMaxRowsTreeColumn.Header = "Max Rows";
      this.partMaxRowsTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partMaxRowsTreeColumn.TooltipText = null;
      this.partMaxRowsTreeColumn.Width = 60;
      // 
      // partCommentTreeColumn
      // 
      this.partCommentTreeColumn.Header = "Comment";
      this.partCommentTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partCommentTreeColumn.TooltipText = null;
      this.partCommentTreeColumn.Width = 100;
      // 
      // partEngineTreeColumn
      // 
      this.partEngineTreeColumn.Header = "Engine";
      this.partEngineTreeColumn.IsVisible = false;
      this.partEngineTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partEngineTreeColumn.TooltipText = null;
      this.partEngineTreeColumn.Width = 90;
      // 
      // partTablespaceTreeColumn
      // 
      this.partTablespaceTreeColumn.Header = "Tablespace";
      this.partTablespaceTreeColumn.IsVisible = false;
      this.partTablespaceTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partTablespaceTreeColumn.TooltipText = null;
      this.partTablespaceTreeColumn.Width = 90;
      // 
      // partNodegroupTreeColumn
      // 
      this.partNodegroupTreeColumn.Header = "Nodegroup";
      this.partNodegroupTreeColumn.IsVisible = false;
      this.partNodegroupTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.partNodegroupTreeColumn.TooltipText = null;
      this.partNodegroupTreeColumn.Width = 90;
      // 
      // partNodeStateIcon
      // 
      this.partNodeStateIcon.LeftMargin = 1;
      this.partNodeStateIcon.ParentColumn = this.partNodeStateColumn;
      // 
      // partNameNodeControl
      // 
      this.partNameNodeControl.IncrementalSearchEnabled = true;
      this.partNameNodeControl.LeftMargin = 3;
      this.partNameNodeControl.ParentColumn = this.partNameTreeColumn;
      this.partNameNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partNameNodeControl.VirtualMode = true;
      // 
      // partValuesNodeControl
      // 
      this.partValuesNodeControl.IncrementalSearchEnabled = true;
      this.partValuesNodeControl.LeftMargin = 3;
      this.partValuesNodeControl.ParentColumn = this.partValuesTreeColumn;
      this.partValuesNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partValuesNodeControl.VirtualMode = true;
      // 
      // partDataDirNodeControl
      // 
      this.partDataDirNodeControl.IncrementalSearchEnabled = true;
      this.partDataDirNodeControl.LeftMargin = 3;
      this.partDataDirNodeControl.ParentColumn = this.partDataDirTreeColumn;
      this.partDataDirNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partDataDirNodeControl.VirtualMode = true;
      // 
      // partIndexDirNodeControl
      // 
      this.partIndexDirNodeControl.IncrementalSearchEnabled = true;
      this.partIndexDirNodeControl.LeftMargin = 3;
      this.partIndexDirNodeControl.ParentColumn = this.partIndexDirTreeColumn;
      this.partIndexDirNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partIndexDirNodeControl.VirtualMode = true;
      // 
      // partMinRowsNodeControl
      // 
      this.partMinRowsNodeControl.IncrementalSearchEnabled = true;
      this.partMinRowsNodeControl.LeftMargin = 3;
      this.partMinRowsNodeControl.ParentColumn = this.partMinRowsTreeColumn;
      this.partMinRowsNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partMinRowsNodeControl.VirtualMode = true;
      // 
      // partMaxRowsNodeControl
      // 
      this.partMaxRowsNodeControl.IncrementalSearchEnabled = true;
      this.partMaxRowsNodeControl.LeftMargin = 3;
      this.partMaxRowsNodeControl.ParentColumn = this.partMaxRowsTreeColumn;
      this.partMaxRowsNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partMaxRowsNodeControl.VirtualMode = true;
      // 
      // partCommentNodeControl
      // 
      this.partCommentNodeControl.IncrementalSearchEnabled = true;
      this.partCommentNodeControl.LeftMargin = 3;
      this.partCommentNodeControl.ParentColumn = this.partCommentTreeColumn;
      this.partCommentNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partCommentNodeControl.VirtualMode = true;
      // 
      // partEngineNodeControl
      // 
      this.partEngineNodeControl.IncrementalSearchEnabled = true;
      this.partEngineNodeControl.LeftMargin = 3;
      this.partEngineNodeControl.ParentColumn = this.partEngineTreeColumn;
      this.partEngineNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partEngineNodeControl.VirtualMode = true;
      // 
      // partTablespaceNodeControl
      // 
      this.partTablespaceNodeControl.IncrementalSearchEnabled = true;
      this.partTablespaceNodeControl.LeftMargin = 3;
      this.partTablespaceNodeControl.ParentColumn = this.partTablespaceTreeColumn;
      this.partTablespaceNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partTablespaceNodeControl.VirtualMode = true;
      // 
      // partNodegroupNodeControl
      // 
      this.partNodegroupNodeControl.IncrementalSearchEnabled = true;
      this.partNodegroupNodeControl.LeftMargin = 3;
      this.partNodegroupNodeControl.ParentColumn = this.partNodegroupTreeColumn;
      this.partNodegroupNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      this.partNodegroupNodeControl.VirtualMode = true;
      // 
      // subpartFunction
      // 
      this.subpartFunction.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.subpartFunction.Enabled = false;
      this.subpartFunction.FormattingEnabled = true;
      this.subpartFunction.Items.AddRange(new object[] {
            "Disable",
            "HASH",
            "LINEAR HASH",
            "KEY",
            "LINEAR KEY"});
      this.subpartFunction.Location = new System.Drawing.Point(113, 60);
      this.subpartFunction.Name = "subpartFunction";
      this.subpartFunction.Size = new System.Drawing.Size(114, 25);
      this.subpartFunction.TabIndex = 5;
      this.toolTip.SetToolTip(this.subpartFunction, "Function that is used to determine the partition.");
      this.subpartFunction.SelectedIndexChanged += new System.EventHandler(this.subpartFunction_SelectedIndexChanged);
      // 
      // partParams
      // 
      this.partParams.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.partParams.Enabled = false;
      this.partParams.Location = new System.Drawing.Point(309, 29);
      this.partParams.Name = "partParams";
      this.partParams.Size = new System.Drawing.Size(172, 24);
      this.partParams.TabIndex = 2;
      this.toolTip.SetToolTip(this.partParams, "The expression or column list used by the function to determine the partition.");
      this.partParams.TextChanged += new System.EventHandler(this.partParams_TextChanged);
      // 
      // subpartParams
      // 
      this.subpartParams.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.subpartParams.Enabled = false;
      this.subpartParams.Location = new System.Drawing.Point(309, 60);
      this.subpartParams.Name = "subpartParams";
      this.subpartParams.Size = new System.Drawing.Size(172, 24);
      this.subpartParams.TabIndex = 7;
      this.toolTip.SetToolTip(this.subpartParams, "The expression or column list used by the function to determine the partition.");
      this.subpartParams.TextChanged += new System.EventHandler(this.subpartParams_TextChanged);
      // 
      // partFunction
      // 
      this.partFunction.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.partFunction.Enabled = false;
      this.partFunction.FormattingEnabled = true;
      this.partFunction.Items.AddRange(new object[] {
            "HASH",
            "LINEAR HASH",
            "KEY",
            "LINEAR KEY",
            "RANGE",
            "LIST"});
      this.partFunction.Location = new System.Drawing.Point(113, 29);
      this.partFunction.Name = "partFunction";
      this.partFunction.Size = new System.Drawing.Size(114, 25);
      this.partFunction.TabIndex = 1;
      this.toolTip.SetToolTip(this.partFunction, "Function that is used to determine the partition.");
      this.partFunction.SelectedIndexChanged += new System.EventHandler(this.partFunction_SelectedIndexChanged);
      // 
      // subpartManual
      // 
      this.subpartManual.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.subpartManual.AutoSize = true;
      this.subpartManual.Enabled = false;
      this.subpartManual.Location = new System.Drawing.Point(649, 62);
      this.subpartManual.Name = "subpartManual";
      this.subpartManual.Size = new System.Drawing.Size(72, 21);
      this.subpartManual.TabIndex = 9;
      this.subpartManual.Text = "Manual";
      this.toolTip.SetToolTip(this.subpartManual, "Check to manually specify partitioning ranges/values.");
      this.subpartManual.UseVisualStyleBackColor = true;
      this.subpartManual.CheckedChanged += new System.EventHandler(this.subpartManual_CheckedChanged);
      // 
      // partManual
      // 
      this.partManual.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.partManual.AutoSize = true;
      this.partManual.Enabled = false;
      this.partManual.Location = new System.Drawing.Point(649, 31);
      this.partManual.Name = "partManual";
      this.partManual.Size = new System.Drawing.Size(72, 21);
      this.partManual.TabIndex = 4;
      this.partManual.Text = "Manual";
      this.toolTip.SetToolTip(this.partManual, "Check to manually specify partitioning ranges/values.");
      this.partManual.UseVisualStyleBackColor = true;
      this.partManual.CheckedChanged += new System.EventHandler(this.partManual_CheckedChanged);
      // 
      // fkModelOnlyCheck
      // 
      this.fkModelOnlyCheck.AutoSize = true;
      this.fkModelOnlyCheck.Location = new System.Drawing.Point(45, 90);
      this.fkModelOnlyCheck.Name = "fkModelOnlyCheck";
      this.fkModelOnlyCheck.Size = new System.Drawing.Size(167, 21);
      this.fkModelOnlyCheck.TabIndex = 22;
      this.fkModelOnlyCheck.Text = "Skip in SQL generation";
      this.toolTip.SetToolTip(this.fkModelOnlyCheck, resources.GetString("fkModelOnlyCheck.ToolTip"));
      this.fkModelOnlyCheck.CheckedChanged += new System.EventHandler(this.fkModelOnlyCheck_CheckedChanged);
      // 
      // indicesTreeView
      // 
      this.indicesTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.indicesTreeView.Columns.Add(this.indexNameColumn);
      this.indicesTreeView.Columns.Add(this.indexTypeColumn);
      this.indicesTreeView.ContextMenuStrip = this.indexContextMenuStrip;
      this.indicesTreeView.DefaultToolTipProvider = null;
      this.indicesTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.indicesTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.indicesTreeView.FullRowSelect = true;
      this.indicesTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.indicesTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.indicesTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.indicesTreeView.LoadOnDemand = true;
      this.indicesTreeView.Location = new System.Drawing.Point(0, 0);
      this.indicesTreeView.Model = null;
      this.indicesTreeView.Name = "indicesTreeView";
      this.indicesTreeView.NodeControls.Add(this.indexNameNodeControl);
      this.indicesTreeView.NodeControls.Add(this.indexTypeNodeControl);
      this.indicesTreeView.ScrollPosition = new System.Drawing.Point(0, 0);
      this.indicesTreeView.SelectedNode = null;
      this.indicesTreeView.SelectionMode = Aga.Controls.Tree.TreeSelectionMode.Multi;
      this.indicesTreeView.ShowHeader = true;
      this.indicesTreeView.ShowLines = false;
      this.indicesTreeView.ShowPlusMinus = false;
      this.indicesTreeView.Size = new System.Drawing.Size(241, 303);
      this.indicesTreeView.TabIndex = 0;
      this.indicesTreeView.Text = "indicesTreeViewAdv";
      this.toolTip.SetToolTip(this.indicesTreeView, "Use the right mouse button on the index grid to bring up the popup menu");
      this.indicesTreeView.UseColumns = true;
      this.indicesTreeView.NodeMouseDoubleClick += new System.EventHandler<Aga.Controls.Tree.TreeNodeAdvMouseEventArgs>(this.default_NodeMouseDoubleClick);
      this.indicesTreeView.SelectionChanged += new System.EventHandler(this.indicesTreeView_SelectionChanged);
      // 
      // collapsePictureBox
      // 
      this.collapsePictureBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.collapsePictureBox.Image = global::MySQL.GUI.Workbench.Plugins.Properties.Resources.EditorCollapse;
      this.collapsePictureBox.Location = new System.Drawing.Point(694, 8);
      this.collapsePictureBox.Margin = new System.Windows.Forms.Padding(10, 3, 0, 3);
      this.collapsePictureBox.Name = "collapsePictureBox";
      this.collapsePictureBox.Size = new System.Drawing.Size(26, 26);
      this.collapsePictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
      this.collapsePictureBox.TabIndex = 16;
      this.collapsePictureBox.TabStop = false;
      this.toolTip.SetToolTip(this.collapsePictureBox, "Click to change the number of displayed available table options");
      this.collapsePictureBox.Click += new System.EventHandler(this.collapsePictureBox_Click);
      // 
      // virtualRadioButton
      // 
      this.virtualRadioButton.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.virtualRadioButton.AutoSize = true;
      this.virtualRadioButton.Enabled = false;
      this.virtualRadioButton.Location = new System.Drawing.Point(397, 69);
      this.virtualRadioButton.Name = "virtualRadioButton";
      this.virtualRadioButton.Size = new System.Drawing.Size(126, 21);
      this.virtualRadioButton.TabIndex = 20;
      this.virtualRadioButton.TabStop = true;
      this.virtualRadioButton.Text = "Virtual";
      this.toolTip.SetToolTip(this.virtualRadioButton, "Only valid for generated columns. Determines that the column is  computed on dema" +
        "nd.");
      this.virtualRadioButton.UseVisualStyleBackColor = true;
      this.virtualRadioButton.CheckedChanged += new System.EventHandler(this.storageRadioButton_CheckedChanged);
      // 
      // storedRadioButton
      // 
      this.storedRadioButton.AutoSize = true;
      this.storedRadioButton.Enabled = false;
      this.storedRadioButton.Location = new System.Drawing.Point(529, 69);
      this.storedRadioButton.Name = "storedRadioButton";
      this.storedRadioButton.Size = new System.Drawing.Size(70, 21);
      this.storedRadioButton.TabIndex = 21;
      this.storedRadioButton.TabStop = true;
      this.storedRadioButton.Text = "Stored";
      this.toolTip.SetToolTip(this.storedRadioButton, "Only valid for generated columns. Determines that the column is actually stored.");
      this.storedRadioButton.UseVisualStyleBackColor = true;
      this.storedRadioButton.CheckedChanged += new System.EventHandler(this.storageRadioButton_CheckedChanged);
      // 
      // partitionTreeMenuStrip
      // 
      this.partitionTreeMenuStrip.ImageScalingSize = new System.Drawing.Size(20, 20);
      this.partitionTreeMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.showClusterSettingsToolStripMenuItem,
            this.toolStripMenuItem2,
            this.createSubpartitionToolStripMenuItem,
            this.deleteRowToolStripMenuItem});
      this.partitionTreeMenuStrip.Name = "partitionTreeMenuStrip";
      this.partitionTreeMenuStrip.Size = new System.Drawing.Size(221, 82);
      // 
      // showClusterSettingsToolStripMenuItem
      // 
      this.showClusterSettingsToolStripMenuItem.Name = "showClusterSettingsToolStripMenuItem";
      this.showClusterSettingsToolStripMenuItem.Size = new System.Drawing.Size(220, 24);
      this.showClusterSettingsToolStripMenuItem.Text = "Show Cluster Settings";
      // 
      // toolStripMenuItem2
      // 
      this.toolStripMenuItem2.Name = "toolStripMenuItem2";
      this.toolStripMenuItem2.Size = new System.Drawing.Size(217, 6);
      // 
      // createSubpartitionToolStripMenuItem
      // 
      this.createSubpartitionToolStripMenuItem.Name = "createSubpartitionToolStripMenuItem";
      this.createSubpartitionToolStripMenuItem.Size = new System.Drawing.Size(220, 24);
      this.createSubpartitionToolStripMenuItem.Text = "Add Sub-Partition";
      // 
      // deleteRowToolStripMenuItem
      // 
      this.deleteRowToolStripMenuItem.Name = "deleteRowToolStripMenuItem";
      this.deleteRowToolStripMenuItem.Size = new System.Drawing.Size(220, 24);
      this.deleteRowToolStripMenuItem.Text = "Delete Row";
      // 
      // insertsTabPage
      // 
      this.insertsTabPage.BackColor = System.Drawing.Color.White;
      this.insertsTabPage.Location = new System.Drawing.Point(0, 0);
      this.insertsTabPage.Margin = new System.Windows.Forms.Padding(0);
      this.insertsTabPage.Name = "insertsTabPage";
      this.insertsTabPage.Size = new System.Drawing.Size(730, 309);
      this.insertsTabPage.TabIndex = 6;
      this.insertsTabPage.Text = "Inserts";
      this.insertsTabPage.Enter += new System.EventHandler(this.insertsTabPage_Enter);
      // 
      // optionsTabPage
      // 
      this.optionsTabPage.AutoScroll = true;
      this.optionsTabPage.BackColor = System.Drawing.Color.White;
      this.optionsTabPage.Controls.Add(this.panel8);
      this.optionsTabPage.Controls.Add(this.panel7);
      this.optionsTabPage.Controls.Add(this.panel6);
      this.optionsTabPage.Controls.Add(this.panel5);
      this.optionsTabPage.Location = new System.Drawing.Point(0, 0);
      this.optionsTabPage.Name = "optionsTabPage";
      this.optionsTabPage.Padding = new System.Windows.Forms.Padding(3);
      this.optionsTabPage.Size = new System.Drawing.Size(730, 309);
      this.optionsTabPage.TabIndex = 5;
      this.optionsTabPage.Text = "Options";
      // 
      // panel8
      // 
      this.panel8.Controls.Add(this.optMergeMethod);
      this.panel8.Controls.Add(this.label48);
      this.panel8.Controls.Add(this.label49);
      this.panel8.Controls.Add(this.label46);
      this.panel8.Controls.Add(this.label47);
      this.panel8.Controls.Add(this.optUnionTables);
      this.panel8.Controls.Add(this.label45);
      this.panel8.Controls.Add(this.bevel11);
      this.panel8.Dock = System.Windows.Forms.DockStyle.Top;
      this.panel8.Location = new System.Drawing.Point(3, 429);
      this.panel8.Name = "panel8";
      this.panel8.Size = new System.Drawing.Size(703, 90);
      this.panel8.TabIndex = 5;
      // 
      // optMergeMethod
      // 
      this.optMergeMethod.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optMergeMethod.FormattingEnabled = true;
      this.optMergeMethod.Items.AddRange(new object[] {
            "Don\'t Use",
            "First Table",
            "Last Table"});
      this.optMergeMethod.Location = new System.Drawing.Point(119, 46);
      this.optMergeMethod.Name = "optMergeMethod";
      this.optMergeMethod.Size = new System.Drawing.Size(204, 25);
      this.optMergeMethod.TabIndex = 1;
      this.optMergeMethod.SelectedIndexChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label48
      // 
      this.label48.Location = new System.Drawing.Point(9, 46);
      this.label48.Name = "label48";
      this.label48.Size = new System.Drawing.Size(104, 21);
      this.label48.TabIndex = 25;
      this.label48.Text = "Merge Method:";
      this.label48.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label49
      // 
      this.label49.AutoEllipsis = true;
      this.label49.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label49.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label49.Location = new System.Drawing.Point(338, 46);
      this.label49.Name = "label49";
      this.label49.Size = new System.Drawing.Size(400, 22);
      this.label49.TabIndex = 26;
      this.label49.Text = "The union table which should be used for inserts.";
      this.label49.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label46
      // 
      this.label46.AutoEllipsis = true;
      this.label46.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label46.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label46.Location = new System.Drawing.Point(338, 19);
      this.label46.Name = "label46";
      this.label46.Size = new System.Drawing.Size(400, 22);
      this.label46.TabIndex = 23;
      this.label46.Text = "Comma separated list of MyISAM tables that should be used by the MERGE table. Enc" +
    "lose the list with parentheses.";
      this.label46.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label47
      // 
      this.label47.Location = new System.Drawing.Point(9, 19);
      this.label47.Name = "label47";
      this.label47.Size = new System.Drawing.Size(104, 21);
      this.label47.TabIndex = 21;
      this.label47.Text = "Union Tables:";
      this.label47.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // optUnionTables
      // 
      this.optUnionTables.Location = new System.Drawing.Point(119, 19);
      this.optUnionTables.Name = "optUnionTables";
      this.optUnionTables.Size = new System.Drawing.Size(204, 24);
      this.optUnionTables.TabIndex = 0;
      this.optUnionTables.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label45
      // 
      this.label45.AutoSize = true;
      this.label45.Location = new System.Drawing.Point(0, 0);
      this.label45.Name = "label45";
      this.label45.Size = new System.Drawing.Size(132, 17);
      this.label45.TabIndex = 6;
      this.label45.Text = "Merge Table Options";
      // 
      // bevel11
      // 
      this.bevel11.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel11.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel11.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel11.Location = new System.Drawing.Point(0, 0);
      this.bevel11.Name = "bevel11";
      this.bevel11.Size = new System.Drawing.Size(703, 13);
      this.bevel11.TabIndex = 5;
      this.bevel11.Text = "bevel11";
      // 
      // panel7
      // 
      this.panel7.Controls.Add(this.label41);
      this.panel7.Controls.Add(this.optIndexDirectory);
      this.panel7.Controls.Add(this.label42);
      this.panel7.Controls.Add(this.label43);
      this.panel7.Controls.Add(this.label44);
      this.panel7.Controls.Add(this.optDataDirectory);
      this.panel7.Controls.Add(this.label40);
      this.panel7.Controls.Add(this.bevel10);
      this.panel7.Dock = System.Windows.Forms.DockStyle.Top;
      this.panel7.Location = new System.Drawing.Point(3, 345);
      this.panel7.Name = "panel7";
      this.panel7.Size = new System.Drawing.Size(703, 84);
      this.panel7.TabIndex = 4;
      // 
      // label41
      // 
      this.label41.AutoEllipsis = true;
      this.label41.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label41.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label41.Location = new System.Drawing.Point(338, 46);
      this.label41.Name = "label41";
      this.label41.Size = new System.Drawing.Size(400, 22);
      this.label41.TabIndex = 23;
      this.label41.Text = "Directory where to put the tables index file. This works only for MyISAM tables o" +
    "nly and not on some operating systems (Windows).";
      this.label41.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // optIndexDirectory
      // 
      this.optIndexDirectory.Location = new System.Drawing.Point(119, 46);
      this.optIndexDirectory.Name = "optIndexDirectory";
      this.optIndexDirectory.Size = new System.Drawing.Size(204, 24);
      this.optIndexDirectory.TabIndex = 1;
      this.optIndexDirectory.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label42
      // 
      this.label42.Location = new System.Drawing.Point(9, 46);
      this.label42.Name = "label42";
      this.label42.Size = new System.Drawing.Size(104, 21);
      this.label42.TabIndex = 21;
      this.label42.Text = "Index Directory:";
      this.label42.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label43
      // 
      this.label43.AutoEllipsis = true;
      this.label43.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label43.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label43.Location = new System.Drawing.Point(338, 19);
      this.label43.Name = "label43";
      this.label43.Size = new System.Drawing.Size(400, 22);
      this.label43.TabIndex = 20;
      this.label43.Text = "Directory where to put the tables data file. This works only for MyISAM tables on" +
    "ly and not on some operating systems (Windows).";
      this.label43.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label44
      // 
      this.label44.Location = new System.Drawing.Point(9, 19);
      this.label44.Name = "label44";
      this.label44.Size = new System.Drawing.Size(104, 21);
      this.label44.TabIndex = 18;
      this.label44.Text = "Data Directory:";
      this.label44.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // optDataDirectory
      // 
      this.optDataDirectory.Location = new System.Drawing.Point(119, 19);
      this.optDataDirectory.Name = "optDataDirectory";
      this.optDataDirectory.Size = new System.Drawing.Size(204, 24);
      this.optDataDirectory.TabIndex = 0;
      this.optDataDirectory.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label40
      // 
      this.label40.AutoSize = true;
      this.label40.Location = new System.Drawing.Point(0, 0);
      this.label40.Name = "label40";
      this.label40.Size = new System.Drawing.Size(107, 17);
      this.label40.TabIndex = 4;
      this.label40.Text = "Storage Options";
      // 
      // bevel10
      // 
      this.bevel10.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel10.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel10.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel10.Location = new System.Drawing.Point(0, 0);
      this.bevel10.Name = "bevel10";
      this.bevel10.Size = new System.Drawing.Size(703, 13);
      this.bevel10.TabIndex = 3;
      this.bevel10.Text = "bevel10";
      // 
      // panel6
      // 
      this.panel6.Controls.Add(this.optKeyBlockSize);
      this.panel6.Controls.Add(this.label5);
      this.panel6.Controls.Add(this.label56);
      this.panel6.Controls.Add(this.label29);
      this.panel6.Controls.Add(this.label39);
      this.panel6.Controls.Add(this.optUseChecksum);
      this.panel6.Controls.Add(this.bevel9);
      this.panel6.Controls.Add(this.label37);
      this.panel6.Controls.Add(this.label33);
      this.panel6.Controls.Add(this.label30);
      this.panel6.Controls.Add(this.optRowFormat);
      this.panel6.Controls.Add(this.optMaxRows);
      this.panel6.Controls.Add(this.label35);
      this.panel6.Controls.Add(this.label36);
      this.panel6.Controls.Add(this.label34);
      this.panel6.Controls.Add(this.optMinRows);
      this.panel6.Controls.Add(this.optAvgRowLength);
      this.panel6.Controls.Add(this.label31);
      this.panel6.Controls.Add(this.label32);
      this.panel6.Dock = System.Windows.Forms.DockStyle.Top;
      this.panel6.Location = new System.Drawing.Point(3, 136);
      this.panel6.Name = "panel6";
      this.panel6.Size = new System.Drawing.Size(703, 209);
      this.panel6.TabIndex = 3;
      // 
      // optKeyBlockSize
      // 
      this.optKeyBlockSize.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optKeyBlockSize.FormattingEnabled = true;
      this.optKeyBlockSize.Items.AddRange(new object[] {
            "Default",
            "1 KB",
            "2 KB",
            "4 KB",
            "8 KB",
            "16 KB"});
      this.optKeyBlockSize.Location = new System.Drawing.Point(119, 52);
      this.optKeyBlockSize.Name = "optKeyBlockSize";
      this.optKeyBlockSize.Size = new System.Drawing.Size(204, 25);
      this.optKeyBlockSize.TabIndex = 20;
      this.optKeyBlockSize.SelectedIndexChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label5
      // 
      this.label5.Location = new System.Drawing.Point(9, 52);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(104, 21);
      this.label5.TabIndex = 21;
      this.label5.Text = "Key Block Size:";
      this.label5.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label56
      // 
      this.label56.AutoEllipsis = true;
      this.label56.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label56.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label56.Location = new System.Drawing.Point(338, 52);
      this.label56.Name = "label56";
      this.label56.Size = new System.Drawing.Size(400, 22);
      this.label56.TabIndex = 22;
      this.label56.Text = "Block size for COMPRESSED format.";
      this.label56.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label29
      // 
      this.label29.AutoEllipsis = true;
      this.label29.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label29.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label29.Location = new System.Drawing.Point(338, 165);
      this.label29.Name = "label29";
      this.label29.Size = new System.Drawing.Size(400, 22);
      this.label29.TabIndex = 19;
      this.label29.Text = "Activate this option if you want MySQL to maintain a live checksum for all rows. " +
    "This makes the table a little slower to update, but also makes it easier to find" +
    " corrupted tables.";
      this.label29.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label39
      // 
      this.label39.AutoSize = true;
      this.label39.Location = new System.Drawing.Point(0, 0);
      this.label39.Name = "label39";
      this.label39.Size = new System.Drawing.Size(86, 17);
      this.label39.TabIndex = 2;
      this.label39.Text = "Row Options";
      // 
      // optUseChecksum
      // 
      this.optUseChecksum.AutoSize = true;
      this.optUseChecksum.Location = new System.Drawing.Point(119, 170);
      this.optUseChecksum.Name = "optUseChecksum";
      this.optUseChecksum.Size = new System.Drawing.Size(120, 21);
      this.optUseChecksum.TabIndex = 4;
      this.optUseChecksum.Text = "Use Checksum";
      this.optUseChecksum.UseVisualStyleBackColor = true;
      this.optUseChecksum.CheckedChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // bevel9
      // 
      this.bevel9.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel9.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel9.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel9.Location = new System.Drawing.Point(0, 0);
      this.bevel9.Name = "bevel9";
      this.bevel9.Size = new System.Drawing.Size(703, 13);
      this.bevel9.TabIndex = 1;
      this.bevel9.Text = "bevel9";
      // 
      // label37
      // 
      this.label37.AutoEllipsis = true;
      this.label37.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label37.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label37.Location = new System.Drawing.Point(338, 139);
      this.label37.Name = "label37";
      this.label37.Size = new System.Drawing.Size(400, 22);
      this.label37.TabIndex = 17;
      this.label37.Text = "The maximum number of rows you plan to store in the table.";
      this.label37.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label33
      // 
      this.label33.Location = new System.Drawing.Point(9, 85);
      this.label33.Name = "label33";
      this.label33.Size = new System.Drawing.Size(104, 21);
      this.label33.TabIndex = 12;
      this.label33.Text = "Avg. Row Length:";
      this.label33.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label30
      // 
      this.label30.AutoEllipsis = true;
      this.label30.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label30.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label30.Location = new System.Drawing.Point(338, 112);
      this.label30.Name = "label30";
      this.label30.Size = new System.Drawing.Size(400, 22);
      this.label30.TabIndex = 17;
      this.label30.Text = "The minimum number of rows you plan to store in the table.";
      this.label30.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // optRowFormat
      // 
      this.optRowFormat.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optRowFormat.FormattingEnabled = true;
      this.optRowFormat.Items.AddRange(new object[] {
            "Don\'t Use",
            "Default",
            "Dynamic",
            "Fixed",
            "Compressed",
            "Redundant",
            "Compact"});
      this.optRowFormat.Location = new System.Drawing.Point(119, 19);
      this.optRowFormat.Name = "optRowFormat";
      this.optRowFormat.Size = new System.Drawing.Size(204, 25);
      this.optRowFormat.TabIndex = 0;
      this.optRowFormat.SelectedIndexChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // optMaxRows
      // 
      this.optMaxRows.Location = new System.Drawing.Point(119, 139);
      this.optMaxRows.Name = "optMaxRows";
      this.optMaxRows.Size = new System.Drawing.Size(204, 24);
      this.optMaxRows.TabIndex = 3;
      this.optMaxRows.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label35
      // 
      this.label35.Location = new System.Drawing.Point(9, 19);
      this.label35.Name = "label35";
      this.label35.Size = new System.Drawing.Size(104, 21);
      this.label35.TabIndex = 10;
      this.label35.Text = "Row Format:";
      this.label35.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label36
      // 
      this.label36.Location = new System.Drawing.Point(9, 139);
      this.label36.Name = "label36";
      this.label36.Size = new System.Drawing.Size(104, 21);
      this.label36.TabIndex = 15;
      this.label36.Text = "Max. Rows:";
      this.label36.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label34
      // 
      this.label34.AutoEllipsis = true;
      this.label34.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label34.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label34.Location = new System.Drawing.Point(338, 19);
      this.label34.Name = "label34";
      this.label34.Size = new System.Drawing.Size(400, 22);
      this.label34.TabIndex = 11;
      this.label34.Text = resources.GetString("label34.Text");
      this.label34.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // optMinRows
      // 
      this.optMinRows.Location = new System.Drawing.Point(119, 112);
      this.optMinRows.Name = "optMinRows";
      this.optMinRows.Size = new System.Drawing.Size(204, 24);
      this.optMinRows.TabIndex = 2;
      this.optMinRows.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // optAvgRowLength
      // 
      this.optAvgRowLength.Location = new System.Drawing.Point(119, 85);
      this.optAvgRowLength.Name = "optAvgRowLength";
      this.optAvgRowLength.Size = new System.Drawing.Size(204, 24);
      this.optAvgRowLength.TabIndex = 1;
      this.optAvgRowLength.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label31
      // 
      this.label31.Location = new System.Drawing.Point(9, 112);
      this.label31.Name = "label31";
      this.label31.Size = new System.Drawing.Size(104, 21);
      this.label31.TabIndex = 15;
      this.label31.Text = "Min. Rows:";
      this.label31.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label32
      // 
      this.label32.AutoEllipsis = true;
      this.label32.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label32.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label32.Location = new System.Drawing.Point(338, 85);
      this.label32.Name = "label32";
      this.label32.Size = new System.Drawing.Size(400, 22);
      this.label32.TabIndex = 14;
      this.label32.Text = "An approximation of the average row length for your table. You need to set this o" +
    "nly for large tables with variable-size records.";
      this.label32.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // panel5
      // 
      this.panel5.Controls.Add(this.label28);
      this.panel5.Controls.Add(this.label38);
      this.panel5.Controls.Add(this.optDelayKeyUpdates);
      this.panel5.Controls.Add(this.bevel8);
      this.panel5.Controls.Add(this.label26);
      this.panel5.Controls.Add(this.label23);
      this.panel5.Controls.Add(this.optAutoIncrement);
      this.panel5.Controls.Add(this.optPackKeys);
      this.panel5.Controls.Add(this.label27);
      this.panel5.Controls.Add(this.label22);
      this.panel5.Controls.Add(this.label24);
      this.panel5.Controls.Add(this.label25);
      this.panel5.Controls.Add(this.optTablePassword);
      this.panel5.Dock = System.Windows.Forms.DockStyle.Top;
      this.panel5.Location = new System.Drawing.Point(3, 3);
      this.panel5.Name = "panel5";
      this.panel5.Size = new System.Drawing.Size(703, 133);
      this.panel5.TabIndex = 2;
      // 
      // label28
      // 
      this.label28.AutoEllipsis = true;
      this.label28.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label28.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label28.Location = new System.Drawing.Point(338, 99);
      this.label28.Name = "label28";
      this.label28.Size = new System.Drawing.Size(400, 22);
      this.label28.TabIndex = 19;
      this.label28.Text = "Use this option to delay the key updates until the table is closed. This works fo" +
    "r MyISAM only.";
      this.label28.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label38
      // 
      this.label38.AutoSize = true;
      this.label38.Location = new System.Drawing.Point(0, 0);
      this.label38.Name = "label38";
      this.label38.Size = new System.Drawing.Size(104, 17);
      this.label38.TabIndex = 1;
      this.label38.Text = "General Options";
      // 
      // optDelayKeyUpdates
      // 
      this.optDelayKeyUpdates.AutoSize = true;
      this.optDelayKeyUpdates.Location = new System.Drawing.Point(119, 104);
      this.optDelayKeyUpdates.Name = "optDelayKeyUpdates";
      this.optDelayKeyUpdates.Size = new System.Drawing.Size(145, 21);
      this.optDelayKeyUpdates.TabIndex = 3;
      this.optDelayKeyUpdates.Text = "Delay Key Updates";
      this.optDelayKeyUpdates.UseVisualStyleBackColor = true;
      this.optDelayKeyUpdates.CheckedChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // bevel8
      // 
      this.bevel8.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel8.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel8.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel8.Location = new System.Drawing.Point(0, 0);
      this.bevel8.Name = "bevel8";
      this.bevel8.Size = new System.Drawing.Size(703, 13);
      this.bevel8.TabIndex = 0;
      this.bevel8.Text = "bevel8";
      // 
      // label26
      // 
      this.label26.AutoEllipsis = true;
      this.label26.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label26.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label26.Location = new System.Drawing.Point(338, 70);
      this.label26.Name = "label26";
      this.label26.Size = new System.Drawing.Size(400, 22);
      this.label26.TabIndex = 17;
      this.label26.Text = "The initial AUTO_INCREMENT value for the table.";
      this.label26.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label23
      // 
      this.label23.Location = new System.Drawing.Point(9, 16);
      this.label23.Name = "label23";
      this.label23.Size = new System.Drawing.Size(104, 21);
      this.label23.TabIndex = 10;
      this.label23.Text = "Pack Keys:";
      this.label23.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // optAutoIncrement
      // 
      this.optAutoIncrement.Location = new System.Drawing.Point(119, 70);
      this.optAutoIncrement.Name = "optAutoIncrement";
      this.optAutoIncrement.Size = new System.Drawing.Size(204, 24);
      this.optAutoIncrement.TabIndex = 2;
      this.optAutoIncrement.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // optPackKeys
      // 
      this.optPackKeys.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optPackKeys.FormattingEnabled = true;
      this.optPackKeys.Items.AddRange(new object[] {
            "Don\'t use",
            "Default",
            "Pack None",
            "Pack All"});
      this.optPackKeys.Location = new System.Drawing.Point(119, 16);
      this.optPackKeys.Name = "optPackKeys";
      this.optPackKeys.Size = new System.Drawing.Size(204, 25);
      this.optPackKeys.TabIndex = 0;
      this.optPackKeys.SelectedIndexChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label27
      // 
      this.label27.Location = new System.Drawing.Point(9, 70);
      this.label27.Name = "label27";
      this.label27.Size = new System.Drawing.Size(104, 21);
      this.label27.TabIndex = 15;
      this.label27.Text = "Auto Increment:";
      this.label27.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label22
      // 
      this.label22.AutoEllipsis = true;
      this.label22.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label22.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label22.Location = new System.Drawing.Point(338, 16);
      this.label22.Name = "label22";
      this.label22.Size = new System.Drawing.Size(400, 22);
      this.label22.TabIndex = 11;
      this.label22.Text = "Use this option to generate smaller indices. This usually makes updates slower an" +
    "d reads faster. Setting it to DEFAULT tells the storage engine to only pack long" +
    " CHAR/VARCHAR columns.";
      this.label22.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label24
      // 
      this.label24.AutoEllipsis = true;
      this.label24.Font = new System.Drawing.Font("Tahoma", 6.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label24.ForeColor = System.Drawing.SystemColors.ControlDarkDark;
      this.label24.Location = new System.Drawing.Point(338, 43);
      this.label24.Name = "label24";
      this.label24.Size = new System.Drawing.Size(400, 22);
      this.label24.TabIndex = 14;
      this.label24.Text = "Password to encrypt the table definition file. This option does not do anything i" +
    "n the standard MySQL version.";
      this.label24.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label25
      // 
      this.label25.Location = new System.Drawing.Point(9, 43);
      this.label25.Name = "label25";
      this.label25.Size = new System.Drawing.Size(104, 21);
      this.label25.TabIndex = 12;
      this.label25.Text = "Table Password:";
      this.label25.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // optTablePassword
      // 
      this.optTablePassword.Location = new System.Drawing.Point(119, 43);
      this.optTablePassword.Name = "optTablePassword";
      this.optTablePassword.Size = new System.Drawing.Size(204, 24);
      this.optTablePassword.TabIndex = 1;
      this.optTablePassword.TextChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // partitioningTabPage
      // 
      this.partitioningTabPage.BackColor = System.Drawing.Color.White;
      this.partitioningTabPage.Controls.Add(this.tableLayoutPanel2);
      this.partitioningTabPage.Location = new System.Drawing.Point(0, 0);
      this.partitioningTabPage.Name = "partitioningTabPage";
      this.partitioningTabPage.Padding = new System.Windows.Forms.Padding(3);
      this.partitioningTabPage.Size = new System.Drawing.Size(730, 309);
      this.partitioningTabPage.TabIndex = 4;
      this.partitioningTabPage.Text = "Partitioning";
      // 
      // tableLayoutPanel2
      // 
      this.tableLayoutPanel2.AutoSize = true;
      this.tableLayoutPanel2.ColumnCount = 7;
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel2.Controls.Add(this.panel4, 0, 0);
      this.tableLayoutPanel2.Controls.Add(this.partManual, 6, 1);
      this.tableLayoutPanel2.Controls.Add(this.subpartManual, 6, 2);
      this.tableLayoutPanel2.Controls.Add(this.label50, 0, 1);
      this.tableLayoutPanel2.Controls.Add(this.label54, 2, 1);
      this.tableLayoutPanel2.Controls.Add(this.partFunction, 1, 1);
      this.tableLayoutPanel2.Controls.Add(this.subpartParams, 3, 2);
      this.tableLayoutPanel2.Controls.Add(this.partParams, 3, 1);
      this.tableLayoutPanel2.Controls.Add(this.label55, 4, 1);
      this.tableLayoutPanel2.Controls.Add(this.partCount, 5, 1);
      this.tableLayoutPanel2.Controls.Add(this.label52, 0, 2);
      this.tableLayoutPanel2.Controls.Add(this.subpartFunction, 1, 2);
      this.tableLayoutPanel2.Controls.Add(this.label51, 4, 2);
      this.tableLayoutPanel2.Controls.Add(this.label53, 2, 2);
      this.tableLayoutPanel2.Controls.Add(this.subpartCount, 5, 2);
      this.tableLayoutPanel2.Controls.Add(this.partitionTreeView, 0, 3);
      this.tableLayoutPanel2.Dock = System.Windows.Forms.DockStyle.Fill;
      this.tableLayoutPanel2.Location = new System.Drawing.Point(3, 3);
      this.tableLayoutPanel2.Name = "tableLayoutPanel2";
      this.tableLayoutPanel2.RowCount = 4;
      this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel2.Size = new System.Drawing.Size(724, 303);
      this.tableLayoutPanel2.TabIndex = 0;
      // 
      // panel4
      // 
      this.panel4.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.panel4.AutoSize = true;
      this.tableLayoutPanel2.SetColumnSpan(this.panel4, 7);
      this.panel4.Controls.Add(this.partEnable);
      this.panel4.Controls.Add(this.bevel12);
      this.panel4.Location = new System.Drawing.Point(3, 3);
      this.panel4.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
      this.panel4.Name = "panel4";
      this.panel4.Size = new System.Drawing.Size(718, 23);
      this.panel4.TabIndex = 44;
      // 
      // partEnable
      // 
      this.partEnable.AutoSize = true;
      this.partEnable.Location = new System.Drawing.Point(0, -1);
      this.partEnable.Name = "partEnable";
      this.partEnable.Size = new System.Drawing.Size(142, 21);
      this.partEnable.TabIndex = 0;
      this.partEnable.Text = "Enable Partitioning";
      this.partEnable.UseVisualStyleBackColor = true;
      this.partEnable.CheckedChanged += new System.EventHandler(this.partEnable_CheckedChanged);
      // 
      // bevel12
      // 
      this.bevel12.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel12.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel12.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel12.Location = new System.Drawing.Point(0, 0);
      this.bevel12.Name = "bevel12";
      this.bevel12.Size = new System.Drawing.Size(718, 10);
      this.bevel12.TabIndex = 4;
      this.bevel12.Text = "bevel12";
      // 
      // label50
      // 
      this.label50.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.label50.Location = new System.Drawing.Point(3, 31);
      this.label50.Name = "label50";
      this.label50.Size = new System.Drawing.Size(104, 21);
      this.label50.TabIndex = 31;
      this.label50.Text = "Partition By:";
      this.label50.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label54
      // 
      this.label54.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.label54.Location = new System.Drawing.Point(233, 31);
      this.label54.Name = "label54";
      this.label54.Size = new System.Drawing.Size(70, 21);
      this.label54.TabIndex = 38;
      this.label54.Text = "Parameters:";
      this.label54.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label55
      // 
      this.label55.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.label55.Location = new System.Drawing.Point(487, 31);
      this.label55.Name = "label55";
      this.label55.Size = new System.Drawing.Size(104, 21);
      this.label55.TabIndex = 34;
      this.label55.Text = "Partition Count:";
      this.label55.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // partCount
      // 
      this.partCount.Enabled = false;
      this.partCount.Location = new System.Drawing.Point(597, 29);
      this.partCount.Name = "partCount";
      this.partCount.Size = new System.Drawing.Size(46, 24);
      this.partCount.TabIndex = 3;
      this.partCount.TextChanged += new System.EventHandler(this.partCount_Changed);
      // 
      // label52
      // 
      this.label52.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.label52.Location = new System.Drawing.Point(3, 62);
      this.label52.Name = "label52";
      this.label52.Size = new System.Drawing.Size(104, 21);
      this.label52.TabIndex = 37;
      this.label52.Text = "Subpartition By:";
      this.label52.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label51
      // 
      this.label51.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.label51.Location = new System.Drawing.Point(487, 62);
      this.label51.Name = "label51";
      this.label51.Size = new System.Drawing.Size(104, 21);
      this.label51.TabIndex = 40;
      this.label51.Text = "Subpartition Count:";
      this.label51.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label53
      // 
      this.label53.Anchor = System.Windows.Forms.AnchorStyles.Left;
      this.label53.Location = new System.Drawing.Point(233, 62);
      this.label53.Name = "label53";
      this.label53.Size = new System.Drawing.Size(70, 21);
      this.label53.TabIndex = 6;
      this.label53.Text = "Parameters:";
      this.label53.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // subpartCount
      // 
      this.subpartCount.Enabled = false;
      this.subpartCount.Location = new System.Drawing.Point(597, 60);
      this.subpartCount.Name = "subpartCount";
      this.subpartCount.Size = new System.Drawing.Size(46, 24);
      this.subpartCount.TabIndex = 8;
      this.subpartCount.TextChanged += new System.EventHandler(this.subpartCount_Changed);
      // 
      // partitionTreeView
      // 
      this.partitionTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.partitionTreeView.Columns.Add(this.partNodeStateColumn);
      this.partitionTreeView.Columns.Add(this.partNameTreeColumn);
      this.partitionTreeView.Columns.Add(this.partValuesTreeColumn);
      this.partitionTreeView.Columns.Add(this.partDataDirTreeColumn);
      this.partitionTreeView.Columns.Add(this.partIndexDirTreeColumn);
      this.partitionTreeView.Columns.Add(this.partMinRowsTreeColumn);
      this.partitionTreeView.Columns.Add(this.partMaxRowsTreeColumn);
      this.partitionTreeView.Columns.Add(this.partCommentTreeColumn);
      this.partitionTreeView.Columns.Add(this.partEngineTreeColumn);
      this.partitionTreeView.Columns.Add(this.partTablespaceTreeColumn);
      this.partitionTreeView.Columns.Add(this.partNodegroupTreeColumn);
      this.tableLayoutPanel2.SetColumnSpan(this.partitionTreeView, 7);
      this.partitionTreeView.DefaultToolTipProvider = null;
      this.partitionTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.partitionTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.partitionTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.partitionTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.partitionTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.partitionTreeView.LoadOnDemand = true;
      this.partitionTreeView.Location = new System.Drawing.Point(3, 91);
      this.partitionTreeView.Model = null;
      this.partitionTreeView.Name = "partitionTreeView";
      this.partitionTreeView.NodeControls.Add(this.partNodeStateIcon);
      this.partitionTreeView.NodeControls.Add(this.partNameNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partValuesNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partDataDirNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partIndexDirNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partMinRowsNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partMaxRowsNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partCommentNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partEngineNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partTablespaceNodeControl);
      this.partitionTreeView.NodeControls.Add(this.partNodegroupNodeControl);
      this.partitionTreeView.ScrollPosition = new System.Drawing.Point(0, 0);
      this.partitionTreeView.SelectedNode = null;
      this.partitionTreeView.ShowHeader = true;
      this.partitionTreeView.ShowLines = false;
      this.partitionTreeView.ShowPlusMinus = false;
      this.partitionTreeView.Size = new System.Drawing.Size(718, 221);
      this.partitionTreeView.TabIndex = 0;
      this.partitionTreeView.UseColumns = true;
      // 
      // triggersTabPage
      // 
      this.triggersTabPage.BackColor = System.Drawing.Color.White;
      this.triggersTabPage.Location = new System.Drawing.Point(0, 0);
      this.triggersTabPage.Name = "triggersTabPage";
      this.triggersTabPage.Padding = new System.Windows.Forms.Padding(3);
      this.triggersTabPage.Size = new System.Drawing.Size(730, 309);
      this.triggersTabPage.TabIndex = 3;
      this.triggersTabPage.Text = "Triggers";
      // 
      // foreignKeysTabPage
      // 
      this.foreignKeysTabPage.BackColor = System.Drawing.Color.White;
      this.foreignKeysTabPage.Controls.Add(this.foreignKeyWarningPanel);
      this.foreignKeysTabPage.Controls.Add(this.foreignKeyPageSplitContainer);
      this.foreignKeysTabPage.Location = new System.Drawing.Point(0, 0);
      this.foreignKeysTabPage.Name = "foreignKeysTabPage";
      this.foreignKeysTabPage.Padding = new System.Windows.Forms.Padding(3);
      this.foreignKeysTabPage.Size = new System.Drawing.Size(730, 309);
      this.foreignKeysTabPage.TabIndex = 2;
      this.foreignKeysTabPage.Text = "Foreign Keys";
      // 
      // foreignKeyWarningPanel
      // 
      this.foreignKeyWarningPanel.BackColor = System.Drawing.Color.White;
      this.foreignKeyWarningPanel.Controls.Add(this.label11);
      this.foreignKeyWarningPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.foreignKeyWarningPanel.Location = new System.Drawing.Point(3, 3);
      this.foreignKeyWarningPanel.Name = "foreignKeyWarningPanel";
      this.foreignKeyWarningPanel.Size = new System.Drawing.Size(724, 303);
      this.foreignKeyWarningPanel.TabIndex = 1;
      // 
      // label11
      // 
      this.label11.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.label11.Location = new System.Drawing.Point(120, 43);
      this.label11.Name = "label11";
      this.label11.Size = new System.Drawing.Size(498, 80);
      this.label11.TabIndex = 0;
      this.label11.Text = resources.GetString("label11.Text");
      // 
      // foreignKeyPageSplitContainer
      // 
      this.foreignKeyPageSplitContainer.BackColor = System.Drawing.Color.White;
      this.foreignKeyPageSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.foreignKeyPageSplitContainer.Location = new System.Drawing.Point(3, 3);
      this.foreignKeyPageSplitContainer.Name = "foreignKeyPageSplitContainer";
      // 
      // foreignKeyPageSplitContainer.Panel1
      // 
      this.foreignKeyPageSplitContainer.Panel1.Controls.Add(this.fkTreeView);
      // 
      // foreignKeyPageSplitContainer.Panel2
      // 
      this.foreignKeyPageSplitContainer.Panel2.Controls.Add(this.fkColumnsTreeView);
      this.foreignKeyPageSplitContainer.Panel2.Controls.Add(this.panel3);
      this.foreignKeyPageSplitContainer.Size = new System.Drawing.Size(724, 303);
      this.foreignKeyPageSplitContainer.SplitterDistance = 286;
      this.foreignKeyPageSplitContainer.TabIndex = 2;
      // 
      // fkTreeView
      // 
      this.fkTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.fkTreeView.Columns.Add(this.nameFkColumn);
      this.fkTreeView.Columns.Add(this.targetFkColumn);
      this.fkTreeView.ContextMenuStrip = this.fksContextMenuStrip;
      this.fkTreeView.DefaultToolTipProvider = null;
      this.fkTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.fkTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.fkTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.fkTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.fkTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.fkTreeView.LoadOnDemand = true;
      this.fkTreeView.Location = new System.Drawing.Point(0, 0);
      this.fkTreeView.Model = null;
      this.fkTreeView.Name = "fkTreeView";
      this.fkTreeView.NodeControls.Add(this.nameFkNodeControl);
      this.fkTreeView.NodeControls.Add(this.targetFkNodeControl);
      this.fkTreeView.ScrollPosition = new System.Drawing.Point(0, 0);
      this.fkTreeView.SelectedNode = null;
      this.fkTreeView.SelectionMode = Aga.Controls.Tree.TreeSelectionMode.Multi;
      this.fkTreeView.ShowHeader = true;
      this.fkTreeView.ShowLines = false;
      this.fkTreeView.ShowNodeToolTips = true;
      this.fkTreeView.ShowPlusMinus = false;
      this.fkTreeView.Size = new System.Drawing.Size(286, 303);
      this.fkTreeView.TabIndex = 0;
      this.fkTreeView.Text = "fkTreeViewAdv";
      this.fkTreeView.UseColumns = true;
      this.fkTreeView.NodeMouseDoubleClick += new System.EventHandler<Aga.Controls.Tree.TreeNodeAdvMouseEventArgs>(this.default_NodeMouseDoubleClick);
      this.fkTreeView.SelectionChanged += new System.EventHandler(this.fkTreeView_SelectionChanged);
      this.fkTreeView.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.fkTreeView_KeyPress);
      // 
      // fkColumnsTreeView
      // 
      this.fkColumnsTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.fkColumnsTreeView.Columns.Add(this.columnFkColumn);
      this.fkColumnsTreeView.Columns.Add(this.targetColumnFkColumn);
      this.fkColumnsTreeView.DefaultToolTipProvider = null;
      this.fkColumnsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.fkColumnsTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.fkColumnsTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.fkColumnsTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.fkColumnsTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.fkColumnsTreeView.LoadOnDemand = true;
      this.fkColumnsTreeView.Location = new System.Drawing.Point(0, 0);
      this.fkColumnsTreeView.Model = null;
      this.fkColumnsTreeView.Name = "fkColumnsTreeView";
      this.fkColumnsTreeView.NodeControls.Add(this.columnEnabledFkNodeControl);
      this.fkColumnsTreeView.NodeControls.Add(this.columnFkNodeControl);
      this.fkColumnsTreeView.NodeControls.Add(this.targetColumnFkNodeControl);
      this.fkColumnsTreeView.ScrollPosition = new System.Drawing.Point(0, 0);
      this.fkColumnsTreeView.SelectedNode = null;
      this.fkColumnsTreeView.ShowHeader = true;
      this.fkColumnsTreeView.ShowLines = false;
      this.fkColumnsTreeView.ShowNodeToolTips = true;
      this.fkColumnsTreeView.ShowPlusMinus = false;
      this.fkColumnsTreeView.Size = new System.Drawing.Size(223, 303);
      this.fkColumnsTreeView.TabIndex = 0;
      this.fkColumnsTreeView.Text = "treeViewAdv2";
      this.fkColumnsTreeView.UseColumns = true;
      // 
      // panel3
      // 
      this.panel3.Controls.Add(this.splitContainer6);
      this.panel3.Dock = System.Windows.Forms.DockStyle.Right;
      this.panel3.Location = new System.Drawing.Point(223, 0);
      this.panel3.Name = "panel3";
      this.panel3.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
      this.panel3.Size = new System.Drawing.Size(211, 303);
      this.panel3.TabIndex = 1;
      // 
      // splitContainer6
      // 
      this.splitContainer6.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer6.Location = new System.Drawing.Point(3, 0);
      this.splitContainer6.Name = "splitContainer6";
      this.splitContainer6.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // splitContainer6.Panel1
      // 
      this.splitContainer6.Panel1.Controls.Add(this.label21);
      this.splitContainer6.Panel1.Controls.Add(this.fkModelOnlyCheck);
      this.splitContainer6.Panel1.Controls.Add(this.label10);
      this.splitContainer6.Panel1.Controls.Add(this.bevel7);
      this.splitContainer6.Panel1.Controls.Add(this.label20);
      this.splitContainer6.Panel1.Controls.Add(this.label18);
      this.splitContainer6.Panel1.Controls.Add(this.onDeleteActionComboBox);
      this.splitContainer6.Panel1.Controls.Add(this.onUpdateActionComboBox);
      this.splitContainer6.Panel1.Controls.Add(this.label19);
      this.splitContainer6.Panel1.Controls.Add(this.bevel6);
      // 
      // splitContainer6.Panel2
      // 
      this.splitContainer6.Panel2.Controls.Add(this.fkCommentText);
      this.splitContainer6.Size = new System.Drawing.Size(208, 303);
      this.splitContainer6.SplitterDistance = 152;
      this.splitContainer6.TabIndex = 19;
      // 
      // label21
      // 
      this.label21.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.label21.AutoSize = true;
      this.label21.Location = new System.Drawing.Point(3, 139);
      this.label21.Name = "label21";
      this.label21.Size = new System.Drawing.Size(145, 17);
      this.label21.TabIndex = 20;
      this.label21.Text = "Foreign Key Comment";
      this.label21.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // label10
      // 
      this.label10.AutoSize = true;
      this.label10.Location = new System.Drawing.Point(45, 72);
      this.label10.Name = "label10";
      this.label10.Size = new System.Drawing.Size(0, 17);
      this.label10.TabIndex = 21;
      // 
      // bevel7
      // 
      this.bevel7.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel7.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel7.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.bevel7.Location = new System.Drawing.Point(0, 139);
      this.bevel7.Name = "bevel7";
      this.bevel7.Size = new System.Drawing.Size(208, 13);
      this.bevel7.TabIndex = 19;
      this.bevel7.Text = "bevel7";
      // 
      // label20
      // 
      this.label20.Location = new System.Drawing.Point(-3, 43);
      this.label20.Name = "label20";
      this.label20.Size = new System.Drawing.Size(87, 21);
      this.label20.TabIndex = 18;
      this.label20.Text = "On Delete:";
      this.label20.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label18
      // 
      this.label18.AutoSize = true;
      this.label18.Location = new System.Drawing.Point(0, 0);
      this.label18.Name = "label18";
      this.label18.Size = new System.Drawing.Size(131, 17);
      this.label18.TabIndex = 12;
      this.label18.Text = "Foreign Key Options";
      // 
      // onDeleteActionComboBox
      // 
      this.onDeleteActionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.onDeleteActionComboBox.Enabled = false;
      this.onDeleteActionComboBox.FormattingEnabled = true;
      this.onDeleteActionComboBox.Location = new System.Drawing.Point(90, 44);
      this.onDeleteActionComboBox.Name = "onDeleteActionComboBox";
      this.onDeleteActionComboBox.Size = new System.Drawing.Size(118, 25);
      this.onDeleteActionComboBox.TabIndex = 1;
      this.onDeleteActionComboBox.SelectedIndexChanged += new System.EventHandler(this.onDeleteActionComboBox_SelectedIndexChanged);
      // 
      // onUpdateActionComboBox
      // 
      this.onUpdateActionComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.onUpdateActionComboBox.Enabled = false;
      this.onUpdateActionComboBox.FormattingEnabled = true;
      this.onUpdateActionComboBox.Location = new System.Drawing.Point(90, 17);
      this.onUpdateActionComboBox.Name = "onUpdateActionComboBox";
      this.onUpdateActionComboBox.Size = new System.Drawing.Size(118, 25);
      this.onUpdateActionComboBox.TabIndex = 0;
      this.onUpdateActionComboBox.SelectedIndexChanged += new System.EventHandler(this.onUpdateActionComboBox_SelectedIndexChanged);
      // 
      // label19
      // 
      this.label19.Location = new System.Drawing.Point(-3, 16);
      this.label19.Name = "label19";
      this.label19.Size = new System.Drawing.Size(87, 21);
      this.label19.TabIndex = 16;
      this.label19.Text = "On Update:";
      this.label19.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // bevel6
      // 
      this.bevel6.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel6.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel6.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel6.Location = new System.Drawing.Point(0, 0);
      this.bevel6.Name = "bevel6";
      this.bevel6.Size = new System.Drawing.Size(208, 13);
      this.bevel6.TabIndex = 13;
      this.bevel6.Text = "bevel6";
      // 
      // fkCommentText
      // 
      this.fkCommentText.AcceptsTab = true;
      this.fkCommentText.Dock = System.Windows.Forms.DockStyle.Fill;
      this.fkCommentText.Location = new System.Drawing.Point(0, 0);
      this.fkCommentText.Multiline = true;
      this.fkCommentText.Name = "fkCommentText";
      this.fkCommentText.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
      this.fkCommentText.Size = new System.Drawing.Size(208, 147);
      this.fkCommentText.TabIndex = 0;
      this.fkCommentText.TextChanged += new System.EventHandler(this.fkCommentText_TextChanged);
      // 
      // fkIndexLabel
      // 
      this.fkIndexLabel.AutoSize = true;
      this.fkIndexLabel.Location = new System.Drawing.Point(90, 72);
      this.fkIndexLabel.Name = "fkIndexLabel";
      this.fkIndexLabel.Size = new System.Drawing.Size(0, 13);
      this.fkIndexLabel.TabIndex = 22;
      // 
      // indicesTabPage
      // 
      this.indicesTabPage.BackColor = System.Drawing.Color.White;
      this.indicesTabPage.Controls.Add(this.splitContainer1);
      this.indicesTabPage.Location = new System.Drawing.Point(0, 0);
      this.indicesTabPage.Name = "indicesTabPage";
      this.indicesTabPage.Padding = new System.Windows.Forms.Padding(3);
      this.indicesTabPage.Size = new System.Drawing.Size(730, 309);
      this.indicesTabPage.TabIndex = 1;
      this.indicesTabPage.Text = "Indexes";
      // 
      // splitContainer1
      // 
      this.splitContainer1.BackColor = System.Drawing.Color.Transparent;
      this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer1.Location = new System.Drawing.Point(3, 3);
      this.splitContainer1.Name = "splitContainer1";
      // 
      // splitContainer1.Panel1
      // 
      this.splitContainer1.Panel1.Controls.Add(this.indicesTreeView);
      // 
      // splitContainer1.Panel2
      // 
      this.splitContainer1.Panel2.Controls.Add(this.splitContainer3);
      this.splitContainer1.Panel2.Controls.Add(this.panel2);
      this.splitContainer1.Size = new System.Drawing.Size(724, 303);
      this.splitContainer1.SplitterDistance = 241;
      this.splitContainer1.TabIndex = 3;
      // 
      // splitContainer3
      // 
      this.splitContainer3.BackColor = System.Drawing.Color.White;
      this.splitContainer3.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer3.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
      this.splitContainer3.Location = new System.Drawing.Point(0, 0);
      this.splitContainer3.Name = "splitContainer3";
      this.splitContainer3.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // splitContainer3.Panel1
      // 
      this.splitContainer3.Panel1.Controls.Add(this.label8);
      this.splitContainer3.Panel1.Controls.Add(this.bevel3);
      this.splitContainer3.Panel1MinSize = 13;
      // 
      // splitContainer3.Panel2
      // 
      this.splitContainer3.Panel2.Controls.Add(this.indexColumnsTreeView);
      this.splitContainer3.Panel2MinSize = 20;
      this.splitContainer3.Size = new System.Drawing.Size(268, 303);
      this.splitContainer3.SplitterDistance = 25;
      this.splitContainer3.TabIndex = 2;
      // 
      // label8
      // 
      this.label8.AutoSize = true;
      this.label8.Location = new System.Drawing.Point(0, 0);
      this.label8.Name = "label8";
      this.label8.Size = new System.Drawing.Size(100, 17);
      this.label8.TabIndex = 12;
      this.label8.Text = "Index Columns";
      // 
      // bevel3
      // 
      this.bevel3.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel3.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel3.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel3.Location = new System.Drawing.Point(0, 0);
      this.bevel3.Name = "bevel3";
      this.bevel3.Size = new System.Drawing.Size(268, 13);
      this.bevel3.TabIndex = 13;
      this.bevel3.Text = "bevel3";
      // 
      // indexColumnsTreeView
      // 
      this.indexColumnsTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.indexColumnsTreeView.Columns.Add(this.indexColumnNameTreeColumn);
      this.indexColumnsTreeView.Columns.Add(this.indexColumnOrderTreeColumn);
      this.indexColumnsTreeView.Columns.Add(this.indexColumnStorageTreeColumn);
      this.indexColumnsTreeView.Columns.Add(this.indexColumnLengthTreeColumn);
      this.indexColumnsTreeView.DefaultToolTipProvider = null;
      this.indexColumnsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.indexColumnsTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.indexColumnsTreeView.FullRowSelect = true;
      this.indexColumnsTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.indexColumnsTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.indexColumnsTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.indexColumnsTreeView.LoadOnDemand = true;
      this.indexColumnsTreeView.Location = new System.Drawing.Point(0, 0);
      this.indexColumnsTreeView.Model = null;
      this.indexColumnsTreeView.Name = "indexColumnsTreeView";
      this.indexColumnsTreeView.NodeControls.Add(this.indexColumnEnabledNodeControl);
      this.indexColumnsTreeView.NodeControls.Add(this.indexColumnNameNodeControl);
      this.indexColumnsTreeView.NodeControls.Add(this.indexColumnOrderNodeControl);
      this.indexColumnsTreeView.NodeControls.Add(this.indexColumnStorageNodeControl);
      this.indexColumnsTreeView.NodeControls.Add(this.indexColumnLengthNodeControl);
      this.indexColumnsTreeView.ScrollPosition = new System.Drawing.Point(0, 0);
      this.indexColumnsTreeView.SelectedNode = null;
      this.indexColumnsTreeView.ShowHeader = true;
      this.indexColumnsTreeView.ShowLines = false;
      this.indexColumnsTreeView.ShowPlusMinus = false;
      this.indexColumnsTreeView.Size = new System.Drawing.Size(268, 274);
      this.indexColumnsTreeView.TabIndex = 0;
      this.indexColumnsTreeView.Text = "indexColumnsTreeViewAdv";
      this.indexColumnsTreeView.UseColumns = true;
      this.indexColumnsTreeView.NodeMouseDoubleClick += new System.EventHandler<Aga.Controls.Tree.TreeNodeAdvMouseEventArgs>(this.default_NodeMouseDoubleClick);
      // 
      // panel2
      // 
      this.panel2.BackColor = System.Drawing.Color.Transparent;
      this.panel2.Controls.Add(this.splitContainer5);
      this.panel2.Dock = System.Windows.Forms.DockStyle.Right;
      this.panel2.Location = new System.Drawing.Point(268, 0);
      this.panel2.Name = "panel2";
      this.panel2.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
      this.panel2.Size = new System.Drawing.Size(211, 303);
      this.panel2.TabIndex = 4;
      // 
      // splitContainer5
      // 
      this.splitContainer5.BackColor = System.Drawing.Color.White;
      this.splitContainer5.Dock = System.Windows.Forms.DockStyle.Fill;
      this.splitContainer5.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
      this.splitContainer5.Location = new System.Drawing.Point(3, 0);
      this.splitContainer5.Name = "splitContainer5";
      this.splitContainer5.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // splitContainer5.Panel1
      // 
      this.splitContainer5.Panel1.Controls.Add(this.indexParserText);
      this.splitContainer5.Panel1.Controls.Add(this.label17);
      this.splitContainer5.Panel1.Controls.Add(this.label15);
      this.splitContainer5.Panel1.Controls.Add(this.indexRowBlockSizeText);
      this.splitContainer5.Panel1.Controls.Add(this.label13);
      this.splitContainer5.Panel1.Controls.Add(this.bevel4);
      this.splitContainer5.Panel1.Controls.Add(this.label14);
      this.splitContainer5.Panel1.Controls.Add(this.indexStorageTypeComboBox);
      this.splitContainer5.Panel1.Controls.Add(this.bevel5);
      this.splitContainer5.Panel1.Controls.Add(this.label16);
      // 
      // splitContainer5.Panel2
      // 
      this.splitContainer5.Panel2.Controls.Add(this.indexCommentText);
      this.splitContainer5.Size = new System.Drawing.Size(208, 303);
      this.splitContainer5.SplitterDistance = 119;
      this.splitContainer5.TabIndex = 14;
      // 
      // indexParserText
      // 
      this.indexParserText.Location = new System.Drawing.Point(96, 73);
      this.indexParserText.Name = "indexParserText";
      this.indexParserText.Size = new System.Drawing.Size(111, 24);
      this.indexParserText.TabIndex = 2;
      this.indexParserText.TextChanged += new System.EventHandler(this.indexParserText_TextChanged);
      // 
      // label17
      // 
      this.label17.Location = new System.Drawing.Point(3, 72);
      this.label17.Name = "label17";
      this.label17.Size = new System.Drawing.Size(87, 21);
      this.label17.TabIndex = 15;
      this.label17.Text = "Parser:";
      this.label17.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label15
      // 
      this.label15.Location = new System.Drawing.Point(3, 18);
      this.label15.Name = "label15";
      this.label15.Size = new System.Drawing.Size(87, 21);
      this.label15.TabIndex = 14;
      this.label15.Text = "Storage Type:";
      this.label15.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // indexRowBlockSizeText
      // 
      this.indexRowBlockSizeText.Location = new System.Drawing.Point(96, 46);
      this.indexRowBlockSizeText.Name = "indexRowBlockSizeText";
      this.indexRowBlockSizeText.Size = new System.Drawing.Size(111, 24);
      this.indexRowBlockSizeText.TabIndex = 1;
      this.indexRowBlockSizeText.TextChanged += new System.EventHandler(this.indexRowBlockSizeText_TextChanged);
      // 
      // label13
      // 
      this.label13.AutoSize = true;
      this.label13.Location = new System.Drawing.Point(0, 0);
      this.label13.Name = "label13";
      this.label13.Size = new System.Drawing.Size(94, 17);
      this.label13.TabIndex = 8;
      this.label13.Text = "Index Options";
      // 
      // bevel4
      // 
      this.bevel4.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel4.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel4.Dock = System.Windows.Forms.DockStyle.Top;
      this.bevel4.Location = new System.Drawing.Point(0, 0);
      this.bevel4.Name = "bevel4";
      this.bevel4.Size = new System.Drawing.Size(208, 13);
      this.bevel4.TabIndex = 11;
      this.bevel4.Text = "bevel4";
      // 
      // label14
      // 
      this.label14.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.label14.AutoSize = true;
      this.label14.Location = new System.Drawing.Point(0, 106);
      this.label14.Name = "label14";
      this.label14.Size = new System.Drawing.Size(108, 17);
      this.label14.TabIndex = 9;
      this.label14.Text = "Index Comment";
      this.label14.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // indexStorageTypeComboBox
      // 
      this.indexStorageTypeComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.indexStorageTypeComboBox.Enabled = false;
      this.indexStorageTypeComboBox.FormattingEnabled = true;
      this.indexStorageTypeComboBox.Location = new System.Drawing.Point(96, 19);
      this.indexStorageTypeComboBox.Name = "indexStorageTypeComboBox";
      this.indexStorageTypeComboBox.Size = new System.Drawing.Size(111, 25);
      this.indexStorageTypeComboBox.TabIndex = 0;
      this.indexStorageTypeComboBox.SelectedIndexChanged += new System.EventHandler(this.indexStorageTypeComboBox_SelectedIndexChanged);
      // 
      // bevel5
      // 
      this.bevel5.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.bevel5.BevelStyle = MySQL.Utilities.BevelStyleType.Dark;
      this.bevel5.BorderSide = System.Windows.Forms.Border3DSide.Middle;
      this.bevel5.Location = new System.Drawing.Point(0, 106);
      this.bevel5.Name = "bevel5";
      this.bevel5.Size = new System.Drawing.Size(208, 13);
      this.bevel5.TabIndex = 12;
      this.bevel5.Text = "bevel5";
      // 
      // label16
      // 
      this.label16.Location = new System.Drawing.Point(3, 45);
      this.label16.Name = "label16";
      this.label16.Size = new System.Drawing.Size(87, 21);
      this.label16.TabIndex = 6;
      this.label16.Text = "Key Block Size:";
      this.label16.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // indexCommentText
      // 
      this.indexCommentText.AcceptsTab = true;
      this.indexCommentText.Dock = System.Windows.Forms.DockStyle.Fill;
      this.indexCommentText.Location = new System.Drawing.Point(0, 0);
      this.indexCommentText.Multiline = true;
      this.indexCommentText.Name = "indexCommentText";
      this.indexCommentText.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
      this.indexCommentText.Size = new System.Drawing.Size(208, 180);
      this.indexCommentText.TabIndex = 0;
      this.indexCommentText.TextChanged += new System.EventHandler(this.indexCommentText_TextChanged);
      // 
      // columnsTabPage
      // 
      this.columnsTabPage.BackColor = System.Drawing.Color.White;
      this.columnsTabPage.Controls.Add(this.columnListSplitContainer);
      this.columnsTabPage.Controls.Add(this.bevel2);
      this.columnsTabPage.Controls.Add(this.tableLayoutPanel1);
      this.columnsTabPage.Location = new System.Drawing.Point(0, 0);
      this.columnsTabPage.Margin = new System.Windows.Forms.Padding(0);
      this.columnsTabPage.Name = "columnsTabPage";
      this.columnsTabPage.Padding = new System.Windows.Forms.Padding(8, 0, 8, 5);
      this.columnsTabPage.Size = new System.Drawing.Size(730, 309);
      this.columnsTabPage.TabIndex = 0;
      this.columnsTabPage.Text = "Columns";
      // 
      // columnListSplitContainer
      // 
      this.columnListSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.columnListSplitContainer.Location = new System.Drawing.Point(8, 0);
      this.columnListSplitContainer.Name = "columnListSplitContainer";
      // 
      // columnListSplitContainer.Panel1
      // 
      this.columnListSplitContainer.Panel1.Controls.Add(this.columnsTreeView);
      // 
      // columnListSplitContainer.Panel2
      // 
      this.columnListSplitContainer.Panel2.BackColor = System.Drawing.Color.White;
      this.columnListSplitContainer.Panel2.Controls.Add(this.oldTableLayoutPanel);
      this.columnListSplitContainer.Panel2.Padding = new System.Windows.Forms.Padding(5);
      this.columnListSplitContainer.Panel2Collapsed = true;
      this.columnListSplitContainer.Size = new System.Drawing.Size(714, 112);
      this.columnListSplitContainer.SplitterDistance = 421;
      this.columnListSplitContainer.TabIndex = 18;
      // 
      // columnsTreeView
      // 
      this.columnsTreeView.AllowColumnReorder = true;
      this.columnsTreeView.AllowDrop = true;
      this.columnsTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.columnsTreeView.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.columnsTreeView.Columns.Add(this.columnNameTreeColumn);
      this.columnsTreeView.Columns.Add(this.datatypeTreeColumn);
      this.columnsTreeView.Columns.Add(this.pkTreeColumn);
      this.columnsTreeView.Columns.Add(this.nnTreeColumn);
      this.columnsTreeView.Columns.Add(this.uqTreeColumn);
      this.columnsTreeView.Columns.Add(this.binTreeColumn);
      this.columnsTreeView.Columns.Add(this.unTreeColumn);
      this.columnsTreeView.Columns.Add(this.zfTreeColumn);
      this.columnsTreeView.Columns.Add(this.aiTreeColumn);
      this.columnsTreeView.Columns.Add(this.gTreeColumn);
      this.columnsTreeView.Columns.Add(this.defaultTreeColumn);
      this.columnsTreeView.DefaultToolTipProvider = null;
      this.columnsTreeView.DisplayDraggingNodes = true;
      this.columnsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.columnsTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.columnsTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.columnsTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.columnsTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.columnsTreeView.LoadOnDemand = true;
      this.columnsTreeView.Location = new System.Drawing.Point(0, 0);
      this.columnsTreeView.Model = null;
      this.columnsTreeView.Name = "columnsTreeView";
      this.columnsTreeView.NodeControls.Add(this.columnIconNodeControl);
      this.columnsTreeView.NodeControls.Add(this.nameNodeControl);
      this.columnsTreeView.NodeControls.Add(this.datatypeComboBoxNodeControl);
      this.columnsTreeView.NodeControls.Add(this.pkNodeControl);
      this.columnsTreeView.NodeControls.Add(this.nnNodeControl);
      this.columnsTreeView.NodeControls.Add(this.binNodeControl);
      this.columnsTreeView.NodeControls.Add(this.unNodeControl);
      this.columnsTreeView.NodeControls.Add(this.uqNodeControl);
      this.columnsTreeView.NodeControls.Add(this.zfNodeControl);
      this.columnsTreeView.NodeControls.Add(this.aiNodeControl);
      this.columnsTreeView.NodeControls.Add(this.gNodeControl);
      this.columnsTreeView.NodeControls.Add(this.defaultNodeControl);
      this.columnsTreeView.ScrollPosition = new System.Drawing.Point(0, 0);
      this.columnsTreeView.SelectedNode = null;
      this.columnsTreeView.SelectionMode = Aga.Controls.Tree.TreeSelectionMode.Multi;
      this.columnsTreeView.ShowHeader = true;
      this.columnsTreeView.ShowLines = false;
      this.columnsTreeView.ShowPlusMinus = false;
      this.columnsTreeView.Size = new System.Drawing.Size(714, 112);
      this.columnsTreeView.TabIndex = 4;
      this.columnsTreeView.Text = "columnTreeViewAdv";
      this.columnsTreeView.UseColumns = true;
      this.columnsTreeView.ItemDrag += new System.Windows.Forms.ItemDragEventHandler(this.columnsTreeView_ItemDrag);
      this.columnsTreeView.NodeMouseDoubleClick += new System.EventHandler<Aga.Controls.Tree.TreeNodeAdvMouseEventArgs>(this.columnsTreeView_NodeMouseDoubleClick);
      this.columnsTreeView.SelectionChanged += new System.EventHandler(this.columnsTreeView_SelectionChanged);
      this.columnsTreeView.DragDrop += new System.Windows.Forms.DragEventHandler(this.columnsTreeView_DragDrop);
      this.columnsTreeView.DragEnter += new System.Windows.Forms.DragEventHandler(this.columnsTreeView_DragEnter);
      this.columnsTreeView.DragOver += new System.Windows.Forms.DragEventHandler(this.columnsTreeView_DragOver);
      // 
      // pkTreeColumn
      // 
      this.pkTreeColumn.Header = "PK";
      this.pkTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.pkTreeColumn.TooltipText = "Belongs to primary key";
      this.pkTreeColumn.Width = 30;
      // 
      // uqTreeColumn
      // 
      this.uqTreeColumn.Header = "UQ";
      this.uqTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.uqTreeColumn.TooltipText = "Unique index";
      this.uqTreeColumn.Width = 30;
      // 
      // binTreeColumn
      // 
      this.binTreeColumn.Header = "B";
      this.binTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.binTreeColumn.TooltipText = "Is binary column";
      this.binTreeColumn.Width = 30;
      // 
      // unTreeColumn
      // 
      this.unTreeColumn.Header = "UN";
      this.unTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.unTreeColumn.TooltipText = "Unsigned data type";
      this.unTreeColumn.Width = 30;
      // 
      // zfTreeColumn
      // 
      this.zfTreeColumn.Header = "ZF";
      this.zfTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.zfTreeColumn.TooltipText = "Fill up values for that column with 0\'s if it is numeric";
      this.zfTreeColumn.Width = 30;
      // 
      // pkNodeControl
      // 
      this.pkNodeControl.LeftMargin = 3;
      this.pkNodeControl.ParentColumn = this.pkTreeColumn;
      this.pkNodeControl.VirtualMode = true;
      // 
      // binNodeControl
      // 
      this.binNodeControl.LeftMargin = 3;
      this.binNodeControl.ParentColumn = this.binTreeColumn;
      this.binNodeControl.VirtualMode = true;
      // 
      // unNodeControl
      // 
      this.unNodeControl.LeftMargin = 3;
      this.unNodeControl.ParentColumn = this.unTreeColumn;
      this.unNodeControl.VirtualMode = true;
      // 
      // uqNodeControl
      // 
      this.uqNodeControl.LeftMargin = 3;
      this.uqNodeControl.ParentColumn = this.uqTreeColumn;
      this.uqNodeControl.VirtualMode = true;
      // 
      // zfNodeControl
      // 
      this.zfNodeControl.LeftMargin = 3;
      this.zfNodeControl.ParentColumn = this.zfTreeColumn;
      this.zfNodeControl.VirtualMode = true;
      // 
      // oldTableLayoutPanel
      // 
      this.oldTableLayoutPanel.ColumnCount = 2;
      this.oldTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.oldTableLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
      this.oldTableLayoutPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.oldTableLayoutPanel.Location = new System.Drawing.Point(5, 5);
      this.oldTableLayoutPanel.Name = "oldTableLayoutPanel";
      this.oldTableLayoutPanel.RowCount = 2;
      this.oldTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.oldTableLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
      this.oldTableLayoutPanel.Size = new System.Drawing.Size(-10, -10);
      this.oldTableLayoutPanel.TabIndex = 0;
      // 
      // bevel2
      // 
      this.bevel2.BackColor = System.Drawing.Color.White;
      this.bevel2.BevelStyle = MySQL.Utilities.BevelStyleType.White;
      this.bevel2.BorderSide = System.Windows.Forms.Border3DSide.Bottom;
      this.bevel2.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.bevel2.Location = new System.Drawing.Point(8, 112);
      this.bevel2.Margin = new System.Windows.Forms.Padding(0);
      this.bevel2.Name = "bevel2";
      this.bevel2.Size = new System.Drawing.Size(714, 10);
      this.bevel2.TabIndex = 17;
      this.bevel2.Text = "bevel2";
      // 
      // tableLayoutPanel1
      // 
      this.tableLayoutPanel1.AutoSize = true;
      this.tableLayoutPanel1.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.tableLayoutPanel1.BackColor = System.Drawing.Color.Transparent;
      this.tableLayoutPanel1.ColumnCount = 6;
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50.51546F));
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.tableLayoutPanel1.Controls.Add(this.columnCollationComboBox, 1, 1);
      this.tableLayoutPanel1.Controls.Add(this.label12, 0, 2);
      this.tableLayoutPanel1.Controls.Add(this.label9, 0, 1);
      this.tableLayoutPanel1.Controls.Add(this.label6, 0, 0);
      this.tableLayoutPanel1.Controls.Add(this.label57, 2, 0);
      this.tableLayoutPanel1.Controls.Add(this.defaultLabel, 2, 1);
      this.tableLayoutPanel1.Controls.Add(this.columnNameTextBox, 1, 0);
      this.tableLayoutPanel1.Controls.Add(this.columnDataTypeTextBox, 3, 0);
      this.tableLayoutPanel1.Controls.Add(this.columnDefaultTextBox, 3, 1);
      this.tableLayoutPanel1.Controls.Add(this.pkCheckBox, 3, 3);
      this.tableLayoutPanel1.Controls.Add(this.nnCheckBox, 4, 3);
      this.tableLayoutPanel1.Controls.Add(this.uniqueCheckBox, 5, 3);
      this.tableLayoutPanel1.Controls.Add(this.binaryCheckBox, 3, 4);
      this.tableLayoutPanel1.Controls.Add(this.unsignedCheckBox, 4, 4);
      this.tableLayoutPanel1.Controls.Add(this.zeroFillCheckBox, 5, 4);
      this.tableLayoutPanel1.Controls.Add(this.aiCheckBox, 3, 5);
      this.tableLayoutPanel1.Controls.Add(this.columnCommentTextBox, 1, 2);
      this.tableLayoutPanel1.Controls.Add(this.storageLabel, 2, 2);
      this.tableLayoutPanel1.Controls.Add(this.generatedCheckbox, 4, 5);
      this.tableLayoutPanel1.Controls.Add(this.virtualRadioButton, 3, 2);
      this.tableLayoutPanel1.Controls.Add(this.storedRadioButton, 4, 2);
      this.tableLayoutPanel1.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.tableLayoutPanel1.Location = new System.Drawing.Point(8, 122);
      this.tableLayoutPanel1.Margin = new System.Windows.Forms.Padding(0);
      this.tableLayoutPanel1.Name = "tableLayoutPanel1";
      this.tableLayoutPanel1.Padding = new System.Windows.Forms.Padding(0, 5, 8, 8);
      this.tableLayoutPanel1.RowCount = 6;
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 20F));
      this.tableLayoutPanel1.Size = new System.Drawing.Size(714, 182);
      this.tableLayoutPanel1.TabIndex = 15;
      // 
      // columnCollationComboBox
      // 
      this.columnCollationComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.columnCollationComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.columnCollationComboBox.Enabled = false;
      this.columnCollationComboBox.FormattingEnabled = true;
      this.columnCollationComboBox.Location = new System.Drawing.Point(108, 38);
      this.columnCollationComboBox.Name = "columnCollationComboBox";
      this.columnCollationComboBox.Size = new System.Drawing.Size(183, 25);
      this.columnCollationComboBox.TabIndex = 7;
      this.columnCollationComboBox.SelectedIndexChanged += new System.EventHandler(this.columnCollationComboBox_SelectedIndexChanged);
      // 
      // label12
      // 
      this.label12.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label12.AutoSize = true;
      this.label12.Location = new System.Drawing.Point(3, 66);
      this.label12.Name = "label12";
      this.label12.Size = new System.Drawing.Size(99, 27);
      this.label12.TabIndex = 9;
      this.label12.Text = "Comments:";
      this.label12.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label9
      // 
      this.label9.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label9.AutoSize = true;
      this.label9.Location = new System.Drawing.Point(3, 35);
      this.label9.Name = "label9";
      this.label9.Size = new System.Drawing.Size(99, 31);
      this.label9.TabIndex = 6;
      this.label9.Text = "Collation:";
      this.label9.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label6
      // 
      this.label6.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label6.AutoSize = true;
      this.label6.Location = new System.Drawing.Point(3, 5);
      this.label6.Name = "label6";
      this.label6.Size = new System.Drawing.Size(99, 30);
      this.label6.TabIndex = 10;
      this.label6.Text = "Column Name:";
      this.label6.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label57
      // 
      this.label57.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label57.AutoSize = true;
      this.label57.Location = new System.Drawing.Point(314, 5);
      this.label57.Margin = new System.Windows.Forms.Padding(20, 0, 3, 0);
      this.label57.Name = "label57";
      this.label57.Size = new System.Drawing.Size(77, 30);
      this.label57.TabIndex = 11;
      this.label57.Text = "Data Type:";
      this.label57.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // defaultLabel
      // 
      this.defaultLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.defaultLabel.AutoSize = true;
      this.defaultLabel.Location = new System.Drawing.Point(297, 35);
      this.defaultLabel.Name = "defaultLabel";
      this.defaultLabel.Size = new System.Drawing.Size(94, 31);
      this.defaultLabel.TabIndex = 12;
      this.defaultLabel.Text = "Default:";
      this.defaultLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // columnNameTextBox
      // 
      this.columnNameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.columnNameTextBox.Enabled = false;
      this.columnNameTextBox.Location = new System.Drawing.Point(108, 8);
      this.columnNameTextBox.Name = "columnNameTextBox";
      this.columnNameTextBox.Size = new System.Drawing.Size(183, 24);
      this.columnNameTextBox.TabIndex = 5;
      this.columnNameTextBox.Tag = "0";
      this.columnNameTextBox.Leave += new System.EventHandler(this.columnTextBox_Leave);
      // 
      // columnDataTypeTextBox
      // 
      this.columnDataTypeTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.tableLayoutPanel1.SetColumnSpan(this.columnDataTypeTextBox, 3);
      this.columnDataTypeTextBox.Enabled = false;
      this.columnDataTypeTextBox.Location = new System.Drawing.Point(397, 8);
      this.columnDataTypeTextBox.Name = "columnDataTypeTextBox";
      this.columnDataTypeTextBox.Size = new System.Drawing.Size(306, 24);
      this.columnDataTypeTextBox.TabIndex = 6;
      this.columnDataTypeTextBox.Tag = "2";
      this.columnDataTypeTextBox.Leave += new System.EventHandler(this.columnTextBox_Leave);
      // 
      // columnDefaultTextBox
      // 
      this.columnDefaultTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.tableLayoutPanel1.SetColumnSpan(this.columnDefaultTextBox, 3);
      this.columnDefaultTextBox.Enabled = false;
      this.columnDefaultTextBox.Location = new System.Drawing.Point(397, 38);
      this.columnDefaultTextBox.Name = "columnDefaultTextBox";
      this.columnDefaultTextBox.Size = new System.Drawing.Size(306, 24);
      this.columnDefaultTextBox.TabIndex = 8;
      this.columnDefaultTextBox.Tag = "3";
      this.columnDefaultTextBox.Leave += new System.EventHandler(this.columnTextBox_Leave);
      // 
      // pkCheckBox
      // 
      this.pkCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.pkCheckBox.AutoSize = true;
      this.pkCheckBox.Enabled = false;
      this.pkCheckBox.Location = new System.Drawing.Point(397, 96);
      this.pkCheckBox.Name = "pkCheckBox";
      this.pkCheckBox.Size = new System.Drawing.Size(126, 21);
      this.pkCheckBox.TabIndex = 10;
      this.pkCheckBox.Tag = "0";
      this.pkCheckBox.Text = "Primary Key";
      this.pkCheckBox.UseVisualStyleBackColor = true;
      this.pkCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // nnCheckBox
      // 
      this.nnCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.nnCheckBox.AutoSize = true;
      this.nnCheckBox.Enabled = false;
      this.nnCheckBox.Location = new System.Drawing.Point(529, 96);
      this.nnCheckBox.Name = "nnCheckBox";
      this.nnCheckBox.Size = new System.Drawing.Size(93, 21);
      this.nnCheckBox.TabIndex = 11;
      this.nnCheckBox.Tag = "1";
      this.nnCheckBox.Text = "Not Null";
      this.nnCheckBox.UseVisualStyleBackColor = true;
      this.nnCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // uniqueCheckBox
      // 
      this.uniqueCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.uniqueCheckBox.AutoSize = true;
      this.uniqueCheckBox.Enabled = false;
      this.uniqueCheckBox.Location = new System.Drawing.Point(628, 96);
      this.uniqueCheckBox.Name = "uniqueCheckBox";
      this.uniqueCheckBox.Size = new System.Drawing.Size(75, 21);
      this.uniqueCheckBox.TabIndex = 12;
      this.uniqueCheckBox.Tag = "2";
      this.uniqueCheckBox.Text = "Unique";
      this.uniqueCheckBox.UseVisualStyleBackColor = true;
      this.uniqueCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // binaryCheckBox
      // 
      this.binaryCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.binaryCheckBox.AutoSize = true;
      this.binaryCheckBox.Enabled = false;
      this.binaryCheckBox.Location = new System.Drawing.Point(397, 123);
      this.binaryCheckBox.Name = "binaryCheckBox";
      this.binaryCheckBox.Size = new System.Drawing.Size(126, 21);
      this.binaryCheckBox.TabIndex = 13;
      this.binaryCheckBox.Tag = "3";
      this.binaryCheckBox.Text = "Binary";
      this.binaryCheckBox.UseVisualStyleBackColor = true;
      this.binaryCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // unsignedCheckBox
      // 
      this.unsignedCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.unsignedCheckBox.AutoSize = true;
      this.unsignedCheckBox.Enabled = false;
      this.unsignedCheckBox.Location = new System.Drawing.Point(529, 123);
      this.unsignedCheckBox.Name = "unsignedCheckBox";
      this.unsignedCheckBox.Size = new System.Drawing.Size(93, 21);
      this.unsignedCheckBox.TabIndex = 14;
      this.unsignedCheckBox.Tag = "4";
      this.unsignedCheckBox.Text = "Unsigned";
      this.unsignedCheckBox.UseVisualStyleBackColor = true;
      this.unsignedCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // zeroFillCheckBox
      // 
      this.zeroFillCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.zeroFillCheckBox.AutoSize = true;
      this.zeroFillCheckBox.Enabled = false;
      this.zeroFillCheckBox.Location = new System.Drawing.Point(628, 123);
      this.zeroFillCheckBox.Name = "zeroFillCheckBox";
      this.zeroFillCheckBox.Size = new System.Drawing.Size(75, 21);
      this.zeroFillCheckBox.TabIndex = 15;
      this.zeroFillCheckBox.Tag = "5";
      this.zeroFillCheckBox.Text = "Zero Fill";
      this.zeroFillCheckBox.UseVisualStyleBackColor = true;
      this.zeroFillCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // aiCheckBox
      // 
      this.aiCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.aiCheckBox.AutoSize = true;
      this.aiCheckBox.Enabled = false;
      this.aiCheckBox.Location = new System.Drawing.Point(397, 150);
      this.aiCheckBox.Name = "aiCheckBox";
      this.aiCheckBox.Size = new System.Drawing.Size(126, 21);
      this.aiCheckBox.TabIndex = 16;
      this.aiCheckBox.Tag = "6";
      this.aiCheckBox.Text = "Auto Increment";
      this.aiCheckBox.UseVisualStyleBackColor = true;
      this.aiCheckBox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // columnCommentTextBox
      // 
      this.columnCommentTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.columnCommentTextBox.Location = new System.Drawing.Point(108, 69);
      this.columnCommentTextBox.Multiline = true;
      this.columnCommentTextBox.Name = "columnCommentTextBox";
      this.tableLayoutPanel1.SetRowSpan(this.columnCommentTextBox, 4);
      this.columnCommentTextBox.Size = new System.Drawing.Size(183, 102);
      this.columnCommentTextBox.TabIndex = 17;
      this.columnCommentTextBox.Tag = "1";
      this.columnCommentTextBox.Leave += new System.EventHandler(this.columnTextBox_Leave);
      // 
      // storageLabel
      // 
      this.storageLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.storageLabel.AutoSize = true;
      this.storageLabel.Location = new System.Drawing.Point(297, 66);
      this.storageLabel.Name = "storageLabel";
      this.storageLabel.Size = new System.Drawing.Size(94, 27);
      this.storageLabel.TabIndex = 18;
      this.storageLabel.Text = "Storage:";
      this.storageLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // generatedCheckbox
      // 
      this.generatedCheckbox.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.generatedCheckbox.AutoSize = true;
      this.generatedCheckbox.Enabled = false;
      this.generatedCheckbox.Location = new System.Drawing.Point(529, 150);
      this.generatedCheckbox.Name = "generatedCheckbox";
      this.generatedCheckbox.Size = new System.Drawing.Size(93, 21);
      this.generatedCheckbox.TabIndex = 19;
      this.generatedCheckbox.Tag = "7";
      this.generatedCheckbox.Text = "Generated";
      this.generatedCheckbox.UseVisualStyleBackColor = true;
      this.generatedCheckbox.CheckStateChanged += new System.EventHandler(this.columnFlagsChanged);
      // 
      // headingLayoutPanel
      // 
      this.headingLayoutPanel.AutoSize = true;
      this.headingLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.headingLayoutPanel.BackColor = System.Drawing.Color.White;
      this.headingLayoutPanel.ColumnCount = 6;
      this.headingLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 80F));
      this.headingLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.headingLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.headingLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.headingLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 50F));
      this.headingLayoutPanel.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
      this.headingLayoutPanel.Controls.Add(this.pictureBox1, 0, 0);
      this.headingLayoutPanel.Controls.Add(this.optEngine, 4, 1);
      this.headingLayoutPanel.Controls.Add(this.label3, 3, 1);
      this.headingLayoutPanel.Controls.Add(this.label4, 3, 0);
      this.headingLayoutPanel.Controls.Add(this.schemaLabel, 4, 0);
      this.headingLayoutPanel.Controls.Add(this.optCollation, 2, 1);
      this.headingLayoutPanel.Controls.Add(this.label2, 1, 1);
      this.headingLayoutPanel.Controls.Add(this.nameTextBox, 2, 0);
      this.headingLayoutPanel.Controls.Add(this.label1, 1, 0);
      this.headingLayoutPanel.Controls.Add(this.label7, 1, 2);
      this.headingLayoutPanel.Controls.Add(this.optComments, 2, 2);
      this.headingLayoutPanel.Controls.Add(this.collapsePictureBox, 5, 0);
      this.headingLayoutPanel.Dock = System.Windows.Forms.DockStyle.Top;
      this.headingLayoutPanel.Location = new System.Drawing.Point(0, 0);
      this.headingLayoutPanel.Name = "headingLayoutPanel";
      this.headingLayoutPanel.Padding = new System.Windows.Forms.Padding(10, 5, 10, 5);
      this.headingLayoutPanel.RowCount = 3;
      this.headingLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 32F));
      this.headingLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 32F));
      this.headingLayoutPanel.RowStyles.Add(new System.Windows.Forms.RowStyle());
      this.headingLayoutPanel.Size = new System.Drawing.Size(730, 129);
      this.headingLayoutPanel.TabIndex = 13;
      // 
      // pictureBox1
      // 
      this.pictureBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(13, 8);
      this.pictureBox1.Name = "pictureBox1";
      this.headingLayoutPanel.SetRowSpan(this.pictureBox1, 2);
      this.pictureBox1.Size = new System.Drawing.Size(44, 58);
      this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
      this.pictureBox1.TabIndex = 10;
      this.pictureBox1.TabStop = false;
      // 
      // optEngine
      // 
      this.optEngine.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
      this.optEngine.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optEngine.Enabled = false;
      this.optEngine.FormattingEnabled = true;
      this.optEngine.Location = new System.Drawing.Point(472, 40);
      this.optEngine.Name = "optEngine";
      this.optEngine.Size = new System.Drawing.Size(209, 25);
      this.optEngine.TabIndex = 2;
      this.optEngine.SelectedIndexChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label3
      // 
      this.label3.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
      this.label3.AutoSize = true;
      this.label3.Location = new System.Drawing.Point(399, 37);
      this.label3.Name = "label3";
      this.label3.Padding = new System.Windows.Forms.Padding(5, 0, 0, 0);
      this.label3.Size = new System.Drawing.Size(59, 32);
      this.label3.TabIndex = 12;
      this.label3.Text = "Engine:";
      this.label3.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label4
      // 
      this.label4.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(399, 5);
      this.label4.Name = "label4";
      this.label4.Padding = new System.Windows.Forms.Padding(5, 0, 0, 0);
      this.label4.Size = new System.Drawing.Size(67, 32);
      this.label4.TabIndex = 14;
      this.label4.Text = "Schema:";
      this.label4.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // schemaLabel
      // 
      this.schemaLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
      this.schemaLabel.AutoSize = true;
      this.schemaLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.schemaLabel.Location = new System.Drawing.Point(472, 5);
      this.schemaLabel.Name = "schemaLabel";
      this.schemaLabel.Size = new System.Drawing.Size(60, 32);
      this.schemaLabel.TabIndex = 15;
      this.schemaLabel.Text = "schema";
      this.schemaLabel.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
      // 
      // optCollation
      // 
      this.optCollation.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
      this.optCollation.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.optCollation.FormattingEnabled = true;
      this.optCollation.Location = new System.Drawing.Point(184, 40);
      this.optCollation.Name = "optCollation";
      this.optCollation.Size = new System.Drawing.Size(209, 25);
      this.optCollation.TabIndex = 1;
      this.optCollation.SelectedIndexChanged += new System.EventHandler(this.tableOptChanged);
      // 
      // label2
      // 
      this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(114, 37);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(64, 32);
      this.label2.TabIndex = 9;
      this.label2.Text = "Collation:";
      this.label2.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // nameTextBox
      // 
      this.nameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
      this.nameTextBox.BackColor = System.Drawing.SystemColors.Window;
      this.nameTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.nameTextBox.Location = new System.Drawing.Point(184, 9);
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.Size = new System.Drawing.Size(209, 24);
      this.nameTextBox.TabIndex = 0;
      this.nameTextBox.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.nameTextBox_KeyPress);
      this.nameTextBox.Leave += new System.EventHandler(this.nameTextBox_Leave);
      this.nameTextBox.MinimumSize = new System.Drawing.Size(209, 24);
      // 
      // label1
      // 
      this.label1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(94, 5);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(84, 32);
      this.label1.TabIndex = 1;
      this.label1.Text = "Table Name:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label7
      // 
      this.label7.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
      this.label7.AutoSize = true;
      this.label7.Location = new System.Drawing.Point(93, 72);
      this.label7.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
      this.label7.Name = "label7";
      this.label7.Padding = new System.Windows.Forms.Padding(5, 0, 0, 0);
      this.label7.Size = new System.Drawing.Size(85, 52);
      this.label7.TabIndex = 13;
      this.label7.Text = "Comments:";
      this.label7.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // optComments
      // 
      this.optComments.AcceptsReturn = true;
      this.optComments.AcceptsTab = true;
      this.optComments.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.headingLayoutPanel.SetColumnSpan(this.optComments, 3);
      this.optComments.Location = new System.Drawing.Point(184, 72);
      this.optComments.Multiline = true;
      this.optComments.Name = "optComments";
      this.optComments.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.optComments.Size = new System.Drawing.Size(497, 49);
      this.optComments.TabIndex = 3;
      this.optComments.TextChanged += new System.EventHandler(this.commentsTextBox_TextChanged);
      // 
      // mainTabControl
      // 
      this.mainTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.mainTabControl.AuxControl = null;
      this.mainTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.mainTabControl.CanCloseLastTab = false;
      this.mainTabControl.CanReorderTabs = false;
      this.mainTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.mainTabControl.Controls.Add(this.columnsTabPage);
      this.mainTabControl.Controls.Add(this.indicesTabPage);
      this.mainTabControl.Controls.Add(this.foreignKeysTabPage);
      this.mainTabControl.Controls.Add(this.triggersTabPage);
      this.mainTabControl.Controls.Add(this.partitioningTabPage);
      this.mainTabControl.Controls.Add(this.optionsTabPage);
      this.mainTabControl.Controls.Add(this.insertsTabPage);
      this.mainTabControl.DefaultTabSwitch = true;
      this.mainTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainTabControl.HideWhenEmpty = false;
      this.mainTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.mainTabControl.ItemSize = new System.Drawing.Size(54, 21);
      this.mainTabControl.Location = new System.Drawing.Point(0, 129);
      this.mainTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.mainTabControl.MaxTabSize = 200;
      this.mainTabControl.Name = "mainTabControl";
      this.mainTabControl.RenderWithGlow = true;
      this.mainTabControl.SelectedIndex = 0;
      this.mainTabControl.ShowCloseButton = false;
      this.mainTabControl.ShowFocusState = true;
      this.mainTabControl.Size = new System.Drawing.Size(730, 330);
      this.mainTabControl.TabIndex = 0;
      this.mainTabControl.TabStop = false;
      this.mainTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      this.mainTabControl.SelectedIndexChanged += new System.EventHandler(this.mainTabControl_SelectedIndexChanged);
      this.mainTabControl.PreviewKeyDown += new System.Windows.Forms.PreviewKeyDownEventHandler(this.mainTabControl_PreviewKeyDown);
      // 
      // topPanel
      // 
      this.topPanel.AutoSize = true;
      this.topPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.topPanel.Controls.Add(this.headingLayoutPanel);
      this.topPanel.Dock = System.Windows.Forms.DockStyle.Top;
      this.topPanel.Location = new System.Drawing.Point(0, 0);
      this.topPanel.Name = "topPanel";
      this.topPanel.Size = new System.Drawing.Size(730, 129);
      this.topPanel.TabIndex = 15;
      // 
      // DbMysqlTableEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 17F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(730, 459);
      this.Controls.Add(this.mainTabControl);
      this.Controls.Add(this.topPanel);
      this.DoubleBuffered = true;
      this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.KeyPreview = true;
      this.Name = "DbMysqlTableEditor";
      this.TabText = "DbMysqlTableEditor";
      this.Text = "DbMysqlTableEditor";
      this.Load += new System.EventHandler(this.DbMysqlTableEditor_Load);
      this.Shown += new System.EventHandler(this.DbMysqlTableEditor_Shown);
      this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.DbMysqlTableEditor_KeyDown);
      this.indexContextMenuStrip.ResumeLayout(false);
      this.fksContextMenuStrip.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.collapsePictureBox)).EndInit();
      this.partitionTreeMenuStrip.ResumeLayout(false);
      this.optionsTabPage.ResumeLayout(false);
      this.panel8.ResumeLayout(false);
      this.panel8.PerformLayout();
      this.panel7.ResumeLayout(false);
      this.panel7.PerformLayout();
      this.panel6.ResumeLayout(false);
      this.panel6.PerformLayout();
      this.panel5.ResumeLayout(false);
      this.panel5.PerformLayout();
      this.partitioningTabPage.ResumeLayout(false);
      this.partitioningTabPage.PerformLayout();
      this.tableLayoutPanel2.ResumeLayout(false);
      this.tableLayoutPanel2.PerformLayout();
      this.panel4.ResumeLayout(false);
      this.panel4.PerformLayout();
      this.foreignKeysTabPage.ResumeLayout(false);
      this.foreignKeyWarningPanel.ResumeLayout(false);
      this.foreignKeyPageSplitContainer.Panel1.ResumeLayout(false);
      this.foreignKeyPageSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.foreignKeyPageSplitContainer)).EndInit();
      this.foreignKeyPageSplitContainer.ResumeLayout(false);
      this.panel3.ResumeLayout(false);
      this.splitContainer6.Panel1.ResumeLayout(false);
      this.splitContainer6.Panel1.PerformLayout();
      this.splitContainer6.Panel2.ResumeLayout(false);
      this.splitContainer6.Panel2.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer6)).EndInit();
      this.splitContainer6.ResumeLayout(false);
      this.indicesTabPage.ResumeLayout(false);
      this.splitContainer1.Panel1.ResumeLayout(false);
      this.splitContainer1.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
      this.splitContainer1.ResumeLayout(false);
      this.splitContainer3.Panel1.ResumeLayout(false);
      this.splitContainer3.Panel1.PerformLayout();
      this.splitContainer3.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer3)).EndInit();
      this.splitContainer3.ResumeLayout(false);
      this.panel2.ResumeLayout(false);
      this.splitContainer5.Panel1.ResumeLayout(false);
      this.splitContainer5.Panel1.PerformLayout();
      this.splitContainer5.Panel2.ResumeLayout(false);
      this.splitContainer5.Panel2.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer5)).EndInit();
      this.splitContainer5.ResumeLayout(false);
      this.columnsTabPage.ResumeLayout(false);
      this.columnsTabPage.PerformLayout();
      this.columnListSplitContainer.Panel1.ResumeLayout(false);
      this.columnListSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.columnListSplitContainer)).EndInit();
      this.columnListSplitContainer.ResumeLayout(false);
      this.tableLayoutPanel1.ResumeLayout(false);
      this.tableLayoutPanel1.PerformLayout();
      this.headingLayoutPanel.ResumeLayout(false);
      this.headingLayoutPanel.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.mainTabControl.ResumeLayout(false);
      this.topPanel.ResumeLayout(false);
      this.topPanel.PerformLayout();
      this.ResumeLayout(false);
      this.PerformLayout();

		}

    #endregion

    private Aga.Controls.Tree.TreeColumn columnNameTreeColumn;
		private Aga.Controls.Tree.TreeColumn datatypeTreeColumn;
		private Aga.Controls.Tree.TreeColumn nnTreeColumn;
		private Aga.Controls.Tree.TreeColumn aiTreeColumn;
		private Aga.Controls.Tree.TreeColumn defaultTreeColumn;
    private MySQL.Utilities.AdvNodeTextBox nameNodeControl;
		private Aga.Controls.Tree.NodeControls.NodeCheckBox nnNodeControl;
		private Aga.Controls.Tree.NodeControls.NodeCheckBox aiNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox gNodeControl;
    private MySQL.Utilities.AdvNodeTextBox defaultNodeControl;
		private Aga.Controls.Tree.TreeColumn indexNameColumn;
		private Aga.Controls.Tree.TreeColumn indexTypeColumn;
		private Aga.Controls.Tree.TreeColumn indexColumnNameTreeColumn;
		private Aga.Controls.Tree.TreeColumn indexColumnStorageTreeColumn;
    private Aga.Controls.Tree.TreeColumn indexColumnLengthTreeColumn;
		private Aga.Controls.Tree.NodeControls.NodeIcon columnIconNodeControl;
		private MySQL.Utilities.AdvNodeTextBox indexNameNodeControl;
    private MySQL.Utilities.AdvNodeComboBox indexTypeNodeControl;
		private MySQL.Utilities.AdvNodeComboBox indexColumnNameNodeControl;
		private MySQL.Utilities.AdvNodeComboBox indexColumnOrderNodeControl;
    private MySQL.Utilities.AdvNodeTextBox indexColumnLengthNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox indexColumnEnabledNodeControl;
    private Aga.Controls.Tree.TreeColumn nameFkColumn;
    private Aga.Controls.Tree.TreeColumn targetFkColumn;
    private Aga.Controls.Tree.TreeColumn columnFkColumn;
    private Aga.Controls.Tree.TreeColumn targetColumnFkColumn;
    private MySQL.Utilities.AdvNodeCheckBox columnEnabledFkNodeControl;
    private MySQL.Utilities.AdvNodeTextBox columnFkNodeControl;
    private MySQL.Utilities.AdvNodeTextBox nameFkNodeControl;
    private MySQL.Utilities.AdvNodeComboBox targetFkNodeControl;
    private MySQL.Utilities.AdvNodeComboBox targetColumnFkNodeControl;
    private System.Windows.Forms.ToolTip toolTip;
    private Aga.Controls.Tree.NodeControls.NodeStateIcon partNodeStateIcon;
    private Aga.Controls.Tree.TreeColumn partNodeStateColumn;
    private Aga.Controls.Tree.TreeColumn partNameTreeColumn;
    private Aga.Controls.Tree.TreeColumn partValuesTreeColumn;
    private Aga.Controls.Tree.TreeColumn partDataDirTreeColumn;
    private Aga.Controls.Tree.TreeColumn partIndexDirTreeColumn;
    private Aga.Controls.Tree.TreeColumn partMinRowsTreeColumn;
    private Aga.Controls.Tree.TreeColumn partMaxRowsTreeColumn;
    private Aga.Controls.Tree.TreeColumn partCommentTreeColumn;
    private Aga.Controls.Tree.TreeColumn partEngineTreeColumn;
    private Aga.Controls.Tree.TreeColumn partTablespaceTreeColumn;
    private Aga.Controls.Tree.TreeColumn partNodegroupTreeColumn;
    private System.Windows.Forms.ContextMenuStrip partitionTreeMenuStrip;
    private System.Windows.Forms.ToolStripMenuItem showClusterSettingsToolStripMenuItem;
    private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
    private System.Windows.Forms.ToolStripMenuItem deleteRowToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem createSubpartitionToolStripMenuItem;
    private Aga.Controls.Tree.TreeColumn indexColumnOrderTreeColumn;
    private MySQL.Utilities.AdvNodeComboBox indexColumnStorageNodeControl;

    private MySQL.Utilities.AdvNodeTextBox partNameNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partValuesNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partDataDirNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partIndexDirNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partMinRowsNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partMaxRowsNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partCommentNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partEngineNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partTablespaceNodeControl;
    private MySQL.Utilities.AdvNodeTextBox partNodegroupNodeControl;
    private System.Windows.Forms.ContextMenuStrip fksContextMenuStrip;
    private System.Windows.Forms.ToolStripMenuItem deleteSelectedFKsToolStripMenuItem;
    private MySQL.Utilities.AdvNodeComboBox datatypeComboBoxNodeControl;
    private System.Windows.Forms.ContextMenuStrip indexContextMenuStrip;
    private System.Windows.Forms.ToolStripMenuItem deleteSelectedIndicesToolStripMenuItem;
    private System.Windows.Forms.TabPage insertsTabPage;
    private System.Windows.Forms.TabPage optionsTabPage;
    private System.Windows.Forms.Panel panel8;
    private System.Windows.Forms.ComboBox optMergeMethod;
    private System.Windows.Forms.Label label48;
    private System.Windows.Forms.Label label49;
    private System.Windows.Forms.Label label46;
    private System.Windows.Forms.Label label47;
    private System.Windows.Forms.TextBox optUnionTables;
    private System.Windows.Forms.Label label45;
    private MySQL.Utilities.Bevel bevel11;
    private System.Windows.Forms.Panel panel7;
    private System.Windows.Forms.Label label41;
    private System.Windows.Forms.TextBox optIndexDirectory;
    private System.Windows.Forms.Label label42;
    private System.Windows.Forms.Label label43;
    private System.Windows.Forms.Label label44;
    private System.Windows.Forms.TextBox optDataDirectory;
    private System.Windows.Forms.Label label40;
    private MySQL.Utilities.Bevel bevel10;
    private System.Windows.Forms.Panel panel6;
    private System.Windows.Forms.Label label29;
    private System.Windows.Forms.Label label39;
    private System.Windows.Forms.CheckBox optUseChecksum;
    private MySQL.Utilities.Bevel bevel9;
    private System.Windows.Forms.Label label37;
    private System.Windows.Forms.Label label33;
    private System.Windows.Forms.Label label30;
    private System.Windows.Forms.ComboBox optRowFormat;
    private System.Windows.Forms.TextBox optMaxRows;
    private System.Windows.Forms.Label label35;
    private System.Windows.Forms.Label label36;
    private System.Windows.Forms.Label label34;
    private System.Windows.Forms.TextBox optMinRows;
    private System.Windows.Forms.TextBox optAvgRowLength;
    private System.Windows.Forms.Label label31;
    private System.Windows.Forms.Label label32;
    private System.Windows.Forms.Panel panel5;
    private System.Windows.Forms.Label label28;
    private System.Windows.Forms.Label label38;
    private System.Windows.Forms.CheckBox optDelayKeyUpdates;
    private MySQL.Utilities.Bevel bevel8;
    private System.Windows.Forms.Label label26;
    private System.Windows.Forms.Label label23;
    private System.Windows.Forms.TextBox optAutoIncrement;
    private System.Windows.Forms.ComboBox optPackKeys;
    private System.Windows.Forms.Label label27;
    private System.Windows.Forms.Label label22;
    private System.Windows.Forms.Label label24;
    private System.Windows.Forms.Label label25;
    private System.Windows.Forms.TextBox optTablePassword;
    private System.Windows.Forms.TabPage partitioningTabPage;
    private System.Windows.Forms.TableLayoutPanel tableLayoutPanel2;
    private System.Windows.Forms.Panel panel4;
    private System.Windows.Forms.CheckBox partEnable;
    private MySQL.Utilities.Bevel bevel12;
    private System.Windows.Forms.CheckBox partManual;
    private System.Windows.Forms.CheckBox subpartManual;
    private System.Windows.Forms.Label label50;
    private System.Windows.Forms.Label label54;
    private System.Windows.Forms.ComboBox partFunction;
    private System.Windows.Forms.TextBox subpartParams;
    private System.Windows.Forms.TextBox partParams;
    private System.Windows.Forms.Label label55;
    private System.Windows.Forms.TextBox partCount;
    private System.Windows.Forms.Label label52;
    private System.Windows.Forms.ComboBox subpartFunction;
    private System.Windows.Forms.Label label51;
    private System.Windows.Forms.Label label53;
    private System.Windows.Forms.TextBox subpartCount;
    private Aga.Controls.Tree.TreeViewAdv partitionTreeView;
    private System.Windows.Forms.TabPage triggersTabPage;
    private System.Windows.Forms.TabPage foreignKeysTabPage;
    private System.Windows.Forms.SplitContainer foreignKeyPageSplitContainer;
    private Aga.Controls.Tree.TreeViewAdv fkTreeView;
    private Aga.Controls.Tree.TreeViewAdv fkColumnsTreeView;
    private System.Windows.Forms.Panel panel3;
    private System.Windows.Forms.SplitContainer splitContainer6;
    private System.Windows.Forms.Label label21;
    private MySQL.Utilities.Bevel bevel7;
    private System.Windows.Forms.Label label20;
    private System.Windows.Forms.Label label18;
    private System.Windows.Forms.ComboBox onDeleteActionComboBox;
    private System.Windows.Forms.ComboBox onUpdateActionComboBox;
    private System.Windows.Forms.Label label19;
    private MySQL.Utilities.Bevel bevel6;
    private System.Windows.Forms.TextBox fkCommentText;
    private System.Windows.Forms.TabPage indicesTabPage;
    private System.Windows.Forms.SplitContainer splitContainer1;
    private Aga.Controls.Tree.TreeViewAdv indicesTreeView;
    private System.Windows.Forms.SplitContainer splitContainer3;
    private System.Windows.Forms.Label label8;
    private MySQL.Utilities.Bevel bevel3;
    private Aga.Controls.Tree.TreeViewAdv indexColumnsTreeView;
    private System.Windows.Forms.Panel panel2;
    private System.Windows.Forms.SplitContainer splitContainer5;
    private System.Windows.Forms.TextBox indexParserText;
    private System.Windows.Forms.Label label17;
    private System.Windows.Forms.Label label15;
    private System.Windows.Forms.TextBox indexRowBlockSizeText;
    private System.Windows.Forms.Label label13;
    private MySQL.Utilities.Bevel bevel4;
    private System.Windows.Forms.Label label14;
    private System.Windows.Forms.ComboBox indexStorageTypeComboBox;
    private MySQL.Utilities.Bevel bevel5;
    private System.Windows.Forms.Label label16;
    private System.Windows.Forms.TextBox indexCommentText;
    private System.Windows.Forms.TabPage columnsTabPage;
    private Aga.Controls.Tree.TreeViewAdv columnsTreeView;
    private System.Windows.Forms.Label label12;
    private System.Windows.Forms.ComboBox columnCollationComboBox;
    private System.Windows.Forms.Label label9;
    private System.Windows.Forms.TableLayoutPanel headingLayoutPanel;
    private System.Windows.Forms.Label label7;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.ComboBox optCollation;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.TextBox optComments;
    private System.Windows.Forms.ComboBox optEngine;
    private System.Windows.Forms.PictureBox pictureBox1;
    private MySQL.Controls.FlatTabControl mainTabControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox pkNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox binNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox unNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox uqNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeCheckBox zfNodeControl;
    private Aga.Controls.Tree.TreeColumn pkTreeColumn;
    private Aga.Controls.Tree.TreeColumn binTreeColumn;
    private Aga.Controls.Tree.TreeColumn unTreeColumn;
    private Aga.Controls.Tree.TreeColumn uqTreeColumn;
    private Aga.Controls.Tree.TreeColumn zfTreeColumn;
    private System.Windows.Forms.Label fkIndexLabel;
    private System.Windows.Forms.CheckBox fkModelOnlyCheck;
    private System.Windows.Forms.Label label10;
    private System.Windows.Forms.Panel foreignKeyWarningPanel;
    private System.Windows.Forms.Label label11;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Label schemaLabel;
    private System.Windows.Forms.PictureBox collapsePictureBox;
    private System.Windows.Forms.TableLayoutPanel tableLayoutPanel1;
    private System.Windows.Forms.Label label6;
    private Utilities.Bevel bevel2;
    private System.Windows.Forms.Label label57;
    private System.Windows.Forms.Label defaultLabel;
    private System.Windows.Forms.TextBox columnNameTextBox;
    private System.Windows.Forms.TextBox columnDataTypeTextBox;
    private System.Windows.Forms.TextBox columnDefaultTextBox;
    private System.Windows.Forms.CheckBox pkCheckBox;
    private System.Windows.Forms.CheckBox nnCheckBox;
    private System.Windows.Forms.CheckBox uniqueCheckBox;
    private System.Windows.Forms.CheckBox binaryCheckBox;
    private System.Windows.Forms.CheckBox unsignedCheckBox;
    private System.Windows.Forms.CheckBox zeroFillCheckBox;
    private System.Windows.Forms.CheckBox aiCheckBox;
    private System.Windows.Forms.Panel topPanel;
    private System.Windows.Forms.SplitContainer columnListSplitContainer;
    private System.Windows.Forms.TableLayoutPanel oldTableLayoutPanel;
    private System.Windows.Forms.TextBox columnCommentTextBox;
    private System.Windows.Forms.ComboBox optKeyBlockSize;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.Label label56;
    private System.Windows.Forms.Label storageLabel;
    private Aga.Controls.Tree.TreeColumn gTreeColumn;
    private System.Windows.Forms.CheckBox generatedCheckbox;
    private System.Windows.Forms.RadioButton virtualRadioButton;
    private System.Windows.Forms.RadioButton storedRadioButton;
	}
}
