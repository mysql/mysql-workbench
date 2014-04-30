namespace MySQL.GUI.Workbench
{
    partial class ModelLayerForm
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ModelLayerForm));
      this.layersTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.selectionTreeColumn = new Aga.Controls.Tree.TreeColumn();
      this.nodeStateIcon = new Aga.Controls.Tree.NodeControls.NodeStateIcon();
      this.nameNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.selectedNodeControl = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.headerPanel1 = new MySQL.Controls.HeaderPanel();
      this.SuspendLayout();
      // 
      // layersTreeColumn
      // 
      this.layersTreeColumn.Header = "Layers && Figures";
      this.layersTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.layersTreeColumn.TooltipText = null;
      this.layersTreeColumn.Width = 150;
      // 
      // selectionTreeColumn
      // 
      this.selectionTreeColumn.Header = "";
      this.selectionTreeColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.selectionTreeColumn.TooltipText = null;
      this.selectionTreeColumn.Width = 20;
      // 
      // nodeStateIcon
      // 
      this.nodeStateIcon.LeftMargin = 1;
      this.nodeStateIcon.ParentColumn = this.layersTreeColumn;
      // 
      // nameNodeControl
      // 
      this.nameNodeControl.DataPropertyName = "Text";
      this.nameNodeControl.IncrementalSearchEnabled = true;
      this.nameNodeControl.LeftMargin = 3;
      this.nameNodeControl.ParentColumn = this.layersTreeColumn;
      this.nameNodeControl.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      // 
      // selectedNodeControl
      // 
      this.selectedNodeControl.DataPropertyName = "Text";
      this.selectedNodeControl.IncrementalSearchEnabled = true;
      this.selectedNodeControl.LeftMargin = 0;
      this.selectedNodeControl.ParentColumn = this.selectionTreeColumn;
      // 
      // headerPanel1
      // 
      this.headerPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.headerPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.headerPanel1.ForeColor = System.Drawing.Color.White;
      this.headerPanel1.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 0, 0);
      this.headerPanel1.Location = new System.Drawing.Point(4, 3);
      this.headerPanel1.Name = "headerPanel1";
      this.headerPanel1.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.headerPanel1.Size = new System.Drawing.Size(279, 356);
      this.headerPanel1.TabIndex = 4;
      this.headerPanel1.Text = "Layer Tree";
      // 
      // ModelLayerForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.BackColor = System.Drawing.SystemColors.Window;
      this.ClientSize = new System.Drawing.Size(287, 362);
      this.Controls.Add(this.headerPanel1);
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "ModelLayerForm";
      this.Padding = new System.Windows.Forms.Padding(4, 3, 4, 3);
      this.TabText = "Layers";
      this.Text = "Layers";
      this.ResumeLayout(false);

        }

        #endregion

        private Aga.Controls.Tree.NodeControls.NodeStateIcon nodeStateIcon;
      private Aga.Controls.Tree.NodeControls.NodeTextBox nameNodeControl;
      private Aga.Controls.Tree.NodeControls.NodeTextBox selectedNodeControl;
      private Aga.Controls.Tree.TreeColumn layersTreeColumn;
      private Aga.Controls.Tree.TreeColumn selectionTreeColumn;
      private MySQL.Controls.HeaderPanel headerPanel1;

    }
}