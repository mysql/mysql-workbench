/* 
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
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

using System;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

using Aga.Controls.Tree.NodeControls;

namespace Aga.Controls.Tree
{
  public partial class TreeViewAdv
  {
    protected override AccessibleObject CreateAccessibilityInstance()
    {
      return new TreeViewAdvAccessibleObject(this);
    }

    /// <summary>
    /// Represents the tree control itself.
    /// </summary>
    public class TreeViewAdvAccessibleObject : Control.ControlAccessibleObject
    {
      private TreeViewAdv treeView;

      public TreeViewAdvAccessibleObject(TreeViewAdv owner)
        : base(owner)
      {
        treeView = owner;
      }

      public override AccessibleRole Role
      {
        get { return AccessibleRole.Outline; }
      }

      public override string Name
      {
        get { return treeView.Name; }
      }

      /// <summary>
      /// Returns the node under the mouse pointer.
      /// </summary>
      /// <param name="x">X screen coordinate.</param>
      /// <param name="y">Y screen coordinate.</param>
      /// <returns>The node under the mouse pointer.</returns>
      public override AccessibleObject HitTest(int x, int y)
      {
        TreeNodeAdv node = treeView.GetNodeAt(treeView.PointToClient(new Point(x, y)));
        if (node != null)
          return new AdvNodeAccessibleObject(node, null, this);

        if (treeView.RectangleToScreen(treeView.ClientRectangle).Contains(x, y))
          return this;

        return null;
      }

      /// <summary>
      /// Returns a child node.
      /// </summary>
      /// <param name="index">The index of the child node.</param>
      /// <returns>The child node.</returns>
      public override AccessibleObject GetChild(int index)
      {
        return new AdvNodeAccessibleObject(treeView.Root.Nodes[index], null, this);
      }

      /// <summary>
      /// Returns the number of children.
      /// </summary>
      /// <returns>The number of children in the tree root.</returns>
      public override int GetChildCount()
      {
        return treeView.Root.Nodes.Count;
      }
    }

    /// <summary>
    /// Accessibility representation of a node in the treeview.
    /// </summary>
    public class AdvNodeAccessibleObject : AccessibleObject
    {
      TreeNodeAdv node;
      AdvNodeAccessibleObject parent;
      TreeViewAdvAccessibleObject owner;

      public AdvNodeAccessibleObject(TreeNodeAdv advNode, AdvNodeAccessibleObject parent,
        TreeViewAdvAccessibleObject owner)
        : base()
      {
        node = advNode;
        this.parent = parent;
        this.owner = owner;
      }

      /// <summary>
      /// The bounding rectangle in screen coordinates.
      /// </summary>
      public override Rectangle Bounds
      {
        get
        {
          if (!node.IsVisible)
            return Rectangle.Empty;

          Rectangle bounds = node.Tree.GetNodeBounds(node);
          Point p = node.Tree.ScrollPosition;

          int colHeaderY = node.Tree.UseColumns ? node.Tree.ColumnHeaderHeight : 0;

          bounds.Offset(-p.X, -p.Y * node.Tree.RowHeight + colHeaderY);
          return node.Tree.RectangleToScreen(bounds);
        }
      }

      /// <summary>
      /// The name of the default action.
      /// </summary>
      public override string DefaultAction
      {
        get { return node.IsExpanded ? "Collapse" : "Expand"; }
      }

      /// <summary>
      /// Performs the default action, which is either collapsing or expand the tree node
      /// </summary>
      public override void DoDefaultAction()
      {
        if (node.IsExpanded)
          node.Collapse();
        else node.Expand();
      }

      /// <summary>
      /// Either selects or focuses the tree node.
      /// </summary>
      /// <param name="flags">either select or focus.</param>
      public override void Select(AccessibleSelection flags)
      {
        if ((flags & AccessibleSelection.TakeSelection) > 0)
          node.Tree.SelectedNode = node;
        else
          if ((flags & AccessibleSelection.TakeFocus) > 0)
          {
            TreeNodeAdv parent = node.Parent;
            while (parent != null)
            {
              if (!parent.IsExpanded)
                parent.Expand();
              parent = parent.Parent;
            }

            node.Tree.ScrollTo(node);
            node.Tree.Focus();
          }
        }

      /// <summary>
      /// This does nothing because it is done by the tree.
      /// </summary>
      /// <param name="x"></param>
      /// <param name="y"></param>
      /// <returns></returns>
      public override AccessibleObject HitTest(int x, int y)
      {
        return null;
      }

      /// <summary>
      /// The name is the first node control value which returns a string.
      /// </summary>
      public override string Name
      {
        get
        {
          foreach (NodeControlInfo info in node.Tree.GetNodeControls(node))
          {
            BindableControl ctrl = info.Control as BindableControl;
            if (ctrl != null)
            {
              string val = ctrl.GetValue(node) as string;
              if (val != null)
                return val;
            }
          }
          return null;
        }
      }

      /// <summary>
      /// A number of state flags
      /// The Checked state uses the value of the first checkbox it finds.
      /// </summary>
      public override AccessibleStates State
      {
        get
        {
          AccessibleStates states = AccessibleStates.Selectable;

          if (node.IsExpanded)
            states |= AccessibleStates.Expanded;
          else
            states |= AccessibleStates.Collapsed;

          Rectangle treeRect = node.Tree.ClientRectangle;
          treeRect.Offset(node.Tree.PointToScreen(Point.Empty));

          if (!node.IsVisible || !treeRect.IntersectsWith(Bounds) || treeRect.IsEmpty)
            states |= AccessibleStates.Invisible;

          if (node.IsSelected)
            states |= AccessibleStates.Selected;

          foreach (NodeControlInfo info in node.Tree.GetNodeControls(node))
          {
            NodeCheckBox cb = info.Control as NodeCheckBox;
            if (cb != null)
            {
              if (((CheckState)cb.GetValue(node)) == CheckState.Checked)
                states |= AccessibleStates.Checked;
            }
          }
          return states;
        }
      }

      public override AccessibleRole Role
      {
        get { return AccessibleRole.OutlineItem; }
      }


      /// <summary>
      /// Returns both child nodes and table cells, if available.
      /// </summary>
      /// <param name="index"></param>
      /// <returns></returns>
      public override AccessibleObject GetChild(int index)
      {
        int ctrlCount = node.Tree.Columns.Count;

        if (index < ctrlCount)
          return new AdvNodeCellAccessibleObject(node, index, this);

        return new AdvNodeAccessibleObject(node.Nodes[index - ctrlCount], this, owner);
      }

      public override int GetChildCount()
      {
        return node.Nodes.Count + node.Tree.Columns.Count;
      }

      public override AccessibleObject Parent
      {
        get
        {
          if (node.Parent != node.Tree.Root)
            return parent != null ? parent : new AdvNodeAccessibleObject(node.Parent, null, owner);
          
          return owner;
        }
      }
    }

    public class AdvNodeCellAccessibleObject : AccessibleObject
    {
      TreeNodeAdv node;
      AdvNodeAccessibleObject parent;
      int colIndex;

      public AdvNodeCellAccessibleObject(TreeNodeAdv advNode, int colIndex, AdvNodeAccessibleObject parent)
        : base()
      {
        this.node = advNode;
        this.colIndex = colIndex;
        this.parent = parent;
      }

      public override Rectangle Bounds
      {
        get
        {
          if (!node.IsVisible)
            return Rectangle.Empty;

          Rectangle colBounds = node.Tree.GetColumnBounds(colIndex);
          Rectangle nodeBounds = node.Tree.GetNodeBounds(node);
          Rectangle bounds = new Rectangle(colBounds.X, nodeBounds.Y, colBounds.Width, nodeBounds.Height);
          Point p = node.Tree.ScrollPosition;

          int colHeaderY = node.Tree.UseColumns ? node.Tree.ColumnHeaderHeight : 0;

          bounds.Offset(-p.X, -p.Y * node.Tree.RowHeight + colHeaderY);
          return node.Tree.RectangleToScreen(bounds);
        }
      }

      public override AccessibleRole Role
      {
        get { return AccessibleRole.Cell; }
      }

      public override string Name
      {
        get
        {
          string header = node.Tree.Columns[colIndex].Header;
          if (string.IsNullOrEmpty(header))
            header = "Column" + colIndex;
          return header;
        }
      }

      /// <summary>
      /// The cell value is the contents of all node control values belonging to the column the cell is in.
      /// </summary>
      public override string Value
      {
        get
        {
          StringBuilder sb = new StringBuilder();
          foreach (NodeControlInfo info in node.Tree.GetNodeControls(node))
          {
            BindableControl ctrl = info.Control as BindableControl;
            if (ctrl != null && ctrl.ParentColumn != null && ctrl.ParentColumn.Index == colIndex)
            {
              object val = ctrl.GetValue(node);
              if (val != null)
                sb.AppendLine(val.ToString());
            }
          }
          return sb.ToString().Trim();
        }
      }

      public override AccessibleObject Parent
      {
        get
        {
          return parent;
        }
      }

      /// <summary>
      /// This does nothing because it is done by the tree.
      /// </summary>
      /// <param name="x"></param>
      /// <param name="y"></param>
      /// <returns></returns>
      public override AccessibleObject HitTest(int x, int y)
      {
        return null;
      }
    }
  }
}
