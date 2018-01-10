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

/**
 * Implementation of a composite used to select a file system object like a file, folder, device,
 * drive etc.
 */

#pragma once

#include "mforms/label.h"
#include "mforms/box.h"
#include "mforms/button.h"
#include "mforms/textentry.h"
#include "mforms/filechooser.h"
#include "base/trackable.h"

namespace mforms {

  class MFORMS_EXPORT FsObjectSelector : public Box {
  private:
    Button* _browse_button;
    TextEntry* _edit;
    FileChooserType _type;
    std::string _extensions;
    std::string _default_extension;
    std::function<void()> _on_validate;
    boost::signals2::scoped_connection
      _browse_connection; // The connection created when connecting the browse callback.
    bool _show_hidden;

  protected:
    void enable_file_browsing();
    void filename_changed();
    void browse_file_callback();

  public:
    FsObjectSelector(bool horizontal = true);
    FsObjectSelector(Button* button, TextEntry* edit);
    ~FsObjectSelector();

    void initialize(const std::string& initial_path, FileChooserType type, const std::string& extensions,
                    bool show_hidden = false, std::function<void()> on_validate = std::function<void()>());
    void set_filename(const std::string& path);
    std::string get_filename();
    void set_enabled(bool value);
    void set_browse_callback(std::function<void()> browse_callback);

    TextEntry* get_entry() const {
      return _edit;
    }

    virtual std::string get_string_value();
    virtual int get_int_value();
    virtual bool get_bool_value();

#ifndef SWIG
    boost::signals2::signal<void()>* signal_changed() {
      return _edit->signal_changed();
    }
#endif

    static void clear_stored_filenames();
    static bool check_and_confirm_file_overwrite(TextEntry* entry, const std::string& default_extension = "");
    bool check_and_confirm_file_overwrite();
  };
}
