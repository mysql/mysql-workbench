using MySQL.Utilities;

namespace MySQL.GUI.Workbench.Plugins
{
	partial class DbObjectEditorPages
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
      this.commonTabControl = new MySQL.Controls.FlatTabControl();
      this.privilegesTabPage = new System.Windows.Forms.TabPage();
      this.privSplitContainer1 = new System.Windows.Forms.SplitContainer();
      this.rolesTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.rolesColumn = new Aga.Controls.Tree.TreeColumn();
      this.roleNameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.privLabel = new System.Windows.Forms.Label();
      this.privCheckedListBox = new System.Windows.Forms.CheckedListBox();
      this.privAllRolesPanel = new System.Windows.Forms.Panel();
      this.allRolesTreeView = new Aga.Controls.Tree.TreeViewAdv();
      this.allRolesColumn = new Aga.Controls.Tree.TreeColumn();
      this.allRolesNameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.privAllRolesButtonPanel = new System.Windows.Forms.Panel();
      this.roleRemoveButton = new System.Windows.Forms.Button();
      this.roleAssignButton = new System.Windows.Forms.Button();
      this.panel1 = new System.Windows.Forms.Panel();
      this.commonTabControl.SuspendLayout();
      this.privilegesTabPage.SuspendLayout();
      ((System.ComponentModel.ISupportInitialize)(this.privSplitContainer1)).BeginInit();
      this.privSplitContainer1.Panel1.SuspendLayout();
      this.privSplitContainer1.Panel2.SuspendLayout();
      this.privSplitContainer1.SuspendLayout();
      this.privAllRolesPanel.SuspendLayout();
      this.privAllRolesButtonPanel.SuspendLayout();
      this.panel1.SuspendLayout();
      this.SuspendLayout();
      // 
      // commonTabControl
      // 
      this.commonTabControl.Alignment = System.Windows.Forms.TabAlignment.Bottom;
      this.commonTabControl.BackgroundColor = System.Drawing.Color.White;
      this.commonTabControl.CanCloseLastTab = false;
      this.commonTabControl.CanReorderTabs = false;
      this.commonTabControl.ContentPadding = new System.Windows.Forms.Padding(0);
      this.commonTabControl.Controls.Add(this.privilegesTabPage);
      this.commonTabControl.Dock = System.Windows.Forms.DockStyle.Fill;
      this.commonTabControl.HideWhenEmpty = false;
      this.commonTabControl.ItemPadding = new System.Windows.Forms.Padding(6, 0, 6, 0);
      this.commonTabControl.Location = new System.Drawing.Point(0, 0);
      this.commonTabControl.MaxTabSize = 200;
      this.commonTabControl.Name = "commonTabControl";
      this.commonTabControl.SelectedIndex = 0;
      this.commonTabControl.ShowCloseButton = true;
      this.commonTabControl.ShowFocusState = true;
      this.commonTabControl.Size = new System.Drawing.Size(800, 236);
      this.commonTabControl.TabIndex = 0;
      this.commonTabControl.TabStyle = MySQL.Controls.FlatTabControl.TabStyleType.TopNormal;
      this.commonTabControl.Visible = false;
      // 
      // privilegesTabPage
      // 
      this.privilegesTabPage.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.privilegesTabPage.Controls.Add(this.privSplitContainer1);
      this.privilegesTabPage.Location = new System.Drawing.Point(3, 24);
      this.privilegesTabPage.Name = "privilegesTabPage";
      this.privilegesTabPage.Padding = new System.Windows.Forms.Padding(8);
      this.privilegesTabPage.Size = new System.Drawing.Size(794, 209);
      this.privilegesTabPage.TabIndex = 0;
      this.privilegesTabPage.Text = "Privileges";
      // 
      // privSplitContainer1
      // 
      this.privSplitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.privSplitContainer1.Location = new System.Drawing.Point(8, 8);
      this.privSplitContainer1.Name = "privSplitContainer1";
      // 
      // privSplitContainer1.Panel1
      // 
      this.privSplitContainer1.Panel1.Controls.Add(this.rolesTreeView);
      // 
      // privSplitContainer1.Panel2
      // 
      this.privSplitContainer1.Panel2.Controls.Add(this.panel1);
      this.privSplitContainer1.Panel2.Controls.Add(this.privAllRolesPanel);
      this.privSplitContainer1.Panel2.Padding = new System.Windows.Forms.Padding(4, 0, 0, 0);
      this.privSplitContainer1.Size = new System.Drawing.Size(778, 193);
      this.privSplitContainer1.SplitterDistance = 227;
      this.privSplitContainer1.TabIndex = 4;
      // 
      // rolesTreeView
      // 
      this.rolesTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.rolesTreeView.Columns.Add(this.rolesColumn);
      this.rolesTreeView.DefaultToolTipProvider = null;
      this.rolesTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.rolesTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.rolesTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.rolesTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
      this.rolesTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.rolesTreeView.LoadOnDemand = true;
      this.rolesTreeView.Location = new System.Drawing.Point(0, 0);
      this.rolesTreeView.Model = null;
      this.rolesTreeView.Name = "rolesTreeView";
      this.rolesTreeView.NodeControls.Add(this.roleNameNodeControl);
      this.rolesTreeView.SelectedNode = null;
      this.rolesTreeView.ShowLines = false;
      this.rolesTreeView.ShowPlusMinus = false;
      this.rolesTreeView.Size = new System.Drawing.Size(227, 193);
      this.rolesTreeView.TabIndex = 0;
      this.rolesTreeView.UseColumns = true;
      this.rolesTreeView.SelectionChanged += new System.EventHandler(this.rolesTreeView_SelectionChanged);
      // 
      // rolesColumn
      // 
      this.rolesColumn.Header = "Assigned Roles";
      this.rolesColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.rolesColumn.TooltipText = null;
      this.rolesColumn.Width = 200;
      // 
      // roleNameNodeControl
      // 
      this.roleNameNodeControl.DataPropertyName = "Text";
      this.roleNameNodeControl.EditEnabled = false;
      this.roleNameNodeControl.IncrementalSearchEnabled = true;
      this.roleNameNodeControl.LeftMargin = 3;
      this.roleNameNodeControl.ParentColumn = this.rolesColumn;
      // 
      // privLabel
      // 
      this.privLabel.AutoSize = true;
      this.privLabel.Dock = System.Windows.Forms.DockStyle.Top;
      this.privLabel.Location = new System.Drawing.Point(0, 0);
      this.privLabel.Name = "privLabel";
      this.privLabel.Size = new System.Drawing.Size(98, 13);
      this.privLabel.TabIndex = 12;
      this.privLabel.Text = "Assigned Privileges";
      // 
      // privCheckedListBox
      // 
      this.privCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.privCheckedListBox.FormattingEnabled = true;
      this.privCheckedListBox.IntegralHeight = false;
      this.privCheckedListBox.Location = new System.Drawing.Point(0, 13);
      this.privCheckedListBox.Name = "privCheckedListBox";
      this.privCheckedListBox.Size = new System.Drawing.Size(235, 180);
      this.privCheckedListBox.TabIndex = 0;
      this.privCheckedListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.privCheckedListBox_ItemCheck);
      // 
      // privAllRolesPanel
      // 
      this.privAllRolesPanel.Controls.Add(this.allRolesTreeView);
      this.privAllRolesPanel.Controls.Add(this.privAllRolesButtonPanel);
      this.privAllRolesPanel.Dock = System.Windows.Forms.DockStyle.Right;
      this.privAllRolesPanel.Location = new System.Drawing.Point(239, 0);
      this.privAllRolesPanel.Name = "privAllRolesPanel";
      this.privAllRolesPanel.Padding = new System.Windows.Forms.Padding(3, 0, 0, 0);
      this.privAllRolesPanel.Size = new System.Drawing.Size(308, 193);
      this.privAllRolesPanel.TabIndex = 4;
      // 
      // allRolesTreeView
      // 
      this.allRolesTreeView.BackColor = System.Drawing.SystemColors.Window;
      this.allRolesTreeView.Columns.Add(this.allRolesColumn);
      this.allRolesTreeView.DefaultToolTipProvider = null;
      this.allRolesTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
      this.allRolesTreeView.DragDropMarkColor = System.Drawing.Color.Black;
      this.allRolesTreeView.GridColor = System.Drawing.SystemColors.Control;
      this.allRolesTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
      this.allRolesTreeView.LoadOnDemand = true;
      this.allRolesTreeView.Location = new System.Drawing.Point(44, 0);
      this.allRolesTreeView.Model = null;
      this.allRolesTreeView.Name = "allRolesTreeView";
      this.allRolesTreeView.NodeControls.Add(this.allRolesNameNodeControl);
      this.allRolesTreeView.SelectedNode = null;
      this.allRolesTreeView.SelectionMode = Aga.Controls.Tree.TreeSelectionMode.MultiSameParent;
      this.allRolesTreeView.ShowLines = false;
      this.allRolesTreeView.ShowPlusMinus = false;
      this.allRolesTreeView.Size = new System.Drawing.Size(264, 193);
      this.allRolesTreeView.TabIndex = 0;
      this.allRolesTreeView.Text = "treeViewAdv1";
      this.allRolesTreeView.UseColumns = true;
      this.allRolesTreeView.SelectionChanged += new System.EventHandler(this.allRolesTreeView_SelectionChanged);
      // 
      // allRolesColumn
      // 
      this.allRolesColumn.Header = "Available Roles";
      this.allRolesColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.allRolesColumn.TooltipText = null;
      this.allRolesColumn.Width = 235;
      // 
      // allRolesNameNodeControl
      // 
      this.allRolesNameNodeControl.DataPropertyName = "Text";
      this.allRolesNameNodeControl.EditEnabled = false;
      this.allRolesNameNodeControl.IncrementalSearchEnabled = true;
      this.allRolesNameNodeControl.LeftMargin = 3;
      this.allRolesNameNodeControl.ParentColumn = this.allRolesColumn;
      // 
      // privAllRolesButtonPanel
      // 
      this.privAllRolesButtonPanel.Controls.Add(this.roleRemoveButton);
      this.privAllRolesButtonPanel.Controls.Add(this.roleAssignButton);
      this.privAllRolesButtonPanel.Dock = System.Windows.Forms.DockStyle.Left;
      this.privAllRolesButtonPanel.Location = new System.Drawing.Point(3, 0);
      this.privAllRolesButtonPanel.Name = "privAllRolesButtonPanel";
      this.privAllRolesButtonPanel.Size = new System.Drawing.Size(41, 193);
      this.privAllRolesButtonPanel.TabIndex = 1;
      // 
      // roleRemoveButton
      // 
      this.roleRemoveButton.Enabled = false;
      this.roleRemoveButton.Location = new System.Drawing.Point(3, 46);
      this.roleRemoveButton.Name = "roleRemoveButton";
      this.roleRemoveButton.Size = new System.Drawing.Size(32, 23);
      this.roleRemoveButton.TabIndex = 1;
      this.roleRemoveButton.Text = ">";
      this.roleRemoveButton.UseVisualStyleBackColor = true;
      this.roleRemoveButton.Click += new System.EventHandler(this.roleRemoveButton_Click);
      // 
      // roleAssignButton
      // 
      this.roleAssignButton.Enabled = false;
      this.roleAssignButton.Location = new System.Drawing.Point(3, 17);
      this.roleAssignButton.Name = "roleAssignButton";
      this.roleAssignButton.Size = new System.Drawing.Size(32, 23);
      this.roleAssignButton.TabIndex = 0;
      this.roleAssignButton.Text = "<";
      this.roleAssignButton.UseVisualStyleBackColor = true;
      this.roleAssignButton.Click += new System.EventHandler(this.roleAssignButton_Click);
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.privCheckedListBox);
      this.panel1.Controls.Add(this.privLabel);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.panel1.Location = new System.Drawing.Point(4, 0);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(235, 193);
      this.panel1.TabIndex = 5;
      // 
      // DbObjectEditorPages
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(800, 236);
      this.Controls.Add(this.commonTabControl);
      this.Name = "DbObjectEditorPages";
      this.TabText = "DbObjectEditor";
      this.Text = "DbObjectEditor";
      this.commonTabControl.ResumeLayout(false);
      this.privilegesTabPage.ResumeLayout(false);
      this.privSplitContainer1.Panel1.ResumeLayout(false);
      this.privSplitContainer1.Panel2.ResumeLayout(false);
      ((System.ComponentModel.ISupportInitialize)(this.privSplitContainer1)).EndInit();
      this.privSplitContainer1.ResumeLayout(false);
      this.privAllRolesPanel.ResumeLayout(false);
      this.privAllRolesButtonPanel.ResumeLayout(false);
      this.panel1.ResumeLayout(false);
      this.panel1.PerformLayout();
      this.ResumeLayout(false);

		}

		#endregion

    private MySQL.Controls.FlatTabControl commonTabControl;

    private System.Windows.Forms.TabPage privilegesTabPage;
    private System.Windows.Forms.SplitContainer privSplitContainer1;
    private Aga.Controls.Tree.TreeViewAdv rolesTreeView;
    private Aga.Controls.Tree.TreeColumn rolesColumn;
    private System.Windows.Forms.Label privLabel;
    private System.Windows.Forms.Panel privAllRolesPanel;
    private System.Windows.Forms.CheckedListBox privCheckedListBox;
    private Aga.Controls.Tree.TreeViewAdv allRolesTreeView;
    private System.Windows.Forms.Panel privAllRolesButtonPanel;
    private System.Windows.Forms.Button roleAssignButton;
    private System.Windows.Forms.Button roleRemoveButton;
    private Aga.Controls.Tree.NodeControls.NodeTextBox roleNameNodeControl;
    private Aga.Controls.Tree.TreeColumn allRolesColumn;
    private Aga.Controls.Tree.NodeControls.NodeTextBox allRolesNameNodeControl;
    private System.Windows.Forms.Panel panel1;
	}
}