/* 
 * Copyright (c) 2011, Oracle and/or its affiliates. All rights reserved.
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

#include "tut_stdafx.h"

#include "testgrt.h"
#include "grt_test_utility.h"
#include "structs.test.h"
#include "grtdb/db_object_helpers.h"
#include "grts/structs.db.mysql.h"

BEGIN_TEST_DATA_CLASS(grtpp_serialization_test)
public:
  GRT grt;
END_TEST_DATA_CLASS

TEST_MODULE(grtpp_serialization_test, "GRT: serialization");

//using namespace grt;

//BEGIN_TESTS

//struct test_data {
//
//};
//
//
//typedef test_group<test_data> serializ_tg;
//typedef serializ_tg::object itgo;
//serializ_tg serialization_group("serizalization");



/*
 * - create bridged object for testing
 * - create tree with all item types
 * - repeat appearance of some item types
 * - serialize tree
 * - unserialize tree
 * - compare trees
 *   - check whether object ids continue the same
 *   - check whether referenced objects continue the same
 *   - perform self-test of bridged objects
 */

TEST_FUNCTION(1)
{
  grt.load_metaclasses("data/structs.test.xml");

  ensure_equals("load structs", grt.get_metaclasses().size(), 6U);
  grt.scan_metaclasses_in("../../res/grt/");
  grt.end_loading_metaclasses();
}

void test_serialization(GRT& grt, const ValueRef& val)
{
  static const std::string filename("serialization_test.xml");
  grt.serialize(val, filename);
  ValueRef res_val(grt.unserialize(filename));
  grt_ensure_equals(
    "serialization test",
    res_val,
    val,
    true);
}

TEST_FUNCTION(2)
{
  StringRef sv("<tag1>%string_value/</tag1>");
  IntegerRef iv(-1);
  DoubleRef dv(1.12345678901234);

  // test simple types
  test_serialization(grt, sv);
  test_serialization(grt, iv);
  test_serialization(grt, dv);
  test_serialization(grt, ValueRef(sv));
  test_serialization(grt, ValueRef(iv));
  test_serialization(grt, ValueRef(dv));

  // test object type
  //const char* OBJ_BOOK_PATH("test.Book");
  //const char* OBJ_PUBLISHER_PATH("test.Publisher");
  //const char* OBJ_AUTHOR_PATH("test.Author");
  const size_t AUTHORS_COUNT(3);

  //ObjectRef obj(grt.create_object_from_va<ObjectRef>(OBJ_BOOK_PATH, NULL));
  test_BookRef obj(&grt);
    
  obj->set_member("title", sv);
  obj->set_member("pages", iv);

  test_PublisherRef publisher(&grt);
  publisher->set_member("name", sv);
  publisher->set_member("phone", StringRef(((std::string)sv) + " 555-55-55"));
  obj->set_member("publisher", publisher);

  //ObjectRef authors_arr[AUTHORS_COUNT];
  for (size_t n= 0; n<AUTHORS_COUNT; n++)
  {
    test_AuthorRef author(&grt);
    char buf[8];
    sprintf(buf, "%i", n);
    author->set_member("name", StringRef(std::string("Author") + buf));
    obj->authors().insert(author);
  }

  obj->set_member("price", DoubleRef(dv+1.1));

  test_AuthorRef author(&grt);
  DictRef extras(DictRef::cast_from(obj->get_member("extras")));
  extras.set("extra_string", sv);
  extras.set("extra_int", iv);
  extras.set("extra_double", dv);
  extras.set("extra_obj", author);

  test_serialization(grt, obj);
}


TEST_FUNCTION(3)
{
  ObjectListRef list(&grt);

  test_BookRef book1(&grt);
  test_BookRef book2(&grt);
  test_AuthorRef author(&grt);

  author->name("the author");
  
  book1->title("the book1");
  book1->authors().insert(author);

  book2->title("the book2");
  book2->authors().insert(author);
  
  list.insert(book1);
  list.insert(book2);

  test_serialization(grt, list);
}

TEST_FUNCTION(4)
{
  db_mysql_CatalogRef catalog(db_mysql_CatalogRef::cast_from(grt.unserialize("data/serialization/catalog.xml")));

  ObjectRef owner = catalog->schemata().get(0)->tables().get(0)->indices().get(0)->owner();
  tut::ensure("Check owner set", NULL != owner.valueptr());
  
  tut::ensure("Check owner", catalog->schemata().get(0)->tables().get(0).valueptr() == owner.valueptr());
}


TEST_FUNCTION(5)
{
  // test serialization of lists with NULL values

  grt::ListRef<db_Table> list(&grt);

  list.insert(db_TableRef(&grt));
  list.insert(db_TableRef());
  list.insert(db_TableRef(&grt));

  grt.serialize(list, "null_list.xml");

  list= grt::ListRef<db_Table>::cast_from(grt.unserialize("null_list.xml"));

  ensure("list[0]", list[0].is_valid());
  ensure("list[1]", list[1].is_valid()==false);
  ensure("list[2]", list[2].is_valid());
}


#ifdef badtest
TEST_FUNCTION(5)
{
  // dontfollow means the object will be saved as a link, not that it wont be saved

  static const std::string filename("serialization_test.xml");

  {
    db_CatalogRef catalog(&grt);
    ListRef<db_SimpleDatatype> datatypes(&grt);
    catalog.simpleDatatypes(datatypes);
    db_SimpleDatatypeRef datatype(&grt);
    datatypes.insert(datatype);
    grt.serialize(catalog, filename); // the only set attr simpleDatatypes shouldn't be serialized
  }

  // now compare with empty catalog
  db_CatalogRef catalog(&grt);
  ValueRef res_catalog(grt.unserialize(filename));
  grt_ensure_equals(
    "Check attr:dontfollow=\"1\"",
    res_catalog,
    catalog);
}
#endif


END_TESTS
