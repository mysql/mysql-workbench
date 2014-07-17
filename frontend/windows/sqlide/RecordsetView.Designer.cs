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
      this.SuspendLayout();
      // 
      // toolStripSeparator1
      // 
      this.toolStripSeparator1.Name = "toolStripSeparator1";
      this.toolStripSeparator1.Size = new System.Drawing.Size(149, 6);
      // 
      // RecordsetView
      // 
      this.ClientSize = new System.Drawing.Size(811, 375);
      this.Name = "RecordsetView";
      this.Text = "RecordsetView";
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
  }
}