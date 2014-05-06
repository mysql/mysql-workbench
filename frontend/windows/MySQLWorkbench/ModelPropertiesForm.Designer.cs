namespace MySQL.GUI.Workbench
{
	partial class ModelPropertiesForm
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ModelPropertiesForm));
            this.propertiesTreeView = new Aga.Controls.Tree.TreeViewAdv();
            this.nameTreeColumn = new Aga.Controls.Tree.TreeColumn();
            this.valueTreeColumn = new Aga.Controls.Tree.TreeColumn();
            this.nameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
            this.headerPanel1 = new MySQL.Controls.HeaderPanel();
            this.headerPanel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // propertiesTreeView
            // 
            this.propertiesTreeView.BackColor = System.Drawing.SystemColors.Window;
            this.propertiesTreeView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.propertiesTreeView.Columns.Add(this.nameTreeColumn);
            this.propertiesTreeView.Columns.Add(this.valueTreeColumn);
            this.propertiesTreeView.DefaultToolTipProvider = null;
            this.propertiesTreeView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.propertiesTreeView.DragDropMarkColor = System.Drawing.Color.Black;
            this.propertiesTreeView.FullRowSelect = true;
            this.propertiesTreeView.GridColor = System.Drawing.SystemColors.Control;
            this.propertiesTreeView.GridLineStyle = Aga.Controls.Tree.GridLineStyle.Horizontal;
            this.propertiesTreeView.Indent = 16;
            this.propertiesTreeView.LineColor = System.Drawing.SystemColors.ControlDark;
            this.propertiesTreeView.LoadOnDemand = true;
            this.propertiesTreeView.Location = new System.Drawing.Point(0, 24);
            this.propertiesTreeView.Model = null;
            this.propertiesTreeView.Name = "propertiesTreeView";
            this.propertiesTreeView.NodeControls.Add(this.nameNodeControl);
            this.propertiesTreeView.SelectedNode = null;
            this.propertiesTreeView.SelectionMode = Aga.Controls.Tree.TreeSelectionMode.Multi;
            this.propertiesTreeView.ShowLines = false;
            this.propertiesTreeView.ShowNodeToolTips = true;
            this.propertiesTreeView.ShowPlusMinus = false;
            this.propertiesTreeView.Size = new System.Drawing.Size(224, 245);
            this.propertiesTreeView.TabIndex = 0;
            this.propertiesTreeView.UseColumns = true;
            // 
            // nameTreeColumn
            // 
            this.nameTreeColumn.Header = "Name";
            this.nameTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
            this.nameTreeColumn.TooltipText = null;
            this.nameTreeColumn.Width = 108;
            // 
            // valueTreeColumn
            // 
            this.valueTreeColumn.Header = "Value";
            this.valueTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
            this.valueTreeColumn.TooltipText = null;
            this.valueTreeColumn.Width = 90;
            // 
            // nameNodeControl
            // 
            this.nameNodeControl.DataPropertyName = "Text";
            this.nameNodeControl.EditEnabled = false;
            this.nameNodeControl.IncrementalSearchEnabled = true;
            this.nameNodeControl.LeftMargin = 3;
            this.nameNodeControl.ParentColumn = this.nameTreeColumn;
            this.nameNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
            this.nameNodeControl.VirtualMode = true;
            // 
            // headerPanel1
            // 
            this.headerPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
            this.headerPanel1.Controls.Add(this.propertiesTreeView);
            this.headerPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.headerPanel1.ForeColor = System.Drawing.Color.White;
            this.headerPanel1.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
            this.headerPanel1.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 0, 0);
            this.headerPanel1.Location = new System.Drawing.Point(4, 3);
            this.headerPanel1.Name = "headerPanel1";
            this.headerPanel1.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
            this.headerPanel1.Size = new System.Drawing.Size(224, 269);
            this.headerPanel1.TabIndex = 2;
            this.headerPanel1.Text = "Properties Editor";
            // 
            // ModelPropertiesForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.SystemColors.Window;
            this.ClientSize = new System.Drawing.Size(232, 275);
            this.Controls.Add(this.headerPanel1);
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Name = "ModelPropertiesForm";
            this.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
            this.TabText = "Properties";
            this.Text = "Properties";
            this.headerPanel1.ResumeLayout(false);
            this.ResumeLayout(false);

		}

		#endregion

        private Aga.Controls.Tree.TreeViewAdv propertiesTreeView;
		private Aga.Controls.Tree.TreeColumn nameTreeColumn;
		private Aga.Controls.Tree.TreeColumn valueTreeColumn;
    private Aga.Controls.Tree.NodeControls.NodeTextBox nameNodeControl;
    private MySQL.Controls.HeaderPanel headerPanel1;
	}
}