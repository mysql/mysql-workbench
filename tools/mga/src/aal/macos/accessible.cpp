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

#include "utilities.h"

#include "accessible.h"

#import "Carbon/Carbon.h" // For keyboard constants.

using namespace aal;
using namespace geometry;

extern "C" AXError _AXUIElementGetWindow(AXUIElementRef, CGWindowID *out);

// Used for conversion of CFStringRef instances to std::string.
#define toString(ref) [(__bridge NSString *)ref UTF8String]

//----------------------------------------------------------------------------------------------------------------------

static std::string getNativeRole(AXUIElementRef ref, CFStringRef type = kAXRoleAttribute) {
  CFTypeRef result;
  AXError error = AXUIElementCopyAttributeValue(ref, type, &result);

  // Roles are supported for all elements. So, if there is an error it must be something else.
  if (error == kAXErrorCannotComplete || result == nullptr)
    return "";

  std::string roleString = toString(result);
  CFRelease(result);

  return roleString;
}

//----------------------------------------------------------------------------------------------------------------------

static bool nativeRoleIsOneOf(AXUIElementRef ref, std::vector<CFStringRef> roles, CFStringRef type = kAXRoleAttribute) {
  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(ref, type, &value);
  if (error != kAXErrorSuccess)
    return false;

  bool result = false;
  for (auto &role : roles) {
    if (CFEqual(value, role)) {
      result = true;
      break;
    }
  }
  CFRelease(value);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

Accessible::Accessible(AXUIElementRef accessible) : _native(accessible), _role(Role::Unknown) {
  if (_native != nullptr) {
    CFRetain(_native);
    _role = determineRole(_native);
  }
}

//----------------------------------------------------------------------------------------------------------------------

Accessible::~Accessible() {
  if (_native != nullptr)
    CFRelease(_native);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::accessibilitySetup() {
  NSDictionary *options = @{ (__bridge NSString *)kAXTrustedCheckOptionPrompt: @YES };
  BOOL accessibilityEnabled = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options) == TRUE;
  return accessibilityEnabled;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getByPid(const int pid) {
  AXUIElementRef element = AXUIElementCreateApplication(pid);
  auto result = std::unique_ptr<Accessible>(new Accessible(element));
  CFRelease(element);

  // Try to get the role of the app (which is always valid, unless we cannot connect).
  if (!result)
    return nullptr;

  if (getNativeRole(result->_native).empty())
    return nullptr;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::clone() const {
  return AccessibleRef(new Accessible(_native));
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isRoot() const {
  AXUIElementRef parent = nullptr;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXParentAttribute, (CFTypeRef *)&parent);
  bool result = (error != kAXErrorSuccess) || parent == nullptr;
  if (parent != nullptr)
    CFRelease(parent);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isValid() const {
  if (_native == nullptr)
    return false;

  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXRoleAttribute, &value);
  if (error == kAXErrorSuccess) {
    CFRelease(value);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

size_t Accessible::getHash() const {
  std::hash<std::string> stringHash;
  std::hash<size_t> numberHash;
  size_t result = 17;

  result = result * 31 + stringHash(getName());
  result = result * 31 + numberHash(static_cast<size_t>(getRole()));
  result = result * 31 + stringHash(getID());

  auto parent = getParent();
  if (parent)
    result = result * 31 + parent->getHash();

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::canFocus() const {
  // Don't check only for settable values. Sometimes focus cannot be set when it is already.
  return isSettable(_native, kAXFocusedAttribute) || isFocused();
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isFocused() const {
  return getBoolValue(_native, kAXFocusedAttribute, "focused", true);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setFocused() {
  // Cannot set focus twice, so check if it is set already.
  if (canFocus() && !isFocused())
    setBoolValue(_native, kAXFocusedAttribute, true, "focused");
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isEnabled() const {
  return getBoolValue(_native, kAXEnabledAttribute, "enabled");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setEnabled(bool value) {
  return setBoolValue(_native, kAXEnabledAttribute, value, "enabled");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * This value is not the opposite of isReadOnly(). Being read-only means you cannot change the element's value at all.
 * Being not-editable means you cannot change the text (caption) of the element via typing or direct assignment, but
 * instead only via it's associated drop down.
 * It's a value only related to combobox types (NSComboBox + NSPopupButton here).
 */
bool Accessible::isEditable() const {
  return nativeRoleIsOneOf(_native, { kAXComboBoxRole });
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isReadOnly() const {
  return !isSettable(_native, kAXValueAttribute);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSecure() const {
  if (_role != Role::TextBox)
    throw std::runtime_error("The secure mode is only supported for text boxes.");

  CFTypeRef value;
  AXUIElementCopyAttributeValue(_native, kAXSubroleAttribute, &value);
  if (value == nullptr)
    return false;

  bool result = CFEqual(value, kAXSecureTextFieldSubrole);
  CFRelease(value);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isHorizontal() const {
  switch (_role) {
    case Role::ScrollBar:
    case Role::Slider: {
      bool result = false;
      CFTypeRef orientation;
      AXUIElementCopyAttributeValue(_native, kAXOrientationAttribute, &orientation);
      if (orientation != nullptr) {
        result = CFEqual(orientation, kAXHorizontalOrientationValue);
        CFRelease(orientation);
      }
      return result;
    }

    case Role::SplitContainer: {
      // Determining the splitter (group) orientation requires to get the first native splitter child element
      // and check its orientation. This is also orthogonal to the splitter group's orientation.
      CFArrayRef splitters;
      AXUIElementCopyAttributeValues(_native, kAXSplittersAttribute, 0, 1, &splitters);
      if (splitters == nullptr)
        return false;

      bool result = false;
      AXUIElementRef splitter = static_cast<AXUIElementRef>(CFArrayGetValueAtIndex(splitters, 0));

      CFTypeRef orientation;
      AXUIElementCopyAttributeValue(splitter, kAXOrientationAttribute, &orientation);
      if (orientation != nullptr) {
        result = CFEqual(orientation, kAXVerticalOrientationValue); // Orthogonal to the group's orientation.
        CFRelease(orientation);
      }
      CFRelease(splitters);

      return result;
    }

    default:
      throw std::runtime_error("This element does not support layout informations.");
  }
}

//----------------------------------------------------------------------------------------------------------------------

CheckState Accessible::getCheckState() const {
  if (_role != Role::CheckBox && _role != Role::RadioButton && _role != Role::MenuItem)
    throw std::runtime_error("Check states not supported by this element.");

  // For menu items the value property is not supported and no other attribute exists to indicate the check state.
  // Hence we have to use the character that is shown for that.
  if (_role == Role::MenuItem) {
    std::string checkValue = getStringValue(_native, kAXMenuItemMarkCharAttribute, "menu item check mark value", true);
    if (checkValue.empty())
      return CheckState::Unchecked;
    if (checkValue == "-")
      return CheckState::Indeterminate;

    return CheckState::Checked;
  }

  switch (getNumberValue(_native, kAXValueAttribute, "value").intValue) {
    case 0:
      return CheckState::Unchecked;
    case 1:
      return CheckState::Checked;
    default:
      return CheckState::Indeterminate;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setCheckState(CheckState state) {
  if (_role != Role::CheckBox && _role != Role::RadioButton)
    throw std::runtime_error("Check states not supported by this element.");

  switch (state) {
    case CheckState::Unchecked:
      setValue(0);
      break;
    case CheckState::Checked:
      setValue(1);
      break;
    default:
      setValue(2);
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isExpandable() const {
  switch (_role) {
    case Role::ComboBox:
      return true;
    case Role::Row:
    case Role::Expander:
      return isSupported(_native, kAXDisclosingAttribute);
    default:
      throw std::runtime_error("Expand state not supported by this element.");
  }
  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isExpanded() {
  switch (_role) {
    case Role::ComboBox: { // We treat comboboxes and popup buttons both as comboboxes here.
      if (nativeRoleIsOneOf(_native, { kAXComboBoxRole }))
        return getBoolValue(_native, kAXExpandedAttribute, "expanded");

      auto children = getChildren(_native);
      return children.count > 0;
    }
    case Role::Row:
    case Role::Expander:
      return getBoolValue(_native, kAXDisclosingAttribute, "expanded");
    default:
      throw std::runtime_error("Expand state not supported by this element.");
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setExpanded(bool value) {
  switch (_role) {
    case Role::ComboBox:
      if (nativeRoleIsOneOf(_native, { kAXComboBoxRole }))
        setBoolValue(_native, kAXExpandedAttribute, value, "expanded");
      else {
        if (value)
          AXUIElementPerformAction(_native, kAXShowMenuAction);
        else {
          auto children = getChildren(_native);
          if (children.count > 0) {
            AXUIElementRef menu = (__bridge AXUIElementRef)children[0];
            AXUIElementPerformAction(menu, kAXCancelAction);
          }
        }
      }
      break;
    case Role::Row:
    case Role::Expander:
      setBoolValue(_native, kAXDisclosingAttribute, value, "expanded");
      break;
    default:
      throw std::runtime_error("Expand state not supported by this element.");
  }
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getValue() const {
  return [getNumberValue(_native, kAXValueAttribute, "value") doubleValue];
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getMaxValue() const {
  return [getNumberValue(_native, kAXMaxValueAttribute, "max value") doubleValue];
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getMinValue() const {
  return [getNumberValue(_native, kAXMinValueAttribute, "min value") doubleValue];
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setValue(double value) {
  setNumberValue(_native, kAXValueAttribute, [NSNumber numberWithDouble: value], "value");
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getRange() const {
  if (_role != Role::ScrollBar)
    throw std::runtime_error("The range attribute is only supported for scrollbars.");

  //return getMaxValue() - getMinValue();
  return 0; // No known way to determine this value.
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getActiveTabPage() const {
  if (_role != Role::TabView)
    throw std::runtime_error("Only tab views have tab pages.");

  for (auto &page : tabPages()) {
    if (page->getValue() == 1)
      return page->getTitle();
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setActiveTabPage(std::string const& name) {
  if (_role != Role::TabView)
    throw std::runtime_error("Only tab views have tab pages.");

  for (auto &page : tabPages()) {
    if (page->getTitle() == name)
      page->click();
      //page->setFocused(); Works too, but is slower.
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::activate() {
  if (_role != Role::TabPage && _role != Role::MenuItem && _role != Role::Menu)
    throw std::runtime_error("Cannot activate this element.");

  auto native = _native;
  if (_role == Role::Menu)
    native = getParent(_native);
  press(native);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isActiveTab() const {
  if (_role != Role::TabPage)
    throw std::runtime_error("This element has no activity status.");

  return getBoolValue(_native, kAXValueAttribute, "value");
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSelected() const {
  if (_role != Role::Row && _role != Role::Column && _role != Role::MenuItem)
    throw std::runtime_error("This element cannot be selected.");

  return getBoolValue(_native, kAXSelectedAttribute, "selected");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelected(bool value) {
  if (_role != Role::Row && _role != Role::Column && _role != Role::MenuItem)
    throw std::runtime_error("This element cannot be selected.");

  setBoolValue(_native, kAXSelectedAttribute, value, "selected");
}

//----------------------------------------------------------------------------------------------------------------------

double Accessible::getScrollPosition() const {
  if (_role != Role::ScrollBar)
    throw std::runtime_error("The scroll position is only supported by scrollbars.");

  return [getNumberValue(_native, kAXValueAttribute, "scroll position") doubleValue];
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setScrollPosition(double value) {
  if (_role != Role::ScrollBar)
    throw std::runtime_error("The scroll position is only supported by scrollbars.");

  setNumberValue(_native, kAXValueAttribute, [NSNumber numberWithDouble: value], "scroll position");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Show to element's (context) menu, if supported.
 */
void Accessible::showMenu() const {
  AXUIElementSetMessagingTimeout(_native, 0.1);
  AXError error = AXUIElementPerformAction(_native, kAXShowMenuAction);
  AXUIElementSetMessagingTimeout(_native, 0);
  handleUnsupportedError(error, "context menu");
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::menuShown() const {
  if (_role != Role::Menu)
    throw std::runtime_error("Shown attribute only valid for menus.");

  auto visibleChildren = getChildren(_native, 10, true);
  return visibleChildren.count > 0;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getID() const {
  return getStringValue(_native, kAXIdentifierAttribute, "id", true);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getName() const {
  // Accessibility on macOS is pretty confusing. There's a field `accessibilityLabel`, which when set uses the
  // description key (not the label value key as one would think). Still the content of the description is shown as
  // label in the accessibility inspector. If a title is set via `accessibilityTitle`, the label is set instead if no
  // explicit label value has been assigned.
  auto result = getStringValue(_native, kAXDescriptionAttribute, "name", true);
  if (result.empty()) // Some elements (particularly those created internally) have no description/label.
    result = getStringValue(_native, kAXTitleAttribute, "name", true);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getHelp() const {
  return getStringValue(_native, kAXHelpAttribute, "help");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns true if this is an internal element, like a window zoom button or other implicitly added content.
 */
bool Accessible::isInternal() const {
  // There are quite a few containers that are implicitely created (e.g. windows + scrollboxes), which would qualify
  // as internal elements. However, if they are taken out we lose all their (potentially not-internal) child elements.
  CFIndex count;
  AXError error = AXUIElementGetAttributeValueCount(_native, kAXChildrenAttribute, &count);
  if (error == kAXErrorSuccess && count > 0) {
    return false;
  }

  if (getName().empty()) {
    auto identifier = getID();
    if (mga::Utilities::hasPrefix(identifier, "_NS:"))
      return true;
  }

  // Some internal elements have no internal identifier.
  if (nativeRoleIsOneOf(_native,
    { kAXCloseButtonSubrole, kAXZoomButtonSubrole, kAXFullScreenButtonSubrole, kAXMinimizeButtonSubrole },
    kAXSubroleAttribute)) {
    return true;
  }

  return false;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::equals(Accessible *other) const {
  return CFEqual(this->_native, other->_native);
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getParent() const {
  AXUIElementRef parent = getParent(_native);
  if (parent == nullptr)
    return nullptr;

  Accessible *accessible = new Accessible(parent);
  CFRelease(parent);

  return AccessibleRef(accessible);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * If this element is part of a treeview or grid this function returns the row to which the element belongs
 * (otherwise an invalid reference is returned). Rows + columns are not everywhere natively supported in which case
 * we fake an instance to make this concept consistent accross platforms.
 */
AccessibleRef Accessible::getContainingRow() const {
  if (_role == Role::Row)
    return clone();

  auto run = getParent(_native);
  while (run != nullptr) {
    if (nativeRoleIsOneOf(run, { kAXRowRole })) {
      auto result = AccessibleRef(new Accessible(run));
      CFRelease(run);
      return result;
    }

    auto temp = getParent(run);
    CFRelease(run);
    run = temp;
  }

  return nullptr;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getHorizontalScrollBar() const {
  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXHorizontalScrollBarAttribute, &value);
  handleUnsupportedError(error, "horizontal scrollbar");

  auto result = AccessibleRef(new Accessible(static_cast<AXUIElementRef>(value)));
  if (error == kAXErrorSuccess)
    CFRelease(value);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getVerticalScrollBar() const {
  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXVerticalScrollBarAttribute, &value);
  handleUnsupportedError(error, "vertical scrollbar");

  auto result = AccessibleRef(new Accessible(static_cast<AXUIElementRef>(value)));
  if (error == kAXErrorSuccess)
    CFRelease(value);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getHeader() const {
  if (_role != Role::Column)
    throw std::runtime_error("This element does not have a heading.");

  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXHeaderAttribute, &value);
  handleUnsupportedError(error, "header");

  auto result = AccessibleRef(new Accessible(static_cast<AXUIElementRef>(value)));
  if (error == kAXErrorSuccess)
    CFRelease(value);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::getCloseButton() const {
  if (_role != Role::TabPage)
    throw std::runtime_error("This element does not have a close button.");

  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXCloseButtonAttribute, &value);
  if (value == nullptr)
    return nullptr;
  handleUnsupportedError(error, "close button");

  auto result = AccessibleRef(new Accessible(static_cast<AXUIElementRef>(value)));
  CFRelease(value);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::show() {
  NOT_IMPLEMENTED;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::bringToFront() {
  switch (_role) {
    case Role::Window: {
      AXError error = AXUIElementPerformAction(_native, kAXRaiseAction);
      handleUnsupportedError(error, "bringToFront");
      // fallthrough
    }

    case Role::Application: {
      pid_t pid;
      AXUIElementGetPid(_native, &pid);
      NSRunningApplication *application = [NSRunningApplication runningApplicationWithProcessIdentifier: pid];
      [application activateWithOptions: NSApplicationActivateAllWindows | NSApplicationActivateIgnoringOtherApps];
      break;
    }

    default: {
      throw std::runtime_error("Action not supported by this element");
      break;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

@interface HighlightWindow : NSWindow
@end

@implementation HighlightWindow
- (NSTimeInterval)animationResizeTime: (NSRect)newFrame {
  return 0.1;
}

@end

static HighlightWindow *highlightWindow = nil;

void Accessible::highlight(NSColor *color) const {
  if (isValid() && hasBounds(_native)) {
    Rectangle bounds = getBounds(true);
    NSRect frame = NSMakeRect(bounds.position.x, bounds.position.y, bounds.size.width, bounds.size.height);
    float screenHeight = NSMaxY([[NSScreen.screens objectAtIndex: 0] frame]);
    frame.origin.y = screenHeight - (bounds.position.y + bounds.size.height);

    if (highlightWindow == nil) {
      highlightWindow = [[HighlightWindow alloc] initWithContentRect: frame
                                                           styleMask: NSWindowStyleMaskBorderless
                                                             backing: NSBackingStoreBuffered
                                                               defer: YES];
      highlightWindow.level = NSScreenSaverWindowLevel;
      highlightWindow.hasShadow = NO;
      highlightWindow.opaque = NO;
      highlightWindow.backgroundColor = color;
      highlightWindow.alphaValue = 0.5;
      highlightWindow.ignoresMouseEvents = YES;
      [highlightWindow orderFront: nil];
    } else {
      highlightWindow.backgroundColor = color;
      [highlightWindow setFrame: frame display: YES animate: YES];
      [highlightWindow orderFront: nil];
    }
    //[NSRunLoop.currentRunLoop runMode: NSDefaultRunLoopMode
    //                       beforeDate: [NSDate.date dateByAddingTimeInterval: 0.010]];
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::removeHighlight() const {
  if (highlightWindow != nil) {
    [highlightWindow orderOut: nil];
    //[NSRunLoop.currentRunLoop runMode: NSDefaultRunLoopMode
    //                       beforeDate: [NSDate.date dateByAddingTimeInterval: 0.010]];
  }
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isHighlightActive() const {
  return highlightWindow.visible;
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getPlatformRoleName() const {
  return getNativeRole(_native);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::printNativeInfo() const {
  printInfo(_native);
}

//----------------------------------------------------------------------------------------------------------------------

static std::map<std::string, std::string> attributeToPropertyMap = {
  // informational attributes
  { toString(kAXTitleAttribute), "Title" },
  { toString(kAXDescriptionAttribute), "Description" },
  { toString(kAXHelpAttribute), "Help" },
  { toString(kAXIdentifierAttribute), "Internal Identifier" },

  // hierarchy or relationship attributes
  { toString(kAXParentAttribute), "Parent" },
  { toString(kAXChildrenAttribute), "Children" },
  { toString(kAXSelectedChildrenAttribute), "Selected Children" },
  { toString(kAXVisibleChildrenAttribute), "Visible Children" },
  { toString(kAXWindowAttribute), "Window" },
  { toString(kAXTopLevelUIElementAttribute), "Top Level UI Element" },
  { toString(kAXTitleUIElementAttribute), "Title UI Element" },

  // visual state attributes
  { toString(kAXEnabledAttribute), "Enabled" },
  { toString(kAXFocusedAttribute), "Focused" },
  { toString(kAXPositionAttribute), "Position" },
  { toString(kAXSizeAttribute), "Size" },

  // value attributes
  { toString(kAXValueAttribute), "Value" },
  { toString(kAXValueDescriptionAttribute), "" },
  { toString(kAXMinValueAttribute), "Min Value" },
  { toString(kAXMaxValueAttribute), "Max Value" },
  { toString(kAXValueIncrementAttribute), "Value Increment" },
  { toString(kAXValueWrapsAttribute), "Wraps Content" },

  // text-specific attributes
  { toString(kAXSelectedTextAttribute), "Selected Text" },
  { toString(kAXSelectedTextRangeAttribute), "Selected Text Range" },
  { toString(kAXSelectedTextRangesAttribute), "Selected Text Ranges" },
  { toString(kAXVisibleCharacterRangeAttribute), "Visible Character Range" },
  { toString(kAXNumberOfCharactersAttribute), "Character Count" },

  // window, sheet, or drawer-specific attributes
  { toString(kAXMainAttribute), "Main Window" },
  { toString(kAXMinimizedAttribute), "Minimized" },
  { toString(kAXCloseButtonAttribute), "Close Button" },
  { toString(kAXZoomButtonAttribute), "Zoom Button" },
  { toString(kAXMinimizeButtonAttribute), "Minimize Button" },
  { toString(kAXToolbarButtonAttribute), "Toolbar Button" },
  { toString(kAXFullScreenButtonAttribute), "Full Screen Button" },
  { toString(kAXGrowAreaAttribute), "Grow Area" },
  { toString(kAXModalAttribute), "Modal" },
  { toString(kAXDefaultButtonAttribute), "Default Button" },
  { toString(kAXCancelButtonAttribute), "Cancel Button" },

  // menu or menu item-specific attributes
  { toString(kAXMenuItemCmdCharAttribute), "" },
  { toString(kAXMenuItemCmdVirtualKeyAttribute), "" },
  { toString(kAXMenuItemCmdGlyphAttribute), "" },
  { toString(kAXMenuItemCmdModifiersAttribute), "" },
  { toString(kAXMenuItemMarkCharAttribute), "" },
  { toString(kAXMenuItemPrimaryUIElementAttribute), "" },

  // application element-specific attributes
  { toString(kAXMenuBarAttribute), "Menu Bar" },
  { toString(kAXWindowsAttribute), "Windows" },
  { toString(kAXFrontmostAttribute), "Front Most Application" },
  { toString(kAXHiddenAttribute), "Hidden" },
  { toString(kAXMainWindowAttribute), "Main Window" },
  { toString(kAXFocusedWindowAttribute), "Focused Window" },
  { toString(kAXFocusedUIElementAttribute), "Focused Element" },

  // date/time-specific attributes
  { toString(kAXHourFieldAttribute), "Hour Field" },
  { toString(kAXMinuteFieldAttribute), "Minute Field" },
  { toString(kAXSecondFieldAttribute), "Second Field" },
  { toString(kAXAMPMFieldAttribute), "AM/PM Field" },
  { toString(kAXDayFieldAttribute), "Day Field" },
  { toString(kAXMonthFieldAttribute), "Month Field" },
  { toString(kAXYearFieldAttribute), "Year Field" },

  // table, outline, or browser-specific attributes
  { toString(kAXRowsAttribute), "Rows" },
  { toString(kAXVisibleRowsAttribute), "Visible Rows" },
  { toString(kAXSelectedRowsAttribute), "Selected Rows" },
  { toString(kAXColumnsAttribute), "Columns" },
  { toString(kAXVisibleColumnsAttribute), "Visible Columns" },
  { toString(kAXSelectedColumnsAttribute), "Selected Columns" },
  { toString(kAXSortDirectionAttribute), "Sort Direction" },
  { toString(kAXColumnHeaderUIElementsAttribute), "Column Headers" },
  { toString(kAXIndexAttribute), "Index" },
  { toString(kAXDisclosingAttribute), "Has Disclosing Arrow" },
  { toString(kAXDisclosedRowsAttribute), "Disclosed Rows" },

  // miscellaneous or role-specific attributes
  { toString(kAXHorizontalScrollBarAttribute), "Horizontal Scrollbar" },
  { toString(kAXVerticalScrollBarAttribute), "Vertical Scrollbar" },
  { toString(kAXOrientationAttribute), "Orientation" },
  { toString(kAXHeaderAttribute), "Header" },
  { toString(kAXEditedAttribute), "Edited" },
  { toString(kAXTabsAttribute), "Tabs" },
  { toString(kAXOverflowButtonAttribute), "Overflow Button" },
  { toString(kAXFilenameAttribute), "Filename" },
  { toString(kAXExpandedAttribute), "Expanded" },
  { toString(kAXSelectedAttribute), "Selected" },
  { toString(kAXSplittersAttribute), "Splitters" },
  { toString(kAXContentsAttribute), "Contents" },
  { toString(kAXNextContentsAttribute), "Following Elements" },
  { toString(kAXPreviousContentsAttribute), "Preceding Elements" },
  { toString(kAXDocumentAttribute), "Document" },
  { toString(kAXIncrementorAttribute), "Incrementor" },
  { toString(kAXDecrementButtonAttribute), "Decrement Button" },
  { toString(kAXIncrementButtonAttribute), "Increment Button" },
  { toString(kAXColumnTitleAttribute), "Column Title" },
  { toString(kAXURLAttribute), "URL" },
  { toString(kAXLabelValueAttribute), "Label Value" },
  { toString(kAXShownMenuUIElementAttribute), "Context Menu Items" },
  { toString(kAXIsApplicationRunningAttribute), "Application Running" },
  { toString(kAXFocusedApplicationAttribute), "Application Focused" },
  { toString(kAXElementBusyAttribute), "Busy" },

  // Attributes without a constant.
  { "AXEnhancedUserInterface", "Enhanced UI" },
  { "AXFullScreen", "Full Screen" },
  { "AXFrame", "Bounds" },
  { "AXPlaceholderValue", "Placeholder Value" },
};

static std::set<std::string> ignoredAttributes = {
  toString(kAXRoleAttribute),
  toString(kAXRoleDescriptionAttribute),
  toString(kAXSubroleAttribute),
  toString(kAXExtrasMenuBarAttribute),
  toString(kAXInsertionPointLineNumberAttribute),
  toString(kAXInsertionPointLineNumberAttribute),
  toString(kAXGrowAreaAttribute),
  toString(kAXProxyAttribute),
  toString(kAXDisclosedByRowAttribute),
  toString(kAXServesAsTitleForUIElementsAttribute),
  toString(kAXLinkedUIElementsAttribute),
  toString(kAXSharedFocusElementsAttribute),
  toString(kAXAllowedValuesAttribute),
  toString(kAXSharedTextUIElementsAttribute),
  toString(kAXSharedCharacterRangeAttribute),
  toString(kAXProxyAttribute),
  toString(kAXExtrasMenuBarAttribute),

  // matte-specific attributes
  toString(kAXMatteHoleAttribute),
  toString(kAXMatteContentUIElementAttribute),

  // ruler-specific attributes
  toString(kAXMarkerUIElementsAttribute),
  toString(kAXUnitsAttribute),
  toString(kAXUnitDescriptionAttribute),
  toString(kAXMarkerTypeAttribute),
  toString(kAXMarkerTypeDescriptionAttribute),

  toString(kAXLabelUIElementsAttribute),
  toString(kAXAlternateUIVisibleAttribute),

  "AXFunctionRowTopLevelElements",
  "AXChildrenInNavigationOrder",
  "AXTextInputMarkedRange",
  "AXAuditIssues",
  "AXSections",
};

static std::map<std::string, std::string> subRoleMap = {
  // standard subroles
  { toString(kAXCloseButtonSubrole), "Close Button" },
  { toString(kAXMinimizeButtonSubrole), "Minimize Button" },
  { toString(kAXZoomButtonSubrole), "Zoom Button" },
  { toString(kAXToolbarButtonSubrole), "Toolbar Button" },
  { toString(kAXFullScreenButtonSubrole), "Full Screen Button" },
  { toString(kAXSecureTextFieldSubrole), "Secure Text Field" },
  { toString(kAXTableRowSubrole), "Table Row" },
  { toString(kAXOutlineRowSubrole), "Outline Row" },

  // new subroles
  { toString(kAXStandardWindowSubrole), "Standard Window" },
  { toString(kAXDialogSubrole), "Dialog" },
  { toString(kAXSystemDialogSubrole), "System Dialog" },
  { toString(kAXFloatingWindowSubrole), "Floating Window" },
  { toString(kAXSystemFloatingWindowSubrole), "System Floating Window" },
  { toString(kAXIncrementArrowSubrole), "Increment Arrow" },
  { toString(kAXDecrementArrowSubrole), "Decrement Arrow" },
  { toString(kAXIncrementPageSubrole), "Increment Page" },
  { toString(kAXDecrementPageSubrole), "Decrement Page" },
  { toString(kAXSortButtonSubrole), "Sort Button" },
  { toString(kAXSearchFieldSubrole), "Search Field" },
  { toString(kAXTimelineSubrole), "Time Line" },
  { toString(kAXRatingIndicatorSubrole), "Rating Indicator" },
  { toString(kAXContentListSubrole), "Content List" },
  { toString(kAXDefinitionListSubrole), "Definition List" },
  { toString(kAXDescriptionListSubrole), "Description List" },
  { toString(kAXToggleSubrole), "Toggle" },
  { toString(kAXSwitchSubrole), "Switch" },

  // dock subroles
  { toString(kAXApplicationDockItemSubrole), "Application Dock Item" },
  { toString(kAXDocumentDockItemSubrole), "Document Dock Item" },
  { toString(kAXFolderDockItemSubrole), "Folder Dock Item" },
  { toString(kAXMinimizedWindowDockItemSubrole), "Minimized Window Dock Item" },
  { toString(kAXURLDockItemSubrole), "Dock Item" },
  { toString(kAXDockExtraDockItemSubrole), "Dock Extra Item" },
  { toString(kAXTrashDockItemSubrole), "Trash Dock Item" },
  { toString(kAXSeparatorDockItemSubrole), "Separator Dock Item" },
  { toString(kAXProcessSwitcherListSubrole), "Process Switcher List" },

  // Others
  { "AXTabButton", "Tab" },
  { "AXTextLink", "Hyperlink" },
};

// Attributes refering to other UI elements.
static std::set<std::string> references = {
  toString(kAXParentAttribute),
  toString(kAXChildrenAttribute),
  toString(kAXSelectedChildrenAttribute),
  toString(kAXVisibleChildrenAttribute),
  toString(kAXWindowAttribute),
  toString(kAXTopLevelUIElementAttribute),
  toString(kAXTitleUIElementAttribute),
  toString(kAXMainAttribute),
  toString(kAXCloseButtonAttribute),
  toString(kAXZoomButtonAttribute),
  toString(kAXMinimizeButtonAttribute),
  toString(kAXToolbarButtonAttribute),
  toString(kAXFullScreenButtonAttribute),
  toString(kAXGrowAreaAttribute),
  toString(kAXDefaultButtonAttribute),
  toString(kAXCancelButtonAttribute),
  toString(kAXMenuBarAttribute),
  toString(kAXWindowsAttribute),
  toString(kAXMainWindowAttribute),
  toString(kAXFocusedWindowAttribute),
  toString(kAXFocusedUIElementAttribute),
  toString(kAXHourFieldAttribute),
  toString(kAXMinuteFieldAttribute),
  toString(kAXSecondFieldAttribute),
  toString(kAXAMPMFieldAttribute),
  toString(kAXDayFieldAttribute),
  toString(kAXMonthFieldAttribute),
  toString(kAXYearFieldAttribute),
  toString(kAXRowsAttribute),
  toString(kAXVisibleRowsAttribute),
  toString(kAXSelectedRowsAttribute),
  toString(kAXColumnsAttribute),
  toString(kAXVisibleColumnsAttribute),
  toString(kAXSelectedColumnsAttribute),
  toString(kAXColumnHeaderUIElementsAttribute),
  toString(kAXDisclosedRowsAttribute),
  toString(kAXHorizontalScrollBarAttribute),
  toString(kAXVerticalScrollBarAttribute),
  toString(kAXHeaderAttribute),
  toString(kAXTabsAttribute),
  toString(kAXOverflowButtonAttribute),
  toString(kAXSplittersAttribute),
  toString(kAXContentsAttribute),
  toString(kAXNextContentsAttribute),
  toString(kAXPreviousContentsAttribute),
  toString(kAXDocumentAttribute),
  toString(kAXIncrementorAttribute),
  toString(kAXDecrementButtonAttribute),
  toString(kAXIncrementButtonAttribute),
  toString(kAXShownMenuUIElementAttribute),
};

static std::map<std::string, std::string> actionMap = {
  { toString(kAXPressAction), "Click Element"},
  { toString(kAXIncrementAction), "Increment Value" },
  { toString(kAXDecrementAction), "Decrement Value" },
  { toString(kAXConfirmAction), "Confirm" },
  { toString(kAXCancelAction), "Cancel" },
  { toString(kAXShowAlternateUIAction), "Show Alternate UI" },
  { toString(kAXShowDefaultUIAction), "Show Default UI" },
  { toString(kAXRaiseAction), "Bring to Front" },
  { toString(kAXShowMenuAction), "Show Menu" },
  { toString(kAXPickAction), "Select Item" },
  { "AXScrollLeftByPage", "Scroll Page Left" },
  { "AXScrollRightByPage", "Scroll Page Right" },
  { "AXScrollUpByPage", "Scroll Page Up" },
  { "AXScrollDownByPage", "Scroll Page Down" },
};

/**
 * Returns human readable details about this instance.
 */
AccessibleDetails Accessible::getDetails() const {
  std::string subRole = getNativeRole(_native, kAXSubroleAttribute);
  auto subRoleIterator = subRoleMap.find(subRole);
  if (subRoleIterator != subRoleMap.end())
    subRole = subRoleIterator->second;
  AccessibleDetails result = { roleToFriendlyString(_role), subRole, {}, {} };

  CFArrayRef array;
  AXUIElementCopyAttributeNames(_native, &array);
  if (array != nil) {
    NSArray *properties = (__bridge NSArray *)array;
    for (NSString *property in properties) {
      std::string name = property.UTF8String;
      if (ignoredAttributes.count(name) > 0)
        continue;

      bool containsReference = references.count(name) > 0;
      auto iterator = attributeToPropertyMap.find(name);
      if (!iterator->second.empty()) {
        if (iterator != attributeToPropertyMap.end())
          name = iterator->second;
        Boolean settable = false;
        AXUIElementIsAttributeSettable(_native, (CFStringRef)property, &settable);

        std::string stringValue;
        CFTypeRef value;
        AXError error = AXUIElementCopyAttributeValue(_native, (CFStringRef)property, &value);
        if (error == kAXErrorSuccess) {
          stringValue = valueDescription((AXValueRef)value);
        }

        result.properties.push_back({ name, stringValue, !settable, containsReference });
      }
    }

    CFRelease(array);
  }

  AXUIElementCopyActionNames(_native, &array);
  if (array != nil) {
    NSArray *actions = (__bridge NSArray *)array;
    for (NSString *action in actions) {
      std::string description = "<empty>";
      CFStringRef value;
      AXError error = AXUIElementCopyActionDescription(_native, (CFStringRef)action, &value);
      if (error == kAXErrorSuccess)
        description = toString(value);

      std::string name = action.UTF8String;
      auto iterator = actionMap.find(name);
      if (iterator != actionMap.end())
        name = iterator->second;
      result.actions.push_back({ name, description });
    }
    CFRelease(array);
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::takeScreenShot(std::string const& path, bool onlyWindow, geometry::Rectangle rect) const {
  CGImageRef image = nullptr;
  CGRect r = CGRectMake(rect.position.x, rect.position.y, rect.size.width, rect.size.height);
  if (onlyWindow) {
    // If getting a screenshot for a window only the given coordinates are in the window coordinate system
    // and must be converted to screen coordinates first.
    CFTypeRef pointRef;
    CGPoint point;
    AXError error = AXUIElementCopyAttributeValue(_native, kAXPositionAttribute, &pointRef);
    handleUnsupportedError(error, "position");

    AXValueGetValue(static_cast<AXValueRef>(pointRef), static_cast<AXValueType>(kAXValueCGPointType), (void *)&point);
    CFRelease(pointRef);
    r.origin.x += point.x;
    r.origin.y += point.y;

    CGWindowID windowId;
    if (_AXUIElementGetWindow(_native, &windowId) == kAXErrorSuccess) {
      image = CGWindowListCreateImage(r, kCGWindowListOptionIncludingWindow, windowId,
                                      kCGWindowImageBoundsIgnoreFraming);
    } else
      throw std::runtime_error("Can't get window id");
  } else {
    image = CGWindowListCreateImage(r, kCGWindowListOptionAll, kCGNullWindowID, kCGWindowImageDefault);
  }

  if (image) {
    writeImageToFile(image, [NSString stringWithUTF8String: path.c_str()]);
    CFRelease(image);
  } else
    throw std::runtime_error("Can't take screenshot");
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Retrieves the frame of the view represented by this accessible.
 * Origin is the upper left corner of the screen holding the menu bar or the upper left corner of this UI element,
 * depending on the screenCoordinates parameter (so +y points down).
 */
Rectangle Accessible::getBounds(bool screenCoordinates) const {
  return Accessible::getBounds(_native, screenCoordinates);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setBounds(geometry::Rectangle const& bounds) {
  Accessible::setBounds(_native, bounds);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getText() const {
  return getStringValue(_native, kAXValueAttribute, "text");
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getTitle() const {
  if (_role == Role::GroupBox) {
    CFTypeRef titleElement;
    AXError error = AXUIElementCopyAttributeValue(_native, kAXTitleUIElementAttribute, &titleElement);
    if (error != kAXErrorSuccess || titleElement == nullptr) {
      return "";
    }
    std::string title = getStringValue(static_cast<AXUIElementRef>(titleElement), kAXValueAttribute, "title");
    return title;
  }

  return getStringValue(_native, kAXTitleAttribute, "title");
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t Accessible::getCaretPosition() const {
  AXValueRef theValue;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXSelectedTextRangeAttribute, (CFTypeRef *)&theValue);
  handleUnsupportedError(error, "selected text range");

  NSRange range;
  AXValueGetValue(theValue, static_cast<AXValueType>(kAXValueCFRangeType), (void *)&range);
  return static_cast<std::size_t>(range.location);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Setting the caret position clears the current selection range.
 */
void Accessible::setCaretPosition(size_t position) {
  NSRange range;
  range.location = position;
  range.length = 0;

  AXValueRef valueRef = AXValueCreate(static_cast<AXValueType>(kAXValueCFRangeType), static_cast<const void *>(&range));
  AXUIElementSetAttributeValue(_native, kAXSelectedTextRangeAttribute, valueRef);
}

//----------------------------------------------------------------------------------------------------------------------

std::size_t Accessible::getCharacterCount() const {
  auto text = getStringValue(_native, kAXValueAttribute, "value");
  auto wideText = mga::Utilities::s2ws(text);
  return wideText.size();
}

//----------------------------------------------------------------------------------------------------------------------

std::set<size_t> Accessible::getSelectedIndexes() const {
  if (_role != Role::ComboBox && _role != Role::IconView)
    throw std::runtime_error("This element does not support selected indexes.");

  std::set<size_t> result;
  if (_native) {
    AXUIElementRef list = _native;
    NSArray *contentList = nil;

    if (_role == Role::ComboBox) {
      if (nativeRoleIsOneOf(_native, { kAXPopUpButtonRole })) {
        // A popup button then. Can have a single child (a menu).
        AXUIElementRef menu = getFirstChild(_native);
        if (menu != nullptr) {
          auto children = getChildren(menu);
          CFRelease(menu);
          for (NSUInteger i = 0; i < children.count; ++i) {
            if (!getStringValue((__bridge AXUIElementRef)children[i], kAXMenuItemMarkCharAttribute,
                                "menu item marker").empty()) {
              result.insert(i);
            }
          }
        }
        return result;
      } else {
        auto subParts = getChildren(_native, 2);

        // Selected indices can only be determined if the combobox is expanded currently.
        if (subParts.count < 2)
          return result;

        AXUIElementRef scrollArea = (__bridge AXUIElementRef)(subParts[1]);
        CFTypeRef content;
        AXUIElementCopyAttributeValue(scrollArea, kAXContentsAttribute, &content);
        if (!content)
          return result;

        contentList = (NSArray *)CFBridgingRelease(content);
        list = (__bridge AXUIElementRef)(contentList[0]);
      }
    }

    auto children = getChildren(list);
    if (children == nullptr) {
      return result;
    }

    CFArrayRef selected;
    AXUIElementCopyAttributeValues(list, kAXSelectedChildrenAttribute, 0, 99999, &selected);
    if (selected == nullptr) {
      return result;
    }

    NSArray *selectedList = (NSArray *)CFBridgingRelease(selected);
    for  (id entry in selectedList) {
      result.insert([children indexOfObject: entry]);
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectedIndexes(std::set<size_t> const& indexes) {
  if (_role != Role::ComboBox && _role != Role::IconView)
    throw std::runtime_error("This element does not support selected indexes.");

  if (_native) {
    AXUIElementRef list = _native;
    NSArray *contentList = nil;

    if (_role == Role::ComboBox) {
      if (nativeRoleIsOneOf(_native, { kAXPopUpButtonRole })) {
        AXUIElementRef menu = getFirstChild(_native);
        if (menu != nullptr) {
          auto children = getChildren(menu);
          CFRelease(menu);
          if (!indexes.empty() && *indexes.begin() < children.count) {
            press((__bridge AXUIElementRef)children[*indexes.begin()]);
          }
        }
        return;
      } else {
        auto subParts = getChildren(_native, 2);
        if (subParts.count < 2)
          return;

        AXUIElementRef scrollArea = (__bridge AXUIElementRef)(subParts[1]);
        CFTypeRef content;
        AXUIElementCopyAttributeValue(scrollArea, kAXContentsAttribute, &content);
        if (!content)
          return;

        contentList = (NSArray *)CFBridgingRelease(content);
        list = (__bridge AXUIElementRef)contentList[0];
      }
    }

    auto children = getChildren(list);
    if (children == nil) {
      return;
    }

    NSMutableArray *selection = [NSMutableArray new];
    for (size_t index : indexes) {
      [selection addObject: [children objectAtIndex: index]];
    }

    AXError error = AXUIElementSetAttributeValue(list, kAXSelectedChildrenAttribute, (__bridge CFTypeRef)selection);
    handleUnsupportedError(error, "selected indexes");
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::insertText(const std::size_t offset, const std::string &text) {
  NSRange range;
  range.location = offset;
  range.length = 0;
  AXValueRef valueRef = AXValueCreate(static_cast<AXValueType>(kAXValueCFRangeType), static_cast<const void *>(&range));
  AXError error = AXUIElementSetAttributeValue(_native, kAXSelectedTextRangeAttribute, valueRef);
  handleUnsupportedError(error, "selection range");

  setStringValue(_native, kAXSelectedTextAttribute, text, "text");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setText(std::string const& text) {
  setStringValue(_native, kAXValueAttribute, text, "text");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setTitle(std::string const& text) {
  setStringValue(_native, kAXTitleAttribute, text, "title");
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getDescription() const {
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getSelectedText() const {
  return getStringValue(_native, kAXSelectedTextAttribute, "selected text");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectedText(std::string const& text) {
  return setStringValue(_native, kAXSelectedTextAttribute, text, "selected text");
}

//----------------------------------------------------------------------------------------------------------------------

aal::TextRange Accessible::getSelectionRange() const {
  AXValueRef theValue;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXSelectedTextRangeAttribute, (CFTypeRef *)&theValue);
  handleUnsupportedError(error, "selected text range");

  NSRange range;
  AXValueGetValue(theValue, static_cast<AXValueType>(kAXValueCFRangeType), (void *)&range);
  return aal::TextRange(range.location, range.location + range.length);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setSelectionRange(TextRange range) {
  NSRange nativeRange;
  nativeRange.location = range.start;
  nativeRange.length = range.end - range.start;
  AXValueRef valueRef = AXValueCreate((AXValueType)kAXValueCFRangeType, (const void *)&nativeRange);
  AXUIElementSetAttributeValue(_native, kAXSelectedTextRangeAttribute, valueRef);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getDate() const {
  if (_role != Role::DatePicker)
    throw std::runtime_error("This element does not support date values.");

  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(_native, kAXValueAttribute, &value);
  handleUnsupportedError(error, "date");

  NSDate *date = (NSDate *)CFBridgingRelease(value);
  NSISO8601DateFormatter *formatter = [NSISO8601DateFormatter new];
  formatter.formatOptions |= kCFISO8601DateFormatWithFractionalSeconds;
  NSString *string = [formatter stringFromDate: date];
  std::string result = string.UTF8String;
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setDate(std::string const& date) {
  if (_role != Role::DatePicker)
    throw std::runtime_error("This element does not support date values.");

  NSISO8601DateFormatter *formatter = [NSISO8601DateFormatter new];
  formatter.formatOptions |= kCFISO8601DateFormatWithFractionalSeconds;
  NSDate *value = [formatter dateFromString: [NSString stringWithUTF8String: date.c_str()]];

  AXError error = AXUIElementSetAttributeValue(_native, kAXValueAttribute, (__bridge CFTypeRef)value);
  handleUnsupportedError(error, "date");
}

//----------------------------------------------------------------------------------------------------------------------

static CGEventType eventTypeFromButton(MouseButton button, bool down) {
  switch (button) {
      case MouseButton::Right:
      return down ? kCGEventRightMouseDown : kCGEventRightMouseUp;
    case MouseButton::Middle:
      return down ? kCGEventOtherMouseDown : kCGEventOtherMouseUp;
    default:
      return down ? kCGEventLeftMouseDown : kCGEventLeftMouseUp;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void sendMouseEvent(CGEventType type, CGPoint position, CGMouseButton button) {
  // The mouse button parameter is ignored, except for "other" mouse buttons.
  CGEventRef mouseEvent = CGEventCreateMouseEvent(nullptr, type, position, button);
  CGEventSetFlags(mouseEvent, kCGEventFlagMaskNonCoalesced);
  CGEventPost(kCGSessionEventTap, mouseEvent);
  CFRelease(mouseEvent);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseDown(geometry::Point pos, MouseButton button) {
  CGEventType eventType = eventTypeFromButton(button, true);
  sendMouseEvent(eventType, CGPointMake(pos.x, pos.y), kCGMouseButtonRight);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseUp(geometry::Point pos, MouseButton button) {
  CGEventType eventType = eventTypeFromButton(button, false);
  sendMouseEvent(eventType, CGPointMake(pos.x, pos.y), kCGMouseButtonRight);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseMove(geometry::Point pos) const {
  CGEventRef event = CGEventCreate(nil);
  CGPoint currentPos = CGEventGetLocation(event);
  CFRelease(event);
  sendMouseEvent(kCGEventMouseMoved, CGPointMake(currentPos.x + pos.x, currentPos.y + pos.y), kCGMouseButtonLeft);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseMoveTo(geometry::Point pos) const {
  sendMouseEvent(kCGEventMouseMoved, CGPointMake(pos.x, pos.y), kCGMouseButtonLeft);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::mouseDrag(geometry::Point source, geometry::Point target, MouseButton button) {
  if (button != MouseButton::Left && button != MouseButton::Right)
    return;

  CGEventType downEventType = button == MouseButton::Left ? kCGEventLeftMouseDown : kCGEventRightMouseDown;
  CGEventType dragEventType = button == MouseButton::Left ? kCGEventLeftMouseDragged : kCGEventRightMouseDragged;
  CGPoint sourcePoint = CGPointMake(source.x, source.y);
  CGPoint targetPoint = CGPointMake(target.x, target.y);

  // Start with a mouse move event (which isn't strictly necessary) to make code working that determines
  // elements on mouse hover.
  CGEventRef mouseMoveEvent = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, sourcePoint, kCGMouseButtonLeft);
  CGEventPost(kCGSessionEventTap, mouseMoveEvent);
  CFRelease(mouseMoveEvent);
  [NSThread sleepForTimeInterval: 0.1];

  CGEventRef mouseDownEvent = CGEventCreateMouseEvent(NULL, downEventType, sourcePoint, kCGMouseButtonLeft);
  CGEventPost(kCGSessionEventTap, mouseDownEvent);
  CFRelease(mouseDownEvent);

  [NSThread sleepForTimeInterval: 0.1];

  CGEventRef mouseDragEvent = CGEventCreateMouseEvent(NULL, dragEventType, targetPoint, kCGMouseButtonLeft);
  CGEventPost(kCGSessionEventTap, mouseDragEvent);
  CFRelease(mouseDragEvent);
  [NSThread sleepForTimeInterval: 0.1];

  CGEventRef mouseUpEvent = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, sourcePoint, kCGMouseButtonLeft);
  CGEventPost(kCGSessionEventTap, mouseUpEvent);
  CFRelease(mouseUpEvent);
  [NSThread sleepForTimeInterval: 0.1];

}

//----------------------------------------------------------------------------------------------------------------------

geometry::Point Accessible::getMousePosition() const {
  CGEventRef event = CGEventCreate(nil);
  CGPoint currentPos = CGEventGetLocation(event);
  CFRelease(event);
  return geometry::Point(currentPos.x, currentPos.y);
}

//----------------------------------------------------------------------------------------------------------------------

// Key enum -> native keycodes map. Index is aal::Key.
static std::vector<CGKeyCode> keyCodeMap = {
  0xFFFF,
  kVK_ANSI_0, kVK_ANSI_1, kVK_ANSI_2, kVK_ANSI_3, kVK_ANSI_4, kVK_ANSI_5, kVK_ANSI_6, kVK_ANSI_7, kVK_ANSI_8, kVK_ANSI_9,
  kVK_ANSI_KeypadPlus, kVK_ANSI_KeypadMinus,
  kVK_ANSI_A, kVK_ANSI_B, kVK_ANSI_C, kVK_ANSI_D, kVK_ANSI_E, kVK_ANSI_F, kVK_ANSI_G, kVK_ANSI_H, kVK_ANSI_I,
  kVK_ANSI_J, kVK_ANSI_K, kVK_ANSI_L, kVK_ANSI_M, kVK_ANSI_N, kVK_ANSI_O, kVK_ANSI_P, kVK_ANSI_Q, kVK_ANSI_R,
  kVK_ANSI_S, kVK_ANSI_T, kVK_ANSI_U, kVK_ANSI_V, kVK_ANSI_W, kVK_ANSI_X, kVK_ANSI_Y, kVK_ANSI_Z,

  kVK_Tab, 0 /* backspace */, kVK_Return, kVK_ANSI_Period, kVK_ANSI_Comma, kVK_ANSI_Semicolon, kVK_ANSI_Slash,
  kVK_ANSI_Backslash, kVK_ANSI_LeftBracket, kVK_ANSI_RightBracket,

  kVK_Delete, kVK_UpArrow, kVK_Escape, kVK_DownArrow, kVK_LeftArrow, kVK_RightArrow, kVK_PageUp, kVK_PageDown, kVK_End,
  kVK_Home, kVK_Space,

  kVK_F1, kVK_F2, kVK_F3, kVK_F4, kVK_F5, kVK_F6, kVK_F7, kVK_F8, kVK_F9, kVK_F10, kVK_F11, kVK_F12
};

//----------------------------------------------------------------------------------------------------------------------

static CGEventFlags modifierToFlags(aal::Modifier modifier) {
  CGEventFlags result = 0;

  if (containsModifier(modifier, aal::Modifier::ShiftLeft) || containsModifier(modifier, aal::Modifier::ShiftRight))
    result |= kCGEventFlagMaskShift;
  if (containsModifier(modifier, aal::Modifier::ControlLeft) || containsModifier(modifier, aal::Modifier::ControlRight))
    result |= kCGEventFlagMaskControl;
  if (containsModifier(modifier, aal::Modifier::AltLeft) || containsModifier(modifier, aal::Modifier::AltRight))
    result |= kCGEventFlagMaskAlternate;
  if (containsModifier(modifier, aal::Modifier::MetaLeft) || containsModifier(modifier, aal::Modifier::MetaRight))
    result |= kCGEventFlagMaskCommand;

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyDown(aal::Key k, aal::Modifier modifier) const {
  CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(NULL, keyCodeMap[static_cast<size_t>(k)], true);
  CGEventSetFlags(keyDownEvent, modifierToFlags(modifier));

  pid_t pid;
  AXUIElementGetPid(_native, &pid);
  CGEventPostToPid(pid, keyDownEvent);

  CFRelease(keyDownEvent);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyUp(aal::Key k, aal::Modifier modifier) const {
  CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(NULL, keyCodeMap[static_cast<size_t>(k)], false);
  CGEventSetFlags(keyUpEvent, modifierToFlags(modifier));

  pid_t pid;
  AXUIElementGetPid(_native, &pid);
  CGEventPostToPid(pid, keyUpEvent);

  CFRelease(keyUpEvent);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::keyPress(aal::Key k, aal::Modifier modifier) const {
  keyDown(k, modifier);
  keyUp(k, modifier);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Sends key events to the target, generated from a given string. This way there's no need to deal with keyboard layouts.
 */
void Accessible::typeString(std::string const& input) const {
  if (input.empty())
    return;

  // Apparently there must be an initial HID event for an application (or just the modal runloop?) before the artificial
  // key events work actually. Otherwise the set unicode string call below might close modal windows under certain
  // circumstances (e.g. when running as sub process from another GUI app, like XCode), failing so the entire input
  // method.
  mouseMove({ 0, -10 });
  mouseMove({ 0, 10 });

  std::wstring utf16 = mga::Utilities::s2ws(input);
  CGEventSourceRef eventSource = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);

  CGEventRef keyDownEvent = CGEventCreateKeyboardEvent(eventSource, 0, true);
  CGEventRef keyUpEvent = CGEventCreateKeyboardEvent(eventSource, 0, false);

  for (auto iterator : utf16) {
    UniChar temp = iterator;
    CGEventKeyboardSetUnicodeString(keyDownEvent, 1, &temp);
    CGEventPost(kCGSessionEventTap, keyDownEvent);
    CGEventKeyboardSetUnicodeString(keyUpEvent, 1, &temp);
    CGEventPost(kCGSessionEventTap, keyUpEvent);
  }

  CFRelease(keyDownEvent);
  CFRelease(keyUpEvent);
  CFRelease(eventSource);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::click() {
  press(_native);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Simulates pressing <return>, which is used when setting a selection index in a combobox (which can be both NSCombobox
 * and NSPopupButton, the latter doesn't support confirm).
 */
void Accessible::confirm(bool checkError) {
  AXError error = AXUIElementPerformAction(_native, kAXConfirmAction);
  if (checkError) // Not always wanted.
    handleUnsupportedError(error, "confirm");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::stepUp() {
  if (_role != Role::Stepper)
    throw std::runtime_error("Only stepper elements support this action.");

  AXError error = AXUIElementPerformAction(_native, kAXIncrementAction);
  handleUnsupportedError(error, "increment");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::stepDown() {
  if (_role != Role::Stepper)
    throw std::runtime_error("Only stepper elements support this action.");

  AXError error = AXUIElementPerformAction(_native, kAXDecrementAction);
  handleUnsupportedError(error, "decrement");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollLeft() {
  if (_role != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");

  // Seems there are no constants for these actions.
  // Ignore errors here (we already checked the role). On macOS 10.13 it reports an unsupported attribut error
  // which is weird, given that we perform an action.
  AXUIElementPerformAction(_native, CFSTR("AXScrollLeftByPage"));
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollRight() {
  if (_role != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");

  AXUIElementPerformAction(_native, CFSTR("AXScrollRightByPage"));
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollUp() {
  if (_role != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");

  AXUIElementPerformAction(_native, CFSTR("AXScrollUpByPage"));
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::scrollDown() {
  if (_role != Role::ScrollBox)
    throw std::runtime_error("Only scrollbox elements support this action.");

  AXUIElementPerformAction(_native, CFSTR("AXScrollDownByPage"));
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::increment() {
  if (_role != Role::Slider)
    throw std::runtime_error("Only slider elements support this action.");

  AXUIElementPerformAction(_native, kAXIncrementAction);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::decrement() {
  if (_role != Role::Slider)
    throw std::runtime_error("Only slider elements support this action.");

  AXError error = AXUIElementPerformAction(_native, kAXDecrementAction);
  handleUnsupportedError(error, "decrement");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::children(AccessibleList &result, bool recursive) const {
  if (_native) {
    auto children = getChildren(_native);
    if (children == nil)
      return;

    for (NSUInteger i = 0; i < children.count; ++i) {
      AXUIElementRef ref = (__bridge AXUIElementRef)(children[i]);
      Accessible *childAcc = new Accessible(ref);
      result.emplace_back(childAcc);

      if (recursive)
        childAcc->children(result, recursive);
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::children() const {
  AccessibleList result;
  if (_native) {
    auto children = getChildren(_native);
    if (children == nil)
      return result;

    for (NSUInteger i = 0; i < children.count; ++i) {
      AXUIElementRef ref = (__bridge AXUIElementRef)(children[i]);
      result.emplace_back(new Accessible(ref));
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::windows() const {
  AccessibleList result;
  if (_native) {
    CFArrayRef array;
    AXUIElementCopyAttributeValues(_native, kAXWindowsAttribute, 0, 99999, &array);

    if (array == nullptr)
      return result;

    NSArray *windows = (NSArray *)CFBridgingRelease(array);
    for (NSUInteger i = 0; i < windows.count; ++i) {
      AXUIElementRef ref = (__bridge AXUIElementRef)(windows[i]);
      result.emplace_back(new Accessible(ref));
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::tabPages() const {
  if (_role != Role::TabView)
    throw std::runtime_error("This element has no tabs.");

  AccessibleList result;
  if (_native) {
    CFTypeRef array;
    AXUIElementCopyAttributeValue(_native, kAXTabsAttribute, &array);

    if (array == nullptr)
      return result;

    NSArray *tabs = (NSArray *)CFBridgingRelease(array);
    for (NSUInteger i = 0; i < tabs.count; ++i) {
      AXUIElementRef ref = (__bridge AXUIElementRef)(tabs[i]);
      result.emplace_back(new Accessible(ref));

      // Need to set the role explicitly, as on macOS tabpages are in reality buttons.
      // There are no "physical" tabpages. They are simulated by the tab buttons.
      result.back()->_role = Role::TabPage;
    }
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::rows() const {
  if (_role != Role::TreeView && _role != Role::Grid)
    throw std::runtime_error("This element has no rows.");

  AccessibleList result;
  if (_native) {
    CFArrayRef rows;
    AXUIElementCopyAttributeValues(_native, kAXRowsAttribute, 0, 99999, &rows);

    if (rows == nullptr)
      return result;

    CFIndex i, c = CFArrayGetCount(rows);
    for (i = 0; i < c; ++i) {
      AXUIElementRef ref = static_cast<AXUIElementRef>(CFArrayGetValueAtIndex(rows, i));
      result.emplace_back(new Accessible(ref));
      result.back()->_role = Role::Row;
    }

    CFRelease(rows);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::rowEntries() const {
  if (_role != Role::Row)
    throw std::runtime_error("This element has no row entries.");

  AccessibleList result;
  if (_native) {
    auto children = getChildren(_native);
    if (children == nil)
      return result;

    // The first child entry is usually a group consisting of the disclosure triangle and the other content.
    // We only return that other content.
    if (children.count > 0) {
      AXUIElementRef first = (__bridge AXUIElementRef)(children[0]);
      if (nativeRoleIsOneOf(first, { kAXGroupRole })) {
        auto groupEntries = getChildren(first);
        for (NSUInteger i = 1; i < groupEntries.count; ++i) {
          AXUIElementRef entryRef = (__bridge AXUIElementRef)(groupEntries[i]);
          result.emplace_back(new Accessible(entryRef));
        }
      } else {
        result.emplace_back(new Accessible(first));
      }

      for (NSUInteger i = 1; i < children.count; ++i) {
        AXUIElementRef ref = (__bridge AXUIElementRef)(children[i]);
        result.emplace_back(new Accessible(ref));
      }
    }
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::columns() const {
  if (_role != Role::TreeView && _role != Role::Grid)
    throw std::runtime_error("This element has no columns.");
  
  AccessibleList result;
  if (_native) {
    CFArrayRef columns;
    AXUIElementCopyAttributeValues(_native, kAXColumnsAttribute, 0, 99999, &columns);

    if (columns == nullptr)
      return result;

    CFIndex i, c = CFArrayGetCount(columns);
    for (i = 0; i < c; ++i) {
      AXUIElementRef ref = static_cast<AXUIElementRef>(CFArrayGetValueAtIndex(columns, i));
      result.emplace_back(new Accessible(ref));
    }

    CFRelease(columns);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleList Accessible::columnEntries() const {
  if (_role != Role::Column)
    throw std::runtime_error("This element has no column entries.");

  AccessibleList result;
  if (_native) {
    CFArrayRef rows;
    AXUIElementCopyAttributeValues(_native, kAXRowsAttribute, 0, 99999, &rows);

    if (rows == nullptr)
      return result;

    CFIndex i, c = CFArrayGetCount(rows);
    for (i = 0; i < c; ++i) {
      AXUIElementRef ref = static_cast<AXUIElementRef>(CFArrayGetValueAtIndex(rows, i));
      if (nativeRoleIsOneOf(ref, { kAXGroupRole })) {
        auto groupEntries = getChildren(ref);
        for (NSUInteger j = 1; j < groupEntries.count; ++j) {
          AXUIElementRef entryRef = (__bridge AXUIElementRef)(groupEntries[j]);
          result.emplace_back(new Accessible(entryRef));
        }
      } else {
        result.emplace_back(new Accessible(ref));
      }
    }

    CFRelease(rows);
  }
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

AccessibleRef Accessible::fromPoint(geometry::Point point, Accessible *application) {
  AXUIElementRef element;
  if (AXUIElementCopyElementAtPosition(application->_native, point.x, point.y, &element) != kAXErrorSuccess)
    return nullptr;

  auto result = AccessibleRef(new Accessible(element));
  CFRelease(element);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

Role Accessible::determineRole(AXUIElementRef element) {
  static std::map<std::string, Role> roleMap = {
    { toString(kAXApplicationRole), Role::Application },
    { toString(kAXWindowRole), Role::Window },
    { toString(kAXButtonRole), Role::Button },
    { toString(kAXRadioButtonRole), Role::RadioButton },
    { toString(kAXRadioGroupRole), Role::RadioGroup },
    { toString(kAXCheckBoxRole), Role::CheckBox },
    { toString(kAXComboBoxRole), Role::ComboBox },
    { toString(kAXPopUpButtonRole), Role::ComboBox },
    { toString(kAXDisclosureTriangleRole), Role::Expander },
    { toString(kAXTableRole), Role::Grid },
    { toString(kAXTextFieldRole), Role::TextBox },
    { toString(kAXTextAreaRole), Role::TextBox },
    { toString(kAXOutlineRole), Role::TreeView },
    { toString(kAXStaticTextRole), Role::Label },
    { toString(kAXMenuRole), Role::Menu },
    { toString(kAXMenuBarRole), Role::MenuBar },
    { toString(kAXMenuBarItemRole), Role::MenuItem },
    { toString(kAXMenuItemRole), Role::MenuItem },
    { toString(kAXSplitGroupRole), Role::SplitContainer },
    { toString(kAXSplitterRole), Role::Splitter },
    { toString(kAXGroupRole), Role::GroupBox },
    { toString(kAXImageRole), Role::Image },
    { toString(kAXTabGroupRole), Role::TabView },
    { "AXDateTimeArea", Role::DatePicker }, // Can't find a constant for this role.
    { toString(kAXRowRole), Role::Row },
    { toString(kAXColumnRole), Role::Column },
    { toString(kAXCellRole), Role::Column },
    { toString(kAXScrollAreaRole), Role::ScrollBox },
    { toString(kAXSliderRole), Role::Slider },
    { toString(kAXIncrementorRole), Role::Stepper },
    { toString(kAXListRole), Role::List },
    { toString(kAXGridRole), Role::IconView },
    { toString(kAXProgressIndicatorRole), Role::ProgressIndicator },
    { toString(kAXBusyIndicatorRole), Role::BusyIndicator },
    { toString(kAXScrollBarRole), Role::ScrollBar },
    { toString(kAXValueIndicatorRole), Role::ScrollThumb },
    { "AXLink", Role::HyperLink },
  };

  // For certain elements we use the subrole to get a better role description.
  std::string subRoleString = getNativeRole(element, kAXSubroleAttribute);
  if (subRoleString == "AXTabButton")
    return Role::TabPage;

  std::string roleString = getNativeRole(element);
  if (roleMap.find(roleString) != roleMap.end())
    return roleMap[roleString];

  return Role::Unknown;
}

//----------------------------------------------------------------------------------------------------------------------

NSArray* Accessible::getChildren(AXUIElementRef ref, size_t count, bool visibleOnly) {
  auto attribute = kAXChildrenAttribute;
  if (visibleOnly)
    attribute = kAXVisibleChildrenAttribute;
  return getArrayValue(ref, attribute, count);
}

//----------------------------------------------------------------------------------------------------------------------

NSArray* Accessible::getArrayValue(AXUIElementRef ref, CFStringRef attribute, size_t count) {
  CFArrayRef result = nullptr;
  AXError error = AXUIElementCopyAttributeValues(ref, attribute, 0, static_cast<CFIndex>(count), &result);
  if (error != kAXErrorSuccess) {
    return nil;
  }

  return (NSArray *)CFBridgingRelease(result);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Accessible::getStringValue(AXUIElementRef ref, CFStringRef attribute,
                                       std::string const& attributeName, bool noThrow) {
  CFTypeRef result;
  AXError error = AXUIElementCopyAttributeValue(ref, attribute, &result);
  if (error == kAXErrorNoValue)
    return "";

  if (!noThrow)
    handleUnsupportedError(error, attributeName);

  if (result == nullptr || CFGetTypeID(result) != CFStringGetTypeID())
    return "";

  std::string text = toString(result);
  CFRelease(result);

  return text;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setStringValue(AXUIElementRef ref, CFStringRef attribute, std::string const& value,
                                std::string const& attributeName) {
  if (!isSettable(ref, attribute))
    throw std::runtime_error("Attribute cannot be set: " + attributeName);

  NSString *native = [NSString stringWithUTF8String: value.c_str()];
  AXError error = AXUIElementSetAttributeValue(ref, attribute, (__bridge CFStringRef)native);
  handleUnsupportedError(error, attributeName);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::getBoolValue(AXUIElementRef ref, CFStringRef attribute, std::string const& attributeName,
                              bool noThrow) {
  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(ref, attribute, &value);
  if (error == kAXErrorNoValue)
    return false;

  if (!noThrow)
    handleUnsupportedError(error, attributeName);
  else if (error != kAXErrorSuccess)
    return false;

  bool result = false;
  if (CFGetTypeID(value) == CFStringGetTypeID()) {
    NSString *s = (__bridge NSString *)value;
    result = s.boolValue;
  } else if (CFGetTypeID(value) == CFBooleanGetTypeID()) {
    result = value == kCFBooleanTrue;
  } if (CFGetTypeID(value) == CFNumberGetTypeID()) {
    NSNumber *n = (__bridge NSNumber *)value;
    result = n.boolValue;
  }

  CFRelease(value);

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setBoolValue(AXUIElementRef ref, CFStringRef attribute, bool value, std::string const& attributeName) {
  if (!isSettable(ref, attribute))
    throw std::runtime_error("Attribute cannot be set: " + attributeName);

  NSNumber *temp = [NSNumber numberWithBool: value];
  AXError error = AXUIElementSetAttributeValue(ref, attribute, (__bridge CFTypeRef)temp);
  handleUnsupportedError(error, attributeName);
}

//----------------------------------------------------------------------------------------------------------------------

NSNumber *Accessible::getNumberValue(AXUIElementRef ref, CFStringRef attribute, std::string const& attributeName,
                                     bool noThrow) {
  CFTypeRef result;
  AXError error = AXUIElementCopyAttributeValue(ref, attribute, &result);
  if (error == kAXErrorNoValue)
    return nil;

  if (!noThrow)
    handleUnsupportedError(error, attributeName);
  else if (error != kAXErrorSuccess)
    return nil;

  return (__bridge NSNumber *)result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setNumberValue(AXUIElementRef ref, CFStringRef attribute, NSNumber *value,
                                std::string const& attributeName) {
  if (!isSettable(ref, attribute))
    throw std::runtime_error("Attribute cannot be set: " + attributeName);

  AXError error = AXUIElementSetAttributeValue(ref, attribute, (__bridge CFTypeRef)value);
  handleUnsupportedError(error, attributeName);
}

//----------------------------------------------------------------------------------------------------------------------

AXUIElementRef Accessible::getElementValue(AXUIElementRef ref, bool noThrow) {
  CFTypeRef value;
  AXError error = AXUIElementCopyAttributeValue(ref, kAXValueAttribute, &value);
  if (error == kAXErrorNoValue)
    return nil;

  if (!noThrow)
    handleUnsupportedError(error, "value");
  else if (error != kAXErrorSuccess)
    return nil;

  return static_cast<AXUIElementRef>(value);
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::hasBounds(AXUIElementRef ref) {
  return isSupported(ref, kAXPositionAttribute) && isSupported(ref, kAXSizeAttribute);
}

//----------------------------------------------------------------------------------------------------------------------

Rectangle Accessible::getBounds(AXUIElementRef ref, bool screenCoordinates) {
  AXValueRef valueRef;
  CGSize size;
  CGPoint point;

  AXError error = AXUIElementCopyAttributeValue(ref, kAXPositionAttribute, (CFTypeRef *)&valueRef);
  handleUnsupportedError(error, "position");

  Rectangle result;
  if (error == kAXErrorNoValue) {
    return result;
  }

  AXValueGetValue(valueRef, static_cast<AXValueType>(kAXValueCGPointType), (void *)&point);
  CFRelease(valueRef);

  error = AXUIElementCopyAttributeValue(ref, kAXSizeAttribute, (CFTypeRef *)&valueRef);
  if (error != kAXErrorSuccess) {
    return Rectangle();
  }

  AXValueGetValue(valueRef, static_cast<AXValueType>(kAXValueCGSizeType), (void *)&size);
  CFRelease(valueRef);

  if (!screenCoordinates) {
    AXUIElementRef parent = getParent(ref);
    if (parent != nullptr && hasBounds(parent)) {
      Rectangle parentBounds;
      parentBounds = getBounds(parent, true);
      result = Rectangle(point.x - parentBounds.minX(), point.y - parentBounds.minY(), size.width, size.height);
    } else {
      result = Rectangle(point.x, point.y, size.width, size.height);
    }
  } else {
    result = Rectangle(point.x, point.y, size.width, size.height);
  }

  return result;
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::setBounds(AXUIElementRef ref, geometry::Rectangle const& bounds) {
  CGSize size = { static_cast<CGFloat>(bounds.size.width), static_cast<CGFloat>(bounds.size.height) };
  CGPoint point = { static_cast<CGFloat>(bounds.position.x), static_cast<CGFloat>(bounds.position.y) };

  AXValueRef valueRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGPointType), &point);
  AXError error = AXUIElementSetAttributeValue(ref, kAXPositionAttribute, valueRef);
  CFRelease(valueRef);
  handleUnsupportedError(error, "position");

  valueRef = AXValueCreate(static_cast<AXValueType>(kAXValueCGSizeType), &size);
  error = AXUIElementSetAttributeValue(ref, kAXSizeAttribute, valueRef);
  CFRelease(valueRef);
  handleUnsupportedError(error, "size");
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSupported(AXUIElementRef ref, CFStringRef attribute) {
  CFTypeRef result;
  AXError error = AXUIElementCopyAttributeValue(ref, attribute, &result);
  if (result != nullptr)
    CFRelease(result);

  return error != kAXErrorAttributeUnsupported;
}

//----------------------------------------------------------------------------------------------------------------------

bool Accessible::isSettable(AXUIElementRef ref, CFStringRef attribute) {
  Boolean settable;
  AXUIElementIsAttributeSettable(ref, attribute, &settable);

  return settable;
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns a one line description of the given value.
 */
std::string Accessible::valueDescription(AXValueRef value) {
  std::stringstream ss;

  switch (AXValueGetType(value)) {
    case kAXValueCGPointType: {
      CGPoint point;
      if (AXValueGetValue(value, kAXValueTypeCGPoint, &point)) {
        ss << "{ x: " << point.x << ", y: " << point.y << " }";
      }
      break;
    }
      
    case kAXValueCGSizeType: {
      CGSize size;
      if (AXValueGetValue(value, kAXValueTypeCGSize, &size)) {
        ss << "{ width: " << size.width << ", height: " << size.height << " }";
      }
      break;
    }

    case kAXValueCGRectType: {
      CGRect rect;
      if (AXValueGetValue(value, kAXValueTypeCGRect, &rect)) {
        ss << "{ x: " << rect.origin.x << ", y: " << rect.origin.y << ", width: "
          << rect.size.width << ", height: " << rect.size.height << " }";
      }
      break;
    }

    case kAXValueCFRangeType: {
      CFRange range;
      if (AXValueGetValue(value, kAXValueTypeCFRange, &range)) {
        ss << "{ location: " << range.location << ", length: " << range.length << " }";
      }
      break;
    }

    default:
      if (CFGetTypeID(value) == CFArrayGetTypeID()) {
        NSArray *array = (__bridge NSArray *)value;
        ss << array.count << (array.count == 1 ? " element" : " elements");
      } else if (CFGetTypeID(value) == AXUIElementGetTypeID()) {
        std::string name = getStringValue((AXUIElementRef)value, kAXDescriptionAttribute, "name", true);
        if (name.empty())
          name = getStringValue((AXUIElementRef)value, kAXTitleAttribute, "name", true);

        if (name.empty()) {
          ss << "<no name>";
        } else {
          ss << name;
        }

        Role role = determineRole((AXUIElementRef)value);
        ss << " (" << roleToFriendlyString(role) << ")";
      } else {
        ss << [[(__bridge id)value description] UTF8String];
      }

      break;
  }

  return ss.str();
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::handleUnsupportedError(AXError error, std::string const& attribute) {
  // The two exceptions here use a very short timeout and don't wait for completion (to avoid blocking MGA).
  // Hence they will also produce the cannot-complete error, but that's ok in those cases.
  if (error == kAXErrorCannotComplete && attribute != "context menu" && attribute != "press")
    throw std::runtime_error("Cannot complete the accessibility call. Probably lost connection to target app.");

  if (error == kAXErrorAttributeUnsupported)
    throw std::runtime_error("Unsupported attribute: " + attribute);
  if (error == kAXErrorActionUnsupported)
    throw std::runtime_error("Unsupported action: " + attribute);
  if (error == kAXErrorInvalidUIElement)
    throw std::runtime_error("The specified UI element is not valid");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::writeImageToFile(CGImageRef image, NSString *path) const {
  CFURLRef url = (__bridge CFURLRef)[NSURL fileURLWithPath: path isDirectory: NO];
  CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, kUTTypePNG, 1, nil);
  if (!destination)
    throw std::runtime_error("Can't create file: " + std::string(path.UTF8String));

  CGImageDestinationAddImage(destination, image, nil);

  if (!CGImageDestinationFinalize(destination)) {
    CFRelease(destination);
    throw std::runtime_error("Error during save file: " + std::string(path.UTF8String));
  }

  CFRelease(destination);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the first child element of the given parent. The caller must unref the result (if not null).
 */
AXUIElementRef Accessible::getFirstChild(AXUIElementRef parent) {
  auto children = getChildren(parent, 1);
  if (children == nil)
    return nullptr;

  return (__bridge_retained AXUIElementRef)(children[0]);
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Returns the parent of the given element. The caller must unref the result (if not null);
 */
AXUIElementRef Accessible::getParent(AXUIElementRef child) {
  CFTypeRef parent;
  AXError error = AXUIElementCopyAttributeValue(child, kAXParentAttribute, &parent);
  if (error != kAXErrorSuccess || parent == nullptr) {
    return nullptr;
  }

  return static_cast<AXUIElementRef>(parent);
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::press(AXUIElementRef element) {
  // Don't wait for the press action to complete (could open a modal window).
  AXUIElementSetMessagingTimeout(element, 0.1);
  AXError error = AXUIElementPerformAction(element, kAXPressAction);
  AXUIElementSetMessagingTimeout(element, 0); // Restore default.
  handleUnsupportedError(error, "press");
}

//----------------------------------------------------------------------------------------------------------------------

void Accessible::printInfo(AXUIElementRef element) {
  std::vector<std::string> parents;
  parents.push_back(valueDescription((AXValueRef)element));

  AXUIElementRef run = getParent(element);
  while (run != nullptr) {
    parents.insert(parents.begin(), valueDescription((AXValueRef)run));
    AXUIElementRef parent = getParent(run);
    CFRelease(run);
    run = parent;
  }

  for (auto &entry : parents) {
    std::cout << entry << std::endl;
  }

  CFArrayRef array;
  AXUIElementCopyAttributeNames(element, &array);
  if (array != nil) {
    std::cout << std::endl << "Attributes:" << std::endl;

    NSArray *names = (__bridge NSArray *)array;
    for (NSString *name in names) {
      Boolean settable;
      AXUIElementIsAttributeSettable(element, (CFStringRef)name, &settable);
      std::cout << "\t" << name.UTF8String << (settable ? " (R/W)" : " (R)") << ": ";

      CFTypeRef value;
      AXError error = AXUIElementCopyAttributeValue(element, (CFStringRef)name, &value);
      if (error == kAXErrorSuccess) {
        std::cout << valueDescription((AXValueRef)value);
      }
      std::cout << std::endl;
    }

    CFRelease(array);
  }

  AXUIElementCopyParameterizedAttributeNames(element, &array);
  if (array != nil) {
    NSArray *names = (__bridge NSArray *)array;
    if (names.count > 0) {
      std::cout << std::endl << "Parameterized Attributes:" << std::endl;
      for (NSString *name in names) {
        std::cout << "\t" << name.UTF8String;

        CFTypeRef value;
        AXError error = AXUIElementCopyAttributeValue(element, (CFStringRef)name, &value);
        if (error == kAXErrorSuccess) {
          std::cout  << ": "<< valueDescription((AXValueRef)value);
        }
        std::cout << std::endl;
      }
    }
    CFRelease(array);
  }

  AXUIElementCopyActionNames(element, &array);
  if (array != nil) {
    NSArray *names = (__bridge NSArray *)array;
    if (names.count > 0) {
      std::cout << std::endl << "Actions:" << std::endl;

      for (NSString *name in names) {
        CFStringRef value;
        AXUIElementCopyActionDescription(element, (CFStringRef)name, &value);
        std::cout << "\t" << name.UTF8String << " - " << toString(value) <<  std::endl;
      }
    }
    CFRelease(array);
  }

  std::cout << std::endl;
}

//----------------------------------------------------------------------------------------------------------------------
