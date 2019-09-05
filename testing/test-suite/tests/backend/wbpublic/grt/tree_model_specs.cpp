/*
 * Copyright (c) 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt/tree_model.h"

#include "casmine.h"
#include "wb_test_helpers.h"

using namespace grt;
using namespace bec;

namespace {

$ModuleEnvironment() {};

$describe("grt tree model base") {

  $it("Base tests", []() {
    NodeId node, node2;

    $expect(node.is_valid()).toBeFalse("clean node");

    $expect(node.depth()).toEqual(0U, "clean node depth");

    node = NodeId(5);
    $expect(node.is_valid()).toBeTrue("node(5)");
    $expect(node.depth()).toEqual(1U, "node(5).depth()");
    $expect(node[0]).toEqual(5U, "node(5)[0]");

    node2 = node.append(7);
    $expect(node.depth()).toEqual(2U, "node append");
    $expect(node[0]).toEqual(5U, "node append[0]");
    $expect(node[1]).toEqual(7U, "node append[1]");

    $expect(node2.depth()).toEqual(2U, "node append ret");
    $expect(node2[0]).toEqual(5U, "node append ret[0]");
    $expect(node2[1]).toEqual(7U, "node append ret[1]");

    $expect(node).toEqual(node2, "node compare");

    node2 = NodeId(5);
    $expect(node).Not.toEqual(node2, "node compare");

    node2.append(7);
    node2.append(11);

    $expect(node).Not.toEqual(node2, "node compare");

    node = node2;
    $expect(node).toEqual(node2, "node assign/compare");
  });

  $it("Serialization", []() {
    // serialization
    NodeId node;
    std::string s;

    s = node.toString();
    $expect(s).toEqual("", "() toString");
    $expect(NodeId(s)).toEqual(node, "() parse");

    node.append(3);
    s = node.toString();
    $expect(node).toEqual(NodeId(3), "(3) check");
    $expect(s).toEqual("3", "(3) toString");
    $expect(NodeId(s).toString()).toEqual(node.toString(), "(3) parse");

    node.append(0);
    s = node.toString();
    $expect(s).toEqual("3.0", "(3,0) toString");
    $expect(NodeId(s).toString()).toEqual(node.toString(), "(3,0) parse");

    node.append(1);
    s = node.toString();
    $expect(s).toEqual("3.0.1", "(3,0,1) toString");
    $expect(NodeId(s).toString()).toEqual(node.toString(), "(3,0,1) parse");
  });

  $it("Common tree_model methods", []() {
    $pending("needs implementation");
  });
}

}
