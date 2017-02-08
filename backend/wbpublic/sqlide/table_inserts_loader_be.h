/*
 * Copyright (c) 2007, 2017, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _TABLE_INSERTS_LOADER_BE_H_
#define _TABLE_INSERTS_LOADER_BE_H_

#include "wbpublic_public_interface.h"
#include "grts/structs.db.h"

class SqlFacade;

class WBPUBLICBACKEND_PUBLIC_FUNC TableInsertsLoader {
public:
  TableInsertsLoader();
  virtual ~TableInsertsLoader() {
  }

  void process_table(db_TableRef table, const std::string &inserts_script);
};

#endif /* _TABLE_INSERTS_LOADER_BE_H_ */
