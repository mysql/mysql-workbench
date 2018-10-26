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

#pragma once

namespace MySQL {
  namespace Base {

  public
    enum class MenuItemType {
      MenuAction = bec::MenuAction,
      MenuSeparator = bec::MenuSeparator,
      MenuCascade = bec::MenuCascade,
      MenuCheck = bec::MenuCheck,
      MenuRadio = bec::MenuRadio,

      MenuUnavailable = bec::MenuUnavailable
    };

  public
    ref class MenuItem {
      System::String ^ caption;
      System::String ^ shortcut;
      System::String ^ internalName;
      MenuItemType type;

      bool enabled;
      bool checked;

      System::Collections::Generic::List<MenuItem ^> ^ subitems;

    public:
      MenuItem(const bec::MenuItem &item);

      System::String ^ get_caption();
      System::String ^ get_shortcut();
      System::String ^ getInternalName();
      MenuItemType get_type();

      bool get_checked();
      void set_checked(bool value);

      bool get_enabled();
      void set_enabled(bool value);

      System::Collections::Generic::List<MenuItem ^> ^ get_subitems();
    };

  public
    enum class ToolbarItemType {
      ToolbarAction = bec::ToolbarAction,
      ToolbarSeparator = bec::ToolbarSeparator,
      ToolbarToggle = bec::ToolbarToggle,
      ToolbarLabel = bec::ToolbarLabel,
      ToolbarDropDown = bec::ToolbarDropDown,
      ToolbarRadio = bec::ToolbarRadio,
      ToolbarCheck = bec::ToolbarCheck,
      ToolbarSearch = bec::ToolbarSearch
    };

  public
    ref class ToolbarItem {
      int icon;
      int alt_icon;
      System::String ^ internalName;
      System::String ^ caption;
      System::String ^ command;
      System::String ^ tooltip;
      ToolbarItemType type;

      bool enabled;
      bool checked;

    public:
      ToolbarItem(const bec::ToolbarItem &item)
        : icon(item.icon),
          alt_icon(item.alt_icon),
          internalName(CppStringToNative(item.name)),
          caption(CppStringToNative(item.caption)),
          command(CppStringToNative(item.command)),
          tooltip(CppStringToNative(item.tooltip)),
          type((ToolbarItemType)item.type),
          enabled(item.enabled),
          checked(item.checked) {
      }

      int get_icon() {
        return icon;
      }

      int get_alt_icon() {
        return alt_icon;
      }

      System::String ^ getInternalName() { return internalName; }

        System::String
        ^ get_caption() { return caption; }

        System::String
        ^ get_command() { return command; }

        System::String
        ^ get_tooltip() { return tooltip; }

        ToolbarItemType get_type() {
        return type;
      }

      bool get_checked() {
        return checked;
      }

      bool get_enabled() {
        return enabled;
      }
    };

  public
    ref class UIForm {
    protected:
      bec::UIForm *inner;
      System::Runtime::InteropServices::GCHandle m_gch;

      UIForm(bec::UIForm *inn);
      UIForm();

      System::IntPtr GetFixedId();
      void ReleaseHandle();

    public:
      virtual ~UIForm();

      void init(bec::UIForm *inn);
      bec::UIForm *get_unmanaged_object();
      static UIForm ^ GetFromFixedId(System::IntPtr ip);
      bool can_close();
      void close();
      System::String ^ get_title();
      System::String ^ form_id();
    };
  }
}
