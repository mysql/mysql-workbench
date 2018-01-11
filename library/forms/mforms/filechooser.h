/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "base/file_utilities.h"
#include <mforms/base.h>
#include <mforms/view.h>
#include <vector>

namespace mforms {
  enum FileChooserType { OpenFile = 1, SaveFile = 2, OpenDirectory = 3 };

  class Form;
  class FileChooser;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef SWIG
  struct MFORMS_EXPORT FileChooserImplPtrs {
    bool (*create)(FileChooser *self, mforms::Form *owner, FileChooserType type, bool show_hidden);
    void (*set_title)(FileChooser *self, const std::string &title);
    bool (*run_modal)(FileChooser *self);
    void (*set_directory)(FileChooser *self, const std::string &path);
    void (*set_path)(FileChooser *self, const std::string &path);
    std::string (*get_directory)(FileChooser *self);
    std::string (*get_path)(FileChooser *self);
    void (*set_extensions)(FileChooser *self, const std::string &extensions, const std::string &default_extension,
                           bool allow_all_file_types);

    void (*add_selector_option)(FileChooser *self, const std::string &name, const std::string &label,
                                const std::vector<std::pair<std::string, std::string> > &options);
    std::string (*get_selector_option_value)(FileChooser *self, const std::string &name);
  };
#endif
#endif

  /** A File Picker dialog.
   */
  class MFORMS_EXPORT FileChooser : public View {
    FileChooserImplPtrs *_filechooser_impl;

  public:
    typedef std::vector<std::pair<std::string, std::string> > StringPairVector;

    // keeps a mapping of option name -> list of value identifiers
    std::map<std::string, std::vector<std::string> > _selector_options;
    StringPairVector split_extensions(const std::string &extensions, bool file_extensions = true);

  public:
    /** Constructor.

     Type of file chooser may be one of OpenFile, SaveFile or OpenDirectory.
     Set show_hidden to true if you wanna show hidden files and folders like "~/.ssh".
     */
    FileChooser(FileChooserType, bool show_hidden = false);
    FileChooser(mforms::Form *owner, FileChooserType, bool show_hidden = false);

    /** Sets the text to be shown in the title of the dialog window. */
    virtual void set_title(const std::string &title);

    /** Shows the dialog and wait for user input.

     Returns true if the user clicks OK, false otherwise. */
    virtual bool run_modal();

    /** Set initial directory for the chooser. */
    void set_directory(const std::string &path);

    /** Set initial directory and filename for the chooser. */
    void set_path(const std::string &path);

    /** Gets the selected path. */
    std::string get_path();

    /** Gets the currently selected directory. */
    std::string get_directory();

    /** Set allowed file extensions.

     The format is "Foo files (*.foo)|*.foo|SQL Scripts (*.sql)|*.sql"
     default_extension selects the default (ie "foo")
     */
    void set_extensions(const std::string &extensions, const std::string &default_extension,
                        bool allow_all_file_types = true);

    /** Adds a selector type option to the file dialog, with the givan label and list of value caption/identifiers.
     If name is 'format', the option is treated as a file extension, where the identifier of the option is the file
     extension */
    void add_selector_option(const std::string &name, const std::string &label, const StringPairVector &options);
    /** Adds a selector type option to the file dialog, with the givan label and list of value caption/identifiers,
     separated by |
     The format of the string is Caption1|identifier1|Caption2|identifier2...*/
    void add_selector_option(const std::string &name, const std::string &label, const std::string &options);
    /** Gets the value id of the selector option of the given name */
    std::string get_selector_option_value(const std::string &name);

    static std::string last_directory;
  };
};
