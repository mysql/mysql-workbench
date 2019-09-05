/*
 * Copyright (c) 2017, 2019, Oracle and/or its affiliates. All rights reserved.
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

#include "platform.h"
#include "global.h"

#include "utilities.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

double Stat::getTimeMS(TimeSpecs spec) const {
  timespec t = getTimeSpec(spec);
  return (double)t.tv_sec * 1000 + ((double)t.tv_nsec / 1000000);
}

//----------------------------------------------------------------------------------------------------------------------

std::string Stat::getTimeString(TimeSpecs spec) const {
  timespec t = getTimeSpec(spec);
  
  char buffer[100];
  strftime(buffer, sizeof(buffer), "%c %Z", localtime(&t.tv_sec)) ;
  return buffer;
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::runLoopRun(ScriptingContext &context) const {
  while (!JSContext::stopRunloop) {
    context.expireTimers();

    // TODO: file + sockets.

    context.runImmediates();

    if (!context.callbacksPending())
      break;
  }
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::writeText(std::string const& text, bool error) const {
  if (error)
    std::cerr << text << std::flush;
  else
    std::cout << text << std::flush;
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::createFolder(std::string const& name) const {
#ifdef _MSC_VER
  std::ignore = name;
#else
  if (mkdir(name.c_str(), 0755) != 0)
    throw std::runtime_error("Error while creating folder '" + name + "': " + Utilities::getLastError());
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::removeFolder(std::string const& name) const {
#ifdef _MSC_VER
  std::ignore = name;
#else
  if (::remove(name.c_str()) != 0)
    throw std::runtime_error("Error while removing '" + name + "': " + Utilities::getLastError());
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::removeFile(std::string const& name) const {
#ifdef _MSC_VER
  std::ignore = name;
#else
  if (::remove(name.c_str()) != 0)
    throw std::runtime_error("Error while removing '" + name + "': " + Utilities::getLastError());
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::defineOsConstants(ScriptingContext &context, JSObject &constants) const {
  JSObject signals(&context);
  constants.defineProperty("signals", signals);

  DEFINE_CONSTANT(signals, SIGINT);
  DEFINE_CONSTANT(signals, SIGILL);
  DEFINE_CONSTANT(signals, SIGABRT);
  DEFINE_CONSTANT(signals, SIGFPE);
  DEFINE_CONSTANT(signals, SIGSEGV);
  DEFINE_CONSTANT(signals, SIGTERM);

  JSObject errorNumbers(&context);
  constants.defineProperty("errno", errorNumbers);

  DEFINE_CONSTANT(errorNumbers, E2BIG);
  DEFINE_CONSTANT(errorNumbers, EACCES);
  DEFINE_CONSTANT(errorNumbers, EADDRINUSE);
  DEFINE_CONSTANT(errorNumbers, EADDRNOTAVAIL);
  DEFINE_CONSTANT(errorNumbers, EAFNOSUPPORT);
  DEFINE_CONSTANT(errorNumbers, EAGAIN);
  DEFINE_CONSTANT(errorNumbers, EALREADY);
  DEFINE_CONSTANT(errorNumbers, EBADF);
  DEFINE_CONSTANT(errorNumbers, EBADMSG);
  DEFINE_CONSTANT(errorNumbers, EBUSY);
  DEFINE_CONSTANT(errorNumbers, ECANCELED);
  DEFINE_CONSTANT(errorNumbers, ECHILD);
  DEFINE_CONSTANT(errorNumbers, ECONNABORTED);
  DEFINE_CONSTANT(errorNumbers, ECONNREFUSED);
  DEFINE_CONSTANT(errorNumbers, ECONNRESET);
  DEFINE_CONSTANT(errorNumbers, EDEADLK);
  DEFINE_CONSTANT(errorNumbers, EDESTADDRREQ);
  DEFINE_CONSTANT(errorNumbers, EDOM);
  DEFINE_CONSTANT(errorNumbers, EEXIST);
  DEFINE_CONSTANT(errorNumbers, EFAULT);
  DEFINE_CONSTANT(errorNumbers, EFBIG);
  DEFINE_CONSTANT(errorNumbers, EHOSTUNREACH);
  DEFINE_CONSTANT(errorNumbers, EIDRM);
  DEFINE_CONSTANT(errorNumbers, EILSEQ);
  DEFINE_CONSTANT(errorNumbers, EINPROGRESS);
  DEFINE_CONSTANT(errorNumbers, EINTR);
  DEFINE_CONSTANT(errorNumbers, EINVAL);
  DEFINE_CONSTANT(errorNumbers, EIO);
  DEFINE_CONSTANT(errorNumbers, EISCONN);
  DEFINE_CONSTANT(errorNumbers, EISDIR);
  DEFINE_CONSTANT(errorNumbers, ELOOP);
  DEFINE_CONSTANT(errorNumbers, EMFILE);
  DEFINE_CONSTANT(errorNumbers, EMLINK);
  DEFINE_CONSTANT(errorNumbers, EMSGSIZE);
  DEFINE_CONSTANT(errorNumbers, ENAMETOOLONG);
  DEFINE_CONSTANT(errorNumbers, ENETDOWN);
  DEFINE_CONSTANT(errorNumbers, ENETRESET);
  DEFINE_CONSTANT(errorNumbers, ENETUNREACH);
  DEFINE_CONSTANT(errorNumbers, ENFILE);
  DEFINE_CONSTANT(errorNumbers, ENOBUFS);
  DEFINE_CONSTANT(errorNumbers, ENODATA);
  DEFINE_CONSTANT(errorNumbers, ENODEV);
  DEFINE_CONSTANT(errorNumbers, ENOENT);
  DEFINE_CONSTANT(errorNumbers, ENOEXEC);
  DEFINE_CONSTANT(errorNumbers, ENOLCK);
  DEFINE_CONSTANT(errorNumbers, ENOLINK);
  DEFINE_CONSTANT(errorNumbers, ENOMEM);
  DEFINE_CONSTANT(errorNumbers, ENOMSG);
  DEFINE_CONSTANT(errorNumbers, ENOPROTOOPT);
  DEFINE_CONSTANT(errorNumbers, ENOSPC);
  DEFINE_CONSTANT(errorNumbers, ENOSR);
  DEFINE_CONSTANT(errorNumbers, ENOSTR);
  DEFINE_CONSTANT(errorNumbers, ENOSYS);
  DEFINE_CONSTANT(errorNumbers, ENOTCONN);
  DEFINE_CONSTANT(errorNumbers, ENOTDIR);
  DEFINE_CONSTANT(errorNumbers, ENOTEMPTY);
  DEFINE_CONSTANT(errorNumbers, ENOTSOCK);
  DEFINE_CONSTANT(errorNumbers, ENOTSUP);
  DEFINE_CONSTANT(errorNumbers, ENOTTY);
  DEFINE_CONSTANT(errorNumbers, ENXIO);
  DEFINE_CONSTANT(errorNumbers, EOPNOTSUPP);
  DEFINE_CONSTANT(errorNumbers, EOVERFLOW);
  DEFINE_CONSTANT(errorNumbers, EPERM);
  DEFINE_CONSTANT(errorNumbers, EPIPE);
  DEFINE_CONSTANT(errorNumbers, EPROTO);
  DEFINE_CONSTANT(errorNumbers, EPROTONOSUPPORT);
  DEFINE_CONSTANT(errorNumbers, EPROTOTYPE);
  DEFINE_CONSTANT(errorNumbers, ERANGE);
  DEFINE_CONSTANT(errorNumbers, EROFS);
  DEFINE_CONSTANT(errorNumbers, ESPIPE);
  DEFINE_CONSTANT(errorNumbers, ESRCH);
  DEFINE_CONSTANT(errorNumbers, ETIME);
  DEFINE_CONSTANT(errorNumbers, ETIMEDOUT);
  DEFINE_CONSTANT(errorNumbers, ETXTBSY);
  DEFINE_CONSTANT(errorNumbers, EWOULDBLOCK);
  DEFINE_CONSTANT(errorNumbers, EXDEV);
}

//----------------------------------------------------------------------------------------------------------------------

void Platform::defineFsConstants(ScriptingContext &context, JSObject &constants) const {
  std::ignore = context;
  
  DEFINE_CONSTANT(constants, F_OK);
  DEFINE_CONSTANT(constants, R_OK);
  DEFINE_CONSTANT(constants, W_OK);
  DEFINE_CONSTANT(constants, X_OK);
}


//----------------------------------------------------------------------------------------------------------------------

std::string Platform::getHomeDir() const {
#ifdef _MSC_VER
  return "";
#else
  struct passwd *pw = getpwuid(getuid());

  return pw->pw_dir;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

std::string Platform::getHostName() const {
#ifdef _MSC_VER
  return "";
#else
  char name[MAXHOSTNAMELEN +1];
  gethostname(name, sizeof(name));
  name[sizeof(name) -1 ] = '\0';

  return name;
#endif
}


//----------------------------------------------------------------------------------------------------------------------

std::string Platform::getRelease() const {
#ifndef _MSC_VER
  struct utsname info;
  if (uname(&info) >= 0) {
    return info.release;
  }
#endif

  return "";
}

//----------------------------------------------------------------------------------------------------------------------

UserInfo Platform::getCurrentUserInfo() const {
  UserInfo result { "", 0 , 0, "", ""};

#ifndef _MSC_VER
  long size = sysconf(_SC_GETPW_R_SIZE_MAX);

  if (size > -1) {
    char buffer[size];
    struct passwd pwd, *ptr = nullptr;

    if (getpwuid_r(getuid(), &pwd, buffer, static_cast<size_t>(size), &ptr) == 0 && ptr != nullptr) {
      result.userName = pwd.pw_name != nullptr ? pwd.pw_name : "";
      result.uid = static_cast<int>(pwd.pw_uid);
      result.gid = static_cast<int>(pwd.pw_gid);
      result.shell = pwd.pw_shell != nullptr ? pwd.pw_shell : "";
      result.homeDir = pwd.pw_dir != nullptr ? pwd.pw_dir : "";
    }
  }
#endif

  return result;
}

//----------------------------------------------------------------------------------------------------------------------
