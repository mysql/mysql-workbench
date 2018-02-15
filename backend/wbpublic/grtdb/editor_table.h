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

#include "grtdb/editor_dbobject.h"
#include "grt/tree_model.h"

#include "grtdb/charset_list.h"

#include "grts/structs.db.mgmt.h"

#include "wbpublic_public_interface.h"

#define TableEditorBE_VERSION 2

class Recordset;
typedef std::shared_ptr<Recordset> RecordsetRef;
class Recordset_table_inserts_storage;
typedef std::shared_ptr<Recordset_table_inserts_storage> RecordsetTableInsertsStorageRef;

namespace mforms {
  class Form;
  class View;
  class Box;
  class ContextMenu;
  class GridView;
}

namespace bec {

  class TableEditorBE;
  class IndexListBE;
  class FKConstraintListBE;

  // ColumnNamesSet sets alias for type which is used to return a set of all column
  // names from all tables in the schema. This type is used by TableColumnsListBE
  typedef std::set<std::string> ColumnNamesSet;

  class WBPUBLICBACKEND_PUBLIC_FUNC TableColumnsListBE : public ListModel {
  public:
    enum ColumnListColumns {
      Name,
      Type,
      IsPK,
      IsNotNull,
      IsUnique,
      IsBinary,
      IsUnsigned,
      IsZerofill,
      Flags,
      Default,
      CharsetCollation,
      Charset,
      Collation,
      Comment,
      HasCharset,

      LastColumn
    };

    TableColumnsListBE(TableEditorBE *owner);

    bool get_row(const NodeId &node, std::string &name, std::string &type, bool &ispk, bool &notnull, bool &isunique,
                 bool &isbinary, bool &isunsigned, bool &iszerofill, std::string &flags, std::string &defvalue,
                 std::string &charset, std::string &collation, std::string &comment);

    virtual IconId get_field_icon(const NodeId &node, size_t column, IconSize size);

    virtual void refresh();
    virtual size_t count();
    size_t real_count();

    bool set_column_type(const NodeId &node, const GrtObjectRef &type);

    bool set_column_type_from_string(db_ColumnRef &column, const std::string &type);

    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);
    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);

    /**
     * This is needed so we can reset placeholder info when then user cancelled the edit operation.
     * Used in gtk frontend.
     */
    void reset_placeholder();

    virtual void reorder(const NodeId &node, size_t nindex);
    void reorder_many(const std::vector<std::size_t> &rows, std::size_t nindex);

    std::vector<std::string> get_datatype_flags(const ::bec::NodeId &node, bool all = false);
    bool set_column_flag(const ::bec::NodeId &node, const std::string &flag_name, int is_set);
    int get_column_flag(const ::bec::NodeId &node, const std::string &flag_name);

    virtual std::string quote_value_if_needed(const db_ColumnRef &column, const std::string &value);
    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes);
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes);

    virtual bool can_delete_node(const NodeId &node);
    virtual bool delete_node(const NodeId &node);

    virtual std::vector<std::string> get_datatype_names();

    ColumnNamesSet get_column_names_completion_list() const;

    bool has_unique_index(const db_ColumnRef &col);
    bool make_unique_index(const db_ColumnRef &col, bool flag);

  protected:
    TableEditorBE *_owner;
    size_t _editing_placeholder_row;

    void update_primary_index_order();

    // for internal use only
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC IndexColumnsListBE : public ListModel {
  public:
    enum IndexColumnsListColumns { Name, Descending, Length, OrderIndex };

    IndexColumnsListBE(IndexListBE *owner);

    virtual void refresh();
    virtual size_t count();

    void set_column_enabled(const NodeId &node, bool flag);
    bool get_column_enabled(const NodeId &node);

    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);
    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);

    size_t get_max_order_index();

  protected:
    IndexListBE *_owner;

    // for internal use only
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);

    db_IndexColumnRef get_index_column(const db_ColumnRef &column);
    size_t get_index_column_index(const db_ColumnRef &column);
    void set_index_column_order(const db_IndexColumnRef &column, size_t order);
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC IndexListBE : public ListModel {
    friend class IndexColumnsListBE;
    friend class TableEditorBE;

  public:
    enum IndexListColumns { Name, Type, Visible, Comment, LastColumn };

    IndexListBE(TableEditorBE *owner);

    virtual void refresh();
    virtual size_t count();
    size_t real_count();

    // for editable lists only
    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);

    IndexColumnsListBE *get_columns() {
      return &_column_list;
    }

    db_IndexRef get_selected_index();
    void select_index(const NodeId &node);

    bool index_editable(const db_IndexRef &index);
    db_ForeignKeyRef index_belongs_to_fk(const db_IndexRef &index);

    TableEditorBE *get_owner() {
      return _owner;
    }

    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes);
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes);

    virtual bool can_delete_node(const NodeId &node);
    virtual bool delete_node(const NodeId &node);

  protected:
    // for internal use only
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);

    NodeId add_column(const db_ColumnRef &column, const db_IndexRef &index = db_IndexRef());
    void remove_column(const NodeId &node);

  protected:
    IndexColumnsListBE _column_list;
    TableEditorBE *_owner;
    NodeId _selected;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC FKConstraintColumnsListBE : public ListModel {
  public:
    enum FKConstraintColumnsListColumns { Enabled, Column, RefColumn };

    FKConstraintColumnsListBE(FKConstraintListBE *owner);

    virtual void refresh();
    virtual size_t count();

    std::vector<std::string> get_ref_columns_list(const NodeId &node, bool filtered = true);

    // for editable lists only
    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);
    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);

    bool set_column_is_fk(const NodeId &node, bool flag);
    ssize_t get_fk_column_index(const NodeId &node);
    bool get_column_is_fk(const NodeId &node);

    FKConstraintListBE *get_owner() {
      return _owner;
    }

  protected:
    // for internal use only
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);

    bool set_fk_column_pair(const db_ColumnRef &column, const db_ColumnRef &refcolumn);

    // temporary list of referenced columns for each FK column
    // if id is in the map, then it's enabled, if column is nil, it's unset
    // only valid entries will be committed to actual table
    std::map<std::string, db_ColumnRef> _referenced_columns;

    FKConstraintListBE *_owner;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC FKConstraintListBE : public ListModel {
    friend class FKConstraintColumnsListBE;

  public:
    enum FKConstraintListColumns { Name, OnDelete, OnUpdate, RefTable, Comment, Index, ModelOnly };
    FKConstraintListBE(TableEditorBE *owner);

    NodeId add_column(const db_ColumnRef &column, const db_ColumnRef &refcolumn,
                      const db_ForeignKeyRef &fk = db_ForeignKeyRef());

    virtual void remove_column(const NodeId &node);

    virtual void refresh();
    virtual size_t count();
    size_t real_count();

    // for editable lists only
    virtual bool set_field(const NodeId &node, ColumnId column, const std::string &value);
    virtual bool set_field(const NodeId &node, ColumnId column, ssize_t value);

    void select_fk(const NodeId &node);
    db_ForeignKeyRef get_selected_fk();

    TableEditorBE *get_owner() {
      return _owner;
    }

    FKConstraintColumnsListBE *get_columns() {
      return &_column_list;
    }

    virtual bool can_delete_node(const NodeId &node);
    virtual bool delete_node(const NodeId &node);

    virtual MenuItemList get_popup_items_for_nodes(const std::vector<NodeId> &nodes);
    virtual bool activate_popup_item_for_nodes(const std::string &name, const std::vector<NodeId> &nodes);

  protected:
    // for internal use only
    virtual bool get_field_grt(const NodeId &node, ColumnId column, grt::ValueRef &value);

  protected:
    FKConstraintColumnsListBE _column_list;
    TableEditorBE *_owner;
    NodeId _selected_fk;
    size_t _editing_placeholder_row;
  };

  class WBPUBLICBACKEND_PUBLIC_FUNC TableEditorBE : public DBObjectEditorBE {
  public:
    enum PartialRefreshes { RefreshColumnMoveUp, RefreshColumnMoveDown, RefreshColumnList, RefreshColumnCollation };

    TableEditorBE(const db_TableRef &table);

    virtual std::string get_title();
    virtual bool can_close();

    db_TableRef get_table() {
      return db_TableRef::cast_from(get_object());
    };

    virtual TableColumnsListBE *get_columns() = 0;
    virtual IndexListBE *get_indexes() = 0;
    FKConstraintListBE *get_fks() {
      return &_fk_list;
    }

    RecordsetRef get_inserts_model();
    mforms::View *get_inserts_panel();

    virtual void set_name(const std::string &name);

    // table options
    virtual void set_table_option_by_name(const std::string &name, const std::string &value) = 0;
    virtual std::string get_table_option_by_name(const std::string &name) = 0;

    // column editing
    virtual NodeId add_column(const std::string &name);
    virtual void remove_column(const NodeId &column);
    void rename_column(const db_ColumnRef &column, const std::string &name);
    NodeId duplicate_column(const db_ColumnRef &col, ssize_t insert_after = -1);

    db_ColumnRef get_column_with_name(const std::string &name);

    // fk editing
    virtual NodeId add_fk(const std::string &name);
    virtual bool remove_fk(const NodeId &fk);
    virtual NodeId add_fk_with_columns(const std::vector<NodeId> &columns);
    virtual bool check_column_referenceable_by_fk(const db_ColumnRef &column1, const db_ColumnRef &column2) = 0;

    // index editing
    virtual NodeId add_index(const std::string &name);
    virtual bool remove_index(const NodeId &index, bool delete_even_if_foreign);

    virtual NodeId add_index_with_columns(const std::vector<NodeId> &columns);

    // helper utils for columns
    virtual bool parse_column_type(const std::string &str, db_ColumnRef &column);
    virtual std::string format_column_type(db_ColumnRef &column);

    virtual std::vector<std::string> get_index_types() = 0;

    void show_export_wizard(mforms::Form *owner);
    void show_import_wizard();

    virtual MySQLEditor::Ref get_sql_editor();

    virtual db_TableRef create_stub_table(const std::string &schema, const std::string &table) = 0;

    void column_count_changed();
    bool showErrorMessage(const std::string &type);

  protected:
    FKConstraintListBE _fk_list;

    void undo_called(grt::UndoAction *action, grt::UndoAction *expected);

  private:
    mforms::Box *_inserts_panel;
    mforms::GridView *_inserts_grid;
    RecordsetRef _inserts_model;
    RecordsetTableInsertsStorageRef _inserts_storage;

    void inserts_column_resized(int);
    void restore_inserts_columns();
    void catalogChanged(const std::string &member, const grt::ValueRef &value);

    void update_selection_for_menu_extra(mforms::ContextMenu *menu, const std::vector<int> &rows, int column);
    void open_field_editor(int row, int column);
  };
};
