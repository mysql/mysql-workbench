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

#include <cairomm/cairomm.h>
#include <set>
#include "role.h"
#include <atspi/atspi.h>
#include "geometry.h"
#include "textrange.h"

namespace aal {

class Accessible {
protected:
  AtspiAccessible *_accessible;

  bool _isRoot;
  int _pid;
  AtspiRole _role;
//   std::vector<std::string> _availableInterfaces;
//   std::vector<std::string> _availableActions;

protected:
  std::string getPlatformRoleName() const;

  bool getState(AtspiStateType state, AtspiAccessible *acc = nullptr) const;
  void expandCollapse(bool expand = false);
  bool isVisible(AtspiAccessible *acc = nullptr) const;
  double getIncrementValue();
  bool implementsValue();
  int getIndex() const ;
  
  void gatherAvailableActions() const;
  int getActionIndex(const std::string &action) const;
  void triggerAction(const std::string &action) const;
  
  void gatherAvailableInterfaces() const;
  bool interfaceAvailable(const std::string &interaface) const;
  AtspiAction *getInterfaceAction(AtspiAccessible *acc = nullptr) const;
  AtspiText *getInterfaceText(AtspiAccessible *acc = nullptr) const;
  AtspiEditableText *getInterfaceEditableText(AtspiAccessible *acc = nullptr) const;
  AtspiTable *getInterfaceTable(AtspiAccessible *acc = nullptr) const;
  AtspiSelection *getInterfaceSelection(AtspiAccessible *acc = nullptr) const;
  
public:
  Accessible();
  Accessible(AtspiAccessible *accessible, bool isRoot);
  Accessible(AtspiAccessible *accessible, AtspiRole forceRole);
  virtual ~Accessible();

  static AtspiRole getPlatformRole(AtspiAccessible *acc = nullptr);
//   static void handleAtspiError(GError *err, const std::string &attr);

  AccessibleRef clone() const;
  
  bool isRoot() const { return _isRoot; };
  bool isValid() const { return _accessible != nullptr; }
  bool equals(Accessible *other) const;
  bool menuShown() const;
  void showMenu();
  
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

  bool canFocus() const;
  bool isFocused() const;
  void setFocused();

  std::string getID() const;
  std::string getName() const;
  std::string getHelp() const;
  virtual aal::Role getRole() const;

  bool isInternal() const;
  bool isEnabled() const;
  
  bool isEditable() const;
  bool isReadOnly() const;
  bool isSecure() const;
  bool isHorizontal() const;
  
  CheckState getCheckState() const;
  void setCheckState(CheckState state);
  
  double getValue() const;
  double getMaxValue() const;
  double getMinValue() const;
  void setValue(const double value);
  double getRange() const;

  std::string getActiveTabPage() const;
  void setActiveTabPage(std::string const& name);
  void activate();
  bool isActiveTab() const;

  bool isSelected() const;
  void setSelected(bool value);

  double getScrollPosition() const;
  void setScrollPosition(double value);

  std::string getTitle() const ;
  void setTitle(const std::string &title);

  std::string getDescription() const;
  std::string getText() const;
  
  std::size_t getCaretPosition() const;
  void setCaretPosition(size_t position);
  
  void insertText(size_t offset, const std::string &text);
  std::string getText(size_t offset, size_t len);
  void setText(const std::string &text);

  std::string getSelectedText() const;
  void setSelectedText(std::string const& text);

  TextRange getSelectionRange() const;
  void setSelectionRange(TextRange range);
  
  std::string getDate() const;
  void setDate(std::string const& date);
  
  size_t getCharacterCount() const;
  
  std::set<size_t> getSelectedIndexes() const;
  void setSelectedIndexes(std::set<size_t> const& indexes);

  void mouseDown(const geometry::Point &pos, const MouseButton button = MouseButton::Left);
  void mouseUp(const geometry::Point &pos, const MouseButton button = MouseButton::Left);
  void mouseMove(const geometry::Point &pos);
  void mouseMoveTo(const geometry::Point &pos);
  void mouseDrag(geometry::Point source, geometry::Point target, MouseButton button = MouseButton::Left);
  geometry::Point getMousePosition() const;

  void keyDown(const aal::Key k, aal::Modifier modifier);
  void keyUp(const aal::Key k, aal::Modifier modifier);
  void keyPress(const aal::Key k, aal::Modifier modifier);
  void typeString(std::string const& input) const;

  void click();
  void confirm();
  void stepUp();
  void stepDown();
  void scrollLeft();
  void scrollRight();
  void scrollUp();
  void scrollDown();
  void increment();
  void decrement();
  
  static geometry::Rectangle getBounds(AtspiAccessible *acc, bool screenCoordinates);
  geometry::Rectangle getBounds(bool screenCoordinates) const;
  void setBounds(geometry::Rectangle const& bounds);

  void takeScreenShot(const std::string &path, bool onlyWindow, geometry::Rectangle rect) const;
  void saveImage(std::string const& path) const;

  void show();
  void bringToFront();
  void highlight() const;
  void removeHighlight() const;

  int getPid();

  bool isExpandable() const;
  bool isExpanded() const;
  void setExpanded(bool value);
  void setEnabled(bool value);

  std::string dump(bool recursive = false, std::string const& indentation = "") const;
  void printNativeInfo() const;

  static AccessibleRef getByPid(int pid);

  static bool accessibilitySetup();
};

}
