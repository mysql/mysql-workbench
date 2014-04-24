namespace MySQL.GUI.Workbench
{
  partial class ExceptionDialog
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
      this.components = new System.ComponentModel.Container();
      System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ExceptionDialog));
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      this.copyInfoButton = new System.Windows.Forms.Button();
      this.reportBugButton = new System.Windows.Forms.Button();
      this.titleLabel = new System.Windows.Forms.Label();
      this.messageLabel = new System.Windows.Forms.Label();
      this.richTextBox1 = new System.Windows.Forms.RichTextBox();
      this.closeButton = new System.Windows.Forms.Button();
      this.contextMenuStrip1 = new System.Windows.Forms.ContextMenuStrip(this.components);
      this.copyStackTraceToClipboardToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.contextMenuStrip1.SuspendLayout();
      this.SuspendLayout();
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = global::MySQL.GUI.Workbench.Properties.Resources.message_wb_bug;
      this.pictureBox1.Location = new System.Drawing.Point(12, 12);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(151, 156);
      this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.AutoSize;
      this.pictureBox1.TabIndex = 0;
      this.pictureBox1.TabStop = false;
      // 
      // copyInfoButton
      // 
      this.copyInfoButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.copyInfoButton.Location = new System.Drawing.Point(276, 206);
      this.copyInfoButton.Name = "copyInfoButton";
      this.copyInfoButton.Size = new System.Drawing.Size(172, 23);
      this.copyInfoButton.TabIndex = 1;
      this.copyInfoButton.Text = "Copy Bug Info to Clipboard";
      this.copyInfoButton.UseVisualStyleBackColor = true;
      this.copyInfoButton.Visible = false;
      this.copyInfoButton.Click += new System.EventHandler(this.copyInfoButton_Click);
      // 
      // reportBugButton
      // 
      this.reportBugButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.reportBugButton.Location = new System.Drawing.Point(454, 206);
      this.reportBugButton.Name = "reportBugButton";
      this.reportBugButton.Size = new System.Drawing.Size(97, 23);
      this.reportBugButton.TabIndex = 2;
      this.reportBugButton.Text = "Report Bug";
      this.reportBugButton.UseVisualStyleBackColor = true;
      this.reportBugButton.Click += new System.EventHandler(this.reportBugButton_Click);
      // 
      // titleLabel
      // 
      this.titleLabel.AutoSize = true;
      this.titleLabel.Font = new System.Drawing.Font("Tahoma", 10F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.titleLabel.ForeColor = System.Drawing.Color.Black;
      this.titleLabel.Location = new System.Drawing.Point(169, 12);
      this.titleLabel.Name = "titleLabel";
      this.titleLabel.Size = new System.Drawing.Size(332, 17);
      this.titleLabel.TabIndex = 3;
      this.titleLabel.Text = "MySQL Workbench has encountered a problem";
      // 
      // messageLabel
      // 
      this.messageLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.messageLabel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.messageLabel.ForeColor = System.Drawing.Color.Black;
      this.messageLabel.Location = new System.Drawing.Point(169, 39);
      this.messageLabel.Name = "messageLabel";
      this.messageLabel.Size = new System.Drawing.Size(463, 46);
      this.messageLabel.TabIndex = 4;
      this.messageLabel.Text = "label2";
      // 
      // richTextBox1
      // 
      this.richTextBox1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                  | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.richTextBox1.BackColor = System.Drawing.SystemColors.ButtonFace;
      this.richTextBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
      this.richTextBox1.Location = new System.Drawing.Point(172, 77);
      this.richTextBox1.Name = "richTextBox1";
      this.richTextBox1.ReadOnly = true;
      this.richTextBox1.Size = new System.Drawing.Size(460, 123);
      this.richTextBox1.TabIndex = 5;
      this.richTextBox1.Text = resources.GetString("richTextBox1.Text");
      // 
      // closeButton
      // 
      this.closeButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.closeButton.DialogResult = System.Windows.Forms.DialogResult.Abort;
      this.closeButton.Location = new System.Drawing.Point(557, 206);
      this.closeButton.Name = "closeButton";
      this.closeButton.Size = new System.Drawing.Size(75, 23);
      this.closeButton.TabIndex = 6;
      this.closeButton.Text = "Close";
      this.closeButton.UseVisualStyleBackColor = true;
      // 
      // contextMenuStrip1
      // 
      this.contextMenuStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.copyStackTraceToClipboardToolStripMenuItem});
      this.contextMenuStrip1.Name = "contextMenuStrip1";
      this.contextMenuStrip1.Size = new System.Drawing.Size(229, 48);
      // 
      // copyStackTraceToClipboardToolStripMenuItem
      // 
      this.copyStackTraceToClipboardToolStripMenuItem.Name = "copyStackTraceToClipboardToolStripMenuItem";
      this.copyStackTraceToClipboardToolStripMenuItem.Size = new System.Drawing.Size(228, 22);
      this.copyStackTraceToClipboardToolStripMenuItem.Text = "Copy stack trace to clipboard";
      this.copyStackTraceToClipboardToolStripMenuItem.Click += new System.EventHandler(this.copyStackTraceToClipboardToolStripMenuItem_Click);
      // 
      // ExceptionDialog
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(644, 241);
      this.ContextMenuStrip = this.contextMenuStrip1;
      this.Controls.Add(this.closeButton);
      this.Controls.Add(this.richTextBox1);
      this.Controls.Add(this.messageLabel);
      this.Controls.Add(this.titleLabel);
      this.Controls.Add(this.reportBugButton);
      this.Controls.Add(this.copyInfoButton);
      this.Controls.Add(this.pictureBox1);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
      this.MinimumSize = new System.Drawing.Size(650, 38);
      this.Name = "ExceptionDialog";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
      this.Text = "MySQL Workbench Unexpected Error";
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.contextMenuStrip1.ResumeLayout(false);
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.PictureBox pictureBox1;
    private System.Windows.Forms.Button copyInfoButton;
    private System.Windows.Forms.Button reportBugButton;
    private System.Windows.Forms.Label titleLabel;
    private System.Windows.Forms.Label messageLabel;
    private System.Windows.Forms.RichTextBox richTextBox1;
    private System.Windows.Forms.Button closeButton;
    private System.Windows.Forms.ContextMenuStrip contextMenuStrip1;
    private System.Windows.Forms.ToolStripMenuItem copyStackTraceToClipboardToolStripMenuItem;
  }
}