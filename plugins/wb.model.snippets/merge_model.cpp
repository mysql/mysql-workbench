/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "merge_model.h"
#include "grt.h"
#include "grtpp_util.h"

#include "grts/structs.workbench.h"
#include "grts/structs.db.h"
#include "grts/structs.workbench.model.h"

#include "base/wb_iterators.h"
#include "base/string_utilities.h"

#include "sqlide/recordset_table_inserts_storage.h"

#include "mforms/form.h"
#include "mforms/box.h"
#include "mforms/button.h"
#include "mforms/listbox.h"

#include "schema_select_form.h"

struct tolower_pred {
  std::string tolower(const std::string& arg) {
    return base::tolower(arg);
  };
};

template <class T, class TOwner>
void copy_additional_data(T obj, std::string new_name, TOwner new_owner) {
  update_ids(obj);
};

template <class TOwner>
void copy_additional_data(workbench_physical_DiagramRef obj, std::string old_name, TOwner new_owner) {
  grt::BaseListRef args(true);
  grt::Module* module = grt::GRT::get()->get_module("Workbench");
  grt::StringRef img_file_path(grt::StringRef::cast_from(module->call_function("getTempDir", args)));
  update_ids(obj);
  grt::ListRef<model_Figure> figures = obj->figures();
  for (size_t i = 0; i < figures.count(); ++i)
    if (workbench_model_ImageFigureRef::can_wrap(figures[i])) {
      workbench_model_ImageFigureRef image = workbench_model_ImageFigureRef::cast_from(figures[i]);
      std::string img_path = img_file_path;
      img_path.append("/").append(image->filename());
      image->setImageFile(img_path);
    }
}

template <class TOwner>
void copy_additional_data(db_TableRef obj, std::string old_name, TOwner new_owner) {
  grt::BaseListRef args(true);
  grt::Module* module = grt::GRT::get()->get_module("Workbench");
  grt::StringRef db_file_path(grt::StringRef::cast_from(module->call_function("getDbFilePath", args)));

  Recordset_table_inserts_storage::Ref input_storage = Recordset_table_inserts_storage::create_with_path(db_file_path);
  input_storage->table(obj);

  Recordset::Ref rs = Recordset::create();
  rs->data_storage(input_storage);
  rs->reset();

  // update IDs, INSERTs will go the table with ne IDs
  update_ids(obj);
  Recordset_table_inserts_storage::Ref output_storage = Recordset_table_inserts_storage::create();
  output_storage->table(obj);
  // Trigger creation of table to store INSERTs data
  output_storage->unserialize(Recordset::create());

  output_storage->serialize(rs);
  std::string table_inserts_sql = output_storage->sql_script();
};

template <class T>
void merge_list(grt::ListRef<T> list1, const grt::ListRef<T>& list2, const GrtObjectRef& dst_owner) {
  std::set<std::string> existing_names;

  for (size_t c = list1.count(), i = 0; i < c; i++)
    existing_names.insert(base::tolower(*list1[i]->name()));

  for (size_t c = list2.count(), i = 0; i < c; i++) {
    if (!GrtObjectRef::can_wrap(list2[i]))
      continue;
    std::string name = list2[i]->name();

    std::set<std::string>::const_iterator (std::set<std::string>::*findFn)(const std::string&) const =
      &std::set<std::string>::find;
    std::string new_name = grt::get_name_suggestion(
      std::bind(
        std::not_equal_to<std::set<std::string>::const_iterator>(),
        std::bind(findFn, &existing_names, std::bind(&tolower_pred::tolower, tolower_pred(), std::placeholders::_1)),
        existing_names.end()),
      name, false);
    GrtObjectRef new_obj = list2[i];
    new_obj->owner(dst_owner);
    if (new_name != name) {
      new_obj->name(new_name);
      existing_names.insert(base::tolower(new_name));
    }
    list1.insert(grt::ListRef<T>::value_type::cast_from(new_obj));
    copy_additional_data(grt::Ref<T>::cast_from(new_obj), name, dst_owner);
  }
}

template <class T>
void update_list(grt::ListRef<T> list) {
  for (size_t c = list.count(), i = 0; i < c; i++) {
    grt::Ref<T> obj = grt::Ref<T>::cast_from(list[i]);
    copy_additional_data(obj, obj->name(), obj->owner());
  }
}

void merge_schema( // workbench_DocumentRef document,
  const db_SchemaRef& target_schema, const db_SchemaRef& schema) {
  merge_list(target_schema->tables(), schema->tables(), GrtObjectRef::cast_from(target_schema));
  merge_list(target_schema->views(), schema->views(), target_schema);
  merge_list(target_schema->routines(), schema->routines(), target_schema);
  merge_list(target_schema->routineGroups(), schema->routineGroups(), target_schema);
}

void update_schema(db_SchemaRef& schema) {
  update_list(schema->tables());
  update_list(schema->views());
  update_list(schema->routines());
}

void merge_catalog(grt::Module* module, db_CatalogRef& dest_cat, const db_CatalogRef src_cat) {
  if (src_cat->schemata().count() == 1) {
    if (dest_cat->schemata().count() == 1) { // Merge to exiting schema regardless of name
      merge_schema(dest_cat->schemata().get(0), src_cat->schemata().get(0));
      return;
    } else {
      SchemaSelectionForm form(module, dest_cat->schemata(), src_cat->schemata().get(0));
      if (!form.run())
        return;
      // If selection is not valid procceed to regular merge and include diagram
      if (form.get_selection().is_valid()) {
        merge_schema(form.get_selection(), src_cat->schemata().get(0));
        return;
      }
      // Ask where to merge
    }
  }

  for (size_t sz = src_cat->schemata().count(), i = 0; i < sz; i++) {
    bool merged = false;
    for (size_t sz = dest_cat->schemata().count(), j = 0; j < sz; j++)
      if (strcmp(src_cat->schemata().get(i)->name().c_str(), dest_cat->schemata().get(j)->name().c_str()) == 0) {
        merge_schema(dest_cat->schemata().get(j), src_cat->schemata().get(i));
        merged = true;
        break;
      }
    if (merged)
      continue;
    db_SchemaRef schema = src_cat->schemata().get(i);
    schema->owner(dest_cat);
    dest_cat->schemata().insert(schema);
    update_schema(schema);
  }
};

void merge_diagrams(grt::ListRef<workbench_physical_Diagram>& dest_diagrams,
                    const grt::ListRef<workbench_physical_Diagram>& src_diagrams, const GrtObjectRef& dst_owner) {
  merge_list(dest_diagrams, src_diagrams, dst_owner);
};
