namespace MySQL.Grt.Db
{
  partial class ColumnFilterDialog
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
      this.filterExpressionLabel = new System.Windows.Forms.Label();
      this.filterExpressionTB = new System.Windows.Forms.TextBox();
      this.cancelButton = new System.Windows.Forms.Button();
      this.okButton = new System.Windows.Forms.Button();
      this.panel1 = new System.Windows.Forms.Panel();
      this.panel1.SuspendLayout();
      this.SuspendLayout();
      // 
      // filterExpressionLabel
      // 
      this.filterExpressionLabel.AutoSize = true;
      this.filterExpressionLabel.Location = new System.Drawing.Point(12, 9);
      this.filterExpressionLabel.Name = "filterExpressionLabel";
      this.filterExpressionLabel.Size = new System.Drawing.Size(177, 13);
      this.filterExpressionLabel.TabIndex = 0;
      this.filterExpressionLabel.Text = "Filter expression for column `NAME`:";
      // 
      // filterExpressionTB
      // 
      this.filterExpressionTB.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                  | System.Windows.Forms.AnchorStyles.Right)));
      this.filterExpressionTB.Location = new System.Drawing.Point(15, 25);
      this.filterExpressionTB.MinimumSize = new System.Drawing.Size(4, 21);
      this.filterExpressionTB.Multiline = true;
      this.filterExpressionTB.Name = "filterExpressionTB";
      this.filterExpressionTB.Size = new System.Drawing.Size(273, 21);
      this.filterExpressionTB.TabIndex = 1;
      // 
      // cancelButton
      // 
      this.cancelButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
      this.cancelButton.Location = new System.Drawing.Point(132, 54);
      this.cancelButton.Name = "cancelButton";
      this.cancelButton.Size = new System.Drawing.Size(75, 23);
      this.cancelButton.TabIndex = 2;
      this.cancelButton.Text = "Cancel";
      this.cancelButton.UseVisualStyleBackColor = true;
      // 
      // okButton
      // 
      this.okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
      this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
      this.okButton.Location = new System.Drawing.Point(213, 54);
      this.okButton.Name = "okButton";
      this.okButton.Size = new System.Drawing.Size(75, 23);
      this.okButton.TabIndex = 3;
      this.okButton.Text = "OK";
      this.okButton.UseVisualStyleBackColor = true;
      // 
      // panel1
      // 
      this.panel1.Controls.Add(this.filterExpressionLabel);
      this.panel1.Controls.Add(this.filterExpressionTB);
      this.panel1.Controls.Add(this.cancelButton);
      this.panel1.Controls.Add(this.okButton);
      this.panel1.Dock = System.Windows.Forms.DockStyle.Fill;
      this.panel1.Location = new System.Drawing.Point(0, 0);
      this.panel1.MinimumSize = new System.Drawing.Size(300, 89);
      this.panel1.Name = "panel1";
      this.panel1.Size = new System.Drawing.Size(300, 89);
      this.panel1.TabIndex = 4;
      // 
      // ColumnFilterDialog
      // 
      this.AcceptButton = this.okButton;
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.CancelButton = this.cancelButton;
      this.ClientSize = new System.Drawing.Size(300, 89);
      this.ControlBox = false;
      this.Controls.Add(this.panel1);
      this.MinimumSize = new System.Drawing.Size(316, 125);
      this.Name = "ColumnFilterDialog";
      this.StartPosition = System.Windows.Forms.FormStartPosition.Manual;
      this.Text = "Column Filter Dialog";
      this.Activated += new System.EventHandler(this.ColumnFilterDialog_Activated);
      this.panel1.ResumeLayout(false);
      this.panel1.PerformLayout();
      this.ResumeLayout(false);

    }

    #endregion

    private System.Windows.Forms.Button cancelButton;
    private System.Windows.Forms.Button okButton;
    private System.Windows.Forms.TextBox filterExpressionTB;
    private System.Windows.Forms.Label filterExpressionLabel;
    private System.Windows.Forms.Panel panel1;
  }
}