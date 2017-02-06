/*
 * Copyright (c) 2009, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/ui_form.h"

#include "workbench/wb_context_ui.h"

typedef NS_ENUM(NSInteger, PropertyType) {
  PROPERTY_TYPE_UNDEFINED = 0,
  PROPERTY_TYPE_STRING = 1,
  PROPERTY_TYPE_BOOL = 2,
  PROPERTY_TYPE_COLOR = 3
};

@interface WBObjectPropertiesController : NSObject

- (void)updateForForm:(bec::UIForm *)form;

@end

// TODO: move to NSString extension file.
@interface NSString (WBExtensions)

@property(readonly, copy) NSString *stringBySplittingCamelCase;

@end
