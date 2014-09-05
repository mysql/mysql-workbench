using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;
using System.Drawing;

namespace Aga.Controls.Tree
{
	internal class ClickColumnState : ColumnState
	{
		private Point _location;

		public ClickColumnState(TreeViewAdv tree, TreeColumn column, Point location)
			: base(tree, column)
		{
			_location = location;
		}

		public override void KeyDown(KeyEventArgs args)
		{
		}

		public override void MouseDown(TreeNodeAdvMouseEventArgs args)
		{
            // Handling of the right click does not need view update
            if (args.Button != MouseButtons.Right)
            {
                Tree.ChangeInput();
                Tree.UpdateView();
            }

            Tree.OnColumnClicked(Column, args.Button);
        }

		public override bool MouseMove(MouseEventArgs args)
		{
			if (TreeViewAdv.Dist(_location, args.Location) > TreeViewAdv.ItemDragSensivity
				&& Tree.AllowColumnReorder)
			{
                Tree.Input = new ReorderColumnState(Tree, Column, args.Location);
                Tree.UpdateView();
			}
			return true;
		}

		public override void MouseUp(TreeNodeAdvMouseEventArgs args)
		{
		}
	}
}
