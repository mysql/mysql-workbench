/*
 * Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "base/accessibility.h"

using namespace base;

//---------------------------------------------------------------------------------------------------------------------

Accessible::~Accessible() {
};

//---------------------------------------------------------------------------------------------------------------------

std::string Accessible::getAccessibilityTitle() {
  return "";
}

//---------------------------------------------------------------------------------------------------------------------

std::string Accessible::getAccessibilityDescription() {
  return "";
}

//---------------------------------------------------------------------------------------------------------------------

std::string Accessible::getAccessibilityValue() {
  return "";
}

//---------------------------------------------------------------------------------------------------------------------

int Accessible::getAccessibilityChildCount() {
  return 0;
}

//---------------------------------------------------------------------------------------------------------------------

Accessible* Accessible::getAccessibilityChild(int index) {
  return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------

base::Rect Accessible::getAccessibilityBounds() {
  return base::Rect();
}

//---------------------------------------------------------------------------------------------------------------------

Accessible* Accessible::accessibilityHitTest(int x, int y) {
  return nullptr;
}

//---------------------------------------------------------------------------------------------------------------------

std::string Accessible::getAccessibilityDefaultAction() {
  return "";
}

//---------------------------------------------------------------------------------------------------------------------

void Accessible::accessibilityDoDefaultAction() {
};

//---------------------------------------------------------------------------------------------------------------------
