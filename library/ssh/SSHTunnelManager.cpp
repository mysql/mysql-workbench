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

#ifndef _MSC_VER
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#else
#  ifndef SHUT_RDWR
#    define SHUT_RDWR SD_BOTH
#  endif
#endif
#include <libssh/callbacks.h>
#include <libssh/libssh.h>
#include <vector>
#include "base/log.h"
#include "SSHTunnelManager.h"

DEFAULT_LOG_DOMAIN("SSHTunnelManager")
namespace ssh {

  SSHTunnelManager::SSHTunnelManager()
      : _wakeupSocketPort(0), _wakeupSocket(-1) {
#if _MSC_VER
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
      throw SSHTunnelException("Error at WSAStartup()\n");
    }
#endif
    initLibSSH();
    auto retVal = createSocket();
    logInfo("Wakeup socket port created: %d\n", retVal.port);
    _wakeupSocketPort = retVal.port;
    _wakeupSocket = retVal.socketHandle;
  }

  SSHTunnelManager::~SSHTunnelManager() {
    _stop = true;

    shutdown(_wakeupSocket, SHUT_RDWR);
    // first, let's shutdown all sockets
    for (auto &it : _socketList) {
      shutdown(it.first, SHUT_RDWR);
    }

    stop();  // wait for thread to finish
    auto sockLock = lockSocketList();
    for (auto &it : _socketList) {
      it.second->stop();
      it.second.release();
    }
#if _MSC_VER
    WSACleanup();
#endif
  }

  base::RecMutexLock SSHTunnelManager::lockSocketList() {
    base::RecMutexLock mutexLock(_socketMutex);
    return mutexLock;
  }

  void SSHTunnelManager::run() {
    localSocketHandler();
  }

  sockInfo SSHTunnelManager::createSocket() {
    sockInfo returnVal;
    errno = 0;
    returnVal.socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if (returnVal.socketHandle == -1) {
      throw SSHTunnelException("unable to create socket: " + getError());
    }

    int val = 1;
    errno = 0;
    if (setsockopt(returnVal.socketHandle, SOL_SOCKET, SO_REUSEADDR, (char *) &val, sizeof(val)) == -1) {
      wbCloseSocket(returnVal.socketHandle);
      throw SSHTunnelException("unable to set socket option: " + getError());
    }

    setSocketNonBlocking(returnVal.socketHandle);

    struct sockaddr_in addr, info;
    socklen_t len = sizeof(struct sockaddr_in);
    memset(&addr, 0, len);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(0);  // OS will pick random port for us, we will get it later.

    if (bind(returnVal.socketHandle, (struct sockaddr*) &addr, len) == -1) {
      wbCloseSocket(returnVal.socketHandle);
      throw SSHTunnelException("unable to bind: " + getError());
    }

    getsockname(returnVal.socketHandle, (struct sockaddr*) &info, &len);
    returnVal.port = htons(info.sin_port);

    if (listen(returnVal.socketHandle, 2) == -1) {
      wbCloseSocket(returnVal.socketHandle);
      throw SSHTunnelException("can't listen on socket: " + getError());
    }

    return returnVal;
  }

  std::tuple<SSHReturnType, base::any> SSHTunnelManager::createTunnel(std::shared_ptr<SSHSession> &session) {
    logDebug3("About to create ssh tunnel.\n");
    auto sockLock = lockSocketList();
    for (auto &it : _socketList) {
      if (it.second->getConfig() == session->getConfig()) {
        logDebug3("Found existing ssh tunnel.\n");
        return std::make_tuple(SSHReturnType::CONNECTED, it.second->getLocalPort());
      }
    }

    auto ret = createSocket();
    logDebug2("Tunnel port created on socket: %d\n", ret.port);
    std::unique_ptr<SSHTunnelHandler> handler(new SSHTunnelHandler(ret.port, ret.socketHandle, session));
    handler->start();
    _socketList.insert(std::make_pair(ret.socketHandle, std::move(handler)));
    pokeWakeupSocket();  // If we're connected, we should notify manager that it shoud reload connection list.
    return std::make_tuple(SSHReturnType::CONNECTED, ret.port);
  }

  int SSHTunnelManager::lookupTunnel(const SSHConnectionConfig &config) {
    auto sockLock = lockSocketList();

    for (auto &it : _socketList) {
      if (it.second->getConfig() == config) {
        if (!it.second->isRunning()) {
          disconnect(config);
          logWarning("Dead tunnel found, clearing it up.\n");
          return 0;
        }
        return it.second->getLocalPort();
      }
    }

    return 0;
  }

  // We need to handle wakeupsocket connection, this should be enough.
  static void acceptAndClose(int socket) {
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    errno = 0;
    int clientSock = accept(socket, (struct sockaddr*) &client, &addrlen);
    wbCloseSocket(clientSock);
  }

  std::vector<pollfd> SSHTunnelManager::getSocketList() {
    std::vector<pollfd> socketList;
    {
      auto sockLock = lockSocketList();
      for (auto &it : _socketList) {
        pollfd p;
        p.fd = it.second->getLocalSocket();
        p.events = POLLIN;
        socketList.push_back(p);
      }
    }

    {
      pollfd p;
      p.fd = _wakeupSocket;
      p.events = POLLIN;
      socketList.push_back(p);
    }
    return socketList;
  }

  void SSHTunnelManager::localSocketHandler() {
    auto socketList = getSocketList();
    int rc = 0;
    do {
      auto pollSocketList = socketList;  // We need to duplicate this as we will be changing it later, so we could loose other socket data.
      rc = wbPoll(pollSocketList.data(), pollSocketList.size());
      if (rc < 0) {
        logError("poll() error: %s.\n", getError().c_str());
        break;
      }

      if (rc == 0) {
        logError("poll() timeout.\n");
        break;
      }

      for (auto &pollIt : pollSocketList) {
        if (pollIt.revents == 0)
          continue;
        if (pollIt.revents == POLLERR) {
          logError("Error revents: %d.\n", pollIt.revents);
          _stop = true;
          break;
        }

        if (pollIt.fd == _wakeupSocket)  // This is special case we reload fds and continue.
            {
          logDebug2("Wakeup socket got connection, reloading socketList.\n");
          socketList.clear();
          acceptAndClose(pollIt.fd);
          if (_stop)
            break;

          socketList = getSocketList();
          continue;
        } else { // This is a new connection, we need to handle it.
          auto sockLock = lockSocketList();
          auto it = _socketList.find(pollIt.fd);
          if (it != _socketList.end()) {
            it->second->handleNewConnection(pollIt.fd);
          } else {
            // Let's check if this is something that wasn't removed from the sock list, then just close it.
            bool found = false;
            for (auto s : pollSocketList) {
              if (s.fd == pollIt.fd && s.fd != _wakeupSocket) {
                shutdown(pollIt.fd, SHUT_RDWR);
                found = true;
                break;
              }
            }

            if (found) { // We have to reload the socket list here.
              socketList = getSocketList();
            }

            if (!found) {
              logError("Something went wrong, incoming socket connection wasn't found in the socketList, abort.\n");
              _stop = true;
              break;
            }
          }
        }
      }
    } while (!_stop);

    auto sockLock = lockSocketList();
    for (auto &sIt : _socketList) {
      sIt.second.release();
      shutdown(sIt.first, SHUT_RDWR);
    }

    // This means wakeup socket is also cleared.
    _wakeupSocket = 0;
    _socketList.clear();
  }

  void SSHTunnelManager::pokeWakeupSocket() {
    if (_wakeupSocketPort == 0) {
      logError("Somehow wakeup socket isn't set yet.\n");
      return;
    }

    struct sockaddr_in server;
    struct sockaddr *serverptr = (struct sockaddr*) &server;
    int sock;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      logError("Error occured opening wakeup socket");
      return;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_port = htons(_wakeupSocketPort);
    if (connect(sock, serverptr, sizeof(server)) < 0) {
      logDebug2("We've connected. Now we wait for socket to catch up and disconnect us.");
      ssize_t readlen = 0;
      std::vector<char> buff(1, '\0');
      errno = 0;
      readlen = recv(sock, buff.data(), buff.size(), 0);
      if (readlen == 0)
        logDebug2("Wakeup socket received info.\n");
      else
        logError("Wakeup socket error: %s.\n", getError().c_str());
    }

    shutdown(sock, SHUT_RDWR);
  }

  void SSHTunnelManager::disconnect(const SSHConnectionConfig &config) {
    auto sockLock = lockSocketList();
    for (auto &it : _socketList) {
      if (it.second->getConfig() == config) {
        // Here we need to perform disconnect
        it.second->stop();
        it.second.release();
        shutdown(it.first, SHUT_RDWR);
        _socketList.erase(it.first);
        logDebug2("Shutdown port: %d\n", config.localport);
        break;
      }
    }
  }

} /* namespace ssh */
