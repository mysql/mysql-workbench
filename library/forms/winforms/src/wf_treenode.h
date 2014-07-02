/* 
 * Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

namespace MySQL {
  namespace Forms {

    public ref class TreeViewNode : Aga::Controls::Tree::Node
    {
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

      property std::string MyTag
      {
        std::string get();
        void set(std::string s);
      }

      property mforms::TreeNodeData* Data
      {
        mforms::TreeNodeData* get();
        void set(mforms::TreeNodeData *d);
      }

      property String ^Caption[int]
      {
        String ^get(int index);
        void set(int index, String^ newText);
      }

      property String ^FullCaption
      {
        String ^get();
      }

      property Drawing::Bitmap^ Icon[int]
      {
        Drawing::Bitmap^ get(int index);
        void set(int index, Drawing::Bitmap^ newIcon);
      }

      property mforms::TreeNodeTextAttributes Attributes[int]
      {
        mforms::TreeNodeTextAttributes get(int index);
        void set(int index, mforms::TreeNodeTextAttributes newAttributes);
      }
    };

    private class TreeNodeWrapper : public mforms::TreeNode
    {
    private:
      gcroot<Aga::Controls::Tree::TreeViewAdv ^> nativeTree;
      gcroot<TreeViewNode ^> nativeNode;

      TreeNodeViewWrapper *treeWrapper;

      bool isRoot;
      int refCount;
    protected:
      bool is_root() const;
      Aga::Controls::Tree::TreeModel ^model() const;

    public:
      TreeNodeWrapper(TreeNodeViewWrapper *wrapper, TreeViewNode ^node);
      TreeNodeWrapper(TreeNodeViewWrapper *wrapper);
  
      Aga::Controls::Tree::TreeNodeAdv^ find_node_adv();
      int node_index();

      virtual void release();
      virtual void retain();

      virtual bool equals(const mforms::TreeNode &other);
      virtual bool is_valid() const;
    
      virtual void set_icon_path(int column, const std::string &icon);

      virtual void set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs);
      virtual void set_string(int column, const std::string &value);
      virtual void set_int(int column, int value);
      virtual void set_long(int column, boost::int64_t value);
      virtual void set_bool(int column, bool value);
      virtual void set_float(int column, double value);

      virtual std::string get_string(int column) const;
      virtual int get_int(int column) const;
      virtual boost::int64_t get_long(int column) const;
      virtual bool get_bool(int column) const;
      virtual double get_float(int column) const;
    
      virtual int count() const;
      virtual mforms::TreeNodeRef insert_child(int index);
      virtual void remove_from_parent();
      virtual mforms::TreeNodeRef get_child(int index) const;
      virtual mforms::TreeNodeRef get_parent() const;
      virtual mforms::TreeNodeRef previous_sibling() const;
      virtual mforms::TreeNodeRef next_sibling() const;
      virtual void remove_children();

      Drawing::Bitmap^ get_cached_icon(const std::string &icon_id);
      virtual std::vector<mforms::TreeNodeRef> add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes, int position = -1);
      virtual void add_children_from_skeletons(array<MySQL::Forms::TreeViewNode^>^ parents, const std::vector<mforms::TreeNodeSkeleton>& children);
    
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
