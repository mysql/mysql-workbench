/*
 * Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include <glib.h>
#include "file_charset_dialog.h"
#include "grt/common.h"
#include "grtpp_util.h"

#include "base/string_utilities.h"

#include <mforms/button.h>
#include <mforms/selector.h>
#include <mforms/box.h>
#include <mforms/label.h>
#include <mforms/utilities.h>

#include "grts/structs.db.h"

using namespace mforms;
using namespace base;

FileCharsetDialog::FileCharsetDialog(const std::string &title, const std::string &message)
: Form(nullptr, mforms::FormNone) {
  set_name("File Charset");
  setInternalName("file_charset_dialog");
  set_title("Open SQL File");

  _charset = manage(new Selector(SelectorCombobox));
  _ok = manage(new Button());
  _cancel = manage(new Button());
  _run = manage(new Button());

  Box *vbox = manage(new Box(false));
  vbox->set_padding(12);
  vbox->set_spacing(12);

  Label *l = manage(new Label(title));
  l->set_style(BoldStyle);
  vbox->add(l, false, true);

  l = manage(new Label(message));
  vbox->add(l, false, true);

  Box *hbox = manage(new Box(true));
  vbox->add(hbox, false, true);

  hbox->add(manage(new Label(_("Character Set Encoding Name:"))), false, true);
  hbox->add(_charset, true, true);

  Box *bbox = manage(new Box(true));
  vbox->add(bbox, false, true);
  bbox->set_spacing(12);

  _run->signal_clicked()->connect(std::bind(&FileCharsetDialog::run_clicked, this));

  _ok->set_text(_("OK"));
  _cancel->set_text(_("Cancel"));
  _run->set_text(_("Run SQL Script..."));
  Utilities::add_end_ok_cancel_buttons(bbox, _ok, _cancel);
  bbox->add(_run, false, true);

  set_content(vbox);
  center();
}

std::string FileCharsetDialog::run(const std::string &default_encoding) {
  grt::ListRef<db_CharacterSet> charsets(
    grt::ListRef<db_CharacterSet>::cast_from(grt::GRT::get()->get("/wb/rdbmsMgmt/rdbms/0/characterSets")));
  std::list<std::string> chlist;
  GRTLIST_FOREACH(db_CharacterSet, charsets, ch) {
    // insert sorted
    chlist.insert(std::lower_bound(chlist.begin(), chlist.end(), *(*ch)->name()), *(*ch)->name());
  }
  _charset->add_items(chlist);

  _run_clicked = false;
  _charset->set_value(default_encoding);
  if (run_modal(_ok, _cancel))
    return _charset->get_string_value();
  return "";
}

void FileCharsetDialog::run_clicked() {
  _run_clicked = true;
  end_modal(false);
}

FileCharsetDialog::Result FileCharsetDialog::ensure_filedata_utf8(const char *data, size_t length,
                                                                  const std::string &encoding,
                                                                  const std::string &filename, char *&utf8_data,
                                                                  std::string *original_encoding) {
  // Byte order marks.
  const char *utf16le_bom = "\xff\xfe";
  const char *utf16be_bom = "\xfe\xff";
  const char *utf32le_bom = "\xff\xfe\0\0";
  const char *utf32be_bom = "\0\0\xfe\xff";
  const char *utf8_bom = "\xef\xbb\xbf";

  size_t utf8_data_length = 0;
  const gchar *end = NULL;
  bool retrying = false;
retry:
  if (!g_utf8_validate(data, (gssize)length, &end)) {
    std::string default_encoding = "latin1";

    // Check if there is a byte-order-mark to provide a better suggestion for the source encoding.
    if (length >= 2) {
      if (strncmp(data, utf16le_bom, 2) == 0)
        default_encoding = "UTF-16LE";
      else if (strncmp(data, utf16be_bom, 2) == 0)
        default_encoding = "UTF-16BE";

      if (length >= 4) {
        if (strncmp(data, utf32le_bom, 4) == 0)
          default_encoding = "UTF-32LE";
        else if (strncmp(data, utf32be_bom, 4) == 0)
          default_encoding = "UTF-32BE";
      }
    }

    std::string charset;
    char *converted;
    gsize bytes_read, bytes_written;
    GError *error = NULL;

    if (encoding.empty() || retrying) {
      FileCharsetDialog dlg(
        _("Unknown File Encoding"),
        strfmt("The file '%s' is not UTF-8 encoded.\n\n"
               "Please select the encoding of the file and press OK for Workbench to convert and open it.\n"
               "Note that as Workbench works with UTF-8 text, if you save back to the original file,\n"
               "its contents will be replaced with the converted data.\n\n"
               "WARNING: If your file contains binary data, it may become corrupted.\n\n"
               "Click \"Run SQL Script...\" to execute the file without opening for editing.",
               filename.c_str()));
      charset = dlg.run(default_encoding);
      if (charset.empty()) {
        if (dlg._run_clicked)
          return RunInstead;
        return Cancelled;
      }
    } else {
      charset = encoding;
      retrying = true; // in case we fail..
    }

    converted = g_convert(data, (gssize)length, "UTF-8", charset.c_str(), &bytes_read, &bytes_written, &error);
    if (!converted) {
      int res;

      res = Utilities::show_error(_("Could not Convert Text Data"),
                                  strfmt(_("The file contents could not be converted from '%s' to UTF-8:\n%s\n"),
                                         charset.c_str(), error ? error->message : "Unknown error"),
                                  _("Choose Encoding"), _("Cancel"));
      if (error)
        g_error_free(error);

      if (res == ResultOk)
        goto retry;

      return Cancelled;
    } else if (bytes_read < length) {
      // Conversion was not complete. We can retry or return at least the partial result which
      // could be converted.
      int res;

      res = Utilities::show_error(_("Could not Convert Text Data"),
                                  strfmt(_("Some of the file contents could not be converted from '%s' to UTF-8:\n%s\n"
                                           "Click Ignore to open the partial file anyway, or choose another encoding."),
                                         charset.c_str(), error ? error->message : "Unknown error"),
                                  _("Ignore"), _("Cancel"), _("Choose Encoding"));
      if (error)
        g_error_free(error);
      if (res == ResultOk) {
        utf8_data = converted;
        utf8_data_length = bytes_written;
        // Proceed to an eventual BOM removal.
      } else {
        g_free(converted);
        if (res == ResultCancel)
          return Cancelled;
        else
          goto retry;
      }
    } else {
      utf8_data = converted; // We still need to go through the following BOM check.
      utf8_data_length = bytes_written;
    }
    if (original_encoding)
      *original_encoding = charset;

    // Check (again) for a byte-order-mark and skip it if there is one.
    // We only have UTF-8 now, so only the BOM for this encoding is checked.
    if ((utf8_data_length >= 3) && (strncmp(utf8_data, utf8_bom, 3) == 0)) {
      memmove(utf8_data, utf8_data + 3, utf8_data_length - 3);
      utf8_data[utf8_data_length - 3] = '\0';
    }
  } else {
    // data is already utf8
    utf8_data = NULL;
    utf8_data_length = 0;
  }

  return Accepted;
}
