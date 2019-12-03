/*
 * Copyright (c) 2009, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "base/string_utilities.h"

#include "workbench/wb_context.h"

#import "WBMainController.h"
#import "MainWindowController.h"
#import "MCppUtilities.h"

// for _NSGetArg*
#include <crt_externs.h>

#import "WBPluginPanel.h"
#import "WBPluginEditorBase.h"
#import "WBPluginWindowBase.h"
#import "WBPluginWindowController.h"

#import "WBModelDiagramPanel.h"

#include "workbench/wb_context_ui.h"
#include "model/wb_context_model.h"
#include "grtui/gui_plugin_base.h"

#import "WBSQLQueryUI.h"

#import "WBMenuManager.h"
#import "WBDiagramSizeController.h"
#import "WBModelDiagramPanel.h"

#import "ScintillaView.h"

#include "mforms/toolbar.h"
#import "MFCodeEditor.h"

#include "workbench/wb_module.h"
#include "base/log.h"
#include "base/file_utilities.h"

#include "wb_command_ui.h"

static GThread *mainthread = 0;

DEFAULT_LOG_DOMAIN("Workbench")

@interface WBMainController () {
  wb::WBContext *_wb;
  wb::WBOptions *_options;
  std::map<std::string, FormPanelFactory> _formPanelFactories;

  NSMutableArray *_editorWindows;

  BOOL _initFinished;
  BOOL _showingUnhandledException;

  // Define a set of methods which backend can call to interact with user and frontend
  wb::WBFrontendCallbacks wbcallbacks;

  MainWindowController *mainController;
  NSMutableArray *pageSetupNibObjects;

  IBOutlet NSPanel *pageSetup;
  IBOutlet NSPopUpButton *paperSize;
  IBOutlet NSTextField *paperSizeLabel;
  IBOutlet NSButton *landscapeButton;
  IBOutlet NSButton *portraitButton;
  // We need to keep extra reference, so it's not released too early.
  std::shared_ptr<wb::WBContextUI> _wbContext;
}

@end

@implementation WBMainController

static std::string showFileDialog(const std::string &type, const std::string &title, const std::string &extensions) {
  NSMutableArray *fileTypes = [NSMutableArray array];
  std::vector<std::string> exts(base::split(extensions, ","));

  for (std::vector<std::string>::const_iterator iter = exts.begin(); iter != exts.end(); ++iter) {
    if (iter->find('|') != std::string::npos) {
      std::string ext = iter->substr(iter->find('|') + 1);
      ext = base::replaceString(ext, "*.", "");
      [fileTypes addObject:@(ext.c_str())];
    } else
      [fileTypes addObject:@(iter->c_str())];
  }

  if (type == "open") {
    NSOpenPanel *panel = [NSOpenPanel openPanel];

    panel.title = @(title.c_str());
    panel.allowedFileTypes = fileTypes;

    if ([panel runModal] == NSModalResponseOK)
      return (panel.URL.path).UTF8String;
  } else if (type == "save") {
    NSSavePanel *panel = [NSSavePanel savePanel];

    panel.title = @(title.c_str());

    panel.allowedFileTypes = fileTypes;

    if ([panel runModal] == NSModalResponseOK)
      return (panel.URL.path).UTF8String;
  }

  return "";
}

- (IBAction)inputDialogClose:(id)sender {
  [NSApp stopModalWithCode: [sender tag]];
}

static void windowShowStatusText(const std::string &text, MainWindowController *controller) {
  NSString *string = @(text.c_str());

  // setStatusText must release the param
  if ([NSThread isMainThread])
    controller.statusText = string;
  else
    [controller performSelectorOnMainThread: @selector(setStatusText:) withObject: string waitUntilDone: NO];
}

static NativeHandle windowOpenPlugin(grt::Module *ownerModule, const std::string &shlib, const std::string &class_name,
                                     const grt::BaseListRef &args, bec::GUIPluginFlags flags,
                                     MainWindowController *controller) {
  std::string path = ownerModule->path();

  // Check if this is a bundle plugin or a plain dylib plugin
  if (g_str_has_suffix(shlib.c_str(), ".mwbplugin")) {
    NSBundle *pluginBundle;
    NSString *bundlePath;

    // For bundled plugins, we load it, find the requested class and instantiate it.

    // determine the path for the plugin bundle by stripping Contents/Framework/dylibname
    bundlePath = [NSString stringWithCPPString: path]
      .stringByDeletingLastPathComponent.stringByDeletingLastPathComponent.stringByDeletingLastPathComponent;

    logDebug("Opening plugin bundle %s\n", shlib.c_str());

    pluginBundle = [NSBundle bundleWithPath: bundlePath];
    if (pluginBundle == nil) {
      logError("Plugin bundle %s could not be found\n", bundlePath.UTF8String);
      NSAlert *alert = [NSAlert new];
      alert.messageText = @"Missing Plugin";
      alert.informativeText = [NSString stringWithFormat: @"The plugin %s could not be found.", shlib.c_str()];
      alert.alertStyle = NSAlertStyleCritical;
      [alert addButtonWithTitle: @"Close"];
      [alert runModal];

      return 0;
    }

    if (!pluginBundle.isLoaded) {
      NSError *error = nil;
      [pluginBundle loadAndReturnError: &error];
      if (error != nil) {
        NSString *s = [NSString stringWithFormat: @"%@", error];
        logError("Error loading bundle: %s\n", s.UTF8String);

        NSAlert *alert = [NSAlert new];
        alert.messageText = @"Error Loading Plugin";
        alert.informativeText = [NSString stringWithFormat: @"The plugin %s could not be loaded. See log file for details", shlib.c_str()];
        alert.alertStyle = NSAlertStyleCritical;
        [alert addButtonWithTitle: @"Close"];
        [alert runModal];

        return 0;
      }
    }

    Class pclass = [pluginBundle classNamed: @(class_name.c_str())];
    if (!pclass) {
      logError("Plugin class %s was not found in bundle %s\n", class_name.c_str(), bundlePath.UTF8String);

      NSAlert *alert = [NSAlert new];
      alert.messageText = @"Error Opening Plugin";
      alert.informativeText = [NSString
        stringWithFormat: @"The plugin %s does not contain the published object %s", shlib.c_str(), class_name.c_str()];
      alert.alertStyle = NSAlertStyleCritical;
      [alert addButtonWithTitle: @"Close"];
      [alert runModal];

      return 0;
    }

    if ((flags & bec::StandaloneWindowFlag)) {
      // create a window for the panel further down
    } else if (!(flags & bec::ForceNewWindowFlag)) {
      // Check if there is already a panel with an editor for this plugin type.
      id existingPanel = [controller findPanelForPluginType: [pclass class]];
      if (existingPanel != nil) {
        // check if it can be closed
        if ([existingPanel respondsToSelector: @selector(willClose)] && ![existingPanel willClose]) {
          flags = (bec::GUIPluginFlags)(flags | bec::ForceNewWindowFlag);
        } else {
          // drop the old plugin->handle mapping
          bec::GRTManager::get()->get_plugin_manager()->forget_gui_plugin_handle((__bridge NativeHandle)existingPanel);

          if ([existingPanel respondsToSelector: @selector(pluginEditor)]) {
            id editor = [existingPanel pluginEditor];

            if ([editor respondsToSelector: @selector(reinitWithArguments:)]) {
              id oldIdentifier = [editor panelId];

              [controller closeBottomPanelWithIdentifier: oldIdentifier];
              [editor reinitWithArguments: args];
              [controller addBottomPanel: existingPanel];
            }
          }
          return (__bridge NativeHandle)existingPanel;
        }
      }
    }

    // Instantiate and initialize the plugin.
    id plugin = [[pclass alloc] initWithModule: ownerModule arguments:args];

    if ([plugin isKindOfClass: [WBPluginEditorBase class]]) {
      if ((flags & bec::StandaloneWindowFlag)) {
        WBPluginWindowController *editor = [[WBPluginWindowController alloc] initWithPlugin: plugin];

        [controller.owner->_editorWindows addObject: editor];

        return (__bridge NativeHandle)editor;
      } else {
        WBPluginPanel *panel = [[WBPluginPanel alloc] initWithPlugin: plugin];
        [controller addBottomPanel: panel];

        return (__bridge NativeHandle)panel;
      }
    } else if ([plugin isKindOfClass: WBPluginWindowBase.class]) {
      [plugin showModal];
      return nil;
    } else {
      logWarning("Plugin %s is of unknown type\n", [[plugin className] UTF8String]);
      return nil;
    }
  } else {
    logWarning("open_plugin() called for an unknown plugin type\n");
    return 0;
  }
}

static void windowShowPlugin(NativeHandle handle, MainWindowController *controller) {
  id plugin = (__bridge id)handle;

  if ([plugin respondsToSelector: @selector(setHidden:)])
    [plugin setHidden: NO];
  else if ([plugin respondsToSelector: @selector(topView)])
    [controller reopenEditor: plugin];
}

static void windowHidePlugin(NativeHandle handle, MainWindowController *controller) {
  id plugin = (__bridge id)handle;

  if ([plugin respondsToSelector: @selector(setHidden:)])
    [plugin setHidden: YES];
}

static void windowPerformCommand(const std::string &command, MainWindowController *controller, WBMainController *main) {
  if (command == "reset_layout")
    [controller resetWindowLayout];
  else if (command == "overview.mysql_model")
    [controller showMySQLOverview: nil];
  else if (command == "diagram_size")
    [main showDiagramProperties: nil];
  else if (command == "wb.page_setup")
    [main showPageSetup: nil];
  else
    [controller forwardCommandToPanels: command];
}

static mdc::CanvasView *windowCreateView(const model_DiagramRef &diagram, MainWindowController *controller) {
  return [controller createView: diagram.id().c_str() name: diagram->name().c_str()];
}

static void windowDestroyView(mdc::CanvasView *view, MainWindowController *controller) {
  [controller destroyView:view];
}

static void windowSwitchedView(mdc::CanvasView *cview, MainWindowController *controller) {
  [controller switchToDiagramWithIdentifier: cview->get_tag().c_str()];
}

static void windowCreateMainFormView(const std::string &type, std::shared_ptr<bec::UIForm> form, WBMainController *main,
                                     MainWindowController *controller) {
  if (main->_formPanelFactories.find(type) == main->_formPanelFactories.end()) {
    throw std::logic_error("Form type " + type + " not supported by frontend");
  } else {
    WBBasePanel *panel = main->_formPanelFactories[type](controller, form);

    [controller addTopPanelAndSwitch: panel];
  }
}

static void windowDestroyMainFormView(bec::UIForm *form, MainWindowController *controller) {
}

static void windowToolChanged(mdc::CanvasView *canvas, MainWindowController *controller) {
  if ([controller.selectedMainPanel isKindOfClass: [WBModelDiagramPanel class]]) {
    [(WBModelDiagramPanel *)controller.selectedMainPanel canvasToolChanged: canvas];
  }
}

static void windowRefreshGui(wb::RefreshType type, const std::string &arg1, NativeHandle arg2,
                             MainWindowController *controller) {
  [controller refreshGUI: type argument1: arg1 argument2: arg2];
}

static void windowLockGui(bool lock, MainWindowController *controller) {
  [controller blockGUI: lock];
}

static bool quitApplication(MainWindowController *controller) {
  if (!controller.closeAllPanels)
    return false;

  [NSApp terminate: nil];

  return true;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
  logInfo("Shutting down Workbench\n");

  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  [[NSNotificationCenter defaultCenter] removeObserver: self];

  wb::WBContextUI::get()->get_wb()->finalize();
  logInfo("Workbench shutdown done\n");
}

static void call_copy() {
  if (![NSApp.keyWindow.firstResponder tryToPerform: @selector(copy:) with: nil])
    if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_copy())
      wb::WBContextUI::get()->get_active_form()->copy();
}

static void call_cut() {
  if (![NSApp.keyWindow.firstResponder tryToPerform: @selector(cut:) with: nil])
    if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_cut())
      wb::WBContextUI::get()->get_active_form()->cut();
}

static void call_paste() {
  if (![NSApp.keyWindow.firstResponder tryToPerform: @selector(paste:) with: nil])
    if (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_paste())
      wb::WBContextUI::get()->get_active_form()->paste();
}

static void call_select_all() {
  if (![NSApp.keyWindow.firstResponder tryToPerform: @selector(selectAll:) with: nil])
    if (wb::WBContextUI::get()->get_active_form())
      wb::WBContextUI::get()->get_active_form()->select_all();
}

static void call_delete() {
  id responder = NSApp.keyWindow.firstResponder;

  if (![responder tryToPerform: @selector(delete:) with: nil])
    if (![responder tryToPerform: @selector(deleteBackward:) with: nil])
      if (wb::WBContextUI::get()->get_active_form())
        wb::WBContextUI::get()->get_active_form()->delete_selection();
}

//--------------------------------------------------------------------------------------------------

static bool validate_copy() {
  if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(selectedRange)]) {
    NSRange textRange = [(id)NSApp.keyWindow.firstResponder selectedRange];
    return textRange.length > 0;
  }
  if (/*[[[NSApp keyWindow] firstResponder] isKindOfClass: [NSTableView class]]
      &&*/ [NSApp.keyWindow.firstResponder respondsToSelector: @selector(copy:)])
    return true; //[[(NSTableView*)[[NSApp keyWindow] firstResponder] selectedRowIndexes] count] > 0;

  return (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_copy());
}

//--------------------------------------------------------------------------------------------------

static bool validate_cut() {
  if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(selectedRange)]) {
    NSRange textRange = [(id)NSApp.keyWindow.firstResponder selectedRange];
    if (textRange.length > 0) {
      if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(isEditable)])
        return [(id)NSApp.keyWindow.firstResponder isEditable];
      return true;
    }
    return false;
  } else if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(cut:)])
    return true;

  return (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_cut());
}

//--------------------------------------------------------------------------------------------------

static bool validate_paste() {
  if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(isEditable)]) {
    // Two conditions must be met if target can be considered as pastable.
    // 1) The target is editable.
    BOOL isEditable = [(id)NSApp.keyWindow.firstResponder isEditable];

    // 2) The pasteboard contains text.
    NSArray *supportedTypes = @[ NSPasteboardTypeString ];
    NSString *bestType = [[NSPasteboard generalPasteboard] availableTypeFromArray: supportedTypes];

    return isEditable && (bestType != nil);
  } else if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(paste:)])
    return true;

  return (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_paste());
}

//--------------------------------------------------------------------------------------------------

static bool validate_select_all() {
  if ([NSApp.keyWindow.firstResponder respondsToSelector: @selector(isSelectable)])
    return [(id)NSApp.keyWindow.firstResponder isSelectable];

  return (wb::WBContextUI::get()->get_active_form());
}

//--------------------------------------------------------------------------------------------------

static bool validate_delete() {
  NSResponder *responder = NSApp.keyWindow.firstResponder;
  SEL selector = NSSelectorFromString(@"canDeleteItem:");
  if ([responder respondsToSelector: selector]) {
    #pragma clang diagnostic ignored "-Warc-performSelector-leaks"
    return [responder performSelector: selector withObject: nil];
  }

  if ([responder respondsToSelector: @selector(selectedRange)]) {
    NSRange textRange = [(id)NSApp.keyWindow.firstResponder selectedRange];
    return textRange.length > 0;
  }

  return (wb::WBContextUI::get()->get_active_form() && wb::WBContextUI::get()->get_active_form()->can_delete());
}

//--------------------------------------------------------------------------------------------------

// XXX deprecated, remove it eventually
static void call_closetab_old(MainWindowController *controller) {
  if (controller.window.isKeyWindow) {
    id activePanel = controller.activePanel;
    bec::UIForm *form = [activePanel formBE];
    if (form && form->get_form_context_name() == "home")
      return;

    if (![activePanel respondsToSelector: @selector(closeActiveEditorTab)] || ![activePanel closeActiveEditorTab])
      [controller closePanel: activePanel];
  }
}

static void call_close_tab(MainWindowController *controller) {
  if (controller.window.isKeyWindow) {
    id activePanel = controller.activePanel;
    bec::UIForm *form = [activePanel formBE];
    if (form && form->get_form_context_name() == "home")
      return;

    [controller closePanel: activePanel];
  }
}

static void call_close_editor(MainWindowController *controller) {
  if (controller.window.isKeyWindow) {
    id activePanel = controller.activePanel;
    bec::UIForm *form = [activePanel formBE];
    if (form && form->get_form_context_name() == "home")
      return;

    [activePanel closeActiveEditorTab];
  }
}

//--------------------------------------------------------------------------------------------------

static bool validate_closetab_old(MainWindowController *controller) {
  WBBasePanel *activePanel = controller.activePanel;
  bec::UIForm *form = activePanel.formBE;
  if (form && form->get_form_context_name() == "home")
    return false;

  // find where this belongs to
  return activePanel != nil && controller.window.isKeyWindow;
}

static bool validate_close_tab(MainWindowController *controller) {
  WBBasePanel *activePanel = controller.activePanel;
  bec::UIForm *form = activePanel.formBE;
  if (form && form->get_form_context_name() == "home")
    return false;
  return activePanel != nil && controller.window.isKeyWindow;
}

static bool validate_close_editor(MainWindowController *controller) {
  WBBasePanel *activePanel = controller.activePanel;
  bec::UIForm *form = activePanel.formBE;
  if (form && form->get_form_context_name() == "home")
    return false;

  return activePanel != nil && controller.window.isKeyWindow &&
         [activePanel respondsToSelector: @selector(closeActiveEditorTab)];
}

//--------------------------------------------------------------------------------------------------

static void call_toggle_fullscreen(MainWindowController *controller) {
  [controller.window toggleFullScreen: controller.window];
}

//--------------------------------------------------------------------------------------------------

static bool validate_toggle_fullscreen(MainWindowController *controller) {
  return NSAppKitVersionNumber > NSAppKitVersionNumber10_6;
}

//--------------------------------------------------------------------------------------------------

static bool validate_find_replace() {
  id firstResponder = NSApp.keyWindow.firstResponder;
  if ([firstResponder isKindOfClass: [ScintillaView class]] || [firstResponder isKindOfClass: [NSTextView class]] ||
      [firstResponder isKindOfClass: [SCIContentView class]])
    return true;
  return false;
}

//--------------------------------------------------------------------------------------------------

static void call_find_replace(bool do_replace) {
  if (validate_find_replace()) {
    id firstResponder = NSApp.keyWindow.firstResponder;
    if ([firstResponder isKindOfClass: [SCIContentView class]]) {
      while (firstResponder && ![firstResponder isKindOfClass: [ScintillaView class]])
        firstResponder = [firstResponder superview];
    }
    if ([firstResponder isKindOfClass: [MFCodeEditor class]])
      [firstResponder showFindPanel: do_replace];
  } else
    NSBeep();
}

//--------------------------------------------------------------------------------------------------

static void call_find(MainWindowController *controller) {
  id firstResponder = NSApp.keyWindow.firstResponder;
  if ([firstResponder isKindOfClass: [SCIContentView class]]) {
    while (firstResponder && ![firstResponder isKindOfClass: [ScintillaView class]])
      firstResponder = [firstResponder superview];
  }
  if ([firstResponder isKindOfClass: [MFCodeEditor class]])
    [firstResponder showFindPanel:NO];
  else
    [controller performSearchObject: nil];
}

static bool validate_find(MainWindowController *controller) {
  bec::UIForm *form = wb::WBContextUI::get()->get_active_main_form();
  if (form && form->get_toolbar() && form->get_toolbar()->find_item("find"))
    return true;
  return validate_find_replace();
}

//--------------------------------------------------------------------------------------------------

static void call_undo(MainWindowController *controller) {
  id firstResponder = NSApp.keyWindow.firstResponder;

  SEL selector = NSSelectorFromString(@"undo:");
  if ([firstResponder respondsToSelector: selector])
    [firstResponder tryToPerform: selector with: nil];
  else if ([firstResponder isKindOfClass: [NSTextView class]])
    return [[firstResponder undoManager] undo];
  else if (wb::WBContextUI::get()->get_active_main_form())
    wb::WBContextUI::get()->get_active_main_form()->undo();
}

static bool validate_undo(MainWindowController *controller) {
  id firstResponder = NSApp.keyWindow.firstResponder;

  if ([firstResponder respondsToSelector: @selector(canUndo)])
    return [firstResponder canUndo];
  else if ([firstResponder isKindOfClass: [NSTextView class]])
    return [firstResponder undoManager].canUndo;
  else if ([firstResponder isKindOfClass: [SCIContentView class]])
    return true;
  else if (wb::WBContextUI::get()->get_active_main_form())
    return wb::WBContextUI::get()->get_active_main_form()->can_undo();
  return false;
}

static void call_redo(MainWindowController *controller) {
  id firstResponder = NSApp.keyWindow.firstResponder;

  SEL selector = NSSelectorFromString(@"redo:");
  if ([firstResponder respondsToSelector: selector])
    [firstResponder tryToPerform: selector with: nil];
  else if ([firstResponder isKindOfClass: [NSTextView class]])
    return [[firstResponder undoManager] redo];
  else if (wb::WBContextUI::get()->get_active_main_form())
    wb::WBContextUI::get()->get_active_main_form()->redo();
}

static bool validate_redo(MainWindowController *controller) {
  id firstResponder = NSApp.keyWindow.firstResponder;

  if ([firstResponder respondsToSelector: @selector(canRedo)])
    return [firstResponder canRedo];
  else if ([firstResponder isKindOfClass: [NSTextView class]])
    return [firstResponder undoManager].canRedo;
  else if ([firstResponder isKindOfClass: [SCIContentView class]])
    return true;
  else if (wb::WBContextUI::get()->get_active_main_form())
    return wb::WBContextUI::get()->get_active_main_form()->can_redo();
  return false;
}

- (void)textSelectionChanged:(NSNotification *)notification {
  id firstResponder = NSApp.keyWindow.firstResponder;

  if (notification.object == firstResponder ||
      ([firstResponder respondsToSelector: @selector(superview)] && notification.object == [firstResponder superview])) {
    // refresh edit menu
    wb::WBContextUI::get()->get_command_ui()->revalidate_edit_menu_items();
  }
}

//--------------------------------------------------------------------------------------------------

- (void)registerCommandsWithBackend {
  std::list<std::string> commands;

  commands.push_back("overview.mysql_model");
  commands.push_back("diagram_size");
  commands.push_back("wb.page_setup");

  commands.push_back("wb.toggleSidebar");
  commands.push_back("wb.toggleSecondarySidebar");
  commands.push_back("wb.toggleOutputArea");

  commands.push_back("wb.next_tab");
  commands.push_back("wb.back_tab");
  commands.push_back("wb.next_query_tab");
  commands.push_back("wb.back_query_tab");

  auto commandUI = wb::WBContextUI::get()->get_command_ui();
  commandUI->add_frontend_commands(commands);

  commandUI->add_builtin_command(
    "closetab", std::bind(call_closetab_old, mainController), std::bind(validate_closetab_old, mainController));
  commandUI->add_builtin_command("close_tab", std::bind(call_close_tab, mainController),
                                                                std::bind(validate_close_tab, mainController));
  commandUI->add_builtin_command(
    "close_editor", std::bind(call_close_editor, mainController), std::bind(validate_close_editor, mainController));

  commandUI->add_builtin_command("toggle_fullscreen",
                                                                std::bind(call_toggle_fullscreen, mainController),
                                                                std::bind(validate_toggle_fullscreen, mainController));

  commandUI->add_builtin_command("find", std::bind(call_find, mainController),
                                                                std::bind(validate_find, mainController));
  commandUI->add_builtin_command("find_replace", std::bind(call_find_replace, true),
                                                                std::bind(validate_find_replace));

  commandUI->add_builtin_command("undo", std::bind(call_undo, mainController),
                                                                std::bind(validate_undo, mainController));
  commandUI->add_builtin_command("redo", std::bind(call_redo, mainController),
                                                                std::bind(validate_redo, mainController));
  commandUI->add_builtin_command("copy", std::bind(call_copy), std::bind(validate_copy));
  commandUI->add_builtin_command("cut", std::bind(call_cut), std::bind(validate_cut));
  commandUI->add_builtin_command("paste", std::bind(call_paste),
                                                                std::bind(validate_paste));
  commandUI->add_builtin_command("delete", std::bind(call_delete), std::bind(validate_delete));
  commandUI->add_builtin_command("selectAll", std::bind(call_select_all), std::bind(validate_select_all));
}

static void flush_main_thread() {
  // flush stuff that could be called with performSelectorOnMainThread:
  [[NSRunLoop currentRunLoop] acceptInputForMode: NSDefaultRunLoopMode beforeDate: [NSDate date]];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
  std::string path = filename.fileSystemRepresentation;
  if (_initFinished) {
    if (_wb->open_file_by_extension(path, true))
      return YES;
  } else {
    _options->open_at_startup = path;
    return YES;
  }
  return NO;
}

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
  wb::WBContextUI::get()->init_finish(_options);

  _initFinished = YES;
  [[NSNotificationCenter defaultCenter] addObserver: self
                                           selector: @selector(windowDidBecomeKey:)
                                               name: NSWindowDidBecomeKeyNotification
                                             object: nil];
}

#include "mforms/../cocoa/MFMenuBar.h"

- (void)windowDidBecomeKey:(NSNotification *)notification {
  if (notification.object != mainController.window) {
    cf_swap_edit_menu();
  } else {
    cf_unswap_edit_menu();
    bec::UIForm *form = _wb->get_active_main_form();
    if (form && form->get_menubar())
      form->get_menubar()->validate();
  }
}

- (void)applicationDidBecomeActive:(NSNotification *)notification {
  base::NotificationInfo info;
  base::NotificationCenter::get()->send("GNApplicationActivated", nullptr, info);
}

- (void)applicationDidResignActive:(NSNotification *)notification {
  base::NotificationInfo info;
  base::NotificationCenter::get()->send("GNApplicationDeactivated", nullptr, info);
}

static NSString *applicationSupportFolder() {
  NSArray *res = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);

  if (res.count > 0)
    return res[0];
  return @"/tmp/";
}

- (void)setupBackend {
  mainthread = g_thread_self();

  try {
    // Setup backend stuff
    _wb = wb::WBContextUI::get()->get_wb();

    bec::GRTManager::get()->get_dispatcher()->set_main_thread_flush_and_wait(flush_main_thread);

    mainController.owner = self;
    [mainController setup];

    // Assign those callback methods
    wbcallbacks.show_file_dialog =
      std::bind(showFileDialog, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
    wbcallbacks.show_status_text = std::bind(windowShowStatusText, std::placeholders::_1, mainController);
    wbcallbacks.open_editor =
      std::bind(windowOpenPlugin, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                std::placeholders::_4, std::placeholders::_5, mainController);
    wbcallbacks.show_editor = std::bind(windowShowPlugin, std::placeholders::_1, mainController);
    wbcallbacks.hide_editor = std::bind(windowHidePlugin, std::placeholders::_1, mainController);
    wbcallbacks.perform_command = std::bind(windowPerformCommand, std::placeholders::_1, mainController, self);
    wbcallbacks.create_diagram = std::bind(windowCreateView, std::placeholders::_1, mainController);
    wbcallbacks.destroy_view = std::bind(windowDestroyView, std::placeholders::_1, mainController);
    wbcallbacks.switched_view = std::bind(windowSwitchedView, std::placeholders::_1, mainController);
    wbcallbacks.create_main_form_view =
      std::bind(windowCreateMainFormView, std::placeholders::_1, std::placeholders::_2, self, mainController);
    wbcallbacks.destroy_main_form_view = std::bind(windowDestroyMainFormView, std::placeholders::_1, mainController);
    wbcallbacks.tool_changed = std::bind(windowToolChanged, std::placeholders::_1, mainController);
    wbcallbacks.refresh_gui =
      std::bind(windowRefreshGui, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, mainController);
    wbcallbacks.lock_gui = std::bind(windowLockGui, std::placeholders::_1, mainController);
    wbcallbacks.quit_application = std::bind(quitApplication, mainController);

    // Add shipped python module search path to PYTHONPATH.
    {
      char *path = getenv("PYTHONPATH");
      if (path) {
        path = g_strdup_printf("PYTHONPATH=%s:%s", path, _options->library_search_path.c_str());
        putenv(path); // path should not be freed
      } else {
        path = g_strdup_printf("PYTHONPATH=%s", _options->library_search_path.c_str());
        putenv(path); // path should not be freed
      }
    }

    wb::WBContextUI::get()->init(&wbcallbacks, _options);

    _wb->flush_idle_tasks(false);
  } catch (std::exception &exc) {
    MShowCPPException(exc);
  }
}

//--------------------------------------------------------------------------------------------------

/**
 * Instantiates the option class for the backend and parses the command line.
 */
- (void)setupOptionsAndParseCommandline {
  int argc = *_NSGetArgc();
  char **argv = *_NSGetArgv();
  _options = new wb::WBOptions(argv[0]);

  _options->basedir = [NSBundle mainBundle].resourcePath.fileSystemRepresentation;
  _options->struct_search_path = _options->basedir + "/grt";
  _options->plugin_search_path = std::string([NSBundle mainBundle].builtInPlugInsPath.fileSystemRepresentation);
  _options->module_search_path = std::string([NSBundle mainBundle].builtInPlugInsPath.fileSystemRepresentation) + ":" +
                                 std::string([NSBundle mainBundle].resourcePath.fileSystemRepresentation) + "/plugins";
  _options->library_search_path =
    std::string([NSBundle mainBundle].resourcePath.fileSystemRepresentation) + "/libraries";
  _options->cdbc_driver_search_path = std::string([NSBundle mainBundle].privateFrameworksPath.fileSystemRepresentation);

  try {
    int rc = 0;
    if (!_options->programOptions->parse(std::vector<std::string>(argv + 1, argv + argc), rc)) {
      logInfo("Exiting with rc %i after parsing arguments\n", rc);
      exit(rc);
    }
    _options->analyzeCommandLineArguments();
  } catch (std::exception &exc) {
    logInfo("Exiting with error message: %s\n", exc.what());
    exit(1);
  }

  if (!_options->user_data_dir.empty()) {
    _options->user_data_dir =
      [NSString stringWithCPPString:_options->user_data_dir].stringByExpandingTildeInPath.UTF8String;
    if (!base::is_directory(_options->user_data_dir)) {
      try {
        if (!base::copyDirectoryRecursive(
              [applicationSupportFolder() stringByAppendingString:@"/MySQL/Workbench"].fileSystemRepresentation,
              _options->user_data_dir)) {
          logError("Unable to prepare new config directory: %s\n", _options->user_data_dir.c_str());
          exit(1);
        }
      } catch (std::exception &exc) {
        logError("There was a problem preparing new config directory. The error was: %s\n", exc.what());
        exit(1);
      }
    }
  } else
    _options->user_data_dir =
      [applicationSupportFolder() stringByAppendingString:@"/MySQL/Workbench"].fileSystemRepresentation;

  // no dock icon when the app will quit when finished running script
  if (_options->quit_when_done)
    [NSApp setActivationPolicy:NSApplicationActivationPolicyAccessory];
}

//--------------------------------------------------------------------------------------------------

extern "C" {
  extern void mforms_cocoa_init();
  extern void mforms_cocoa_check();
};

static void init_mforms() {
  logDebug("Initializing mforms\n");

  extern void cf_record_grid_init();
  static BOOL inited = NO;

  if (!inited) {
    inited = YES;

    mforms_cocoa_init();
    cf_record_grid_init();
  }
}

- (instancetype)init {
  self = [super init];
  if (self != nil) {
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(textSelectionChanged:)
                                                 name: NSTextViewDidChangeSelectionNotification
                                               object: nil];

    _editorWindows = [NSMutableArray array];
  }
  return self;
}

- (void)registerFormPanelFactory:(FormPanelFactory)fac forFormType:(const std::string &)type {
  _formPanelFactories[type] = fac;
}

- (void)awakeFromNib {
  // Since we use this controller to load multiple xibs the awakeFromNib function is called each time of such a load
  // (include the own loading). We can use the mainWindow as indicator (which is set up here).
  // TODO: refactor out classes with own xib files into own controller classes and use them here instead.
  if (mainController == nil) {
    // Prepare the logger to be ready as first part.
    base::Logger([applicationSupportFolder() stringByAppendingString:@"/MySQL/Workbench"].fileSystemRepresentation);
    logInfo("Starting up Workbench\n");

    [self setupOptionsAndParseCommandline];

    init_mforms();

    NSApplication.sharedApplication.delegate = self;

    mainController = [MainWindowController new];

    [self setupBackend];

    NSTimer *timer =
      [NSTimer scheduledTimerWithTimeInterval: 0.5 target: self selector: @selector(idleTasks:) userInfo: nil repeats:YES];
    [[NSRunLoop mainRunLoop] addTimer: timer forMode:NSModalPanelRunLoopMode];

    NSToolbar *toolbar = mainController.window.toolbar;
    [toolbar setShowsBaselineSeparator:NO];

    [self registerCommandsWithBackend];

    setupSQLQueryUI(self, mainController);

    // don't show the main window if we'll quit after running a script
    if ((_options->quit_when_done && !_options->run_at_startup.empty()))
      [mainController.window orderOut: nil];
    else
      [mainController showWindow: nil];

    // do the final setup for after the window is shown
    [mainController setupReady];

    mforms_cocoa_check();

    // XXX hack to work-around problem with opening object editors
    {
      NSString *pluginsPath = [NSBundle mainBundle].builtInPlugInsPath;
      NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:pluginsPath];
      NSString *file;

      while ((file = [dirEnum nextObject])) {
        if ([file.pathExtension isEqualToString:@"mwbplugin"]) {
          NSString *path = [pluginsPath stringByAppendingPathComponent:file];

          NSBundle *bundle = [NSBundle bundleWithPath:path];
          [bundle load];
        }
      }
    }
    _wbContext = wb::WBContextUI::get();
  }
}

- (void)flushIdleTasks:(id)arg {
  if (!_showingUnhandledException) {
    _showingUnhandledException = YES;
    try {
      _wb->flush_idle_tasks(false);
    } catch (const std::exception &exc) {
      NSAlert *alert = [NSAlert new];
      alert.messageText = @"Unhandled Exception";
      alert.informativeText = [NSString stringWithFormat:@"An unhandled exception has occurred: %s", exc.what()];
      alert.alertStyle = NSAlertStyleCritical;
      [alert addButtonWithTitle:@"Close"];
      [alert runModal];
    }
    _showingUnhandledException = NO;
  }
}

- (void)idleTasks:(NSTimer *)timer {
  static NSArray *modes = nil;
  if (!modes)
    modes = @[NSDefaultRunLoopMode, NSModalPanelRunLoopMode];

  // if we call flush_idle_tasks() directly here, we could get blocked by a plugin with a
  // modal loop. In that case, the timer would not get fired again until the modal loop
  // terminates, which is a problem. So we defer the execution to after this method returns.
  [[NSRunLoop currentRunLoop] performSelector: @selector(flushIdleTasks:)
                                       target: self
                                     argument: self
                                        order: 0
                                        modes: modes];
}

- (void)requestRefresh {
  @autoreleasepool {
    [self performSelector: @selector(idleTasks:) withObject: nil afterDelay:0];
  }
}

- (IBAction)menuItemClicked:(id)sender {
// identifiers from the App menu. These are set in the nib
#define APP_MENU_ABOUT 100
#define APP_MENU_PREFERENCES 101
#define APP_MENU_QUIT 102

  // special handling for some Application menu items
  switch ([sender tag]) {
    case APP_MENU_ABOUT:
      wb::WBContextUI::get()->get_command_ui()->activate_command("builtin:show_about");
      break;

    case APP_MENU_PREFERENCES:
      wb::WBContextUI::get()->get_command_ui()->activate_command("plugin:wb.form.showOptions");
      break;

    case APP_MENU_QUIT:
      quitApplication(mainController);
      break;
  }
}

- (IBAction)showDiagramProperties:(id)sender {
  WBDiagramSizeController *controller = [[WBDiagramSizeController alloc] init];

  [controller showModal];
}

- (IBAction)buttonClicked:(id)sender {
  if ([sender tag] == 10) {
    [NSApp stopModalWithCode:NSModalResponseOK];
    [pageSetup orderOut: nil];
  } else if ([sender tag] == 11) {
    [NSApp stopModalWithCode:NSModalResponseCancel];
    [pageSetup orderOut: nil];
  } else if (sender == landscapeButton) {
    landscapeButton.state = NSControlStateValueOn;
    portraitButton.state = NSControlStateValueOff;
  } else if (sender == portraitButton) {
    landscapeButton.state = NSControlStateValueOff;
    portraitButton.state = NSControlStateValueOn;
  }
}

- (void)selectCollectionItem:(id)sender {
  if (sender == paperSize) {
    paperSizeLabel.stringValue = paperSize.selectedItem.representedObject;
    [paperSizeLabel sizeToFit];
  }
}

- (void)showPageSetup:(id)sender {
  logDebug("Showing page setup dialog\n");

  app_PageSettingsRef settings(wb::WBContextUI::get()->get_page_settings());

  if (!settings.is_valid())
    return;

  if (!pageSetup) {
    NSMutableArray *temp;
    if (![NSBundle.mainBundle loadNibNamed:@"PageSetup" owner: self topLevelObjects:&temp])
      return;

    pageSetupNibObjects = temp;

    if ([[NSFileManager defaultManager] fileExistsAtPath:@"/System/Library/PrivateFrameworks/PrintingPrivate.framework/"
                                                         @"Versions/A/Plugins/PrintingCocoaPDEs.bundle/Contents/"
                                                         @"Resources/Landscape.tiff"]) {
      landscapeButton.image = [[NSImage alloc]
        initWithContentsOfFile:@"/System/Library/PrivateFrameworks/PrintingPrivate.framework/Versions/A/Plugins/"
                               @"PrintingCocoaPDEs.bundle/Contents/Resources/Landscape.tiff"];
      portraitButton.image = [[NSImage alloc]
        initWithContentsOfFile:@"/System/Library/PrivateFrameworks/PrintingPrivate.framework/Versions/A/Plugins/"
                               @"PrintingCocoaPDEs.bundle/Contents/Resources/Portrait.tiff"];
    } else {
      landscapeButton.image = [[NSImage alloc]
        initWithContentsOfFile:@"/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/Print.framework/"
                               @"Versions/A/Plugins/PrintingCocoaPDEs.bundle/Contents/Resources/Landscape.tiff"];
      portraitButton.image = [[NSImage alloc]
        initWithContentsOfFile:@"/System/Library/Frameworks/Carbon.framework/Versions/A/Frameworks/Print.framework/"
                               @"Versions/A/Plugins/PrintingCocoaPDEs.bundle/Contents/Resources/Portrait.tiff"];
    }
  }

  std::list<wb::WBPaperSize> paper_sizes = wb::WBContextUI::get()->get_paper_sizes(false);

  [paperSize removeAllItems];
  for (std::list<wb::WBPaperSize>::const_iterator iter = paper_sizes.begin(); iter != paper_sizes.end(); ++iter) {
    [paperSize addItemWithTitle: [NSString stringWithCPPString:iter->name]];
    [paperSize itemAtIndex:paperSize.numberOfItems - 1].representedObject =
      [NSString stringWithCPPString: iter->description];
  }
  if (settings->paperType().is_valid()) {
    [paperSize selectItemWithTitle: [NSString stringWithCPPString: *settings->paperType()->name()]];
    [self selectCollectionItem:paperSize];
  }
  if (settings->orientation() == "landscape") {
    landscapeButton.state = NSControlStateValueOn;
    portraitButton.state = NSControlStateValueOff;
  } else {
    landscapeButton.state = NSControlStateValueOff;
    portraitButton.state = NSControlStateValueOn;
  }

  if ([NSApp runModalForWindow:pageSetup] == NSModalResponseOK) {
    logDebug("Page settings accepted. Updating model...\n");
    std::string type = paperSize.titleOfSelectedItem.UTF8String;
    app_PaperTypeRef paperType(grt::find_named_object_in_list(_wb->get_root()->options()->paperTypes(), type));
    std::string orientation;

    if (paperType != settings->paperType())
      settings->paperType(paperType);
    if (landscapeButton.state == NSControlStateValueOn)
      orientation = "landscape";
    else
      orientation = "portrait";
    if (orientation != *settings->orientation())
      settings->orientation(orientation);

    _wb->get_model_context()->update_page_settings();
  }

  logDebug("Page setup dialog done\n");
}

@end
