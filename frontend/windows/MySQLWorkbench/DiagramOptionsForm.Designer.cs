namespace MySQL.GUI.Workbench
{
  partial class DiagramOptionsForm
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
      this.button1 = new System.Windows.Forms.Button();
      this.button2 = new System.Windows.Forms.Button();
      this.warningLabel = new System.Windows.Forms.Label();
      this.contentPanel = new MySQL.Controls.DrawablePanel();
      this.heightUpDown = new System.Windows.Forms.NumericUpDown();
      this.label2 = new System.Windows.Forms.Label();
      this.widthUpDown = new System.Windows.Forms.NumericUpDown();
      this.label1 = new System.Windows.Forms.Label();
      this.label3 = new System.Windows.Forms.Label();
      this.label4 = new System.Windows.Forms.Label();
      this.label5 = new System.Windows.Forms.Label();
      this.diagramNameEdit = new System.Windows.Forms.TextBox();
      ((System.ComponentModel.ISupportInitialize)(this.heightUpDown)).BeginInit();
      ((System.ComponentModel.ISupportInitialize)(this.widthUpDown)).BeginInit();
      this.SuspendLayout();
      // 
      // button1
      // 
      this.button1.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.button1.DialogResult = System.Windows.Forms.DialogResult.Cancel;
      this.button1.Location = new System.Drawing.Point(324, 356);
      this.button1.Name = "button1";
      this.button1.Size = new System.Drawing.Size(75, 23);
      this.button1.TabIndex = 0;
      this.button1.Text = "&Cancel";
      this.button1.UseVisualStyleBackColor = true;
      // 
      // button2
      // 
      this.button2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.button2.DialogResult = System.Windows.Forms.DialogResult.OK;
      this.button2.Location = new System.Drawing.Point(243, 356);
      this.button2.Name = "button2";
      this.button2.Size = new System.Drawing.Size(75, 23);
      this.button2.TabIndex = 1;
      this.button2.Text = "&OK";
      this.button2.UseVisualStyleBackColor = true;
      this.button2.Click += new System.EventHandler(this.button2_Click);
      // 
      // warningLabel
      // 
      this.warningLabel.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
      this.warningLabel.AutoSize = true;
      this.warningLabel.Location = new System.Drawing.Point(12, 356);
      this.warningLabel.Name = "warningLabel";
      this.warningLabel.Size = new System.Drawing.Size(0, 13);
      this.warningLabel.TabIndex = 2;
      // 
      // contentPanel
      // 
      this.contentPanel.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                  | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.contentPanel.CustomBackground = false;
      this.contentPanel.Location = new System.Drawing.Point(12, 32);
      this.contentPanel.Name = "contentPanel";
      this.contentPanel.Size = new System.Drawing.Size(387, 278);
      this.contentPanel.TabIndex = 3;
      this.contentPanel.SizeChanged += new System.EventHandler(this.contentPanel_SizeChanged);
      this.contentPanel.Paint += new System.Windows.Forms.PaintEventHandler(this.contentPanel_Paint);
      this.contentPanel.MouseDown += new System.Windows.Forms.MouseEventHandler(this.contentPanel_MouseDown);
      this.contentPanel.MouseMove += new System.Windows.Forms.MouseEventHandler(this.contentPanel_MouseMove);
      this.contentPanel.MouseUp += new System.Windows.Forms.MouseEventHandler(this.contentPanel_MouseUp);
      // 
      // heightUpDown
      // 
      this.heightUpDown.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
      this.heightUpDown.Location = new System.Drawing.Point(266, 316);
      this.heightUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
      this.heightUpDown.Name = "heightUpDown";
      this.heightUpDown.Size = new System.Drawing.Size(52, 20);
      this.heightUpDown.TabIndex = 11;
      this.heightUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
      this.heightUpDown.ValueChanged += new System.EventHandler(this.heightUpDown_ValueChanged);
      // 
      // label2
      // 
      this.label2.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
      this.label2.AutoSize = true;
      this.label2.Location = new System.Drawing.Point(219, 318);
      this.label2.Name = "label2";
      this.label2.Size = new System.Drawing.Size(41, 13);
      this.label2.TabIndex = 10;
      this.label2.Text = "Height:";
      // 
      // widthUpDown
      // 
      this.widthUpDown.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
      this.widthUpDown.Location = new System.Drawing.Point(90, 316);
      this.widthUpDown.Minimum = new decimal(new int[] {
            1,
            0,
            0,
            0});
      this.widthUpDown.Name = "widthUpDown";
      this.widthUpDown.Size = new System.Drawing.Size(52, 20);
      this.widthUpDown.TabIndex = 9;
      this.widthUpDown.Value = new decimal(new int[] {
            1,
            0,
            0,
            0});
      this.widthUpDown.ValueChanged += new System.EventHandler(this.widthUpDown_ValueChanged);
      // 
      // label1
      // 
      this.label1.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
      this.label1.AutoSize = true;
      this.label1.Location = new System.Drawing.Point(46, 318);
      this.label1.Name = "label1";
      this.label1.Size = new System.Drawing.Size(38, 13);
      this.label1.TabIndex = 8;
      this.label1.Text = "Width:";
      // 
      // label3
      // 
      this.label3.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
      this.label3.AutoSize = true;
      this.label3.Location = new System.Drawing.Point(148, 318);
      this.label3.Name = "label3";
      this.label3.Size = new System.Drawing.Size(36, 13);
      this.label3.TabIndex = 12;
      this.label3.Text = "pages";
      // 
      // label4
      // 
      this.label4.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
      this.label4.AutoSize = true;
      this.label4.Location = new System.Drawing.Point(324, 318);
      this.label4.Name = "label4";
      this.label4.Size = new System.Drawing.Size(36, 13);
      this.label4.TabIndex = 12;
      this.label4.Text = "pages";
      // 
      // label5
      // 
      this.label5.AutoSize = true;
      this.label5.Location = new System.Drawing.Point(46, 9);
      this.label5.Name = "label5";
      this.label5.Size = new System.Drawing.Size(38, 13);
      this.label5.TabIndex = 13;
      this.label5.Text = "Name:";
      // 
      // diagramNameEdit
      // 
      this.diagramNameEdit.Location = new System.Drawing.Point(90, 6);
      this.diagramNameEdit.Name = "diagramNameEdit";
      this.diagramNameEdit.Size = new System.Drawing.Size(184, 20);
      this.diagramNameEdit.TabIndex = 14;
      this.diagramNameEdit.TextChanged += new System.EventHandler(this.diagramNameEdit_TextChanged);
      // 
      // DiagramOptionsForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(411, 391);
      this.ControlBox = false;
      this.Controls.Add(this.diagramNameEdit);
      this.Controls.Add(this.label5);
      this.Controls.Add(this.label4);
      this.Controls.Add(this.label3);
      this.Controls.Add(this.heightUpDown);
      this.Controls.Add(this.label2);
      this.Controls.Add(this.widthUpDown);
      this.Controls.Add(this.label1);
      this.Controls.Add(this.contentPanel);
      this.Controls.Add(this.warningLabel);
      this.Controls.Add(this.button2);
      this.Controls.Add(this.button1);
      this.Name = "DiagramOptionsForm";
      this.Text = "Diagram Properties";
      ((System.ComponentModel.ISupportInitialize)(this.heightUpDown)).EndInit();
      ((System.ComponentModel.ISupportInitialize)(this.widthUpDown)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Button button1;
    private System.Windows.Forms.Button button2;
    private System.Windows.Forms.Label warningLabel;
    private MySQL.Controls.DrawablePanel contentPanel;
    private System.Windows.Forms.NumericUpDown heightUpDown;
    private System.Windows.Forms.Label label2;
    private System.Windows.Forms.NumericUpDown widthUpDown;
    private System.Windows.Forms.Label label1;
    private System.Windows.Forms.Label label3;
    private System.Windows.Forms.Label label4;
    private System.Windows.Forms.Label label5;
    private System.Windows.Forms.TextBox diagramNameEdit;
  }
}