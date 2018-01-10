/*
 * Copyright (c) 2010, 2018, Oracle and/or its affiliates. All rights reserved.
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

#ifndef _DB_MYSQL_VALIDATION_PAGE_H_
#define _DB_MYSQL_VALIDATION_PAGE_H_

#include "db_mysql_public_interface.h"
#include "grt/grt_manager.h"
#include "grts/structs.db.mysql.h"
#include "grt/grt_string_list_model.h"
#include "base/trackable.h"

/*
This is a shared header for validation page on SQL export pluigns
- script export
- script sync
- db fwd-eng
- db sync
*/

class WBPLUGINDBMYSQLBE_PUBLIC_FUNC DbMySQLValidationPage : public base::trackable {
public:
  typedef std::function<int()> Validation_finished_cb;
  typedef std::function<int(int)> Validation_step_finished_cb;

private:
  Validation_finished_cb _validation_finished_cb;
  Validation_step_finished_cb _validation_step_finished_cb;

protected:
  bec::MessageListBE *messages_list;

public:
  DbMySQLValidationPage();
  ~DbMySQLValidationPage();

  void run_validation();

  void validation_finished(grt::ValueRef res);
  grt::ValueRef validation_task(grt::StringRef);

  void validation_message(const grt::Message &);
  void validation_finished_cb(Validation_finished_cb cb) {
    _validation_finished_cb = cb;
  }

  void validation_step_finished_cb(Validation_step_finished_cb cb) {
    _validation_step_finished_cb = cb;
  }

  bec::MessageListBE *get_messages_list() {
    return messages_list;
  }
};

#endif // _DB_MYSQL_VALIDATION_PAGE_H_
