using System.Windows.Forms;

using MySQL.Controls;

namespace MySQL.GUI.Workbench
{
	public partial class TextEditorForm : TabDocument
	{
		public TextEditorForm()
		{
			InitializeComponent();
		}

		public RichTextBox RichTextBox
		{
			get { return richTextBox; }
		}

    private void TextEditorForm_KeyDown(object sender, KeyEventArgs e)
    {
      switch (e.KeyCode)
      {
        case Keys.Escape:
          DialogResult = DialogResult.Cancel;
          Close();
          e.Handled = true;
          break;
        case Keys.Enter:
          DialogResult = DialogResult.OK;
          Close();
          e.Handled = true;
          break;
      }
    }
	}
}