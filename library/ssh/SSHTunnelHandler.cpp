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

#include "SSHTunnelHandler.h"

#include "base/log.h"

DEFAULT_LOG_DOMAIN("SSHTunnelHandler")

#ifndef MSG_NOSIGNAL
#  if _MSC_VER
#     define MSG_NOSIGNAL 0
#  else
#    define MSG_NOSIGNAL 0x4000
#  endif
#endif

namespace ssh {

  SSHTunnelHandler::SSHTunnelHandler(uint16_t localPort, int localSocket, std::shared_ptr<SSHSession> session)
      : _session(std::move(session)), _localPort(localPort), _localSocket(localSocket), _pollTimeout(-1) {
    _event = ssh_event_new();
    ssh_event_add_session(_event, _session->getSession()->getCSession());
  }

  SSHTunnelHandler::~SSHTunnelHandler() {
    stop();
    ssh_event_remove_session(_event, _session->getSession()->getCSession());
    ssh_event_free(_event);
    if (_session) {
      _session->disconnect();
      _session.reset();
    }

  }

  int SSHTunnelHandler::getLocalSocket() const {
    return _localSocket;
  }

  int SSHTunnelHandler::getLocalPort() const {
    return _localPort;
  }

  SSHConnectionConfig SSHTunnelHandler::getConfig() const {
    return _session->getConfig();
  }

  void SSHTunnelHandler::run() {
    handleConnection();
  }

  // This is noop function so ssh_even_dopoll will exit once client socket will have new data

    static int onSocketEvent(socket_t fd, int revents, void *userdata) {
      //the return should be:
      //  0 success
      // -1 the internal ssh_poll_handle was removed/freed and should be removed from the context
      // -2 an error happened and the ssh_event_dopoll() should stop
      return 0;
    }

  void SSHTunnelHandler::handleConnection() {
    logDebug3("Start tunnel handler thread.\n");
    int rc = 0;

    do {
      std::unique_lock<std::recursive_mutex> lock(_newConnMtx);
      if(!_newConnection.empty()) {
        prepareTunnel(_newConnection.back());
        _newConnection.pop_back();
      }
      lock.unlock();
      rc = ssh_event_dopoll(_event, 100);

      if (rc == SSH_ERROR) {
        logError("There was an error handling connection poll, retrying: %s\n", _session->getSession()->getError());

        for (auto &sIt : _clientSocketList) {
          ssh_event_remove_fd(_event, sIt.first);
          sIt.second->close();
          wbCloseSocket(sIt.first);
          sIt.second.reset();
        }
        _clientSocketList.clear();

        ssh_event_remove_session(_event, _session->getSession()->getCSession());
        ssh_event_free(_event);

        if (!_session->isConnected())
          _session->reconnect();
        if (!_session->isConnected()) {
          logError("Unable to reconnect session.\n");
          break;
        }

        _event = ssh_event_new();
        ssh_event_add_session(_event, _session->getSession()->getCSession());
        continue;
      }

      for (auto it = _clientSocketList.begin(); it != _clientSocketList.end() && !_stop;) {
        try {
          transferDataFromClient(it->first, it->second.get());
          transferDataToClient(it->first, it->second.get());
          ++it;
        } catch (SSHTunnelException &exc) {
            ssh_event_remove_fd(_event, it->first);
            it->second->close();
            it->second.reset();
            wbCloseSocket(it->first);
            it = _clientSocketList.erase(it);
          logError("Error during data transfer: %s\n", exc.what());
        }
      }

    } while (!_stop);

    for (auto &sIt : _clientSocketList) {
      ssh_event_remove_fd(_event, sIt.first);
      sIt.second->close();
      wbCloseSocket(sIt.first);
      sIt.second.reset();
    }
    _clientSocketList.clear();
    logDebug3("Tunnel handler thread stopped.\n");
  }

  void SSHTunnelHandler::handleNewConnection(int incomingSocket) {
    logDebug3("About to handle new connection.\n");
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    errno = 0;
    int clientSock = accept(incomingSocket, (struct sockaddr*) &client, &addrlen);
    if (clientSock < 0) {
      if (errno != EWOULDBLOCK || errno == EINTR)
        logError("accept() failed: %s\n.", getError().c_str());
      return;
    }

    setSocketNonBlocking(clientSock);

    std::lock_guard<std::recursive_mutex> guard(_newConnMtx);
    _newConnection.push_back(clientSock);
    logDebug3("Accepted new connection.\n");
  }

  void SSHTunnelHandler::transferDataFromClient(int sock, ssh::Channel *chan) {
    ssize_t readlen = 0;
    std::vector<char> buff(_session->getConfig().bufferSize, '\0');

    while (!_stop && (readlen = recv(sock, buff.data(), buff.size(), 0)) > 0) {
      int bWritten = 0;
      for (char* buffPtr = buff.data(); readlen > 0 && !_stop; buffPtr += bWritten, readlen -= bWritten) {
        try {
          bWritten = chan->write(buffPtr, readlen);
          if (bWritten <= 0) {
            throw SSHTunnelException("unable to write, remote end disconnected");
          }
        } catch (SshException &exc) {
          throw SSHTunnelException(exc.getError());
        }
      }
    }
  }

  void SSHTunnelHandler::transferDataToClient(int sock, ssh::Channel *chan) {
    ssize_t readlen = 0;
    std::vector<char> buff(_session->getConfig().bufferSize, '\0');
    do {
      try {
        readlen = chan->readNonblocking(buff.data(), buff.size());
      } catch (SshException &exc) {
        throw SSHTunnelException(exc.getError());
      }

      if (readlen < 0 && readlen != SSH_AGAIN)
        throw SSHTunnelException("unable to read, remote end disconnected");

      if (readlen == 0) {
        if (chan->isClosed())
          throw SSHTunnelException("channel is closed");
        break;
      }

      ssize_t bWritten = 0;
      for (char* buffPtr = buff.data(); readlen > 0 && !_stop; buffPtr += bWritten, readlen -= bWritten) {
        bWritten = send(sock, buffPtr, readlen, MSG_NOSIGNAL);
        if (bWritten <= 0)
          throw SSHTunnelException("unable to write, client disconnected");
      }
    } while (!_stop && true);
  }

  std::unique_ptr<ssh::Channel> SSHTunnelHandler::openTunnel() {
    std::unique_ptr<ssh::Channel> channel(new ssh::Channel(*(_session->getSession())));
    ssh_channel_set_blocking(channel->getCChannel(), false);

    int rc = SSH_ERROR;
    std::size_t i = 0;

    while ((_session->getConfig().connectTimeout * 1000 - (i * 100))  > 0) {
      rc = channel->openForward(_session->getConfig().remotehost.c_str(), _session->getConfig().remoteport,
                              _session->getConfig().localhost.c_str(), _session->getConfig().localport);
      if (rc == SSH_AGAIN) {
        logDebug3("Unable to open channel, wait a moment and retry.\n");
        i++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } else {
        logDebug("Channel successfully opened\n");
        break;
      }
    }

    // If we're here and it's still not ok, we throw exception as we can't open the channel.
    if (rc != SSH_OK)
      throw SSHTunnelException("Unable to open channel");

    return channel;
  }

  void SSHTunnelHandler::prepareTunnel(int clientSocket) {
    std::unique_ptr<ssh::Channel> channel;
    try {
      channel = openTunnel();
    } catch (ssh::SSHTunnelException &exc) {
      wbCloseSocket(clientSocket);
      logError("Unable to open tunnel. Exception when opening tunnel: %s\n", exc.what());
      return;
    } catch (ssh::SshException &exc) {
      wbCloseSocket(clientSocket);
      logError("Unable to open tunnel. Exception when opening tunnel: %s\n", exc.getError().c_str());
      return;
    }

    short events = POLLIN;
    if (ssh_event_add_fd(_event, clientSocket, events, onSocketEvent, this) != SSH_OK) {
      logError("Unable to open tunnel. Could not register event handler.\n");
      channel.reset();
      wbCloseSocket(clientSocket);
      return;
    } else {
      logDebug("Tunnel created.\n");
    }

    _clientSocketList.insert(std::make_pair(clientSocket, std::move(channel)));
    return;
  }

} /* namespace ssh */
