/*
 * Copyright (c) 2012, 2017, Oracle and/or its affiliates. All rights reserved.
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

#include "base/threading.h"

#if !defined(__APPLE__) && !defined(_WIN32)
#include <unistd.h>
#include <wait.h>
#endif

#include <algorithm>
#include "base/string_utilities.h"

using namespace base;

#ifdef _WIN32

void base::launchTool(const std::string &name, const std::vector<std::string> &params) {
  std::stringstream ss;
  std::copy(params.begin(), params.end(), std::ostream_iterator<std::string>(ss, " "));
  std::wstring wexe = base::string_to_wstring(name);
  std::wstring wparam = base::string_to_wstring(ss.str());
  SHELLEXECUTEINFO shellExeInfo;
  memset(&shellExeInfo, 0, sizeof(shellExeInfo));
  shellExeInfo.cbSize = sizeof(shellExeInfo);
  shellExeInfo.fMask = NULL;
  shellExeInfo.lpVerb = L"runas";
  shellExeInfo.lpFile = wexe.c_str();
  shellExeInfo.lpParameters = wparam.c_str();
  shellExeInfo.nShow = SW_SHOWMAXIMIZED;
  SetLastError(ERROR_SUCCESS);
  if (ShellExecuteEx(&shellExeInfo) == FALSE) {
    LPVOID msgBuf = NULL;
    DWORD lastErr = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastErr,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&msgBuf, 0, NULL);
    std::wstring msg = (LPCTSTR)msgBuf;
    LocalFree(msgBuf);
    SetLastError(ERROR_SUCCESS);
    throw std::runtime_error(base::wstring_to_string(msg));
  }
}

void base::launchApplication(const std::string &path, const std::vector<std::string> &params) {
  launchTool(path, params);
}

#elif __APPLE__

void base::launchTool(const std::string &name, const std::vector<std::string> &params) {
  NSString *path = [NSString stringWithUTF8String:name.c_str()];

  NSMutableArray *args = [NSMutableArray new];
  for (size_t i = 0; i < params.size(); ++i)
    [args addObject:[NSString stringWithUTF8String:params[i].c_str()]];

  NSTask *task = [NSTask launchedTaskWithLaunchPath:path arguments:args];
  if (task == nil)
    throw std::runtime_error("Running the tool failed.");
}

void base::launchApplication(const std::string &name, const std::vector<std::string> &params) {
  NSString *appName = [NSString stringWithUTF8String:name.c_str()];

  NSMutableArray *args = [NSMutableArray new];
  for (size_t i = 0; i < params.size(); ++i)
    [args addObject:[NSString stringWithUTF8String:params[i].c_str()]];

  NSError *error = nil;
  NSString *root = [NSBundle.mainBundle.bundlePath stringByDeletingLastPathComponent];
  NSURL *url = [NSURL fileURLWithPath:[root stringByAppendingPathComponent:appName]];

  NSRunningApplication *application =
    [NSWorkspace.sharedWorkspace launchApplicationAtURL:url
                                                options:NSWorkspaceLaunchAndHide
                                          configuration:@{
                                            NSWorkspaceLaunchConfigurationArguments : args
                                          }
                                                  error:&error];
  if (error != nil)
    throw std::runtime_error([error.localizedDescription stringByAppendingString:url.absoluteString].UTF8String);
  [application activateWithOptions:NSApplicationActivateAllWindows];
}

#else
#include <glibmm/spawn.h>
#include <glibmm/vectorutils.h>
#include <glibmm/miscutils.h>

void base::launchTool(const std::string &name, const std::vector<std::string> &params) {
  auto tmpParams = params;
  tmpParams.insert(tmpParams.begin(), name);

  auto envp = Glib::ArrayHandler<std::string>::array_to_vector(g_get_environ(), Glib::OWNERSHIP_NONE);
  envp.erase(std::remove_if(envp.begin(), envp.end(),
                            [](const std::string &str) { return str.find("LD_PRELOAD") != std::string::npos; }),
             envp.end());
  Glib::Pid pid;
  try {
    Glib::spawn_async(Glib::get_current_dir(), tmpParams, envp, Glib::SPAWN_DEFAULT, sigc::slot<void>(), &pid);
  } catch (Glib::SpawnError &serr) {
    throw std::runtime_error(serr.what());
  }
}

void base::launchApplication(const std::string &name, const std::vector<std::string> &params) {
  launchTool(name, params);
}

#endif

#if GLIB_CHECK_VERSION(2, 32, 0)
void base::threading_init() {
  // noop
}
#else
static bool g_thread_is_ready = false;
void base::threading_init() {
  if (!g_thread_is_ready && !g_thread_supported())
    g_thread_init(NULL);
  g_thread_is_ready = true;
}
#endif

//--------------------------------------------------------------------------------------------------

Mutex::Mutex() {
#if GLIB_CHECK_VERSION(2, 32, 0)
  g_mutex_init(&mutex);
#else
  threading_init();

  mutex = g_mutex_new();
#endif
}

//--------------------------------------------------------------------------------------------------

Mutex::~Mutex() {
#if GLIB_CHECK_VERSION(2, 32, 0)
  g_mutex_clear(&mutex);
#else
  g_mutex_free(mutex);
#endif
}

//--------------------------------------------------------------------------------------------------

void Mutex::swap(Mutex &o) {
#if GLIB_CHECK_VERSION(2, 32, 0)
  GMutex tmp = o.mutex;
  o.mutex = mutex;
  mutex = tmp;
#else
  GMutex *tmp = o.mutex;
  o.mutex = mutex;
  mutex = tmp;
#endif
}

//--------------------------------------------------------------------------------------------------

MutexLock::MutexLock(Mutex &mutex) : ptr(&mutex) {
  if (!ptr)
    throw std::logic_error("NULL ptr given");
  ptr->lock();
}

//--------------------------------------------------------------------------------------------------

// take ownership of an existing lock (the other lock will be reset)
MutexLock::MutexLock(const MutexLock &mlock) : ptr(mlock.ptr) {
  const_cast<MutexLock *>(&mlock)->ptr = NULL;
}

//--------------------------------------------------------------------------------------------------

MutexLock &MutexLock::operator=(MutexLock &mlock) {
  ptr = mlock.ptr;
  mlock.ptr = NULL;
  return *this;
}

//--------------------------------------------------------------------------------------------------

MutexLock::~MutexLock() {
  if (ptr)
    ptr->unlock();
}

//----------------- Cond ---------------------------------------------------------------------------

Cond::Cond() {
#if GLIB_CHECK_VERSION(2, 32, 0)
  g_cond_init(&cond);
#else
  threading_init();

  cond = g_cond_new();
#endif
}

//--------------------------------------------------------------------------------------------------

Cond::~Cond() {
#if GLIB_CHECK_VERSION(2, 32, 0)
  g_cond_clear(&cond);
#else
  g_cond_free(cond);
#endif
}

//----------------- Semaphore ----------------------------------------------------------------------

static int semaphore_data = 1; // Dummy data to identify non-NULL return values.

Semaphore::Semaphore(int initial_count) {
  _queue = g_async_queue_new();

  // Push as many "messages" to the queue as is specified by the initial count.
  // This amount is what is available before we lock in wait() or try_wait().
  while (initial_count-- > 0)
    g_async_queue_push(_queue, &semaphore_data);
}

//--------------------------------------------------------------------------------------------------

Semaphore::~Semaphore() {
  g_async_queue_unref(_queue);
}

//--------------------------------------------------------------------------------------------------

void Semaphore::post() {
  g_async_queue_push(_queue, &semaphore_data);
}

//--------------------------------------------------------------------------------------------------

void Semaphore::wait() {
  g_async_queue_pop(_queue); // Waits if there is no data in the queue.
}

//--------------------------------------------------------------------------------------------------

bool Semaphore::try_wait() {
  return g_async_queue_try_pop(_queue) != NULL;
}

//--------------------------------------------------------------------------------------------------
