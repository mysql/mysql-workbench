namespace MySQL.GUI.Workbench.Plugins
{
  partial class UserDatatypesEditor
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
      this.treeViewAdv1 = new Aga.Controls.Tree.TreeViewAdv();
      this.nameColumn = new Aga.Controls.Tree.TreeColumn();
      this.definitionColumn = new Aga.Controls.Tree.TreeColumn();
      this.nameTextBox = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.definitionTextBox = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.SuspendLayout();
      // 
      // treeViewAdv1
      // 
      this.treeViewAdv1.BackColor = System.Drawing.SystemColors.Window;
      this.treeViewAdv1.Columns.Add(this.nameColumn);
      this.treeViewAdv1.Columns.Add(this.definitionColumn);
      this.treeViewAdv1.DefaultToolTipProvider = null;
      this.treeViewAdv1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.treeViewAdv1.DragDropMarkColor = System.Drawing.Color.Black;
      this.treeViewAdv1.LineColor = System.Drawing.SystemColors.ControlDark;
      this.treeViewAdv1.Location = new System.Drawing.Point(0, 0);
      this.treeViewAdv1.Model = null;
      this.treeViewAdv1.Name = "treeViewAdv1";
      this.treeViewAdv1.NodeControls.Add(this.nameTextBox);
      this.treeViewAdv1.NodeControls.Add(this.definitionTextBox);
      this.treeViewAdv1.SelectedNode = null;
      this.treeViewAdv1.Size = new System.Drawing.Size(452, 219);
      this.treeViewAdv1.TabIndex = 0;
      this.treeViewAdv1.Text = "usertypeTree";
      // 
      // nameColumn
      // 
      this.nameColumn.Header = "Name";
      this.nameColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.nameColumn.TooltipText = null;
      // 
      // definitionColumn
      // 
      this.definitionColumn.Header = "Definition";
      this.definitionColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.definitionColumn.TooltipText = null;
      // 
      // nameTextBox
      // 
      this.nameTextBox.IncrementalSearchEnabled = true;
      this.nameTextBox.LeftMargin = 3;
      this.nameTextBox.ParentColumn = this.nameColumn;
      // 
      // definitionTextBox
      // 
      this.definitionTextBox.IncrementalSearchEnabled = true;
      this.definitionTextBox.LeftMargin = 3;
      this.definitionTextBox.ParentColumn = this.definitionColumn;
      // 
      // UserDatatypesEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(452, 219);
      this.Controls.Add(this.treeViewAdv1);
      this.Name = "UserDatatypesEditor";
      this.ShowIcon = false;
      this.Text = "User Datatypes";
      this.ResumeLayout(false);

    }

    #endregion

    private Aga.Controls.Tree.TreeViewAdv treeViewAdv1;
    private Aga.Controls.Tree.TreeColumn nameColumn;
    private Aga.Controls.Tree.TreeColumn definitionColumn;
    private Aga.Controls.Tree.NodeControls.NodeTextBox nameTextBox;
    private Aga.Controls.Tree.NodeControls.NodeTextBox definitionTextBox;
  }
}