/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#ifdef _MSC_VER
  #ifdef _WIN64
    typedef __int64 ssize_t;
  #else
    typedef int ssize_t;
  #endif
#endif

#include <boost/signals2.hpp>
#include "base/threading.h"

namespace grt {
  class GRT;
  class MetaClass;

  namespace internal {
    class Serializer;
    class Unserializer;
  };

  //------------------------------------------------------------------------------------------------

  enum Type { UnknownType, AnyType = UnknownType, IntegerType, DoubleType, StringType, ListType, DictType, ObjectType };

  struct MYSQLGRT_PUBLIC SimpleTypeSpec {
    Type type;
    std::string object_class;

    SimpleTypeSpec() : type(UnknownType) {
    }

    SimpleTypeSpec(const SimpleTypeSpec &o) : type(o.type), object_class(o.object_class) {
    }

    inline SimpleTypeSpec &operator=(const SimpleTypeSpec &o) {
      type = o.type;
      object_class = o.object_class;
      return *this;
    }

    inline bool operator==(const SimpleTypeSpec &o) const {
      return o.type == type && o.object_class == object_class;
    }

    inline bool operator!=(const SimpleTypeSpec &o) const {
      return o.type != type || o.object_class != object_class;
    }
  };

  /*!
    \brief Type definition for GRT objects
    Describes type if a member defined by TypeSpec is of type: Integer, Double, String, etc
    Describes class if a member defined by TypeSpec is of Object type
  */
  struct MYSQLGRT_PUBLIC TypeSpec {
    SimpleTypeSpec base;    //!< Type of the object itself
    SimpleTypeSpec content; //!< Type of the stored items in case when object is of type of ListRef<T> or Dict
    bool operator==(const TypeSpec &t) const {
      return (base == t.base && content == t.content);
    }
  };

  class ValueRef;
  class BaseListRef;

  //------------------------------------------------------------------------------------------------

  class MYSQLGRT_PUBLIC type_error : public std::logic_error {
  public:
    type_error(Type expected, Type actual);
    type_error(TypeSpec expected, TypeSpec actual);
    type_error(Type expected, Type actual, Type container);
    type_error(const std::string &expected, const std::string &actual);
    type_error(const std::string &expected, const std::string &actual, Type container);
    type_error(const std::string &expected, Type actual);
    type_error(const std::string &msg) : std::logic_error(msg) {}
  };

//------------------------------------------------------------------------------------------------

namespace internal {

    class Object;

    class MYSQLGRT_PUBLIC Value {
    public:
      virtual ~Value() {}

      virtual Type get_type() const = 0;

      Value *retain();
      void release();

      virtual std::string debugDescription(const std::string &indentation = "") const = 0;
      virtual std::string toString() const = 0;

      base::refcount_t refcount() const;

      virtual void mark_global() const {}
      virtual void unmark_global() const {}

      virtual bool equals(const Value *) const = 0;
      virtual bool less_than(const Value *) const = 0;

      // This method helps to free memory allocated by Value.
      // It is overridden in Object, List and Dict.
      virtual void reset_references() {}

    protected:
      Value() : _refcount(0) {}

    private:
      Value(const Value &) {}

      volatile mutable base::refcount_t _refcount;
    };

    // 32 bit or 64 bit integer type.
    class MYSQLGRT_PUBLIC Integer : public Value {
    public:
      typedef ssize_t storage_type;

    public:
      Integer(storage_type value);
      static Integer *get(storage_type value);

      static Type static_type() {
        return IntegerType;
      }
      virtual Type get_type() const {
        return IntegerType;
      }
      virtual std::string debugDescription(const std::string &indentation = "") const;
      virtual std::string toString() const;

      inline operator storage_type() const {
        return _value;
      }
      inline storage_type operator*() const {
        return _value;
      }

      virtual bool equals(const Value *) const;
      virtual bool less_than(const Value *) const;

    protected:
      storage_type _value;
    };

    //------------------------------------------------------------------------------------------------

    class MYSQLGRT_PUBLIC Double : public Value {
    public:
      typedef double storage_type;

    public:
      Double(storage_type value);
      static Double *get(storage_type value);

      static Type static_type() {
        return DoubleType;
      }
      virtual Type get_type() const {
        return DoubleType;
      }
      virtual std::string debugDescription(const std::string &indentation = "") const;
      virtual std::string toString() const;

      inline operator storage_type() const {
        return _value;
      }
      inline storage_type operator*() const {
        return _value;
      }

      virtual bool equals(const Value *) const;
      virtual bool less_than(const Value *) const;

    protected:
      storage_type _value;
    };

    //------------------------------------------------------------------------------------------------

    class MYSQLGRT_PUBLIC String : public Value {
    public:
      typedef std::string storage_type;

    public:
      String(const storage_type &value);
      static String *get(const storage_type &value);

      static Type static_type() {
        return StringType;
      }
      virtual Type get_type() const {
        return StringType;
      }
      virtual std::string debugDescription(const std::string &indentation = "") const;
      virtual std::string toString() const;

      inline operator storage_type() const {
        return _value;
      }
      inline const storage_type &operator*() const {
        return _value;
      }
      inline const char *c_str() const {
        return _value.c_str();
      }
      inline bool empty() const {
        return _value.empty();
      }

      virtual bool equals(const Value *) const;
      virtual bool less_than(const Value *) const;

    protected:
      storage_type _value;
    };

    //------------------------------------------------------------------------------------------------

    class MYSQLGRT_PUBLIC List : public Value {
    public:
      typedef std::vector<ValueRef> storage_type;
      enum { npos = 0xffffffff };

      typedef std::vector<ValueRef>::const_iterator raw_const_iterator;
      typedef std::vector<ValueRef>::const_reverse_iterator raw_const_reverse_iterator;
      typedef std::vector<ValueRef>::iterator raw_iterator;

    public:
      List(bool allow_null);
      List(Type type, const std::string &content_class, bool allow_null);

      static Type static_type() {
        return ListType;
      }
      virtual Type get_type() const {
        return ListType;
      }
      inline const SimpleTypeSpec &content_type_spec() const {
        return _content_type;
      }
      inline Type content_type() const {
        return _content_type.type;
      }
      inline const std::string &content_class_name() const {
        return _content_type.object_class;
      }
      virtual std::string debugDescription(const std::string &indentation = "") const;
      virtual std::string toString() const;

      inline const ValueRef &get(size_t index) const {
        if (index >= count())
          throw bad_item(index, count());
        return _content[index];
      }

      virtual void set_unchecked(size_t index, const ValueRef &value);
      virtual void insert_unchecked(const ValueRef &value, size_t index = npos);

      void set_checked(size_t index, const ValueRef &value);
      void insert_checked(const ValueRef &value, size_t index = npos);

      bool check_assignable(const ValueRef &value) const;
      bool null_allowed() const {
        return _allow_null;
      }

      inline size_t count() const {
        return _content.size();
      }

      virtual void remove(const ValueRef &value);
      virtual void remove(size_t index);
      void reorder(size_t oi, size_t ni);

      size_t get_index(const ValueRef &value);

      inline const ValueRef &operator[](size_t i) const {
        return get(i);
      }

      virtual bool equals(const Value *) const;
      virtual bool less_than(const Value *) const;

      raw_const_iterator raw_begin() const {
        return _content.begin();
      }
      raw_const_iterator raw_end() const {
        return _content.end();
      }

      raw_const_reverse_iterator raw_rbegin() const {
        return _content.rbegin();
      }
      raw_const_reverse_iterator raw_rend() const {
        return _content.rend();
      }

      raw_iterator raw_begin() {
        return _content.begin();
      }
      raw_iterator raw_end() {
        return _content.end();
      }

    public:
      void __retype(Type type, const std::string &content_class);

      virtual void mark_global() const;
      virtual void unmark_global() const;

      virtual void reset_references();

    protected:
      friend class ::grt::GRT;
      friend class internal::Serializer;
      friend class internal::Unserializer;

      virtual ~List();

      storage_type _content;
      SimpleTypeSpec _content_type;
      bool _allow_null;

      mutable short _is_global;
    };

    class MYSQLGRT_PUBLIC OwnedList : public List {
    public:
      OwnedList(Type type, const std::string &content_class, Object *owner, bool allow_null);

      virtual void set_unchecked(size_t index, const ValueRef &value);
      virtual void insert_unchecked(const ValueRef &value, size_t index = npos);

      virtual void remove(const ValueRef &value);
      virtual void remove(size_t index);

      Object *owner_of_owned_list() const {
        return _owner;
      }

    protected:
      Object *_owner; // internal: set if it belongs to an object
    };

    //------------------------------------------------------------------------------------------------

    class MYSQLGRT_PUBLIC Dict : public Value {
    public:
      typedef std::map<std::string, ValueRef> storage_type;
      typedef storage_type::const_iterator const_iterator;
      typedef storage_type::const_iterator iterator;

    public:
      Dict(bool allow_null);
      Dict(Type type, const std::string &content_class, bool allow_null);

      static Type static_type() {
        return DictType;
      }
      virtual Type get_type() const {
        return DictType;
      }
      virtual std::string debugDescription(const std::string &indentation = "") const;
      virtual std::string toString() const;

      Type content_type() const {
        return _content_type.type;
      }
      const std::string &content_class_name() const {
        return _content_type.object_class;
      }

      ValueRef operator[](const std::string &key) const;

      const_iterator begin() const;
      const_iterator end() const;

      bool has_key(const std::string &key) const;
      ValueRef get(const std::string &key) const;
      virtual void set(const std::string &key, const ValueRef &value);
      virtual void remove(const std::string &key);
      virtual void reset_entries();
      size_t count() const {
        return _content.size();
      }

      std::vector<std::string> keys() const;

      virtual bool equals(const Value *) const;
      virtual bool less_than(const Value *) const;

      virtual void mark_global() const;
      virtual void unmark_global() const;

      virtual void reset_references();

    protected:
      friend class ::grt::GRT;

      storage_type _content;
      SimpleTypeSpec _content_type;
      bool _allow_null;

      mutable short _is_global; // whether value is attached to the global GRT tree
    };

    class MYSQLGRT_PUBLIC OwnedDict : public Dict {
    public:
      OwnedDict(Type type, const std::string &content_class, Object *owner, bool allow_null);

      virtual void set(const std::string &key, const ValueRef &value);
      virtual void remove(const std::string &key);
      virtual void reset_entries();

      Object *owner_of_owned_dict() const {
        return _owner;
      }

    protected:
      Object *_owner; // internal: set if it belongs to an object
    };

    //------------------------------------------------------------------------------------------------

    /** Base GRT Object class.
     *
     * This is subclassed by automatically generated GRT object classes.
     *
     * @ingroup GRT
     */
    class MYSQLGRT_PUBLIC Object : public Value {
    public:
      static std::string static_class_name() {
        return "Object";
      }

      virtual ~Object();

      const std::string &id() const;
      MetaClass *get_metaclass() const;
      const std::string &class_name() const;

      static Type static_type() {
        return ObjectType;
      }
      virtual Type get_type() const {
        return ObjectType;
      }
      virtual std::string debugDescription(const std::string &indentation = "") const;
      virtual std::string toString() const;

      bool is_instance(MetaClass *gclass) const;
      bool is_instance(const std::string &name) const;

      void set_member(const std::string &member, const ValueRef &value);
      ValueRef get_member(const std::string &member) const;
      std::string get_string_member(const std::string &member) const;
      Double::storage_type get_double_member(const std::string &member) const;
      Integer::storage_type get_integer_member(const std::string &member) const;
      bool has_member(const std::string &member) const;

      bool has_method(const std::string &method) const;

      virtual bool equals(const Value *) const;
      virtual bool less_than(const Value *) const;

      virtual ValueRef call(const std::string &method, const BaseListRef &args);

      bool is_global() const {
        return _is_global != 0;
      }

      boost::signals2::signal<void(const std::string &, const ValueRef &)> *signal_changed() {
        return &_changed_signal;
      }
      boost::signals2::signal<void(OwnedList *, bool, const grt::ValueRef &)> *signal_list_changed() {
        return &_list_changed_signal;
      }
      boost::signals2::signal<void(OwnedDict *, bool, const std::string &)> *signal_dict_changed() {
        return &_dict_changed_signal;
      }

      virtual void reset_references();

    public:
      virtual void init();

      void __set_id(const std::string &id);

      virtual void mark_global() const;
      virtual void unmark_global() const;

    protected:
      friend class OwnedList;
      friend class OwnedDict;
      friend class ::grt::GRT;
      friend class internal::Unserializer;

      explicit Object(MetaClass *gclass);

      void owned_member_changed(const std::string &name, const grt::ValueRef &ovalue, const grt::ValueRef &nvalue);
      void member_changed(const std::string &name, const grt::ValueRef &ovalue, const grt::ValueRef &nvalue);

      virtual void owned_list_item_added(OwnedList *list, const grt::ValueRef &value);
      virtual void owned_list_item_removed(OwnedList *list, const grt::ValueRef &value);

      virtual void owned_dict_item_set(OwnedDict *dict, const std::string &key);
      virtual void owned_dict_item_removed(OwnedDict *dict, const std::string &key);

      MetaClass *_metaclass;
      std::string _id;
      boost::signals2::signal<void(const std::string &, const grt::ValueRef &)> _changed_signal;
      boost::signals2::signal<void(OwnedList *, bool, const grt::ValueRef &)> _list_changed_signal;
      boost::signals2::signal<void(OwnedDict *, bool, const std::string &)> _dict_changed_signal;

      // ObjectValidFlag _valid_flag;

      mutable short _is_global; // whether object is attached to the global GRT tree

      //    public:
      //      const ObjectValidFlag &weakref_valid_flag() const { return _valid_flag; }
    };

    //----------------------------------------------------------------------------------------------------

    /** Registry for GRT object classes.
     *
     * This object is a singleton used to globally store the list of all
     * GRT object implementing C++ classes available. Each class, such
     * as db_Table, must call the registration function once. Whenever a
     * GRT instance is created, it will look at the list of classes and
     * register them with itself (so that allocation functions, member and
     * method pointers can be properly initialized).
     *
     * @ingroup GRTInternal
     */
    typedef void (*ClassRegistrationFunction)();

    struct MYSQLGRT_PUBLIC ClassRegistry {
    private:
      friend class ::grt::GRT;

      std::map<std::string, ClassRegistrationFunction> classes;

      ClassRegistry();

      /** Register all known classes in the given GRT context.
       */
      void register_all();

      /**
       * This one is neede dfor testing purposes only
       */
      void cleanUp();


      static ClassRegistry *get_instance();

      bool isEmpty();
    public:
      /** Template function to globally register a GRT class.
       */
      template <class C>
      static void register_class() {
        get_instance()->classes[C::static_class_name()] = &C::grt_register;
      }
    };

  }; // internal
};   // grt
