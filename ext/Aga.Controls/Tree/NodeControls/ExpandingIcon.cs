using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.Threading;
using System.Windows.Forms;

namespace Aga.Controls.Tree.NodeControls
{
	/// <summary>
	/// Displays an animated icon for those nodes, who are in expanding state. 
	/// Parent TreeView must have AsyncExpanding property set to true.
	/// </summary>
	public class ExpandingIcon: NodeControl
	{
		private static GifDecoder _gif = ResourceHelper.LoadingIcon;
		private static int _index = 0;
		private static Thread _animatingThread;

		public override Size MeasureSize(TreeNodeAdv node, DrawContext context)
		{
			return ResourceHelper.LoadingIcon.FrameSize;
		}

		protected override void OnIsVisibleValueNeeded(NodeControlValueEventArgs args)
		{
			args.Value = args.Node.IsExpandingNow;
			base.OnIsVisibleValueNeeded(args);
		}

		public override void Draw(TreeNodeAdv node, DrawContext context)
		{
			Rectangle rect = GetBounds(node, context);
			Image img = _gif.GetFrame(_index).Image;
			context.Graphics.DrawImage(img, rect.Location);
		}

		public static void Start()
		{
			_index = 0;
			if (_animatingThread == null)
			{
				_animatingThread = new Thread(new ThreadStart(IterateIcons));
				_animatingThread.IsBackground = true;
				_animatingThread.Priority = ThreadPriority.Lowest;
				_animatingThread.Start();
			}
		}

		private static void IterateIcons()
		{
			while (true)
			{
				if (_index < _gif.FrameCount - 1)
					_index++;
				else
					_index = 0;

				if (IconChanged != null)
					IconChanged(null, EventArgs.Empty);

				int delay = _gif.GetFrame(_index).Delay;
				Thread.Sleep(delay);
			}
		}

		public static event EventHandler IconChanged;
	}
}
