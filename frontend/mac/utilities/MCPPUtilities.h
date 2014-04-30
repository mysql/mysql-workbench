//
//  MCPPUtilities.h
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 2/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#include <exception>
#include <string>
#include <vector>
#include <list>

#import "NSString_extras.h"
#import "NSArray_extras.h"
#import "NSColor_extras.h"

void MShowCPPException(const std::exception &exc);

void MFillPopupButtonWithStrings(NSPopUpButton *popup, const std::vector<std::string> &items);

NSArray *MArrayFromStringVector(const std::vector<std::string> &items);
NSArray *MArrayFromStringList(const std::list<std::string> &items);
