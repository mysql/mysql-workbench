namespace MySQL.GUI.Workbench
{
	partial class TextEditorForm
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
      this.richTextBox = new System.Windows.Forms.RichTextBox();
      this.SuspendLayout();
      // 
      // richTextBox
      // 
      this.richTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
      this.richTextBox.Location = new System.Drawing.Point(0, 0);
      this.richTextBox.Name = "richTextBox";
      this.richTextBox.Size = new System.Drawing.Size(647, 347);
      this.richTextBox.TabIndex = 0;
      this.richTextBox.Text = "";
      // 
      // TextEditorForm
      // 
      this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
      this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
      this.ClientSize = new System.Drawing.Size(647, 347);
      this.Controls.Add(this.richTextBox);
      this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
      this.KeyPreview = true;
      this.Name = "TextEditorForm";
      this.ShowInTaskbar = false;
      this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
      this.TabText = "Text Editor";
      this.Text = "Text Editor";
      this.TopMost = true;
      this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.TextEditorForm_KeyDown);
      this.ResumeLayout(false);

		}

		#endregion

		private System.Windows.Forms.RichTextBox richTextBox;
	}
}