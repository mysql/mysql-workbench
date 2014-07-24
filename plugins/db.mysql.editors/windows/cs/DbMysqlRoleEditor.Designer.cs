namespace MySQL.GUI.Workbench.Plugins
{
  partial class DbMysqlRoleEditor
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(DbMysqlRoleEditor));
      this.flatTabControl1 = new MySQL.Controls.FlatTabControl();
      this.rolePage = new System.Windows.Forms.TabPage();
      this.label6 = new System.Windows.Forms.Label();
      this.rolesTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.roleColumn = new Aga.Controls.Tree.TreeColumn();
      this.roleNameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.ParentComboBox = new System.Windows.Forms.ComboBox();
      this.nameTextBox = new System.Windows.Forms.TextBox();
      this.label5 = new System.Windows.Forms.Label();
      this.label1 = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.privilegesPage = new System.Windows.Forms.TabPage();
      this.checkAllButton = new System.Windows.Forms.Button();
      this.uncheckAllButton = new System.Windows.Forms.Button();
      this.label4 = new System.Windows.Forms.Label();
      this.splitContainer2 = new System.Windows.Forms.SplitContainer();
      this.roleObjectsTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.objectsContextMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.objectIconNodeControl = new Aga.Controls.Tree.NodeControls.NodeIcon();
      this.objectNameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.label2 = new System.Windows.Forms.Label();
      this.privCheckedListBox = new System.Windows.Forms.CheckedListBox();
      this.label3 = new System.Windows.Forms.Label();
      this.catalogColumn = new Aga.Controls.Tree.TreeColumn();
      this.catalogObjectNameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.catalogNameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.flatTabControl1.SuspendLayout();
      this.rolePage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.privilegesPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer2)).BeginInit();
      this.splitContainer2.Panel1.SuspendLayout();
      this.splitContainer2.Panel2.SuspendLayout();
      this.splitContainer2.SuspendLayout();
      this.SuspendLayout();
      // 
      // flatTabControl1
      // 
      this.flatTabControl1.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.flatTabControl1.BackgroundColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.flatTabControl1.CanCloseLastTab = false;
      this.flatTabControl1.CanReorderTabs = false;
      this.flatTabControl1.ContentPadding = new System.Windows.Forms.Padding(0);
      this.flatTabControl1.Controls.Add(this.rolePage);
      this.flatTabControl1.Controls.Add(this.privilegesPage);
      this.flatTabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.flatTabControl1.HideWhenEmpty = false;
      this.flatTabControl1.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.flatTabControl1.Location = new System.Drawing.Point(0, 0);
      this.flatTabControl1.Margin = new System.Windows.Forms.Padding(0);
      this.flatTabControl1.MaxTabSize = 200;
      this.flatTabControl1.Name = "flatTabControl1";
      this.flatTabControl1.SelectedIndex = 0;
      this.flatTabControl1.ShowCloseButton = false;
      this.flatTabControl1.ShowFocusState = true;
      this.flatTabControl1.Size = new System.Drawing.Size(788, 234);
      this.flatTabControl1.TabIndex = 0;
      this.flatTabControl1.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.BottomNormal;
      // 
      // rolePage
      // 
      this.rolePage.BackColor = System.Drawing.Color.White;
      this.rolePage.Controls.Add(this.label6);
      this.rolePage.Controls.Add(this.rolesTreeView);
      this.rolePage.Controls.Add(this.ParentComboBox);
      this.rolePage.Controls.Add(this.nameTextBox);
      this.rolePage.Controls.Add(this.label5);
      this.rolePage.Controls.Add(this.label1);
      this.rolePage.Controls.Add(this.pictureBox1);
      this.rolePage.Location = new System.Drawing.Point(0, 0);
      this.rolePage.Name = "rolePage";
      this.rolePage.Padding = new System.Windows.Forms.Padding(3);
      this.rolePage.Size = new System.Drawing.Size(788, 213);
      this.rolePage.TabIndex = 0;
      this.rolePage.Text = "Role";
      // 
      // label6
      // 
      this.label6.Font = new System.Drawing.Font("Tahoma", 7.5F);
      this.label6.Location = new System.Drawing.Point(137, 67);
      this.label6.Name = "label6";
      this.label6.Size = new System.Drawing.Size(171, 63);
      this.label6.TabIndex = 6;
      this.label6.Text = "Privileges of a parent role will be available to its descentants.";
      // 
      // rolesTreeView
      // 
      this.rolesTreeView.AllowDrop = true;
      this.rolesTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.rolesTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.rolesTreeView.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.rolesTreeView.Columns.Add(this.roleColumn);
      this.rolesTreeView.DefaultToolTipProvider = null;
      this.rolesTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.rolesTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.rolesTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.rolesTreeView.LoadOnDemand = true;
      this.rolesTreeView.Location = new System.Drawing.Point(313, 12);
      this.rolesTreeView.Model = null;
      this.rolesTreeView.Name = "rolesTreeView";
      this.rolesTreeView.NodeControls.Add(this.roleNameNodeControl);
      this.rolesTreeView.SelectedNode = null;
      this.rolesTreeView.Size = new System.Drawing.Size(463, 195);
      this.rolesTreeView.TabIndex = 0;
      this.rolesTreeView.UseColumns = true;
      // 
      // roleColumn
      // 
      this.roleColumn.Header = "Roles";
      this.roleColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.roleColumn.TooltipText = null;
      this.roleColumn.Width = 180;
      // 
      // roleNameNodeControl
      // 
      this.roleNameNodeControl.IncrementalSearchEnabled = true;
      this.roleNameNodeControl.LeftMargin = 3;
      this.roleNameNodeControl.ParentColumn = this.roleColumn;
      this.roleNameNodeControl.VirtualMode = true;
      // 
      // ParentComboBox
      // 
      this.ParentComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
      this.ParentComboBox.FormattingEnabled = true;
      this.ParentComboBox.Location = new System.Drawing.Point(137, 39);
      this.ParentComboBox.Name = "ParentComboBox";
      this.ParentComboBox.Size = new System.Drawing.Size(171, 21);
      this.ParentComboBox.TabIndex = 5;
      this.ParentComboBox.SelectedIndexChanged += new System.EventHandler(this.ParentComboBox_SelectedIndexChanged);
      // 
      // nameTextBox
      // 
      this.nameTextBox.Location = new System.Drawing.Point(137, 12);
      this.nameTextBox.Name = "nameTextBox";
      this.nameTextBox.Size = new System.Drawing.Size(171, 21);
      this.nameTextBox.TabIndex = 4;
      this.nameTextBox.TextChanged += new System.EventHandler(this.nameTextBox_TextChanged);
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Location = new System.Drawing.Point(88, 42);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(43, 13);
      this.label5.TabIndex = 3;
      this.label5.Text = "Parent:";
      this.label5.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // label1
      // 
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(96, 15);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(38, 13);
      this.label1.TabIndex = 2;
      this.label1.Text = "Name:";
      this.label1.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = ((System.Drawing.Image)(resources.GetObject("pictureBox1.Image")));
      this.pictureBox1.Location = new System.Drawing.Point(12, 12);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 48);
      this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
      this.pictureBox1.TabIndex = 1;
      this.pictureBox1.TabStop = false;
      // 
      // privilegesPage
      // 
      this.privilegesPage.BackColor = System.Drawing.Color.White;
      this.privilegesPage.Controls.Add(this.checkAllButton);
      this.privilegesPage.Controls.Add(this.uncheckAllButton);
      this.privilegesPage.Controls.Add(this.label4);
      this.privilegesPage.Controls.Add(this.splitContainer2);
      this.privilegesPage.Location = new System.Drawing.Point(0, 0);
      this.privilegesPage.Name = "privilegesPage";
      this.privilegesPage.Padding = new System.Windows.Forms.Padding(5);
      this.privilegesPage.Size = new System.Drawing.Size(788, 213);
      this.privilegesPage.TabIndex = 1;
      this.privilegesPage.Text = "Privileges";
      // 
      // checkAllButton
      // 
      this.checkAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.checkAllButton.Location = new System.Drawing.Point(546, 155);
      this.checkAllButton.Name = "checkAllButton";
      this.checkAllButton.Size = new System.Drawing.Size(135, 23);
      this.checkAllButton.TabIndex = 3;
      this.checkAllButton.Text = "Check All Privileges";
      this.checkAllButton.UseVisualStyleBackColor = true;
      this.checkAllButton.Click += new System.EventHandler(this.checkAllButton_Click);
      // 
      // uncheckAllButton
      // 
      this.uncheckAllButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.uncheckAllButton.Location = new System.Drawing.Point(546, 179);
      this.uncheckAllButton.Name = "uncheckAllButton";
      this.uncheckAllButton.Size = new System.Drawing.Size(135, 23);
      this.uncheckAllButton.TabIndex = 3;
      this.uncheckAllButton.Text = "Uncheck All Privileges";
      this.uncheckAllButton.UseVisualStyleBackColor = true;
      this.uncheckAllButton.Click += new System.EventHandler(this.uncheckAllButton_Click);
      // 
      // label4
      // 
      this.label4.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
      this.label4.Location = new System.Drawing.Point(547, 28);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(233, 122);
      this.label4.TabIndex = 2;
      this.label4.Text = resources.GetString("label4.Text");
      // 
      // splitContainer2
      // 
      this.splitContainer2.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left)));
      this.splitContainer2.Location = new System.Drawing.Point(12, 12);
      this.splitContainer2.Name = "splitContainer2";
      // 
      // splitContainer2.Panel1
      // 
      this.splitContainer2.Panel1.Controls.Add(this.roleObjectsTreeView);
      this.splitContainer2.Panel1.Controls.Add(this.label2);
      // 
      // splitContainer2.Panel2
      // 
      this.splitContainer2.Panel2.Controls.Add(this.privCheckedListBox);
      this.splitContainer2.Panel2.Controls.Add(this.label3);
      this.splitContainer2.Size = new System.Drawing.Size(528, 193);
      this.splitContainer2.SplitterDistance = 254;
      this.splitContainer2.TabIndex = 1;
      // 
      // roleObjectsTreeView
      // 
      this.roleObjectsTreeView.AllowDrop = true;
      this.roleObjectsTreeView.ShowPlusMinus = false;
      this.roleObjectsTreeView.RowHeight = 18;
      this.roleObjectsTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.roleObjectsTreeView.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.roleObjectsTreeView.ContextMenuStrip = this.objectsContextMenu;
      this.roleObjectsTreeView.DefaultToolTipProvider = null;
      this.roleObjectsTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.roleObjectsTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.roleObjectsTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.roleObjectsTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.roleObjectsTreeView.Location = new System.Drawing.Point(0, 16);
      this.roleObjectsTreeView.Model = null;
      this.roleObjectsTreeView.Name = "roleObjectsTreeView";
      this.roleObjectsTreeView.NodeControls.Add(this.objectIconNodeControl);
      this.roleObjectsTreeView.NodeControls.Add(this.objectNameNodeControl);
      this.roleObjectsTreeView.SelectedNode = null;
      this.roleObjectsTreeView.ShowLines = false;
      this.roleObjectsTreeView.Size = new System.Drawing.Size(254, 177);
      this.roleObjectsTreeView.TabIndex = 1;
      this.roleObjectsTreeView.SelectionChanged += new System.EventHandler(this.roleObjectsTreeView_SelectionChanged);
      this.roleObjectsTreeView.DragDrop += new System.Windows.Forms.DragEventHandler(this.roleObjectsTreeView_DragDrop);
      this.roleObjectsTreeView.DragEnter += new System.Windows.Forms.DragEventHandler(this.roleObjectsTreeView_DragEnter);
      // 
      // objectsContextMenu
      // 
      this.objectsContextMenu.Name = "objectsContextMenu";
      this.objectsContextMenu.Size = new System.Drawing.Size(61, 4);
      this.objectsContextMenu.Opening += new System.ComponentModel.CancelEventHandler(this.objectsContextMenu_Opening);
      // 
      // objectIconNodeControl
      // 
      this.objectIconNodeControl.LeftMargin = 0;
      this.objectIconNodeControl.ParentColumn = null;
      this.objectIconNodeControl.VirtualMode = true;
      // 
      // objectNameNodeControl
      // 
      this.objectNameNodeControl.EditEnabled = false;
      this.objectNameNodeControl.IncrementalSearchEnabled = true;
      this.objectNameNodeControl.LeftMargin = 3;
      this.objectNameNodeControl.ParentColumn = null;
      this.objectNameNodeControl.VirtualMode = true;
      // 
      // label2
      // 
      this.label2.AutoSize = true;
      this.label2.Dock = System.Windows.Forms.DockStyle.Top;
      this.label2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
      this.label2.Location = new System.Drawing.Point(0, 0);
      this.label2.Name = "label2";
      this.label2.Padding = new System.Windows.Forms.Padding(0, 0, 0, 3);
      this.label2.Size = new System.Drawing.Size(50, 16);
      this.label2.TabIndex = 1;
      this.label2.Text = "Objects";
      // 
      // privCheckedListBox
      // 
      this.privCheckedListBox.CheckOnClick = true;
      this.privCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.privCheckedListBox.FormattingEnabled = true;
      this.privCheckedListBox.IntegralHeight = false;
      this.privCheckedListBox.Location = new System.Drawing.Point(0, 16);
      this.privCheckedListBox.Name = "privCheckedListBox";
      this.privCheckedListBox.Size = new System.Drawing.Size(270, 177);
      this.privCheckedListBox.TabIndex = 2;
      this.privCheckedListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.privCheckedListBox_ItemCheck);
      // 
      // label3
      // 
      this.label3.AutoSize = true;
      this.label3.Dock = System.Windows.Forms.DockStyle.Top;
      this.label3.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold);
      this.label3.Location = new System.Drawing.Point(0, 0);
      this.label3.Name = "label3";
      this.label3.Padding = new System.Windows.Forms.Padding(0, 0, 0, 3);
      this.label3.Size = new System.Drawing.Size(62, 16);
      this.label3.TabIndex = 1;
      this.label3.Text = "Privileges";
      // 
      // catalogColumn
      // 
      this.catalogColumn.Header = "All Objects";
      this.catalogColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.catalogColumn.TooltipText = null;
      this.catalogColumn.Width = 170;
      // 
      // catalogObjectNameNodeControl
      // 
      this.catalogObjectNameNodeControl.IncrementalSearchEnabled = true;
      this.catalogObjectNameNodeControl.LeftMargin = 3;
      this.catalogObjectNameNodeControl.ParentColumn = this.catalogColumn;
      this.catalogObjectNameNodeControl.VirtualMode = true;
      // 
      // catalogNameNodeControl
      // 
      this.catalogNameNodeControl.IncrementalSearchEnabled = true;
      this.catalogNameNodeControl.LeftMargin = 3;
      this.catalogNameNodeControl.ParentColumn = this.roleColumn;
      // 
      // DbMysqlRoleEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.Color.White;
      this.ClientSize = new System.Drawing.Size(788, 234);
      this.Controls.Add(this.flatTabControl1);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F);
      this.Name = "DbMysqlRoleEditor";
      this.Text = "DbMysqlRoleEditor";
      this.flatTabControl1.ResumeLayout(false);
      this.rolePage.ResumeLayout(false);
      this.rolePage.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.privilegesPage.ResumeLayout(false);
      this.splitContainer2.Panel1.ResumeLayout(false);
      this.splitContainer2.Panel1.PerformLayout();
      this.splitContainer2.Panel2.ResumeLayout(false);
      this.splitContainer2.Panel2.PerformLayout();
      ((System.ComponentModel.ISupportInitialize)(this.splitContainer2)).EndInit();
      this.splitContainer2.ResumeLayout(false);
      this.ResumeLayout(false);

    }

    #endregion

    private MySQL.Controls.FlatTabControl flatTabControl1;
    private System.Windows.Forms.TabPage rolePage;
    private Aga.Controls.Tree.TreeViewAdv rolesTreeView;
    private Aga.Controls.Tree.TreeColumn roleColumn;
    private Aga.Controls.Tree.NodeControls.NodeTextBox catalogNameNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeTextBox roleNameNodeControl;
    private Aga.Controls.Tree.TreeColumn catalogColumn;
    private Aga.Controls.Tree.NodeControls.NodeTextBox catalogObjectNameNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeIcon objectIconNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeTextBox objectNameNodeControl;
    private System.Windows.Forms.SplitContainer splitContainer2;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.TabPage privilegesPage;
    private System.Windows.Forms.Label label4;
    private Aga.Controls.Tree.TreeViewAdv roleObjectsTreeView;
    private System.Windows.Forms.CheckedListBox privCheckedListBox;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Button checkAllButton;
    private System.Windows.Forms.Button uncheckAllButton;
    private System.Windows.Forms.ComboBox ParentComboBox;
    private System.Windows.Forms.TextBox nameTextBox;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.Label label6;
    private System.Windows.Forms.ContextMenuStrip objectsContextMenu;
  }
}
