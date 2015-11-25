/* 
 * Copyright (c) 2010, 2015, Oracle and/or its affiliates. All rights reserved.
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

#import "MFBase.h"
#include <boost/function.hpp>

@interface MHudController : NSObject

@property (readonly, strong) NSPanel *hud;

+ (void)showHudWithTitle: (NSString*) title andDescription: (NSString*) description;
+ (BOOL)hideHud;

+ (BOOL)runModalHudWithTitle: (NSString*) title andDescription: (NSString*) description
                 notifyReady: (boost::function<void ()>)signalReady
                cancelAction: (boost::function<bool ()>)cancelAction;
+ (void)stopModalHud;

- (IBAction)cancelClicked:(id)sender;

- (instancetype)init NS_DESIGNATED_INITIALIZER;

- (void)showAnimatedWithFrame: (NSRect) frame title: (NSString*) title andDescription: (NSString*) description;
- (void)hideAnimated;

@end
