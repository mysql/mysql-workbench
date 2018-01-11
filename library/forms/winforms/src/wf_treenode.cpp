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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_treeview.h"
#include "wf_treenode.h"

using namespace System::Drawing;
using namespace System::IO;
using namespace System::Windows::Forms;

using namespace Aga::Controls::Tree;

using namespace MySQL;
using namespace MySQL::Forms;
using namespace MySQL::Utilities;

//----------------- TreeViewNode -------------------------------------------------------------------

TreeViewNode::TreeViewNode() {
  myTag = NULL;
  data = NULL;
  attributes = new std::vector<mforms::TreeNodeTextAttributes>();
}

//--------------------------------------------------------------------------------------------------

TreeViewNode::~TreeViewNode() {
  delete attributes;
  delete myTag;
  if (data)
    data->release();
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::DestroyDataRecursive() {
  for (int i = 0; i < Nodes->Count; ++i)
    dynamic_cast<MySQL::Forms::TreeViewNode ^>(Nodes[i])->DestroyDataRecursive();
  Data = nullptr;
};

//--------------------------------------------------------------------------------------------------

std::string TreeViewNode::MyTag::get() {
  if (myTag != NULL)
    return *myTag;
  return "";
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::MyTag::set(std::string s) {
  if (myTag)
    delete myTag;
  myTag = new std::string(s);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeData *TreeViewNode::Data::get() {
  return data;
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Data::set(mforms::TreeNodeData *d) {
  if (data != d) {
    if (data)
      data->release();
    data = d;
    if (data)
      data->retain();
  }
}

//--------------------------------------------------------------------------------------------------

String ^ TreeViewNode::Caption::get(int index) {
  if (index < 0 || index >= captions.Count)
    return "";
  return captions[index] == nullptr ? "" : captions[index];
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Caption::set(int index, String ^ newText) {
  while (index >= captions.Count)
    captions.Add(nullptr);
  captions[index] = newText;

  // The simple Text property of the base class is used to identify nodes (e.g. on sort) by the tree.
  // So we have to set text there too to make this work.
  newText = "";
  for each(String ^ caption in captions) newText += caption + ":";
  Text = newText;
}

//--------------------------------------------------------------------------------------------------

/**
 * Retrieves the caption of all columns in one string (elements are separated by tab).
 */
String ^ TreeViewNode::FullCaption::get() {
  String ^ result = "";
  for each(String ^ entry in captions) {
      if (result->Length > 0)
        result += "\t";
      result += entry == nullptr ? "" : entry;
    }
  return result;
}

//--------------------------------------------------------------------------------------------------

Bitmap ^ TreeViewNode::Icon::get(int index) {
  if (icons.ContainsKey(index))
    return icons[index];
  return nullptr;
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Icon::set(int index, Bitmap ^ newIcon) {
  icons[index] = newIcon;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeTextAttributes TreeViewNode::Attributes::get(int index) {
  if (index < 0 || index >= (int)attributes->size())
    return mforms::TreeNodeTextAttributes();
  return (*attributes)[index];
}

//--------------------------------------------------------------------------------------------------

void TreeViewNode::Attributes::set(int index, mforms::TreeNodeTextAttributes newAttributes) {
  while (index >= (int)attributes->size())
    attributes->push_back(mforms::TreeNodeTextAttributes());

  (*attributes)[index] = newAttributes;
}

//----------------- TreeNodeWrapper ----------------------------------------------------------------

TreeNodeWrapper::TreeNodeWrapper(TreeViewWrapper *wrapper, TreeNodeAdv ^ node) {
  treeWrapper = wrapper;
  nativeNodeAdv = node;
  nativeNode = dynamic_cast<TreeViewNode ^>(node->Tag);
  isRoot = false;
  refCount = 0;
}

//--------------------------------------------------------------------------------------------------

TreeNodeWrapper::TreeNodeWrapper(TreeViewWrapper *wrapper) {
  treeWrapper = wrapper;
  nativeNodeAdv = treeWrapper->GetManagedObject<TreeViewAdv>()->Root;
  TreeModel ^ model = dynamic_cast<TreeModel ^>(treeWrapper->GetManagedObject<TreeViewAdv>()->Model);
  nativeNode = dynamic_cast<Node ^>(model->Root);
  isRoot = true;
  refCount = 0;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::release() {
  refCount--;
  if (refCount == 0)
    delete this;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::retain() {
  refCount++;
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::node_index() {
  if (isRoot)
    return -1;

  return nativeNode->Index;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::equals(const mforms::TreeNode &other) {
  const TreeNodeWrapper *oth = dynamic_cast<const TreeNodeWrapper *>(&other);
  if (oth)
    return (Node ^)oth->nativeNode == (Node ^)nativeNode;
  return false;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::is_valid() const {
  return !isRoot;
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::level() const {
  return nativeNodeAdv->Level;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_icon_path(int column, const std::string &icon) {
  if (!isRoot && !icon.empty()) {
    bool invalidate = true;
    String ^ str_icon = CppStringToNative(icon);
    if (!TreeViewNode::iconStorage.ContainsKey(str_icon)) {
      String ^ path = CppStringToNative(mforms::App::get()->get_resource_path(icon));
      if (File::Exists(path))
        TreeViewNode::iconStorage[str_icon] = gcnew Bitmap(path);
      else
        invalidate = false;
    }

    if (invalidate) {
      TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
      node->Icon[column] = TreeViewNode::iconStorage[str_icon];
      treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
    }
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_selected(bool flag) {
  nativeNodeAdv->IsSelected = flag;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::scrollToNode() {
  TreeViewAdv ^ view = treeWrapper->GetManagedObject<TreeViewAdv>();
  if (view != nullptr) {
    view->ScrollTo(nativeNodeAdv);
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_attributes(int column, const mforms::TreeNodeTextAttributes &attrs) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Attributes[column] = attrs;
    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_string(int column, const std::string &value) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Caption[column] = CppStringToNative(value);
    treeWrapper->node_value_set(column);
    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_int(int column, int value) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Caption[column] = Convert::ToString(value);
    treeWrapper->node_value_set(column);
    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_long(int column, std::int64_t value) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Caption[column] = Convert::ToString(value);
    treeWrapper->node_value_set(column);
    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_bool(int column, bool value) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Caption[column] = value ? "1" : "0";
    treeWrapper->node_value_set(column);
    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_float(int column, double value) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Caption[column] = Convert::ToString(value);
    treeWrapper->node_value_set(column);
    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }
}

//--------------------------------------------------------------------------------------------------

std::string TreeNodeWrapper::get_string(int column) const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return NativeToCppString(node->Caption[column]);
  }

  return "";
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::get_int(int column) const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return Convert::ToInt32(node->Caption[column]);
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::get_bool(int column) const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return (node->Caption[column] == "Checked") || (node->Caption[column] == "1") ? true : false;
  }

  return false;
}

//--------------------------------------------------------------------------------------------------

std::int64_t TreeNodeWrapper::get_long(int column) const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return Convert::ToInt64(node->Caption[column]);
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

double TreeNodeWrapper::get_float(int column) const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return Convert::ToDouble(node->Caption[column]);
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

int TreeNodeWrapper::count() const {
  return nativeNodeAdv->Children->Count;
}

//--------------------------------------------------------------------------------------------------

Bitmap ^ TreeNodeWrapper::get_cached_icon(const std::string &icon_id) {
  Bitmap ^ icon;

  String ^ str_icon = CppStringToNative(icon_id);

  if (!TreeViewNode::iconStorage.ContainsKey(str_icon)) {
    String ^ path = CppStringToNative(mforms::App::get()->get_resource_path(icon_id));
    if (File::Exists(path))
      TreeViewNode::iconStorage[str_icon] = gcnew Bitmap(path);
  }

  if (TreeViewNode::iconStorage.ContainsKey(str_icon))
    icon = TreeViewNode::iconStorage[str_icon];

  return icon;
}

//--------------------------------------------------------------------------------------------------

std::vector<mforms::TreeNodeRef> TreeNodeWrapper::add_node_collection(const mforms::TreeNodeCollectionSkeleton &nodes,
                                                                      int position) {
  std::vector<mforms::TreeNodeRef> result;

  if (!nodes.captions.empty()) {
    // The icon will be the same for all first level nodes.
    Bitmap ^ icon = nullptr;
    if (!nodes.icon.empty())
      icon = get_cached_icon(nodes.icon);

    std::vector<TreeNodeWrapper> parents;
    for (size_t i = 0; i < nodes.captions.size(); ++i) {
      MySQL::Forms::TreeViewNode ^ child = gcnew MySQL::Forms::TreeViewNode();
      TreeNodeAdv ^ treeNode;
      if (position < 0) {
        nativeNode->Nodes->Add(child);
        treeNode = nativeNodeAdv->Children[nativeNodeAdv->Children->Count - 1];
      } else {
        nativeNode->Nodes->Insert(position, child);
        treeNode = nativeNodeAdv->Children[position];
      }

      child->Caption[0] = CppStringToNative(nodes.captions[i]);
      child->Icon[0] = icon;

      TreeNodeWrapper *nodeWrapper = new TreeNodeWrapper(treeWrapper, treeNode);
      parents.push_back(*nodeWrapper);
      result.push_back(mforms::TreeNodeRef(nodeWrapper));
    }

    // Now add the child nodes.
    if (!nodes.children.empty())
      add_children_from_skeletons(parents, nodes.children);

    treeWrapper->GetManagedObject<TreeViewAdv>()->Invalidate();
  }

  return result;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Adds the same list of child nodes (as given in the children vector) to each node in the parents vector.
 */
void TreeNodeWrapper::add_children_from_skeletons(std::vector<TreeNodeWrapper> parents,
                                                  const std::vector<mforms::TreeNodeSkeleton> &children) {
  for (size_t child_index = 0; child_index < children.size(); ++child_index) {
    String ^ caption = CppStringToNative(children[child_index].caption);
    Bitmap ^ icon = get_cached_icon(children[child_index].icon);
    std::string tag = children[child_index].tag;

    std::vector<TreeNodeWrapper> child_nodes;
    for (size_t parent_index = 0; parent_index < parents.size(); ++parent_index) {
      MySQL::Forms::TreeViewNode ^ child = gcnew MySQL::Forms::TreeViewNode();
      TreeNodeAdv ^ treeNode;
      parents[parent_index].nativeNode->Nodes->Add(child);
      treeNode =
        parents[parent_index].nativeNodeAdv->Children[parents[parent_index].nativeNodeAdv->Children->Count - 1];

      child->Caption[0] = caption;
      child->Icon[0] = icon;
      child->MyTag = tag;

      TreeNodeWrapper nodeWrapper(treeWrapper, treeNode);
      child_nodes.push_back(nodeWrapper);
    }

    // For each created node insert child nodes according to the children's child list.
    if (!children[child_index].children.empty())
      add_children_from_skeletons(child_nodes, children[child_index].children);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 *	The compiler doesn't allow to assign that value from outside (another wrapper instance),
 *	so take a detour via an assignment function.
 */
void TreeNodeWrapper::node_changed(TreeNodeAdv ^ new_node) {
  nativeNodeAdv = new_node;
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::insert_child(int index) {
  // Insert node into the model, which will implicitly create a tree node.
  MySQL::Forms::TreeViewNode ^ child = gcnew MySQL::Forms::TreeViewNode();
  TreeNodeAdv ^ treeNode;
  if (index < 0) {
    nativeNode->Nodes->Add(child);
    treeNode = nativeNodeAdv->Children[nativeNodeAdv->Children->Count - 1];
  } else {
    nativeNode->Nodes->Insert(index, child);
    treeNode = nativeNodeAdv->Children[index];
  }

  return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, treeNode));
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::insert_child(int index, const mforms::TreeNode &child) {
  // Inserting an existing node only works if both belong to the same tree.
  TreeNodeWrapper *wrapper = (TreeNodeWrapper *)&child;
  if (treeWrapper != wrapper->treeWrapper)
    return;

  TreeNodeAdv ^ treeNode;
  if (index < 0) {
    nativeNode->Nodes->Add(wrapper->nativeNode);
    treeNode = nativeNodeAdv->Children[nativeNodeAdv->Children->Count - 1];
  } else {
    nativeNode->Nodes->Insert(index, wrapper->nativeNode);
    treeNode = nativeNodeAdv->Children[index];
  }

  wrapper->node_changed(treeNode);
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::remove_from_parent() {
  if (!isRoot && nativeNode->Parent != nullptr) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    if (!node->MyTag.empty())
      treeWrapper->process_mapping(nullptr, node->MyTag);

    node->Parent->Nodes->Remove(nativeNode);
  }
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::remove_children() {
  for each(Node ^ child in nativeNode->Nodes) {
      TreeViewNode ^ node = dynamic_cast<TreeViewNode ^>(child);
      if (node != nullptr && !node->MyTag.empty())
        treeWrapper->process_mapping(nullptr, node->MyTag);
    }
  nativeNode->Nodes->Clear();
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::move_node(mforms::TreeNodeRef node, bool before) {
  // Nodes must belong to the same tree.
  TreeNodeWrapper *wrapper = (TreeNodeWrapper *)node.ptr();
  if (treeWrapper != wrapper->treeWrapper)
    return;

  TreeViewNode ^ thisNode = (TreeViewNode ^)(Node ^)nativeNode;
  std::string tag = thisNode->MyTag;

  // Remove this node from parent and *after* that determine the target index, as this might
  // change when this and the other node have a common parent.
  remove_from_parent();
  int targetIndex = wrapper->nativeNode->Index;
  if (!before)
    ++targetIndex;

  wrapper->nativeNode->Parent->Nodes->Insert(targetIndex, nativeNode);

  // Update also the *Adv node as this has been recreated by the code above.
  nativeNodeAdv = wrapper->nativeNodeAdv->Parent->Children[targetIndex];
  treeWrapper->process_mapping(nativeNodeAdv, tag);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::get_child(int index) const {
  TreeNodeAdv ^ child = nativeNodeAdv->Children[index];
  if (child != nullptr)
    return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, child));

  return mforms::TreeNodeRef();
}

//--------------------------------------------------------------------------------------------------

int MySQL::Forms::TreeNodeWrapper::get_child_index(mforms::TreeNodeRef node) const {
  const TreeNodeWrapper *wrapper = dynamic_cast<const TreeNodeWrapper *>(node.ptr());
  return nativeNode->Nodes->IndexOf(wrapper->nativeNode);
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::get_parent() const {
  if (!isRoot) {
    TreeNodeAdv ^ parent = nativeNodeAdv->Parent;
    if (parent != nullptr && parent->Index > -1) // The hidden root node has an index of -1;
      return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, parent));
    else
      return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper));
  }

  return mforms::TreeNodeRef();
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::previous_sibling() const {
  if (isRoot || nativeNodeAdv->Index == 0)
    return mforms::TreeNodeRef();

  TreeNodeAdv ^ node = nativeNodeAdv->Parent->Children[nativeNodeAdv->Index - 1];
  return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, node));
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeRef TreeNodeWrapper::next_sibling() const {
  if (isRoot || nativeNodeAdv->Index == nativeNodeAdv->Parent->Children->Count - 1)
    return mforms::TreeNodeRef();

  TreeNodeAdv ^ node = nativeNodeAdv->Parent->Children[nativeNodeAdv->Index + 1];
  return mforms::TreeNodeRef(new TreeNodeWrapper(treeWrapper, node));
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::expand() {
  if (!isRoot)
    get_parent()->expand(); // Recursively expand all parent nodes.
  nativeNodeAdv->Expand();
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::collapse() {
  nativeNodeAdv->Collapse();
}

//--------------------------------------------------------------------------------------------------

bool TreeNodeWrapper::is_expanded() {
  return nativeNodeAdv->IsExpanded;
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_tag(const std::string &tag) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    if (!node->MyTag.empty())
      treeWrapper->process_mapping(nullptr, node->MyTag);
    node->MyTag = tag;
    treeWrapper->process_mapping(nativeNodeAdv, node->MyTag);
  }
}

//--------------------------------------------------------------------------------------------------

std::string TreeNodeWrapper::get_tag() const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return node->MyTag;
  }

  return "";
}

//--------------------------------------------------------------------------------------------------

void TreeNodeWrapper::set_data(mforms::TreeNodeData *data) {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    node->Data = data;
  }
}

//--------------------------------------------------------------------------------------------------

mforms::TreeNodeData *TreeNodeWrapper::get_data() const {
  if (!isRoot) {
    TreeViewNode ^ node = (TreeViewNode ^)(Node ^)nativeNode;
    return node->Data;
  }

  return NULL;
}

//--------------------------------------------------------------------------------------------------
