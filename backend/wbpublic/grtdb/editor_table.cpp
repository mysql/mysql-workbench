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

#include "editor_table.h"
#include "sqlide/recordset_table_inserts_storage.h"
#include "grtui/inserts_export_form.h"

#include "db_object_helpers.h"
#include "grtpp_util.h"
#include "grt/clipboard.h"
#include "grt/validation_manager.h"
#include "grtdb/db_object_helpers.h"
#include "grts/structs.workbench.physical.h"

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "objimpl/db.query/db_query_EditableResultset.h"

#include "base/log.h"

#include "mforms/box.h"
#include "mforms/form.h"
#include "mforms/menubar.h"
#include "mforms/gridview.h"
#include "mforms/toolbar.h"
#include "mforms/utilities.h"

#include <algorithm>
#undef min

using namespace bec;
using namespace grt;
using namespace base;

DEFAULT_LOG_DOMAIN("TableEditorBE")

/*
 The big mess of user datatypes, synonyms and column flags.

 - A column can have a datatype, a list of flags specific for the type (eg INT UNSIGNED ZEROFILL)
 and other parameters.
 - Some datatypes have aliases or synonyms, eg: INTEGER is just an alias for INT in MySQL
 - Users can define User Defined Datatypes and use them in columns. If a column type is set to a UDT
 no other parameters can be changed. Flags like UNSIGNED cannot be changed for the column that
 uses the UDT anymore. However, the UDT definition can be changed and if that happens, all uses
 of that UDT will be updated automatically.
 - Originally, server aliases were handled by a "synonyms" list for each simple datatype.
 So, INT had a list of synonyms that contained INTEGER. Both were basically treated as if they were
 the same, the same kind of parameters and flags could be toggled for either.
 - At some point, for a reason I don't know, someone removed the synonyms list and made
 such aliases be implemented as UDTs.
 - That caused all sorts of problems, mostly because aliases/synonyms can have parameters changed
 at will in the column definition while UDTs can't.
 - Now, to bring back sanity to this whole mess, we have to either:
   - revert the removal of aliases/synonyms and make things pretty again; or
   - introduce a hack to distinguish between real UDTs and UDTs that were corrupted into aliases
 - So, to distinguish between real UDTs and corrupt-UDTs you should check if the object id.
 */

static std::string getTemplate(workbench_physical_ModelRef model, const std::string &name, bool isEditingLiveObject) {
  if (isEditingLiveObject)
    return bec::GRTManager::get()->get_app_option_string(name);
  else
    return grt::StringRef::cast_from(bec::getModelOption(model, name));
}

//----------------------------------------------------------------------------------------------------------------------

TableColumnsListBE::TableColumnsListBE(TableEditorBE *owner) : _owner(owner) {
  _editing_placeholder_row = -1;
}

//----------------------------------------------------------------------------------------------------------------------

static bool sort_simple_type(const db_SimpleDatatypeRef &a, const db_SimpleDatatypeRef &b) {
  int i = strcmp(a->group()->name().c_str(), b->group()->name().c_str());
  if (i == 0)
    i = strcmp(a->name().c_str(), b->name().c_str());
  return i < 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::vector<std::string> TableColumnsListBE::get_datatype_names() {
  std::vector<std::string> types;

  // TODO replace this hard coded list with a dynamically generated top-used types list (same for charset).
  types.push_back("INT");
  types.push_back("VARCHAR()");
  types.push_back("DECIMAL()");
  types.push_back("DATETIME");
  types.push_back("BLOB");

  GrtVersionRef target_version = GrtVersionRef::cast_from(bec::getModelOption(workbench_physical_ModelRef::cast_from(_owner->get_catalog()->owner()), "CatalogVersion"));
  std::vector<db_SimpleDatatypeRef> sorted_types;
  grt::ListRef<db_SimpleDatatype> stypes(_owner->get_catalog()->simpleDatatypes());
  for (grt::ListRef<db_SimpleDatatype>::const_iterator iter = stypes.begin(); iter != stypes.end(); ++iter) {
    if (target_version.is_valid() && !bec::CatalogHelper::is_type_valid_for_version(*iter, target_version)) {
      if (!base::same_string((*iter)->name(), "json", false))
        continue;
    }

    sorted_types.push_back(*iter);
  }

  // sort by group and name
  std::sort(sorted_types.begin(), sorted_types.end(), sort_simple_type);

  // insert grouped by group
  db_DatatypeGroupRef group;
  for (std::vector<db_SimpleDatatypeRef>::const_iterator iter = sorted_types.begin(); iter != sorted_types.end();
       ++iter) {
    if (group != (*iter)->group()) {
      group = (*iter)->group();
      types.push_back("-");
    }

    std::string tmp;

    if ((*iter)->parameterFormatType() == 1 || (*iter)->parameterFormatType() == 2 ||
        (*iter)->parameterFormatType() == 3 || (*iter)->parameterFormatType() == 4 ||
        (*iter)->parameterFormatType() == 10)
      tmp = *(*iter)->name() + "()";
    else
      tmp = (*iter)->name();

    if (types.empty() || types.back() != tmp)
      types.push_back(tmp);
  }

  // insert UDTs
  types.push_back("-");
  grt::ListRef<db_UserDatatype> utypes(_owner->get_catalog()->userDatatypes());
  for (grt::ListRef<db_UserDatatype>::const_iterator iter = utypes.begin(); iter != utypes.end(); ++iter)
    types.push_back(*(*iter)->name());

  return types;
}

//----------------------------------------------------------------------------------------------------------------------

bec::ColumnNamesSet TableColumnsListBE::get_column_names_completion_list() const {
  bec::ColumnNamesSet column_names;

  db_SchemaRef schema = db_SchemaRef::cast_from(_owner->get_table()->owner());
  if (schema.is_valid()) {
    grt::ListRef<db_Table> tables = schema->tables();
    for (ssize_t i = tables.count() - 1; i >= 0; --i) {
      grt::ListRef<db_Column> columns = tables[i]->columns();
      for (ssize_t j = columns.count() - 1; j >= 0; --j) {
        column_names.insert(columns[j]->name().c_str());
      }
    }
  }

  return column_names;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief Get info for a table column
 *
 * @param node - identifier of the column node
 * @param name - returns name of column
 * @param type - returns formatted datatype of the column
 * @param ispk - returns true if column is part of a PK
 * @param notnull - returns true if column is NOT NULL
 * @param flags - returns comma separated list of additional flags
 * @param defvalue - returns default value of column
 * @param defnull - returns true if default value of column is NULL
 * @param charset - returns character set name of column
 * @param collation - returns collation name for column
 *
 * @return false if row is invalid
 */
bool TableColumnsListBE::get_row(const NodeId &node, std::string &name, std::string &type, bool &ispk, bool &notnull,
                                 bool &isunique, bool &isbinary, bool &isunsigned, bool &iszerofill, std::string &flags,
                                 std::string &defvalue, std::string &charset, std::string &collation,
                                 std::string &comment) {
  if (node[0] < real_count()) {
    db_ColumnRef col = _owner->get_table()->columns().get(node[0]);

    name = col->name();
    type = _owner->format_column_type(col);
    ispk = _owner->get_table()->isPrimaryKeyColumn(col) != 0;
    notnull = (col->isNotNull() != 0);
    isbinary = get_column_flag(node, "BINARY") != 0;
    isunsigned = get_column_flag(node, "UNSIGNED") != 0;
    iszerofill = get_column_flag(node, "ZEROFILL") != 0;
    flags = ""; // XXX
    defvalue = col->defaultValue();
    charset = col->characterSetName();
    collation = col->collationName();
    comment = col->comment();

    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void TableColumnsListBE::reorder(const NodeId &node, size_t nindex) {
  if (node[0] < real_count()) {
    AutoUndoEdit undo(_owner);

    _owner->get_table()->columns().reorder(node[0], nindex);
    update_primary_index_order();

    _owner->update_change_date();

    _owner->freeze_refresh_on_object_change();
    (*_owner->get_table()->signal_refreshDisplay())("column");
    _owner->thaw_refresh_on_object_change(true);
    undo.end(strfmt(_("Reorder Column '%s.%s'"), _owner->get_name().c_str(),
                    _owner->get_table()->columns()[node[0]]->name().c_str()));

    if (nindex < node[0])
      _owner->do_partial_ui_refresh(::bec::TableEditorBE::RefreshColumnMoveUp);
    else
      _owner->do_partial_ui_refresh(::bec::TableEditorBE::RefreshColumnMoveDown);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableColumnsListBE::reorder_many(const std::vector<std::size_t> &rows, std::size_t targetIndex) {
  if (!rows.empty()) {
    std::vector<size_t> indices(rows);
    std::sort(indices.begin(), indices.end());

    AutoUndoEdit undo(_owner);

    for (size_t i = 0; i < indices.size(); i++) {
      // Watch out how the backend does the reorder. If the source index is smaller than the target
      // index we will end up one node after the one we intended (the node is first removed
      // moving so all following indices one index down).
      _owner->get_table()->columns().reorder(indices[i], (indices[i] < targetIndex) ? targetIndex - 1 : targetIndex);

      // Update all those indices in our list between the current index and the target index
      // if the current index is below that value. This is because remaining indices move down in the list
      // if a node is removed from there and inserted after them in the list.
      if (indices[i] < targetIndex) {
        for (size_t j = i + 1; j < indices.size(); j++)
          if (indices[j] > indices[i] && indices[j] < targetIndex)
            indices[j]--;
      } else
        // If the current is above the target index then increase the latter to ensure
        // that the multiple indices stay in the same order.
        targetIndex++;
    }

    update_primary_index_order();

    _owner->update_change_date();
    (*_owner->get_table()->signal_refreshDisplay())("column");
    undo.end(strfmt(_("Reorder Columns in '%s'"), _owner->get_name().c_str()));

    _owner->do_partial_ui_refresh(::bec::TableEditorBE::RefreshColumnList);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableColumnsListBE::update_primary_index_order() {
  if (_owner->get_table()->primaryKey().is_valid()) {
    ListRef<db_Column> columns(_owner->get_table()->columns());
    ListRef<db_IndexColumn> icolumns(_owner->get_table()->primaryKey()->columns());
    if (icolumns.count() > 1) {
      size_t pos = 0;

      // update the PRIMARY index columns order to match the order they appear in the table
      for (size_t c = real_count(), i = 0; i < c; i++) {
        for (size_t ic = icolumns.count(), ii = pos; ii < ic; ii++) {
          if (icolumns[ii]->referencedColumn() == columns[i]) {
            if (ii != pos) {
              // out of place, reorder
              icolumns->reorder(ii, pos);
            }
            pos++;
            break;
          }
        }
        if (pos >= icolumns.count())
          break;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::set_column_type(const NodeId &node, const GrtObjectRef &type) {
  if (type.is_instance(db_UserDatatype::static_class_name())) {
    db_UserDatatypeRef utype(db_UserDatatypeRef::cast_from(type));

    AutoUndoEdit undo(_owner);

    if (node[0] >= real_count())
      _owner->add_column(
        grt::get_name_suggestion_for_list_object(_owner->get_table()->columns(), utype->name(), false));

    bool flag = set_field(node, Type, utype->name());

    undo.end(strfmt(_("Add Column to '%s'"), _owner->get_name().c_str()));

    return flag;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

IconId TableColumnsListBE::get_field_icon(const NodeId &node, size_t column, IconSize size) {
  if (node[0] < real_count()) {
    if (column == Type) {
      return 0;
    } else if (column == Name) {
      db_ColumnRef col = _owner->get_table()->columns().get(node[0]);

      if (_owner->get_table()->isPrimaryKeyColumn(col))
        return IconManager::get_instance()->get_icon_id(col, Icon11, "pk");
      else if (_owner->get_table()->isForeignKeyColumn(col)) {
        if (*col->isNotNull())
          return IconManager::get_instance()->get_icon_id(col, Icon11, "fknn");
        else
          return IconManager::get_instance()->get_icon_id(col, Icon11, "fk");
      } else {
        if (*col->isNotNull())
          return IconManager::get_instance()->get_icon_id(col, Icon11, "nn");
        else
          return IconManager::get_instance()->get_icon_id(col, Icon11);
      }
    }
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

void TableColumnsListBE::refresh() {
}

//----------------------------------------------------------------------------------------------------------------------

size_t TableColumnsListBE::real_count() {
  return _owner->get_table()->columns().count();
}

size_t TableColumnsListBE::count() {
  return _owner->get_table()->columns().count() + 1;
}

bool TableColumnsListBE::set_column_type_from_string(db_ColumnRef &col, const std::string &type) {
  bool flag = _owner->parse_column_type(type, col);
  if (flag) {
    if (col->simpleType().is_valid()) {
      // Remove flags that don't exist in new datatype.
      if (col->flags().count() > 0) {
        grt::StringListRef valid_flags(col->simpleType()->flags());

        for (ssize_t i = col->flags().count() - 1; i >= 0; i--) {
          if (valid_flags.get_index(col->flags().get(i)) == BaseListRef::npos)
            col->flags().remove(i);
        }
      }
    } else if (col->userType().is_valid()) {
      col->flags().remove_all();

      // Read top of file for note on column flag handling

      // Don't set flags from userType or we have trouble when userType is updated, that was
      // incorrect behavior.
    }
  } else
    logWarning("%s is not a valid column type\n", type.c_str());
  return flag;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::has_unique_index(const db_ColumnRef &col) {
  db_TableRef table(_owner->get_table());

  for (size_t c = table->indices().count(), i = 0; i < c; i++) {
    db_IndexRef index(table->indices()[i]);

    if (index->indexType() == "UNIQUE" && index->columns().count() == 1 &&
        index->columns()[0]->referencedColumn() == col)
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::make_unique_index(const db_ColumnRef &col, bool flag) {
  if (flag == has_unique_index(col))
    return false;

  db_TableRef table(_owner->get_table());

  if (flag) {
    db_IndexRef index = grt::GRT::get()->create_object<db_Index>(table->indices().content_class_name());

    index->name(*col->name() + "_UNIQUE");
    index->owner(table);

    index->indexType("UNIQUE");
    index->unique(1);

    db_IndexColumnRef icolumn = grt::GRT::get()->create_object<db_IndexColumn>(index->columns().content_class_name());
    icolumn->owner(index);
    icolumn->referencedColumn(col);

    index->columns().insert(icolumn);

    AutoUndoEdit undo(_owner);

    _owner->update_change_date();
    table->indices().insert(index);

    undo.end(strfmt(_("Add Unique Index for '%s'.'%s'"), _owner->get_name().c_str(), col->name().c_str()));
  } else {
    bool found = false;
    AutoUndoEdit undo(_owner);

    for (size_t c = table->indices().count(), i = 0; i < c; i++) {
      db_IndexRef index(table->indices()[i]);

      if (index->indexType() == "UNIQUE" && index->columns().count() == 1 &&
          index->columns()[0]->referencedColumn() == col) {
        table->indices().remove(i);
        found = true;
        break;
      }
    }

    _owner->update_change_date();

    if (found)
      undo.end(strfmt(_("Remove Unique Index for '%s'.'%s'"), _owner->get_name().c_str(), col->name().c_str()));
    else
      return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  RefreshUI::Blocker __centry(*_owner);

  std::string old;

  // either the row was edited or cancelled (sent "" as value)
  if (node[0] == count() - 1) {
    // not a placeholder anymore
    _editing_placeholder_row = -1;
  }

  // if the user enters a new column name in the editing placeholder row
  if (node[0] == count() - 1 && ((ColumnListColumns)column == Name || (ColumnListColumns)column == Type) &&
      !value.empty()) {
    AutoUndoEdit undo(_owner);
    // add new column

    std::string col_name;

    if ((ColumnListColumns)column == Name) {
      col_name = base::trim_right(value);
      _owner->add_column(col_name);

      db_ColumnRef col(_owner->get_table()->columns()[node[0]]);

      auto model = workbench_physical_ModelRef::cast_from(_owner->get_catalog()->owner());
      if (node[0] == 0) {
        _owner->get_table()->addPrimaryKeyColumn(col);
        set_column_type_from_string(col,
                                    getTemplate(model,
                                                "DefaultPkColumnType", _owner->is_editing_live_object()));
      } else {
        set_column_type_from_string(
          col, getTemplate(model, "DefaultColumnType",
                           _owner->is_editing_live_object()));
      }
    } else {
      std::string name;

      _editing_placeholder_row = node[0];
      get_field(node, Name, name);
      _editing_placeholder_row = -1;

      _owner->add_column(name);

      db_ColumnRef col(_owner->get_table()->columns()[node[0]]);
      col_name = col->name();

      if (node[0] == 0)
        _owner->get_table()->addPrimaryKeyColumn(col);

      set_column_type_from_string(col, grt::StringRef(value));
    }

    undo.end(strfmt(_("Add column '%s.%s'"), _owner->get_name().c_str(), col_name.c_str()));
    return true;
  } else if (node[0] >= real_count())
    return false;

  db_ColumnRef col;
  col = _owner->get_table()->columns().get(node[0]);

  get_field(node, column, old);

  switch ((ColumnListColumns)column) {
    case Name: {
      std::string value_ = base::trim_right(value);
      if (old != value_)
        _owner->rename_column(col, value_);
    }
      return true;
    case Type: {
      bool result = true;
      if (value != old) {
        AutoUndoEdit undo(_owner);

        if (set_column_type_from_string(col, value)) {
          _owner->update_change_date();
          undo.end(strfmt(_("Set Type of '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
        } else {
          result = _owner->showErrorMessage(value);
          undo.cancel();
        }

        _owner->do_partial_ui_refresh(::bec::TableEditorBE::RefreshColumnCollation);
      }
      return result;
    }

    case IsPK:
      return false;
    case IsNotNull:
      return false;
    case Flags:
      return false;
    case Default:
      if (old != value) {
        AutoUndoEdit undo(_owner, col, "defaultValue");

        std::string value_ = base::trim_right(value);

        db_SimpleDatatypeRef sData;

        if (col->userType().is_valid()) // Get SimpleData from userType when in modeling.
          sData = col->userType()->actualType();
        else if (col->simpleType().is_valid()) // When in SQLIDE we use simpleType directly.
          sData = col->simpleType();

        // Change true to 1 and false to 0 only when user selected numeric type and it's natural number (INT based like
        // TINY/MEDIUM/BIG).
        if (!value_.empty() && sData.is_valid() && sData->group().is_valid() && sData->group()->name() == "numeric" &&
            sData->numericScale() == 0) {
          if (same_string(value_, "true", false))
            value_ = "1";
          else if (same_string(value_, "false", false))
            value_ = "0";
        }

        bec::ColumnHelper::set_default_value(col, value_.empty() ? "" : quote_value_if_needed(col, value_));
        _owner->update_change_date();

        undo.end(strfmt(_("Set Default Value of '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
      }
      return true;
    case CharsetCollation:
      if (old != value) {
        std::string charset, collation;
        _owner->parse_charset_collation(value, charset, collation);
        if (charset != *col->characterSetName() || collation != *col->collationName()) {
          AutoUndoEdit undo(_owner);

          col->characterSetName(charset);
          col->collationName(collation);
          _owner->update_change_date();

          undo.end(strfmt(_("Set Collation/Charset of '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
        }
      }
      return true;
    case Charset:
      if (old != value) {
        AutoUndoEdit undo(_owner);

        col->characterSetName(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Set Character Set of '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
      }
      return true;
    case Collation:
      if (old != value) {
        AutoUndoEdit undo(_owner);

        col->collationName(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Set Collation of '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
      }
      return true;
    case HasCharset:
      return false;
    case Comment:
      if (old != value) {
        AutoUndoEdit undo(_owner, col, "comment");

        col->comment(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Set Comment of '%s.%s'"), _owner->get_name().c_str(), col->name().c_str()));
      }
      return true;
    case LastColumn:
      return false;
    default:
      return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void TableColumnsListBE::reset_placeholder() {
  _editing_placeholder_row = -1;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  RefreshUI::Blocker __centry(*_owner);
  db_ColumnRef col;

  // only entering a new name can create a new column
  if (node[0] == count() - 1) {
    // hack to get notifications of starting to edit placeholder row
    if (value == 1)
      _editing_placeholder_row = node[0];
    else
      _editing_placeholder_row = -1;
    return false;
  }
  col = _owner->get_table()->columns().get(node[0]);

  switch ((ColumnListColumns)column) {
    case Name:
    case Type:
      return false;
    case IsPK:
      if (col->simpleType().is_valid()) {
        if (col->simpleType()->name() == "JSON") {
          return false;
        }
      }
      if (*_owner->get_table()->isPrimaryKeyColumn(col) != (value != 0)) {
        AutoUndoEdit undo(_owner);

        if (value != 0) {
          _owner->get_table()->addPrimaryKeyColumn(col);
          if (col->defaultValueIsNull())
            bec::ColumnHelper::set_default_value(col, "");

        } else
          _owner->get_table()->removePrimaryKeyColumn(col);

        bool nvalue = _owner->get_table()->isPrimaryKeyColumn(col) != 0;

        _owner->update_change_date();

        if (nvalue)
          undo.end(strfmt(_("Set '%s.%s' PK"), _owner->get_name().c_str(), (*col->name()).c_str()));
        else
          undo.end(strfmt(_("Unset '%s.%s' PK"), _owner->get_name().c_str(), (*col->name()).c_str()));
      }
      return true;
    case IsNotNull: {
      FreezeRefresh frz(_owner);
      AutoUndoEdit undo(_owner);
      col->isNotNull(value != 0);

      // When setting the not-null flag then having a default value of NULL is meaningless.
      // Remove that if it is set.
      if (col->defaultValueIsNull() && col->isNotNull())
        bec::ColumnHelper::set_default_value(col, "");

      TableHelper::update_foreign_keys_from_column_notnull(_owner->get_table(), col);
      _owner->update_change_date();
      (*_owner->get_table()->signal_refreshDisplay())("column");
      undo.end(strfmt(_("Set '%s.%s' NOT NULL"), _owner->get_name().c_str(), (*col->name()).c_str()));
    }
      return true;
    case IsBinary: {
      FreezeRefresh frz(_owner);
      return set_column_flag(node, "BINARY", value != 0);
    }
    case IsUnsigned: {
      FreezeRefresh frz(_owner);
      return set_column_flag(node, "UNSIGNED", value != 0);
    }
    case IsZerofill: {
      FreezeRefresh frz(_owner);
      return set_column_flag(node, "ZEROFILL", value != 0);
    }
    case IsUnique: {
      FreezeRefresh frz(_owner);
      return make_unique_index(col, value != 0);
    }
    case Flags:
    case Default:
      return false;
    case CharsetCollation:
    case Charset:
    case Collation:
    case Comment:
    case HasCharset:
      return false;
    case LastColumn:
      return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  if (node[0] < real_count()) {
    db_ColumnRef col = _owner->get_table()->columns().get(node[0]);

    switch ((ColumnListColumns)column) {
      case Name:
        value = col->name();
        return true;
      case Type:
        value = grt::StringRef(_owner->format_column_type(col));
        return true;
      case IsPK:
        value = grt::IntegerRef(_owner->get_table()->isPrimaryKeyColumn(col) ? 1 : 0);
        return true;
      case IsNotNull:
        value = col->isNotNull();
        return true;
      case IsBinary:
        value = grt::IntegerRef(get_column_flag(node, "BINARY"));
        return true;
      case IsUnsigned:
        value = grt::IntegerRef(get_column_flag(node, "UNSIGNED"));
        return true;
      case IsZerofill:
        value = grt::IntegerRef(get_column_flag(node, "ZEROFILL"));
        return true;
      case IsUnique:
        value = grt::IntegerRef(has_unique_index(col) ? 1 : 0);
        return true;
      case Flags:
        value = grt::StringRef("");
        return true;
      case Default:
        value = col->defaultValue();
        return true;
      case CharsetCollation:
        value = grt::StringRef(_owner->format_charset_collation(col->characterSetName(), col->collationName()));
        return true;
      case Charset:
        value = col->characterSetName();
        return true;
      case Collation:
        value = col->collationName();
        return true;
      case HasCharset:
        value = grt::IntegerRef(0);
        /* moved to db specific code
        if (col->simpleType().is_valid())
        {
          if (col->simpleType()->group()->name() == "string" || col->simpleType()->group()->name() == "text")
            value= grt::IntegerRef(1);
        }*/
        return true;
      case Comment:
        value = col->comment();
        return true;
      case LastColumn:
        return false;
    }
  } else if (node[0] == count() - 1) {
    auto model = workbench_physical_ModelRef::cast_from(_owner->get_catalog()->owner());
    if (column == Name && _editing_placeholder_row == node[0]) {
      if (node[0] == 0) {
        value = grt::StringRef(
          base::replaceVariable(getTemplate(model, "PkColumnNameTemplate", _owner->is_editing_live_object()),
                                "%table%", _owner->get_name().c_str()));
      } else {
        std::string templ =
          base::replaceVariable(getTemplate(model, "ColumnNameTemplate", _owner->is_editing_live_object()),
                                "%table%", _owner->get_name().c_str());

        value = grt::StringRef(grt::get_name_suggestion_for_list_object(_owner->get_table()->columns(), templ, false));
      }
    } else if (column == Type && _editing_placeholder_row == node[0]) {
      if (node[0] == 0)
        value = grt::StringRef(getTemplate(model, "DefaultPkColumnType",
                                           _owner->is_editing_live_object()));
      else
        value = grt::StringRef(getTemplate(model, "DefaultColumnType",
                                           _owner->is_editing_live_object()));

    } else {
      value = grt::StringRef("");
      return false;
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a list of flags that are available for a column type.
 */
std::vector<std::string> TableColumnsListBE::get_datatype_flags(const ::bec::NodeId &node, bool all) {
  db_ColumnRef col;
  std::vector<std::string> retval;

  if (node.is_valid()) {
    if (node[0] < real_count())
      col = _owner->get_table()->columns().get(node[0]);

    if (!col.is_valid() || !col->simpleType().is_valid())
      return retval;

    grt::StringListRef datatype_flags;

    if (col->simpleType().is_valid())
      datatype_flags = col->simpleType()->flags();
    else if (col->userType().is_valid() && col->userType()->actualType().is_valid()) {
      // Return flags only for type aliases. UDTs cannot have their types changed.
      // See note at top of file for more about UDTs vs aliases.

      // the method to identify a type alias is a hack
      if (g_str_has_prefix(col->userType().id().c_str(), "com.mysql.rdbms.mysql.userdatatype."))
        datatype_flags = col->userType()->actualType()->flags();
    }

    if (!datatype_flags.is_valid())
      return retval;

    size_t flags_count = datatype_flags.count();

    for (size_t i = 0; i < flags_count; i++) {
      std::string s = datatype_flags.get(i).c_str();
      if (!all) {
        if (s == "UNSIGNED" || s == "ZEROFILL" || s == "BINARY")
          continue;
      }
      retval.push_back(s);
    }
  }
  return retval;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::set_column_flag(const ::bec::NodeId &node, const std::string &flag_name, int is_set) {
  db_ColumnRef col;
  std::vector<std::string> retval;

  if (node.is_valid()) {
    if (node[0] < real_count())
      col = _owner->get_table()->columns().get(node[0]);

    if (!col.is_valid())
      return false;

    bool found = false;
    grt::StringListRef col_flags = col->flags();
    size_t flags_count = col_flags.count();

    for (size_t i = 0; i < flags_count; i++) {
      grt::StringRef existing_flag = col_flags.get(i);
      if (flag_name.compare(existing_flag.c_str()) == 0) {
        found = true;
        if (!is_set) {
          AutoUndoEdit undo(_owner);

          col_flags.remove(i);
          _owner->update_change_date();
          (*_owner->get_table()->signal_refreshDisplay())("column");
          undo.end(
            strfmt(_("Unset %s of '%s.%s'"), flag_name.c_str(), _owner->get_name().c_str(), col->name().c_str()));
        }
        break;
      }
    }

    std::vector<std::string> allowed_flags(get_datatype_flags(node, true));
    if (!found && is_set && std::find(allowed_flags.begin(), allowed_flags.end(), flag_name) != allowed_flags.end()) {
      AutoUndoEdit undo(_owner);

      col_flags.insert(grt::StringRef(flag_name));
      _owner->update_change_date();

      (*_owner->get_table()->signal_refreshDisplay())("column");

      undo.end(strfmt(_("Set %s of '%s.%s'"), flag_name.c_str(), _owner->get_name().c_str(), col->name().c_str()));

      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

int TableColumnsListBE::get_column_flag(const ::bec::NodeId &node, const std::string &flag_name) {
  db_ColumnRef col;
  std::vector<std::string> retval;

  if (node.is_valid()) {
    if (node[0] < real_count())
      col = _owner->get_table()->columns().get(node[0]);

    if (!col.is_valid())
      return 0;

    if (col->simpleType().is_valid()) {
      grt::StringListRef col_flags = col->flags();
      size_t flags_count = col_flags.count();

      for (size_t i = 0; i < flags_count; i++) {
        grt::StringRef existing_flag = col_flags.get(i);
        if (g_ascii_strcasecmp(flag_name.c_str(), existing_flag.c_str()) == 0)
          return 1;
      }
    } else if (col->userType().is_valid())
      return std::string(col->userType()->flags()).find(flag_name) != std::string::npos;
  }
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::string TableColumnsListBE::quote_value_if_needed(const db_ColumnRef &column, const std::string &value) {
  std::string datatypeName;
  std::string datatypeGroupName;

  if (column->userType().is_valid() && column->userType()->actualType().is_valid())
    datatypeName = column->userType()->actualType()->name();
  else if (column->simpleType().is_valid()) {
    datatypeName = column->simpleType()->name();
    datatypeGroupName = column->simpleType()->group()->name();
  }

  // Only quote (var)char and do it only if not already quoted and not given the special values 0/NULL.
  // Non-text types (like datetime) shouldn't be auto-quoted.
  // Using the "text" and "string" groups here includes more than we'd need (e.g. TEXT blobs), but as long
  // as this function is only used for default values (which blobs cannot have) we are fine.
  bool isChar = (g_ascii_strcasecmp(datatypeGroupName.c_str(), "string") == 0) ||
                (g_ascii_strcasecmp(datatypeGroupName.c_str(), "text") == 0) ||
                (g_ascii_strcasecmp(datatypeName.c_str(), "ENUM") == 0) ||
                (g_ascii_strcasecmp(datatypeName.c_str(), "SET") == 0);

  if (isChar && (value != "NULL") && (value != "0") && (value[0] != '\''))
    return std::string("'").append(escape_sql_string(value)).append("'");
  return value;
}

//----------------------------------------------------------------------------------------------------------------------

MenuItemList TableColumnsListBE::get_popup_items_for_nodes(const std::vector<NodeId> &nodes) {
  MenuItemList items;
  MenuItem sep;
  sep.type = MenuSeparator;

  MenuItem item;

  item.caption = "Move Up";
  item.internalName = "moveUpToolStripMenuItem";
  item.accessibilityName = "Move Up";
  item.enabled = nodes.size() == 1;
  items.push_back(item);

  item.caption = "Move Down";
  item.internalName = "moveDownToolStripMenuItem";
  item.accessibilityName = "Move Down";
  item.enabled = nodes.size() == 1;
  items.push_back(item);
  items.push_back(sep);

  item.caption = "Copy";
  item.internalName = "copyColumnToolStripMenuItem";
  item.accessibilityName = "Copy";
  item.enabled = nodes.size() > 0;
  items.push_back(item);

  item.caption = "Cut";
  item.internalName = "cutColumnToolStripMenuItem";
  item.accessibilityName = "Cut";
  item.enabled = nodes.size() > 0;
  items.push_back(item);

  item.caption = "Paste";
  item.internalName = "pasteColumnToolStripMenuItem";
  item.accessibilityName = "Paste";
  item.enabled = false;
  bec::Clipboard *clip = bec::GRTManager::get()->get_clipboard();
  if (clip->is_data()) {
    std::list<grt::ObjectRef> data = clip->get_data();
    bool ok = true;
    for (std::list<grt::ObjectRef>::iterator i = data.begin(); i != data.end(); ++i) {
      if (!db_ColumnRef::can_wrap(*i))
        ok = false;
    }
    item.enabled = ok;
  }

  items.push_back(item);

  item.caption = "Delete Selected";
  item.internalName = "deleteSelectedColumnsToolStripMenuItem";
  item.accessibilityName = "Delete Selected Columns";
  item.enabled = nodes.size() > 0;
  items.push_back(item);
  items.push_back(sep);

  item.caption = "Refresh";
  item.internalName = "refreshGridToolStripMenuItem";
  item.accessibilityName = "Refresh Grid";
  item.enabled = true;
  items.push_back(item);
  items.push_back(sep);

  item.caption = "Clear Default";
  item.internalName = "clearDefaultToolStripMenuItem";
  item.accessibilityName = "Clear Default";
  item.enabled = nodes.size() > 0;
  items.push_back(item);

  item.caption = "Default NULL";
  item.internalName = "defaultNULLToolStripMenuItem";
  item.accessibilityName = "Default to NULL";
  item.enabled = nodes.size() > 0;
  items.push_back(item);

  return items;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &orig_nodes) {
  std::vector<NodeId> nodes(orig_nodes);
  std::sort(nodes.begin(), nodes.end());

  if (name == "moveUpToolStripMenuItem") {
    if (nodes.size() == 1) {
      size_t row = nodes.front()[0];
      reorder(nodes.front(), row - 1);
    }
  } else if (name == "moveDownToolStripMenuItem") {
    if (nodes.size() == 1) {
      size_t row = nodes.front()[0];
      reorder(nodes.front(), row + 1);
    }
  } else if (name == "copyColumnToolStripMenuItem") {
    bec::Clipboard *clip = bec::GRTManager::get()->get_clipboard();
    clip->clear();
    for (std::vector<NodeId>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
      if ((*iter)[0] < real_count()) {
        db_ColumnRef col(_owner->get_table()->columns().get((*iter)[0]));
        clip->append_data(col);
      }
    }
    clip->set_content_description("Table Column");
  } else if (name == "cutColumnToolStripMenuItem") {
    bec::Clipboard *clip = bec::GRTManager::get()->get_clipboard();
    clip->clear();
    for (std::vector<NodeId>::const_iterator iter = nodes.begin(); iter != nodes.end(); ++iter) {
      if ((*iter)[0] < real_count()) {
        db_ColumnRef col(_owner->get_table()->columns().get((*iter)[0]));
        clip->append_data(col);
      }
    }
    clip->set_content_description("Table Column");
    for (std::vector<NodeId>::reverse_iterator iter = nodes.rbegin(); iter != nodes.rend(); ++iter)
      delete_node(*iter);
  } else if (name == "pasteColumnToolStripMenuItem") {
    bec::Clipboard *clip = bec::GRTManager::get()->get_clipboard();
    if (clip->is_data()) {
      AutoUndoEdit undo(_owner);
      std::list<grt::ObjectRef> data = clip->get_data();
      ssize_t insert_after = nodes.empty() ? -1 : nodes[0][0];

      if (insert_after >= (ssize_t)real_count())
        insert_after = -1;

      for (std::list<grt::ObjectRef>::const_iterator i = data.begin(); i != data.end(); ++i) {
        db_ColumnRef column(db_ColumnRef::cast_from(*i));
        _owner->duplicate_column(column, insert_after < 0 ? -1 : insert_after++);
      }

      refresh();
      _owner->update_change_date();
      undo.end(base::strfmt("Paste columns to table %s", _owner->get_name().c_str()));
    }
  } else if (name == "deleteSelectedColumnsToolStripMenuItem") {
    for (std::vector<NodeId>::reverse_iterator iter = nodes.rbegin(); iter != nodes.rend(); ++iter) {
      delete_node(*iter);
    }
  } else if (name == "refreshGridToolStripMenuItem") {
    refresh();
    _owner->do_partial_ui_refresh(TableEditorBE::RefreshColumnList);
  } else if (name == "clearDefaultToolStripMenuItem") {
    AutoUndoEdit undo(_owner);
    bool changed = false;

    for (std::vector<NodeId>::reverse_iterator iter = nodes.rbegin(); iter != nodes.rend(); ++iter) {
      if ((*iter)[0] < real_count()) {
        db_ColumnRef col(_owner->get_table()->columns().get((*iter)[0]));

        if (col.is_valid()) {
          bec::ColumnHelper::set_default_value(col, "");
          _owner->update_change_date();
          changed = true;
        }
      }
    }
    if (changed) {
      undo.end(_("Clear Column Default"));
      _owner->do_partial_ui_refresh(TableEditorBE::RefreshColumnList);
    } else
      undo.cancel();
  } else if (name == "defaultNULLToolStripMenuItem") {
    AutoUndoEdit undo(_owner);
    bool changed = false;

    for (std::vector<NodeId>::reverse_iterator iter = nodes.rbegin(); iter != nodes.rend(); ++iter) {
      if ((*iter)[0] < real_count()) {
        db_ColumnRef col(_owner->get_table()->columns().get((*iter)[0]));

        if (col.is_valid()) {
          bec::ColumnHelper::set_default_value(col, "NULL");
          _owner->update_change_date();
          changed = true;
        }
      }
    }
    if (changed) {
      undo.end(_("Set Column Default to NULL"));
      _owner->do_partial_ui_refresh(TableEditorBE::RefreshColumnList);
    } else
      undo.cancel();
  } else
    return false;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::can_delete_node(const NodeId &node) {
  return node.is_valid() && node[0] < real_count();
}

//----------------------------------------------------------------------------------------------------------------------

bool TableColumnsListBE::delete_node(const NodeId &node) {
  if (!can_delete_node(node))
    return false;

  _owner->remove_column(node);

  return true;
}

//----------------- IndexColumnsListBE ---------------------------------------------------------------------------------

IndexColumnsListBE::IndexColumnsListBE(IndexListBE *owner) : _owner(owner) {
}

//----------------------------------------------------------------------------------------------------------------------

void IndexColumnsListBE::refresh() {
}

//----------------------------------------------------------------------------------------------------------------------

size_t IndexColumnsListBE::count() {
  return _owner->get_owner()->get_table()->columns().count();
}

//----------------------------------------------------------------------------------------------------------------------

db_IndexColumnRef IndexColumnsListBE::get_index_column(const db_ColumnRef &column) {
  if (column.is_valid() && _owner->get_selected_index().is_valid()) {
    grt::ListRef<db_IndexColumn> index_columns(_owner->get_selected_index()->columns());

    for (size_t c = index_columns.count(), i = 0; i < c; i++) {
      // because of the way the index editing works, it's impossible
      // to have 2 index columns referring to the same table column
      if (index_columns[i]->referencedColumn() == column)
        return index_columns[i];
    }
  }
  return db_IndexColumnRef();
}

//----------------------------------------------------------------------------------------------------------------------

size_t IndexColumnsListBE::get_index_column_index(const db_ColumnRef &column) {
  if (column.is_valid() && _owner->get_selected_index().is_valid()) {
    grt::ListRef<db_IndexColumn> index_columns(_owner->get_selected_index()->columns());

    for (size_t c = index_columns.count(), i = 0; i < c; i++) {
      // because of the way the index editing works, it's impossible
      // to have 2 index columns referring to the same table column
      if (index_columns[i]->referencedColumn() == column)
        return i;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

void IndexColumnsListBE::set_index_column_order(const db_IndexColumnRef &column, size_t order) {
  grt::ListRef<db_IndexColumn> index_columns(_owner->get_selected_index()->columns());
  size_t index = index_columns.get_index(column); // Check, we may get a BaseListRef::npos (-1) index

  g_return_if_fail(index != grt::BaseListRef::npos);
  index_columns.reorder(index, order);
}

//----------------------------------------------------------------------------------------------------------------------

void IndexColumnsListBE::set_column_enabled(const NodeId &node, bool flag) {
  if (get_column_enabled(node) != flag) {
    if (flag)
      _owner->add_column(_owner->get_owner()->get_table()->columns()[node[0]]);
    else
      _owner->remove_column(node);
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexColumnsListBE::get_column_enabled(const NodeId &node) {
  db_ColumnRef column(_owner->get_owner()->get_table()->columns()[node[0]]);
  return get_index_column(column).is_valid();
}

//----------------------------------------------------------------------------------------------------------------------

size_t IndexColumnsListBE::get_max_order_index() {
  size_t order = 0;

  if (_owner) {
    const db_IndexRef index = _owner->get_selected_index();

    if (index.is_valid())
      order = index->columns().count();
  }

  return order;
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexColumnsListBE::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  db_IndexColumnRef icolumn;

  if (node[0] >= count())
    return false;

  icolumn = get_index_column(_owner->get_owner()->get_table()->columns()[node[0]]);
  if(!icolumn.is_valid())
    return false;

  if (!_owner->index_editable(_owner->get_selected_index()) && column != OrderIndex &&
      !(icolumn->referencedColumn()->simpleType().is_valid() && column == Length &&
        icolumn->referencedColumn()->simpleType()->group()->name() == "string")) // index order can be changed for PKs
    return false; // index length can be changed for PK if it's text

  switch (column) {
    case Name:
      return false;
    case Descending:
      if (icolumn.is_valid()) {
        AutoUndoEdit undo(_owner->get_owner());

        set_column_enabled(node, true);
        icolumn->descend(value != 0);
        _owner->get_owner()->update_change_date();

        undo.end(strfmt(_("Set Storage Order of Index Column '%s.%s.%s'"), _owner->get_owner()->get_name().c_str(),
                        _owner->get_selected_index()->name().c_str(), icolumn->name().c_str()));
      }
      return true;
    case Length:
      if (icolumn.is_valid()) {
        AutoUndoEdit undo(_owner->get_owner());

        icolumn->columnLength(value);
        _owner->get_owner()->update_change_date();

        undo.end(strfmt(_("Set Length of Index Column '%s.%s.%s'"), _owner->get_owner()->get_name().c_str(),
                        _owner->get_selected_index()->name().c_str(), icolumn->name().c_str()));
      }
      return true;
    case OrderIndex:
      if (icolumn.is_valid()) {
        if (value >= 1 && (size_t)value <= get_max_order_index()) {
          AutoUndoEdit undo(_owner->get_owner());

          set_index_column_order(icolumn, value - 1);
          _owner->get_owner()->update_change_date();

          undo.end(strfmt(_("Reorder for Index Column '%s.%s.%s'"), _owner->get_owner()->get_name().c_str(),
                          _owner->get_selected_index()->name().c_str(), icolumn->name().c_str()));
        }
      }
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexColumnsListBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  db_IndexColumnRef icolumn;

  if (node[0] >= count())
    return false;

  if (!_owner->index_editable(_owner->get_selected_index()) &&
      column != OrderIndex) // index order can be changed for PKs
    return false;

  switch (column) {
    case Name:
    case Descending:
    case Length:
      return false;
    case OrderIndex: {
      int order = 0;
      if (sscanf(value.c_str(), "%i", &order) != 1)
        return false;

      return set_field(node, column, order);
    }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexColumnsListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  db_TableRef table = _owner->get_owner()->get_table();
  db_ColumnRef dbcolumn;

  if (node[0] < table->columns().count())
    dbcolumn = table->columns()[node[0]];

  switch (column) {
    case Name: {
      if (dbcolumn.is_valid())
        value = dbcolumn->name();
      else
        value = grt::StringRef("");
    }
      return true;
    case Descending: {
      db_IndexColumnRef icolumn = get_index_column(dbcolumn);

      if (icolumn.is_valid())
        value = icolumn->descend();
      else
        value = grt::IntegerRef(0);
    }
      return true;
    case Length: {
      db_IndexColumnRef icolumn = get_index_column(dbcolumn);

      if (icolumn.is_valid())
        value = icolumn->columnLength();
      else
        value = grt::IntegerRef(0);
    }
      return true;
    case OrderIndex: {
      ssize_t i = get_index_column_index(dbcolumn);

      if (i < 0)
        value = grt::StringRef("");
      else
        value = grt::StringRef(std::to_string(i + 1));
      return true;
    }
  }
  return false;
}

//----------------- IndexListBE ----------------------------------------------------------------------------------------

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4355) // 'this' : used in base member initializer list
                                  // that's ok as we just store pointer for later use
#endif

IndexListBE::IndexListBE(TableEditorBE *owner) : _column_list(this), _owner(owner) {
}

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

//----------------------------------------------------------------------------------------------------------------------

db_IndexRef IndexListBE::get_selected_index() {
  if (_selected.is_valid() && _selected[0] < real_count())
    return _owner->get_table()->indices().get(_selected[0]);
  return db_IndexRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexListBE::index_editable(const db_IndexRef &index) {
  if (index.is_valid()) {
    // check if index is PRIMARY (primary key index) or FOREIGN (fk index)
    // FK indices can now have the type changed, as they dont depend on indexType for
    // identifying them as FK index anymore --alfredo 10/04/16
    if (strcmp(index->indexType().c_str(), "PRIMARY") == 0)
      return false;
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

db_ForeignKeyRef IndexListBE::index_belongs_to_fk(const db_IndexRef &index) {
  // check if the index belongs to a FK
  if (index.is_valid()) {
    ListRef<db_ForeignKey> fks(db_TableRef::cast_from(index->owner())->foreignKeys());
    for (size_t c = fks.count(), i = 0; i < c; i++)
      if (fks[i]->index() == index)
        return fks[i];
  }
  return db_ForeignKeyRef();
}

//----------------------------------------------------------------------------------------------------------------------

MenuItemList IndexListBE::get_popup_items_for_nodes(const std::vector<NodeId> &nodes) {
  db_IndexRef index;

  if (!nodes.empty() && nodes[0][0] < _owner->get_table()->indices().count())
    index = _owner->get_table()->indices()[nodes[0][0]];

  MenuItemList items;
  MenuItem item;
  item.caption = _("Delete Selected");
  item.internalName = "deleteIndices";
  item.accessibilityName = "Delete Indices";
  item.enabled = index.is_valid() && nodes.size() > 0 && /*!index_belongs_to_fk(index) && */ index_editable(index);
  items.push_back(item);

  return items;
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexListBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &unsorted_nodes) {
  std::vector<NodeId> nodes(unsorted_nodes);
  std::sort(nodes.begin(), nodes.end());

  if (name == "deleteIndices") {
    for (std::vector<NodeId>::reverse_iterator node = nodes.rbegin(); node != nodes.rend(); ++node) {
      if ((*node)[0] < _owner->get_table()->indices().count()) {
        db_IndexRef index(_owner->get_table()->indices().get((*node)[0]));
        db_ForeignKeyRef fk;
        if (index.is_valid() && (fk = index_belongs_to_fk(index)).is_valid()) {
          // don't let the index to be deleted unless there's another suitable index
          if (bec::TableHelper::find_index_usable_by_fk(fk, index, false).is_valid())
            ;
          else {
            mforms::Utilities::show_message(
              "Cannot Delete Index",
              base::strfmt(
                "The index '%s' belongs to the Foreign Key '%s'.\nYou must delete the Foreign Key to delete the index.",
                index->name().c_str(), fk->name().c_str()),
              "OK", "", "");
            continue;
          }
        }
      }
      _owner->remove_index(*node, true);
    }
  } else
    return false;

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexListBE::can_delete_node(const NodeId &node) {
  return node.is_valid() && node[0] < real_count();
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexListBE::delete_node(const NodeId &node) {
  if (!can_delete_node(node))
    return false;

  _owner->remove_index(node, false);

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

NodeId IndexListBE::add_column(const db_ColumnRef &column, const db_IndexRef &aIndex) {
  db_IndexRef index = aIndex.is_valid() ? aIndex : get_selected_index();

  if (!index.is_valid())
    return NodeId();

  if (/*!index_editable(index) || */ index_belongs_to_fk(index).is_valid())
    return NodeId();

  // special handling for PK columns
  if (strcmp(index->indexType().c_str(), "PRIMARY") == 0) {
    AutoUndoEdit undo(_owner);
    _owner->get_table()->addPrimaryKeyColumn(column);
    _owner->update_change_date();

    undo.end(strfmt(_("Add column '%s' to primary key from '%s'"), column->name().c_str(), _owner->get_name().c_str()));
  } else {
    std::string column_struct = index.get_metaclass()->get_member_type("columns").content.object_class;
    db_IndexColumnRef icolumn = grt::GRT::get()->create_object<db_IndexColumn>(column_struct);
    icolumn->owner(index);
    icolumn->referencedColumn(column);

    AutoUndoEdit undo(_owner);

    index->columns().insert(icolumn);
    _owner->update_change_date();

    undo.end(strfmt(_("Add column '%s' to index '%s.%s'"), column->name().c_str(), _owner->get_name().c_str(),
                    index->name().c_str()));
  }
  _column_list.refresh();

  return NodeId(index->columns().count() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListBE::remove_column(const NodeId &node) {
  RefreshUI::Blocker __centry(*_owner);

  db_IndexRef index = get_selected_index();
  if (/*!index_editable(index) || */ index_belongs_to_fk(index).is_valid())
    return;

  db_ColumnRef column(_owner->get_table()->columns().get(node[0]));

  // special handling for PK columns
  if (strcmp(index->indexType().c_str(), "PRIMARY") == 0) {
    AutoUndoEdit undo(_owner);
    _owner->get_table()->removePrimaryKeyColumn(column);
    _owner->update_change_date();

    undo.end(
      strfmt(_("Remove column '%s' from primary key from '%s'"), column->name().c_str(), _owner->get_name().c_str()));
  } else {
    for (size_t c = index->columns().count(), i = 0; i < c; i++) {
      if (index->columns().get(i)->referencedColumn() == column) {
        AutoUndoEdit undo(_owner);

        index->columns().remove(i);
        _owner->update_change_date();

        undo.end(strfmt(_("Remove column '%s' from index '%s.%s'"), column->name().c_str(), _owner->get_name().c_str(),
                        index->name().c_str()));
        _column_list.refresh();
        break;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListBE::refresh() {
}

//----------------------------------------------------------------------------------------------------------------------

size_t IndexListBE::count() {
  return real_count() + 1;
}

//----------------------------------------------------------------------------------------------------------------------

size_t IndexListBE::real_count() {
  return _owner->get_table()->indices().count();
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexListBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  if (!node.is_valid())
    return false;

  db_IndexRef index;
  std::vector<std::string> index_types;

  // if (node[0] == count() && column == Name)
  if (node[0] == real_count()) // a field added for new index placeholder
  {
    if (value.empty())
      return false;
    if (column == Name) {
      _owner->add_index(value);
      return true;
    } else
      _owner->add_index(("index" + std::to_string(count())).c_str());
  }

  index = _owner->get_table()->indices().get(node[0]);

  if (!index_editable(index) && column != Comment) // Allow editing comments on PKs.
    return false;

  switch (column) {
    case Name:
      if (index->name() != value) {
        AutoUndoEdit undo(_owner, index, "name");
        index->name(value);
        _owner->update_change_date();
        undo.end(strfmt(_("Rename Index '%s.%s'"), _owner->get_name().c_str(), index->name().c_str()));
        bec::ValidationManager::validate_instance(index, CHECK_NAME);
      }
      return true;
    case Type:
      index_types = _owner->get_index_types();
      if (std::find(index_types.begin(), index_types.end(), value) == index_types.end())
        return false;
      // fk indices or primary indices cant be created by user
      if (value == "PRIMARY")
        return false;
      if (index->indexType() != value) {
        AutoUndoEdit undo(_owner);

        index->indexType(value);
        index->unique(value == "UNIQUE");
        _owner->update_change_date();

        undo.end(strfmt(_("Set Type of Index '%s.%s'"), _owner->get_name().c_str(), index->name().c_str()));
      }
      return true;
    case Comment:
      if (index->comment() != value) {
        AutoUndoEdit undo(_owner, index, "comment");

        index->comment(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Set Comment of Index '%s.%s'"), _owner->get_name().c_str(), index->name().c_str()));
      }
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool IndexListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  db_IndexRef index;

  if (node[0] < real_count())
    index = _owner->get_table()->indices().get(node[0]);

  switch (column) {
    case Name:
      if (node[0] < real_count())
        value = index->name();
      else
        value = grt::StringRef("");
      return true;
    case Type:
      if (node[0] < real_count())
        value = index->indexType();
      else
        value = grt::StringRef("");
      return true;
    case Comment:
      if (node[0] < real_count())
        value = index->comment();
      else
        value = grt::StringRef("");
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void IndexListBE::select_index(const NodeId &node) {
  _selected = node;
  _column_list.refresh();
}

//----------------- FKConstraintColumnsListBE --------------------------------------------------------------------------

FKConstraintColumnsListBE::FKConstraintColumnsListBE(FKConstraintListBE *owner) : _owner(owner) {
}

//----------------------------------------------------------------------------------------------------------------------

void FKConstraintColumnsListBE::refresh() {
  _referenced_columns.clear();

  db_ForeignKeyRef fk(_owner->get_selected_fk());
  if (fk.is_valid()) {
    for (size_t i = fk->columns().count(); i >= 1; --i) {
      db_ColumnRef column(fk->columns()[i - 1]);
      bool ok = false;
      if (column.is_valid() && i - 1 < fk->referencedColumns().count()) {
        db_ColumnRef refcolumn(fk->referencedColumns()[i - 1]);
        if (refcolumn.is_valid())
          ok = true;
        _referenced_columns[column.id()] = refcolumn;
      }

      if (!ok) {
        // invalid! remove this entry
        fk->columns().remove(i - 1);
        if (i - 1 < fk->referencedColumns().count())
          fk->referencedColumns().remove(i - 1);

        logWarning("Removed corrupt column definition for Foreign Key %s\n", fk->name().c_str());
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::set<std::string> get_indexed_column_ids(const db_TableRef &table) {
  std::set<std::string> set;
  for (size_t c = table->indices().count(), i = 0; i < c; i++) {
    db_IndexRef index(table->indices()[i]);
    for (size_t cc = index->columns().count(), j = 0; j < cc; j++)
      set.insert(index->columns()[j]->referencedColumn().id());
  }
  return set;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a list of candidate referenced column names for the FK.
 *
 * @param node of the source column in the FK column list.
 * @return List of possible column names.
 */
std::vector<std::string> FKConstraintColumnsListBE::get_ref_columns_list(const NodeId &node, bool filtered) {
  db_ForeignKeyRef fk(_owner->get_selected_fk());

  if (fk.is_valid() && fk->referencedTable().is_valid() &&
      (size_t)node[0] < _owner->get_owner()->get_table()->columns().count()) {
    std::vector<std::string> names, names2;
    db_TableRef table(fk->referencedTable());
    grt::ListRef<db_Column> columns(table->columns());
    std::set<std::string> indexed_column_ids(get_indexed_column_ids(table));
    db_ColumnRef srccolumn(_owner->get_owner()->get_table()->columns()[node[0]]);

    for (size_t c = columns.count(), i = 0; i < c; i++) {
      if (!filtered)
        names.push_back(columns[i]->name());
      else if (_owner->get_owner()->check_column_referenceable_by_fk(columns[i], srccolumn)) {
        // column must have index to be in a Fk
        if (table->isPrimaryKeyColumn(columns[i]))
          names.push_back(columns[i]->name());
        else if (indexed_column_ids.find(columns[i]->id()) != indexed_column_ids.end())
          names2.push_back(columns[i]->name());
      }
    }
    names.insert(names.end(), names2.begin(), names2.end());

    if (fk->referencedTable()->isStub())
      names.push_back("Specify Column...");

    return names;
  }

  return std::vector<std::string>();
}

//----------------------------------------------------------------------------------------------------------------------

size_t FKConstraintColumnsListBE::count() {
  if (_owner->get_selected_fk().is_valid())
    return _owner->get_owner()->get_table()->columns().count();
  return 0;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintColumnsListBE::set_fk_column_pair(const db_ColumnRef &column, const db_ColumnRef &refcolumn) {
  _referenced_columns[column.id()] = refcolumn;

  db_ForeignKeyRef fk(_owner->get_selected_fk());

  AutoUndoEdit undo(_owner->get_owner());
  bool changed_real_fk = false;

  // check if the srccolumn is already in FK
  size_t index = fk->columns().get_index(column);
  if (index != grt::BaseListRef::npos) {
    if (!refcolumn.is_valid()) {
      // remove real
      size_t table_column_index = _owner->get_owner()->get_table()->columns().get_index(column);
      if (table_column_index != grt::BaseListRef::npos) {
        _owner->remove_column(table_column_index);
        changed_real_fk = true;
      }
    } else {
      // update real
      fk->referencedColumns().set(index, refcolumn);
      changed_real_fk = true;
    }
  } else {
    // if everything is ok, create it for real
    if (column.is_valid() && refcolumn.is_valid()) {
      _owner->add_column(column, refcolumn, fk);
      changed_real_fk = true;
    }
  }

  if (changed_real_fk) {
    TableHelper::update_foreign_key_index(fk);

    _owner->get_owner()->update_change_date();
    undo.end(strfmt(_("Set Ref. Column for FK '%s.%s'"), _owner->get_owner()->get_name().c_str(), fk->name().c_str()));
  } else
    undo.cancel();

  return changed_real_fk;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintColumnsListBE::set_column_is_fk(const NodeId &node, bool flag) {
  if (get_column_is_fk(node) != flag) {
    if (flag) {
      // disallow
      if (flag && get_ref_columns_list(node).empty())
        return false;

      // guess some column from the ref table
      db_ForeignKeyRef fk(_owner->get_selected_fk());

      // std::string type;
      db_ColumnRef srccolumn(_owner->get_owner()->get_table()->columns()[node[0]]);
      db_ColumnRef refcolumn;

      // type= ColumnHelper::format_column_type(srccolumn, 0, true);

      if (fk.is_valid() && fk->referencedTable().is_valid()) {
        db_TableRef table(fk->referencedTable());
        grt::ListRef<db_Column> columns(table->columns());

        for (size_t c = columns.count(), i = 0; i < c; i++) {
          if (srccolumn != columns[i]) {
            // if (ColumnHelper::format_column_type(columns[i], 0, true) == type)
            if (_owner->get_owner()->check_column_referenceable_by_fk(columns[i], srccolumn)) {
              if (table->isPrimaryKeyColumn(columns[i])) {
                refcolumn = columns[i];
                break;
              } else {
                if (!refcolumn.is_valid())
                  refcolumn = columns[i];
              }
            }
          }
        }
      }

      // create it virtually only unless whole definition is valid
      set_fk_column_pair(srccolumn, refcolumn);
    } else {
      db_ColumnRef srccolumn(_owner->get_owner()->get_table()->columns()[node[0]]);

      // remove it virtually
      if (_referenced_columns.find(srccolumn.id()) != _referenced_columns.end())
        _referenced_columns.erase(srccolumn.id());

      _owner->remove_column(node);
    }
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

ssize_t FKConstraintColumnsListBE::get_fk_column_index(const NodeId &node) {
  db_TableRef table = _owner->get_owner()->get_table();
  db_ForeignKeyRef fk(_owner->get_selected_fk());

  if (fk.is_valid() && node[0] < table->columns().count()) {
    db_ColumnRef column = table->columns()[node[0]];

    // find index in list of columns for the FK
    for (size_t c = fk->columns().count(), i = 0; i < c; i++) {
      if (fk->columns().get(i) == column)
        return i;
    }
  }
  return -1;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintColumnsListBE::get_column_is_fk(const NodeId &node) {
  if (node[0] < _owner->get_owner()->get_table()->columns().count()) {
    db_ColumnRef srccolumn(_owner->get_owner()->get_table()->columns()[node[0]]);

    return get_fk_column_index(node) >= 0 || _referenced_columns.find(srccolumn.id()) != _referenced_columns.end();
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintColumnsListBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  db_ForeignKeyRef fk(_owner->get_selected_fk());
  db_ColumnRef tcolumn;

  switch (column) {
    case Enabled:
      return false;

    case Column:
      // this is not changeable
      return false;

    case RefColumn:
      if (fk.is_valid() && fk->referencedTable().is_valid()) {
        std::unique_ptr<AutoUndoEdit> undo;

        tcolumn = grt::find_named_object_in_list(fk->referencedTable()->columns(), value);

        if (tcolumn.is_valid() == false)
          return false;

        // special handling for stub tables
        if (fk->referencedTable()->isStub() && !tcolumn.is_valid() && !_owner->get_owner()->is_editing_live_object()) {
          std::string column_name;
          // ask for a column name and add it to the stub table
          if (mforms::Utilities::request_input(_("Specify Referenced Column"), _("Referenced Column Name:"), "",
                                               column_name)) {
            tcolumn = grt::find_named_object_in_list(fk->referencedTable()->columns(), column_name);
            if (!tcolumn.is_valid()) {
              undo = std::unique_ptr<AutoUndoEdit>(new AutoUndoEdit(_owner->get_owner()));

              // create the column
              tcolumn = grt::copy_object(_owner->get_owner()->get_table()->columns()[node[0]]);
              fk->referencedTable()->columns().insert(tcolumn);
              tcolumn->owner(fk->referencedTable());
              tcolumn->name(column_name);

              fk->referencedTable()->addPrimaryKeyColumn(tcolumn);
            }
          } else
            return false;
        }

        ssize_t index = get_fk_column_index(node);

        // if columns is not enabled yet, enable it
        if (index < 0) {
          set_field(node, Enabled, 1);
          index = get_fk_column_index(node);
          if (index < 0)
            return false;
        }

        // check if column is acceptable
        std::vector<std::string> allowed_list = get_ref_columns_list(node);
        if (std::find(allowed_list.begin(), allowed_list.end(), value) == allowed_list.end()) {
          std::set<std::string> indexed_column_ids(get_indexed_column_ids(fk->referencedTable()));

          if (indexed_column_ids.find(tcolumn->id()) == indexed_column_ids.end()) {
            mforms::Utilities::show_message(
              "Create Foreign Key",
              base::strfmt(
                "Selected column %s must be indexed and be of a compatible type for a Foreign Key to be created.",
                tcolumn->name().c_str()),
              "OK");
          } else {
            db_TableRef reftable = fk->referencedTable();
            std::string hint;
            hint =
              base::strfmt("\nHint: source column has type %s%s,\nColumn %s of referenced table is %s%s.",
                           _owner->get_owner()->get_table()->columns()[node[0]]->flags().get_index("UNSIGNED") !=
                               grt::BaseListRef::npos
                             ? "UNSIGNED "
                             : "",
                           _owner->get_owner()->get_table()->columns()[node[0]]->formattedType().c_str(), value.c_str(),
                           tcolumn->flags().get_index("UNSIGNED") != grt::BaseListRef::npos ? "UNSIGNED " : "",
                           tcolumn->formattedType().c_str());
            // show a warning
            mforms::Utilities::show_message(
              "Create Foreign Key",
              base::strfmt("The selected columns do not have compatible types.\n%s", hint.c_str()), "OK");
          }
          return false;
        }

        set_fk_column_pair(fk->columns()[index], tcolumn);

        if (undo.get()) {
          undo->end(base::strfmt("Set Column of Foreign Key %s.%s.%s",
                                 _owner->get_owner()->get_schema()->name().c_str(),
                                 _owner->get_owner()->get_name().c_str(), fk->name().c_str()));
        }
      }
      return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintColumnsListBE::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  db_ForeignKeyRef fk(_owner->get_selected_fk());

  switch (column) {
    case Enabled:
      if (fk.is_valid()) {
        AutoUndoEdit undo(_owner->get_owner());
        if (set_column_is_fk(node, value != 0)) {
          _owner->get_owner()->update_change_date();
          undo.end(
            value ? strfmt(_("Add Column to FK '%s.%s'"), _owner->get_owner()->get_name().c_str(), fk->name().c_str())
                  : strfmt(_("Remove Column from FK '%s.%s'"), _owner->get_owner()->get_name().c_str(),
                           fk->name().c_str()));
        } else {
          undo.cancel();

          db_ColumnRef tcolumn;
          if (node[0] < _owner->get_owner()->get_table()->columns().count())
            tcolumn = fk->owner()->columns()[node[0]];
          db_TableRef reftable = fk->referencedTable();

          if (reftable.is_valid() && tcolumn.is_valid() && get_ref_columns_list(node).empty()) {
            std::string hint;
            if (reftable->primaryKey().is_valid() && reftable->primaryKey()->columns().count() > 0)
              hint = base::strfmt("\nHint: source column has type %s%s,\nPK of referenced table is %s%s.",
                                  tcolumn->flags().get_index("UNSIGNED") != grt::BaseListRef::npos ? "UNSIGNED " : "",
                                  tcolumn->formattedType().c_str(),
                                  reftable->primaryKey()->columns()[0]->referencedColumn()->flags().get_index(
                                    "UNSIGNED") != grt::BaseListRef::npos
                                    ? "UNSIGNED "
                                    : "",
                                  reftable->primaryKey()->columns()[0]->referencedColumn()->formattedType().c_str());

            // show a warning
            mforms::Utilities::show_message(
              "Create Foreign Key",
              base::strfmt("Referenced table has no candidate columns with a compatible type for %s.%s.%s",
                           tcolumn->owner()->name().c_str(), tcolumn->name().c_str(), hint.c_str()),
              "OK");
          }
        }
        return true;
      }
      return false;

    case Column:
      // this is not changeable
      return false;

    case RefColumn:
      return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintColumnsListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  switch (column) {
    case Enabled:
      value = grt::IntegerRef(get_column_is_fk(node) ? 1 : 0);
      return true;

    case Column:
      if (node[0] == count())
        value = grt::StringRef("");
      else
        value = _owner->get_owner()->get_table()->columns()[node[0]]->name();
      return true;

    case RefColumn: {
      db_ForeignKeyRef fk(_owner->get_selected_fk());
      db_ColumnRef tcolumn;
      ssize_t index = get_fk_column_index(node);

      if (fk.is_valid() && index >= 0 && index < (ssize_t)fk->referencedColumns().count()) {
        tcolumn = fk->referencedColumns().get(index);
        if (tcolumn.is_valid())
          value = tcolumn->name();
        else
          value = grt::StringRef("");
      } else
        value = grt::StringRef("");
    }
      return true;
  }
  return false;
}

//----------------- FKConstraintListBE ---------------------------------------------------------------------------------

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4355) // 'this' : used in base member initializer list
                                  // that's ok as we just store pointer for later use
#endif

FKConstraintListBE::FKConstraintListBE(TableEditorBE *owner)
  : _column_list(this), _owner(owner), _editing_placeholder_row(-1) {
}

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

//----------------------------------------------------------------------------------------------------------------------

NodeId FKConstraintListBE::add_column(const db_ColumnRef &column, const db_ColumnRef &refcolumn,
                                      const db_ForeignKeyRef &aFk) {
  db_ForeignKeyRef fk = aFk.is_valid() ? aFk : get_selected_fk();

  if (fk.is_valid()) {
    AutoUndoEdit undo(_owner);

    fk->columns().insert(column);
    fk->referencedColumns().insert(refcolumn);

    // make sure that there's an index that matches the columns in the FK or
    // that the one created for it is up to date
    TableHelper::update_foreign_key_index(fk);

    _owner->update_change_date();

    undo.end(strfmt(_("Add Column to FK '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));

    _column_list.refresh();

    // force a refresh of the columns list in the table figure
    // not needed anymore ((db_ColumnRef)column)->name(column->name());

    // force refresh of relationships
    // not needed anymore fk->referencedTable(fk->referencedTable());

    return NodeId(fk->columns().count() - 1);
  }
  return NodeId();
}

//----------------------------------------------------------------------------------------------------------------------

void FKConstraintListBE::remove_column(const NodeId &node) {
  db_ForeignKeyRef fk = get_selected_fk();

  db_ColumnRef column(_owner->get_table()->columns().get(node[0]));

  const size_t i = fk->columns().get_index(column);
  if (i == BaseListRef::npos) {
    // column is not set
    return;
  }

  AutoUndoEdit undo(_owner);

  fk->columns().remove(i);
  if (fk->referencedColumns().count() > i) {
    fk->referencedColumns().remove(i);
  }
  TableHelper::update_foreign_key_index(fk);

  _owner->update_change_date();
  undo.end(strfmt(_("Remove Column From FK '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));

  _column_list.refresh();
}

//----------------------------------------------------------------------------------------------------------------------

void FKConstraintListBE::select_fk(const NodeId &node) {
  _selected_fk = node;
  if (_owner->is_editing_live_object()) {
    db_ForeignKeyRef fkey = get_selected_fk();
    if (fkey.is_valid()) {
      db_TableRef ref_table = fkey->referencedTable();
      if (ref_table.is_valid()) {
        std::string schema_name = *ref_table->owner()->name();
        std::string ref_table_name = *ref_table->name();
        _owner->on_expand_live_table_stub(_owner, schema_name, ref_table_name);
      }
    }
  }
  _column_list.refresh();
}

//----------------------------------------------------------------------------------------------------------------------

db_ForeignKeyRef FKConstraintListBE::get_selected_fk() {
  if (_selected_fk.is_valid() && _selected_fk[0] < real_count())
    return _owner->get_table()->foreignKeys().get(_selected_fk[0]);
  else
    return db_ForeignKeyRef();
}

//----------------------------------------------------------------------------------------------------------------------

void FKConstraintListBE::refresh() {
}

//----------------------------------------------------------------------------------------------------------------------

size_t FKConstraintListBE::count() {
  return real_count() + 1;
}

//----------------------------------------------------------------------------------------------------------------------

size_t FKConstraintListBE::real_count() {
  return _owner->get_table()->foreignKeys().count();
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintListBE::set_field(const NodeId &node, ColumnId column, ssize_t value) {
  db_ForeignKeyRef fk;

  if (node[0] == count() - 1)
    _editing_placeholder_row = node[0];
  else
    _editing_placeholder_row = -1;

  if (node[0] >= real_count())
    return false;

  fk = _owner->get_table()->foreignKeys().get(node[0]);

  switch (column) {
    case ModelOnly:
      if ((*fk->modelOnly() != 0) != (value != 0)) {
        AutoUndoEdit undo(_owner, fk, "modelOnly");
        fk->modelOnly(value != 0);
        undo.end(strfmt(_("Toggle SQL generation for '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));
      }
      return true;
    default:
      break;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

static bool check_if_null(TableEditorBE *_owner, db_ForeignKeyRef &fk, std::string caption) {
  bool one_col_is_not_null = false;
  for (size_t i = 0; i < fk->columns().count(); i++) {
    if (fk->columns()[i]->isNotNull()) {
      one_col_is_not_null = true;
      break;
    }
  }

  if (one_col_is_not_null) {
    int ret_val = mforms::Utilities::show_warning(
      caption, _("You can't use a SET NULL action if one of the referencing columns is set to NOT NULL.\n"
                 "Would you like to revert the change or remove the NOT NULL from the column attribute?"),
      _("Remove NOT NULL"), _("Revert"));

    if (ret_val == mforms::ResultCancel)
      return false;

    AutoUndoEdit undo(_owner);
    for (size_t i = 0; i < fk->columns().count(); i++) {
      fk->columns()[i]->isNotNull(false);
    }
    _owner->update_change_date();
    (*_owner->get_table()->signal_refreshDisplay())("column");
    undo.end(strfmt(_("Remove NOT NULL for columns '%s'"), fk->owner()->name().c_str()));
    _owner->do_ui_refresh();
  }
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintListBE::set_field(const NodeId &node, ColumnId column, const std::string &value) {
  db_ForeignKeyRef fk;

  if (node[0] == real_count() && column == Name) {
    if (value.empty())
      return false;
    _editing_placeholder_row = -1;
    _owner->add_fk(value);
    return true;
  } else if (node[0] >= real_count())
    return false;

  fk = _owner->get_table()->foreignKeys().get(node[0]);

  switch (column) {
    case Name:
      if (fk->name() != value) {
        AutoUndoEdit undo(_owner, fk, "name");
        TableHelper::rename_foreign_key(_owner->get_table(), fk, value);
        _owner->update_change_date();
        undo.end(_("Rename FK"));
      }
      return true;
    case OnDelete:
      if (fk->deleteRule() != value) {
        if (value.find("SET NULL") != std::string::npos) {
          if (!check_if_null(_owner, fk, _("On Delete Action")))
            return false;
        }
        AutoUndoEdit undo(_owner);

        fk->deleteRule(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Change ON DELETE for FK '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));
      }
      return true;
    case OnUpdate:
      if (fk->updateRule() != value) {
        if (value.find("SET NULL") != std::string::npos) {
          if (!check_if_null(_owner, fk, _("On Update Action")))
            return false;
        }
        AutoUndoEdit undo(_owner);

        fk->updateRule(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Change ON UPDATE for FK '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));
      }
      return true;
    case RefTable: {
      std::vector<std::string> parts = base::split_qualified_identifier(value);
      std::string table;
      db_SchemaRef schema;

      if (!parts.empty()) {
        if (value == "Specify Manually...") {
          std::string table_name;
          if (mforms::Utilities::request_input(_("Specify Referenced Table Manually"), _("Qualified Table Name:"), "",
                                               table_name)) {
            parts = base::split_qualified_identifier(table_name);
          } else {
            parts.clear();
          }
          if (parts.empty())
            return false;
        }

        if (_owner->is_editing_live_object()) {
          if (parts.size() == 1)
            parts.insert(parts.begin(), *_owner->get_schema()->name());
          if (!_owner->on_expand_live_table_stub(_owner, parts[0], parts[1]))
            return false;
        }

        if (parts.size() == 1) {
          table = parts[0];
          schema = _owner->get_schema();
        } else {
          table = parts[1];
          schema = _owner->get_schema_with_name(parts[0]);
        }

        db_TableRef dbtable;
        if (schema.is_valid()) {
          dbtable = grt::find_named_object_in_list(schema->tables(), table, "name");
          if (!dbtable.is_valid())
            return false;
        }

        if (!dbtable.is_valid() || dbtable != fk->referencedTable()) {
          AutoUndoEdit undo(_owner);

          {
            std::string schema_name = schema.is_valid() ? *schema->name() : parts[0];
            logInfo("Creating stub schema and table %s.%s for foreign key\n", schema_name.c_str(), table.c_str());
            dbtable = _owner->create_stub_table(schema_name, table);
            if (!dbtable.is_valid())
              return false;
          }

          fk->referencedTable(dbtable);

          // reset the currently set columns
          fk->columns().remove_all();
          fk->referencedColumns().remove_all();

          get_columns()->refresh();

          TableHelper::update_foreign_key_index(fk);

          _owner->update_change_date();
          bec::ValidationManager::validate_instance(_owner->get_table(), "chk_fk_lgc");
          bec::ValidationManager::validate_instance(dbtable, "chk_fk_lgc");

          undo.end(strfmt(_("Change Ref. Table for FK '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));
        }
      }
    }
      return true;
    case Comment:
      if (fk->comment() != value) {
        AutoUndoEdit undo(_owner, fk, "comment");

        fk->comment(value);
        _owner->update_change_date();

        undo.end(strfmt(_("Change Comment for FK '%s.%s'"), _owner->get_name().c_str(), fk->name().c_str()));
      }
      return true;

    case Index:
      // cannot change index, its auto-created/maintained
      return false;
    case ModelOnly: // handled as int
      return false;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintListBE::get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value) {
  db_ForeignKeyRef fk;

  if (node[0] < real_count())
    fk = _owner->get_table()->foreignKeys().get(node[0]);

  switch (column) {
    case Name:
      if (fk.is_valid())
        value = fk->name();
      else if (_editing_placeholder_row == node[0]) {
        std::string temp =
          base::replaceString(getTemplate(workbench_physical_ModelRef::cast_from(_owner->get_catalog()->owner()),
                                          "FKNameTemplate", _owner->is_editing_live_object()),
                              "%stable%", _owner->get_name().c_str());

        value = grt::StringRef(get_name_suggestion_for_list_object(_owner->get_table()->foreignKeys(),
                                                                   base::replaceString(temp, "%dtable%", ""), true));
      } else
        value = grt::StringRef("");
      return true;
    case OnDelete:
      if (fk.is_valid()) {
        if (fk->deleteRule().empty())
          value = grt::StringRef("RESTRICT"); // empty means default means RESTRICT
        else
          value = fk->deleteRule();
      } else
        value = grt::StringRef("");
      return true;
    case OnUpdate:
      if (fk.is_valid()) {
        if (fk->updateRule().empty())
          value = grt::StringRef("RESTRICT"); // empty means default means RESTRICT
        else
          value = fk->updateRule();
      } else
        value = grt::StringRef("");
      return true;
    case RefTable:
      if (fk.is_valid() && fk->referencedTable().is_valid())
        value =
          grt::StringRef("`" + *fk->referencedTable()->owner()->name() + "`.`" + *fk->referencedTable()->name() + "`");
      else
        value = grt::StringRef("");
      return true;
    case Comment:
      if (fk.is_valid())
        value = fk->comment();
      else
        value = grt::StringRef("");
      return true;
    case Index:
      if (fk.is_valid()) {
        if (fk->index().is_valid())
          value = fk->index()->name();
        else
          value = grt::StringRef(""); // a FK with no index means it just didn't have one created for it
                                      // value = grt::StringRef("INVALID");
      } else
        value = grt::StringRef("");
      return true;
    case ModelOnly:
      if (fk.is_valid()) {
        value = grt::IntegerRef(*fk->modelOnly());
        return true;
      }
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintListBE::can_delete_node(const NodeId &node) {
  return node.is_valid() && node[0] < real_count();
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintListBE::delete_node(const NodeId &node) {
  if (!can_delete_node(node))
    return false;

  _owner->remove_fk(node);

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

MenuItemList FKConstraintListBE::get_popup_items_for_nodes(const std::vector<NodeId> &nodes) {
  MenuItemList items;

  MenuItem item;
  item.caption = _("Delete selected");
  item.internalName = "deleteSelectedFKs";
  item.accessibilityName = "Delete Selected";
  item.enabled = (nodes.size() >= 1);
  items.push_back(item);

  return items;
}

//----------------------------------------------------------------------------------------------------------------------

bool FKConstraintListBE::activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes) {
  std::vector<NodeId> sorted_nodes(nodes);
  std::sort(sorted_nodes.begin(), sorted_nodes.end());

  if (name == "deleteSelectedFKs") {
    for (ssize_t i = sorted_nodes.size() - 1; i >= 0; --i)
      delete_node(sorted_nodes[i]);
  } else
    return false;

  return true;
}

//----------------- TableEditorBE --------------------------------------------------------------------------------------

#ifdef _MSC_VER
  #pragma warning(push)
  #pragma warning(disable : 4355) // 'this' : used in base member initializer list
                                  // that's ok as we just store pointer for later use
#endif

TableEditorBE::TableEditorBE(const db_TableRef &table) : DBObjectEditorBE(table), _fk_list(this) {
  _inserts_panel = NULL;
  _inserts_grid = NULL;

  if (table.class_name() == "db.Table")
    throw std::logic_error("table object is abstract");

  scoped_connect(get_catalog()->signal_changed(),
                 std::bind(&TableEditorBE::catalogChanged, this, std::placeholders::_1, std::placeholders::_2));
}

#ifdef _MSC_VER
  #pragma warning(pop)
#endif

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::set_name(const std::string &name) {
  if (name != get_name()) {
    RefreshUI::Blocker __centry(*this);

    AutoUndoEdit undo(this, get_object(), "name");
    bec::ValidationManager::validate_instance(get_table(), CHECK_NAME);
    std::string name_ = base::trim_right(name);
    get_dbobject()->name(name_);
    undo.end(strfmt(_("Rename Table to '%s'"), name_.c_str()));
  }
}

//----------------------------------------------------------------------------------------------------------------------

NodeId TableEditorBE::add_column(const std::string &name) {
  db_ColumnRef column;

  column = grt::GRT::get()->create_object<db_Column>(
    get_table().get_metaclass()->get_member_type("columns").content.object_class);

  column->name(name);
  column->owner(get_table());

  AutoUndoEdit undo(this);
  get_table()->addColumn(column);
  update_change_date();
  undo.end(strfmt(_("Add Column '%s' to '%s'"), name.c_str(), get_name().c_str()));

  get_columns()->refresh();
  column_count_changed();

  bec::ValidationManager::validate_instance(column, CHECK_NAME);
  bec::ValidationManager::validate_instance(get_table(), "columns-count");
  return NodeId(get_table()->columns().count() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

NodeId TableEditorBE::duplicate_column(const db_ColumnRef &column, ssize_t insert_after) {
  db_ColumnRef new_column = shallow_copy_object(column);
  new_column->oldName(""); // It's new column, so it shouldn't have any old name set.

  std::string name = new_column->name();
  std::string new_name = name;
  for (int i = 1;; i++) {
    if (!find_named_object_in_list(get_table()->columns(), new_name).is_valid())
      break;
    new_name = strfmt("%s_copy%i", name.c_str(), i);
  }
  if (new_name != *new_column->name())
    new_column->name(new_name);
  new_column->owner(get_table());
  get_table()->addColumn(new_column);
  if (insert_after >= 0)
    get_table()->columns()->reorder(get_table()->columns()->get_index(new_column), insert_after);

  bec::ValidationManager::validate_instance(new_column, CHECK_NAME);
  bec::ValidationManager::validate_instance(get_table(), "columns-count");

  column_count_changed();

  return NodeId(get_table()->columns().count() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

db_ColumnRef TableEditorBE::get_column_with_name(const std::string &name) {
  return grt::find_named_object_in_list<db_Column>(get_table()->columns(), name, "name");
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::rename_column(const db_ColumnRef &column, const std::string &name) {
  std::string old_name = column->name();

  AutoUndoEdit undo(this);

  ((db_ColumnRef)column)->name(name);
  update_change_date();

  undo.end(strfmt(_("Rename '%s.%s' to '%s'"), get_name().c_str(), old_name.c_str(), name.c_str()));
  bec::ValidationManager::validate_instance(column, CHECK_NAME);

  column_count_changed();
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::remove_column(const NodeId &node) {
  db_TableRef table = get_table();
  if (node[0] >= table->columns().count())
    return;

  db_ColumnRef column = get_table()->columns()[node[0]];

  AutoUndoEdit undo(this);

  table->removeColumn(column);

  undo.end(strfmt(_("Remove '%s.%s'"), get_name().c_str(), column->name().c_str()));

  get_columns()->refresh();
  bec::ValidationManager::validate_instance(get_table(), "columns-count");

  column_count_changed();
}

//----------------------------------------------------------------------------------------------------------------------

NodeId TableEditorBE::add_fk(const std::string &name) {
  if (!get_table()->columns().count()) {
    mforms::Utilities::show_warning("FK Creation", "Cannot add FK on empty table, add some columns first", "OK");
    return NodeId();
  }
  ListRef<db_ForeignKey> fklist = get_table()->foreignKeys();
  db_ForeignKeyRef fk;

  AutoUndoEdit undo(this);

  fk = TableHelper::create_empty_foreign_key(get_table(), name);

  auto model = workbench_physical_ModelRef::cast_from(get_catalog()->owner());
  fk->updateRule(StringRef(getTemplate(model, "db.ForeignKey:updateRule",
                                       is_editing_live_object())));

  fk->deleteRule(StringRef(getTemplate(model, "db.ForeignKey:deleteRule",
                                       is_editing_live_object())));

  update_change_date();

  undo.end(strfmt(_("Add Foreign Key '%s' to '%s'"), name.c_str(), get_name().c_str()));

  _fk_list.refresh();

  bec::ValidationManager::validate_instance(fk, CHECK_NAME);

  return NodeId(fklist.count() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

bool TableEditorBE::remove_fk(const NodeId &fk) {
  ListRef<db_ForeignKey> fklist = get_table()->foreignKeys();

  if (fk[0] >= fklist.count())
    return false;

  const db_TableRef ref_table = fklist[fk[0]]->referencedTable();

  AutoUndoEdit undo(this);
  std::string name = fklist[fk[0]]->name();
  get_table()->removeForeignKey(fklist[fk[0]], false);
  update_change_date();
  undo.end(strfmt(_("Remove Foreign Key '%s'.'%s'"), get_name().c_str(), name.c_str()));

  _fk_list.refresh();

  // There might be no referenced table yet.
  if (ref_table.is_valid())
    bec::ValidationManager::validate_instance(ref_table, "chk_fk_lgc");
  bec::ValidationManager::validate_instance(get_table(), "chk_fk_lgc");

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

NodeId TableEditorBE::add_index(const std::string &name) {
  if (!get_table()->columns().count()) {
    mforms::Utilities::show_warning("Index Creation", "Cannot add Index on empty table, add some columns first", "OK");
    return NodeId();
  }

  ListRef<db_Index> indices = get_table()->indices();
  db_IndexRef index;

  if (indices.content_class_name() == "db.Index")
    throw std::logic_error("internal bug");

  index = grt::GRT::get()->create_object<db_Index>(indices.content_class_name());
  index->name(name);
  index->owner(get_table());

  std::vector<std::string> types;
  types = get_index_types();

  index->indexType(types[0]);

  AutoUndoEdit undo(this);

  update_change_date();
  indices.insert(index);

  undo.end(strfmt(_("Add Index '%s' to '%s'"), name.c_str(), get_name().c_str()));

  get_indexes()->refresh();

  bec::ValidationManager::validate_instance(index, CHECK_NAME);
  bec::ValidationManager::validate_instance(get_table(), CHECK_EFFICIENCY);

  return NodeId(indices.count() - 1);
}

//----------------------------------------------------------------------------------------------------------------------

bool TableEditorBE::remove_index(const NodeId &index, bool delete_even_if_foreign) {
  if (index[0] >= get_table()->indices().count())
    return false;

  db_IndexRef indexobj(get_table()->indices()[index[0]]);
  db_ForeignKeyRef fk;

  // do not allow removal of PRIMARY or FOREIGN indices by user
  if (!get_indexes()->index_editable(indexobj) ||
      ((fk = get_indexes()->index_belongs_to_fk(indexobj)).is_valid() && !delete_even_if_foreign))
    return false;

  AutoUndoEdit undo(this);
  get_table()->indices().remove_value(indexobj);
  get_indexes()->refresh();
  if (fk.is_valid())
    fk->index(db_IndexRef());
  update_change_date();
  undo.end(strfmt(_("Remove Index '%s'.'%s'"), indexobj->name().c_str(), get_name().c_str()));

  bec::ValidationManager::validate_instance(get_table(), CHECK_EFFICIENCY);

  return true;
}

//----------------------------------------------------------------------------------------------------------------------

NodeId TableEditorBE::add_index_with_columns(const std::vector<NodeId> &columns) {
  AutoUndoEdit undo(this);

  NodeId id = add_index(get_name_suggestion_for_list_object(get_table()->indices(), "index"));
  db_TableRef table = get_table();
  db_IndexRef index = table->indices()[id[0]];
  ListRef<db_Column> clist = table->columns();

  for (std::vector<NodeId>::const_iterator i = columns.begin(); i != columns.end(); ++i) {
    db_ColumnRef column = clist[(*i)[0]];

    get_indexes()->add_column(column, index);
  }

  update_change_date();
  undo.end(strfmt(_("Add Index '%s' to '%s'"), index->name().c_str(), get_name().c_str()));

  bec::ValidationManager::validate_instance(index, CHECK_NAME);

  return id;
}

//----------------------------------------------------------------------------------------------------------------------

NodeId TableEditorBE::add_fk_with_columns(const std::vector<NodeId> &columns) {
  AutoUndoEdit undo(this);

  NodeId id = add_fk(get_name_suggestion_for_list_object(get_table()->foreignKeys(), "fk"));
  db_TableRef table = get_table();
  db_ForeignKeyRef fk = table->foreignKeys()[id[0]];
  ListRef<db_Column> clist = table->columns();

  for (std::vector<NodeId>::const_iterator i = columns.begin(); i != columns.end(); ++i) {
    db_ColumnRef column = clist[(*i)[0]];

    _fk_list.add_column(column, db_ColumnRef(), fk);
  }

  update_change_date();
  undo.end(strfmt(_("Add Foreign Key '%s' to '%s'"), fk->name().c_str(), get_name().c_str()));

  bec::ValidationManager::validate_instance(fk, CHECK_NAME);

  return id;
}

void TableEditorBE::undo_called(grt::UndoAction *action, grt::UndoAction *expected) {
  if (action == expected)
    do_ui_refresh();
}

//----------------------------------------------------------------------------------------------------------------------

bool TableEditorBE::showErrorMessage(const std::string &type) {
  bool ret = false;
  std::string key = base::tolower(type);
  if (key == "json") {
    GrtVersionRef version = GrtVersionRef::cast_from(bec::getModelOption(workbench_physical_ModelRef::cast_from(get_catalog()->owner()), "CatalogVersion"));
    bool versionCheck = bec::is_supported_mysql_version_at_least(version, 5, 7, 8);
    if (!versionCheck) {
      mforms::Utilities::show_message(
        _("Type not supported"), "The JSON data is not available before MySQL 5.7.8. In order to use it, first set the "
        "version for your model to 5.7.8 or higher", _("Ok"));
      ret = true;
    }
  }
  return ret;
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::catalogChanged(const std::string &member, const grt::ValueRef &value) {
  if (member == "version") {
    GrtVersionRef version = GrtVersionRef::cast_from(value);
    GrtVersionRef actualVersion = get_catalog()->version();
    if (!bec::version_greater(version, actualVersion))
      return;
    if (bec::is_supported_mysql_version_at_least(actualVersion, 5, 7, 7))
      return;

    bool messageShown = false;
    int ret = mforms::ResultOther;
    grt::ListRef<db_Schema> schemas = get_catalog()->schemata();
    for (ssize_t i = schemas.count() - 1; i >= 0; --i) {
      if (!schemas[i].is_valid())
        continue;
      grt::ListRef<db_Table> tables = schemas[i]->tables();
      for (ssize_t j = tables.count() - 1; j >= 0; --j) {
        grt::ListRef<db_Column> columns = tables[j]->columns();
        for (ssize_t k = columns.count() - 1; k >= 0; --k) {
          if (!(columns[k].is_valid() && columns[k]->simpleType().is_valid()))
            continue;
          if (!base::same_string(columns[k]->simpleType()->name(), "json", false))
            continue;
          if (!messageShown) {
            ret = mforms::Utilities::show_message(_("Model downgrade"),
                                                  "You have at least one column definition with a data type that is "
                                                  "not supported by this server version (JSON)."
                                                  "If you continue, this data type will be converted to TEXT. Do you "
                                                  "want to apply this change, ignore the incompatibility or cancel the "
                                                  "version change?",
                                                  _("Apply"), _("Cancel"), _("Ignore"));
            messageShown = true;
            if (ret == mforms::ResultCancel) {
              grt::UndoManager *um = grt::GRT::get()->get_undo_manager();
              assert(um != NULL);
              UndoAction *action = um->get_latest_undo_action();
              if (action != NULL && action->description() == "version")
                action->undo(um);
              return;
            } else if (ret == mforms::ResultOther)
              return;
          }
          columns[k]->setParseType("text", get_catalog()->simpleDatatypes());
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool TableEditorBE::parse_column_type(const std::string &str, db_ColumnRef &column) {
  db_CatalogRef catalog(get_catalog());

  bool flag = column->setParseType(str, catalog->simpleDatatypes()) == 1;
  if (flag) {
    grt::UndoManager *um = grt::GRT::get()->get_undo_manager();

    // call _refresh_ui when this parse column type action is undone
    // XXX: everytime we parse a column type 2 new connections are added without removing the old ones!
    scoped_connect(um->signal_undo(),
                   std::bind(&TableEditorBE::undo_called, this, std::placeholders::_1, um->get_latest_undo_action()));
    scoped_connect(um->signal_redo(),
                   std::bind(&TableEditorBE::undo_called, this, std::placeholders::_1, um->get_latest_undo_action()));
  }
  return flag;
}

std::string TableEditorBE::format_column_type(db_ColumnRef &column) {
  return column->formattedRawType();
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::inserts_column_resized(int column) {
  int width = _inserts_grid->get_column_width(column);

  grt::IntegerListRef widths;
  if (grt::IntegerListRef::can_wrap(get_table()->customData().get("InsertsColumnWidths")))
    widths = grt::IntegerListRef::cast_from(get_table()->customData().get("InsertsColumnWidths"));
  else {
    widths = grt::IntegerListRef(grt::Initialized);
    get_table()->customData().set("InsertsColumnWidths", widths);
  }

  while (column >= (int)widths.count())
    widths.insert(grt::IntegerRef(0));

  widths.set(column, grt::IntegerRef(width));
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::restore_inserts_columns() {
  grt::IntegerListRef widths;
  if (grt::IntegerListRef::can_wrap(get_table()->customData().get("InsertsColumnWidths")))
    widths = grt::IntegerListRef::cast_from(get_table()->customData().get("InsertsColumnWidths"));

  for (int i = 0; i < _inserts_grid->get_column_count(); i++) {
    bool flag = false;
    if (widths.is_valid() && i < (int)widths.count()) {
      int width = (int)widths[i];
      if (width > 0) {
        _inserts_grid->set_column_width(i, width);
        flag = true;
      }
    }
    if (!flag && (int)get_table()->columns().count() > i) {
      // set a default
      db_ColumnRef column(get_table()->columns()[i]);
      if (column.is_valid() && column->simpleType().is_valid()) {
        std::string type_group = column->simpleType()->group()->name();
        if (type_group == "string") {
          _inserts_grid->set_column_width(i, std::min((int)column->length() * 15, 200));
        } else if (type_group == "numeric")
          _inserts_grid->set_column_width(i, 80);
        else
          _inserts_grid->set_column_width(i, 150);
      } else
        _inserts_grid->set_column_width(i, 100);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::open_field_editor(int row, int column) {
  Recordset::Ref rset(get_inserts_model());
  if (rset) {
    std::string type;
    db_ColumnRef tcolumn(get_table()->columns()[column]);
    if (tcolumn.is_valid()) {
      if (tcolumn->simpleType().is_valid())
        type = tcolumn->simpleType()->name();
      else if (tcolumn->userType().is_valid() && tcolumn->userType()->actualType().is_valid())
        type = tcolumn->userType()->actualType()->name();
    }
    rset->open_field_data_editor(row, column, type);
  }
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::update_selection_for_menu_extra(mforms::ContextMenu *menu, const std::vector<int> &rows,
                                                    int column) {
  mforms::MenuItem *item = menu->find_item("edit_cell");
  if (item != NULL && !rows.empty()) {
    if (item->signal_clicked()->empty())
      item->signal_clicked()->connect(std::bind(&TableEditorBE::open_field_editor, this, rows[0], column));
  }
}

//----------------------------------------------------------------------------------------------------------------------

// used in unit-tests
Recordset::Ref TableEditorBE::get_inserts_model() {
  if (!_inserts_model) {
    if (get_table().class_name() == "db.Table")
      throw std::logic_error("table object is abstract");

    _inserts_storage = Recordset_table_inserts_storage::create();
    _inserts_storage->table(get_table());

    _inserts_model = Recordset::create();
    _inserts_model->update_selection_for_menu_extra =
      std::bind(&TableEditorBE::update_selection_for_menu_extra, this, std::placeholders::_1, std::placeholders::_2,
                std::placeholders::_3);
    _inserts_model->set_inserts_editor(true);
    _inserts_model->data_storage(_inserts_storage);
    _inserts_model->refresh();
  }
  return _inserts_model;
}

//----------------------------------------------------------------------------------------------------------------------

mforms::View *TableEditorBE::get_inserts_panel() {
  if (!_inserts_panel) {
    mforms::ToolBar *tbar = get_inserts_model()->get_toolbar();
    tbar->find_item("record_export")
      ->signal_activated()
      ->connect(std::bind(&TableEditorBE::show_export_wizard, this, (mforms::Form *)0));
    if (tbar->find_item("record_import"))
      tbar->find_item("record_import")
        ->signal_activated()
        ->connect(std::bind(&TableEditorBE::show_import_wizard, this));

    _inserts_grid = mforms::GridView::create(get_inserts_model());
    restore_inserts_columns();
    _inserts_grid->signal_column_resized()->connect(
      std::bind(&TableEditorBE::inserts_column_resized, this, std::placeholders::_1));

    _inserts_panel = mforms::manage(new mforms::Box(false));
    _inserts_panel->add(mforms::manage(tbar), false, true);
    _inserts_panel->add(mforms::manage(_inserts_grid), true, true);
  }
  return _inserts_panel;
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::show_export_wizard(mforms::Form *owner) {
  if (_inserts_model && _inserts_model->count() > 0) {
    grt::ValueRef option(bec::GRTManager::get()->get_app_option("TableEditor:LastExportDirectory"));
    std::string path = option.is_valid() ? grt::StringRef::cast_from(option) : "";
    option = bec::GRTManager::get()->get_app_option("TableEditor:LastExportExtension");
    std::string extension = option.is_valid() ? grt::StringRef::cast_from(option) : "";
    InsertsExportForm exporter(owner, _inserts_model, extension);
    exporter.set_title(strfmt(_("Export Inserts for %s"), get_name().c_str()));
    if (!path.empty()) {
      path = base::makePath(path, get_name());
      exporter.set_path(path);
    }
    path = exporter.run();
    if (path.empty())
      bec::GRTManager::get()->replace_status_text(_("Export inserts canceled"));
    else {
      bec::GRTManager::get()->replace_status_text(strfmt(_("Exported inserts to %s"), path.c_str()));
      bec::GRTManager::get()->set_app_option("TableEditor:LastExportDirectory",
                                             grt::StringRef(exporter.get_directory()));
      extension = base::extension(path);
      if (!extension.empty() && extension[0] == '.')
        extension = extension.substr(1);
      if (!extension.empty())
        bec::GRTManager::get()->set_app_option("TableEditor:LastExportExtension", grt::StringRef(extension));
    }
  } else
    mforms::Utilities::show_message("Export Data", "No data to be exported.", "OK");
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::show_import_wizard() {
  grt::BaseListRef args(true);

  db_TableRef table(get_table());
  if (table.is_valid() && table->columns().count() > 0) {
    args.ginsert(grtwrap_editablerecordset(table, _inserts_model));

    grt::Module *module = grt::GRT::get()->get_module("SQLIDEUtils");
    if (module) {
      try {
        module->call_function("importRecordsetDataFromFile", args);
      } catch (grt::module_error &exc) {
        mforms::Utilities::show_error("Import Standard INSERTs", "Error during data import:\n" + exc.inner, "OK");
      } catch (std::exception &exc) {
        mforms::Utilities::show_error("Import Standard INSERTs",
                                      "Error during data import:\n" + std::string(exc.what()), "OK");
      }
    } else
      logError("Can't find module SQLIDEUtils for record importer\n");
  }
}

//----------------------------------------------------------------------------------------------------------------------

MySQLEditor::Ref TableEditorBE::get_sql_editor() {
  MySQLEditor::Ref sql_editor = DBObjectEditorBE::get_sql_editor();
  if (sql_editor)
    sql_editor->restrict_content_to(MySQLEditor::ContentTypeTrigger);
  return sql_editor;
}

//----------------------------------------------------------------------------------------------------------------------

std::string TableEditorBE::get_title() {
  return base::strfmt("%s - Table", get_name().c_str());
}

//----------------------------------------------------------------------------------------------------------------------

bool TableEditorBE::can_close() {
  if (_inserts_grid && _inserts_model->has_pending_changes()) {
    int ret = mforms::Utilities::show_message(
      "Close Table Editor",
      base::strfmt("There are unsaved changes to the INSERTs data for %s. If you do not save, "
        "these changes will be discarded.", get_name().c_str()
      ),
      "Save Changes", "Cancel", "Don't Save");

    if (ret == mforms::ResultOk)
      _inserts_model->apply_changes();
    else if (ret == mforms::ResultOther)
      _inserts_model->rollback();
    else
      return false;
  }
  return DBObjectEditorBE::can_close();
}

//----------------------------------------------------------------------------------------------------------------------

void TableEditorBE::column_count_changed() {
  if (_inserts_model)
    _inserts_model->refresh();
  if (_inserts_grid)
    _inserts_grid->update_columns();
}

//----------------------------------------------------------------------------------------------------------------------
