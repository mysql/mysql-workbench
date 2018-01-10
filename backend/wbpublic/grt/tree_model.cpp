/*
 * Copyright (c) 2007, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "tree_model.h"
#include <grtpp_util.h>
#include <iomanip>

using namespace grt;
using namespace bec;

NodeId::NodeId() {
}

//--------------------------------------------------------------------------------------------------

NodeId::NodeId(const NodeId &copy) {
  index = copy.index;
}

//--------------------------------------------------------------------------------------------------

NodeId::NodeId(size_t i) {
  index.push_back(i);
}

//--------------------------------------------------------------------------------------------------

NodeId::NodeId(const std::string &str) {
  try {
    const char *chr = str.c_str();
    size_t size = str.length();
    std::string num;
    num.reserve(size);

    for (size_t i = 0; i < size; i++) {
      if (isdigit(chr[i]))
        num.push_back(chr[i]);
      else if ('.' == chr[i] || ':' == chr[i]) {
        if (!num.empty()) {
          index.push_back(base::atoi<int>(num, 0));
          num.clear();
        }
      } else
        throw std::runtime_error("Wrong format of NodeId");
    }

    if (!num.empty())
      index.push_back(base::atoi<int>(num, 0));
  } catch (...) {
    index.clear();
    throw;
  }
}

//--------------------------------------------------------------------------------------------------

NodeId::~NodeId() {
  index.clear();
}

//--------------------------------------------------------------------------------------------------

bool NodeId::operator<(const NodeId &r) const {
  bool ret = true;

  // Shorter node ids must go before longer. For example in a list ["0.1", "0.1.1"]
  // longer nodeid is a subnode of the "0.1", so in case of deletion subnode deleted first
  // (That's true only when traversing list from the end)
  if (index.size() < r.index.size())
    ret = true;
  else if (index.size() > r.index.size())
    ret = false;
  else {
    // It is assumed that this node id is less than @r. Walk index vectors. If current value
    // from this->index is less than or equal to the corresponding value from r.index the pair is skipped
    // as it complies with assumption that this node is less than @r.
    // Once current value becomes greater than @r's the assumption about current node's
    // less than @r becomes false, therefore this node is greater than @r.
    for (size_t i = 0; i < index.size(); ++i) {
      if (index[i] >= r.index[i]) {
        ret = false;
        break;
      }
    }
  }

  return ret;
}

//--------------------------------------------------------------------------------------------------

bool NodeId::equals(const NodeId &node) const {
  // TODO: Check if we need to compare content of the index and node.index vectors
  return node.index == index;
}

//--------------------------------------------------------------------------------------------------

size_t &NodeId::operator[](size_t i) {
  if (i < index.size())
    return index.at(i);
  throw std::range_error("invalid index");
}

//--------------------------------------------------------------------------------------------------

const size_t &NodeId::operator[](size_t i) const {
  if (i < index.size())
    return index.at(i);
  throw std::range_error("invalid index");
}

//--------------------------------------------------------------------------------------------------

size_t NodeId::end() const {
  if (!index.empty())
    return index.back();
  throw std::logic_error("invalid node id. NodeId::end applied to an empty NodeId instance.");
}

//--------------------------------------------------------------------------------------------------

/**
 * Sets leaf to the previous index, e.g. for node with path "1.3.2" it will become "1.3.1".
 */
bool NodeId::previous() {
  bool ret = false;
  if (!index.empty()) {
    --index.back();
    ret = true;
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

/**
 *	Sets leaf to the next index, e.g. for node with path "1.3.2" it will become "1.3.3".
 */
bool NodeId::next() {
  bool ret = false;
  if (!index.empty()) {
    ++index.back();
    ret = true;
  }
  return ret;
}

//--------------------------------------------------------------------------------------------------

NodeId NodeId::parent() const {
  if (depth() < 2)
    return NodeId();
  NodeId copy(*this);
  copy.index.pop_back();
  return copy;
}

//--------------------------------------------------------------------------------------------------

std::string NodeId::description() const {
  return toString();
}

//--------------------------------------------------------------------------------------------------

std::string NodeId::toString(const char separator) const {
  std::stringstream out;
  for (size_t i = 0; i < index.size(); i++) {
    if (i > 0)
      out << separator;
    out << index[i];
  }
  return out.str();
}

//--------------------------------------------------------------------------------------------------

NodeId &NodeId::append(size_t i) {
  // TODO: does it really make sense to cast to a signed type if the parameter can only be unsigned?
  //       It would require that the caller takes a signed value and casts it to an unsigned one.
  //       Very unlikely to happen.
  //       If these checks are removed we can make append and prepend inline again.
  if ((ssize_t)i < 0)
    throw std::invalid_argument("negative node index is invalid");
  index.push_back(i);
  return *this;
}

//--------------------------------------------------------------------------------------------------

NodeId &NodeId::prepend(size_t i) {
  if ((ssize_t)i < 0)
    throw std::invalid_argument("negative node index is invalid");
  index.insert(index.begin(), i);
  return *this;
}

//----------------- ListModel ----------------------------------------------------------------------

Type ListModel::get_field_type(const NodeId &node, ColumnId column) {
  throw std::logic_error("not implemented");
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(const NodeId &node, ColumnId column, std::string &value) {
  ValueRef v;

  if (!get_field_grt(node, column, v))
    return false;

  value = v.toString();

  return true;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(const NodeId &node, ColumnId column, ssize_t &value) {
  ValueRef v(0);

  if (!get_field_grt(node, column, v))
    return false;

  switch (v.type()) {
    case IntegerType:
      value = IntegerRef::cast_from(v);
      return true;

    default:
      return false;
  }
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(const NodeId &node, ColumnId column, bool &value) {
  ssize_t i;
  if (!get_field(node, column, i))
    return false;

  value = (i != 0);
  return true;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::get_field(const NodeId &node, ColumnId column, double &value) {
  ValueRef v(0);

  if (!get_field_grt(node, column, v))
    return false;

  switch (v.type()) {
    case DoubleType:
      value = DoubleRef::cast_from(v);
      return true;

    case IntegerType:
      value = (double)(int)IntegerRef::cast_from(v);
      return true;

    default:
      return false;
  }
}

//--------------------------------------------------------------------------------------------------

ValueRef ListModel::get_grt_value(const NodeId &node, ColumnId column) {
  ValueRef value;

  get_field_grt(node, column, value);

  return value;
}

//--------------------------------------------------------------------------------------------------

std::string ListModel::get_field_description(const NodeId &node, ColumnId column) {
  return "";
}

//--------------------------------------------------------------------------------------------------

IconId ListModel::get_field_icon(const NodeId &node, ColumnId column, IconSize size) {
  return 0;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_field(const NodeId &node, ColumnId column, double value) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::set_convert_field(const NodeId &node, ColumnId column, const std::string &value) {
  try {
    switch (get_field_type(node, column)) {
      case IntegerType:
        return set_field(node, column, base::atoi<ssize_t>(value));

      case DoubleType:
        return set_field(node, column, base::atof<double>(value));

      case StringType:
        return set_field(node, column, value);

      default:
        return false;
    }
  } catch (...) {
    return false;
  }

  return true;
}

//--------------------------------------------------------------------------------------------------

NodeId ListModel::get_node(size_t index) {
  return index;
}

//--------------------------------------------------------------------------------------------------

bool ListModel::has_next(const NodeId &node) {
  return node[0] + 1 < count();
}

//--------------------------------------------------------------------------------------------------

NodeId ListModel::get_next(const NodeId &node) {
  if (node[0] + 1 < count())
    return node[0] + 1;
  throw std::out_of_range("invalid child");
}

//--------------------------------------------------------------------------------------------------

ValueRef ListModel::parse_value(Type type, const std::string &value) {
  switch (type) {
    case IntegerType:
      try {
        return IntegerRef(base::atoi<ssize_t>(value));
      } catch (...) {
        break;
      }

    case DoubleType:
      try {
        return DoubleRef(base::atof<double>(value));
      } catch (...) {
        break;
      }

    case AnyType:
    case StringType:
      return StringRef(value);

    default:
      break;
  }
  return ValueRef();
}

//--------------------------------------------------------------------------------------------------

/**
 * Move the given node one index down in this list.
 */
void ListModel::reorder_up(const NodeId &node) {
  if (node.end() > 0)
    reorder(node, node.end() - 1);
}

//--------------------------------------------------------------------------------------------------

/**
 * Move the given node one index up in this list.
 */
void ListModel::reorder_down(const NodeId &node) {
  reorder(node, node.end() + 1);
}

//--------------------------------------------------------------------------------------------------

void ListModel::dump(int show_field) {
  g_print("\nDumping list model:\n");
  for (size_t i = 0, c = count(); i < c; i++) {
    NodeId child(i);
    std::string value;

    if (!get_field(child, show_field, value))
      value = "???";

    g_print("- %s\n", value.c_str());
  }
  g_print("\nFinished dumping list model.");
}

//----------------- TreeModel ----------------------------------------------------------------------

size_t TreeModel::count() {
  return count_children(get_root());
}

//--------------------------------------------------------------------------------------------------

NodeId TreeModel::get_node(size_t index) {
  return get_child(get_root(), index);
}

//--------------------------------------------------------------------------------------------------

bool TreeModel::has_next(const NodeId &node) {
  NodeId parent(get_parent(node));

  return node.end() < count_children(parent) - 1;
}

//--------------------------------------------------------------------------------------------------

NodeId TreeModel::get_next(const NodeId &node) {
  if (node.depth() < 2)
    return ListModel::get_next(node);
  else {
    NodeId parent(get_parent(node));

    if (node.end() < count_children(parent) - 1)
      return parent.append(node.end() + 1);

    throw std::out_of_range("last node");
  }
}

//--------------------------------------------------------------------------------------------------

size_t TreeModel::get_node_depth(const NodeId &node) {
  return node.depth();
}

//--------------------------------------------------------------------------------------------------

NodeId TreeModel::get_root() const {
  return NodeId();
}

//--------------------------------------------------------------------------------------------------

bool TreeModel::expand_node(const NodeId &node) {
  return false;
}

//--------------------------------------------------------------------------------------------------

void TreeModel::collapse_node(const NodeId &node) {
}

//--------------------------------------------------------------------------------------------------

bool TreeModel::is_expanded(const NodeId &node) {
  return false;
}

//--------------------------------------------------------------------------------------------------

bool TreeModel::is_expandable(const NodeId &node_id) {
  return count_children(node_id) > 0;
}

//--------------------------------------------------------------------------------------------------

static void dump_node(TreeModel *model, int show_field, const NodeId &node_id) {
  for (size_t i = 0, c = model->count_children(node_id); i < c; i++) {
    NodeId child = model->get_child(node_id, i);
    std::string value;
    char *left = (char *)"-"; // Cast needed to avoid deprecation warning with GCC 4.3

    if (!model->get_field(child, show_field, value))
      value = "???";

    if (model->is_expandable(node_id))
      left = (char *)"+";

    std::stringstream ss;
    ss << std::setw((int)child.depth());
    ss << left;
    g_print("%s %s\n", ss.str().c_str(), value.c_str());

    dump_node(model, show_field, child);
  }
}

//--------------------------------------------------------------------------------------------------

void TreeModel::dump(int show_field) {
  g_print("\nDumping tree model:\n");
  dump_node(this, show_field, NodeId());
  g_print("\nFinished dumping tree model.");
}

//--------------------------------------------------------------------------------------------------
