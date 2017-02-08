/*
 * Copyright (c) 2011, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _INSERTS_EXPORT_FORM_H_
#define _INSERTS_EXPORT_FORM_H_

#include "wbpublic_public_interface.h"
#include "sqlide/recordset_be.h"

#include "mforms/filechooser.h"

class WBPUBLICBACKEND_PUBLIC_FUNC InsertsExportForm : public mforms::FileChooser {
public:
  InsertsExportForm(mforms::Form *owner, Recordset::Ref rset = Recordset::Ref(),
                    const std::string &default_extension = "");

  std::string run();

private:
  Recordset::Ref _record_set;
  std::vector<Recordset_storage_info> _storage_types;
  std::map<std::string, int> _storage_type_index;
};

#endif // _INSERTS_EXPORT_FORM_H_
