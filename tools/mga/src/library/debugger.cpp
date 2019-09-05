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

#include "common.h"
#include "utilities.h"
#include "scripting-context.h"

#include "debugger.h"

using namespace mga;

//----------------------------------------------------------------------------------------------------------------------

DebugAdapter::DebugAdapter(ScriptingContext *context, int port) : _port(port) {
  initializeAndWait();

  duk_debugger_attach(context->_ctx, readCallback, writeCallback, peekCallback, nullptr, nullptr, nullptr,
                      debuggerDetached, this);
}

//----------------------------------------------------------------------------------------------------------------------

bool DebugAdapter::isOpen() {
#if defined (_MSC_VER)
  return _clientSocket != INVALID_SOCKET;
#else
  return _clientSocket >= 0;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void DebugAdapter::close() {
  // Can be called from the read/write callbacks, so mark closed sockets properly.

#if defined (_MSC_VER)
  closesocket(_serverSocket);
  _serverSocket = INVALID_SOCKET;
  closesocket(_clientSocket);
  _clientSocket = INVALID_SOCKET;
  WSACleanup();
#else
  ::close(_serverSocket);
  _serverSocket = -1;
  ::close(_clientSocket);
  _clientSocket = -1;
#endif
}

//----------------------------------------------------------------------------------------------------------------------

void DebugAdapter::initializeAndWait() {

#if defined(_MSC_VER)

  _serverSocket = INVALID_SOCKET;
  _clientSocket = INVALID_SOCKET;

  WSADATA wsaData;

  memset((void *) &wsaData, 0, sizeof(wsaData));
  int rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (rc != 0)
    throw std::runtime_error("Debug adapter: WSAStartup() failed: " + std::to_string(rc));

#define CLEANUP() \
  if (result != nullptr) \
    freeaddrinfo(result); \
  WSACleanup();

  struct addrinfo hints;
  memset((void *) &hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *result = nullptr;
  rc = getaddrinfo("0.0.0.0", std::to_string(_port).c_str(), &hints, &result);
  if (rc != 0) {
    CLEANUP()
    throw std::runtime_error("Debug adapter: getaddrinfo() failed: " + std::to_string(rc));
  }

  _serverSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (_serverSocket == INVALID_SOCKET) {
    CLEANUP()
    throw std::runtime_error("Debug adapter: socket() failed with error: " + std::to_string(WSAGetLastError()));
  }

  rc = bind(_serverSocket, result->ai_addr, (int)result->ai_addrlen);
  if (rc == SOCKET_ERROR) {
    CLEANUP()
    throw std::runtime_error("Debug adapter: bind() failed with error: " + std::to_string(WSAGetLastError()));
  }

  rc = listen(_serverSocket, SOMAXCONN);
  if (rc == SOCKET_ERROR) {
    CLEANUP()
    throw std::runtime_error("Debug adapter: listen() failed with error: " + std::to_string(WSAGetLastError()));
  }

  if (result != nullptr)
    freeaddrinfo(result);

  std::cout << "Waiting for debug connection on port " << _port << "..." << std::endl;

  _clientSocket = accept(_serverSocket, nullptr, nullptr);
  if (_clientSocket == INVALID_SOCKET) {
    closesocket(_serverSocket);
    throw std::runtime_error("Debug adapter: accept() failed: " + std::to_string(WSAGetLastError()));
  }

#else

  _serverSocket = -1;
  _clientSocket = -1;

  _serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (_serverSocket < 0)
    throw std::runtime_error("Debug adapter: failed to create server socket: " + Utilities::getLastError());

  int on = 1;
  if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on)) < 0) {
    ::close(_serverSocket);
    throw std::runtime_error("Debug adapter: failed to set SO_REUSEADDR for server socket: " + Utilities::getLastError());
  }

  struct sockaddr_in addr;
  memset((void *) &addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(_port);

  if (bind(_serverSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    ::close(_serverSocket);
    throw std::runtime_error("Debug adapter: failed to bind server socket: " + Utilities::getLastError());
  }

  listen(_serverSocket, 1);

  std::cout << "Waiting for debug connection on port " << _port << " ... " << std::endl;
  socklen_t length = (socklen_t)sizeof(addr);
  _clientSocket = accept(_serverSocket, (struct sockaddr *)&addr, &length);
  if (_clientSocket < 0) {
    ::close(_serverSocket);
    throw std::runtime_error("Debug adapter: accept() failed: " + Utilities::getLastError());
  }

#endif
}

//----------------------------------------------------------------------------------------------------------------------

void DebugAdapter::debuggerDetached(duk_context *, void *data) {
  // Ensure socket is closed even when detach is initiated by Duktape rather than debug client.
  DebugAdapter *adapter = static_cast<DebugAdapter *>(data);
  adapter->close();
}

//----------------------------------------------------------------------------------------------------------------------

duk_size_t DebugAdapter::readCallback(void *data, char *buffer, duk_size_t length) {
  if (data == nullptr || length == 0 || buffer == nullptr)
    return 0;

  DebugAdapter *adapter = static_cast<DebugAdapter *>(data);
  if (!adapter->isOpen())
    return 0;

#ifdef _MSC_VER
  int len = static_cast<int>(length);
#else
  size_t len = static_cast<size_t>(length);
#endif

  ssize_t ret = recv(adapter->_clientSocket, buffer, len, 0);
  if (ret < 0) {
    adapter->close();
    std::cerr << "Debug adapter: read failed, closing connection: " << Utilities::getLastError() << std::endl;
    return 0;
  } else if (ret == 0) {
    adapter->close();
    std::cerr << "Debug adapter: no more data, closing connection: " << Utilities::getLastError() << std::endl;
    return 0;
  } else if (ret > (ssize_t) length) {
    adapter->close();
    std::cerr << "Debug adapter: too much data was sent, closing connection" << std::endl;
    return 0;
  }

  return static_cast<duk_size_t>(ret);
}

//----------------------------------------------------------------------------------------------------------------------

duk_size_t DebugAdapter::writeCallback(void *data, const char *buffer, duk_size_t length) {
  if (data == nullptr || length == 0 || buffer == nullptr)
    return 0;

  DebugAdapter *adapter = static_cast<DebugAdapter *>(data);
  if (!adapter->isOpen())
    return 0;

#if defined(_MSC_VER)
  ssize_t ret = send(adapter->_clientSocket, buffer, (int)length, 0);
#else
  ssize_t ret = write(adapter->_clientSocket, buffer, length);
#endif
  
  if (ret <= 0 || ret > (ssize_t)length) {
    adapter->close();
    std::cerr << "Debug adapter: write failed, closing connection: " << Utilities::getLastError() << std::endl;
    return 0;
  }

  return (duk_size_t) ret;
}

//----------------------------------------------------------------------------------------------------------------------

duk_size_t DebugAdapter::peekCallback(void *data) {

  if (data == nullptr)
    return 0;

  DebugAdapter *adapter = static_cast<DebugAdapter *>(data);
  if (!adapter->isOpen())
    return 0;

#if defined (_MSC_VER)

  u_long avail = 0;
  int rc = ioctlsocket(adapter->_clientSocket, FIONREAD, &avail);
  if (rc != 0) {
    adapter->close();
    std::cerr << "Debug adapter: peek error, closing connection: " << Utilities::getLastError() << std::endl;
    return 0;
  } else {
    if (avail == 0)
      return 0;
  }

#else

  struct pollfd fds[1];
  fds[0].fd = adapter->_clientSocket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  int pollResult = poll(fds, 1, 0);
  if (pollResult < 0) {
    adapter->close();
    std::cerr << "Debug adapter: peek error, closing connection: " << Utilities::getLastError() << std::endl;
  } else if (pollResult == 0) {
    return 0;
  }

#endif

  return 1;
}

//----------------------------------------------------------------------------------------------------------------------
