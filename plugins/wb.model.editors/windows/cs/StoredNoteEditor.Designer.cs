namespace MySQL.GUI.Workbench.Plugins
{
  partial class StoredNoteEditor
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
      this.panel1 = new System.Windows.Forms.Panel();
      this.panel2 = new System.Windows.Forms.Panel();
      this.discardButton = new System.Windows.Forms.Button();
      this.applyButton = new System.Windows.Forms.Button();
      this.content = new System.Windows.Forms.Panel();
      this.panel1.SuspendLayout();
      this.panel2.SuspendLayout();
      this.SuspendLayout();
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.panel2);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
      this.panel1.Location = new System.Drawing.Point(0, 358);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(622, 32);
      this.panel1.TabIndex = 0;
      // 
      // panel2
      // 
      this.panel2.Controls.Add(this.discardButton);
      this.panel2.Controls.Add(this.applyButton);
      this.panel2.Dock = System.Windows.Forms.DockStyle.Right;
      this.panel2.Location = new System.Drawing.Point(391, 0);
      this.panel2.Name = "panel2";
      this.panel2.Size = new System.Drawing.Size(231, 32);
      this.panel2.TabIndex = 0;
      // 
      // discardButton
      // 
      this.discardButton.Enabled = false;
      this.discardButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
      this.discardButton.Location = new System.Drawing.Point(116, 3);
      this.discardButton.Name = "discardButton";
      this.discardButton.Size = new System.Drawing.Size(107, 23);
      this.discardButton.TabIndex = 1;
      this.discardButton.Text = "Discard Changes";
      this.discardButton.UseVisualStyleBackColor = true;
      this.discardButton.Click += new System.EventHandler(this.discardButton_Click);
      // 
      // applyButton
      // 
      this.applyButton.Enabled = false;
      this.applyButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
      this.applyButton.Location = new System.Drawing.Point(3, 3);
      this.applyButton.Name = "applyButton";
      this.applyButton.Size = new System.Drawing.Size(107, 23);
      this.applyButton.TabIndex = 0;
      this.applyButton.Text = "Apply Changes";
      this.applyButton.UseVisualStyleBackColor = true;
      this.applyButton.Click += new System.EventHandler(this.applyButton_Click);
      // 
      // content
      // 
      this.content.BackColor = System.Drawing.SystemColors.ControlDark;
      this.content.Dock = System.Windows.Forms.DockStyle.Fill;
      this.content.Location = new System.Drawing.Point(0, 0);
      this.content.Name = "content";
      this.content.Size = new System.Drawing.Size(622, 358);
      this.content.TabIndex = 1;
      // 
      // StoredNoteEditor
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(622, 390);
      this.Controls.Add(this.content);
      this.Controls.Add(this.panel1);
      this.Name = "StoredNoteEditor";
      this.TabText = "StoredNoteEditor";
      this.Text = "StoredNoteEditor";
      this.panel1.ResumeLayout(false);
      this.panel2.ResumeLayout(false);
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Panel panel1;
    private System.Windows.Forms.Panel panel2;
    private System.Windows.Forms.Button discardButton;
    private System.Windows.Forms.Button applyButton;
    private System.Windows.Forms.Panel content;
  }
}