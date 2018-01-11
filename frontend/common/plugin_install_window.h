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

#ifndef PLUGIN_INSTALL_WINDOW_H_
#define PLUGIN_INSTALL_WINDOW_H_

#include "mforms/form.h"
#include "mforms/box.h"
#include "mforms/button.h"

namespace wb {
  class WBContextUI;
};

class AddOnDownloadWindow : public mforms::Form {
  class DownloadItem;
  friend class DownloadItem;

public:
  AddOnDownloadWindow(wb::WBContextUI *wbui);

  void install_addon_from_url(const std::string &url);

private:
  mforms::Box _box;
  mforms::Box _bbox;
  mforms::Button _cancel;
  std::list<mforms::View *> _items;
  wb::WBContextUI *_wbui;
  std::string _final_path;

  void download_finished(const std::string &path, DownloadItem *item);
  void download_failed(DownloadItem *item);
};

class PluginInstallWindow : public mforms::Form {
  class InstallItem;
  friend class DownloadItem;
  friend class InstallItem;

public:
  PluginInstallWindow(wb::WBContextUI *wbui);

  bool install_plugin(const std::string &path);

private:
  mforms::Box _box;
  mforms::Box _bbox;
  mforms::Button _ok;
  mforms::Button _cancel;
  wb::WBContextUI *_wbui;
};

#endif
