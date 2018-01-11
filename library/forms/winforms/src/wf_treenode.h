/*
 * Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

namespace MySQL {
  namespace Forms {

  public
    ref class TreeViewNode : Aga::Controls::Tree::Node {
    private:
      std::string *myTag;
      mforms::TreeNodeData *data;
      System::Collections::Generic::List<String ^> captions;
      std::vector<mforms::TreeNodeTextAttributes> *attributes;
      System::Collections::Generic::Dictionary<int, Drawing::Bitmap ^> icons;

    public:
      static System::Collections::Generic::Dictionary<String ^, Drawing::Bitmap ^> iconStorage;

      TreeViewNode();
      virtual ~TreeViewNode();
      void DestroyDataRecursive();

      property std::string MyTag {
        std::string get();
        void set(std::string s);
      }

      property mforms::TreeNodeData *Data {
        mforms::TreeNodeData *get();
        void set(mforms::TreeNodeData * d);
      }

      property String ^ Caption[int] {
        String ^ get(int index);
        void set(int index, String ^ newText);
      }

      property String ^ FullCaption { String ^ get(); }

        property Drawing::Bitmap ^
        Icon[int] {
        Drawing::Bitmap ^ get(int index);
        void set(int index, Drawing::Bitmap ^ newIcon);
      }

      property mforms::TreeNodeTextAttributes Attributes[int] {
        mforms::TreeNodeTextAttributes get(int index);
        void set(int index, mforms::TreeNodeTextAttributes newAttributes);
      }
    };

  private
    class TreeNodeWrapper : public mforms::TreeNode {
    private:
      gcroot<Aga::Controls::Tree::Node ^>
        nativeNode; // The model node (implicitly there's always a model in TreeViewAdv).
      gcroot<Aga::Controls::Tree::TreeNodeAdv ^> nativeNodeAdv; // The tree node for the model node.

      TreeViewWrapper *treeWrapper;

      bool isRoot;
      int refCount;

    protected:
      virtual void add_children_from_skeletons(std::vector<TreeNodeWrapper> parents,
                                               const std::vector<mforms::TreeNodeSkeleton> &children);
      void node_changed(Aga::Controls::Tree::TreeNodeAdv ^ new_node);

    public:
      TreeNodeWrapper(TreeViewWrapper *wrapper, Aga::Controls::Tree::TreeNodeAdv ^ node);
      TreeNodeWrapper(TreeViewWrapper *wrapper);

      int node_index();

      virtual void release();
      virtual void retain();

      virtual bool equals(const mforms::TreeNode &other);
      virtual bool is_valid() const;
      virtual int level() const;

      virtual void set_icon_path(int column, const std::string &icon);
      virtual void set_selected(bool flag);
      virtual void scrollToNode();

      virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs);
      virtual void set_string(int column, const std::string &value);
      virtual void set_int(int column, int value);
      virtual void set_long(int column, std::int64_t value);
      virtual void set_bool(int column, bool value);
      virtual void set_float(int column, double value);

      virtual std::string get_string(int column) const;
      virtual int get_int(int column) const;
      virtual std::int64_t get_long(int column) const;
      virtual bool get_bool(int column) const;
      virtual double get_float(int column) const;

      virtual int count() const;
      virtual mforms::TreeNodeRef insert_child(int index);
      virtual void insert_child(int index, const mforms::TreeNode &child);
      virtual void remove_from_parent();
      virtual mforms::TreeNodeRef get_child(int index) const;
      virtual int get_child_index(mforms::TreeNodeRef node) const;
      virtual mforms::TreeNodeRef get_parent() const;
      virtual mforms::TreeNodeRef previous_sibling() const;
      virtual mforms::TreeNodeRef next_sibling() const;
      virtual void remove_children();
      virtual void move_node(mforms::TreeNodeRef node, bool before);

      Drawing::Bitmap ^ get_cached_icon(const std::string &icon_id);
      virtual std::vector<mforms::TreeNodeRef> add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes,
                                                                   int position = -1);

      virtual void expand();
      virtual void collapse();
      virtual bool is_expanded();

      virtual void set_tag(const std::string &tag);
      virtual std::string get_tag() const;

      virtual void set_data(mforms::TreeNodeData *data);
      virtual mforms::TreeNodeData *get_data() const;
    };
  }
}
