namespace MySQL.GUI.Workbench
{
    partial class SqlIdeForm
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
          Destroy();

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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(SqlIdeForm));
      this.recordsetTabsContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.toolStripMenuItem4 = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem5 = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem6 = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem3 = new System.Windows.Forms.ToolStripSeparator();
      this.renamePage_ToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.treeColumn1 = new Aga.Controls.Tree.TreeColumn();
      this.treeColumn2 = new Aga.Controls.Tree.TreeColumn();
      this.nodeTextBox1 = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.nodeTextBox2 = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.contentSplitContainer = new System.Windows.Forms.SplitContainer();
      this.mainContentSplitContainer = new System.Windows.Forms.SplitContainer();
      this.mainContentTabControl = new MySQL.Controls.FlatTabControl();
      this.editorTabImageList = new System.Windows.Forms.ImageList(this.components);
      this.rightSplitContainer = new System.Windows.Forms.SplitContainer();
      this.outputHeader = new MySQL.Controls.HeaderPanel();
      this.outputPageContent = new System.Windows.Forms.Panel();
      this.actionPanel = new System.Windows.Forms.Panel();
      this.outputPageToolStrip = new System.Windows.Forms.ToolStrip();
      this.outputPaneIcon = new System.Windows.Forms.ToolStripLabel();
      this.outputSelector = new System.Windows.Forms.ToolStripComboBox();
      this.textOutputPage = new System.Windows.Forms.Panel();
//      this.resultSetTextBox = new System.Windows.Forms.TextBox();
      this.historyPage = new System.Windows.Forms.Panel();
      this.historySplitContainer = new System.Windows.Forms.SplitContainer();
      this.mainSplitContainer = new System.Windows.Forms.SplitContainer();
      this.sideArea = new System.Windows.Forms.Panel();
      this.editorTabsContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.toolStripMenuItem1 = new System.Windows.Forms.ToolStripMenuItem();
      this.closeAllToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.closeAllOfTheSameTypeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.closeToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.toolStripMenuItem2 = new System.Windows.Forms.ToolStripSeparator();
      this.copyFullPathToClipboardToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.recordsetTabsContextMenuStrip.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.contentSplitContainer)).BeginInit();
      this.contentSplitContainer.Panel1.SuspendLayout();
      this.contentSplitContainer.Panel2.SuspendLayout();
      this.contentSplitContainer.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.mainContentSplitContainer)).BeginInit();
      this.mainContentSplitContainer.Panel1.SuspendLayout();
      this.mainContentSplitContainer.Panel2.SuspendLayout();
      this.mainContentSplitContainer.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.rightSplitContainer)).BeginInit();
      this.rightSplitContainer.SuspendLayout();
      this.outputHeader.SuspendLayout();
      this.outputPageContent.SuspendLayout();
      this.outputPageToolStrip.SuspendLayout();
      this.textOutputPage.SuspendLayout();
      this.historyPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.historySplitContainer)).BeginInit();
      this.historySplitContainer.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).BeginInit();
      this.mainSplitContainer.Panel1.SuspendLayout();
      this.mainSplitContainer.Panel2.SuspendLayout();
      this.mainSplitContainer.SuspendLayout();
      this.editorTabsContextMenuStrip.SuspendLayout();
      this.SuspendLayout();
      // 
      // treeColumn1
      // 
      this.treeColumn1.Header = "ID";
      this.treeColumn1.SortOrder = System.Windows.Forms.SortOrder.None;
      this.treeColumn1.TooltipText = null;
      // 
      // treeColumn2
      // 
      this.treeColumn2.Header = "NAME";
      this.treeColumn2.SortOrder = System.Windows.Forms.SortOrder.None;
      this.treeColumn2.TooltipText = null;
      this.treeColumn2.Width = 150;
      // 
      // nodeTextBox1
      // 
      this.nodeTextBox1.DataPropertyName = "0";
      this.nodeTextBox1.IncrementalSearchEnabled = true;
      this.nodeTextBox1.LeftMargin = 3;
      this.nodeTextBox1.ParentColumn = this.treeColumn1;
      this.nodeTextBox1.VirtualMode = true;
      // 
      // nodeTextBox2
      // 
      this.nodeTextBox2.DataPropertyName = "1";
      this.nodeTextBox2.IncrementalSearchEnabled = true;
      this.nodeTextBox2.LeftMargin = 3;
      this.nodeTextBox2.ParentColumn = this.treeColumn2;
      this.nodeTextBox2.VirtualMode = true;
      // 
      // contentSplitContainer
      // 
      this.contentSplitContainer.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.contentSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.contentSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
      this.contentSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.contentSplitContainer.Margin = new System.Windows.Forms.Padding(0);
      this.contentSplitContainer.Name = "contentSplitContainer";
      this.contentSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
      // 
      // contentSplitContainer.Panel1
      // 
      this.contentSplitContainer.Panel1.Controls.Add(this.mainContentSplitContainer);
      // 
      // contentSplitContainer.Panel2
      // 
      this.contentSplitContainer.Panel2.Controls.Add(this.outputHeader);
      this.contentSplitContainer.Size = new System.Drawing.Size(735, 735);
      this.contentSplitContainer.SplitterDistance = 416;
      this.contentSplitContainer.SplitterWidth = 6;
      this.contentSplitContainer.TabIndex = 0;
      this.contentSplitContainer.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.contentSplitContainer_SplitterMoved);
      // 
      // mainContentSplitContainer
      // 
      this.mainContentSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainContentSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel2;
      this.mainContentSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.mainContentSplitContainer.Name = "mainContentSplitContainer";
      // 
      // mainContentSplitContainer.Panel1
      // 
      this.mainContentSplitContainer.Panel1.Controls.Add(this.mainContentTabControl);
      // 
      // mainContentSplitContainer.Panel2
      // 
      this.mainContentSplitContainer.Panel2.Controls.Add(this.rightSplitContainer);
      this.mainContentSplitContainer.Panel2MinSize = 200;
      this.mainContentSplitContainer.Size = new System.Drawing.Size(735, 416);
      this.mainContentSplitContainer.SplitterDistance = 424;
      this.mainContentSplitContainer.SplitterWidth = 6;
      this.mainContentSplitContainer.TabIndex = 3;
      this.mainContentSplitContainer.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.mainContentSplitContainer_SplitterMoved);
      // 
      // mainContentTabControl
      // 
      this.mainContentTabControl.AllowDrop = true;
      this.mainContentTabControl.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.mainContentTabControl.CanCloseLastTab = true;
      this.mainContentTabControl.CanReorderTabs = true;
      this.mainContentTabControl.ContentPadding = new System.Windows.Forms.Padding(0, 2, 0, 0);
      this.mainContentTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainContentTabControl.HideWhenEmpty = false;
      this.mainContentTabControl.HotTrack = true;
      this.mainContentTabControl.ImageList = this.editorTabImageList;
      this.mainContentTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.mainContentTabControl.ItemSize = new System.Drawing.Size(73, 19);
      this.mainContentTabControl.Location = new System.Drawing.Point(0, 0);
      this.mainContentTabControl.Margin = new System.Windows.Forms.Padding(0);
      this.mainContentTabControl.MaxTabSize = 200;
      this.mainContentTabControl.Name = "mainContentTabControl";
      this.mainContentTabControl.RenderWithGlow = true;
      this.mainContentTabControl.SelectedIndex = 0;
      this.mainContentTabControl.ShowCloseButton = true;
      this.mainContentTabControl.ShowFocusState = true;
      this.mainContentTabControl.ShowToolTips = true;
      this.mainContentTabControl.Size = new System.Drawing.Size(424, 416);
      this.mainContentTabControl.TabIndex = 2;
      this.mainContentTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.TopNormal;
      this.mainContentTabControl.TabClosing += new System.EventHandler<MySQL.Controls.TabClosingEventArgs>(this.editorTabControl_TabClosing);
      this.mainContentTabControl.TabMoving += new System.EventHandler<MySQL.Controls.TabMovingEventArgs>(this.editorTabControl_TabMoving);
      this.mainContentTabControl.SelectedIndexChanged += new System.EventHandler(this.mainContentTabControl_SelectedIndexChanged);
      this.mainContentTabControl.MouseClick += new System.Windows.Forms.MouseEventHandler(this.editorTabControlMouseClick);
      // 
      // editorTabImageList
      // 
      this.editorTabImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
      this.editorTabImageList.ImageSize = new System.Drawing.Size(16, 16);
      this.editorTabImageList.TransparentColor = System.Drawing.Color.Transparent;
      // 
      // rightSplitContainer
      // 
      this.rightSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.rightSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.rightSplitContainer.Name = "rightSplitContainer";
      this.rightSplitContainer.Orientation = System.Windows.Forms.Orientation.Horizontal;
      this.rightSplitContainer.Panel2Collapsed = true;
      this.rightSplitContainer.Size = new System.Drawing.Size(305, 416);
      this.rightSplitContainer.SplitterDistance = 350;
      this.rightSplitContainer.TabIndex = 0;
      // 
      // outputHeader
      // 
      this.outputHeader.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.outputHeader.Controls.Add(this.outputPageContent);
      this.outputHeader.Controls.Add(this.outputPageToolStrip);
      this.outputHeader.Dock = System.Windows.Forms.DockStyle.Fill;
      this.outputHeader.ForeColor = System.Drawing.Color.White;
      this.outputHeader.ForeColorFocused = System.Drawing.Color.White;
      this.outputHeader.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.outputHeader.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.outputHeader.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.outputHeader.Location = new System.Drawing.Point(0, 0);
      this.outputHeader.Margin = new System.Windows.Forms.Padding(0);
      this.outputHeader.Name = "outputHeader";
      this.outputHeader.Padding = new System.Windows.Forms.Padding(0, 21, 0, 0);
      this.outputHeader.Size = new System.Drawing.Size(735, 313);
      this.outputHeader.TabIndex = 2;
      this.outputHeader.Text = "Output";
      // 
      // outputPageContent
      // 
      this.outputPageContent.Controls.Add(this.actionPanel);
      this.outputPageContent.Dock = System.Windows.Forms.DockStyle.Fill;
      this.outputPageContent.Location = new System.Drawing.Point(0, 48);
      this.outputPageContent.Margin = new System.Windows.Forms.Padding(0);
      this.outputPageContent.MinimumSize = new System.Drawing.Size(0, 80);
      this.outputPageContent.Name = "outputPageContent";
      this.outputPageContent.Size = new System.Drawing.Size(735, 265);
      this.outputPageContent.TabIndex = 1;
      // 
      // actionPanel
      // 
      this.actionPanel.BackColor = System.Drawing.Color.White;
      this.actionPanel.Dock = System.Windows.Forms.DockStyle.Fill;
      this.actionPanel.Location = new System.Drawing.Point(0, 0);
      this.actionPanel.Margin = new System.Windows.Forms.Padding(0);
      this.actionPanel.Name = "actionPanel";
      this.actionPanel.Size = new System.Drawing.Size(735, 265);
      this.actionPanel.TabIndex = 1;
      // 
      // outputPageToolStrip
      // 
      this.outputPageToolStrip.AutoSize = false;
      this.outputPageToolStrip.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(189)))), ((int)(((byte)(199)))), ((int)(((byte)(222)))));
      this.outputPageToolStrip.GripMargin = new System.Windows.Forms.Padding(2, 4, 2, 5);
      this.outputPageToolStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
      this.outputPageToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.outputPaneIcon,
            this.outputSelector});
      this.outputPageToolStrip.Location = new System.Drawing.Point(0, 21);
      this.outputPageToolStrip.Name = "outputPageToolStrip";
      this.outputPageToolStrip.Padding = new System.Windows.Forms.Padding(5, 5, 0, 3);
      this.outputPageToolStrip.Size = new System.Drawing.Size(735, 27);
      this.outputPageToolStrip.TabIndex = 0;
      this.outputPageToolStrip.Text = "outputPageToolStrip";
      // 
      // outputPaneIcon
      // 
      this.outputPaneIcon.AutoSize = false;
      this.outputPaneIcon.Image = global::MySQL.GUI.Workbench.Properties.Resources.wb_toolbar_pages_18x18;
      this.outputPaneIcon.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
      this.outputPaneIcon.Margin = new System.Windows.Forms.Padding(0, 1, 5, 2);
      this.outputPaneIcon.Name = "outputPaneIcon";
      this.outputPaneIcon.Size = new System.Drawing.Size(18, 18);
      // 
      // outputSelector
      // 
      this.outputSelector.DropDownHeight = 110;
      this.outputSelector.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.outputSelector.DropDownWidth = 160;
      this.outputSelector.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.outputSelector.IntegralHeight = false;
      this.outputSelector.Items.AddRange(new object[] {
            "Action Output",
            "Text Output",
            "History Output"});
      this.outputSelector.Margin = new System.Windows.Forms.Padding(0);
      this.outputSelector.Name = "outputSelector";
      this.outputSelector.Size = new System.Drawing.Size(150, 19);
      this.outputSelector.ToolTipText = "Select output pane";
      this.outputSelector.SelectedIndexChanged += new System.EventHandler(this.outputPaneIndexChanged);
      // 
      // textOutputPage
      // 
      this.textOutputPage.BackColor = System.Drawing.SystemColors.ButtonFace;
    //XXX!  this.textOutputPage.Controls.Add(this.resultSetTextBox);
      this.textOutputPage.Location = new System.Drawing.Point(0, 24);
      this.textOutputPage.Margin = new System.Windows.Forms.Padding(0);
      this.textOutputPage.Name = "textOutputPage";
      this.textOutputPage.Size = new System.Drawing.Size(731, 281);
      this.textOutputPage.TabIndex = 1;
      this.textOutputPage.Text = "Text Output";
      // 
      // resultSetTextBox
      // 
   /*   this.resultSetTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.resultSetTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.resultSetTextBox.Location = new System.Drawing.Point(0, 0);
      this.resultSetTextBox.Multiline = true;
      this.resultSetTextBox.Name = "resultSetTextBox";
      this.resultSetTextBox.ReadOnly = true;
      this.resultSetTextBox.ScrollBars = System.Windows.Forms.ScrollBars.Both;
      this.resultSetTextBox.Size = new System.Drawing.Size(731, 281);
      this.resultSetTextBox.TabIndex = 1;
      this.resultSetTextBox.WordWrap = false;
    * */
      // 
      // historyPage
      // 
      this.historyPage.Controls.Add(this.historySplitContainer);
      this.historyPage.Dock = System.Windows.Forms.DockStyle.Fill;
      this.historyPage.Location = new System.Drawing.Point(0, 0);
      this.historyPage.Margin = new System.Windows.Forms.Padding(0);
      this.historyPage.Name = "historyPage";
      this.historyPage.Size = new System.Drawing.Size(731, 281);
      this.historyPage.TabIndex = 0;
      // 
      // historySplitContainer
      // 
      this.historySplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.historySplitContainer.Location = new System.Drawing.Point(0, 0);
      this.historySplitContainer.Name = "historySplitContainer";
      this.historySplitContainer.Size = new System.Drawing.Size(731, 281);
      this.historySplitContainer.SplitterDistance = 105;
      this.historySplitContainer.TabIndex = 0;
      // 
      // mainSplitContainer
      // 
      this.mainSplitContainer.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.mainSplitContainer.Dock = System.Windows.Forms.DockStyle.Fill;
      this.mainSplitContainer.FixedPanel = System.Windows.Forms.FixedPanel.Panel1;
      this.mainSplitContainer.Location = new System.Drawing.Point(0, 0);
      this.mainSplitContainer.Name = "mainSplitContainer";
      // 
      // mainSplitContainer.Panel1
      // 
      this.mainSplitContainer.Panel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.mainSplitContainer.Panel1.Controls.Add(this.sideArea);
      // 
      // mainSplitContainer.Panel2
      // 
      this.mainSplitContainer.Panel2.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.mainSplitContainer.Panel2.Controls.Add(this.contentSplitContainer);
      this.mainSplitContainer.Size = new System.Drawing.Size(941, 735);
      this.mainSplitContainer.SplitterDistance = 200;
      this.mainSplitContainer.SplitterWidth = 6;
      this.mainSplitContainer.TabIndex = 1;
      this.mainSplitContainer.SplitterMoved += new System.Windows.Forms.SplitterEventHandler(this.mainSplitContainer_SplitterMoved);
      // 
      // sideArea
      // 
      this.sideArea.Dock = System.Windows.Forms.DockStyle.Fill;
      this.sideArea.Location = new System.Drawing.Point(0, 0);
      this.sideArea.Name = "sideArea";
      this.sideArea.Size = new System.Drawing.Size(200, 735);
      this.sideArea.TabIndex = 0;
      // 
      // editorTabsContextMenuStrip
      // 
      this.editorTabsContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.toolStripMenuItem1,
            this.closeAllToolStripMenuItem,
            this.closeAllOfTheSameTypeToolStripMenuItem,
            this.closeToolStripMenuItem,
            this.toolStripMenuItem2,
            this.copyFullPathToClipboardToolStripMenuItem});
      this.editorTabsContextMenuStrip.Name = "recordsetPageContextMenuStrip";
      this.editorTabsContextMenuStrip.Size = new System.Drawing.Size(224, 120);
      // 
      // toolStripMenuItem1
      // 
      this.toolStripMenuItem1.Name = "toolStripMenuItem1";
      this.toolStripMenuItem1.Size = new System.Drawing.Size(223, 22);
      this.toolStripMenuItem1.Tag = "1";
      this.toolStripMenuItem1.Text = "Close";
      this.toolStripMenuItem1.Click += new System.EventHandler(this.editorTabContextMenuItemClick);
      // 
      // closeAllToolStripMenuItem
      // 
      this.closeAllToolStripMenuItem.Name = "closeAllToolStripMenuItem";
      this.closeAllToolStripMenuItem.Size = new System.Drawing.Size(223, 22);
      this.closeAllToolStripMenuItem.Tag = "2";
      this.closeAllToolStripMenuItem.Text = "Close All";
      this.closeAllToolStripMenuItem.Click += new System.EventHandler(this.editorTabContextMenuItemClick);
      // 
      // closeAllOfTheSameTypeToolStripMenuItem
      // 
      this.closeAllOfTheSameTypeToolStripMenuItem.Name = "closeAllOfTheSameTypeToolStripMenuItem";
      this.closeAllOfTheSameTypeToolStripMenuItem.Size = new System.Drawing.Size(223, 22);
      this.closeAllOfTheSameTypeToolStripMenuItem.Tag = "5";
      this.closeAllOfTheSameTypeToolStripMenuItem.Text = "Close All Like This";
      this.closeAllOfTheSameTypeToolStripMenuItem.Click += new System.EventHandler(this.editorTabContextMenuItemClick);
      // 
      // closeToolStripMenuItem
      // 
      this.closeToolStripMenuItem.Name = "closeToolStripMenuItem";
      this.closeToolStripMenuItem.Size = new System.Drawing.Size(223, 22);
      this.closeToolStripMenuItem.Tag = "3";
      this.closeToolStripMenuItem.Text = "Close All But This";
      this.closeToolStripMenuItem.Click += new System.EventHandler(this.editorTabContextMenuItemClick);
      // 
      // toolStripMenuItem2
      // 
      this.toolStripMenuItem2.Name = "toolStripMenuItem2";
      this.toolStripMenuItem2.Size = new System.Drawing.Size(220, 6);
      // 
      // copyFullPathToClipboardToolStripMenuItem
      // 
      this.copyFullPathToClipboardToolStripMenuItem.Name = "copyFullPathToClipboardToolStripMenuItem";
      this.copyFullPathToClipboardToolStripMenuItem.Size = new System.Drawing.Size(223, 22);
      this.copyFullPathToClipboardToolStripMenuItem.Tag = "copy_path";
      this.copyFullPathToClipboardToolStripMenuItem.Text = "Copy Full Path To Clipboard";
      this.copyFullPathToClipboardToolStripMenuItem.Click += new System.EventHandler(this.editorTabContextMenuItemClick);
      // 
      // SqlIdeForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(941, 735);
      this.Controls.Add(this.mainSplitContainer);
      this.Font = new System.Drawing.Font("Microsoft Sans Serif", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.Name = "SqlIdeForm";
      this.TabText = "Query (webshop)";
      this.Text = "SQL Editor";
      this.Shown += new System.EventHandler(this.DbSqlEditor_Shown);
      this.SizeChanged += new System.EventHandler(this.onSizeChanged);
      this.recordsetTabsContextMenuStrip.ResumeLayout(false);
      this.contentSplitContainer.Panel1.ResumeLayout(false);
      this.contentSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.contentSplitContainer)).EndInit();
      this.contentSplitContainer.ResumeLayout(false);
      this.mainContentSplitContainer.Panel1.ResumeLayout(false);
      this.mainContentSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.mainContentSplitContainer)).EndInit();
      this.mainContentSplitContainer.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.rightSplitContainer)).EndInit();
      this.rightSplitContainer.ResumeLayout(false);
      this.outputHeader.ResumeLayout(false);
      this.outputPageContent.ResumeLayout(false);
      this.outputPageToolStrip.ResumeLayout(false);
      this.outputPageToolStrip.PerformLayout();
      this.textOutputPage.ResumeLayout(false);
      this.textOutputPage.PerformLayout();
      this.historyPage.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.historySplitContainer)).EndInit();
      this.historySplitContainer.ResumeLayout(false);
      this.mainSplitContainer.Panel1.ResumeLayout(false);
      this.mainSplitContainer.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.mainSplitContainer)).EndInit();
      this.mainSplitContainer.ResumeLayout(false);
      this.editorTabsContextMenuStrip.ResumeLayout(false);
      this.ResumeLayout(false);

        }

        #endregion

        private Aga.Controls.Tree.TreeColumn treeColumn1;
        private Aga.Controls.Tree.TreeColumn treeColumn2;
        private Aga.Controls.Tree.NodeControls.NodeTextBox nodeTextBox1;
        private Aga.Controls.Tree.NodeControls.NodeTextBox nodeTextBox2;
        private System.Windows.Forms.ContextMenuStrip recordsetTabsContextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem renamePage_ToolStripMenuItem;
        private MySQL.Controls.HeaderPanel outputHeader;
        private System.Windows.Forms.Panel outputPageContent;
        private System.Windows.Forms.Panel textOutputPage;
     //   private System.Windows.Forms.TextBox resultSetTextBox;
        private System.Windows.Forms.Panel actionPanel;
        private System.Windows.Forms.ToolStrip outputPageToolStrip;
        private System.Windows.Forms.ToolStripComboBox outputSelector;
        private System.Windows.Forms.Panel historyPage;
        private System.Windows.Forms.SplitContainer historySplitContainer;
        private System.Windows.Forms.SplitContainer contentSplitContainer;
        private System.Windows.Forms.SplitContainer mainSplitContainer;
        private MySQL.Controls.FlatTabControl mainContentTabControl;
        private System.Windows.Forms.Panel sideArea;
        private System.Windows.Forms.SplitContainer mainContentSplitContainer;
        private System.Windows.Forms.SplitContainer rightSplitContainer;
        private System.Windows.Forms.ImageList editorTabImageList;
        private System.Windows.Forms.ContextMenuStrip editorTabsContextMenuStrip;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem1;
        private System.Windows.Forms.ToolStripMenuItem closeAllToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem closeToolStripMenuItem;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem2;
        private System.Windows.Forms.ToolStripMenuItem copyFullPathToClipboardToolStripMenuItem;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem4;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem5;
        private System.Windows.Forms.ToolStripMenuItem toolStripMenuItem6;
        private System.Windows.Forms.ToolStripSeparator toolStripMenuItem3;
        private System.Windows.Forms.ToolStripMenuItem closeAllOfTheSameTypeToolStripMenuItem;
        private System.Windows.Forms.ToolStripLabel outputPaneIcon;
    }
}