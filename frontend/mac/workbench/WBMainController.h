/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#import <Cocoa/Cocoa.h>
#include <map>
#include <string>
#include <boost/shared_ptr.hpp>

namespace bec
{
  class UIForm;
};

namespace wb
{
  class WBContextUI;
  class WBContext;
  struct WBOptions;
};

@class WBMainWindow;
@class WBBasePanel;

typedef WBBasePanel *(*FormPanelFactory)(WBMainWindow *mainwin, boost::shared_ptr<bec::UIForm> form);


@interface WBMainController : NSObject <NSApplicationDelegate, NSFileManagerDelegate>
{
  wb::WBContextUI *_wbui;
  wb::WBContext *_wb;
  wb::WBOptions *_options;
  std::map<std::string, FormPanelFactory> *_formPanelFactories;
  
  NSMutableArray *_editorWindows;
  
  BOOL _initFinished;
  BOOL _showingUnhandledException;

  IBOutlet WBMainWindow *mainWindow;

  IBOutlet NSPanel *pageSetup;
  IBOutlet NSPopUpButton *paperSize;
  IBOutlet NSTextField *paperSizeLabel;
  IBOutlet NSButton *landscapeButton;
  IBOutlet NSButton *portraitButton;
  
  IBOutlet NSTextField *inputDialogMessage;
  IBOutlet NSTextField *inputDialogText;
  IBOutlet NSSecureTextField *inputDialogSecureText;
  IBOutlet NSPanel *inputDialog;
}

- (void)registerFormPanelFactory:(FormPanelFactory)fac forFormType:(const std::string&)type;

- (IBAction)menuItemClicked:(id)sender;
- (IBAction)showDiagramProperties:(id)sender;
- (void)requestRefresh;

- (IBAction)buttonClicked:(id)sender;

- (IBAction)inputDialogClose:(id)sender;
- (void) showPageSetup: (id)sender;
@end
