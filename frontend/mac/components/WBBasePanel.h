/*
 * Copyright (c) 2008, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "ui_form.h"

@interface WBBasePanel : NSObject {
  NSView *decoratorView;
}

@property(weak) IBOutlet NSView *topView;

@property(readonly) NSString *title;
@property(readonly) NSString *identifier;
@property(readonly) NSImage *tabIcon;
@property(readonly) bec::UIForm *formBE;
@property(readonly) NSSize minimumSize;
@property(readonly, strong) NSMenu *menuBar;
@property(readonly, strong) NSView *decoratedTopView;
@property(readonly) BOOL willClose;

- (void)didOpen;
- (void)didActivate;

@end
