namespace MySQL.Utilities
{
  partial class MySQLAdvTextBox
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
      changeTimer.Dispose();
      base.Dispose(disposing);
    }

    #region Component Designer generated code

    /// <summary> 
    /// Required method for Designer support - do not modify 
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.textBox = new System.Windows.Forms.TextBox();
      this.clearSearchPictureBox = new System.Windows.Forms.PictureBox();
      this.searchPictureBox = new System.Windows.Forms.PictureBox();
      this.docActionPictureBox = new System.Windows.Forms.PictureBox();
      ((System.ComponentModel.ISupportInitialize)(this.clearSearchPictureBox)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.searchPictureBox)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.docActionPictureBox)).BeginInit();
      this.SuspendLayout();
      // 
      // textBox
      // 
      this.textBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.textBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.textBox.Location = new System.Drawing.Point(17, 1);
      this.textBox.Name = "textBox";
      this.textBox.Size = new System.Drawing.Size(170, 13);
      this.textBox.TabIndex = 0;
      this.textBox.TextChanged += new System.EventHandler(this.textBox_TextChanged);
      this.textBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBox_KeyDown);
      // 
      // clearSearchPictureBox
      // 
      this.clearSearchPictureBox.Cursor = System.Windows.Forms.Cursors.Hand;
      this.clearSearchPictureBox.Dock = System.Windows.Forms.DockStyle.Right;
      this.clearSearchPictureBox.Image = global::MySQL.Controls.Properties.Resources.search_clear;
      this.clearSearchPictureBox.Location = new System.Drawing.Point(203, 1);
      this.clearSearchPictureBox.Name = "clearSearchPictureBox";
      this.clearSearchPictureBox.Size = new System.Drawing.Size(16, 17);
      this.clearSearchPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
      this.clearSearchPictureBox.TabIndex = 2;
      this.clearSearchPictureBox.TabStop = false;
      this.clearSearchPictureBox.Visible = false;
      this.clearSearchPictureBox.Click += new System.EventHandler(this.clearSearchPictureBox_Click);
      // 
      // searchPictureBox
      // 
      this.searchPictureBox.Dock = System.Windows.Forms.DockStyle.Left;
      this.searchPictureBox.Image = global::MySQL.Controls.Properties.Resources.search_icon;
      this.searchPictureBox.Location = new System.Drawing.Point(1, 1);
      this.searchPictureBox.Name = "searchPictureBox";
      this.searchPictureBox.Size = new System.Drawing.Size(16, 17);
      this.searchPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
      this.searchPictureBox.TabIndex = 1;
      this.searchPictureBox.TabStop = false;
      this.searchPictureBox.Visible = false;
      // 
      // docActionPictureBox
      // 
      this.docActionPictureBox.Cursor = System.Windows.Forms.Cursors.Hand;
      this.docActionPictureBox.Dock = System.Windows.Forms.DockStyle.Right;
      this.docActionPictureBox.Image = global::MySQL.Controls.Properties.Resources.search_doc_action;
      this.docActionPictureBox.Location = new System.Drawing.Point(187, 1);
      this.docActionPictureBox.Name = "docActionPictureBox";
      this.docActionPictureBox.Size = new System.Drawing.Size(16, 17);
      this.docActionPictureBox.SizeMode = System.Windows.Forms.PictureBoxSizeMode.CenterImage;
      this.docActionPictureBox.TabIndex = 3;
      this.docActionPictureBox.TabStop = false;
      this.docActionPictureBox.Visible = false;
      this.docActionPictureBox.Click += new System.EventHandler(this.docActionPictureBox_Click);
      // 
      // MySQLAdvTextBox
      // 
      this.BackColor = System.Drawing.SystemColors.Window;
      this.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
      this.Controls.Add(this.textBox);
      this.Controls.Add(this.docActionPictureBox);
      this.Controls.Add(this.clearSearchPictureBox);
      this.Controls.Add(this.searchPictureBox);
      this.Name = "MySQLAdvTextBox";
      this.Padding = new System.Windows.Forms.Padding(1);
      this.Size = new System.Drawing.Size(220, 19);
      this.Enter += new System.EventHandler(this.MySQLAdvTextBox_Enter);
      ((System.ComponentModel.ISupportInitialize)(this.clearSearchPictureBox)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.searchPictureBox)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.docActionPictureBox)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.TextBox textBox;
    private System.Windows.Forms.PictureBox searchPictureBox;
    private System.Windows.Forms.PictureBox clearSearchPictureBox;
    private System.Windows.Forms.PictureBox docActionPictureBox;
  }
}
