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

#include "wb_backend_public_interface.h"

#include <string>
#include "grt.h"
#include "base/file_utilities.h"
#include "grts/structs.workbench.h"
#include "base/trackable.h"

#ifndef _MSC_VER
#include <cairo/cairo.h>
#endif

#define MAIN_DOCUMENT_NAME "document.mwb.xml"
#define MAIN_DOCUMENT_AUTOSAVE_NAME "document-autosave.mwb.xml"

namespace bec {
  class GRTManager;
}

namespace wb {
  class MYSQLWBBACKEND_PUBLIC_FUNC ModelFile : public base::trackable {
  public:
    ModelFile(const std::string &tmpdir);
    ~ModelFile();

    void create();
    void open(const std::string &path);

    static std::string read_comment(const std::string &path);

    void cleanup();

    bool save_to(const std::string &path, const std::string &comment = "");

    bool has_unsaved_changes() {
      return _dirty;
    }

    workbench_DocumentRef retrieve_document();

    std::list<std::string> get_load_warnings() const {
      return _load_warnings;
    }

    void store_document(const workbench_DocumentRef &doc);
    void store_document_autosave(const workbench_DocumentRef &doc);

    std::list<std::string> get_file_list(const std::string &prefixdir = "");
    bool has_file(const std::string &name);

    std::string get_rel_db_file_path();
    std::string get_db_file_dir_path();
    std::string get_db_file_path();
    void add_db_file(const std::string &content_dir);

    std::string add_image_file(const std::string &path);
    std::string add_script_file(const std::string &path);
    std::string add_note_file(const std::string &path);
    void delete_file(const std::string &path);
    bool undelete_file(const std::string &path);

    void set_file_contents(const std::string &path, const std::string &data);
    void set_file_contents(const std::string &path, const char *data, size_t size);
    std::string get_file_contents(const std::string &path);

    std::string get_path_for(const std::string &file);
    std::string get_tempdir_path() {
      return _content_dir;
    }

    static void copy_file(const std::string &path, const std::string &dest);

    void copy_file_to(const std::string &file, const std::string &dest);

    std::string in_disk_document_version() const {
      return _loaded_version;
    }

    // image management
    cairo_surface_t *get_image(const std::string &path);

    boost::signals2::signal<void()> *signal_changed() {
      return &_changed_signal;
    }

    static const std::string lock_filename;

  private:
    base::LockFile *_temp_dir_lock;
    base::RecMutex _mutex;
    std::string _temp_dir;                //< temporary files directory
    std::string _content_dir;             //< path for directory where document contents are stored in disk
    std::list<std::string> _delete_queue; //< files marked for deletion
    std::string _loaded_version;          //< version of the model file as stored in disk

    std::list<std::string> _load_warnings; //< warnings from loaded model

    bool _dirty;

    typedef std::map<std::string, std::string> TableInsertsSqlScripts; // table guid -> sql script (inserts)
    TableInsertsSqlScripts
      table_inserts_sql_scripts; // for model upgrade only: move insert sql scripts from xml to sqlite db

    boost::signals2::signal<void()> _changed_signal;

    workbench_DocumentRef unserialize_document(xmlDocPtr xmldoc, const std::string &path);

  private:
    bool attempt_xml_document_upgrade(xmlDocPtr xmldoc, const std::string &version);
    workbench_DocumentRef attempt_document_upgrade(const workbench_DocumentRef &doc, xmlDocPtr xmldoc,
                                                   const std::string &version);
    void cleanup_upgrade_data();

    void check_and_fix_data_file_bug();
    bool check_and_fix_duplicate_uuid_bug(xmlDocPtr xmldoc);

    void check_and_fix_inconsistencies(xmlDocPtr xmldoc, const std::string &version);

    void check_and_fix_inconsistencies(const workbench_DocumentRef &doc, const std::string &version);

  public:
    static std::list<std::string> unpack_zip(const std::string &zipfile, const std::string &destdir);
    void pack_zip(const std::string &zipfile, const std::string &destdir, const std::string &comment = "");

  private:
    static std::string add_attachment_file(const std::string &destdir, const std::string &path);

  private:
    std::string create_document_dir(const std::string &dir, const std::string &prefix);
    bool semantic_check(workbench_DocumentRef doc);
  };
};
