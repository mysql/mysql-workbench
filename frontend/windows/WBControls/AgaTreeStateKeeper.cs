/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0,
 * as published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms, as
 * designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an additional
 * permission to link the program and your derivative works with the
 * separately licensed software that they have included with MySQL.
 * This program is distributed in the hope that it will be useful,  but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA 
 */

using System.Diagnostics;
using System.Collections;

using Aga.Controls.Tree;
using Aga.Controls.Tree.NodeControls;

namespace MySQL.Utilities
{
  /// <summary>
  /// This class is used to save and restore certain states of a given AdvTreeView.
  /// This is necessary to make them persistent over refresh cycles.
  /// 
  /// The used expand state keeping algorithm is robust against any structure change in the tree
  /// except for node level changes (move a node a level up or down). When moving up a node the outcome
  /// depends on the remaining tree structure if the expand state is restored. When moving down
  /// the node is simply ignored and expand state restoration continues as usual.
  /// 
  /// The algorithm is based on the node's captions so it works only if the nodes actually have
  /// captions (in the main column). Do not use duplicate captions for nodes on the same level or
  /// direct parent levels to avoid confusing the tree walker.
  /// </summary>
  public class AgaTreeStateKeeper
  {
    private Queue nodePath = new Queue();
    private const string backtrack = "\u0011";

    public TreeViewAdv Tree { get; set; }

    public AgaTreeStateKeeper(TreeViewAdv tree)
    {
      Debug.Assert(tree != null, "State keeper needs a tree to work with.");

      Tree = tree;
      Tree.BeginUpdate();

      TreeNodeAdv node = Tree.Root;
      do
      {
        if (node.IsExpanded && node.Children.Count > 0)
        {
          nodePath.Enqueue(GetCaption(node));
          node = node.Children[0];
        }
        else
        {
          if (node.NextNode != null)
            node = node.NextNode;
          else
          {
            // No further sibling. Walk up the parents path until we find one with a next sibling.
            do
            {
              if (node.Parent == null)
                node = null;
              else
              {
                node = node.Parent;
                nodePath.Enqueue(backtrack);
                if (node.NextNode != null)
                {
                  node = node.NextNode;
                  break;
                }
              }
            }
            while (node != null);
          }
        }
      }
      while (node != null);
    }

    public void RestoreStates()
    {
      Debug.Assert(Tree != null, "State keeper needs a tree to work with.");

      if (nodePath.Count > 0)
      {
        TreeNodeAdv node = Tree.Root;
        do
        {
          string current = nodePath.Dequeue() as string;
          if (current == backtrack)
            node = node.Parent;
          else
            if (node.Children.Count == 0)
              node = node.NextNode;
            else
            {
              foreach (TreeNodeAdv child in node.Children)
                if (GetCaption(child) == current)
                {
                  child.IsExpanded = true;
                  node = child;
                  break;
                }
            }
        }
        while (node != null && nodePath.Count > 0);
      }

      Tree.EndUpdate();
    }

    /// <summary>
    /// Returns the caption of the given node in the first (main) column.
    /// </summary>
    /// <param name="node"></param>
    /// <returns></returns>
    private string GetCaption(TreeNodeAdv node)
    {
      foreach (NodeControl control in Tree.NodeControls)
      {
        if (!Tree.UseColumns || control.ParentColumn.Index == 0)
        {
          BaseTextControl bindable = control as BaseTextControl;
          if (bindable == null)
            continue;
          object value = bindable.GetValue(node);
          if (value != null)
          {
            string result = value.ToString();
            if (result.Length > 0)
              return result;
          }
        }
      }
      return "";
    }

  }
}
