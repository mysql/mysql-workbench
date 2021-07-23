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

#include "common.h"
#include "aalcommon.h"

#include "role.h"
#include "geometry.h"
#include "textrange.h"

namespace aal {

// Human readable values of a single accessible property.
struct AccessibleProperty {
  std::string name;
  std::string value;
  bool readOnly;
  bool containsReference;
};

// Ditto for actions.
struct AccessibleAction {
  std::string name;
  std::string description;
};

// A struct containing human readable names and values of roles + properties for an Accessible instance.
struct AccessibleDetails {
  std::string role;
  std::string subRole;

  std::vector<AccessibleProperty> properties;
  std::vector<AccessibleAction> actions;
};

class Accessible {
public:
  Accessible() = delete;
  Accessible(AXUIElementRef accessible);
  virtual ~Accessible();

  static bool accessibilitySetup();
  static AccessibleRef getByPid(const int pid);

  AccessibleRef clone() const;

  bool isRoot() const;
  bool isValid() const;
  size_t getHash() const;

  bool canFocus() const;
  bool isFocused() const;
  void setFocused();

  bool isEnabled() const;
  void setEnabled(bool value); 

  bool isEditable() const;
  bool isReadOnly() const;
  bool isSecure() const;
  bool isHorizontal() const;

  CheckState getCheckState() const;
  void setCheckState(CheckState state);

  bool isExpandable() const;
  bool isExpanded();
  void setExpanded(bool value);

  double getValue() const;
  double getMaxValue() const;
  double getMinValue() const;
  void setValue(double value);
  double getRange() const;

  std::string getActiveTabPage() const;
  void setActiveTabPage(std::string const& name);
  void activate();
  bool isActiveTab() const;

  bool isSelected() const;
  void setSelected(bool value);

  double getScrollPosition() const;
  void setScrollPosition(double value);

  void showMenu() const;
  bool menuShown() const;

  bool equals(Accessible *other) const;

  AccessibleRef getParent() const;
  AccessibleRef getContainingRow() const;
  AccessibleRef getHorizontalScrollBar() const;
  AccessibleRef getVerticalScrollBar() const;
  AccessibleRef getHeader() const;
  AccessibleRef getCloseButton() const;

  void children(AccessibleList &result, bool recursive) const;
  AccessibleList children() const;
  AccessibleList windows() const;
  AccessibleList tabPages() const;
  AccessibleList rows() const;
  AccessibleList rowEntries() const;
  AccessibleList columns() const;
  AccessibleList columnEntries() const;

  static AccessibleRef fromPoint(geometry::Point point, Accessible *application);

  std::string getID() const;
  std::string getName() const;
  std::string getHelp() const;
  aal::Role getRole() const { return _role; };
  bool isInternal() const;

  geometry::Rectangle getBounds(bool screenCoordinates) const;
  void setBounds(geometry::Rectangle const& bounds);

  size_t getCaretPosition() const;
  void setCaretPosition(size_t position);

  size_t getCharacterCount() const;

  std::set<size_t> getSelectedIndexes() const;
  void setSelectedIndexes(std::set<size_t> const& indexes);

  std::string getText() const; // For text *content* only (edits, labels etc.).
  void setText(std::string const& text);

  std::string getTitle() const; // For all captions (buttons, menu items, windows etc.).
  void setTitle(std::string const& text);

  void insertText(const std::size_t offset, const std::string &text);
  std::string getDescription() const;

  std::string getSelectedText() const;
  void setSelectedText(std::string const& text);

  aal::TextRange getSelectionRange() const;
  void setSelectionRange(TextRange range);

  std::string getDate() const;
  void setDate(std::string const& date);

  void mouseDown(geometry::Point pos, MouseButton button = MouseButton::Left);
  void mouseUp(geometry::Point pos, MouseButton button = MouseButton::Left);
  void mouseMove(geometry::Point pos) const;
  void mouseMoveTo(geometry::Point pos) const;
  void mouseDrag(geometry::Point source, geometry::Point target, MouseButton button = MouseButton::Left);
  geometry::Point getMousePosition() const;

  void keyDown(aal::Key k, aal::Modifier modifier) const;
  void keyUp(aal::Key k, aal::Modifier modifier) const;
  void keyPress(aal::Key k, aal::Modifier modifier) const;
  void typeString(std::string const& input) const;

  void click();
  void confirm(bool checkError = true);
  void stepUp();
  void stepDown();
  void scrollLeft();
  void scrollRight();
  void scrollUp();
  void scrollDown();
  void increment();
  void decrement();

  void show();
  void bringToFront();
  void highlight(NSColor *color = NSColor.systemPinkColor) const;
  void removeHighlight() const;
  bool isHighlightActive() const;

  std::string getPlatformRoleName() const;
  std::string dump(bool recursive = false, std::string const& indentation = "") const;
  void printNativeInfo() const;
  AccessibleDetails getDetails() const;

  void takeScreenShot(std::string const& path, bool onlyWindow, geometry::Rectangle rect) const;
  
  static void handleUnsupportedError(AXError error, std::string const& attribute);
  
private:
  AXUIElementRef _native;
  Role _role = Role::Unknown;

  static Role determineRole(AXUIElementRef element);
  
  static NSArray* getChildren(AXUIElementRef ref, size_t count = 99999, bool visibleOnly = false);
  static NSArray* getArrayValue(AXUIElementRef ref, CFStringRef attribute, size_t count = 99999);
  static std::string getStringValue(AXUIElementRef ref, CFStringRef attribute, std::string const& attributeName,
                             bool noThrow = false);
  static void setStringValue(AXUIElementRef ref, CFStringRef attribute, std::string const& value,
                      std::string const& attributeName);
  static bool getBoolValue(AXUIElementRef ref, CFStringRef attribute, std::string const& attributeName,
                    bool noThrow = false);
  static void setBoolValue(AXUIElementRef ref, CFStringRef attribute, bool value, std::string const& attributeName);
  static NSNumber* getNumberValue(AXUIElementRef ref, CFStringRef attribute, std::string const& attributeName,
                           bool noThrow = false);
  static void setNumberValue(AXUIElementRef ref, CFStringRef attribute, NSNumber* value,
                      std::string const& attributeName);
  static AXUIElementRef getElementValue(AXUIElementRef ref, bool noThrow = false);
  static bool hasBounds(AXUIElementRef ref);
  static geometry::Rectangle getBounds(AXUIElementRef ref, bool screenCoordinates);
  static void setBounds(AXUIElementRef ref, geometry::Rectangle const& bounds);

  static bool isSupported(AXUIElementRef ref, CFStringRef attribute);
  static bool isSettable(AXUIElementRef ref, CFStringRef attribute);

  static std::string valueDescription(AXValueRef value);

  void writeImageToFile(CGImageRef image, NSString *path) const;

  static AXUIElementRef getFirstChild(AXUIElementRef parent);
  static AXUIElementRef getParent(AXUIElementRef child);

  static void press(AXUIElementRef element);

  static void printInfo(AXUIElementRef element);
};

}
