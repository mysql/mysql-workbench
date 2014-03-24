using System;
using System.Collections.Generic;
using System.Text;
using MySQL.Workbench;
using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace MySQL.GUI.Workbench
{

	/// <summary>
	/// A tree model class for option-settings implementing ITreeModel from the TreeViewAdv control
	/// </summary>
	public class OptionsTreeModel : ITreeModel
	{
		/// <summary>
		/// The tree view control this model is used for
		/// </summary>
		protected TreeViewAdv treeView;

		/// <summary>
		/// The top node if any, otherwise null
		/// </summary>
		protected Node topNode = null;

		protected NodeTextBox keyNodeControl = null;
		protected NodeTextBox valueNodeControl = null;

    protected Dictionary<String, String> values = new Dictionary<string, string>();
    protected String modelId = "";

		/// <summary>
		/// Hide the default constructor
		/// </summary>
		private OptionsTreeModel()
		{
		}

		protected WbContext wbContext;
		protected List<String> explicitOptions;

		/// <summary>
		/// The constructor that has to be overwritten in the derived model classes
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model is used for</param>
		protected OptionsTreeModel(TreeViewAdv TreeView)
			: this()
		{
			treeView = TreeView;
		}

		/// <summary>
		/// The constructor that has to be overwritten in the derived model classes
		/// </summary>
		/// <param name="TreeView">The TreeViewAdv control this model is used for</param>
		/// <param name="explicitOptions">String List of Options handled explicitly (not in TreeView)</param>
		/// <param name="wbContext">Workbench-Context-Object for accessing options</param>
		public OptionsTreeModel(TreeViewAdv TreeView, List<String> ExplicitOptions, WbContext WbContext, String ModelId, NodeTextBox KeyNodeControl, NodeTextBox ValueNodeControl)
			: this(TreeView)
		{
			wbContext = WbContext;
      modelId = ModelId;
			explicitOptions = ExplicitOptions;

			keyNodeControl = KeyNodeControl;
			valueNodeControl = ValueNodeControl;

			keyNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(keyNodeControl_ValueNeeded);
			valueNodeControl.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(valueNodeControl_ValueNeeded);
			valueNodeControl.ValuePushed += new EventHandler<NodeControlValueEventArgs>(valueNodeControl_ValuePushed);

      FetchValues();

			// Ensure that the VirtualMode is enabled
			// nodeStateIcon.VirtualMode = true;

			// Assign virtual value events for displaying and processing the edited value content
			// nodeStateIcon.ValueNeeded += new EventHandler<NodeControlValueEventArgs>(StateIconNeeded);
		}


    void FetchValues()
    {
      values.Clear();

      List<String> keylist = wbContext.get_wb_options_keys(modelId);

      foreach (string key in keylist)
      {
        if (explicitOptions.IndexOf(key) < 0 && key[0] != '@')
        {
          String value = wbContext.get_wb_options_value(modelId, key);
          values[key] = value;
        }
      }
    }

    public void StoreValues()
    {
      foreach (String key in values.Keys)
      {
        wbContext.set_wb_options_value(modelId, key, values[key]);
      }
    }

		void valueNodeControl_ValuePushed(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null)
			{
				// update if changed
				if (String.Compare(values[e.Node.ToString()], e.Value.ToString()) != 0)
					{ values[e.Node.ToString()]= e.Value.ToString(); }
			}
		}

		void valueNodeControl_ValueNeeded(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null)
			{
				e.Value = values[e.Node.ToString()];
			}
		}

		void keyNodeControl_ValueNeeded(object sender, NodeControlValueEventArgs e)
		{
			if (e.Node != null)
			{
				e.Value = e.Node.Tag;
			}
		}

		/// <summary>
		/// Virtual function that needs to be overwritten in derived model classes. 
		/// Has to return a list of child nodes for the given path
		/// </summary>
		/// <param name="treePath">The path of the parent node</param>
		/// <returns>The list of child nodes for the given parent path node</returns>
		public System.Collections.IEnumerable GetChildren(TreePath treePath)
		{
			List<Node> items = new List<Node>();

			foreach (string key in values.Keys)
			{
				Node node;
				//string keyname;
				//keyname = wbContext.get_wb_options_value(key);

				node = new Node(key);

				items.Add(node);
			}

			return items;
		}

		/// <summary>
		/// Used to specify whether the given node can be expanded
		/// </summary>
		/// <param name="treePath">The path of the node</param>
		/// <returns>False if the node can be expanded, true if the node is a leaf</returns>
		public bool IsLeaf(TreePath treePath)
		{
      if (treePath.FullPath.Length > 0)
        return true;
      return false;
		}


		#region Events

		public event EventHandler<TreeModelEventArgs> NodesChanged;
		internal void OnNodesChanged(TreeModelEventArgs args)
		{
			if (NodesChanged != null)
				NodesChanged(this, args);
		}

		public event EventHandler<TreePathEventArgs> StructureChanged;
		public void OnStructureChanged(TreePathEventArgs args)
		{
			if (StructureChanged != null)
				StructureChanged(this, args);
		}

		public event EventHandler<TreeModelEventArgs> NodesInserted;
		internal void OnNodeInserted(Node parent, int index, Node node)
		{
			if (NodesInserted != null)
			{
				TreeModelEventArgs args = new TreeModelEventArgs(GetPath(parent), new int[] { index }, new object[] { node });
				NodesInserted(this, args);
			}
		}

		public event EventHandler<TreeModelEventArgs> NodesRemoved;
		internal void OnNodeRemoved(Node parent, int index, Node node)
		{
			if (NodesRemoved != null)
			{
				TreeModelEventArgs args = new TreeModelEventArgs(GetPath(parent), new int[] { index }, new object[] { node });
				NodesRemoved(this, args);
			}
		}
		#endregion

		/// <summary>
		/// Returns the path of the given node
		/// </summary>
		/// <param name="node">Node of interest</param>
		/// <returns>The path to the given node</returns>
		public TreePath GetPath(Node node)
		{
			if (node == topNode)
				return TreePath.Empty;
			else
			{
				Stack<object> stack = new Stack<object>();
				while (node != topNode)
				{
					stack.Push(node);
					node = node.Parent;
				}
				return new TreePath(stack.ToArray());
			}
		}

		/// <summary>
		/// Refreshes the tree
		/// </summary>
		public void RefreshModel()
		{
			OnStructureChanged(new TreePathEventArgs());
		}


		/// <summary>
		/// Detaches the events handlers from the tree
		/// </summary>
		public void DetachEvents()
		{
			// remove virtual value events
			valueNodeControl.ValueNeeded -= valueNodeControl_ValueNeeded;
			valueNodeControl.ValuePushed -= valueNodeControl_ValuePushed;

		}
	}
}

