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

#pragma once

// XXX TODO get rid of this or rewrite to remove the macro-fest

#include "grt.h"
#include <deque>
#include <gmodule.h>
#include "grtpp_util.h"
#include "grts/structs.h"
#include "grts/structs.workbench.physical.h"
#include "grts/structs.model.h"
#include "grts/structs.workbench.logical.h"

namespace bec {

  typedef std::deque<GrtObjectRef> CallStack;

  template <class T>
  struct CatalogIterator {
    typedef int (T::*CGrtCb)(const GrtObjectRef &);
    typedef std::function<int(GrtObjectRef)> CGrtSlot;
    std::vector<CGrtSlot> allTypesSlots;
    void append_allTypesCb(T *self, CGrtCb cb) {
      allTypesSlots.push_back(std::bind(cb, self, std::placeholders::_1));
    }

#define WB_ITERATOR_SUPPORT_OBJECT_TYPE(type)                             \
  typedef int (T::*C##type##Cb)(const grt::Ref<type> &);                  \
  typedef std::function<int(grt::Ref<type>)> C##type##Slot;               \
  std::vector<C##type##Slot> type##Slots;                                 \
  std::vector<CGrtSlot> type##GrtSlots;                                   \
  void append(T *self, C##type##Cb cb) {                                  \
    type##Slots.push_back(std::bind(cb, self, std::placeholders::_1));    \
  }                                                                       \
  void append_##type##GrtCb(T *self, CGrtCb cb) {                         \
    type##GrtSlots.push_back(std::bind(cb, self, std::placeholders::_1)); \
  }

    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Schema);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Table);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Trigger);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Column);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Index);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_ForeignKey);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_View);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Routine);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_RoutineGroup);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_Role);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_User);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(db_RolePrivilege);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(model_Layer);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(model_Object);
    WB_ITERATOR_SUPPORT_OBJECT_TYPE(model_Diagram);

    CallStack call_stack;

  private:
    struct stack_item {
      CallStack &_stack;
      stack_item(const GrtObjectRef &value, CallStack &_stack) : _stack(_stack) {
        _stack.push_back(value);
      }
      ~stack_item() {
        _stack.pop_back();
      }
    };

  public:
#define EXIST_CB(type) (!type##Slots.empty() || !type##GrtSlots.empty() || !allTypesSlots.empty())

#define CALL_CB(type, obj)                                                    \
  {                                                                           \
    for (size_t i = 0, cb_count = type##Slots.size(); i < cb_count; i++) {    \
      res &= type##Slots[i](obj);                                             \
      if (!res && breakOnError)                                               \
        return res;                                                           \
    }                                                                         \
    for (size_t i = 0, cb_count = type##GrtSlots.size(); i < cb_count; i++) { \
      res &= type##GrtSlots[i](obj);                                          \
      if (!res && breakOnError)                                               \
        return res;                                                           \
    }                                                                         \
    for (size_t i = 0, cb_count = allTypesSlots.size(); i < cb_count; i++) {  \
      res &= allTypesSlots[i](obj);                                           \
      if (!res && breakOnError)                                               \
        return res;                                                           \
    }                                                                         \
  }
#define ITERATE_LIST(type, list, object)                         \
  {                                                              \
    grt::ListRef<type> list = object->list();                    \
    if (EXIST_CB(type))                                          \
      for (size_t i = 0, count = list.count(); i < count; i++) { \
        grt::Ref<type> item = list.get(i);                       \
        CALL_CB(type, item);                                     \
      }                                                          \
  }

#define ITERATE_LIST_DEEP(type, list, object)                  \
  {                                                            \
    grt::ListRef<type> list = object->list();                  \
    /*if (EXIST_CB(type)) TODO ?? */                           \
    for (size_t i = 0, count = list.count(); i < count; i++) { \
      grt::Ref<type> item = list.get(i);                       \
      CALL_CB(type, item);                                     \
      res &= iterate(Self, item, breakOnError);                \
      if (!res && breakOnError)                                \
        return res;                                            \
    }                                                          \
  }

    int iterate(T &Self, const workbench_logical_ModelRef &model, bool breakOnError = false,
                bool model_diagrams = true) {
      stack_item _centry(model, call_stack);

      int res = 1;

      if (model_diagrams) {
        res &= iterate_diagrams(Self, model, breakOnError);
        if (!res && breakOnError)
          return res;
      }

      return res;
    }

    int iterate(T &Self, const workbench_physical_ModelRef &model, bool breakOnError = false,
                bool model_diagrams = true) {
      stack_item _centry(model, call_stack);

      int res = 1;
      if (model_diagrams) {
        res &= iterate_diagrams(Self, model, breakOnError);
        if (!res && breakOnError)
          return res;
      }
      res &= iterate(Self, model->catalog(), breakOnError);
      if (!res && breakOnError)
        return res;

      return res;
    }

    int iterate(T &Self, const model_ObjectRef &figure, bool breakOnError = false) {
      int res = 1;
      stack_item _centry(figure, call_stack);

      for (size_t i = 0, cb_count = allTypesSlots.size(); i < cb_count; i++) {
        res &= allTypesSlots[i](figure);
        if (!res && breakOnError)
          return res;
      }

      if (EXIST_CB(db_Table) && workbench_physical_TableFigureRef::can_wrap(figure)) {
        workbench_physical_TableFigureRef table_figure(workbench_physical_TableFigureRef::cast_from(figure));
        if (table_figure->table().is_valid()) {
          CALL_CB(db_Table, table_figure->table());
          res = iterate(Self, table_figure->table(), breakOnError);
          if (!res && breakOnError)
            return res;
        }
      }

      if (EXIST_CB(db_View) && workbench_physical_ViewFigureRef::can_wrap(figure)) {
        workbench_physical_ViewFigureRef view_figure(workbench_physical_ViewFigureRef::cast_from(figure));
        if (view_figure->view().is_valid())
          CALL_CB(db_View, view_figure->view());
      }

      if ((EXIST_CB(db_RoutineGroup) || EXIST_CB(db_Routine)) &&
          workbench_physical_RoutineGroupFigureRef::can_wrap(figure)) {
        workbench_physical_RoutineGroupFigureRef rgroup_figure(
          workbench_physical_RoutineGroupFigureRef::cast_from(figure));
        if (rgroup_figure->routineGroup().is_valid()) {
          db_RoutineGroupRef routineGroup = rgroup_figure->routineGroup();
          CALL_CB(db_RoutineGroup, routineGroup);

          ITERATE_LIST(db_Routine, routines, rgroup_figure->routineGroup());
        }
      }
      return res;
    }

    int iterate(T &Self, const model_DiagramRef &view, bool breakOnError = false) {
      stack_item _centry(view, call_stack);
      int res = 1;
      ITERATE_LIST(model_Layer, layers, view);
      ITERATE_LIST_DEEP(model_Object, figures, view);

      return res;
    }

    int iterate_diagrams(T &Self, const model_ModelRef &model, bool breakOnError = false) {
      stack_item _centry(model, call_stack);
      int res = 1;
      ITERATE_LIST_DEEP(model_Diagram, diagrams, model);
      return res;
    }

    int iterate(T &Self, const db_TableRef &table, bool breakOnError = false) {
      if (!EXIST_CB(db_Column) && !EXIST_CB(db_Index) && !EXIST_CB(db_ForeignKey) && !EXIST_CB(db_Trigger))
        return 1;
      stack_item _centry(table, call_stack);

      int res = 1;

      ITERATE_LIST(db_Column, columns, table);
      ITERATE_LIST(db_Index, indices, table);
      ITERATE_LIST(db_ForeignKey, foreignKeys, table);
      ITERATE_LIST(db_Trigger, triggers, table);

      return res;
    }

    int iterate(T &Self, const db_SchemaRef &schema, bool breakOnError = false) {
      stack_item _centry(schema, call_stack);
      int res = 1;
      ITERATE_LIST_DEEP(db_Table, tables, schema);
      ITERATE_LIST(db_View, views, schema);
      ITERATE_LIST(db_Routine, routines, schema);
      ITERATE_LIST(db_RoutineGroup, routineGroups, schema);
      return res;
    }

    int iterate(T &Self, const db_CatalogRef &catalog, bool breakOnError = false) {
      int res = 1;
      stack_item _centry(catalog, call_stack);

      ITERATE_LIST_DEEP(db_Schema, schemata, catalog);
      ITERATE_LIST(db_User, users, catalog);
      ITERATE_LIST_DEEP(db_Role, roles, catalog);

      return res;
    }

    int iterate(T &Self, const db_RoleRef &role, bool breakOnError = false) {
      int res = 1;
      stack_item _centry(role, call_stack);

      ITERATE_LIST(db_RolePrivilege, privileges, role);

      return res;
    }

#define CASE_ITERATE(type)              \
  if (grt::Ref<type>::can_wrap(object)) \
    return iterate(Self, grt::Ref<type>::cast_from(object), breakOnError);

#define CASE_ITERATE_MODEL(type, model_diagrams) \
  if (grt::Ref<type>::can_wrap(object))          \
    return iterate(Self, grt::Ref<type>::cast_from(object), breakOnError, model_diagrams);

    int iterate(T &Self, const GrtObjectRef &object, bool breakOnError = false, bool model_diagrams = true) {
      stack_item _centry(object, call_stack);
      CASE_ITERATE_MODEL(workbench_logical_Model, model_diagrams);
      CASE_ITERATE_MODEL(workbench_physical_Model, model_diagrams);
      CASE_ITERATE(model_Diagram);
      CASE_ITERATE(model_Object);
      CASE_ITERATE(db_Catalog);
      CASE_ITERATE(db_Schema);
      CASE_ITERATE(db_Table);
      CASE_ITERATE(db_Role);

      int res = 1;
      for (size_t i = 0, cb_count = allTypesSlots.size(); i < cb_count; i++) {
        res &= allTypesSlots[i](object);
        if (!res && breakOnError)
          return res;
      }
      return 1;
    }
  };
}
