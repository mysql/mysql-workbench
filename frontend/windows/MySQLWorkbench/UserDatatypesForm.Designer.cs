namespace MySQL.GUI.Workbench
{
  partial class UserDatatypesForm
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
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(UserDatatypesForm));
      this.nameColumn = new Aga.Controls.Tree.TreeColumn();
      this.definitionColumn = new Aga.Controls.Tree.TreeColumn();
      this.flagsColumn = new Aga.Controls.Tree.TreeColumn();
      this.nodeStateIcon = new Aga.Controls.Tree.NodeControls.NodeStateIcon();
      this.nameTextBox = new MySQL.Utilities.AdvNodeTextBox();
      this.definitionTextBox = new MySQL.Utilities.AdvNodeTextBox();
      this.flagsTextBox = new Aga.Controls.Tree.NodeControls.NodeTextBox();
      this.headerPanel1 = new MySQL.Controls.HeaderPanel();
      this.SuspendLayout();
      // 
      // nameColumn
      // 
      this.nameColumn.Header = "Name";
      this.nameColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.nameColumn.TooltipText = null;
      this.nameColumn.Width = 75;
      // 
      // definitionColumn
      // 
      this.definitionColumn.Header = "Definition";
      this.definitionColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.definitionColumn.TooltipText = null;
      this.definitionColumn.Width = 80;
      // 
      // flagsColumn
      // 
      this.flagsColumn.Header = "Flags";
      this.flagsColumn.SortOrder = System.Windows.Forms.SortOrder.None;
      this.flagsColumn.TooltipText = null;
      this.flagsColumn.Width = 65;
      // 
      // nodeStateIcon
      // 
      this.nodeStateIcon.LeftMargin = 0;
      this.nodeStateIcon.ParentColumn = this.nameColumn;
      // 
      // nameTextBox
      // 
      this.nameTextBox.IncrementalSearchEnabled = true;
      this.nameTextBox.LeftMargin = 3;
      this.nameTextBox.ParentColumn = this.nameColumn;
      this.nameTextBox.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      // 
      // definitionTextBox
      // 
      this.definitionTextBox.IncrementalSearchEnabled = true;
      this.definitionTextBox.LeftMargin = 3;
      this.definitionTextBox.ParentColumn = this.definitionColumn;
      this.definitionTextBox.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      // 
      // flagsTextBox
      // 
      this.flagsTextBox.IncrementalSearchEnabled = true;
      this.flagsTextBox.LeftMargin = 3;
      this.flagsTextBox.ParentColumn = this.flagsColumn;
      this.flagsTextBox.Trimming = System.Drawing.StringTrimming.EllipsisCharacter;
      // 
      // headerPanel1
      // 
      this.headerPanel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(40)))), ((int)(((byte)(55)))), ((int)(((byte)(82)))));
      this.headerPanel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.headerPanel1.ForeColor = System.Drawing.Color.White;
      this.headerPanel1.ForeColorFocused = System.Drawing.Color.White;
      this.headerPanel1.HeaderColor = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderColorFocused = System.Drawing.Color.FromArgb(((int)(((byte)(73)))), ((int)(((byte)(97)))), ((int)(((byte)(132)))));
      this.headerPanel1.HeaderPadding = new System.Windows.Forms.Padding(5, 0, 5, 0);
      this.headerPanel1.Location = new System.Drawing.Point(0, 0);
      this.headerPanel1.Name = "headerPanel1";
      this.headerPanel1.Padding = new System.Windows.Forms.Padding(0, 24, 0, 0);
      this.headerPanel1.Size = new System.Drawing.Size(242, 266);
      this.headerPanel1.TabIndex = 2;
      this.headerPanel1.Text = "User Types List";
      // 
      // UserDatatypesForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(242, 266);
      this.Controls.Add(this.headerPanel1);
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.Name = "UserDatatypesForm";
      this.TabText = "UserDatatypesForm";
      this.Text = "UserDatatypesForm";
      this.ResumeLayout(false);

    }

    #endregion

    private Aga.Controls.Tree.TreeColumn nameColumn;
    private Aga.Controls.Tree.TreeColumn definitionColumn;
    private MySQL.Utilities.AdvNodeTextBox nameTextBox;
    private MySQL.Utilities.AdvNodeTextBox definitionTextBox;
    private Aga.Controls.Tree.NodeControls.NodeStateIcon nodeStateIcon;
    private Aga.Controls.Tree.TreeColumn flagsColumn;
    private Aga.Controls.Tree.NodeControls.NodeTextBox flagsTextBox;
    private MySQL.Controls.HeaderPanel headerPanel1;
  }
}