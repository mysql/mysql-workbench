namespace MySQL.GUI.Workbench
{
	partial class ModelCatalogForm
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ModelCatalogForm));
      this.topPanel = new System.Windows.Forms.Panel();
      this.comboBox1 = new System.Windows.Forms.ComboBox();
      this.nodeStateIcon = new Aga.Controls.Tree.NodeControls.NodeStateIcon();
      this.nameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.presenceNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.headerPanel1 = new MySQL.Controls.HeaderPanel();
      this.topPanel.SuspendLayout();
      this.headerPanel1.SuspendLayout();
      this.SuspendLayout();
      // 
      // topPanel
      // 
      this.topPanel.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(189)))), ((int)(((byte)(199)))), ((int)(((byte)(222)))));
      this.topPanel.Controls.Add(this.comboBox1);
      this.topPanel.Dock = System.Windows.Forms.DockStyle.Top;
      this.topPanel.Location = new System.Drawing.Point(0, 24);
      this.topPanel.Margin = new System.Windows.Forms.Padding(0);
      this.topPanel.Name = "topPanel";
      this.topPanel.Padding = new System.Windows.Forms.Padding(4);
      this.topPanel.Size = new System.Drawing.Size(279, 28);
      this.topPanel.TabIndex = 3;
      this.topPanel.Visible = false;
      // 
      // comboBox1
      // 
      this.comboBox1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.comboBox1.FormattingEnabled = true;
      this.comboBox1.Location = new System.Drawing.Point(4, 4);
      this.comboBox1.Name = "comboBox1";
      this.comboBox1.Size = new System.Drawing.Size(271, 21);
      this.comboBox1.TabIndex = 1;
      // 
      // nodeStateIcon
      // 
      this.nodeStateIcon.LeftMargin = 1;
      this.nodeStateIcon.ParentColumn = null;
      // 
      // nameNodeControl
      // 
      this.nameNodeControl.DataPropertyName = "Text";
      this.nameNodeControl.IncrementalSearchEnabled = true;
      this.nameNodeControl.LeftMargin = 3;
      this.nameNodeControl.ParentColumn = null;
      // 
      // presenceNodeControl
      // 
      this.presenceNodeControl.IncrementalSearchEnabled = true;
      this.presenceNodeControl.LeftMargin = 0;
      this.presenceNodeControl.ParentColumn = null;
      // 
      // headerPanel1
      // 
      this.headerPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.headerPanel1.Controls.Add(this.topPanel);
      this.headerPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.headerPanel1.ForeColor = System.Drawing.Color.White;
      this.headerPanel1.ForeColorFocused = System.Drawing.Color.White;
      this.headerPanel1.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 0, 0);
      this.headerPanel1.Location = new System.Drawing.Point(4, 3);
      this.headerPanel1.Name = "headerPanel1";
      this.headerPanel1.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.headerPanel1.Size = new System.Drawing.Size(279, 337);
      this.headerPanel1.TabIndex = 4;
      this.headerPanel1.Text = "Catalog Tree";
      // 
      // ModelCatalogForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.SystemColors.Window;
      this.ClientSize = new System.Drawing.Size(287, 343);
      this.Controls.Add(this.headerPanel1);
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "ModelCatalogForm";
      this.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
      this.TabText = "Catalog";
      this.Text = "Catalog";
      this.topPanel.ResumeLayout(false);
      this.headerPanel1.ResumeLayout(false);
      this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.Panel topPanel;
    private System.Windows.Forms.ComboBox comboBox1;
		private Aga.Controls.Tree.NodeControls.NodeStateIcon nodeStateIcon;
    private Aga.Controls.Tree.NodeControls.NodeTextBox nameNodeControl;
    private Aga.Controls.Tree.NodeControls.NodeTextBox presenceNodeControl;
    private MySQL.Controls.HeaderPanel headerPanel1;


	}
}