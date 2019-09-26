/*
 * Copyright (c) 2008, 2019, Oracle and/or its affiliates. All rights reserved.
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

/*! \mainpage MForms - mini forms library
 *
 *  \anchor MForms
 *
 * \section Introduction
 * MForms is a small GUI toolkit library written for use in the "a href="http://wb.mysql.com""MySQL Workbench"/a"
 * project. It is small and cross-platform,
 * while providing access to fully native controls (through .NET in Windows, GTK in Linux and Cocoa in MacOSX).
 * It is not meant to be a full-fledged control library, but instead a light-weight and simple library that allows
 * writing simple forms that will work in any supported platform. It is used by Workbench in some specific parts where
 * the UI requirements are not very complex, such as in wizards, simple confirmation dialogs and the Administrator
 * feature.
 * The library is written in C++, but a Python wrapper is available, making it possible to easily write cross-platform
 * plugins
 * with simple to moderately complex GUIs.
 *
 * \section Layouting
 * Because each platform has widgets of different sizes, with different layout characteristics,
 * using absolute coordinates and sizes for layouting would not work very well. A dialog written
 * in Windows containing text and a couple of buttons could look fine in the OS where it was written in,
 * but would look cluttered and probably too small on macOS. To work around that, MForms uses dynamic
 * layouting containers instead of static coordinates to place and size controls, similar to what GTK uses.
 * Basically, there are several types of containers where you can place one or more controls inside, each
 * having a specific layouting behavior.
 * \li Form - A top level window which can contain a single control (usually another container).
 * The window will be sized automatically to fit its contents,  but can also be set to a size by hand.
 * \li Box - A container that can be filled with one or more controls in a vertical or horizontal layout.
 * Each child control can be set to either use only the minimal space required by it or to fill up all space available
 * in the box in the direction of the layout. In the direction perpendicular to the layout (ie vertical
 * in a horizontal box and vice-versa), the smallest possible size that can accommodate all child controls
 * will be applied to all its contents.
 * \li Table - A container that can organize one or more controls in a grid. You may set the number of rows
 *  and columns in the table and place your controls in specific cells of that grid.
 * \li ScrollView - A container that can contain a single other control and will add scrollbars if the contents don't
 * fit the available space.
 *
 */

#include "mforms/view.h"
#include "mforms/form.h"
#include "mforms/button.h"
#include "mforms/checkbox.h"
#include "mforms/textentry.h"
#include "mforms/textbox.h"
#include "mforms/label.h"
#include "mforms/selector.h"
#include "mforms/listbox.h"
#include "mforms/tabview.h"
#include "mforms/box.h"
#include "mforms/panel.h"
#include "mforms/filechooser.h"
#include "mforms/radiobutton.h"
#include "mforms/imagebox.h"
#include "mforms/progressbar.h"
#include "mforms/table.h"
#include "mforms/scrollpanel.h"
#include "mforms/treeview.h"
#include "mforms/wizard.h"
#include "mforms/drawbox.h"
#include "mforms/tabswitcher.h"
#include "mforms/app.h"
#include "mforms/appview.h"
#include "mforms/utilities.h"
#include "mforms/uistyle.h"
#include "mforms/appview.h"
#include "mforms/sectionbox.h"
#include "mforms/widgets.h"
#include "mforms/menu.h"
#include "mforms/splitter.h"
#include "mforms/popup.h"
#include "mforms/code_editor.h"
#include "mforms/menubar.h"
#include "mforms/toolbar.h"
#include "mforms/hypertext.h"
#include "mforms/popover.h"
#include "mforms/fs_object_selector.h"
#include "mforms/simpleform.h"
#include "mforms/find_panel.h"
#include "mforms/native.h"
#include "mforms/canvas.h"
#include "mforms/gridview.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace mforms {

  class MFORMS_EXPORT ControlFactory {
  public:
    ViewImplPtrs _view_impl;
    FormImplPtrs _form_impl;
    BoxImplPtrs _box_impl;
    ButtonImplPtrs _button_impl;
    CheckBoxImplPtrs _checkbox_impl;
    TextEntryImplPtrs _textentry_impl;
    TextBoxImplPtrs _textbox_impl;
    LabelImplPtrs _label_impl;
    SelectorImplPtrs _selector_impl;
    ListBoxImplPtrs _listbox_impl;
    TabViewImplPtrs _tabview_impl;
    PanelImplPtrs _panel_impl;
    FileChooserImplPtrs _filechooser_impl;
    RadioButtonImplPtrs _radio_impl;
    ImageBoxImplPtrs _imagebox_impl;
    ProgressBarImplPtrs _progressbar_impl;
    TableImplPtrs _table_impl;
    ScrollPanelImplPtrs _spanel_impl;
    WizardImplPtrs _wizard_impl;
    DrawBoxImplPtrs _drawbox_impl;
    MenuImplPtrs _menu_impl;
    SplitterImplPtrs _splitter_impl;
    PopupImplPtrs _popup_impl;
    CodeEditorImplPtrs _code_editor_impl;
    MenuItemImplPtrs _menu_item_impl;
    ToolBarImplPtrs _tool_bar_impl;
    HyperTextImplPtrs _hypertext_impl;
    PopoverImplPtrs _popover_impl;
    CanvasImplPtrs _canvas_impl;

    AppImplPtrs _app_impl;
    AppViewImplPtrs _app_view_impl;
    UtilitiesImplPtrs _utilities_impl;

  public:
    TreeViewImplPtrs _treeview_impl;
    FindPanelImplPtrs _findpanel_impl;

    ControlFactory();
    ~ControlFactory();

    static ControlFactory *get_instance();

    void check_impl();
    void shutdown();
  };
};

#endif
