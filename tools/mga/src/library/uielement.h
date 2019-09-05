/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "jsexport.h"

namespace mga {
  class AutomationContext;
  class ScriptingContext;
  class UIRootElement;

  // The main interface to the accessibility/automation layer on each platform.
  class UIElement : public JSExport {
    friend class APath;
    friend class AutomationContext;
  public:
    UIElement() = delete;
    UIElement(aal::AccessibleRef ref, UIRootElement *root);
    virtual ~UIElement();

    UIElement* operator = (UIElement const& other) = delete;
    UIElementRef clone() const;

    bool isValid() const;
    std::string getName() const;
    aal::Role getRole() const;
    
    bool hasChild(UIElement *child) const;
    UIElementRef childById(std::string const& name, bool throwIfMoreThanOne = true) const;
    UIElementRef childByName(std::string const& name, bool throwIfMoreThanOne = true) const;
    UIElementRef containingRow() const;
    UIElementRef horizontalScrollBar() const;
    UIElementRef verticalScrollBar() const;
    UIElementRef header() const;
    UIElementRef closeButton() const;

    bool equals(const UIElement *other) const;

    UIElementRef getParent() const;
    UIElementList children() const;
    UIElementList childrenRecursive() const;
    UIElementList windows() const;
    UIElementList tabPages() const;
    UIElementList rows() const;
    UIElementList rowEntries() const;
    UIElementList columns() const;
    UIElementList columnEntries() const;

    static UIElementRef fromPoint(geometry::Point point, UIElement *application);
    geometry::Point convertToClient(geometry::Point point) const;
    geometry::Point convertToScreen(geometry::Point point) const;
    geometry::Point convertToTarget(UIElement *target, geometry::Point point) const;

    static void registerInContext(ScriptingContext &context, JSObject &exports);

  protected:
    aal::AccessibleRef _accessible;
    UIRootElement *_root;

    static JSVariant getter(ScriptingContext *context, JSExport *element, std::string const& name);
    static void setter(ScriptingContext *context, JSExport *element, std::string const& name, JSVariant value);

    static UIElement* validate(JSExport *element);

    static void defineUIApplication(ScriptingContext &context, JSObject &module);
    static void defineUIWindow(ScriptingContext &context, JSObject &module);
    static void defineUIButtonBase(ScriptingContext &context, JSObject &module);
    static void defineUIButton(ScriptingContext &context, JSObject &module);
    static void defineUIRadioButton(ScriptingContext &context, JSObject &module);
    static void defineUIRadioGroup(ScriptingContext &context, JSObject &module);
    static void defineUICheckBox(ScriptingContext &context, JSObject &module);
    static void defineUIComboBox(ScriptingContext &context, JSObject &module);
    static void defineUIExpander(ScriptingContext &context, JSObject &module);
    static void defineUIGrid(ScriptingContext &context, JSObject &module);
    static void defineUITextBox(ScriptingContext &context, JSObject &module);
    static void defineUITreeView(ScriptingContext &context, JSObject &module);
    static void defineUILabel(ScriptingContext &context, JSObject &module);
    static void defineUIPane(ScriptingContext &context, JSObject &module);

    static void defineUIMenuBase(ScriptingContext &context, JSObject &module);
    static void defineUIMenu(ScriptingContext &context, JSObject &module);
    static void defineUIMenuBar(ScriptingContext &context, JSObject &module);
    static void defineUIMenuItem(ScriptingContext &context, JSObject &module);
    static void defineUISeparator(ScriptingContext &context, JSObject &module);

    static void defineUISplitContainer(ScriptingContext &context, JSObject &module);
    static void defineUISplitter(ScriptingContext &context, JSObject &module);
    static void defineUIGroupBox(ScriptingContext &context, JSObject &module);
    static void defineUIImage(ScriptingContext &context, JSObject &module);
    static void defineUITabView(ScriptingContext &context, JSObject &module);
    static void defineUITabPage(ScriptingContext &context, JSObject &module);
    static void defineUIDatePicker(ScriptingContext &context, JSObject &module);
    static void defineUIRow(ScriptingContext &context, JSObject &module);
    static void defineUIColumn(ScriptingContext &context, JSObject &module);
    static void defineUICell(ScriptingContext &context, JSObject &module);
    static void defineUIScrollBox(ScriptingContext &context, JSObject &module);

    static void defineUISlider(ScriptingContext &context, JSObject &module);
    static void defineUIStepper(ScriptingContext &context, JSObject &module);
    static void defineUIList(ScriptingContext &context, JSObject &module);
    static void defineUIIconView(ScriptingContext &context, JSObject &module);
    static void defineUIProgress(ScriptingContext &context, JSObject &module);
    static void defineUIBusy(ScriptingContext &context, JSObject &module);

    static void defineUIScrollBar(ScriptingContext &context, JSObject &module);
    static void defineUIScrollThumb(ScriptingContext &context, JSObject &module);

    static void defineUIHyperLink(ScriptingContext &context, JSObject &module);
  };

  class UIRootElement : public UIElement {
  public:
    UIRootElement(AutomationContext &context, aal::AccessibleRef ref);

    AutomationContext& context() { return _context; };
  private:
    AutomationContext &_context;
  };
}
