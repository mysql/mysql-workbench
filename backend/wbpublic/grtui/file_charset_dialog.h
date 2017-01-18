/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#pragma once

#include "mforms/form.h"
#include "grt.h"
#include "wbpublic_public_interface.h"

namespace mforms {
  class Selector;
  class Button;
}

class WBPUBLICBACKEND_PUBLIC_FUNC FileCharsetDialog : public mforms::Form {
  mforms::Selector *_charset;
  mforms::Button *_ok;
  mforms::Button *_cancel;
  mforms::Button *_run;

  bool _run_clicked;

  FileCharsetDialog(const std::string &title, const std::string &message);

  void run_clicked();

public:
  enum Result { Cancelled, Accepted, RunInstead };

  std::string run(const std::string &default_encoding);

  static Result ensure_filedata_utf8(const char *data, size_t length, const std::string &encoding,
                                     const std::string &filename, char *&utf8_data,
                                     std::string *original_encoding = nullptr);
};
