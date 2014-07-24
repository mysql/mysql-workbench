/* 
 * Copyright (c) 2009, 2014, Oracle and/or its affiliates. All rights reserved.
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

#import "MFUtilities.h"
#import "MHudController.h"

#import "MFMForms.h"
#import "MFBase.h"
#include <stdexcept>

#include "base/log.h"

DEFAULT_LOG_DOMAIN("utilities");

static void util_beep()
{
  NSBeep();
}

static int util_show_message(const std::string &title, const std::string &text,
                             const std::string &ok, const std::string &cancel,
                             const std::string &other)
{
  int res = [[NSAlert alertWithMessageText: wrap_nsstring(title)
                             defaultButton: ok.empty() ? nil : wrap_nsstring(ok)
                           alternateButton: cancel.empty() ? nil : wrap_nsstring(cancel)
                               otherButton: other.empty() ? nil : wrap_nsstring(other)
                 informativeTextWithFormat: @"%@",
              [wrap_nsstring(text) stringByReplacingOccurrencesOfString: @"%"
                                                             withString: @"%%"]] runModal];
  if (res == NSAlertDefaultReturn)
    return mforms::ResultOk;
  else if (res == NSAlertOtherReturn)
    return mforms::ResultOther;
  else
    return mforms::ResultCancel;
}


static int util_show_message_with_checkbox(const std::string &title, const std::string &text,
                                           const std::string &ok, const std::string &cancel,
                                           const std::string &other,
                                           const std::string &cb_message, bool &cb_answer)
{
  NSAlert *alert = [[[NSAlert alloc] init] autorelease];
  
  [alert setMessageText: wrap_nsstring(title)];
  [alert setInformativeText: [wrap_nsstring(text) stringByReplacingOccurrencesOfString:@"%" withString:@"%%"]];
  [alert setShowsSuppressionButton: YES];
  if (!cb_message.empty())
    [[alert suppressionButton] setTitle: wrap_nsstring(cb_message)];
  
  if (!ok.empty())
    [[alert addButtonWithTitle: wrap_nsstring(ok)] setTag: NSAlertDefaultReturn];
  
  if (!cancel.empty())
    [[alert addButtonWithTitle: wrap_nsstring(cancel)] setTag: NSAlertOtherReturn];
  
  if (!other.empty())
    [[alert addButtonWithTitle: wrap_nsstring(other)] setTag: NSAlertAlternateReturn];
  
  int res = [alert runModal];

  cb_answer = [[alert suppressionButton] state] == NSOnState;
  
  if (res == NSAlertDefaultReturn)
    return mforms::ResultOk;
  else if (res == NSAlertOtherReturn)
    return mforms::ResultCancel;
  else
    return mforms::ResultOther;
}


static void util_set_clipboard_text(const std::string &text)
{
  NSPasteboard *pasteBoard= [NSPasteboard generalPasteboard];
  [pasteBoard declareTypes: [NSArray arrayWithObject:NSStringPboardType] owner:nil];
  [pasteBoard setString: [NSString stringWithUTF8String:text.c_str()]
                                      forType: NSStringPboardType];
}

static std::string util_get_clipboard_text()
{
  NSPasteboard *pasteBoard= [NSPasteboard generalPasteboard];
  return [[pasteBoard stringForType: NSStringPboardType] UTF8String] ?:"";
}

static void util_open_url(const std::string &url)
{
  if (g_file_test(url.c_str(), G_FILE_TEST_EXISTS) || (!url.empty() && url[0] == '/'))
    [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath: [NSString stringWithUTF8String: url.c_str()]]];
  else
    if (![[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString: [[NSString stringWithUTF8String: url.c_str()]
                                                                       stringByAddingPercentEscapesUsingEncoding: NSUTF8StringEncoding]]])
      log_error("Could not open URL %s\n", url.c_str());
}


static std::string get_special_folder(mforms::FolderType type)
{
  switch (type)
  {
    case mforms::Documents:
      return [[NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject] UTF8String];
    case mforms::Desktop:
      return [[NSSearchPathForDirectoriesInDomains(NSDesktopDirectory, NSUserDomainMask, YES) lastObject] UTF8String];
    case mforms::ApplicationData:
      return [[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] UTF8String];
    case mforms::ApplicationSettings:
      return [[[NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject] stringByAppendingPathComponent: @"MySQL/Workbench"] UTF8String];
    case mforms::WinProgramFiles:
    case mforms::WinProgramFilesX86:
      break;
  }
  return "";
}


@interface MFTimerHandler : NSObject
{
  boost::function<bool ()> *callback;
}

- (id)initWithSlot:(boost::function<bool ()>)slot;
- (void)fire:(NSTimer*)timer;

@end


static std::map<mforms::TimeoutHandle, NSTimer*> active_timeouts;
static mforms::TimeoutHandle current_timeout = 0;
static base::Mutex timeout_lock;

@implementation MFTimerHandler

- (id)initWithSlot:(boost::function<bool ()>)slot
{
  self = [super init];
  if (self)
  {
    callback = new boost::function<bool ()>(slot);
  }
  return self;
}

- (void)dealloc
{
  delete callback;
  [super dealloc];
}

- (void)fire:(NSTimer*)timer
{
  {
    base::MutexLock lock(timeout_lock);
  
    for (std::map<mforms::TimeoutHandle, NSTimer*>::iterator it = active_timeouts.begin();
         it != active_timeouts.end(); ++it)
    {
      if (it->second == timer)
      {
        active_timeouts.erase(it);
        break;
      }
    }
  }
  try
  {
    bool ret = (*callback)();
    if (!ret)
    {
      [timer invalidate];
      [self autorelease];
    }
  }
  catch (std::exception &exc)
  {
    log_error("Unhandled exception calling timer callback: %s\n", exc.what());
  }
}

@end

static mforms::TimeoutHandle util_add_timeout(float interval, const boost::function<bool ()> &callback)
{
  base::MutexLock lock(timeout_lock);
  
  MFTimerHandler *handler = [[MFTimerHandler alloc] initWithSlot:callback];

  active_timeouts[++current_timeout] = [NSTimer scheduledTimerWithTimeInterval:interval
                                                                        target:handler selector:@selector(fire:)
                                                                      userInfo:nil repeats:YES];
  return current_timeout;
}


static void util_cancel_timeout(mforms::TimeoutHandle handle)
{
  base::MutexLock lock(timeout_lock);
  std::map<mforms::TimeoutHandle, NSTimer*>::iterator it;
  
  if ((it = active_timeouts.find(handle)) != active_timeouts.end())
  {
    [it->second invalidate];
    active_timeouts.erase(it);
  }
}


static std::string os_error_to_string(OSErr error)
{
  CFStringRef ref = SecCopyErrorMessageString(error, nil);

  std::string result = [(NSString*)ref UTF8String]; // Toll-free-bridged.
  CFRelease(ref);
  return result;
}

static void util_store_password(const std::string &service, const std::string &account, const std::string &password)
{
  // See if we already have a password for this service + account. If so, modify this, otherwise add a new password.
  SecKeychainItemRef item;
  OSErr code = SecKeychainFindGenericPassword(NULL, service.length(), service.c_str(), account.length(),
                                              account.c_str(), NULL, NULL, &item);
  if (code == 0)
  {
    code = SecKeychainItemModifyAttributesAndData(item, NULL, password.length(), password.c_str());
    CFRelease(item);
    if (code == 0)
      return;
  }
  else
  {
    code = SecKeychainAddGenericPassword(NULL, service.length(), service.c_str(), account.length(),
                                         account.c_str(), password.length(), password.c_str(), NULL);
    if (code == 0)
      return;
  }

  throw std::runtime_error("Error storing password:" + os_error_to_string(code));
}


static bool util_find_password(const std::string &service, const std::string &account, std::string &password)
{
  UInt32 password_length= 0;
  void *password_data= NULL;
  
  if (SecKeychainFindGenericPassword(NULL,
                                     service.length(),
                                     service.c_str(),
                                     account.length(),
                                     account.c_str(),
                                     &password_length,
                                     &password_data,
                                     NULL) != 0)
    return false;

  if (password_data)
  {
    password = std::string((char*)password_data, (size_t)password_length);
    SecKeychainItemFreeContent(NULL, password_data);
    return true;
  }
  return false;  
}

static void util_forget_password(const std::string &service, const std::string &account)
{
  SecKeychainItemRef item;
  OSErr code;
  
  if ((code = SecKeychainFindGenericPassword(NULL,
                                             service.length(),
                                             service.c_str(),
                                             account.length(),
                                             account.c_str(),
                                             NULL,
                                             NULL,
                                             &item)) == 0)
  {
    if ((code = SecKeychainItemDelete(item)) != 0)
    {
      CFRelease(item);
      throw std::runtime_error("Error deleting password entry: "+os_error_to_string(code));
    }
    CFRelease(item);
  }
}

//--------------------------------------------------------------------------------------------------

static void util_show_wait_message(const std::string &title, const std::string &message)
{
  [MHudController showHudWithTitle: [NSString stringWithUTF8String: title.c_str()]
                    andDescription: [NSString stringWithUTF8String: message.c_str()]];
}


static bool util_run_cancelable_wait_message(const std::string &title, const std::string &text,
                                             const boost::function<void ()> &start_task, 
                                             const boost::function<bool ()> &cancel_task)
{
  return [MHudController runModalHudWithTitle: [NSString stringWithUTF8String: title.c_str()]
                               andDescription: [NSString stringWithUTF8String: text.c_str()]
                                  notifyReady: start_task
                                 cancelAction: cancel_task];
}


static void util_stop_cancelable_wait_message()
{
  [MHudController stopModalHud];
}

//--------------------------------------------------------------------------------------------------

static bool util_hide_wait_message()
{
  return [MHudController hideHud];
}

//--------------------------------------------------------------------------------------------------

static bool util_move_to_trash(const std::string &path)
{
  FSRef ref;
  if (FSPathMakeRefWithOptions((const UInt8 *)[wrap_nsstring(path) fileSystemRepresentation], 
                               kFSPathMakeRefDoNotFollowLeafSymlink,
                               &ref,
                               NULL) != 0)
    return false;
  
  if (FSMoveObjectToTrashSync(&ref, NULL, kFSFileOperationDefaultOptions) != 0)
    return false;
  return true;
}

//--------------------------------------------------------------------------------------------------

static void reveal_file(const std::string &path)
{
  [[NSWorkspace sharedWorkspace] selectFile: wrap_nsstring(path) inFileViewerRootedAtPath: @""];
}

//--------------------------------------------------------------------------------------------------

@interface MainThreadRunner : NSObject
{
@public
  boost::function<void* ()> slot;
  void *result;
}

@end

@implementation MainThreadRunner

- (void)dealloc
{
  [NSObject cancelPreviousPerformRequestsWithTarget: self];
  [super dealloc];
}

- (void)perform
{
  if (slot)
    result = slot();
  else
    result = NULL;
}

@end


static void *util_perform_from_main_thread(const boost::function<void* ()> &slot, bool wait_response)
{
  if ([NSThread isMainThread])
    return slot ? slot() : NULL;
  else
  {
    MainThreadRunner *tmp = [[MainThreadRunner alloc] init];
    tmp->slot = slot;
    tmp->result = NULL;
    [tmp performSelectorOnMainThread: @selector(perform) 
                          withObject: nil
                       waitUntilDone: wait_response];
    void *result = tmp->result;
    [tmp release];
    return result;
  }
}

static void util_set_thread_name(const std::string &name)
{
  @autoreleasepool {
    [[NSThread currentThread] setName: [NSString stringWithUTF8String: name.c_str()]];
  }
}

//--------------------------------------------------------------------------------------------------

void cf_util_init()
{
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
}
