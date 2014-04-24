# Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; version 2 of the
# License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301  USA

import socket
import threading
import time
import random
import string
from SocketServer import TCPServer
from SocketServer import BaseRequestHandler

class TCPCommandListener(threading.Thread):
  """
  This listener is to use a SocketServer.TCPServer on the logic for listening commands
  the advantage of this version is that is basically a server shipped with python so
  more error prone.

  It requires the implementation of a Handler class which will contain all the handshaking
  logic that was implemented on the original SocketServer (below).

  There's a specific TODO on this version: need to find the way to tell the server to stop
  once the command execution has completed. 
  """

  def __init__(self, output_handler):
    threading.Thread.__init__(self)

    self.handshake = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(8))
    self.close_key = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(8))

    self._server = TCPServer(('127.0.0.1', 0), lambda r, c, s: HandShakeHandler(r, c, s, (self.handshake, self.close_key), output_handler), False)
    self._server.server_bind()

    self.port = self._server.server_address[1]


  def run(self):
    self._server.server_activate()
    self._server.serve_forever()

class HandShakeHandler(BaseRequestHandler):
  """
  This is the command handler needed when using the TCPServer distributed in the python standard library

  It is basically a port of the logic contained on the initial SocketServer class (below) that was
  created for the management of the command executions.
  """

  def __init__(self, request, client_address, server, handshake_keys, output_handler):
    self._handshake = handshake_keys[0]
    self._close_key = handshake_keys[1]

    # If no handshake, authentication is assummed
    self._wait_for_right_client = True if self._handshake else False
    self._client_connected = False
    self._pinger = None
    self._closed_by_client = False
    self._output_handler = output_handler

    BaseRequestHandler.__init__(self, request, client_address, server)

  def authenticate_client(self, handshake):
    if handshake == self._handshake:
      self.request.send("OK")
      self._wait_for_right_client = False
    else:
      self.request.send("ERROR")
      self.request.close()
      self._client_connected = False

  def _ping(self):
    if self._client_connected:
      try:
        # When client still connected this should succeed
        self.request.send(".")
        self._pinger = threading.Timer(1, self._ping)
        self._pinger.start()
      except Exception, e:
        self.exit_status = 1
        self.exit_message = repr(e)
        self._client_connected = False
        # TODO: Find the correct way to shutdown the server once 
        #       it has completed...
        #self.server.shutdown_request(self.request)
        #self.server.shutdown()


  def handle(self):
    # The server will be waiting for client connections while nothing indicates 
    # The stablished criteria to stop the server are:
    # - When the thread is stopped
    # - When a connected client sends the close_key
    # - When authentication occurred (the right client was connected) and the connection got closed
    keep_alive = True
    self._pinger = threading.Timer(1, self._ping)
    self._pinger.start()

    # Once a client is connected the server will process the incoming data
    # There are three scenarios:
    # - The incoming data is a handshake to identify the server should listen that
    #   specific client
    # - The incoming message is a message indicating the client has disconnected
    # - The incoming message is a message indicating the server should STOP listening
    # - The incoming message is just data that needs processing
    self._client_connected = True
    while self._client_connected:
      try:
        data = self.request.recv(1024)
            
        if data:
          # If authentication is needed
          if self._wait_for_right_client:
            self.authenticate_client(data)

          # The client has explicitly indicated its processing
          # is done
          elif self._close_key and self._close_key == data:
            self._closed_by_client = True
            self._client_connected = False
            # TODO: Find the correct way to shutdown the server once 
            #       it has completed...
            #self.server.shutdown_request(self.request)
            #self.server.shutdown()
          else:
            self.process_data(data)

      except Exception, e:
          # Error 10054 indicates the client connection terminated
          print "EXCEPTION : ", e
          #if e.errno == 10054:
          #  self._client_connected = False
          #else:
          #  raise


  def process_data(self, data):
    self._output_handler(data)

class SocketServer(threading.Thread):
  """
  Custom socket server class handling:
  - optional handshake with connected client
  - optional server shutdown based on close_key received by the client

  This class is currently used to manage the command execution through the
  admin helper script which communicates to this server by using an instance of
  the SocketClient class below
  """

  def __init__(self, host, port, handshake = "", close_key = ""):
    threading.Thread.__init__(self)
    
    self.host = host
    self.port = port
    self._handshake = handshake
    self._close_key = close_key

    # If no handshake, authentication is assummed
    self._wait_for_right_client = True if self._handshake else False
    self._client_connected = False
    self._pinger = None
    self._closed_by_client = False
    self._bound = False

    self.exit_status = 0
    self.exit_message = ""

    self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  def bind(self, port, catch_error = True):
    self.port = port
    try:
      self._socket.bind((self.host, self.port))
      self.port = self._socket.getsockname()
      self._bound = True
    except Exception, e:
      self.exit_status = 1
      self.exit_message = repr(e)


  def authenticate_client(self, handshake):
    if handshake == self._handshake:
      self._connection.send("OK")
      self._wait_for_right_client = False
    else:
      self._connection.send("ERROR")
      self._connection.close()
      self._client_connected = False

  def _ping(self):
    if self._client_connected:
      try:
        # When client still connected this should succeed
        self._connection.send(".")
        self._pinger = threading.Timer(1, self._ping)
        self._pinger.start()
      except Exception, e:
        self.exit_status = 1
        self.exit_message = repr(e)
        self._client_connected = False


  def run(self):

    try:
      if not self._bound:
        self.bind(self.port, False)

      # The server will be waiting for client connections while nothing indicates 
      # The stablished criteria to stop the server are:
      # - When the thread is stopped
      # - When a connected client sends the close_key
      # - When authentication occurred (the right client was connected) and the connection got closed
      keep_alive = True
      while self._wait_for_right_client:
        self._socket.listen(1)
        self._connection, self._address = self._socket.accept()

        # At this point a client is connected
        self._client_connected = True
        self._pinger = threading.Timer(1, self._ping)
        self._pinger.start()

        # Once a client is connected the server will process the incoming data
        # There are three scenarios:
        # - The incoming data is a handshake to identify the server should listen that
        #   specific client
        # - The incoming message is a message indicating the client has disconnected
        # - The incoming message is a message indicating the server should STOP listening
        # - The incoming message is just data that needs processing

        while self._client_connected:
          try:
            data = self._connection.recv(1024)
            
            if data:
              # If authentication is needed
              if self._wait_for_right_client:
                self.authenticate_client(data)

              # The client has explicitly indicated its processing
              # is done
              elif self._close_key and data.find(self._close_key) == 0:
                self._closed_by_client = True
                self._client_connected = False
                self._connection.close()

                # The closing key comes in the format of:
                # <close_key> <status>[ <message>]
                # Where:
                #   - close_key is the token generated on this class to notify it that the server should be stopped
                #   - status : 0 if all ended OK on the client. Non 0 value on other case
                #   - message: information about the ocurred error
                key, return_data = data.split(' ', 1)
                if not return_data.startswith('0'):
                  ret_code, error = return_data.split(' ', 1)
                  self.exit_status = 1
                  self.exit_message = error
                  
              else:
                self.process_data(data)

          except Exception, e:
            self.exit_status = 1
            self.exit_message = repr(e)
            self._client_connected = False

    # Any error starting the server will be printed
    except socket.error, e:
      self.exit_status = 1
      self.exit_message = repr(e)

  def process_data(self, data):
    pass


class CustomCommandListener(SocketServer):
    """
    This is the implementationn of a socket server which will be listening for commands
    executed as admin in local windows, the output is received through a socket
    it is used only when the output is required and all the received data will be sent
    to the configured output handler
    """
    def __init__(self, output_handler):

        self.handshake = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(8))
        self.close_key = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(8))
        
        SocketServer.__init__(self, '127.0.0.1', 0, self.handshake, self.close_key)

        SocketServer.bind(self, 0)

        self.host, self.port = self._socket.getsockname()

        self._handler = output_handler

    def process_data(self, data):
        self._handler(data)


class SocketClient(object):
  """
  Custom socket client class handling:
  - optional handshake with server
  - optional server shutdown based on close_key sent by the client
  """

  def __init__(self, host, port, handshake = "", close_key = ""):
    self._host = host
    self._port = port
    self._handshake = handshake
    self._close_key = close_key
    self._connected = False
    self._authenticated = None


  def start(self):
    self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
      self._socket.connect((self._host, self._port))

      self._connected = True
    
      # If a handshake is needed the client sends it and waits
      # for the server's answer
      if self._handshake:
          self._socket.send(self._handshake)
        
          # Validates the server response
          validated = None
          while self._authenticated is None:
              response = self._socket.recv(1024)
              if response:
                  self._authenticated = (response == "OK")
                
                  if not self._authenticated:
                      self._connected = False
                      self.close()
    except socket.error, e:
      # Connection was not done, probably target is not listening
      # This error just gets printed as there's no way to tell the
      # server
      if e.errno == 10061:
        print e

    return self._connected
                

  def send(self, data):
    if self._connected:
        self._socket.send(data)

        
  def close(self, exit_status = 0, msg = ""):
    if self._connected:

        # The server is waiting to receive the closing token alone
        # This delay is inserted so the server reads any pending data
        time.sleep(2)
        if self._close_key and self._authenticated:
            closing_message = "%s %d %s" % (self._close_key, exit_status, msg)
            closing_message.strip()
            self._socket.send(closing_message)
            
        self._socket.close()
        self._connected = False
        
        
