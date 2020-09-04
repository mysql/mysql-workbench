/*
 * Copyright (c) 2009, 2020, Oracle and/or its affiliates.
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

#import "MFUtilities.h"
#import "MHudController.h"

#import "MFMForms.h"
#import "MFBase.h"

#include "base/log.h"
#include "base/string_utilities.h"

DEFAULT_LOG_DOMAIN("utilities");

static void util_beep() {
  NSBeep();
}

//----------------------------------------------------------------------------------------------------------------------

static int util_show_message(const std::string &title, const std::string &text,
                             const std::string &ok, const std::string &cancel,
                             const std::string &other) {
  NSAlert *alert = [NSAlert new];
  alert.messageText = wrap_nsstring(title);
  alert.informativeText = [wrap_nsstring(text) stringByReplacingOccurrencesOfString: @"%" withString: @"%%"];
  if (!ok.empty())
    [alert addButtonWithTitle: wrap_nsstring(ok)];
  if (!cancel.empty())
    [alert addButtonWithTitle: wrap_nsstring(cancel)];
  if (!other.empty())
    [alert addButtonWithTitle: wrap_nsstring(other)];

  NSModalResponse response = [alert runModal];
  if (response == NSAlertFirstButtonReturn)
    return mforms::ResultOk;
  else if (response == NSAlertThirdButtonReturn)
    return mforms::ResultOther;
  else
    return mforms::ResultCancel;
}

//----------------------------------------------------------------------------------------------------------------------

static int util_show_message_with_checkbox(const std::string &title, const std::string &text,
                                           const std::string &ok, const std::string &cancel,
                                           const std::string &other,
                                           const std::string &cb_message, bool &cb_answer) {
  NSAlert *alert = [NSAlert new];
  
  alert.messageText = wrap_nsstring(title);
  alert.informativeText = [wrap_nsstring(text) stringByReplacingOccurrencesOfString:@"%" withString:@"%%"];
  [alert setShowsSuppressionButton: YES];
  if (!cb_message.empty())
    alert.suppressionButton.title = wrap_nsstring(cb_message);
  
  if (!ok.empty())
    [alert addButtonWithTitle: wrap_nsstring(ok)];
  
  if (!cancel.empty())
    [alert addButtonWithTitle: wrap_nsstring(cancel)];
  
  if (!other.empty())
    [alert addButtonWithTitle: wrap_nsstring(other)];
  
  NSModalResponse res = [alert runModal];

  cb_answer = alert.suppressionButton.state == NSControlStateValueOn;
  
  if (res == NSAlertFirstButtonReturn)
    return mforms::ResultOk;
  else if (res == NSAlertThirdButtonReturn)
    return mforms::ResultOther;
  else
    return mforms::ResultCancel;
}

//----------------------------------------------------------------------------------------------------------------------

static void util_set_clipboard_text(const std::string &text) {
  NSPasteboard *pasteBoard= [NSPasteboard generalPasteboard];
  [pasteBoard declareTypes: @[NSPasteboardTypeString] owner:nil];
  [pasteBoard setString: @(text.c_str())
                                      forType: NSPasteboardTypeString];
}

//----------------------------------------------------------------------------------------------------------------------

static std::string util_get_clipboard_text() {
  NSPasteboard *pasteBoard= [NSPasteboard generalPasteboard];
  return [pasteBoard stringForType: NSPasteboardTypeString].UTF8String ?:"";
}

//----------------------------------------------------------------------------------------------------------------------

static void util_open_url(const std::string &url) {
  if (g_file_test(url.c_str(), G_FILE_TEST_EXISTS) || (!url.empty() && url[0] == '/'))
    [[NSWorkspace sharedWorkspace] openURL: [NSURL fileURLWithPath: @(url.c_str())]];
  else {
    std::string fixedUrl = url;
    std::replace(fixedUrl.begin(), fixedUrl.end(), ' ', '+');

    // See if the URL contains a query (and possible fragment). If so escape
    NSString *urlString = @(fixedUrl.c_str());
    NSURL *tmpUrl = [NSURL URLWithString: urlString];
    NSRange range = [urlString rangeOfString: @"?"];

    if (range.location != NSNotFound) {
      urlString = [urlString substringToIndex: range.location];
      urlString = [urlString stringByAppendingString: [NSString stringWithFormat: @"?%@",
        [tmpUrl.query stringByAddingPercentEncodingWithAllowedCharacters: NSCharacterSet.URLQueryAllowedCharacterSet]]];

      NSString *fragment = tmpUrl.fragment;
      if (fragment != nil) {
        urlString = [urlString stringByAppendingString:
                     [fragment stringByAddingPercentEncodingWithAllowedCharacters: NSCharacterSet.URLFragmentAllowedCharacterSet]];
      }
    }

    if (![[NSWorkspace sharedWorkspace] openURL: [NSURL URLWithString: urlString]])
      logError("Could not open URL %s\n", urlString.UTF8String);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static std::string get_special_folder(mforms::FolderType type) {
  switch (type) {
    case mforms::Documents:
      return NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).lastObject.UTF8String;
    case mforms::Desktop:
      return NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES).lastObject.UTF8String;
    case mforms::ApplicationData:
      return NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).lastObject.UTF8String;
    case mforms::ApplicationSettings:
      return [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES).lastObject stringByAppendingPathComponent: @"MySQL/Workbench"].UTF8String;
    case mforms::WinProgramFiles:
    case mforms::WinProgramFilesX86:
      break;
  }
  return "";
}

//----------------------------------------------------------------------------------------------------------------------

@interface MFTimerHandler : NSObject {
  std::function<bool ()> *callback;
}

- (instancetype)initWithSlot:(std::function<bool ()>)slot NS_DESIGNATED_INITIALIZER;
- (void)fire:(NSTimer*)timer;

@end

//----------------------------------------------------------------------------------------------------------------------

static std::map<mforms::TimeoutHandle, NSTimer*> active_timeouts;
static mforms::TimeoutHandle current_timeout = 0;
static base::Mutex timeout_lock;

@implementation MFTimerHandler

- (instancetype)initWithSlot:(std::function<bool ()>)slot {
  self = [super init];
  if (self) {
    callback = new std::function<bool ()>(slot);
  }
  return self;
}

//----------------------------------------------------------------------------------------------------------------------

-(instancetype)init {
  return [self initWithSlot: std::function<bool ()>()];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)dealloc {
  delete callback;
}

//----------------------------------------------------------------------------------------------------------------------

- (void)fire: (NSTimer*)timer {
  try
  {
    bool ret = (*callback)();
    if (!ret) {
      {
        base::MutexLock lock(timeout_lock);

        for (std::map<mforms::TimeoutHandle, NSTimer*>::iterator it = active_timeouts.begin();
             it != active_timeouts.end(); ++it) {
          if (it->second == timer) {
            active_timeouts.erase(it);
            break;
          }
        }
      }

      [timer invalidate];
    }
  }
  catch (std::exception &exc) {
    logError("Unhandled exception calling timer callback: %s\n", exc.what());
  }
}

@end

//----------------------------------------------------------------------------------------------------------------------

static mforms::TimeoutHandle util_add_timeout(float interval, const std::function<bool ()> &callback) {
  base::MutexLock lock(timeout_lock);
  
  MFTimerHandler *handler = [[MFTimerHandler alloc] initWithSlot:callback];

  active_timeouts[++current_timeout] = [NSTimer scheduledTimerWithTimeInterval: interval
                                                                        target: handler
                                                                      selector: @selector(fire:)
                                                                      userInfo: nil
                                                                       repeats: YES];
  return current_timeout;
}

//----------------------------------------------------------------------------------------------------------------------

static void util_cancel_timeout(mforms::TimeoutHandle handle) {
  base::MutexLock lock(timeout_lock);
  std::map<mforms::TimeoutHandle, NSTimer*>::iterator it;
  
  if ((it = active_timeouts.find(handle)) != active_timeouts.end()) {
    [it->second invalidate];
    active_timeouts.erase(it);
  } else
    logWarning("cancel_timeout called on invalid handle %i\n", handle);
}

//----------------------------------------------------------------------------------------------------------------------

static std::string os_error_to_string(OSErr error) {
  CFStringRef ref = SecCopyErrorMessageString(error, nil);

  std::string result = ((__bridge NSString*)ref).UTF8String;
  CFRelease(ref);
  return result;
}

//----------------------------------------------------------------------------------------------------------------------

static void util_store_password(const std::string &service, const std::string &account, const std::string &password) {
  // See if we already have a password for this service + account. If so, modify this, otherwise add a new password.
  SecKeychainItemRef item;
  OSErr code = SecKeychainFindGenericPassword(NULL, (uint32_t)service.length(), service.c_str(),
                                              (uint32_t)account.length(), account.c_str(), NULL, NULL, &item);
  if (code == 0) {
    code = SecKeychainItemModifyAttributesAndData(item, NULL, (uint32_t)password.length(), password.c_str());
    CFRelease(item);
    if (code == 0)
      return;
  } else {
    code = SecKeychainAddGenericPassword(NULL, (uint32_t)service.length(), service.c_str(), (uint32_t)account.length(),
                                         account.c_str(), (uint32_t)password.length(), password.c_str(), NULL);
    if (code == 0)
      return;
  }

  throw std::runtime_error("Error storing password:" + os_error_to_string(code));
}

//----------------------------------------------------------------------------------------------------------------------

static bool util_find_password(const std::string &service, const std::string &account, std::string &password) {
  UInt32 password_length = 0;
  void *password_data = nullptr;
  
  if (SecKeychainFindGenericPassword(nullptr,
                                    (uint32_t)service.size(),
                                    service.c_str(),
                                    (uint32_t)account.size(),
                                    account.c_str(),
                                    &password_length,
                                    &password_data,
                                    nullptr) != 0) {
    return false;
  }

  if (password_data) {
    password = std::string((char*)password_data, (size_t)password_length);
    SecKeychainItemFreeContent(NULL, password_data);
    return true;
  }
  return false;  
}

//----------------------------------------------------------------------------------------------------------------------

static void util_forget_password(const std::string &service, const std::string &account) {
  SecKeychainItemRef item;
  OSErr code;

#if 0
  NSAlert *alert = [NSAlert new];
  alert.messageText = @"Removing password entry";
  [alert runModal];
#endif

  if ((code = SecKeychainFindGenericPassword(nullptr,
                                             (uint32_t)service.size(),
                                             service.c_str(),
                                             (uint32_t)account.size(),
                                             account.c_str(),
                                             nullptr,
                                             nullptr,
                                             &item)) == 0) {
    if ((code = SecKeychainItemDelete(item)) != 0) {
      CFRelease(item);
      throw std::runtime_error("Error deleting password entry: "+os_error_to_string(code));
    }
    CFRelease(item);
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void util_show_wait_message(const std::string &title, const std::string &message) {
  [MHudController showHudWithTitle: @(title.c_str())
                    andDescription: @(message.c_str())];
}

//----------------------------------------------------------------------------------------------------------------------

static bool util_run_cancelable_wait_message(const std::string &title, const std::string &text,
                                             const std::function<void ()> &start_task, 
                                             const std::function<bool ()> &cancel_task) {
  return [MHudController runModalHudWithTitle: @(title.c_str())
                               andDescription: @(text.c_str())
                                  notifyReady: start_task
                                 cancelAction: cancel_task];
}

//----------------------------------------------------------------------------------------------------------------------

static void util_stop_cancelable_wait_message() {
  [MHudController stopModalHud];
}

//----------------------------------------------------------------------------------------------------------------------

static bool util_hide_wait_message() {
  return [MHudController hideHud];
}

//----------------------------------------------------------------------------------------------------------------------

static bool util_move_to_trash(const std::string &path) {
  NSFileManager *manager = NSFileManager.defaultManager;
  NSString *nativePath = @(path.c_str());
  NSURL *url = [NSURL fileURLWithPath: nativePath];

  NSError *error = nil;
  [manager trashItemAtURL: url resultingItemURL: nil error: &error];
  return error == nil;
}

//----------------------------------------------------------------------------------------------------------------------

static void reveal_file(const std::string &path) {
  [[NSWorkspace sharedWorkspace] selectFile: wrap_nsstring(path) inFileViewerRootedAtPath: @""];
}

//----------------------------------------------------------------------------------------------------------------------

@interface MainThreadRunner : NSObject {
@public
  std::function<void* ()> slot;
  void *result;
}

@end

//----------------------------------------------------------------------------------------------------------------------

@implementation MainThreadRunner

- (void)dealloc {
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
}

//----------------------------------------------------------------------------------------------------------------------

- (void)perform {
  if (slot)
    result = slot();
  else
    result = NULL;
}

@end

//----------------------------------------------------------------------------------------------------------------------

static void *util_perform_from_main_thread(const std::function<void* ()> &slot, bool wait_response) {
  if ([NSThread isMainThread])
    return slot ? slot() : NULL;
  else {
    MainThreadRunner *tmp = [[MainThreadRunner alloc] init];
    tmp->slot = slot;
    tmp->result = NULL;
    [tmp performSelectorOnMainThread: @selector(perform) 
                          withObject: nil
                       waitUntilDone: wait_response];
    void *result = tmp->result;
    return result;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static void util_set_thread_name(const std::string &name) {
  @autoreleasepool {
    [NSThread currentThread].name = @(name.c_str());
  }
}

//----------------------------------------------------------------------------------------------------------------------

static double util_get_text_width(const std::string &text, const std::string &font_desc) {
  static NSDictionary *attributeDict = nil;
  static std::string cachedFontName;

  if (!attributeDict || cachedFontName != font_desc) {
    std::string font;
    float size;
    bool bold, italic;
    attributeDict = nil;
    if (base::parse_font_description(font_desc, font, size, bold, italic)) {
      NSFontDescriptor *fd = [NSFontDescriptor fontDescriptorWithName: @(font.c_str()) size: size];
      NSFont *font = [NSFont fontWithDescriptor: [fd fontDescriptorWithSymbolicTraits: (bold ? NSFontBoldTrait : 0) | (italic ? NSFontItalicTrait : 0)]
                                           size: size];

      attributeDict = @{NSFontAttributeName: font};
    }
    cachedFontName = font_desc;
  }

  NSAttributedString *str = [[NSAttributedString alloc] initWithString: @(text.c_str())
                                                            attributes: attributeDict];
  double w = [str size].width;
  return w;
}

//----------------------------------------------------------------------------------------------------------------------

void cf_util_init() {
  ::mforms::ControlFactory *f = ::mforms::ControlFactory::get_instance();

  f->_utilities_impl.beep = &util_beep;
  f->_utilities_impl.show_message= &util_show_message;
  f->_utilities_impl.show_error= &util_show_message;
  f->_utilities_impl.show_warning= &util_show_message;
  f->_utilities_impl.show_message_with_checkbox= &util_show_message_with_checkbox;
  f->_utilities_impl.show_wait_message= &util_show_wait_message;
  f->_utilities_impl.hide_wait_message= &util_hide_wait_message;
  f->_utilities_impl.run_cancelable_wait_message= &util_run_cancelable_wait_message;
  f->_utilities_impl.stop_cancelable_wait_message= &util_stop_cancelable_wait_message;
  f->_utilities_impl.set_clipboard_text= &util_set_clipboard_text;
  f->_utilities_impl.get_clipboard_text= &util_get_clipboard_text;
  f->_utilities_impl.open_url= &util_open_url;
  f->_utilities_impl.add_timeout= &util_add_timeout;
  f->_utilities_impl.cancel_timeout= &util_cancel_timeout;
  f->_utilities_impl.get_special_folder= &get_special_folder;
  f->_utilities_impl.store_password= &util_store_password;
  f->_utilities_impl.find_password= &util_find_password;
  f->_utilities_impl.forget_password= &util_forget_password;
  f->_utilities_impl.move_to_trash= &util_move_to_trash;
  f->_utilities_impl.reveal_file= &reveal_file;
  f->_utilities_impl.perform_from_main_thread= &util_perform_from_main_thread;
  f->_utilities_impl.set_thread_name= &util_set_thread_name;
  f->_utilities_impl.get_text_width = &util_get_text_width;
}

//----------------------------------------------------------------------------------------------------------------------
