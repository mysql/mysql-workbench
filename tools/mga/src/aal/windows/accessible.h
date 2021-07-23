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

#include "geometry.h"
#include "textrange.h"
#include "role.h"

namespace aal {

  class AccessibleWr;
  class ACCESSIBILITY_PUBLIC Accessible {
  public:
    static bool accessibilitySetup();
    static AccessibleRef getByPid(const int pid);
    static int getRunningProcess(std::wstring const& fileName);
    static std::vector<int> getRunningProcessByName(std::wstring const& name);

    Accessible() = delete;
    Accessible(std::unique_ptr<AccessibleWr> accessible);
    virtual ~Accessible();

    AccessibleRef clone() const;

    bool isRoot() const;
    bool isValid() const;
    bool equals(Accessible *other) const;

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
    aal::Role getRole() const;
    bool isInternal() const;
    
    size_t getCaretPosition() const;
    void setCaretPosition(size_t position);
    
    geometry::Rectangle getBounds(bool screenCoordinates) const;
    void setBounds(geometry::Rectangle const& bounds);

    void insertText(size_t offset, const std::string &text);
    std::string getText() const;
    std::string getTitle() const;
    void setText(std::string const& text);
    void setTitle(std::string const& text);
    std::string getDescription() const;

    bool menuShown() const;
    void showMenu() const;
    
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

    void keyDown(const aal::Key k, aal::Modifier modifier) const;
    void keyUp(const aal::Key k, aal::Modifier modifier) const;
    void keyPress(const aal::Key k, aal::Modifier modifier) const;
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
    
    void show();
    void bringToFront();
    void highlight() const;
    void removeHighlight() const;

    bool isExpandable() const;
    bool isExpanded() const;
    void setExpanded(bool value);

    std::string getPlatformRoleName() const;
    std::string dump(bool recursive = false, std::string const& indentation = "") const;
    void printNativeInfo() const;
    
    bool takeScreenShot(std::string const& path, bool onlyWindow, geometry::Rectangle rect) const;

    static std::string getClipboardText();
    static void setClipboardText(const std::string &content);

  private:
    AccessibleRef fromPoint(geometry::Point point);
    std::unique_ptr<AccessibleWr> _accessible;
  };
}
