/*
 * Copyright (c) 2016, 2018, Oracle and/or its affiliates. All rights reserved.
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

﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GUI_Test_App {
  public partial class Form1 : Form {
    public Form1() {
      InitializeComponent();

      // Fill treeview1.
      for (int i = 0; i < 5; ++i) {
        TreeNode node1 = new TreeNode(String.Format("Entry 0.{0}", i));
        for (int j = 0; j < 5; ++j) {
          TreeNode node2 = new TreeNode(String.Format("Entry 1.{0}", j));
          for (int k = 0; k < 5; ++k) {
            TreeNode node3 = new TreeNode(String.Format("Entry 2.{0}", k));
            for (int l = 0; l < 5; ++l) {
              TreeNode node4 = new TreeNode(String.Format("Entry 3.{0}", l));
              for (int m = 0; m < 5; ++m) {
                TreeNode node5 = new TreeNode(String.Format("Entry 4.{0}", m));
                for (int n = 0; n < 5; ++n) {
                  node5.Nodes.Add(new TreeNode(String.Format("Entry 5.{0}", n)));
                }
                node4.Nodes.Add(node5);
              }
              node3.Nodes.Add(node4);
            }
            node2.Nodes.Add(node3);
          }
          node1.Nodes.Add(node2);
        }
        treeView1.Nodes.Add(node1);
      }

      // Fill gridView
      var list = new List<Data>();
      var idx = 1;
      for (int i = 0; i < 19; i++) {
        var data = new Data {
          column1 = idx.ToString(),
          column2 = (idx + 1).ToString(),
          column3 = (idx + 2).ToString(),
          column4 = (idx + 3).ToString()
        };
        list.Add(data);
        idx++;
      }
      var bindingList = new BindingList<Data>(list);
      var source = new BindingSource(bindingList, null);
      dataGridView1.DataSource = source;

      foreach (DataGridViewColumn item in dataGridView1.Columns) {
        item.Width = 50;
      }

      System.Windows.Forms.ImageList myImageList1 = new ImageList();
      myImageList1.ImageSize = new Size(16, 16);

      var files = Directory.GetFiles("C:\\Windows", "*.exe", SearchOption.TopDirectoryOnly);

      int count = (50 / files.Length) + 1;
      idx = 1;
      for (int i = 0; i < count; i++) {
        foreach (var file in files) {
          myImageList1.Images.Add(Icon.ExtractAssociatedIcon(file));
          listView2.Items.Add(String.Format("Title {0}", idx), idx++);
        }
      }
      listView2.LargeImageList = myImageList1;

    }

    private void Button2_Click(object sender, EventArgs e) {

    }

    private void GroupBox2_Enter(object sender, EventArgs e) {

    }

    private void TreeView1_AfterSelect(object sender, TreeViewEventArgs e) {

    }

    private void MenuStrip1_ItemClicked(object sender, ToolStripItemClickedEventArgs e) {

    }
    void TrackBar1_ValueChanged(object sender, EventArgs e) {
      this.label4.Text = trackBar1.Value.ToString() + " %";
    }

    void TrackBar2_ValueChanged(object sender, EventArgs e) {
      this.label5.Text = trackBar2.Value.ToString() + " %";
    }

    void TrackBar3_ValueChanged(object sender, EventArgs e) {
      this.label6.Text = trackBar3.Value.ToString() + " %";
    }

    void TrackBar4_ValueChanged(object sender, EventArgs e) {
      this.label7.Text = trackBar4.Value.ToString() + " %";
    }

    private void button3_Click(object sender, EventArgs e) {
      Dialog dialog = new Dialog();
      dialog.ShowDialog();
    }

    private void button1_Click(object sender, EventArgs e) {
      Dialog dialog = new Dialog();
      dialog.ShowDialog();
    }

    private void redoToolStripMenuItem_Click(object sender, EventArgs e) {
    }

    private void undoToolStripMenuItem_Click(object sender, EventArgs e) {
      Control ctrl = getFocusedControl();
      if (ctrl != null) {
        if (ctrl is TextBox) {
          var box = (TextBox)ctrl;
          if (box.CanUndo) {
            box.Undo();
          }
        }
      }
    }

    [DllImport("user32.dll")]
    public static extern IntPtr GetFocus();

    private Control getFocusedControl(){
      Control focusedControl = null;
      IntPtr focusedHandle = GetFocus();
      if (focusedHandle != IntPtr.Zero) {
        focusedControl = Control.FromHandle(focusedHandle);
      }
      return focusedControl;
    }

    private void cutToolStripMenuItem_Click(object sender, EventArgs e) {
      Control ctrl = getFocusedControl();
      if (ctrl != null) {
        if (ctrl is TextBox) {
          var box = (TextBox)ctrl;
          box.Cut();
        }
      }
    }

    private void copyToolStripMenuItem_Click(object sender, EventArgs e) {
      Control ctrl = getFocusedControl();
      if (ctrl != null) {
        if (ctrl is TextBox) {
          var box = (TextBox)ctrl;
          box.Copy();
        }
      }
    }

    private void pasteToolStripMenuItem_Click(object sender, EventArgs e) {
      Control ctrl = getFocusedControl();
      if (ctrl != null) {
        if (ctrl is TextBox) {
          var box = (TextBox)ctrl;
          box.Paste();
        }
      }
    }

    private void deleteToolStripMenuItem_Click(object sender, EventArgs e) {
    }

    private void selectAllToolStripMenuItem_Click(object sender, EventArgs e) {
    }

    private void fireActionToolStripMenuItem_Click(object sender, EventArgs e)
    {
      this.edit1.Text = "New text";
    }
  }

  class Data {
    [DisplayName("#1")]
    public string column1 { get; set; }
    [DisplayName("#2")]
    public string column2 { get; set; }
    [DisplayName("#3")] 
    public string column3 { get; set; }
    [DisplayName("#4")] 
    public string column4 { get; set; }
  };

}
