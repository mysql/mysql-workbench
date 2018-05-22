/*
 * Copyright (c) 2017, 2018, Oracle and/or its affiliates. All rights reserved.
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


#include "SSHCommon.h"
#include "base/log.h"
#include <fcntl.h>
#include <libssh/sftp.h>

DEFAULT_LOG_DOMAIN("SSHCommon")

static int MutexInit(void **priv) {
  *priv = new std::mutex();
  return 0;
}

static int MutexDestroy(void **lock) {
  delete static_cast<std::mutex *>(*lock);
  *lock = nullptr;
  return 0;
}

static int MutexLock(void **lock) {
  static_cast<std::mutex *>(*lock)->lock();
  return 0;
}

static int MutexUnlock(void **lock) {
  static_cast<std::mutex *>(*lock)->unlock();
  return 0;
}

static unsigned long getThreadId(void) {
  std::hash<std::thread::id> hasher;
  return hasher(std::this_thread::get_id());
}

static struct ssh_threads_callbacks_struct stdThreads = { "threads_stdthread", &MutexInit, &MutexDestroy, &MutexLock,
    &MutexUnlock, &getThreadId };

struct ssh_threads_callbacks_struct * ssh_threads_get_std_threads(void) {
  return &stdThreads;
}

void sshLogCallback(int priority, const char *function, const char *buffer, void *userdata) {
  switch (priority) {
    case SSH_LOG_TRACE:
      logDebug3("libssh: %s %s\n", function, buffer);
      break;
    case SSH_LOG_DEBUG:
    case SSH_LOG_WARN: // this should also go to debug as libssh is too chaty :(
      logDebug3("libssh: %s %s\n", function, buffer);
      break;
    case SSH_LOG_INFO:
      logInfo("libssh: %s %s\n", function, buffer);
      break;
    case SSH_LOG_NONE:
    default:
      break;
  }
}

namespace ssh {
  std::string getError() {
    return std::string(strerror(errno));
  }

  std::string getSftpErrorDescription(int rc) {
    switch (rc)
    {
      case SSH_FX_EOF:
        return "End of File";
      case SSH_FX_NO_SUCH_FILE:
        return "File doesn't exist";
      case SSH_FX_PERMISSION_DENIED:
        return "Permission denied";
      case SSH_FX_FAILURE:
        return "Generic failure";
      case SSH_FX_BAD_MESSAGE:
        return "Server replied unknown message";
      case SSH_FX_NO_CONNECTION:
        return "No connection";
      case SSH_FX_CONNECTION_LOST:
        return "Lost connection";
      case SSH_FX_OP_UNSUPPORTED:
        return "Server doesn't understand this operation";
      case SSH_FX_INVALID_HANDLE:
        return "Invalid file handle";
      case SSH_FX_NO_SUCH_PATH:
        return "No such file or directory";
      case SSH_FX_FILE_ALREADY_EXISTS:
        return "Path already exists";
      case SSH_FX_WRITE_PROTECT:
        return "Filesystem is write protected";
      case SSH_FX_NO_MEDIA:
        return "No media in remote drive";
      case SSH_FX_OK:
        return "";
      default:
        return "Unknown error";
    }
  }

  void setSocketNonBlocking(int sock) {
#ifdef _MSC_VER
    u_long mode = 1;
    int result = ioctlsocket(sock, FIONBIO, &mode);
    if (result != NO_ERROR) {
      wbCloseSocket(sock);
      throw SSHTunnelException("unable to set socket nonblock: "+ getError());
    }
#else
    if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
      close(sock);
      throw SSHTunnelException("unable to set socket nonblock: " + getError());
    }
#endif
  }

  static void setupLibSSH() {
    ssh_threads_set_callbacks(ssh_threads_get_std_threads());
    std::string logLevel = base::Logger::active_level();
    if (logLevel == "none")
      ssh_set_log_level(SSH_LOG_NONE);
    else if (logLevel == "warning" || logLevel == "error")
      ssh_set_log_level(SSH_LOG_WARN);
    else if (logLevel == "info")
      ssh_set_log_level(SSH_LOG_INFO);
    else if (logLevel == "debug1" || logLevel == "debug2")
      ssh_set_log_level(SSH_LOG_DEBUG);
    else if (logLevel == "debug3")
      ssh_set_log_level(SSH_LOG_TRACE);

    ssh_set_log_callback(sshLogCallback);
    ssh_init();
  }

  void initLibSSH() {
    std::call_once(sshInitOnce, []{setupLibSSH();});
  }


  SSHConnectionConfig::SSHConnectionConfig() : localport(0), bufferSize(10240),
    remoteSSHport(22), remoteport(3306), strictHostKeyCheck(true), compressionLevel(5), connectTimeout(10),
    readWriteTimeout(5), commandTimeout(1), commandRetryCount(3) {

  }

  void SSHConnectionConfig::dumpConfig() const {
    logDebug2("SSH Connection config info:\n");
    logDebug2("SSH bufferSize: %lu\n", bufferSize);
    logDebug2("SSH connectTimeout: %lu\n", connectTimeout);
    logDebug2("SSH readWriteTimeout: %lu\n", readWriteTimeout);
    logDebug2("SSH commandTimeout: %lu\n", commandTimeout);
    logDebug2("SSH commandRetryCount: %lu\n", commandRetryCount);
    logDebug2("SSH optionsDir: %s\n", optionsDir.c_str());
    logDebug2("SSH known hosts file: %s\n", knownHostsFile.c_str());
    logDebug2("SSH local host: %s\n", localhost.c_str());
    logDebug2("SSH local port: %d\n", localport);
    logDebug2("SSH remote host: %s\n", remotehost.c_str());
    logDebug2("SSH remote port: %d\n", remoteport);
    logDebug2("SSH remote ssh host: %s\n", remoteSSHhost.c_str());
    logDebug2("SSH remote ssh port: %lu\n", remoteSSHport);
    logDebug2("SSH strict host key check: %s\n", strictHostKeyCheck ? "yes" : "no");
  }

  bool operator==(const SSHConnectionConfig &tun1, const SSHConnectionConfig &tun2) {
    return (tun1.localhost == tun2.localhost && tun1.remoteSSHhost == tun2.remoteSSHhost
        && tun1.remoteSSHport == tun2.remoteSSHport && tun1.remotehost == tun2.remotehost
        && tun1.remoteport == tun2.remoteport);
  }

  bool operator!=(const SSHConnectionConfig &tun1, const SSHConnectionConfig &tun2) {
    return !(tun1 == tun2);
  }

  void SSHThread::_run() {
    _initializationSem.post();
    _finished = false;
    run();
    _finished = true;
  }

  SSHThread::SSHThread()
      : _stop(false), _finished(true), _initializationSem(0) {
  }

  SSHThread::~SSHThread() {
    try {
      stop();
    } catch (...) {
      logError("Unable to stop SSHThread.\n");
    }
  }

  void SSHThread::stop() {
    _stop = true;
    if (_thread.joinable())
      _thread.join();
  }

  bool SSHThread::isRunning() {
    return !_finished;
  }

  void SSHThread::start() {
    if (_finished) {
      _stop = false;
      _thread = std::thread(&SSHThread::_run, this);
      _initializationSem.wait();
    }
  }

  void SSHThread::join() {
    _thread.join();
  }
} /* namespace ssh */
