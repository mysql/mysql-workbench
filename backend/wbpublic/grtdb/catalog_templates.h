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

#ifndef _CATALOG_TEMPLATES_
#define _CATALOG_TEMPLATES_

#include "grts/structs.h"
#include "grts/structs.db.mgmt.h"
#include "grts/structs.db.mysql.h"

namespace ct {

  //! \addtogroup grt_iter GRT iterators
  //!
  //! To iterate through GRT model catalog_iterators can be used.
  //! The following code explains how to use template based iterators
  //!
  //! Enumeration \ref SubcontainerNamesEnum lists types of object for
  //! which iteration is possible.
  //!
  //! \anchor foreachusage
  //! \code
  //! //==============================================================
  //! class TableWalker
  //! {
  //!   public:
  //!     void operator()(const db_TableRef& table);
  //!     int tablesCount() const { return tablesCnt; }
  //!   private:
  //!     int tablesCnt;
  //! };
  //!
  //! //--------------------------------------------------------------
  //! void TableWalker::operator()(const db_TableRef& table)
  //! {
  //!   ++tableCnt;
  //! }
  //!
  //! //--------------------------------------------------------------
  //! {
  //!   db_SchemaRef schema = ...;
  //!   ...
  //!   TableWalker tw;
  //!   ct::for_each<ct::Tables>(schema, tw);
  //!   std::cout << tw.tablesCount();
  //! }
  //!
  //! \endcode
  //!   @{
  //!

  //! \enum SubcontainerNamesEnum
  //! \brief Possible types of objects to iterate on
  enum SubcontainerNamesEnum {
    Schemata = 0,   //!< Schemata
    Tables,         //!< Table
    Views,          //!< View
    Routines,       //!< Routine
    Triggers,       //!< Trigger
    Columns,        //!< Column
    Indices,        //!< Index
    ForeignKeys,    //!< Foreign key
    ReferedColumns, //!< Referenced column
    Users,          //!< User
    Size            //!< Number of values in SubcontainerNamesEnum
  };

  template <typename T, int SubcontainerSelector>
  struct Subc {};

  template <typename T>
  struct Subc<T, ct::Schemata> {
    typedef T ParentRef;
    typedef db_mysql_Schema Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->schemata();
    }
  };

  template <typename T>
  struct Subc<T, ct::Tables> {
    typedef T ParentRef;
    typedef db_mysql_Table Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->tables();
    }
  };

  template <typename T>
  struct Subc<T, ct::Views> {
    typedef T ParentRef;
    typedef db_mysql_View Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->views();
    }
  };

  template <typename T>
  struct Subc<T, ct::Routines> {
    typedef T ParentRef;
    typedef db_mysql_Routine Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->routines();
    }
  };

  template <>
  struct Subc<db_mysql_IndexRef, ct::Columns> {
    typedef db_mysql_IndexRef ParentRef;
    typedef db_mysql_IndexColumn Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->columns();
    }
  };

  template <typename T>
  struct Subc<T, ct::Columns> {
    typedef T ParentRef;
    typedef db_mysql_Column Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->columns();
    }
  };

  template <typename T>
  struct Subc<T, ct::Indices> {
    typedef T ParentRef;
    typedef db_mysql_Index Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->indices();
    }
  };

  template <typename T>
  struct Subc<T, ct::Triggers> {
    typedef T ParentRef;
    typedef db_mysql_Trigger Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->triggers();
    }
  };

  template <typename T>
  struct Subc<T, ct::ForeignKeys> {
    typedef T ParentRef;
    typedef db_mysql_ForeignKey Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->foreignKeys();
    }
  };

  template <typename T>
  struct Subc<T, ct::ReferedColumns> {
    typedef T ParentRef;
    typedef db_Column Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->referencedColumns();
    }
  };

  template <typename T>
  struct Subc<T, ct::Users> {
    typedef T ParentRef;
    typedef db_User Type;
    static grt::ListRef<Type> get(ParentRef p) {
      return p->users();
    }
  };

  //! Iterates over specified type of GRT objects in given container
  //! For example see \ref foreachusage "ct::for_each usage example"
  template <int _Selector, typename _Parent, typename _Pred>
  void for_each(_Parent parent, _Pred& pred) {
    typedef ct::Subc<_Parent, _Selector> Container;
    typedef typename Container::Type Type;
    typedef grt::ListRef<Type> ListType;

    ListType list = Container::get(parent);
    for (size_t i = 0, count = list.count(); i < count; i++) {
      grt::Ref<Type> t = list.get(i);
      pred(t);
    }
  }

  typedef std::vector<std::string> StringList;
  //-----------------------------------------------------------------------------
  template <typename T>
  std::vector<std::string> findDupIds(const grt::ListRef<T>& list) {
    const int count = list.count();
    std::vector<std::string> dup_ids;

    for (int bound = count - 1; bound > 0; --bound) {
      const T& bound_item(list.get(bound));
      for (int i = bound - 1; i >= 0; --i) {
        if (bound_item->name() == list.get(i)->name()) {
          dup_ids.push_back(bound_item->name().c_str());
        }
      }
    }

    return dup_ids;
  }

  template <typename T>
  struct Traits {};

  template <typename T>
  struct TraitsBase {
    typedef T Type;
  };

  template <>
  struct Traits<db_mysql_Schema> : public TraitsBase<db_mysql_Schema> {
    typedef db_mysql_Catalog ParentType;
  };

  template <>
  struct Traits<db_mysql_Table> : public TraitsBase<db_mysql_Table> {
    typedef db_mysql_Schema ParentType;
  };

  template <>
  struct Traits<db_User> : public TraitsBase<db_User> {
    typedef db_mysql_Schema ParentType;
  };

  template <>
  struct Traits<db_Column> : public TraitsBase<db_Column> {
    typedef db_mysql_Table ParentType;
  };

  template <>
  struct Traits<db_mysql_Column> : public TraitsBase<db_mysql_Column> {
    typedef db_mysql_Table ParentType;
  };

  template <>
  struct Traits<db_mysql_Index> : public TraitsBase<db_mysql_Index> {
    typedef db_mysql_Table ParentType;
  };

  template <>
  struct Traits<db_mysql_ForeignKey> : public TraitsBase<db_mysql_ForeignKey> {
    typedef db_mysql_Table ParentType;
  };

  template <>
  struct Traits<db_mysql_View> : public TraitsBase<db_mysql_View> {
    typedef db_mysql_Schema ParentType;
  };

  template <>
  struct Traits<db_mysql_Routine> : public TraitsBase<db_mysql_Routine> {
    typedef db_mysql_Schema ParentType;
  };

  template <>
  struct Traits<db_mysql_Trigger> : public TraitsBase<db_mysql_Trigger> {
    typedef db_mysql_Table ParentType;
  };

// ct_foreach usage:
// ct_foreach(db_mysql_SchemaRef schema, catalog->schemata())
//   print(schema->name());
// or
// ct_foreach(db_mysql_TableRef table, schema->tables())
// {
//    print(table->name())
// }

#define ct_foreach(value, container)                                                         \
  for (int ct_foreach_index = 0, ct_foreach_inner = 1, ct_foreach_count = container.count(); \
       ct_foreach_index < ct_foreach_count; ct_foreach_inner = 1, ct_foreach_index++)        \
    for (value = container.get(ct_foreach_index); ct_foreach_inner == 1; ct_foreach_inner = 0)

} // namespace ct

#endif // _CATALOG_TEMPLATES_

//! @}
