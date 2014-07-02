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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_treenodeview.h"
#include "wf_treenode.h"

using namespace System::Drawing;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace Aga::Controls::Tree;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

//----------------- TreeViewNode -------------------------------------------------------------------

TreeViewNode::TreeViewNode()
{
  myTag = NULL;
  data = NULL;
  attributes = new std::vector<mforms::TreeNodeTextAttributes>();
}

//--------------------------------------------------------------------------------------------------

TreeViewNode::~TreeViewNode()
{
  delete attributes;
  delete myTag;
  if (data)
    data->release();
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::DestroyDataRecursive()
{
  for (int i = 0; i < Nodes->Count; ++i)
    dynamic_cast<MySQL::Forms::TreeViewNode^> (Nodes[i])->DestroyDataRecursive();
  Data = nullptr;
};

//--------------------------------------------------------------------------------------------------

std::string TreeViewNode::MyTag::get()
{
  if (myTag != NULL)
    return *myTag;
  return "";
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::MyTag::set(std::string s)
{
  if (myTag)
    delete myTag;
  myTag = new std::string(s);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeData* TreeViewNode::Data::get()
{
  return data;
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Data::set(mforms::TreeNodeData *d)
{
  if (data != d)
  {
    if (data)
      data->release();
    data = d;
    if (data)
      data->retain();
  }
}

//--------------------------------------------------------------------------------------------------

String^ TreeViewNode::Caption::get(int index)
{
  if (index < 0 || index >= captions.Count)
    return "";
  return captions[index] == nullptr ? "" : captions[index];
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Caption::set(int index, String^ newText)
{
  while (index >= captions.Count)
    captions.Add(nullptr);
  captions[index] = newText;

  // The simple Text property of the base class is used to identify nodes (e.g. on sort) by the tree.
  // So we have to set text there too to make this work.
  newText = "";
  for each (String ^caption in captions)
    newText += caption + ":";
  Text = newText;
}

//--------------------------------------------------------------------------------------------------

/**
 * Retrieves the caption of all columns in one string (elements are separated by tab).
 */
String^ TreeViewNode::FullCaption::get()
{
  String ^result = "";
  for each (String ^entry in captions)
  {
    if (result->Length > 0)
      result += "\t";
    result += entry == nullptr ? "" : entry;
  }
  return result;
}

//--------------------------------------------------------------------------------------------------

Bitmap^ TreeViewNode::Icon::get(int index)
{
  if (icons.ContainsKey(index))
    return icons[index];
  return nullptr;
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Icon::set(int index, Bitmap^ newIcon)
{
  icons[index] = newIcon;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeTextAttributes TreeViewNode::Attributes::get(int index)
{
  if (index < 0 || index >= (int)attributes->size())
    return mforms::TreeNodeTextAttributes();
  return (*attributes)[index];
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Attributes::set(int index, mforms::TreeNodeTextAttributes newAttributes)
{
  while (index >= (int)attributes->size())
    attributes->push_back(mforms::TreeNodeTextAttributes());
    
  (*attributes)[index] = newAttributes;
}

//----------------- TreeNodeWrapper ----------------------------------------------------------------

TreeNodeWrapper::TreeNodeWrapper(TreeNodeViewWrapper *wrapper, TreeViewNode ^node)
{
  treeWrapper = wrapper;
  nativeTree = treeWrapper->GetManagedObject<TreeViewAdv>();
  nativeNode = node;
  isRoot = false;
  refCount = 0;
}

//--------------------------------------------------------------------------------------------------

TreeNodeWrapper::TreeNodeWrapper(TreeNodeViewWrapper *wrapper)
{
  treeWrapper = wrapper;
  nativeTree = treeWrapper->GetManagedObject<TreeViewAdv>();
  isRoot = true;
  refCount = 0;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::release()
{
  refCount--;
  if (refCount == 0)
    delete this;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::retain()
{
  refCount++;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::is_root() const
{
  return isRoot;
}

//--------------------------------------------------------------------------------------------------

static TreeNodeAdv ^find_node_adv_for(Node ^node, TreeNodeAdv ^root)
{
  if (node == nullptr)
    return nullptr;

  if (node->Parent == nullptr)
    return root;
  TreeNodeAdv ^parent = find_node_adv_for(node->Parent, root);
  if (parent != nullptr)
  {
    int i = node->Index;
    if (i >= 0 && i < parent->Children->Count)
      return parent->Children[i];
  }
  return nullptr;
}

//--------------------------------------------------------------------------------------------------

TreeNodeAdv^ TreeNodeWrapper::find_node_adv()
{
  // apparently no mapping between Node to TreeNodeAdv, so we need
  // to scan it around for it...

  return find_node_adv_for(nativeNode, nativeTree->Root);
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::node_index()
{
  if (isRoot)
    return -1;

  return nativeNode->Index;
}

//--------------------------------------------------------------------------------------------------

Aga::Controls::Tree::TreeModel ^TreeNodeWrapper::model() const
{
  return dynamic_cast<Aga::Controls::Tree::TreeModel ^>(nativeTree->Model);
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::equals(const mforms::TreeNode &other)
{
  const TreeNodeWrapper *oth = dynamic_cast<const TreeNodeWrapper*>(&other);
  if (oth)
    return (TreeViewNode ^)oth->nativeNode == (TreeViewNode ^)nativeNode;
  return false;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::is_valid() const
{
  return !isRoot;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_icon_path(int column, const std::string &icon)
{
  if (!isRoot && !icon.empty())
  {
    bool invalidate = true;
    String ^str_icon = CppStringToNative(icon);
    if (!TreeViewNode::iconStorage.ContainsKey(str_icon))
    {
      String^ path = CppStringToNative(mforms::App::get()->get_resource_path(icon));
      if (File::Exists(path))
        TreeViewNode::iconStorage[str_icon] = gcnew Bitmap(path);
      else
        invalidate = false;
    }

    if (invalidate)
    {
      nativeNode->Icon[column] = TreeViewNode::iconStorage[str_icon];
      nativeTree->Invalidate();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_attributes(int column, const mforms::TreeNodeTextAttributes& attrs)
{
  if (!isRoot)
  {
    nativeNode->Attributes[column] = attrs;
    nativeTree->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_string(int column, const std::string &value)
{
  if (!isRoot)
  {
    nativeNode->Caption[column] = CppStringToNative(value);
    treeWrapper->node_value_set(column);
    nativeTree->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_int(int column, int value)
{
  if (!isRoot)
  {
    nativeNode->Caption[column] = Convert::ToString(value);
    treeWrapper->node_value_set(column);
    nativeTree->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_long(int column, boost::int64_t value)
{
  if (!isRoot)
  {
    nativeNode->Caption[column] = Convert::ToString(value);
    treeWrapper->node_value_set(column);
    nativeTree->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_bool(int column, bool value)
{
  if (!isRoot)
  {
    nativeNode->Caption[column] = value ? "1" : "0";
    treeWrapper->node_value_set(column);
    nativeTree->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_float(int column, double value)
{
  if (!isRoot)
  {
    nativeNode->Caption[column] = Convert::ToString(value);
    treeWrapper->node_value_set(column);
    nativeTree->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

std::string TreeNodeWrapper::get_string(int column) const
{
  if (!isRoot)
    return NativeToCppString(nativeNode->Caption[column]);

  return "";
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::get_int(int column) const
{
  if (!isRoot)
    return Convert::ToInt32(nativeNode->Caption[column]);

  return 0;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::get_bool(int column) const
{
  if (!isRoot)
    return (nativeNode->Caption[column] == "Checked") || (nativeNode->Caption[column] == "1") ? true : false;

  return false;
}

//--------------------------------------------------------------------------------------------------

boost::int64_t TreeNodeWrapper::get_long(int column) const
{
  if (!isRoot)
    return Convert::ToInt64(nativeNode->Caption[column]);

  return 0;
}

//--------------------------------------------------------------------------------------------------

double TreeNodeWrapper::get_float(int column) const
{
  if (!isRoot)
    return Convert::ToDouble(nativeNode->Caption[column]);

  return 0;
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::count() const
{
  if (isRoot)
    return model()->Root->Nodes->Count;
  else
    return nativeNode->Nodes->Count;
}

//--------------------------------------------------------------------------------------------------

Bitmap^ TreeNodeWrapper::get_cached_icon(const std::string& icon_id)
{
  Bitmap ^icon;

  String^ str_icon = CppStringToNative(icon_id);

  if (!TreeViewNode::iconStorage.ContainsKey(str_icon))
  {
    String^ path = CppStringToNative(mforms::App::get()->get_resource_path(icon_id));
    if (File::Exists(path))
      TreeViewNode::iconStorage[str_icon] = gcnew Bitmap(path);
  }

  if (TreeViewNode::iconStorage.ContainsKey(str_icon))
    icon = TreeViewNode::iconStorage[str_icon];

  return icon;
}

//--------------------------------------------------------------------------------------------------

std::vector<mforms::TreeNodeRef> TreeNodeWrapper::add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes, int position)
{
  std::vector<mforms::TreeNodeRef> result;

  Node ^parent = isRoot ? model()->Root : nativeNode;

  if (!nodes.captions.empty())
  {
    // The icon will be common to all the nodes in the collection
    Bitmap^ icon;
    if (!nodes.icon.empty())
      icon = get_cached_icon(nodes.icon);

    // Creates an array with all the nodes in the collection
    array<MySQL::Forms::TreeViewNode^>^ added_nodes= gcnew array<MySQL::Forms::TreeViewNode^>((int)nodes.captions.size());
    for(int index=0; index < added_nodes->Length; index++)
      added_nodes[index] = gcnew MySQL::Forms::TreeViewNode();

    // Now initializes the nodes in the collection with the proper information
    for ( int index = 0; index < added_nodes->Length; index++)
    {
      added_nodes[index]->Caption[0] = CppStringToNative(nodes.captions[index]);
      added_nodes[index]->Icon[0] = icon;
    }

    // Finally we add the nodes to the tree
    for ( int index = 0; index < added_nodes->Length; index++)
    {
      if (position == -1)
        parent->Nodes->Add(added_nodes[index]);
      else
        parent->Nodes->Insert(position++, added_nodes[index]);
      
      result.push_back(mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, added_nodes[index])));
    }

    // Now add the child nodes.
    if (!nodes.children.empty())
      add_children_from_skeletons(added_nodes, nodes.children);

    // Refreshes the Tree Control
    nativeTree->Invalidate();
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::add_children_from_skeletons(array<MySQL::Forms::TreeViewNode^>^ parents, const std::vector<mforms::TreeNodeSkeleton>& children)
{
  for (size_t child_index = 0; child_index < children.size(); child_index++)
  {
    // Creates "this" child for each parent
    array<MySQL::Forms::TreeViewNode^>^ added_nodes = gcnew array<MySQL::Forms::TreeViewNode^>(parents->Length);
    for(int index=0; index < added_nodes->Length; index++)
      added_nodes[index] = gcnew MySQL::Forms::TreeViewNode();

    // Gets the attributes for "this" child
    String^ caption = CppStringToNative(children[child_index].caption);
    Bitmap^ icon = get_cached_icon(children[child_index].icon);

    // Initializes the child for the different parents
    for(int added_node_index = 0; added_node_index < parents->Length; added_node_index++)
    {
      added_nodes[added_node_index]->Icon[0] = icon;
      added_nodes[added_node_index]->Caption[0] = caption;
      added_nodes[added_node_index]->MyTag = children[child_index].tag;
    }

    // Now inserts each children to it's corresponding parent
    for(int parent_index = 0; parent_index < parents->Length; parent_index++)
      parents[parent_index]->Nodes->Add(added_nodes[parent_index]);

    // If the new nodes will have children as well insert them.
    if (children[child_index].children.size())
      add_children_from_skeletons(added_nodes, children[child_index].children);

  }
}  

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::insert_child(int index)
{
  Node ^parent = isRoot ? model()->Root : nativeNode;

  MySQL::Forms::TreeViewNode ^child = gcnew MySQL::Forms::TreeViewNode();
  if (index < 0)
    parent->Nodes->Add(child);
  else
    parent->Nodes->Insert(index, child);

  return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, child));
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::remove_from_parent()
{
  if (!isRoot && nativeNode->Parent != nullptr)
  {
    if (!nativeNode->MyTag.empty())
      treeWrapper->process_mapping(nullptr, nativeNode->MyTag);

    nativeNode->Parent->Nodes->Remove(nativeNode);
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::remove_children()
{
  Node ^parent = isRoot ? model()->Root : nativeNode;
  for each (Node ^child in parent->Nodes)
  {
    TreeViewNode ^node = dynamic_cast<TreeViewNode^>(child);
    if (node != nullptr && !node->MyTag.empty())
      treeWrapper->process_mapping(nullptr, node->MyTag);
  }
  parent->Nodes->Clear();
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::get_child(int index) const
{
  Node ^parent = isRoot ? model()->Root : nativeNode;

  TreeViewNode ^child = dynamic_cast<TreeViewNode^>(parent->Nodes[index]);
  if (child != nullptr)
    return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, child));

  return mforms::TreeNodeRef();
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::get_parent() const
{
  if (!isRoot)
  {
    TreeViewNode ^parent = dynamic_cast<TreeViewNode^>(nativeNode->Parent);
    if (parent != nullptr)
      return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, parent));
    else
      return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper));
  }

  return mforms::TreeNodeRef();
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::previous_sibling() const
{
  if (isRoot)
    return mforms::TreeNodeRef();

  TreeViewNode ^node = dynamic_cast<TreeViewNode^>(nativeNode->PreviousNode);
  if (node == nullptr)
    return mforms::TreeNodeRef();

  return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, node));
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::next_sibling() const
{
  if (isRoot)
    return mforms::TreeNodeRef();

  TreeViewNode ^node = dynamic_cast<TreeViewNode^>(nativeNode->NextNode);
  if (node == nullptr)
    return mforms::TreeNodeRef();

  return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, node));
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::expand()
{
  if (!is_root())
    get_parent()->expand();
  TreeNodeAdv ^nodeadv = find_node_adv();
  if (nodeadv != nullptr)
    nodeadv->Expand();
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::collapse()
{
  TreeNodeAdv ^nodeadv = find_node_adv();
  if (nodeadv != nullptr)
    nodeadv->Collapse();
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::is_expanded()
{
  TreeNodeAdv ^nodeadv = find_node_adv();
  if (nodeadv != nullptr)
    return nodeadv->IsExpanded;
  return false;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_tag(const std::string &tag)
{
  if (!isRoot)
  {
    if (!nativeNode->MyTag.empty())
      treeWrapper->process_mapping(nullptr, nativeNode->MyTag);
    nativeNode->MyTag = tag;
    treeWrapper->process_mapping(nativeNode, nativeNode->MyTag);
  }
}

//--------------------------------------------------------------------------------------------------

std::string TreeNodeWrapper::get_tag() const
{
  if (!isRoot)
    return nativeNode->MyTag;

  return "";
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_data(mforms::TreeNodeData *data)
{
  if (!isRoot)
    nativeNode->Data = data;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeData *TreeNodeWrapper::get_data() const
{
  if (!isRoot)
    return nativeNode->Data;

  return NULL;
}

//--------------------------------------------------------------------------------------------------
