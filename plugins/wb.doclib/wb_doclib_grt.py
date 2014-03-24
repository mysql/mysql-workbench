# Copyright (c) 2010, 2013, Oracle and/or its affiliates. All rights reserved.
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

from wb import DefineModule
import grt

from mforms import Utilities, ResultOk, AppView, newWebBrowser, App
import mforms

import sys
import platform
import thread
import socket
import os
import time
from threading import Event


ModuleInfo = DefineModule(name= "WbDocLib", author= "Oracle", version="1.0")


docLibTab = None

class DocLibTab(AppView):
    def __init__(self, server_port):
        AppView.__init__(self, False, "doclib", True)

        self.browser = newWebBrowser()
        self.add(self.browser, True, True)
        self.browser.add_loaded_callback(self.loaded)
        self.browser.navigate("http://localhost:%i" % server_port)
        
        self.on_close(self.handle_on_close)
      
    def handle_on_close(self):
        global docLibTab
        App.get().set_status_text("Closed Doc Library")
        docLibTab = None
        return True

    
    def loaded(self, url):
        App.get().set_view_title(self, "Doc Library")
        App.get().set_status_text("Doc Library Opened")


def get_free_port(bind = ''):
        import socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind((bind,0)) # binding to port zero will actually bind to a free user port
        port = s.getsockname()[1]
        s.close()
        return port


def run_server(datadir, ready_event):
    import mysqldoclib
    import pprint
    global server_port;
    
    retries = 10 

    while server_port is None and --retries > 0:
        try:
            server_port = get_free_port() 
            # there is a slight chance that the port gets taken before we use it again
            # which is the main reason for the retry loop
            mysqldoclib.serve_docs(server_port, bind='localhost', datadir=datadir, ready_event=ready_event)
        except:
            server_port = None
    

server_port = None

@ModuleInfo.plugin("wb.doclib.open", type="standalone", caption= "Open Documentation Library",  pluginMenu= "Extensions")
@ModuleInfo.export(grt.INT)
def openDocLib():
    global docLibTab
    global server_port

    app = App.get()

    # if docs datafiles are not installed, just open the docs home page
    datafile = os.path.join(app.get_resource_path(""), "modules/data/DocLibrary/mysqldoclib.sqlite")
    if not os.path.exists(datafile):
        # Go to the documentation of the latest GA available
        Utilities.open_url("http://dev.mysql.com/doc/refman/5.6/en/")
        return 1

    if docLibTab:
      if docLibTab is True: # this will be True if an external browser is used
          Utilities.open_url("http://localhost:%i"%server_port)
          return 1
      app.select_view(docLibTab)
      return 1

    try:
        import mysqldoclib
    except ImportError:
        Utilities.show_error("Cannot Open Documentation Library", 
                    '''pysqlite2 is not installed, please install python-sqlite2 or pysqlite2 to be able to use this feature.
Try running "easy_install pysqlite" with superuser privileges in the command line shell or, if using
Ubuntu, enable the Universe repository and install the python-pysqlite2 package from there.''',
                    "OK", "", "")
        return 0
    
    if server_port is None:
        ready_event = Event()

        #datadir = "./modules/data/DocLibrary/"
        datadir = os.path.join(app.get_resource_path(""), "modules/data/DocLibrary")

        thread.start_new_thread(run_server, (datadir, ready_event))    

        # wait up to 1s for the doclib server to start
        ready_event.wait(1)

    if platform.system() == "Linux":
        docLibTab = True
        Utilities.open_url("http://localhost:%i"%server_port)
        return 1
    docLibTab = DocLibTab(server_port)
    docLibTab.set_identifier("wb.doclib")
    
    app.dock_view(docLibTab, "maintab")
    app.set_view_title(docLibTab, "Doc Library (loading)")

    app.set_status_text("Opening Doc Library...")

    return 1

