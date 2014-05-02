using System;
using System.Collections.Generic;
using System.Text;
using Aga.Controls.Tree;
using MySQL.Grt;

namespace MySQL.Grt
{
	public class GrtTreeNode : Node
	{
		public GrtTreeNode(string caption, NodeId nodeId, GrtTreeNode parent, GrtTreeModel model)
			: base(caption)
		{
			NodeId = nodeId;
			Parent = parent;
			Model = model;
		}

		private GrtTreeModel model;
		public GrtTreeModel Model
		{
			get { return model; }
			set { model = value; }
		}

		private NodeId nodeId = null;
		public NodeId NodeId
		{
			get { return nodeId; }
			set { nodeId = value; }
		}
	}
}
