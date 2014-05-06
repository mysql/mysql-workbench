using System;

// by mikel
namespace Aga.Controls.Tree
{
	public class TreeViewAdvDrawRowEventArgs : EventArgs
	{
    public DrawContext Context { get; internal set; }

    public TreeNodeAdv Node { get; internal set; }

    public TreeViewAdvDrawRowEventArgs(TreeNodeAdv node, DrawContext context)
		{
			Node = node;
      Context = context;
		}
	}
}
