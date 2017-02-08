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

#include "wf_base.h"
#include "wf_view.h"
#include "wf_filechooser.h"

#include "base/file_utilities.h"

using namespace System::Windows::Forms;

using namespace MySQL;
using namespace MySQL::Forms;

//----------------- FileChooserWrapper -------------------------------------------------------------

FileChooserWrapper::FileChooserWrapper(mforms::FileChooser *backend, mforms::Form *owner) : ViewWrapper(backend) {
  // The owner is currently not used here.
}

//-------------------------------------------------------------------------------------------------

bool FileChooserWrapper::create(mforms::FileChooser *backend, mforms::Form *owner, mforms::FileChooserType type,
                                bool show_hidden) {
  // On Windows hiding/showing hidden files is a global switch in Explorer so we can just
  // ignore the show_hidden flag here. It is necessary for Linux/Mac.
  FileChooserWrapper *wrapper = new FileChooserWrapper(backend, owner);
  wrapper->type = type;

  switch (type) {
    case mforms::OpenFile: {
      FileDialog ^ dialog = FileChooserWrapper::Create<OpenFileDialog>(backend, wrapper);
      dialog->RestoreDirectory = true;
      break;
    }

    case mforms::SaveFile: {
      FileDialog ^ dialog = FileChooserWrapper::Create<SaveFileDialog>(backend, wrapper);
      dialog->RestoreDirectory = true;
      break;
    }

    case mforms::OpenDirectory:
      FileChooserWrapper::Create<FolderBrowserDialog>(backend, wrapper);
      break;

    default:
      return false;
  }
  return true;
}

//-------------------------------------------------------------------------------------------------

void FileChooserWrapper::set_title(mforms::FileChooser *backend, const std::string &title) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      dialog->Title = CppStringToNativeRaw(title);
      break;
    }

    case mforms::OpenDirectory: {
      FolderBrowserDialog ^ dialog = wrapper->GetManagedObject<FolderBrowserDialog>();
      dialog->Description = CppStringToNativeRaw(title);
      break;
    }
  }
}

//-------------------------------------------------------------------------------------------------

bool FileChooserWrapper::run_modal(mforms::FileChooser *backend) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      return dialog->ShowDialog() == DialogResult::OK;
    }

    case mforms::OpenDirectory: {
      FolderBrowserDialog ^ dialog = wrapper->GetManagedObject<FolderBrowserDialog>();
      return dialog->ShowDialog() == DialogResult::OK;
    }
  }

  return false;
}

//-------------------------------------------------------------------------------------------------

void FileChooserWrapper::set_directory(mforms::FileChooser *backend, const std::string &path) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      dialog->InitialDirectory = CppStringToNativeRaw(path);
      break;
    }

    case mforms::OpenDirectory: {
      FolderBrowserDialog ^ dialog = wrapper->GetManagedObject<FolderBrowserDialog>();
      dialog->SelectedPath = CppStringToNativeRaw(path);
      break;
    }
  }
}

//-------------------------------------------------------------------------------------------------

std::string FileChooserWrapper::get_directory(mforms::FileChooser *backend) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      return NativeToCppStringRaw(System::IO::Path::GetDirectoryName(dialog->FileName));
    }

    case mforms::OpenDirectory: {
      FolderBrowserDialog ^ dialog = wrapper->GetManagedObject<FolderBrowserDialog>();
      return NativeToCppStringRaw(dialog->SelectedPath);
    }
  }

  return "";
}

//-------------------------------------------------------------------------------------------------

std::string FileChooserWrapper::get_path(mforms::FileChooser *backend) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      return NativeToCppStringRaw(dialog->FileName);
    }

    case mforms::OpenDirectory: {
      FolderBrowserDialog ^ dialog = wrapper->GetManagedObject<FolderBrowserDialog>();
      return NativeToCppStringRaw(dialog->SelectedPath);
    }
  }

  return "";
}

//-------------------------------------------------------------------------------------------------

void FileChooserWrapper::set_path(mforms::FileChooser *backend, const std::string &path) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      dialog->InitialDirectory = CppStringToNativeRaw(base::dirname(path));
      dialog->FileName = CppStringToNativeRaw(base::basename(path));
      break;
    }

    case mforms::OpenDirectory: {
      FolderBrowserDialog ^ dialog = wrapper->GetManagedObject<FolderBrowserDialog>();
      dialog->SelectedPath = CppStringToNative(path);
      break;
    }
  }
}

//-------------------------------------------------------------------------------------------------

void FileChooserWrapper::set_extensions(mforms::FileChooser *backend, const std::string &extensions,
                                        const std::string &default_extension, bool allow_all_file_types) {
  FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

  switch (wrapper->type) {
    case mforms::OpenFile:
    case mforms::SaveFile: {
      FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
      std::string all_extensions = extensions;
      if (allow_all_file_types)
        all_extensions += "|All Files (*.*)|*.*";

      dialog->AddExtension = true;
      dialog->DefaultExt = CppStringToNativeRaw(default_extension);
      dialog->Filter = CppStringToNative(all_extensions);
      break;
    }

    case mforms::OpenDirectory:
      break;
  }
}

//-------------------------------------------------------------------------------------------------

void FileChooserWrapper::add_selector_option(mforms::FileChooser *backend, const std::string &name,
                                             const std::string &label,
                                             const mforms::FileChooser::StringPairVector &options) {
  if (name == "format") {
    // The backend split the properly formatted string, we now have to concatenate the parts again
    // (and we lost the default extension).
    std::string extensions;
    for (size_t i = 0; i < options.size(); i++) {
      if (extensions.size() > 0)
        extensions += '|';

      extensions += options[i].first + "|*." + options[i].second;
    }
    set_extensions(backend, extensions, "");
  }
}

//-------------------------------------------------------------------------------------------------

std::string FileChooserWrapper::get_selector_option_value(mforms::FileChooser *backend, const std::string &name) {
  if (name == "format") {
    FileChooserWrapper *wrapper = backend->get_data<FileChooserWrapper>();

    switch (wrapper->type) {
      case mforms::OpenFile:
      case mforms::SaveFile: {
        FileDialog ^ dialog = wrapper->GetManagedObject<FileDialog>();
        if (dialog->FilterIndex > 0)
          return backend->_selector_options["format"][dialog->FilterIndex - 1];
      }

      case mforms::OpenDirectory:
        break;
    }
  }

  return "";
}

//-------------------------------------------------------------------------------------------------

void FileChooserWrapper::init() {
  mforms::ControlFactory *f = mforms::ControlFactory::get_instance();

  f->_filechooser_impl.create = &FileChooserWrapper::create;
  f->_filechooser_impl.set_title = &FileChooserWrapper::set_title;
  f->_filechooser_impl.run_modal = &FileChooserWrapper::run_modal;
  f->_filechooser_impl.set_extensions = &FileChooserWrapper::set_extensions;
  f->_filechooser_impl.set_directory = &FileChooserWrapper::set_directory;
  f->_filechooser_impl.get_directory = &FileChooserWrapper::get_directory;
  f->_filechooser_impl.get_path = &FileChooserWrapper::get_path;
  f->_filechooser_impl.set_path = &FileChooserWrapper::set_path;
  f->_filechooser_impl.add_selector_option = &FileChooserWrapper::add_selector_option;
  f->_filechooser_impl.get_selector_option_value = &FileChooserWrapper::get_selector_option_value;
}

//-------------------------------------------------------------------------------------------------
