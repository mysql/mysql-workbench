namespace MySQL.Utilities
{
  partial class MySQLTaskStatusLabel
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

    #region Component Designer generated code

    /// <summary> 
    /// Required method for Designer support - do not modify 
    /// the contents of this method with the code editor.
    /// </summary>
    private void InitializeComponent()
    {
      this.taskPictureBox = new System.Windows.Forms.PictureBox();
      this.taskLabel = new System.Windows.Forms.Label();
      ((System.ComponentModel.ISupportInitialize)(this.taskPictureBox)).BeginInit();
      this.SuspendLayout();
      // 
      // taskPictureBox
      // 
      this.taskPictureBox.Location = new System.Drawing.Point(0, 0);
      this.taskPictureBox.Name = "taskPictureBox";
      this.taskPictureBox.Size = new System.Drawing.Size(13, 14);
      this.taskPictureBox.TabIndex = 0;
      this.taskPictureBox.TabStop = false;
      // 
      // taskLabel
      // 
      this.taskLabel.AutoSize = true;
      this.taskLabel.Location = new System.Drawing.Point(15, 1);
      this.taskLabel.Name = "taskLabel";
      this.taskLabel.Size = new System.Drawing.Size(81, 13);
      this.taskLabel.TabIndex = 1;
      this.taskLabel.Text = "Task to perform";
      // 
      // MySQLTaskStatusLabel
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.AutoSize = true;
      this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
      this.BackColor = System.Drawing.Color.Transparent;
      this.Controls.Add(this.taskLabel);
      this.Controls.Add(this.taskPictureBox);
      this.Name = "MySQLTaskStatusLabel";
      this.Size = new System.Drawing.Size(99, 17);
      ((System.ComponentModel.ISupportInitialize)(this.taskPictureBox)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.PictureBox taskPictureBox;
    private System.Windows.Forms.Label taskLabel;
  }
}
