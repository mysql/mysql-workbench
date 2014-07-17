using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace Aga.Controls.Tree
{
	public class TreeColumnEventArgs: EventArgs
	{
		private TreeColumn _column;

		public TreeColumn Column
		{
			get { return _column; }
		}

    public MouseButtons Button { get; set; }  // ml: added _button.

		public TreeColumnEventArgs(TreeColumn column, MouseButtons button)
		{
			_column = column;
      Button = button;
		}
	}
}
