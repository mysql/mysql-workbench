/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/tree_model.h"
#include "wb_helpers.h"

using namespace grt;
using namespace bec;

BEGIN_TEST_DATA_CLASS(be_tree_model)
public:
END_TEST_DATA_CLASS;

TEST_MODULE(be_tree_model, "grt tree model base");

TEST_FUNCTION(2) {
  NodeId node, node2;

  ensure("clean node", !node.is_valid());

  ensure("clean node depth", node.depth() == 0);

  node = NodeId(5);
  ensure("node(5)", node.is_valid());
  ensure_equals("node(5).depth()", node.depth(), 1U);
  ensure_equals("node(5)[0]", node[0], 5U);

  node2 = node.append(7);
  ensure_equals("node append", node.depth(), 2U);
  ensure_equals("node append[0]", node[0], 5U);
  ensure_equals("node append[1]", node[1], 7U);

  ensure_equals("node append ret", node2.depth(), 2U);
  ensure_equals("node append ret[0]", node2[0], 5U);
  ensure_equals("node append ret[1]", node2[1], 7U);

  ensure("node compare", node == node2);

  node2 = NodeId(5);
  ensure("node compare", !(node == node2));

  node2.append(7);
  node2.append(11);

  ensure("node compare", !(node == node2));

  node = node2;
  ensure("node assign/compare", node == node2);
}

TEST_FUNCTION(3) {
  // serialization
  NodeId node;
  std::string s;

  s = node.toString();
  ensure_equals("() toString", s, "");
  ensure("() parse", NodeId(s) == node);

  node.append(3);
  s = node.toString();
  ensure("(3) check", node == NodeId(3));
  ensure_equals("(3) toString", s, "3");
  ensure_equals("(3) parse", NodeId(s).toString(), node.toString());

  node.append(0);
  s = node.toString();
  ensure_equals("(3,0) toString", s, "3.0");
  ensure_equals("(3,0) parse", NodeId(s).toString(), node.toString());

  node.append(1);
  s = node.toString();
  ensure_equals("(3,0,1) toString", s, "3.0.1");
  ensure_equals("(3,0,1) parse", NodeId(s).toString(), node.toString());
}

// test common tree_model methods (get_parent etc)

END_TESTS
