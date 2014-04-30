namespace MySQL.Grt.Db
{
  partial class RecordsetView
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
        if (model != null)
        {
          // Remove the callbacks to free the fixed pointers the wrapper keeps to this object.
          model.refresh_ui_cb(null);
        }
        gridView.Dispose(); // Will dispose the model too.

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
      this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
      this.columnHeaderContextMenuStrip = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.setColumnFilterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.resetColumnFilterToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.resetAllColumnFiltersToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      this.columnHeaderContextMenuStrip.SuspendLayout();
      this.SuspendLayout();
      // 
      // toolStripSeparator1
      // 
      this.toolStripSeparator1.Name = "toolStripSeparator1";
      this.toolStripSeparator1.Size = new System.Drawing.Size(149, 6);
      // 
      // columnHeaderContextMenuStrip
      // 
      this.columnHeaderContextMenuStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.setColumnFilterToolStripMenuItem,
            this.resetColumnFilterToolStripMenuItem,
            this.resetAllColumnFiltersToolStripMenuItem});
      this.columnHeaderContextMenuStrip.Name = "columnHeaderContextMenuStrip";
      this.columnHeaderContextMenuStrip.Size = new System.Drawing.Size(200, 70);
      // 
      // setColumnFilterToolStripMenuItem
      // 
      this.setColumnFilterToolStripMenuItem.Name = "setColumnFilterToolStripMenuItem";
      this.setColumnFilterToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
      this.setColumnFilterToolStripMenuItem.Text = "Set Column Filter...";
      this.setColumnFilterToolStripMenuItem.Click += new System.EventHandler(this.setColumnFilterToolStripMenuItem_Click);
      // 
      // resetColumnFilterToolStripMenuItem
      // 
      this.resetColumnFilterToolStripMenuItem.Name = "resetColumnFilterToolStripMenuItem";
      this.resetColumnFilterToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
      this.resetColumnFilterToolStripMenuItem.Text = "Reset Column Filter";
      this.resetColumnFilterToolStripMenuItem.Click += new System.EventHandler(this.resetColumnFilterToolStripMenuItem_Click);
      // 
      // resetAllColumnFiltersToolStripMenuItem
      // 
      this.resetAllColumnFiltersToolStripMenuItem.Name = "resetAllColumnFiltersToolStripMenuItem";
      this.resetAllColumnFiltersToolStripMenuItem.Size = new System.Drawing.Size(199, 22);
      this.resetAllColumnFiltersToolStripMenuItem.Text = "Reset All Column Filters";
      this.resetAllColumnFiltersToolStripMenuItem.Click += new System.EventHandler(this.resetAllColumnFiltersToolStripMenuItem_Click);
      // 
      // RecordsetView
      // 
      this.ClientSize = new System.Drawing.Size(811, 375);
      this.Name = "RecordsetView";
      this.Text = "RecordsetView";
      this.columnHeaderContextMenuStrip.ResumeLayout(false);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
    private System.Windows.Forms.ContextMenuStrip columnHeaderContextMenuStrip;
    private System.Windows.Forms.ToolStripMenuItem setColumnFilterToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem resetColumnFilterToolStripMenuItem;
    private System.Windows.Forms.ToolStripMenuItem resetAllColumnFiltersToolStripMenuItem;
  }
}