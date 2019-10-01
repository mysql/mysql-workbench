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

#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/util_functions.h"

#include "inserts_export_form.h"

#include "sqlide/recordset_text_storage.h"
#include "sqlide/recordset_sql_storage.h"

#include "mforms/simpleform.h"
#include "mforms/utilities.h"

using namespace mforms;
using namespace base;
using namespace base;

InsertsExportForm::InsertsExportForm(mforms::Form *owner, Recordset::Ref rset, const std::string &default_ext)
  : FileChooser(owner, SaveFile), _record_set(rset) {
  std::string extlist;
  _storage_types = _record_set->data_storages_for_export();
  for (size_t i = 0; i < _storage_types.size(); i++) {
    extlist.append("|").append(_storage_types[i].description);
    extlist.append("|").append(_storage_types[i].extension);
    _storage_type_index[_storage_types[i].description] = (int)i;
  }

  if (extlist.empty())
    throw std::runtime_error("No export formats found");

  // set_extensions(extlist.substr(1), default_ext);
  add_selector_option("format", _("Format:"), extlist.substr(1));

  set_title(_("Export Inserts Data to File"));
}

std::string InsertsExportForm::run() {
  if (run_modal()) {
    std::string path = get_path();
    std::string ext = base::extension(path);
    if (!ext.empty() && ext[0] == '.')
      ext = ext.substr(1);

    std::string format = get_selector_option_value("format");
    int i = _storage_type_index[format];
    const Recordset_storage_info &info(_storage_types[i]);

    Recordset_data_storage::Ref dataStorage = _record_set->data_storage_for_export(info.name);

    if (dynamic_cast<Recordset_text_storage *>(dataStorage.get())) {
      Recordset_text_storage *textStorage = dynamic_cast<Recordset_text_storage *>(dataStorage.get());
      textStorage->data_format(info.name);
      textStorage->file_path(path);

      Recordset_sql_storage *storage = dynamic_cast<Recordset_sql_storage *>(_record_set->data_storage().get());
      textStorage->parameter_value("GENERATOR_QUERY", _record_set->generator_query());
      textStorage->parameter_value("GENERATE_DATE", base::fmttime(time(NULL), DATETIME_FMT));
      textStorage->parameter_value("TABLE_NAME", storage->table_name().empty() ? "TABLE" : storage->table_name());

      if (!info.arguments.empty()) {
        mforms::SimpleForm form(_("Export Recordset"), _("Export"));

        form.add_label(strfmt(_("Export options for %s"), info.description.c_str()), false);

        // handle arguments specific to the data storage
        for (std::list<std::pair<std::string, std::string> >::const_iterator arg = info.arguments.begin();
             arg != info.arguments.end(); ++arg)
        form.add_text_entry(arg->second, arg->first + ":", textStorage->parameter_value(arg->second));
        form.set_size(400, -1);
        if (!form.show())
          return "";
        for (std::list<std::pair<std::string, std::string> >::const_iterator arg = info.arguments.begin();
             arg != info.arguments.end(); ++arg)
          textStorage->parameter_value(arg->second, form.get_string_view_value(arg->second));
      }
    }
    /*else
    {
      if (!info.arguments.empty())
      {
        mforms::SimpleForm form(strfmt(_("Export as %s"), info.description.c_str()), _("Export"));
        for (std::list<std::pair<std::string,std::string> >::const_iterator arg= info.arguments.begin();
             arg != info.arguments.end(); ++arg)
          form.add_text_entry(arg->first, arg->second, "");
        if (!form.show())
          return "";
      }
    }*/

    try {
      dataStorage->serialize(_record_set);
      return path;
    } catch (std::exception &exc) {
      mforms::Utilities::show_error(_("Export Inserts Data"), exc.what(), _("OK"));
      return "";
    }
  }
  return "";
}
