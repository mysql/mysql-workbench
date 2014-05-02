//
//  MCPPUtilities.mm
//  MySQLWorkbench
//
//  Created by Alfredo Kojima on 2/Oct/08.
//  Copyright 2008 Sun Microsystems Inc. All rights reserved.
//

#import "MCPPUtilities.h"

void MShowCPPException(const std::exception &exc)
{
  if (NSRunAlertPanel(@"Unhandled Backend Exception",
                      @"It is advisable to save your work in a backup file and restart Workbench.\n"
                      "Exception Details:\n%s", 
                      @"Ignore", @"Abort", nil, exc.what()) == NSAlertAlternateReturn)
  {
//    abort();
    [NSApp terminate:nil];
  }
}


/** Fills a NSPopUpButton with the contents of the given vector of strings
 */
void MFillPopupButtonWithStrings(NSPopUpButton *popup, const std::vector<std::string> &items)
{
  [popup removeAllItems];
  NSMutableArray *array= [NSMutableArray arrayWithCapacity: items.size()];
  for (std::vector<std::string>::const_iterator iter= items.begin(); iter != items.end(); ++iter)
    [array addObject: [NSString stringWithCPPString:*iter]];
  [popup addItemsWithTitles: array];
}


NSArray *MArrayFromStringVector(const std::vector<std::string> &items)
{
  NSString **strings;
  NSArray *array;
  
  int j= 0;
  strings= new NSString*[items.size()];
  for (std::vector<std::string>::const_iterator i= items.begin(); i != items.end(); ++i, j++)
    strings[j]= [NSString stringWithCPPString: *i];
  array= [NSArray arrayWithObjects:strings count:j];
  delete []strings;
  
  return array;
}


NSArray *MArrayFromStringList(const std::list<std::string> &items)
{
  NSString **strings;
  NSArray *array;
  
  int j= 0;
  strings= new NSString*[items.size()];
  for (std::list<std::string>::const_iterator i= items.begin(); i != items.end(); ++i, j++)
    strings[j]= [NSString stringWithCPPString: *i];
  array= [NSArray arrayWithObjects:strings count:j];
  delete []strings;
  
  return array;
}
