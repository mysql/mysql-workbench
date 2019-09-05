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

#pragma once 

#include "textrange.h"
#include "geometry.h"
#include "role.h"

namespace aal {

  class AccessibleWr {
  public:
    typedef std::unique_ptr<AccessibleWr> Ref;
    typedef std::vector<Ref> RefList;

    static bool accessibilitySetup();
    static Ref getByPid(const int pid);
    static int getRunningProcess(std::wstring const& fileName);
    static std::vector<int> getRunningProcessByName(std::wstring const& name);
    static std::string NativeToCppString(System::String^ str);

    AccessibleWr(AccessibleNet ^accessible);
    AccessibleWr() = delete;
    virtual ~AccessibleWr();

    Ref clone();

    bool isRoot();
    bool isValid();
    bool equals(AccessibleWr *other);

    bool canFocus() const;
    bool isFocused() const;
    void setFocused();

    CheckState getCheckState() const;
    void setCheckState(CheckState state);

    double getValue() const;
    double getMaxValue() const;
    double getMinValue() const;
    void setValue(const double value);
    double getRange() const;

    void activate();
    void setActiveTabPage(std::string const& title);
    std::string getActiveTabPage() const;
    bool isActiveTabPage() const;
    RefList tabPages() const;

    Ref getParent();
    RefList children();
    Ref fromPoint(geometry::Point point);

    Ref getContainingRow() const;
    RefList rows() const;
    RefList rowEntries() const;
    RefList columns() const;
    Ref getHeader() const;
    RefList columnEntries() const;
    //RefList menuItems() const;
    bool isSelected() const;
    void setSelected(bool value);

    Ref getHorizontalScrollBar() const;
    Ref getVerticalScrollBar() const;
    double getScrollPosition() const;
    void setScrollPosition(double value);

    bool menuShown() const;
    void showMenu() const;

    std::string getID() const;
    std::string getName() const;
    std::string getHelp() const;
    aal::Role getRole() const;
    bool isInternal() const;
    bool isEnabled() const;
    bool isEditable() const;
    bool isReadOnly() const;
    bool isSecure() const;
    bool isHorizontal() const;

    size_t getCaretPosition() const;
    void setCaretPosition(size_t position);
    geometry::Rectangle getBounds(bool screenCoordinates);
    void setBounds(geometry::Rectangle const& bounds);

    std::string getText() const;
    std::string getTitle() const;
    void setText(std::string const& text);
    void setText(size_t offset, std::string const& text);
    void setTitle(std::string const& text);
    std::string getDescription() const;
    std::string getSelectionText() const;
    void setSelectionText(const std::string& text);

    TextRange getSelectionRange();
    void setSelectionRange(TextRange range);
    size_t getCharacterCount() const;

    std::set<size_t> getSelectedIndexes() const;
    void setSelectedIndexes(std::set<size_t> const& indexes);

    void mouseDown(const geometry::Point &pos, const MouseButton button = MouseButton::Left);
    void mouseUp(const geometry::Point &pos, const MouseButton button = MouseButton::Left);
    void mouseMove(const geometry::Point &pos);
    void mouseMoveTo(const geometry::Point &pos);
    geometry::Point getMousePosition() const;

    void keyDown(const aal::Key k, aal::Modifier modifier) const;
    void keyUp(const aal::Key k, aal::Modifier modifier) const;
    void keyPress(const aal::Key k, aal::Modifier modifier) const;
    void typeString(std::string const& input) const;

    void click();
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

    bool isExpandable();
    bool isExpanded();
    void setExpanded(bool value);

    std::string getPlatformRoleName() const;
    bool takeScreenShot(std::string const& path, bool onlyWindow, geometry::Rectangle rect) const;

    void printNativeInfo() const;

    static std::string getClipboardText();
    static void setClipboardText(const std::string &content);

  private:
    gcroot<AccessibleNet^> _managedObject;

    AccessibleNet::MouseButton mapButtonEnum(const MouseButton button);
    System::Windows::Automation::ToggleState mapCheckStateEnum(const CheckState state) const;
    CheckState AccessibleWr::mapCheckStateEnum(System::Windows::Automation::ToggleState state) const;
  };
 
}
