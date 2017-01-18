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

/* Notice: This file and the matching cpp file should be completely free
 * of any dependencies to allow it to be compiled as part of mforms
 * instead of wbpublic
 */

#include "common.h"
#include "trackable.h"

#include <vector>
#include <string>

namespace mforms {
  class MenuBar;
  class ToolBar;
};

namespace bec {

  // XXX deprecated
  enum MenuItemType { MenuAction, MenuSeparator, MenuCascade, MenuCheck, MenuRadio, MenuUnavailable };

  // XXX deprecated
  struct MenuItem;
  typedef BASELIBRARY_PUBLIC_FUNC std::vector<MenuItem> MenuItemList;
  struct BASELIBRARY_PUBLIC_FUNC MenuItem {
    std::string oid;
    std::string caption;
    std::string shortcut;
    std::string name;
    MenuItemType type;

    bool enabled;
    bool checked;

    MenuItemList subitems;

    MenuItem() : type(MenuAction), enabled(true), checked(false) {
    }
  };

  // XXX deprecated
  enum ToolbarItemType {
    ToolbarAction,
    ToolbarSeparator,
    ToolbarToggle,
    ToolbarCheck,
    ToolbarRadio,
    ToolbarLabel,
    ToolbarDropDown,
    ToolbarSearch
  };

  // XXX deprecated
  struct BASELIBRARY_PUBLIC_FUNC ToolbarItem {
    int icon;
    int alt_icon;
    std::string caption;
    std::string name;
    std::string command;
    std::string tooltip;
    ToolbarItemType type;

    bool enabled;
    bool checked;

    ToolbarItem() : icon(0), alt_icon(0), type(ToolbarAction), enabled(true), checked(false) {
    }

    ToolbarItem(int aicon, const std::string &acommand, const std::string &atooltip, ToolbarItemType atype)
      : icon(aicon),
        alt_icon(0),
        name(acommand),
        command(acommand),
        tooltip(atooltip),
        type(atype),
        enabled(true),
        checked(false) {
    }
  };

  // XXX deprecated
  typedef std::vector<ToolbarItem> ToolbarItemList;

#define GNUIFormCreated "GNUIFormCreated"
#define GNUIFormDestroyed "GNUIFormDestroyed"

  /** Base class for application forms.
   *
   * This is the base class for application windows or panel backends that can
   * receive focus and contain a workarea in the application (eg: canvas, overview panel,
   * query pages etc). It provides some virtual methods for common editing operations.
   *
   * @ingroup begrt
   */
  class BASELIBRARY_PUBLIC_FUNC UIForm : public base::trackable {
  public:
    UIForm();
    virtual ~UIForm();

    // unique identifier for the form
    std::string form_id();

    virtual std::string get_title() = 0;

    void set_owner_data(void *data);
    void *get_owner_data();

    void set_frontend_data(void *data);
    void *get_frontend_data();

    virtual bool is_main_form();
    virtual std::string get_form_context_name() const = 0;

    // Target description for cut/copy/delete menu items and for paste, after a copy is made.
    virtual std::string get_edit_target_name();

    virtual bool can_undo();
    virtual bool can_redo();
    virtual bool can_cut();
    virtual bool can_copy();
    virtual bool can_paste();
    virtual bool can_delete();
    virtual bool can_select_all();

    virtual void undo();
    virtual void redo();
    virtual void cut();
    virtual void copy();
    virtual void paste();
    virtual void delete_selection();
    virtual void select_all();

    virtual bool can_close() {
      return true;
    }
    virtual void close(){};

    // for main forms
    virtual mforms::MenuBar *get_menubar() {
      return 0;
    }
    virtual mforms::ToolBar *get_toolbar() {
      return 0;
    }

  protected:
    void *_owner_data;
    void *_frontend_data; // No strong reference for OSX!

  public:
    static bec::UIForm *form_with_id(const std::string &id);
  };
};
