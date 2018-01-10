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

#ifndef _RECORDSET_TEXT_STORAGE_BE_H_
#define _RECORDSET_TEXT_STORAGE_BE_H_

#include "wbpublic_public_interface.h"
#include "recordset_data_storage.h"
#include <map>

class WBPUBLICBACKEND_PUBLIC_FUNC Recordset_text_storage : public Recordset_data_storage {
public:
  class TemplateInfo : public Recordset_storage_info {
  public:
    std::string path;
    std::string include_column_types; // syntax type
    std::string null_syntax;
    std::string row_separator;
    bool pre_quote_strings;
    std::string quote;
  };
  static std::vector<Recordset_storage_info> storage_types();

public:
  typedef std::shared_ptr<Recordset_text_storage> Ref;
  static Ref create() {
    return Ref(new Recordset_text_storage());
  }
  virtual ~Recordset_text_storage();

protected:
  Recordset_text_storage();

protected:
  virtual void do_apply_changes(const Recordset *recordset, sqlite::connection *data_swap_db, bool skip_commit);
  virtual void do_serialize(const Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_unserialize(Recordset *recordset, sqlite::connection *data_swap_db);
  virtual void do_fetch_blob_value(Recordset *recordset, sqlite::connection *data_swap_db, RowId rowid, ColumnId column,
                                   sqlite::variant_t &blob_value);

public:
  virtual ColumnId aux_column_count();

public:
  typedef std::map<std::string, std::string> Parameters;

  void parameters(const Parameters &val) {
    _parameters = val;
  }
  const Parameters &parameters() const {
    return _parameters;
  }

  std::string parameter_value(const std::string &name) const;
  void parameter_value(const std::string &name, const std::string &value) {
    _parameters[name] = value;
  }

protected:
  Parameters _parameters;

public:
  void data_format(const std::string &val) {
    _data_format = val;
  }
  void file_path(const std::string &val) {
    _file_path = val;
  }
  const std::string &file_path() const {
    return _file_path;
  }

protected:
  std::string _data_format;
  std::string _file_path;
};

#endif /* _RECORDSET_TEXT_STORAGE_BE_H_ */
