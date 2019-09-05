/*
 * Copyright (c) 2011, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "grt.h"

#ifdef _MSC_VER
#pragma warning(disable : 4355) // 'this' : used in base member initizalizer list
#ifdef GRT_STRUCTS_TEST_EXPORT
#define GRT_STRUCTS_TEST_PUBLIC __declspec(dllexport)
#else
#define GRT_STRUCTS_TEST_PUBLIC __declspec(dllimport)
#endif
#else
#define GRT_STRUCTS_TEST_PUBLIC
#endif

class test_Bridged;
typedef grt::Ref<test_Bridged> test_BridgedRef;
class test_Base;
typedef grt::Ref<test_Base> test_BaseRef;
class test_Publisher;
typedef grt::Ref<test_Publisher> test_PublisherRef;
class test_Author;
typedef grt::Ref<test_Author> test_AuthorRef;
class test_Publication;
typedef grt::Ref<test_Publication> test_PublicationRef;
class test_Book;
typedef grt::Ref<test_Book> test_BookRef;

namespace mforms {
  class Object;
};

namespace grt {
  class AutoPyObject;
};

class test_Bridged : public grt::internal::Object {
  typedef grt::internal::Object super;

public:
  test_Bridged(grt::MetaClass *meta = 0)
    : grt::internal::Object(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _books(this, false),
      _name(""),
      _x(0),
      _y(0)

  {
  }

  static std::string static_class_name() {
    return "test.Bridged";
  }

  /** Getter for attribute books (read-only)


   \par In Python:
value = obj.books
   */
  grt::ListRef<test_Book> books() const {
    return _books;
  }

private: // the next attribute is read-only
  virtual void books(const grt::ListRef<test_Book> &value) {
    grt::ValueRef ovalue(_books);
    _books = value;
    member_changed("books", ovalue, value);
  }

public:
  /** Getter for attribute name


   \par In Python:
value = obj.name
   */
  grt::StringRef name() const {
    return _name;
  }
  /** Setter for attribute name


    \par In Python:
obj.name = value
   */
  virtual void name(const grt::StringRef &value) {
    grt::ValueRef ovalue(_name);
    _name = value;
    member_changed("name", ovalue, value);
  }

  /** Getter for attribute x


   \par In Python:
value = obj.x
   */
  grt::IntegerRef x() const {
    return _x;
  }
  /** Setter for attribute x


    \par In Python:
obj.x = value
   */
  virtual void x(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_x);
    _x = value;
    member_changed("x", ovalue, value);
  }

  /** Getter for attribute y


   \par In Python:
value = obj.y
   */
  grt::IntegerRef y() const {
    return _y;
  }
  /** Setter for attribute y


    \par In Python:
obj.y = value
   */
  virtual void y(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_y);
    _y = value;
    member_changed("y", ovalue, value);
  }

protected:
  grt::ListRef<test_Book> _books;
  grt::StringRef _name;
  grt::IntegerRef _x;
  grt::IntegerRef _y;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new test_Bridged);
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&test_Bridged::create);
    {
      void (test_Bridged::*setter)(const grt::ListRef<test_Book> &) = &test_Bridged::books;
      grt::ListRef<test_Book> (test_Bridged::*getter)() const = &test_Bridged::books;
      meta->bind_member("books", new grt::MetaClass::Property<test_Bridged, grt::ListRef<test_Book> >(getter, setter));
    }
    {
      void (test_Bridged::*setter)(const grt::StringRef &) = &test_Bridged::name;
      grt::StringRef (test_Bridged::*getter)() const = &test_Bridged::name;
      meta->bind_member("name", new grt::MetaClass::Property<test_Bridged, grt::StringRef>(getter, setter));
    }
    {
      void (test_Bridged::*setter)(const grt::IntegerRef &) = &test_Bridged::x;
      grt::IntegerRef (test_Bridged::*getter)() const = &test_Bridged::x;
      meta->bind_member("x", new grt::MetaClass::Property<test_Bridged, grt::IntegerRef>(getter, setter));
    }
    {
      void (test_Bridged::*setter)(const grt::IntegerRef &) = &test_Bridged::y;
      grt::IntegerRef (test_Bridged::*getter)() const = &test_Bridged::y;
      meta->bind_member("y", new grt::MetaClass::Property<test_Bridged, grt::IntegerRef>(getter, setter));
    }
  }
};

class test_Base : public grt::internal::Object {
  typedef grt::internal::Object super;

public:
  test_Base(grt::MetaClass *meta = 0)
    : grt::internal::Object(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name()))

  {
  }

  static std::string static_class_name() {
    return "test.Base";
  }

protected:
private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new test_Base);
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&test_Base::create);
  }
};

class test_Publisher : public test_Base {
  typedef test_Base super;

public:
  test_Publisher(grt::MetaClass *meta = 0)
    : test_Base(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _books(this, false),
      _name(""),
      _phone("")

  {
  }

  static std::string static_class_name() {
    return "test.Publisher";
  }

  // books is owned by test_Publisher
  /** Getter for attribute books (read-only)


   \par In Python:
value = obj.books
   */
  grt::ListRef<test_Book> books() const {
    return _books;
  }

private: // the next attribute is read-only
  virtual void books(const grt::ListRef<test_Book> &value) {
    grt::ValueRef ovalue(_books);

    _books = value;
    owned_member_changed("books", ovalue, value);
  }

public:
  /** Getter for attribute name

    name of the publisher
   \par In Python:
value = obj.name
   */
  grt::StringRef name() const {
    return _name;
  }
  /** Setter for attribute name

    name of the publisher
    \par In Python:
obj.name = value
   */
  virtual void name(const grt::StringRef &value) {
    grt::ValueRef ovalue(_name);
    _name = value;
    member_changed("name", ovalue, value);
  }

  /** Getter for attribute phone

    phone of the publisher
   \par In Python:
value = obj.phone
   */
  grt::StringRef phone() const {
    return _phone;
  }
  /** Setter for attribute phone

    phone of the publisher
    \par In Python:
obj.phone = value
   */
  virtual void phone(const grt::StringRef &value) {
    grt::ValueRef ovalue(_phone);
    _phone = value;
    member_changed("phone", ovalue, value);
  }

protected:
  grt::ListRef<test_Book> _books; // owned
  grt::StringRef _name;
  grt::StringRef _phone;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new test_Publisher);
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&test_Publisher::create);
    {
      void (test_Publisher::*setter)(const grt::ListRef<test_Book> &) = &test_Publisher::books;
      grt::ListRef<test_Book> (test_Publisher::*getter)() const = &test_Publisher::books;
      meta->bind_member("books",
                        new grt::MetaClass::Property<test_Publisher, grt::ListRef<test_Book> >(getter, setter));
    }
    {
      void (test_Publisher::*setter)(const grt::StringRef &) = &test_Publisher::name;
      grt::StringRef (test_Publisher::*getter)() const = &test_Publisher::name;
      meta->bind_member("name", new grt::MetaClass::Property<test_Publisher, grt::StringRef>(getter, setter));
    }
    {
      void (test_Publisher::*setter)(const grt::StringRef &) = &test_Publisher::phone;
      grt::StringRef (test_Publisher::*getter)() const = &test_Publisher::phone;
      meta->bind_member("phone", new grt::MetaClass::Property<test_Publisher, grt::StringRef>(getter, setter));
    }
  }
};

class test_Author : public test_Base {
  typedef test_Base super;

public:
  test_Author(grt::MetaClass *meta = 0)
    : test_Base(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _name("")

  {
  }

  static std::string static_class_name() {
    return "test.Author";
  }

  /** Getter for attribute name

    name of the author
   \par In Python:
value = obj.name
   */
  grt::StringRef name() const {
    return _name;
  }
  /** Setter for attribute name

    name of the author
    \par In Python:
obj.name = value
   */
  virtual void name(const grt::StringRef &value) {
    grt::ValueRef ovalue(_name);
    _name = value;
    member_changed("name", ovalue, value);
  }

protected:
  grt::StringRef _name;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new test_Author);
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&test_Author::create);
    {
      void (test_Author::*setter)(const grt::StringRef &) = &test_Author::name;
      grt::StringRef (test_Author::*getter)() const = &test_Author::name;
      meta->bind_member("name", new grt::MetaClass::Property<test_Author, grt::StringRef>(getter, setter));
    }
  }
};

class test_Publication : public test_Base {
  typedef test_Base super;

public:
  test_Publication(grt::MetaClass *meta = 0)
    : test_Base(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _title("")

  {
  }

  static std::string static_class_name() {
    return "test.Publication";
  }

  /** Getter for attribute title

    title of the book
   \par In Python:
value = obj.title
   */
  grt::StringRef title() const {
    return _title;
  }
  /** Setter for attribute title

    title of the book
    \par In Python:
obj.title = value
   */
  virtual void title(const grt::StringRef &value) {
    grt::ValueRef ovalue(_title);
    _title = value;
    member_changed("title", ovalue, value);
  }

protected:
  grt::StringRef _title;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new test_Publication);
  }

public:
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&test_Publication::create);
    {
      void (test_Publication::*setter)(const grt::StringRef &) = &test_Publication::title;
      grt::StringRef (test_Publication::*getter)() const = &test_Publication::title;
      meta->bind_member("title", new grt::MetaClass::Property<test_Publication, grt::StringRef>(getter, setter));
    }
  }
};

class test_Book : public test_Publication {
  typedef test_Publication super;

public:
  test_Book(grt::MetaClass *meta = 0)
    : test_Publication(meta ? meta : grt::GRT::get()->get_metaclass(static_class_name())),
      _authors(this, false),
      _extras(this, false),
      _pages(0),
      _price(0.0)

  {
  }

  static std::string static_class_name() {
    return "test.Book";
  }

  // authors is owned by test_Book
  /** Getter for attribute authors (read-only)

    the list of authors
   \par In Python:
value = obj.authors
   */
  grt::ListRef<test_Author> authors() const {
    return _authors;
  }

private: // the next attribute is read-only
  virtual void authors(const grt::ListRef<test_Author> &value) {
    grt::ValueRef ovalue(_authors);

    _authors = value;
    owned_member_changed("authors", ovalue, value);
  }

public:
  /** Getter for attribute extras (read-only)

    extra stuff
   \par In Python:
value = obj.extras
   */
  grt::DictRef extras() const {
    return _extras;
  }

private: // the next attribute is read-only
  virtual void extras(const grt::DictRef &value) {
    grt::ValueRef ovalue(_extras);
    _extras = value;
    member_changed("extras", ovalue, value);
  }

public:
  /** Getter for attribute pages

    number of pages in book
   \par In Python:
value = obj.pages
   */
  grt::IntegerRef pages() const {
    return _pages;
  }
  /** Setter for attribute pages

    number of pages in book
    \par In Python:
obj.pages = value
   */
  virtual void pages(const grt::IntegerRef &value) {
    grt::ValueRef ovalue(_pages);
    _pages = value;
    member_changed("pages", ovalue, value);
  }

  /** Getter for attribute price


   \par In Python:
value = obj.price
   */
  grt::DoubleRef price() const {
    return _price;
  }
  /** Setter for attribute price


    \par In Python:
obj.price = value
   */
  virtual void price(const grt::DoubleRef &value) {
    grt::ValueRef ovalue(_price);
    _price = value;
    member_changed("price", ovalue, value);
  }

  /** Getter for attribute publisher

    the book publisher
   \par In Python:
value = obj.publisher
   */
  grt::Ref<test_Publisher> publisher() const {
    return _publisher;
  }
  /** Setter for attribute publisher

    the book publisher
    \par In Python:
obj.publisher = value
   */
  virtual void publisher(const grt::Ref<test_Publisher> &value) {
    grt::ValueRef ovalue(_publisher);
    _publisher = value;
    member_changed("publisher", ovalue, value);
  }

protected:
  grt::ListRef<test_Author> _authors; // owned
  grt::DictRef _extras;
  grt::IntegerRef _pages;
  grt::DoubleRef _price;
  grt::Ref<test_Publisher> _publisher;

private: // wrapper methods for use by grt
  static grt::ObjectRef create() {
    return grt::ObjectRef(new test_Book);
  }

public:
  std::string toString() const {
    return std::string("Book - title: ") + _title.toString();
  }
  static void grt_register() {
    grt::MetaClass *meta = grt::GRT::get()->get_metaclass(static_class_name());
    if (!meta)
      throw std::runtime_error("error initializing grt object class, metaclass not found");
    meta->bind_allocator(&test_Book::create);
    {
      void (test_Book::*setter)(const grt::ListRef<test_Author> &) = &test_Book::authors;
      grt::ListRef<test_Author> (test_Book::*getter)() const = &test_Book::authors;
      meta->bind_member("authors", new grt::MetaClass::Property<test_Book, grt::ListRef<test_Author> >(getter, setter));
    }
    {
      void (test_Book::*setter)(const grt::DictRef &) = &test_Book::extras;
      grt::DictRef (test_Book::*getter)() const = &test_Book::extras;
      meta->bind_member("extras", new grt::MetaClass::Property<test_Book, grt::DictRef>(getter, setter));
    }
    {
      void (test_Book::*setter)(const grt::IntegerRef &) = &test_Book::pages;
      grt::IntegerRef (test_Book::*getter)() const = &test_Book::pages;
      meta->bind_member("pages", new grt::MetaClass::Property<test_Book, grt::IntegerRef>(getter, setter));
    }
    {
      void (test_Book::*setter)(const grt::DoubleRef &) = &test_Book::price;
      grt::DoubleRef (test_Book::*getter)() const = &test_Book::price;
      meta->bind_member("price", new grt::MetaClass::Property<test_Book, grt::DoubleRef>(getter, setter));
    }
    {
      void (test_Book::*setter)(const grt::Ref<test_Publisher> &) = &test_Book::publisher;
      grt::Ref<test_Publisher> (test_Book::*getter)() const = &test_Book::publisher;
      meta->bind_member("publisher",
                        new grt::MetaClass::Property<test_Book, grt::Ref<test_Publisher> >(getter, setter));
    }
  }
};

inline void register_structs_test_xml() {
  grt::internal::ClassRegistry::register_class<test_Bridged>();
  grt::internal::ClassRegistry::register_class<test_Base>();
  grt::internal::ClassRegistry::register_class<test_Publisher>();
  grt::internal::ClassRegistry::register_class<test_Author>();
  grt::internal::ClassRegistry::register_class<test_Publication>();
  grt::internal::ClassRegistry::register_class<test_Book>();
}

#ifdef AUTO_REGISTER_GRT_CLASSES
static struct _autoreg__structs_test_xml {
  _autoreg__structs_test_xml() {
    register_structs_test_xml();
  }
} __autoreg__structs_test_xml;
#endif
