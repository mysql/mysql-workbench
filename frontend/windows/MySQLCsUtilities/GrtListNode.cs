/* 
 * Copyright (c) 2009, 2012, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

using Aga.Controls.Tree;

namespace MySQL.Grt
{
	public class GrtListNode : Node
	{
		public GrtListNode(string caption, NodeId nodeId, GrtTreeNode parent, GrtListModel model)
			: base(caption)
		{
			NodeId = nodeId;
			Parent = parent;
			Model = model;
		}

		private GrtListModel model;
		public GrtListModel Model
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

    public bool IsValid
    {
      get { return nodeId.is_valid();  }
    }
	}
}
