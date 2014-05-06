namespace MySQL.Utilities
{
  partial class StringInputForm
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
      this.cancelButton = new System.Windows.Forms.Button();
      this.okButton = new System.Windows.Forms.Button();
      this.descLabel = new System.Windows.Forms.Label();
      this.inputTextBox = new System.Windows.Forms.TextBox();
      this.promptLabel = new System.Windows.Forms.Label();
      this.pictureBox1 = new System.Windows.Forms.PictureBox();
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
      this.SuspendLayout();
      // 
      // cancelButton
      // 
      this.cancelButton.DialogResult = System.Windows.Forms.DialogResult.Cancel;
      this.cancelButton.Location = new System.Drawing.Point(368, 96);
      this.cancelButton.Name = "cancelButton";
      this.cancelButton.Size = new System.Drawing.Size(75, 23);
      this.cancelButton.TabIndex = 1;
      this.cancelButton.Text = "Cancel";
      this.cancelButton.UseVisualStyleBackColor = true;
      // 
      // okButton
      // 
      this.okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
      this.okButton.Location = new System.Drawing.Point(287, 96);
      this.okButton.Name = "okButton";
      this.okButton.Size = new System.Drawing.Size(75, 23);
      this.okButton.TabIndex = 0;
      this.okButton.Text = "OK";
      this.okButton.UseVisualStyleBackColor = true;
      // 
      // descLabel
      // 
      this.descLabel.Font = new System.Drawing.Font("Microsoft Sans Serif", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.descLabel.Location = new System.Drawing.Point(12, 9);
      this.descLabel.Name = "descLabel";
      this.descLabel.Size = new System.Drawing.Size(431, 33);
      this.descLabel.TabIndex = 7;
      this.descLabel.Text = "Please enter the requested information and press [OK] to proceed.";
      // 
      // inputTextBox
      // 
      this.inputTextBox.Location = new System.Drawing.Point(169, 53);
      this.inputTextBox.Name = "inputTextBox";
      this.inputTextBox.Size = new System.Drawing.Size(274, 20);
      this.inputTextBox.TabIndex = 0;
      // 
      // promptLabel
      // 
      this.promptLabel.AutoSize = true;
      this.promptLabel.Location = new System.Drawing.Point(130, 53);
      this.promptLabel.Name = "promptLabel";
      this.promptLabel.Padding = new System.Windows.Forms.Padding(0, 5, 0, 0);
      this.promptLabel.Size = new System.Drawing.Size(33, 18);
      this.promptLabel.TabIndex = 3;
      this.promptLabel.Text = "Data:";
      this.promptLabel.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
      // 
      // pictureBox1
      // 
      this.pictureBox1.Image = global::MySQL.Controls.Properties.Resources.dialog_input;
      this.pictureBox1.Location = new System.Drawing.Point(12, 45);
      this.pictureBox1.Name = "pictureBox1";
      this.pictureBox1.Size = new System.Drawing.Size(48, 39);
      this.pictureBox1.TabIndex = 1;
      this.pictureBox1.TabStop = false;
      // 
      // StringInputForm
      // 
      this.AcceptButton = this.okButton;
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.AutoSize = true;
      this.CancelButton = this.cancelButton;
      this.ClientSize = new System.Drawing.Size(455, 125);
      this.Controls.Add(this.descLabel);
      this.Controls.Add(this.inputTextBox);
      this.Controls.Add(this.pictureBox1);
      this.Controls.Add(this.cancelButton);
      this.Controls.Add(this.promptLabel);
      this.Controls.Add(this.okButton);
      this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
      this.Name = "StringInputForm";
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
      this.Text = "Text Input";
      ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
      this.ResumeLayout(false);
      this.PerformLayout();

    }

    #endregion

    private System.Windows.Forms.Button cancelButton;
    private System.Windows.Forms.Button okButton;
    private System.Windows.Forms.TextBox inputTextBox;
    private System.Windows.Forms.Label promptLabel;
    private System.Windows.Forms.Label descLabel;
    private System.Windows.Forms.PictureBox pictureBox1;
  }
}