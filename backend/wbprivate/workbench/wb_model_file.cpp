/*
 * Copyright (c) 2007, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include <algorithm>
#include <fcntl.h>

#include <zip.h>
#include "wb_model_file.h"

#include <algorithm>
#include <set>
#include <stdexcept>
#include <errno.h>

#include "grt.h"

#include "base/log.h"
#include "base/string_utilities.h"
#include "base/file_utilities.h"
#include "base/file_functions.h"
#include "base/util_functions.h"

#include "mforms/utilities.h"
#include "mdc_image.h"

#include "grt/grt_manager.h"

#include "grts/structs.workbench.h"
#include <glib/gstdio.h>

#define DOCUMENT_FORMAT "MySQL Workbench Model"
// version history:
// switched to 1.1.6 in 5.0.20
// switched to 1.2.0 in 5.1.0

// switched from 1.2.0 to 1.3.0 in 5.1.7
// updated to 1.4.0 in 5.2.0
// updated to 1.4.1 in 5.2.19
// updated to 1.4.2 in 5.2.33
// updated to 1.4.3 in 5.2.40
// updated to 1.4.4 in 5.2.42
#define DOCUMENT_VERSION "1.4.4"

#define ZIP_FILE_FORMAT "1.0"

#define IMAGES_DIR "@images"
#define NOTES_DIR "@notes"
#define SCRIPTS_DIR "@scripts"
#define DB_DIR "@db"
#define DB_FILE "data.db"

#define ZIP_FILE_COMMENT DOCUMENT_FORMAT " archive " ZIP_FILE_FORMAT

/* Auto-saving
 *
 * Auto-saving works by saving the model document file (the XML) to the expanded document folder
 * from time to time, named as document-autosave.mwb.xml. The expanded document folder is
 * automatically deleted when it is closed normally.
 * When a document is opened, it will check if there already is a document folder for that file
 * and if so, the recovery function will kick in, using the autosave XML file.
 */

DEFAULT_LOG_DOMAIN("model")

using namespace bec;
using namespace wb;
using namespace base;

const std::string ModelFile::lock_filename("lock");

void ModelFile::copy_file(const std::string &srcfile, const std::string &destfile) {
  char buffer[4098];
  FILE *sf = base_fopen(srcfile.c_str(), "rb");
  if (!sf)
    throw grt::os_error("Could not open file " + srcfile, errno);

  FILE *tf = base_fopen(destfile.c_str(), "w+");
  if (!tf) {
    fclose(sf);
    throw grt::os_error("Could not create file " + destfile, errno);
  }

  size_t c;
  while ((c = fread(buffer, 1, sizeof(buffer), sf)) > 0) {
    if (fwrite(buffer, 1, c, tf) < c) {
      int err = errno;
      fclose(sf);
      fclose(tf);
      throw grt::os_error("Error copying to file " + destfile, err);
    }
  }

  fclose(sf);
  fclose(tf);
}

static int rmdir_recursively(const char *path) {
  int res = 0;
  GError *error = NULL;
  GDir *dir;
  const char *dir_entry;
  gchar *entry_path;

  dir = g_dir_open(path, 0, &error);
  if (!dir && error) {
    res = error->code;
    g_error_free(error);
    return res;
  }

  while ((dir_entry = g_dir_read_name(dir))) {
    if (strcmp(dir_entry, ".") == 0 || strcmp(dir_entry, "..") == 0)
      continue;

    entry_path = g_build_filename(path, dir_entry, NULL);
    if (g_file_test(entry_path, G_FILE_TEST_IS_DIR))
      (void)rmdir_recursively(entry_path);
    else
      (void)g_remove(entry_path);
    g_free(entry_path);
  }

  (void)g_rmdir(path);

  g_dir_close(dir);

  return res;
}

std::string ModelFile::create_document_dir(const std::string &dir, const std::string &prefix) {
  std::string path;
  char s[12];
  int i = 0;

  strcpy(s, "d");
  for (;;) {
    path = dir + "/" + prefix + s;

    try {
      (void)base::create_directory(path, 0700); // return false means dir already exists
    } catch (base::file_error &exc) {
      throw grt::os_error(strfmt("Cannot create directory for document: %s", exc.what()));
    }

    try {
      _temp_dir_lock = new base::LockFile(base::makePath(path, lock_filename.c_str()));
      break;
    } catch (const base::file_locked_error &) {
      // continue
    }
    sprintf(s, "d%i", ++i);
  }

  return path;
}

ModelFile::ModelFile(const std::string &tmpdir) : _temp_dir_lock(0), _dirty(false) {
  _temp_dir = tmpdir;
}

ModelFile::~ModelFile() {
  cleanup();
}

void ModelFile::copy_file_to(const std::string &file, const std::string &dest) {
  copy_file(get_path_for(file), dest);
}

void ModelFile::open(const std::string &path) {
  bool file_is_zip;
  bool file_is_autosave = false;

  RecMutexLock lock(_mutex);

  if (base::is_directory(path) && path[path.size() - 1] == 'd') {
    file_is_zip = false;
    file_is_autosave = true;
  } else {
    FILE *f = base_fopen(path.c_str(), "rb");
    if (!f)
      throw grt::os_error("Could not open file " + path + ": " + strerror(errno));

    unsigned char buffer[10];
    size_t c;
    if ((c = fread(buffer, 1, 10, f)) < 10) {
      fclose(f);
      if (c == 0)
        throw std::runtime_error("File is empty.");
      else
        throw std::runtime_error("Invalid or corrupt file.");
    }
    fclose(f);

    if (buffer[0] == 0x50 && buffer[1] == 0x4b && buffer[2] == 0x03 && buffer[3] == 0x04 && buffer[4] == 0x14)
      file_is_zip = true;
    else {
      // file_is_zip= false;
      if (strncmp((char *)buffer, "<?xml", c < 5 ? c : 5) == 0) // check if this is a XML, otherwise assume zip
        file_is_zip = false;
      else
        file_is_zip = true;
    }
  }

  std::string basename = base::basename(path);

  // do some sanity checks
  if (basename.empty()) {
    throw std::runtime_error("Invalid path " + path);
  }

  std::string auto_save_dir = file_is_autosave ? path : base::makePath(_temp_dir, basename).append("d"); // default
  std::list<std::string> possible_autosaves = base::scan_for_files_matching(auto_save_dir + "*");
  for (std::list<std::string>::const_iterator d = possible_autosaves.begin(); d != possible_autosaves.end(); ++d) {
    gchar *path;
    gsize length;

    // check if this autosave is active/in use
    if (base::LockFile::check(base::makePath(*d, lock_filename.c_str())) != base::LockFile::NotLocked)
      continue;

    if (g_file_get_contents(base::makePath(*d, "real_path").c_str(), &path, &length, NULL)) {
      // ensure this autosave is for the model being opened
      if (std::string(path, length) == path) {
        auto_save_dir = *d;
        g_free(path);
        break;
      }
      g_free(path);
    } else // if the autosave dir has no file, then maybe it's from a version that still didn't save the path info
      break;
  }

  bool recover = false;
  if (!auto_save_dir.empty() && g_file_test(auto_save_dir.c_str(), G_FILE_TEST_EXISTS)) {
    // See if we can acquire the auto save lock. If that fails then another instance of WB has the lock already
    // so we don't do anything with this file.
    try {
      base::LockFile test_lock(base::makePath(auto_save_dir, lock_filename.c_str()));
    } catch (const base::file_locked_error &) {
      mforms::Utilities::show_warning(
        _("Opening Document"),
        base::strfmt(_("Could not lock the document %s.\n"
                       "The file is probably already opened in another instance of the application."),
                     path.c_str()),
        _("OK"));

      return;
    }

    time_t file_ts;
    base::file_mtime(path, file_ts);
    time_t autosave_ts;
    base::file_mtime(base::makePath(auto_save_dir, MAIN_DOCUMENT_NAME), autosave_ts);
    if (autosave_ts == 0)
      base::file_mtime(auto_save_dir, autosave_ts);

    // document dir already exists, ask if it should be recovered or deleted
    if (mforms::Utilities::show_warning(
          _("Document Recovery"),
          base::strfmt(_("The document %s was not properly closed in a previous session on %s.\n"
                         "The file you're about to open was last saved %s.\n"
                         "Would you like to use the recovered model? Continuing without recovering will remove the "
                         "auto-saved data."),
                       path.c_str(), base::fmttime(autosave_ts, DATETIME_FMT).c_str(),
                       base::fmttime(file_ts, DATETIME_FMT).c_str()),
          _("Recover"), _("Continue"), "") == mforms::ResultOk) {
      logInfo("Recovering %s...", path.c_str());
      recover = true;
      _content_dir = auto_save_dir;

      if (g_file_test((auto_save_dir + "/" + MAIN_DOCUMENT_AUTOSAVE_NAME).c_str(), G_FILE_TEST_EXISTS)) {
        g_remove((auto_save_dir + "/" + MAIN_DOCUMENT_NAME).c_str());
        int rc = g_rename((auto_save_dir + "/" + MAIN_DOCUMENT_AUTOSAVE_NAME).c_str(),
                          (auto_save_dir + "/" + MAIN_DOCUMENT_NAME).c_str());
        if (rc < 0) {
          // Rename failed, so try copying the file.
          try {
            copy_file((auto_save_dir + "/" + MAIN_DOCUMENT_AUTOSAVE_NAME).c_str(),
                      (auto_save_dir + "/" + MAIN_DOCUMENT_NAME).c_str());
          } catch (const std::exception &exc) {
            mforms::Utilities::show_error("Error recovering file",
                                          base::strfmt("There was an error recovering the document: %s\n", exc.what()),
                                          "OK", "", "");
            g_rename(auto_save_dir.c_str(), (auto_save_dir + ".cantrecover").c_str());
            recover = false;
          }
        }
      }
    } else // Cancel recovery
    {
      logInfo("Cleaning up leftover auto-save directory %s", auto_save_dir.c_str());
      rmdir_recursively(auto_save_dir.c_str());
    }
  }

  if (!recover) {
    _content_dir = create_document_dir(_temp_dir, basename);

    if (file_is_zip) {
      unpack_zip(path, _content_dir);

      check_and_fix_data_file_bug();
    } else {
      std::string destpath = _content_dir;
      destpath.append("/");
      destpath.append(MAIN_DOCUMENT_NAME);

      // assume old XML format and "convert" it
      copy_file(path, destpath);
    }

    _dirty = false;
  } else {
    _dirty = true;

    // re-lock it for ourselves
    _temp_dir_lock = new base::LockFile(base::makePath(_content_dir, lock_filename.c_str()));
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Returns a previously set zip file comment (if any) without unzipping the file.
 */
std::string ModelFile::read_comment(const std::string &path) {
  std::string schemas;
  int err;
  zip *z = zip_open(path.c_str(), 0, &err);
  if (z != NULL) {
    int length;
    const char *value = zip_get_archive_comment(z, &length, 0);
    if (value && length > 0) {
      std::string comment(value, length);
      if (length > -1) {
        std::size_t found = comment.find("model-schemas:");
        if (found != std::string::npos) {
          const char *ptr = comment.c_str() + found + 15;
          while (*ptr) {
            if (*ptr != '\n')
              schemas += *ptr;
            ptr++;
          }
        }
      }
    }
    zip_close(z);
  }
  return schemas;
}

//--------------------------------------------------------------------------------------------------

void ModelFile::create() {
  RecMutexLock lock(_mutex);

  _content_dir = create_document_dir(_temp_dir, "newmodel.mwb");
  add_db_file(_content_dir);

  _dirty = false;
}

std::string ModelFile::get_path_for(const std::string &file) {
  return _content_dir + "/" + file;
}

//--------------------------------------------------------------------------------------------------

// reading
workbench_DocumentRef ModelFile::retrieve_document() {
  RecMutexLock lock(_mutex);

  xmlDocPtr xmldoc = grt::GRT::get()->load_xml(get_path_for(MAIN_DOCUMENT_NAME));

retry:
  try {
    workbench_DocumentRef doc(unserialize_document(xmldoc, get_path_for(MAIN_DOCUMENT_NAME)));
    xmlFreeDoc(xmldoc);
    xmldoc = NULL;

    // Here the xml content is syntactically correct. Now do some semantic checks for sanity.
    if (!semantic_check(doc))
      throw std::logic_error(_("Invalid model file content."));

    return doc;
  } catch (grt::grt_runtime_error &exc) {
    if (strstr(exc.detail.c_str(), "Type mismatch: expected object of type"))
      if (check_and_fix_duplicate_uuid_bug(xmldoc))
        goto retry;
    throw;
  } catch (std::exception &) {
    if (xmldoc)
      xmlFreeDoc(xmldoc);
    throw;
  }
  return workbench_DocumentRef();
}

//--------------------------------------------------------------------------------------------------

bool ModelFile::semantic_check(workbench_DocumentRef doc) {
  // 1) Is there a valid physical model in the document?
  if (!doc->physicalModels().is_valid() || doc->physicalModels().count() == 0)
    return false;

  return true;
}

//--------------------------------------------------------------------------------------------------

std::list<std::string> ModelFile::unpack_zip(const std::string &zipfile, const std::string &destdir) {
  std::list<std::string> unpacked_files;

  if (g_mkdir_with_parents(destdir.c_str(), 0700) < 0)
    throw grt::os_error(strfmt(_("Cannot create temporary directory for open document: %s"), destdir.c_str()), errno);

  int err;
#ifdef ZIP_DISABLE_DEPRECATED
  // Would be good if we could test for zip_fdopen, but there's no way in the preprocessor.
  // And there's no version macro either for libzip.
  // Define ZIP_DISABLE_DEPRECATED to use the newer APIs.
  int fd = base_open(zipfile, O_RDONLY, S_IREAD); // Error checking is done already before.
  zip *z = zip_fdopen(fd, 0, &err);
#else
  zip *z = zip_open(zipfile.c_str(), 0, &err); // Older versions of libzip.
#endif
  if (z == NULL) {
    if (err == ZIP_ER_NOZIP)
      throw std::runtime_error("The file is not a Workbench document.");
    else if (err == ZIP_ER_MEMORY)
      throw grt::os_error("Cannot allocate enough memory to open document.");
    else if (err == ZIP_ER_NOENT)
      throw grt::os_error("File not found.");

#ifdef ZIP_DISABLE_DEPRECATED
    // the new API doesn't offer a way to extract an error without a pertaining zip*, which is NULL in this case
    std::string msg = "error opening zip archive";
#else
    int len = zip_error_to_str(NULL, 0, 0, err);
    std::string msg;
    if (len > 0) {
      char *buf = (char *)g_malloc(len + 1);
      zip_error_to_str(buf, len + 1, 0, err);
      msg = buf;
      g_free(buf);
    } else
      msg = "error opening zip archive";
#endif

    zip_close(z);
    throw std::runtime_error(strfmt(_("Cannot open document file: %s"), msg.c_str()));
  }

#ifdef ZIP_DISABLE_DEPRECATED
  zip_int64_t count = zip_get_num_entries(z, 0);
#else
  int count = zip_get_num_files(z);
#endif
  for (int i = 0; i < count; i++) {
    zip_file *file = zip_fopen_index(z, i, 0);
    if (!file) {
      const char *err = zip_strerror(z);
      zip_close(z);
      throw std::runtime_error(strfmt(_("Error opening document file: %s"), err));
    }

    const char *zname = zip_get_name(z, i, 0);
    if (strcmp(zname, "/") == 0 || strcmp(zname, "\\") == 0) {
      zip_fclose(file);
      continue;
    }
    std::string dirname = base::dirname(zname);
    std::string basename = base::basename(zname);

    // skip lock file as it is already locked and inaccessible
    if (basename == lock_filename) {
      zip_fclose(file);
      continue;
    }

    std::string outpath = destdir;

    if (!dirname.empty()) {
      outpath.append("/");
      outpath.append(dirname);
      if (g_mkdir_with_parents(outpath.c_str(), 0700) < 0) {
        zip_fclose(file);
        zip_close(z);
        throw grt::os_error(_("Error creating temporary directory while opending document."), errno);
      }
    }
    outpath.append("/");
    outpath.append(basename);

    FILE *outfile = base_fopen(outpath.c_str(), "w+");
    if (!outfile) {
      zip_fclose(file);
      zip_close(z);
      throw grt::os_error(_("Error creating temporary file while opending document."), errno);
    }

    unpacked_files.push_back(outpath);

    char buffer[4098];
    ssize_t c;
    while ((c = (size_t)zip_fread(file, buffer, sizeof(buffer))) > 0) {
      if ((ssize_t)fwrite(buffer, 1, c, outfile) < c) {
        int err = ferror(outfile);
        fclose(outfile);
        zip_fclose(file);
        zip_close(z);
        throw grt::os_error(_("Error writing temporary file while opending document."), err);
      }
    }

    if (c < 0) {
      std::string err = zip_file_strerror(file) ? zip_file_strerror(file) : "";
      zip_fclose(file);
      zip_close(z);
      throw std::runtime_error(strfmt(_("Error opening document file: %s"), err.c_str()));
    }

    zip_fclose(file);
    fclose(outfile);
  }

  zip_close(z);

  return unpacked_files;
}

static void zip_dir_contents(zip *z, const std::string &destdir, const std::string &partial) {
  GError *error = 0;
  GDir *dir = g_dir_open(destdir.empty() ? "." : destdir.c_str(), 0, &error);
  if (!dir) {
    zip_close(z);
    std::string err = error ? error->message : "Cannot open document directory.";
    g_error_free(error);
    throw grt::os_error(err);
  }

  // must add stuff in 2 steps, 1st files only and then dirs only
  for (int add_directories = 0; add_directories < 2; add_directories++) {
    const gchar *entry;
    while ((entry = g_dir_read_name(dir))) {
      std::string tmp = destdir;

      if (!tmp.empty())
        tmp.append("/").append(entry);
      else
        tmp.append(entry);

      if (g_file_test(tmp.c_str(), G_FILE_TEST_IS_DIR)) {
        if (add_directories) {
          try {
            zip_dir_contents(z, destdir.empty() ? entry : destdir + G_DIR_SEPARATOR + entry, tmp);
          } catch (...) {
            g_dir_close(dir);
            throw;
          }
        }
      } else {
        if (!add_directories) {
          zip_source *src = zip_source_file(z, tmp.c_str(), 0, 0);
#ifdef _MSC_VER
          if (!src || zip_file_add(z, tmp.c_str(), src, ZIP_FL_OVERWRITE | ZIP_FL_ENC_UTF_8) < 0) {
            zip_source_free(src);
            g_dir_close(dir);
            throw std::runtime_error(zip_strerror(z));
          }
#else
          if (!src || zip_add(z, tmp.c_str(), src) < 0) {
            zip_source_free(src);
            g_dir_close(dir);
            throw std::runtime_error(zip_strerror(z));
          }
#endif
        }
      }
    }
    g_dir_rewind(dir);
  }
  g_dir_close(dir);
}

void ModelFile::pack_zip(const std::string &zipfile, const std::string &destdir, const std::string &comment) {
  std::string curdir;

  {
    gchar *cwd = g_get_current_dir();
    curdir = cwd;
    g_free(cwd);
  }

  // change to the document data directory (after opening the zip file)
  // so that the zip won't have the full paths of the contents stored
  if (g_chdir(destdir.c_str()) < 0)
    throw grt::os_error("chdir failed.");

  // zip_open will open an existing file even if ZIP_CREATE is specified, so
  // we have to 1st delete the file...
  int err = 0;
  /* XXX: doesn't work yet as zip_close creates a temporary file without considering the name encoding
   *      and opening with zip_fdopen doesn't set the file name member of the zip struct
   *      which then crashes when an attempt is made to derived a temp name from that.
#ifdef ZIP_DISABLE_DEPRECATED
  int fd = base_open(zipfile, O_CREAT, S_IWRITE);
  zip *z = zip_fdopen(fd, 0, &err);
#else
  zip *z = zip_open(zipfile.c_str(), 0, &err); // Older versions of libzip.
#endif
#*/
  zip *z = zip_open(zipfile.c_str(), ZIP_CREATE, &err);
  if (!z) {
    if (err == ZIP_ER_MEMORY)
      throw grt::os_error("Cannot allocate enough temporary memory to save document.");
    else if (err == ZIP_ER_NOENT)
      throw grt::os_error("File or directory not found.");
    else
      throw grt::os_error("Cannot create file.");
  }

  std::string zip_comment = ZIP_FILE_COMMENT;
  if (!comment.empty()) {
    zip_comment += '\n';
    zip_comment += comment;
  }

#if defined(zip_uint16_t) || defined(_MSC_VER)
  zip_set_archive_comment(z, zip_comment.c_str(), (zip_uint16_t)zip_comment.size());
#else
  zip_set_archive_comment(z, zip_comment.c_str(), (int)zip_comment.size());
#endif

  try {
    zip_dir_contents(z, "", "");

    if (zip_close(z) < 0) {
      std::string err = zip_strerror(z) ? zip_strerror(z) : "";

      throw std::runtime_error(strfmt(_("Error writing zip file: %s"), err.c_str()));
    }

    g_chdir(curdir.c_str());
  } catch (...) {
    zip_close(z);
    g_chdir(curdir.c_str());
    throw;
  }
}

workbench_DocumentRef ModelFile::unserialize_document(xmlDocPtr xmldoc, const std::string &path) {
  std::string doctype, version;

  grt::GRT::get()->get_xml_metainfo(xmldoc, doctype, version);

  _loaded_version = version;

  // reset list of warnings found during load
  _load_warnings.clear();

  if (doctype != DOCUMENT_FORMAT)
    throw std::runtime_error("The file does not contain a Workbench document.");

  if (version != DOCUMENT_VERSION) {
    // first phase of document upgrade will upgrade it at XML level
    if (!attempt_xml_document_upgrade(xmldoc, version))
      throw std::runtime_error("The document was created in an incompatible version of the application.");
  }

  check_and_fix_inconsistencies(xmldoc, version);

  grt::ValueRef value(grt::GRT::get()->unserialize_xml(xmldoc, path));

  if (!value.is_valid())
    throw std::runtime_error("Error unserializing document data.");

  if (!workbench_DocumentRef::can_wrap(value))
    throw std::runtime_error("Loaded file does not contain a valid Workbench document.");

  workbench_DocumentRef doc(workbench_DocumentRef::cast_from(value));

  // send phase will upgrade at GRT level
  doc = attempt_document_upgrade(doc, xmldoc, version);

  cleanup_upgrade_data();

  check_and_fix_inconsistencies(doc, version);

  return doc;
}

//--------------------------------------------------------------------------------------------------

/**
 * Core save routine for model files. It does a backup of the existing model file of the given name
 * (if there is one). Checks are performed to ensure existing backup files can be removed and existing
 * model files can be renamed to .bak.
 */
bool ModelFile::save_to(const std::string &path, const std::string &comment) {
  RecMutexLock lock(_mutex);
#ifdef _MSC_VER
  const int read_write = _S_IWRITE | _S_IREAD;
#else
  const int read_write = S_IRUSR | S_IWUSR;
#endif

  if (g_file_test(path.c_str(), G_FILE_TEST_EXISTS)) {
    std::string tmp = path + ".bak";
    if (g_file_test(tmp.c_str(), G_FILE_TEST_EXISTS)) {
      if (g_access(tmp.c_str(), 2) != 0) // Windows has no constants defined like W_OK.
      {
        int result = mforms::Utilities::show_warning(
          _("Backup file is read-only"),
          _("A backup file for this model already exists and must be removed, but is read only."
            "\n\nDo you want to delete it anyway?"),
          _("Delete"), _("Cancel"));
        if (result != mforms::ResultOk)
          return false;

        if (g_chmod(tmp.c_str(), read_write) != 0) {
          mforms::Utilities::show_error(
            _("Cannot change permission"),
            strfmt(_("The read-only state of the file:\n\n%s\n\ncannot be changed. Giving up -"
                     " the model file will not be saved."),
                   tmp.c_str()),
            _("OK"));
          return false;
        }
      }
      g_remove(tmp.c_str());
    }

    // Check if the existing model file is read-only.
    if (g_access(path.c_str(), 2) != 0) {
      int result = mforms::Utilities::show_warning(
        _("Model file is read-only"), _("The model file is read-only.\n\nDo you want to overwrite it anyway?"),
        _("Overwrite File"), _("Cancel"));
      if (result != mforms::ResultOk)
        return false;
      if (g_chmod(path.c_str(), read_write) != 0) {
        mforms::Utilities::show_error(
          _("Cannot change permission"),
          strfmt(_("The read-only state of the file:\n\n%s\n\ncannot be changed. Giving up -"
                   " the model file will not be saved."),
                 path.c_str()),
          _("OK"));
        return false;
      }
    }
    if (g_rename(path.c_str(), tmp.c_str()) < 0)
      throw grt::os_error("Saving the document failed. The existing model file " + path +
                            " could not"
                            "be backed up. The system returned the error: \n\n",
                          errno);
  }

  for (std::list<std::string>::const_iterator iter = _delete_queue.begin(); iter != _delete_queue.end(); ++iter)
    g_remove(get_path_for(*iter).c_str());

  _delete_queue.clear();

  // saving the file for real can delete the autosave
  g_remove(get_path_for("document-autosave.mwb.xml").c_str());
  g_remove(get_path_for("real_path").c_str());

  if (g_path_is_absolute(path.c_str()))
    pack_zip(path, _content_dir, comment);
  else {
    char *prefix = g_get_current_dir();
    pack_zip(std::string(prefix).append("/").append(path), _content_dir, comment);
    g_free(prefix);
  }

  _dirty = false;
  return true;
}

//--------------------------------------------------------------------------------------------------

void ModelFile::cleanup() {
  RecMutexLock lock(_mutex);

  delete _temp_dir_lock;
  _temp_dir_lock = 0;

  if (!_content_dir.empty())
    rmdir_recursively(_content_dir.c_str());
}

void ModelFile::add_db_file(const std::string &content_dir) {
  std::string db_tpl_file_path = bec::GRTManager::get()->get_data_file_path("data/" DB_FILE);
  std::string db_file_dir_path = content_dir + "/" + DB_DIR;
  add_attachment_file(db_file_dir_path, db_tpl_file_path);
}

std::string ModelFile::get_rel_db_file_path() {
  return DB_DIR "/" DB_FILE;
}

std::string ModelFile::get_db_file_dir_path() {
  return _content_dir + "/" + DB_DIR;
}

std::string ModelFile::get_db_file_path() {
  return get_db_file_dir_path() + "/" + DB_FILE;
}

/** Adds an external file to the document.
*/
std::string ModelFile::add_attachment_file(const std::string &destdir, const std::string &path) {
  std::string prefix = destdir + "/";
  if (!path.empty()) {
    prefix += base::basename(path);
  }

  int i = 1;
  std::string destfile = prefix;

  if (!g_file_test(destdir.c_str(), G_FILE_TEST_IS_DIR)) {
    if (g_mkdir_with_parents(destdir.c_str(), 0700) < 0)
      throw grt::os_error("Could not create directory for attached file");
  }

  // if path is not supplied, default value of destfile would be filled with the dirname only
  if (path.empty())
    destfile = strfmt("%s%i", prefix.c_str(), i++);

  while (g_file_test(destfile.c_str(), G_FILE_TEST_EXISTS))
    destfile = strfmt("%s%i", prefix.c_str(), i++);

  if (path.empty()) {
    FILE *f = base_fopen(destfile.c_str(), "w+");
    if (f)
      fclose(f);
    else
      throw grt::os_error("Error creating attached file");
  } else {
    try {
      ModelFile::copy_file(path, destfile);
    } catch (std::exception &exc) {
      throw std::runtime_error(std::string("Error adding file to document: ").append(exc.what()));
    }
  }

  destfile = base::basename(destdir).append("/").append(base::basename(destfile));

  return destfile;
}

std::string ModelFile::add_image_file(const std::string &path) {
  _dirty = true;

  return add_attachment_file(_content_dir + "/" + IMAGES_DIR, path);
}

std::string ModelFile::add_script_file(const std::string &path) {
  _dirty = true;

  return add_attachment_file(_content_dir + "/" + SCRIPTS_DIR, path);
}

std::string ModelFile::add_note_file(const std::string &path) {
  _dirty = true;

  return add_attachment_file(_content_dir + "/" + NOTES_DIR, path);
}

bool ModelFile::has_file(const std::string &name) {
  RecMutexLock lock(_mutex);

  return g_file_test(get_path_for(name).c_str(), G_FILE_TEST_EXISTS) != 0;
}

void ModelFile::set_file_contents(const std::string &path, const std::string &data) {
  set_file_contents(path, data.c_str(), data.size());
}

void ModelFile::set_file_contents(const std::string &path, const char *data, size_t size) {
  std::string fpath = get_path_for(path);

  GError *error = NULL;
  g_file_set_contents(fpath.c_str(), data, (gssize)size, &error);
  if (error != NULL)
    throw std::runtime_error(std::string("Error while setting file contents: ") + error->message);
}

std::string ModelFile::get_file_contents(const std::string &path) {
  gchar *contents = 0;
  gsize length;
  std::string tmp;

  if (g_file_get_contents(get_path_for(path).c_str(), &contents, &length, NULL)) {
    tmp = std::string(contents, length);
    g_free(contents);
    return tmp;
  }

  throw std::runtime_error("Error reading attached file contents.");
}

// writing
void ModelFile::store_document(const workbench_DocumentRef &doc) {
  grt::GRT::get()->serialize(doc, get_path_for(MAIN_DOCUMENT_NAME), DOCUMENT_FORMAT, DOCUMENT_VERSION);

  _dirty = true;
}

void ModelFile::store_document_autosave(const workbench_DocumentRef &doc) {
  grt::GRT::get()->serialize(doc, get_path_for("document-autosave.mwb.xml"), DOCUMENT_FORMAT, DOCUMENT_VERSION);
}

void ModelFile::delete_file(const std::string &path) {
  if (std::find(_delete_queue.begin(), _delete_queue.end(), path) == _delete_queue.end()) {
    _dirty = true;
    _delete_queue.push_back(path);
  }
}

bool ModelFile::undelete_file(const std::string &path) {
  std::list<std::string>::iterator iter;

  if ((iter = std::find(_delete_queue.begin(), _delete_queue.end(), path)) == _delete_queue.end())
    return false;

  _dirty = true;
  _delete_queue.erase(iter);

  return true;
}

//--------------------------------------------------------------------------------------------------

cairo_surface_t *ModelFile::get_image(const std::string &path) {
  return mdc::surface_from_png_image(get_path_for(path));
}

//--------------------------------------------------------------------------------------------------

void ModelFile::check_and_fix_data_file_bug() {
// WB up to 5.2.21 used G_DIR_SEPARATOR for data file paths. This was incorrect
// as the @db\data.db was being treated as a filename outside Windows, instead
// of a file in a subdirectory called @db. The issue was corrected, but the problem
// that files created until 5.2.21 had the data.db file in the wrong place.
// This is not a problem if the files are opened in the same platform, but it is
// when a file written in Windows is opened elsewhere. To solve that, we
// will rename @db\data.db to data.db in the @db subdirectory. One extra complication
// is that some model files will have both data files, because they were opened
// in a different platform and extra data was added since.

#ifndef _MSC_VER
  // check if @db\data.db exists and is a file
  std::string data_filename_in_windows = _content_dir + "/" + DB_DIR + "\\" + DB_FILE;
  if (g_file_test(data_filename_in_windows.c_str(), (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))) {
    // if so, we rename it to @db/data.db, but first check if @db/data.db itself exists
    if (g_file_test(get_db_file_path().c_str(), G_FILE_TEST_EXISTS)) {
      // file already exists, rename it and leave it there
      g_rename(get_db_file_path().c_str(), get_db_file_path().append(".old").c_str());
    }
    g_rename(data_filename_in_windows.c_str(), get_db_file_path().c_str());
  }
#endif
}
