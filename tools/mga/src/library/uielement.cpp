/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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

#include "accessible.h"
#include "context-management.h"
#include "utilities.h"
#include "filesystem.h"
#include "apath.h"
#include "platform.h"

#include "uielement.h"

using namespace mga;
using namespace aal;
using namespace geometry;

//----------------- UIElement ------------------------------------------------------------------------------------------

UIElement::UIElement(AccessibleRef ref, UIElement *root) : _accessible(std::move(ref)), _root(root) {
  if (_root == nullptr)
    _root = this;
}

//----------------------------------------------------------------------------------------------------------------------

UIElement::~UIElement() {
}

//----------------------------------------------------------------------------------------------------------------------

bool UIElement::isValid() const {
  return _accessible && _accessible->isValid();
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a copy of this element which can be freed independently without affecting this instance.
 */
UIElementRef UIElement::clone() const {
  return UIElementRef(new UIElement(_accessible->clone(), _root));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns true if `child` is a direct or indirect child element of this UI element.
 */
bool UIElement::hasChild(UIElement *child) const {
  UIElementRef parent = child->getParent();
  while (parent) {
    if (parent->equals(this))
      return true;

    if (parent->_accessible->isRoot())
      break;
    parent = parent->getParent();
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the child/grand child with the given name. If there's more than one an exception is thrown
 * (unless throwIfMoreThanOne is false).
 */
UIElementRef UIElement::childByName(std::string const& name, bool throwIfMoreThanOne) const {
  APath searcher(_root);
  auto searchResult = searcher.execute(const_cast<UIElement *>(this), "descendant[@name='" + name + "']", false, false);
  if (searchResult.size() > 1 && throwIfMoreThanOne)
    throw std::runtime_error("Found more than one child with that name");
  if (searchResult.empty())
    return nullptr;
  return std::move(searchResult[0]);
}

//----------------------------------------------------------------------------------------------------------------------

UIElementRef UIElement::containingRow() const {
  if (isValid())
    return UIElementRef(new UIElement(_accessible->getContainingRow(), _root));
  return UIElementRef();
}

//----------------------------------------------------------------------------------------------------------------------

UIElementRef UIElement::horizontalScrollBar() const {
  if (isValid())
    return UIElementRef(new UIElement(_accessible->getHorizontalScrollBar(), _root));
  return UIElementRef();
}

//----------------------------------------------------------------------------------------------------------------------

UIElementRef UIElement::verticalScrollBar() const {
  if (isValid())
    return UIElementRef(new UIElement(_accessible->getVerticalScrollBar(), _root));
  return UIElementRef();
}

//----------------------------------------------------------------------------------------------------------------------

UIElementRef UIElement::header() const {
  if (isValid())
    return UIElementRef(new UIElement(_accessible->getHeader(), _root));
  return UIElementRef();
}

//----------------------------------------------------------------------------------------------------------------------

UIElementRef UIElement::closeButton() const {
  if (isValid())
    return UIElementRef(new UIElement(_accessible->getCloseButton(), _root));
  return UIElementRef();
}

//----------------------------------------------------------------------------------------------------------------------

bool UIElement::equals(const UIElement *other) const {
  if (!isValid())
    return false;

  return _accessible->equals(other->_accessible.get());
}

//----------------------------------------------------------------------------------------------------------------------

UIElementRef UIElement::getParent() const {
  if (isValid())
    return UIElementRef(new UIElement(_accessible->getParent(), _root));
  return UIElementRef();
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::children() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->children())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}


//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::childrenRecursive() const {
  if (!isValid())
    return {};
  
  UIElementList result;
  AccessibleVector accessibles;

  _accessible->children(accessibles, true);

  for (auto &entry : accessibles)
    result.emplace_back(new UIElement(std::move(entry), _root));
    
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::windows() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->windows())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::tabPages() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->tabPages())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::rows() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->rows())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::rowEntries() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->rowEntries())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::columns() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->columns())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

UIElementList UIElement::columnEntries() const {
  if (isValid()) {
    UIElementList result;
    for (auto &entry : _accessible->columnEntries())
      result.emplace_back(new UIElement(std::move(entry), _root));

    return result;
  }
  return {};
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the UI element found at the given screen position. Only the current application is considered here.
 */
UIElementRef UIElement::fromPoint(geometry::Point point, UIElement *application) {
  return UIElementRef(new UIElement(Accessible::fromPoint(point, application->_accessible.get()), application->_root));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts the given point (in screen coordinates) into local coordinates of this element.
 */
geometry::Point UIElement::convertToClient(geometry::Point point) const {
  if (!isValid())
    return point;

  geometry::Point position = _accessible->getBounds(true).position;
  return { point.x - position.x, point.y - position.y };
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Converts the given point (in local coordinates of this element) to screen coordintates.
 */
geometry::Point UIElement::convertToScreen(geometry::Point point) const {
  if (!isValid())
    return point;

  geometry::Point position = _accessible->getBounds(true).position;
  return { point.x + position.x, point.y + position.y };
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Point UIElement::convertToTarget(UIElement *target, geometry::Point point) const {
  if (!isValid())
    return geometry::Point();

  // Convert the given point to screen coordinates.
  geometry::Point position = _accessible->getBounds(true).position;
  auto targetPos = target->_accessible->getBounds(true).position;
  return { point.x + position.x - targetPos.x, point.y + position.y - targetPos.y };
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<std::string, size_t> _propertyMap = {
  { "text", 0 },
  { "title", 1 },
  { "description", 2 },
  { "enabled", 3 },
  { "canExpand", 4 },
  { "expanded", 5 },
  { "canFocus", 6 },
  { "focused", 7 },
  { "checkState", 8 },
  { "value", 9 },
  { "maxValue", 10 },
  { "minValue", 11 },
  { "activeTab", 12 },
  { "active", 13 },
  { "editable", 14 }, // Not the same as enabled.
  { "selectedIndexes", 15 },
  { "readOnly", 16 },
  { "isSecure", 17 },
  { "caretPosition", 18 },
  { "selectedText", 19 },
  { "selectionRange", 20 },
  { "characterCount", 21 },
  { "horizontal", 22 },
  { "date", 23 },
  { "selectedIndex", 24 },
  { "selected", 25 },
  { "help", 26 },
  { "horizontal", 27 },
  { "range", 28 },
  { "position", 29 },
};

JSVariant UIElement::getter(ScriptingContext *context, JSExport *element, std::string const& name) {
  auto me = dynamic_cast<UIElement *>(element);
  if (!me->isValid())
    return JSVariant();

  switch (_propertyMap[name]) {
    case 0:
      return me->_accessible->getText();
    case 1:
      return me->_accessible->getTitle();
    case 2:
      return me->_accessible->getDescription();
    case 3:
      return me->_accessible->isEnabled();
    case 4:
      return me->_accessible->isExpandable();
    case 5:
      return me->_accessible->isExpanded();
    case 6:
      return me->_accessible->canFocus();
    case 7:
      return me->_accessible->isFocused();
    case 8:
      return static_cast<int>(me->_accessible->getCheckState());
    case 9:
      return me->_accessible->getValue();
    case 10:
      return me->_accessible->getMaxValue();
    case 11:
      return me->_accessible->getMinValue();
    case 12:
      return me->_accessible->getActiveTabPage();
    case 13:
      return me->_accessible->isActiveTab();
    case 14:
      return me->_accessible->isEditable();
    case 15: {
      JSArray result(context);
      for (auto index : me->_accessible->getSelectedIndexes())
        result.addValue(index);
      return result;
    }
    case 16:
      return me->_accessible->isReadOnly();
    case 17:
      return me->_accessible->isSecure();
    case 18:
      return me->_accessible->getCaretPosition();
    case 19:
      return me->_accessible->getSelectedText();
    case 20:
      return me->_accessible->getSelectionRange();
    case 21:
      return me->_accessible->getCharacterCount();
    case 22:
      return me->_accessible->isHorizontal();
    case 23:
      return me->_accessible->getDate();
    case 24: {
      auto indexes = me->_accessible->getSelectedIndexes();
      if (indexes.empty())
        return -1;
      return *indexes.begin();
    }
    case 25:
      return me->_accessible->isSelected();
    case 26:
      return me->_accessible->getHelp();
    case 27:
      return me->_accessible->isHorizontal();
    case 28:
      return me->_accessible->getRange();
    case 29:
      return me->_accessible->getScrollPosition();
    default:
      return JSVariant();
  }
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::setter(ScriptingContext *context, JSExport *element, std::string const& name, JSVariant value) {
  std::ignore = context;
  
  auto me = dynamic_cast<UIElement *>(element);
  if (!me->isValid())
    return;

  switch (_propertyMap[name]) {
    case 0:
      me->_accessible->setText(value);
      break;
    case 1:
      me->_accessible->setTitle(value);
      break;
    case 3:
      me->_accessible->setEnabled(value);
      break;
    case 5:
      me->_accessible->setExpanded(value);
      break;
    case 7:
      if (value)
        me->_accessible->setFocused();
      break;
    case 8: {
      int state = value;
      me->_accessible->setCheckState(static_cast<CheckState>(state));
      break;
    }
    case 9:
      me->_accessible->setValue(value);
      break;
    case 12:
      me->_accessible->setActiveTabPage(value);
      break;
    case 13:
      me->_accessible->activate();
      break;
    case 15: {
      std::set<size_t> indices;
      JSArray entries = value;
      for (size_t i = 0; i < entries.size(); ++ i)
        indices.insert(entries.get(i));
      me->_accessible->setSelectedIndexes(indices);
      break;
    }
    case 18:
      me->_accessible->setCaretPosition(value);
      break;
    case 19:
      me->_accessible->setSelectedText(value);
      break;
    case 20: {
      JSObject temp = value;
      aal::TextRange range;
      range.start = temp.get("start", 0);
      range.end = temp.get("end", 0);
      me->_accessible->setSelectionRange(range);
      break;
    }
    case 23:
      me->_accessible->setDate(value);
      break;
    case 24: {
      std::set<size_t> indices;
      ssize_t index = value;
      if (index >= 0)
        indices.insert(static_cast<size_t>(index));
      me->_accessible->setSelectedIndexes(indices);
#ifdef __APPLE__
      me->_accessible->confirm(false); // Required for certain types of controls.
#endif
      break;
    }
    case 25:
      me->_accessible->setSelected(value);
      break;
    case 29:
      me->_accessible->setScrollPosition(value);
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Checks if the given element is a UIElement instance and is valid.
 * Throws an exception if that is not the case, otherwise returns the UIElement reference.
 */
UIElement* UIElement::validate(JSExport *element) {
  UIElement *result = dynamic_cast<UIElement *>(element);
  if (result == nullptr || !result->isValid())
    throw std::runtime_error("UIElement is not valid or undefined");

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

// For certain roles we'll create a dedicated JS class with specific properties/methods.
static std::map<aal::Role, std::string> _roleToElement {
  { Role::Unknown, "UIElement" },
  { Role::Any, "" },
  { Role::Application, "UIElement" },
  { Role::Window, "UIWindow" },
  { Role::Button, "UIButton" },
  { Role::RadioButton, "UIRadioButton" },
  { Role::CheckBox, "UICheckBox" },
  { Role::ComboBox, "UIComboBox" },
  { Role::Expander, "UIExpander" },
  { Role::Grid, "UIGrid" },
  { Role::TextBox, "UITextBox" },
  { Role::TreeView, "UITreeView" },
  { Role::Label, "UILabel" },
  { Role::Pane, "UIPane" },
  { Role::Menu, "UIMenu" },
  { Role::MenuBar, "UIMenuBar" },
  { Role::MenuItem, "UIMenuItem" },
  { Role::Separator, "UISeparator" },
  { Role::SplitContainer, "UISplitContainer" },
  { Role::GroupBox, "UIGroupBox" },
  { Role::Image, "UIImage" },
  { Role::TabView, "UITabView" },
  { Role::TabPage, "UITabPage" },
  { Role::DatePicker, "UIDatePicker" },
  { Role::Row, "UIRow" },
  { Role::Column, "UIColumn" },
  { Role::Cell, "UICell" },
  { Role::ScrollBox, "UIScrollBox" },
  { Role::Slider, "UISlider" },
  { Role::Stepper, "UIStepper" },
  { Role::List, "UIList" },
  { Role::IconView, "UIIconView" },
  { Role::ProgressIndicator, "UIProgress" },
  { Role::BusyIndicator, "UIBusy" },
  { Role::ScrollBar, "UIScrollBar" },
  { Role::ScrollThumb, "UIScrollThumb" }
};

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIWindow(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIWindow", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("title", getter, setter);
    prototype.defineVirtualProperty("screen",  [](ScriptingContext *context, JSExport *element,
                                                  std::string const&) -> JSVariant {
      geometry::Rectangle bounds = validate(element)->_accessible->getBounds(true);
      for (auto &screen : Platform::get().getScreens()) {
        if (screen.bounds.contains({ bounds.position.x, bounds.position.y })) {
          JSObject result(context);
          result.defineProperty("bounds", screen.bounds);
          result.defineProperty("resolutionX", screen.resolutionX);
          result.defineProperty("resolutionY", screen.resolutionY);
          result.defineProperty("scaleFactor", screen.scaleFactor);
          return result;
        }
      }
      return JSVariant();
    }, nullptr);

    prototype.defineFunction({ "takeScreenShot" }, 3, [](JSExport *element, JSValues &args) {
      // parameters: path, flag, rectangle
      auto me = validate(element);
      std::string path = args.get(0);
      bool onlyWindow = args.get(1);
      geometry::Rectangle rect = args.getRectangle(2);

      FS::ensureDir(path);

      // Construct a file name for the screenshot consisting of the application name and a timestamp.
      auto tm = std::time(nullptr);
      auto ltm = std::localtime(&tm);

      auto now = std::chrono::system_clock::now();
      auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

      std::stringstream ss;
      ss << path << "/mga_" << std::put_time(ltm, "%Y%m%d_%H%M%S") << "." << ms.count() << ".png";

      path = ss.str();
      me->_accessible->takeScreenShot(path, onlyWindow, rect);

      args.pushResult(path);
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIButtonBase(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIButtonBase", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("title", getter, setter);

    prototype.defineFunction( { "press" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->click();
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIButton(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIButton", "UIButtonBase", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIButtonBase", { backend });
  }, [](JSObject &/*prototype*/) {
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIRadioButton(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIRadioButton", "UIButtonBase", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIButtonBase", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("checkState", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUICheckBox(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UICheckBox", "UIButtonBase", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIButtonBase", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("checkState", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIComboBox(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIComboBox", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("editable", getter, nullptr);
    prototype.defineVirtualProperty("text", getter, setter);
    prototype.defineVirtualProperty("selectedIndex", getter, setter);
    prototype.defineVirtualProperty("expanded", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIExpander(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIExpander", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("expanded", getter, setter);
    prototype.defineVirtualProperty("canExpand", getter, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIGrid(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIGrid", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("rows",  [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      auto rows = me->rows();

      JSArray list(context);
      for (size_t i = 0; i < rows.size(); ++i) {
        UIElement *row = rows[i].release();
        list.addValue(context->createJsInstance(_roleToElement[row->_accessible->getRole()], { row }));
      }
      return list;
    }, nullptr);

    prototype.defineVirtualProperty("columns",  [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      auto columns = me->columns();

      JSArray list(context);
      for (size_t i = 0; i < columns.size(); ++i) {
        UIElement *column = columns[i].release();
        list.addValue(context->createJsInstance(_roleToElement[column->_accessible->getRole()], { column }));
      }
      return list;
    }, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUITextBox(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UITextBox", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("text", getter, setter);
    prototype.defineVirtualProperty("readOnly", getter, nullptr);
    prototype.defineVirtualProperty("isSecure", getter, nullptr);
    prototype.defineVirtualProperty("caretPosition", getter, setter);
    prototype.defineVirtualProperty("selectedText", getter, setter);
    prototype.defineVirtualProperty("selectionRange", getter, setter);
    prototype.defineVirtualProperty("characterCount", getter, nullptr);

    prototype.defineFunction({ "insertText" }, 2, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      int offset = args.get(0);
      std::string text = args.get(1);
      me->_accessible->insertText(static_cast<size_t>(offset), text);
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUITreeView(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UITreeView", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("rows",  [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      auto rows = me->rows();

      JSArray list(context);
      for (size_t i = 0; i < rows.size(); ++i) {
        UIElement *row = rows[i].release();
        list.addValue(context->createJsInstance(_roleToElement[row->_accessible->getRole()], { row }));
      }
      return list;
    }, nullptr);

    prototype.defineVirtualProperty("columns",  [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      auto columns = me->columns();

      JSArray list(context);
      for (size_t i = 0; i < columns.size(); ++i) {
        UIElement *column = columns[i].release();
        list.addValue(context->createJsInstance(_roleToElement[column->_accessible->getRole()], { column }));
      }
      return list;
    }, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUILabel(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UILabel", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("text", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIPane(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIPane", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &/*prototype*/) {
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIMenuBase(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIMenuBase", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
      prototype.defineFunction({ "activate" }, 2, [](JSExport *element, JSValues &) {
        auto me = validate(element);
        me->_accessible->activate();
      });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIMenu(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIMenu", "UIMenuBase", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIMenuBase", { backend });
  }, [](JSObject &/*prototype*/) {
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIMenuBar(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIMenuBar", "UIMenuBase", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIMenuBase", { backend });
  }, [](JSObject &/*prototype*/) {
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIMenuItem(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIMenuItem", "UIMenuBase", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIMenuBase", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("title", getter, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUISeparator(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UISeparator", "UIElement", 1, [](JSObject *, JSValues &args) {
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &/*prototype*/) {
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUISplitContainer(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UISplitContainer", "UIElement", 1, [](JSObject *, JSValues &args) {
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("horizontal", getter, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIGroupBox(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIGroupBox", "UIElement", 1, [](JSObject *, JSValues &args) {
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("title", getter, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIImage(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIImage", "UIElement", 1, [](JSObject *, JSValues &args) {
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineFunction({ "saveContent" }, 2, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      std::string path = args.get(0, ".");
      me->_accessible->saveImage(path);
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUITabView(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UITabView", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("activeTab", getter, setter);

    prototype.defineFunction({ "tabPages" }, 0, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      auto tabs = me->tabPages();

      JSArray list(args.context());
      for (size_t i = 0; i < tabs.size(); ++i) {
        UIElement *tab = tabs[i].release();
        list.addValue(args.context()->createJsInstance(_roleToElement[tab->_accessible->getRole()], { tab }));
      }
      args.pushResult(list);
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUITabPage(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UITabPage", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("title", getter, setter);
    prototype.defineVirtualProperty("active", getter, setter);

    prototype.defineVirtualProperty("closeButton",
      [](ScriptingContext *context, JSExport *element, std::string const&) -> JSVariant {
      UIElement *ptr = validate(element)->closeButton().release();
      if (!ptr->isValid())
        return JSVariant();
      return context->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr });
    }, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIDatePicker(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIDatePicker", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("date", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIRow(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIRow", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("expanded", getter, setter);
    prototype.defineVirtualProperty("canExpand", getter, nullptr);
    prototype.defineVirtualProperty("selected", getter, setter);

    prototype.defineVirtualProperty("entries", [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      auto entries = me->rowEntries();

      JSArray list(context);
      for (auto &entry : entries) {
        UIElement *ptr = entry.release();
        list.addValue(context->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr }));
      }

      return list;
    }, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIColumn(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIColumn", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("selected", getter, setter);

    prototype.defineVirtualProperty("entries", [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      auto entries = me->columnEntries();

      JSArray list(context);
      for (auto &entry : entries) {
        UIElement *ptr = entry.release();
        list.addValue(context->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr }));
      }

      return list;
    }, nullptr);

    prototype.defineVirtualProperty("header",
      [](ScriptingContext *context, JSExport *element, std::string const&) -> JSVariant {
      UIElement *ptr = validate(element)->header().release();
      if (!ptr->isValid())
        return JSVariant();
      return context->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr });
    }, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUICell(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UICell", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("text", getter, setter);
    prototype.defineVirtualProperty("title", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIScrollBox(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIScrollBox", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("horizontalScrollBar", [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      UIElement *ptr = me->horizontalScrollBar().release();
      return context->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr });
    }, nullptr);
    prototype.defineVirtualProperty("verticalScrollBar", [](ScriptingContext *context, JSExport *element, std::string const&) {
      auto me = validate(element);
      UIElement *ptr = me->verticalScrollBar().release();
      return context->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr });
    }, nullptr);

    prototype.defineFunction({ "scrollLeft" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->scrollLeft();
    });
    prototype.defineFunction({ "scrollRight" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->scrollRight();
    });
    prototype.defineFunction({ "scrollUp" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->scrollUp();
    });
    prototype.defineFunction({ "scrollDown" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->scrollDown();
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUISlider(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UISlider", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("value", getter, setter);
    prototype.defineVirtualProperty("maxValue", getter, nullptr);
    prototype.defineVirtualProperty("minValue", getter, nullptr);
    prototype.defineVirtualProperty("horizontal", getter, nullptr);

    prototype.defineFunction({ "increment" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->increment();
    });
    prototype.defineFunction({ "decrement" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->decrement();
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIStepper(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIStepper", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("value", getter, setter);
    prototype.defineVirtualProperty("maxValue", getter, nullptr);
    prototype.defineVirtualProperty("minValue", getter, nullptr);
    prototype.defineVirtualProperty("step", getter, nullptr);

    prototype.defineFunction({ "stepUp" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->stepUp();
    });
    prototype.defineFunction({ "stepDown" }, 0, [](JSExport *element, JSValues &) {
      auto me = validate(element);
      me->_accessible->stepDown();
    });
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIList(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIList", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("selectedIndexes", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIIconView(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIIconView", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("selectedIndexes", getter, setter);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIProgress(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIProgress", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("value", getter, setter);
    prototype.defineVirtualProperty("maxValue", getter, nullptr);
    prototype.defineVirtualProperty("minValue", getter, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIBusy(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIBusy", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    std::ignore = prototype;
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIScrollBar(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIScrollBar", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("horizontal", getter, nullptr);
    prototype.defineVirtualProperty("range", getter, nullptr);
    prototype.defineVirtualProperty("position", getter, setter);
    prototype.defineVirtualProperty("maxValue", getter, nullptr);
    prototype.defineVirtualProperty("minValue", getter, nullptr);
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::defineUIScrollThumb(ScriptingContext &context, JSObject &module) {
  std::ignore = context;
  module.defineClass("UIScrollThumb", "UIElement", 1, [](JSObject *instance, JSValues &args) {
    std::ignore = instance;
    void *backend = args.get(0);
    args.context()->callConstructor("UIElement", { backend });
  }, [](JSObject &prototype) {
    std::ignore = prototype;
  });
}

//----------------------------------------------------------------------------------------------------------------------

void UIElement::registerInContext(ScriptingContext &context, JSObject &exports) {
  // Make sure we are not missing any role in our element map.
  assert(_roleToElement.size() == static_cast<size_t>(Role::Sentinel));

  exports.defineClass("UIElement", "", 1, [](JSObject *instance, JSValues &args) {
    void *value = args.get(0);
    auto backend = reinterpret_cast<UIElement *>(value);
    instance->setBacking(backend);

    // A number of values that can't change, so we can define them as constant properties.
    instance->defineProperty("valid", backend->isValid());
    if (backend->isValid()) {
      instance->defineProperty("name", backend->_accessible->getName());
      instance->defineProperty("type", static_cast<unsigned>(backend->_accessible->getRole()));
    }
  }, [](JSObject &prototype) {
    prototype.defineVirtualProperty("description", getter, nullptr);
    prototype.defineVirtualProperty("enabled", getter, setter);
    prototype.defineVirtualProperty("focused", getter, setter);
    prototype.defineVirtualProperty("canFocus", getter, nullptr);
    prototype.defineVirtualProperty("help", getter, nullptr);

    prototype.defineFunction({ "children" }, 0, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      auto children = me->children();

      JSArray list(args.context());
      for (size_t i = 0; i < children.size(); ++i) {
        // The UIElement instance will now be managed by the JS object.
        UIElement *child = children[i].release();
        list.addValue(args.context()->createJsInstance(_roleToElement[child->_accessible->getRole()], { child }));
      }
      args.pushResult(list);
    });
    
    prototype.defineFunction({ "hasChild" } , 1, [](JSExport *element, JSValues &args) {
      // parameters: UIElement
      JSObject child = args.get(0);
      auto me = validate(element);
      args.pushResult(me->hasChild(dynamic_cast<UIElement *>(child.getBacking())));
    });
    
    prototype.defineFunction({ "childByName" } , 1, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::string name = args.get(0);
      auto me = validate(element);
      UIElementRef child = me->childByName(name);
      if (child) {
        UIElement *ptr = child.release();
        args.pushResult(args.context()->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr }));
      }
    });

    prototype.defineFunction({ "firstChildByName" } , 1, [](JSExport *element, JSValues &args) {
      // parameters: name
      std::string name = args.get(0);
      auto me = validate(element);
      UIElementRef child = me->childByName(name, false);
      if (child) {
        UIElement *ptr = child.release();
        args.pushResult(args.context()->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr }));
      }
    });

    prototype.defineFunction({ "childAtPoint" } , 1, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      geometry::Point point = args.getPoint(0);

      // Get an element from screen and check if that is one of our child elements.
      point = me->convertToScreen(point);
      UIElementRef candidate = UIElement::fromPoint(point, me->_root);
      if (candidate->isValid()) {
        if (me->hasChild(candidate.get())) {
          UIElement *ptr = candidate.release();
          args.pushResult(args.context()->createJsInstance(_roleToElement[ptr->_accessible->getRole()], { ptr }));
        }
      }
    });
    
    prototype.defineFunction({ "getParent" }, 0, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      UIElement *parent = me->getParent().release();
      args.pushResult(args.context()->createJsInstance(_roleToElement[parent->_accessible->getRole()], { parent }));
    });
    
    prototype.defineFunction({ "show" }, 0, [](JSExport *element, JSValues &args) {
      std::ignore = args;
      validate(element)->_accessible->show();
    });

    prototype.defineFunction({ "getBounds" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: flag?
      bool screenCoordinates = args.get(0, true);
      args.pushResult(validate(element)->_accessible->getBounds(screenCoordinates));
    });
    
    prototype.defineFunction({ "convertToScreen" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: Point
      geometry::Point point = args.getPoint(0);
      args.pushResult(validate(element)->convertToScreen(point));
    });
    
    prototype.defineFunction({ "convertToClient" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: Point
      geometry::Point point = args.getPoint(0);
      args.pushResult(validate(element)->convertToClient(point));
    });
    
    prototype.defineFunction({ "convertToTarget" }, 2, [](JSExport *element, JSValues &args) {
      // parameters: UIElement Point?
      JSObject target = args.get(0);
      auto me = validate(element);

      // If there is no point given use the origin of the element (which is at (0, 0)).
      geometry::Point point;
      if (args.is(ValueType::Object, 1))
        point = args.getPoint(1);
      point = me->convertToTarget(dynamic_cast<UIElement *>(target.getBacking()), point);
      args.pushResult(point);
    });
    
    prototype.defineFunction({ "containsPoint" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: Point
      geometry::Point point = args.getPoint(0);
      geometry::Rectangle bounds = validate(element)->_accessible->getBounds(false);
      bounds.position = { 0, 0 };
      args.pushResult(bounds.contains(point));
    });
    
    prototype.defineFunction({ "containsRect" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: Rectangle
      geometry::Rectangle rect = args.getRectangle(0);
      geometry::Rectangle bounds = validate(element)->_accessible->getBounds(false);
      bounds.position = { 0, 0 };
      args.pushResult(bounds.contains(rect));
    });
    
    prototype.defineFunction({ "find" }, 2, [](JSExport *element, JSValues &args) {
      // parameters: path, options?
      std::string path = args.get(0);
      JSObject options = args.get(1, JSObject());
      bool includeEmpty = true;
      bool includeInternal = false;
      if (options.isValid()) {
        includeEmpty = options.get("includeEmptyNames", true);
        includeInternal = options.get("includeInternal", false);
      }
      auto me = validate(element);

      JSArray result(args.context());
      APath searcher(me->_root);
      auto searchResult = searcher.execute(me, path, includeEmpty, includeInternal);
      for (auto &value : searchResult) {
        UIElement *ptr = value.release();
        aal::Role r = ptr->_accessible->getRole();
        result.addValue(args.context()->createJsInstance(_roleToElement[r], { ptr }));
      }
      args.pushResult(result);
    });
    
    prototype.defineFunction({ "equals" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: UIElement
      JSObject other = args.get(0);
      args.pushResult(validate(element)->equals(dynamic_cast<UIElement *>(other.getBacking())));
    });
    
    prototype.defineFunction( { "dump" }, 1, [](JSExport *element, JSValues &args) {
      // parameters: recursive
      bool recursive = args.get(0, false);
      args.pushResult(validate(element)->_accessible->dump(recursive));
    });
    
    prototype.defineFunction( { "printNativeInfo" }, 0, [](JSExport *element, JSValues &/*args*/) {
      validate(element)->_accessible->printNativeInfo();
    });

    prototype.defineFunction( { "containingRow" }, 0, [](JSExport *element, JSValues &args) {
      auto me = validate(element);
      UIElement *row = me->containingRow().release();
      args.pushResult(args.context()->createJsInstance(_roleToElement[row->_accessible->getRole()], { row }));
    });

  });

  // Define our JS class hierarchy. These are all backed by UIElement or directly via the
  // accessibility abstraction layer (AAL).
  defineUIWindow(context, exports);
  defineUIButtonBase(context, exports);
  defineUIButton(context, exports);
  defineUIRadioButton(context, exports);
  defineUICheckBox(context, exports);
  defineUIComboBox(context, exports);
  defineUIExpander(context, exports);
  defineUIGrid(context, exports);
  defineUITextBox(context, exports);
  defineUITreeView(context, exports);
  defineUILabel(context, exports);
  defineUIPane(context, exports);

  defineUIMenuBase(context, exports);
  defineUIMenu(context, exports);
  defineUIMenuBar(context, exports);
  defineUIMenuItem(context, exports);
  defineUISeparator(context, exports);

  defineUISplitContainer(context, exports);
  defineUIGroupBox(context, exports);
  defineUIImage(context, exports);
  defineUITabView(context, exports);
  defineUITabPage(context, exports);
  defineUIDatePicker(context, exports);
  defineUIRow(context, exports);
  defineUIColumn(context, exports);
  defineUICell(context, exports);
  defineUIScrollBox(context, exports);

  defineUISlider(context, exports);
  defineUIStepper(context, exports);
  defineUIList(context, exports);
  defineUIIconView(context, exports);
  defineUIProgress(context, exports);
  defineUIBusy(context, exports);

  defineUIScrollBar(context, exports);
  defineUIScrollThumb(context, exports);
}

//----------------------------------------------------------------------------------------------------------------------
