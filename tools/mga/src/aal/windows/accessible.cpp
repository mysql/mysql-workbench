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

#include "accessible.h"
#include "accessible.wr.h"

using namespace aal;

//----------------------------------------------------------------------------------------------------------------------

Accessible::Accessible(std::unique_ptr<AccessibleWr> accessible)
  : _accessible(std::move(accessible)) { 
}

//----------------------------------------------------------------------------------------------------------------------

Accessible::~Accessible() {
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::clone() const {
  AccessibleRef ref = std::make_unique<aal::Accessible>(_accessible->clone());
  return ref;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::canFocus() const {
  return _accessible->canFocus();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isFocused() const {
  return _accessible->isFocused();	
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setFocused() {
	_accessible->setFocused();
}

//----------------------------------------------------------------------------------------------------------------------

CheckState Accessible::getCheckState() const {
  return _accessible->getCheckState();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setCheckState(CheckState state) {
  _accessible->setCheckState(state);
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getValue() const {
  return _accessible->getValue();}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getMaxValue() const {
  return _accessible->getMaxValue();
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getMinValue() const {
  return _accessible->getMinValue();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setValue(const double value) {
  return _accessible->setValue(value);
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getRange() const {
  if (getRole() != Role::ScrollBar)
    throw std::runtime_error("The range attribute is only supported for scrollbars.");
  return _accessible->getRange();
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getActiveTabPage() const {
  return _accessible->getActiveTabPage();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setActiveTabPage(std::string const& name) {
  _accessible->setActiveTabPage(name);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::activate() {
  if (getRole() != Role::TabPage && getRole() != Role::MenuItem)
    throw std::runtime_error("Cannot activate this element.");
  _accessible->activate();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isActiveTab() const {
  return _accessible->isActiveTabPage();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSelected() const {
  if (getRole() != Role::Row && getRole() != Role::Column)
    throw std::runtime_error("This element cannot be selected.");
  return _accessible->isSelected();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelected(bool value) {
  if (getRole() != Role::Row && getRole() != Role::Column)
    throw std::runtime_error("This element cannot be selected.");
  return _accessible->setSelected(value);
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getScrollPosition() const {
  if (getRole() != Role::ScrollBar)
    throw std::runtime_error("The scroll position is only supported by scrollbars.");
  return _accessible->getScrollPosition();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setScrollPosition(double value) {
  if (getRole() != Role::ScrollBar)
    throw std::runtime_error("The scroll position is only supported by scrollbars.");
  _accessible->setScrollPosition(value);;
}

//----------------------------------------------------------------------------------------------------------------------

std::string aal::Accessible::getID() const
{
  return _accessible->getID();
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getName() const {
  return _accessible->getName();
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getHelp() const {
  return _accessible->getHelp();
}

//----------------------------------------------------------------------------------------------------------------------

Role Accessible::getRole() const {
  return _accessible->getRole();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isInternal() const {
  return _accessible->isInternal();
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getParent() const {
  AccessibleRef ref = std::make_unique<aal::Accessible>(_accessible->getParent()); 
  return ref; 
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getContainingRow() const {
  if (getRole() == Role::Row)
    return clone();
  AccessibleWr::Ref acc = _accessible->getContainingRow();
  AccessibleRef ref = std::make_unique<aal::Accessible>(std::move(acc));
  return ref;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getHorizontalScrollBar() const {
  aal::AccessibleWr::Ref acc = _accessible->getHorizontalScrollBar();
  AccessibleRef ref = std::make_unique<aal::Accessible>(std::move(acc));
  return ref;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getVerticalScrollBar() const {
  aal::AccessibleWr::Ref acc = _accessible->getVerticalScrollBar();
  AccessibleRef ref = std::make_unique<aal::Accessible>(std::move(acc));
  return ref;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isEnabled() const {
  return _accessible->isEnabled();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setEnabled(bool value) {
  std::ignore = value;
  throw std::runtime_error("Attribute is read-only");
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isEditable() const {
  if (getRole() != Role::ComboBox)
    throw std::runtime_error("The editable type is only supported for combobboxes.");
  return _accessible->isEditable();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isReadOnly() const {
  return _accessible->isReadOnly();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSecure() const {
  if (getRole() != Role::TextBox)
    throw std::runtime_error("The secure mode is only supported for text boxes.");
  return _accessible->isSecure();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isHorizontal() const {
  return _accessible->isHorizontal();
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Rectangle Accessible::getBounds(bool screenCoordinates) const {
  return _accessible->getBounds(screenCoordinates);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setBounds(geometry::Rectangle const& bounds) {
  _accessible->setBounds(bounds);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::show() { 
  _accessible->show(); 
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::bringToFront() {
  _accessible->bringToFront();
}


//----------------------------------------------------------------------------------------------------------------------

void Accessible::highlight() const {
  _accessible->highlight();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::removeHighlight() const {
  _accessible->removeHighlight();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::insertText(size_t offset, const std::string &text) {
  _accessible->setText(offset, text);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getText() const {
  return _accessible->getText();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setText(std::string const& text) {
  _accessible->setText(text);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setTitle(std::string const& text) {
  _accessible->setTitle(text);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getTitle() const {
  return getText();
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getDescription() const {
  return _accessible->getDescription();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::menuShown() const {
  return _accessible->menuShown();;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::showMenu() const {
  _accessible->showMenu();;
}


//----------------------------------------------------------------------------------------------------------------------

size_t Accessible::getCaretPosition() const {
  return _accessible->getCaretPosition();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setCaretPosition(size_t position) {
  _accessible->setCaretPosition(position);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getSelectedText() const {
  return _accessible->getSelectionText();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectedText(std::string const& text) {
  _accessible->setSelectionText(text);  
}

//----------------------------------------------------------------------------------------------------------------------

TextRange Accessible::getSelectionRange() const {
  return _accessible->getSelectionRange();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectionRange(TextRange range) {
  _accessible->setSelectionRange(range);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getDate() const {
  if (getRole() != Role::DatePicker)
    throw std::runtime_error("This element does not support date values.");
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setDate(std::string const& date) {
  if (getRole() != Role::DatePicker)
    throw std::runtime_error("This element does not support date values.");
  std::ignore = date;
}

//----------------------------------------------------------------------------------------------------------------------

size_t Accessible::getCharacterCount() const {
  return _accessible->getCharacterCount();
}

//----------------------------------------------------------------------------------------------------------------------

std::set<size_t> Accessible::getSelectedIndexes() const {
  if (getRole() != Role::ComboBox && getRole() != Role::List)
    throw std::runtime_error("This element does not support selected indexes.");
  return _accessible->getSelectedIndexes();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectedIndexes(std::set<size_t> const& indexes) {
  if (getRole() != Role::ComboBox && getRole() != Role::List)
    throw std::runtime_error("This element does not support selected indexes.");
  _accessible->setSelectedIndexes(indexes);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseDown(const geometry::Point &pos, const MouseButton button) {
  _accessible->mouseDown(pos, button);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseUp(const geometry::Point &pos, const MouseButton button) {
  _accessible->mouseUp(pos, button);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseMove(const geometry::Point &pos) {
  _accessible->mouseMove(pos);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseMoveTo(const geometry::Point &pos) {
  _accessible->mouseMoveTo(pos);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseDrag(geometry::Point source, geometry::Point target, MouseButton button) {
  std::ignore = source;
  std::ignore = target;
  std::ignore = button;

  NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------------------------------------------------

geometry::Point aal::Accessible::getMousePosition() const {
  return _accessible->getMousePosition();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyDown(const aal::Key k, aal::Modifier modifier) const {
  _accessible->keyDown(k, modifier);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyUp(const aal::Key k, aal::Modifier modifier) const {
  _accessible->keyUp(k, modifier);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyPress(const aal::Key k, aal::Modifier modifier) const {
  _accessible->keyPress(k, modifier);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::typeString(std::string const& input) const {
  _accessible->typeString(input);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::click() {
  _accessible->click();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::confirm() {
  // Ignored.
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::stepUp() {
  if (getRole() != Role::Stepper)
    throw std::runtime_error("Only stepper elements support this action.");
  _accessible->stepUp();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::stepDown() {
  if (getRole() != Role::Stepper)
    throw std::runtime_error("Only stepper elements support this action.");
  _accessible->stepDown();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollLeft() {
  if (getRole() != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");
  _accessible->scrollLeft();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollRight() {
  if (getRole() != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action."); 
  _accessible->scrollRight();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollUp() {
  if (getRole() != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");
  _accessible->scrollUp();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollDown() {
  if (getRole() != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");
  _accessible->scrollDown();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::increment() {
  if (getRole() != Role::Slider)
    throw std::runtime_error("Only slider elements support this action.");
  _accessible->increment();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::decrement() {
  if (getRole() != Role::Slider)
    throw std::runtime_error("Only slider elements support this action.");
  _accessible->decrement();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isExpandable() const {
  return _accessible->isExpandable();
}

//----------------------------------------------------------------------------------------------------------------------

bool aal::Accessible::isExpanded() const {
  return _accessible->isExpanded();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setExpanded(bool value) {
  _accessible->setExpanded(value);
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::windows() const {
  AccessibleList result;
  if (isRoot()) {
    // Root is a window as well (the application main window), with other windows (e.g. dialogs) as children.
    AccessibleWr::Ref clone = _accessible->clone();
    result.emplace_back(new Accessible(std::move(clone)));

    auto childList = children();
    for (auto &child : childList) {
      if (child->getRole() == Role::Window)
        result.push_back(std::move(child));
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::tabPages() const {
  AccessibleList result;
  if (!isValid())
    return result;

  for (auto &entry : _accessible->tabPages())
    result.emplace_back(new Accessible(std::move(entry)));
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::rows() const {
  if (getRole() != Role::TreeView && getRole() != Role::Grid)
    throw std::runtime_error("This element has no columns.");

  AccessibleList result;
  if (!isValid())
    return result;

  for (auto &entry : _accessible->rows())
    result.emplace_back(new Accessible(std::move(entry)));
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::rowEntries() const {
  if (getRole() != Role::Row)
    throw std::runtime_error("This element has no row entries.");
  AccessibleList result;
  if (!isValid())
    return result;

  for (auto &entry : _accessible->rowEntries())
    result.emplace_back(new Accessible(std::move(entry)));
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getHeader() const {
  if (getRole() != Role::Column)
    throw std::runtime_error("This element does not have a heading.");
  aal::AccessibleWr::Ref acc = _accessible->getHeader();
  AccessibleRef ref = std::make_unique<aal::Accessible>(std::move(acc));
  return ref;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getCloseButton() const {
  NOT_IMPLEMENTED;
  return AccessibleRef();
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::columns() const {
  if (getRole() != Role::TreeView && getRole() != Role::Grid)
    throw std::runtime_error("This element has no columns.");

  AccessibleList result;
  if (!isValid())
    return result;

  for (auto &entry : _accessible->columns())
    result.emplace_back(new Accessible(std::move(entry)));
  return result;;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::columnEntries() const {
  if (getRole() != Role::Column)
    throw std::runtime_error("This element has no column entries.");
  AccessibleList result;
  if (!isValid())
    return result;

  for (auto &entry : _accessible->columnEntries())
    result.emplace_back(new Accessible(std::move(entry)));
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getByPid(int pid) {
  aal::AccessibleWr::Ref acc = aal::AccessibleWr::getByPid(pid);
  AccessibleRef ref = std::make_unique<aal::Accessible>(std::move(acc));
  return ref;
}

//----------------------------------------------------------------------------------------------------------------------

int Accessible::getRunningProcess(std::wstring const& fileName) {
  return AccessibleWr::getRunningProcess(fileName);
}


//----------------------------------------------------------------------------------------------------------------------

std::vector<int> Accessible::getRunningProcessByName(std::wstring const& name) {
  return AccessibleWr::getRunningProcessByName(name);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::accessibilitySetup() {
  return true;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::children(AccessibleList &result, bool recursive) const {
  if (!isValid())
    return;

  for (auto &entry : _accessible->children()) {
    Accessible *childAcc = new Accessible(std::move(entry));
    result.emplace_back(childAcc);

    if (recursive)
      childAcc->children(result, recursive);
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::children() const {
  AccessibleList result;
  if (!isValid())
    return result;

  for (auto &entry : _accessible->children())
    result.emplace_back(new Accessible(std::move(entry))); 
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isRoot() const {
  return _accessible->isRoot();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isValid() const {
  return _accessible && _accessible->isValid();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::equals(Accessible *other) const {
  return _accessible->equals(other->_accessible.get());
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::fromPoint(geometry::Point point) {
  return std::make_unique<aal::Accessible>(std::move(_accessible->fromPoint(point)));
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::fromPoint(geometry::Point point, Accessible *application) {
  return application->fromPoint(point);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getPlatformRoleName() const {
  return _accessible->getPlatformRoleName();  
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::printNativeInfo() const {
  _accessible->printNativeInfo();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::takeScreenShot(std::string const& path, bool onlyWindow, geometry::Rectangle rect) const {
  return _accessible->takeScreenShot(path, onlyWindow, rect);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getClipboardText()
{
  return AccessibleWr::getClipboardText();
}

//----------------------------------------------------------------------------------------------------------------------


void Accessible::setClipboardText(const std::string& content)
{
  AccessibleWr::setClipboardText(content);
}

//----------------------------------------------------------------------------------------------------------------------
