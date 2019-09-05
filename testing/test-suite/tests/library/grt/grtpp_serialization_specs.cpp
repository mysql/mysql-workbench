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

#include "structs.test.h"

#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.mysql.h"

#include "grt_test_helpers.h"
#include "wb_test_helpers.h"
#include "casmine.h"

using namespace grt;
using namespace casmine;

extern void register_all_metaclasses();

namespace {

$ModuleEnvironment() {};

$TestData {
  std::string dataDir;
  std::string outputDir;

  void runSerialization(const ValueRef& val) {
    static const std::string filename(outputDir + "/serialization_test.xml");
    GRT::get()->serialize(val, filename);
    ValueRef res_val(GRT::get()->unserialize(filename));
    deepCompareGrtValues("serialization test", res_val, val, true);
  }
};

$describe("GRT: serialization") {

  $beforeAll([this] () {
    data->dataDir = CasmineContext::get()->tmpDataDir();
    data->outputDir = CasmineContext::get()->outputDir();
    register_all_metaclasses();
    register_structs_test_xml();
    grt::GRT::get()->load_metaclasses(data->dataDir + "/structs.test.xml");

    $expect(GRT::get()->get_metaclasses().size()).toEqual(6U);

    grt::GRT::get()->scan_metaclasses_in("../../res/grt/");
    grt::GRT::get()->end_loading_metaclasses();
  });

  $afterAll([&]() {
    WorkbenchTester::reinitGRT();
  });

  $it("Serialization of simple values + dictionaries", [this]() {
    StringRef sv("<tag1>%string_value/</tag1>");
    IntegerRef iv(-1);
    DoubleRef dv(1.12345678901234);

    // Simple values.
    data->runSerialization(sv);
    data->runSerialization(iv);
    data->runSerialization(dv);
    data->runSerialization(ValueRef(sv));
    data->runSerialization(ValueRef(iv));
    data->runSerialization(ValueRef(dv));

    // Object values.
    const size_t AUTHORS_COUNT(3);
    test_BookRef obj(grt::Initialized);

    obj->set_member("title", sv);
    obj->set_member("pages", iv);

    test_PublisherRef publisher(grt::Initialized);
    publisher->set_member("name", sv);
    publisher->set_member("phone", StringRef(((std::string)sv) + " 555-55-55"));
    obj->set_member("publisher", publisher);

    std::stringstream ss;
    for (size_t n = 0; n < AUTHORS_COUNT; n++) {
      test_AuthorRef author(grt::Initialized);
      ss << n;
      author->set_member("name", StringRef(std::string("Author") + ss.str().c_str()));
      obj->authors().insert(author);
    }

    obj->set_member("price", DoubleRef(dv + 1.1));

    test_AuthorRef author(grt::Initialized);
    DictRef extras(DictRef::cast_from(obj->get_member("extras")));
    extras.set("extra_string", sv);
    extras.set("extra_int", iv);
    extras.set("extra_double", dv);
    extras.set("extra_obj", author);

    data->runSerialization(obj);
  });

  $it("Serialization of a hierarchy", [this]() {
    ObjectListRef list(grt::Initialized);

    test_BookRef book1(grt::Initialized);
    test_BookRef book2(grt::Initialized);
    test_AuthorRef author(grt::Initialized);

    author->name("the author");

    book1->title("the book1");
    book1->authors().insert(author);

    book2->title("the book2");
    book2->authors().insert(author);

    list.insert(book1);
    list.insert(book2);

    data->runSerialization(list);
  });

  $it("Catalog serialization", [this]() {
    auto catalog(db_mysql_CatalogRef::cast_from(grt::GRT::get()->unserialize(data->dataDir + "/serialization/catalog.xml")));

    ObjectRef owner = catalog->schemata().get(0)->tables().get(0)->indices().get(0)->owner();
    $expect(owner.valueptr()).Not.toBeNull();
    $expect(catalog->schemata().get(0)->tables().get(0).valueptr()).toEqual(owner.valueptr());
  });

  $it("Serialization of lists with NULL values", [this]() {
    grt::ListRef<db_Table> list(true);

    list.insert(db_TableRef(grt::Initialized));
    list.insert(db_TableRef());
    list.insert(db_TableRef(grt::Initialized));

    grt::GRT::get()->serialize(list, data->outputDir + "/null_list.xml");

    list = grt::ListRef<db_Table>::cast_from(grt::GRT::get()->unserialize(data->outputDir + "/null_list.xml"));

    $expect(list[0].is_valid()).toBeTrue();
    $expect(list[1].is_valid()).toBeFalse();
    $expect(list[2].is_valid()).toBeTrue();
  });

#ifdef badtest
  $it("", [this]() {
    // "dontfollow" means the object will be saved as a link, not that it won't be saved at all.

    static const std::string filename("output/serialization_test.xml");

    {
      db_CatalogRef catalog(grt::Initialized);
      ListRef<db_SimpleDatatype> datatypes;
      catalog.simpleDatatypes(datatypes);
      db_SimpleDatatypeRef datatype(grt::Initialized);
      datatypes.insert(datatype);
      grt::GRT::get()->serialize(catalog, filename); // the only set attr simpleDatatypes shouldn't be serialized
    }

    // now compare with empty catalog
    db_CatalogRef catalog(grt::Initialized);
    ValueRef res_catalog(grt::GRT::get()->unserialize(filename));
    deepCompareGrtValues("Check attr:dontfollow=\"1\"", res_catalog, catalog);
  });
#endif

}
}
