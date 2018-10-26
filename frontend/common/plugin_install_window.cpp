/*
 * Copyright (c) 2011, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "plugin_install_window.h"

#include "base/string_utilities.h"
#include "base/file_functions.h"
#include "base/log.h"

#include "grt/icon_manager.h"
#include "grt/grt_manager.h"

#include "workbench/wb_model_file.h"
#include "workbench/wb_context_ui.h"

#include "mforms/progressbar.h"
#include "mforms/imagebox.h"
#include "mforms/label.h"

#include "workbench/wb_context.h"

DEFAULT_LOG_DOMAIN("PlugIn")

class AddOnDownloadWindow::DownloadItem : public mforms::Box {
public:
  DownloadItem(AddOnDownloadWindow *owner, const std::string &url);

  void start();

private:
  AddOnDownloadWindow *_owner;
  mforms::ImageBox _icon;
  mforms::Box _rbox;
  mforms::Box _progress_box;
  mforms::Label _caption;
  mforms::ProgressBar _progress;
  mforms::Button _install;
  mforms::Label _info;

  std::string _url;
  std::string _dest_path;

  void download_finished(grt::ValueRef ret);
  void download_failed(const std::exception &exc);
  grt::ValueRef perform_download();
  void handle_output(const grt::Message &message);
};

AddOnDownloadWindow::DownloadItem::DownloadItem(AddOnDownloadWindow *owner, const std::string &url)
  : mforms::Box(true), _owner(owner), _rbox(false), _progress_box(true), _url(url) {
  set_spacing(20);
  _rbox.set_spacing(4);
  _icon.set_image(bec::IconManager::get_instance()->get_icon_path("MySQLPlugin-48.png"));
  add(&_icon, false, false);
  add(&_rbox, true, true);
  _rbox.add(&_caption, false, true);
  _rbox.add(&_progress_box, false, true);
  _rbox.add(&_info, false, true);

  _progress.set_size(350, -1);
  _info.set_style(mforms::SmallStyle);
  _progress_box.add(&_progress, true, true);

  const char *name = strrchr(_url.c_str(), '/');
  if (!name)
    name = _url.c_str();
  else
    name++;

  _dest_path = base::makePath(bec::GRTManager::get()->get_tmp_dir(), name);
  _caption.set_text(base::strfmt("Downloading %s", name));
  _info.set_text("Preparing...");
  _progress.set_value(0.0);
}

void AddOnDownloadWindow::DownloadItem::download_failed(const std::exception &exc) {
  _info.set_text(base::strfmt("Failed: %s", exc.what()));
  _owner->download_failed(this);
}

void AddOnDownloadWindow::DownloadItem::download_finished(grt::ValueRef ret) {
  std::string fn;
  if (ret.is_valid() && grt::StringRef::can_wrap(ret))
    fn = *grt::StringRef::cast_from(ret);
  if (fn.empty()) {
    _info.set_text("Download failed");
    _owner->download_failed(this);
  } else {
    _info.set_text("Completed");
    _owner->download_finished(fn, this);
  }
}

grt::ValueRef AddOnDownloadWindow::DownloadItem::perform_download() {
  grt::Module *module = grt::GRT::get()->get_module("WbUpdater");
  if (!module)
    throw std::runtime_error("Can't locate module WbUpdater");

  grt::BaseListRef args(true);
  args.ginsert(grt::StringRef(_url));
  args.ginsert(grt::StringRef(_dest_path));

  return module->call_function("downloadFile", args);
}

//--------------------------------------------------------------------------------------------------

void AddOnDownloadWindow::DownloadItem::start() {
  bec::GRTTask::Ref task =
    bec::GRTTask::create_task("downloading plugin", bec::GRTManager::get()->get_dispatcher(),
                              std::bind(&AddOnDownloadWindow::DownloadItem::perform_download, this));

  scoped_connect(task->signal_finished(),
                 std::bind(&AddOnDownloadWindow::DownloadItem::download_finished, this, std::placeholders::_1));
  scoped_connect(task->signal_failed(),
                 std::bind(&AddOnDownloadWindow::DownloadItem::download_failed, this, std::placeholders::_1));
  scoped_connect(task->signal_message(),
                 std::bind(&AddOnDownloadWindow::DownloadItem::handle_output, this, std::placeholders::_1));

  bec::GRTManager::get()->get_dispatcher()->add_task(task);
}

//--------------------------------------------------------------------------------------------------

void AddOnDownloadWindow::DownloadItem::handle_output(const grt::Message &message) {
  if (message.type == grt::InfoMsg) {
    std::vector<std::string> s = base::split(message.text, ":");
    if (s.size() == 3) {
      _progress.set_value(atol(s[0].c_str()) / (float)atol(s[1].c_str()));
      _info.set_text(base::strfmt("%s of %s bytes downloaded", s[0].c_str(), s[1].c_str()));
      return;
    }
  }
}

//------------------------------------------------------------------------------------------------

AddOnDownloadWindow::AddOnDownloadWindow(wb::WBContextUI *wbui)
  : mforms::Form(mforms::Form::main_form()), _box(false), _bbox(true), _wbui(wbui) {
  set_title("Install Add-On");
  set_name("Add on Download");
  setInternalName("add_on_download");
  set_content(&_box);
  _box.set_padding(20);
  _box.set_spacing(20);
  _bbox.set_spacing(12);
  _cancel.set_text("Cancel");
  _bbox.add_end(&_cancel, false, true);
  _box.add_end(&_bbox, false, false);
}

void AddOnDownloadWindow::install_addon_from_url(const std::string &url) {
  DownloadItem *item = mforms::manage(new DownloadItem(this, url));
  _items.push_back(item);
  _box.add(item, false, true);
  item->start();

  bool canceled = run_modal(0, &_cancel) == false;

  _items.erase(std::find(_items.begin(), _items.end(), item));
  _box.remove(item);
  delete item;

  if (!canceled)
    _wbui->get_wb()->open_file_by_extension(_final_path, true);
}

void AddOnDownloadWindow::download_failed(DownloadItem *item) {
  // dont auto-close, let user click Cancel after reading error
}

void AddOnDownloadWindow::download_finished(const std::string &path, DownloadItem *item) {
  end_modal(true);
  show(false);
  _final_path = path;
}

//------------------------------------------------------------------------------------------------

class PluginInstallWindow::InstallItem : public mforms::Box {
  mforms::Box _box, _rbox;
  mforms::ImageBox _icon;
  mforms::Label _version;
  mforms::Label _author;
  mforms::Label _name;
  mforms::Label _description;
  mforms::Label _info_caption;

  std::string _path;

public:
  InstallItem(PluginInstallWindow *owner, const std::string &path);

  bool start();
};

PluginInstallWindow::InstallItem::InstallItem(PluginInstallWindow *owner, const std::string &path)
  : mforms::Box(true), _box(true), _rbox(false), _path(path) {
  set_padding(8);
  set_spacing(12);
  _box.set_spacing(20);
  _rbox.set_spacing(4);
  _description.set_style(mforms::SmallStyle);
  _info_caption.set_style(mforms::InfoCaptionStyle);
  _info_caption.set_color("#999999");

  add(&_box, true, true);
  _box.add(&_icon, false, true);
  _box.add(&_rbox, true, true);

  _rbox.add(&_name, false, true);
  _rbox.add(&_description, false, true);
  _rbox.add(&_version, false, true);
  _rbox.add(&_author, false, true);
  _rbox.add(&_info_caption, false, true);
}

static std::string full_file_path(const std::list<std::string> &paths, const std::string &file) {
  for (std::list<std::string>::const_iterator i = paths.begin(); i != paths.end(); ++i) {
    if (g_str_has_suffix(i->c_str(), file.c_str()))
      return *i;
  }
  return "";
}

bool PluginInstallWindow::InstallItem::start() {
  grt::DictRef manifest;
  std::string unpacked_path;

  unpacked_path = _path.substr(0, _path.size() - 1);
  std::list<std::string> contents(wb::ModelFile::unpack_zip(_path, unpacked_path));

  {
    std::string manifest_path = full_file_path(contents, "manifest.xml");
    if (manifest_path.empty()) {
      mforms::Utilities::show_error("Invalid Plugin", "The plugin does not contain a manifest file.", "OK", "", "");
      base_rmdir_recursively(unpacked_path.c_str());
      return false;
    }
    try {
      manifest = grt::DictRef::cast_from(grt::GRT::get()->unserialize(manifest_path));
    } catch (const std::exception &) {
      mforms::Utilities::show_error("Invalid Plugin", "There was an error reading the manifest file from the plugin.",
                                    "OK", "", "");
      base_rmdir_recursively(unpacked_path.c_str());
      return false;
    }
  }

  grt::BaseListRef plugins;

  if (!grt::BaseListRef::can_wrap(manifest.get("plugins"))) {
    mforms::Utilities::show_error("Invalid Plugin", "The manifest file is invalid.", "OK", "", "");
    base_rmdir_recursively(unpacked_path.c_str());
    return false;
  }
  plugins = grt::BaseListRef::cast_from(manifest.get("plugins"));

  if (plugins.count() == 0) {
    mforms::Utilities::show_error("Invalid Plugin", "No plugin functionality was found.", "OK", "", "");
    base_rmdir_recursively(unpacked_path.c_str());
    return false;
  }

  _icon.set_image(full_file_path(contents, manifest.get_string("iconFile")));
  _name.set_text(base::strfmt("%s", manifest.get_string("name").c_str()));
  _description.set_text(base::strfmt("%s", manifest.get_string("description").c_str()));
  _version.set_text(base::strfmt("Version: %s", manifest.get_string("version").c_str()));
  _author.set_text(base::strfmt("Author: %s", manifest.get_string("author").c_str()));
  _info_caption.set_text(base::strfmt("This plugin contains the following functionality:"));

  for (size_t c = plugins.count(), i = 0; i < c; i++) {
    mforms::Label *l = mforms::manage(new mforms::Label(grt::DictRef::cast_from(plugins[i]).get_string("caption")));
    _rbox.add(l, false, true);
    l = mforms::manage(new mforms::Label(grt::DictRef::cast_from(plugins[i]).get_string("description")));
    _rbox.add(l, false, true);
    l->set_style(mforms::SmallStyle);
  }

  base_rmdir_recursively(unpacked_path.c_str());
  return true;
}

//------------------------------------------------------------------------------------------------

PluginInstallWindow::PluginInstallWindow(wb::WBContextUI *wbui)
  : mforms::Form(mforms::Form::main_form()), _box(false), _bbox(true), _wbui(wbui) {
  set_title("Install Add-On");
  set_name("Plugin Installation");
  setInternalName("plugin_installation");
  set_content(&_box);
  _box.set_padding(20);
  _box.set_spacing(20);
  _bbox.set_spacing(12);
  _cancel.set_text("Cancel");
  mforms::Utilities::add_end_ok_cancel_buttons(&_bbox, &_ok, &_cancel);

  _box.add(mforms::manage(new mforms::Label("WARNING: Only install plugins from authors you trust.\nMalicious plugins "
                                            "could pose a security threat to your computer.")),
           false, true);
  _box.add_end(&_bbox, false, false);

  set_size(400, -1);
}

bool PluginInstallWindow::install_plugin(const std::string &path) {
  InstallItem item(this, path);
  _box.add(&item, false, true);
  try {
    if (!item.start()) {
      _box.remove(&item);
      return false;
    }
  } catch (const grt::type_error &exc) {
    mforms::Utilities::show_error("Invalid Plugin", "Error processing plugin, it is probably invalid.", "OK", "", "");
    logWarning("Error: manifest file for %s is invalid, exception while processing: %s\n", path.c_str(), exc.what());
    _box.remove(&item);
    return false;
  }

  _ok.show(true);
  _ok.set_text("Install");
  scoped_connect(_ok.signal_clicked(), std::bind(&mforms::Form::end_modal, this, true));

  if (!run_modal(0, &_cancel)) {
    _box.remove(&item);
    return false;
  }
  _box.remove(&item);
  _wbui->get_wb()->install_module_file(path);
  return true;
}

//-----------------------------------------------------------
