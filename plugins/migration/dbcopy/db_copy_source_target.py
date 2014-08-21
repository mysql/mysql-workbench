# Copyright (c) 2012, 2014, Oracle and/or its affiliates. All rights reserved.
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

import functools

import grt
import mforms

from grt import DBLoginError
from workbench.ui import WizardPage

from migration_source_selection import request_password, test_connectivity


class SourceTargetMainView(WizardPage):
    def __init__(self, main):
        super(SourceTargetMainView, self).__init__(main, 'Connection Selection')
        self._connections = []
        
    def create_ui(self):
        self.content.set_padding(20)

        self.content.add(mforms.newLabel('''Select the connection for the source MySQL server where databases will be copied from
and the destination server where they should be copied to.'''), False, True)

        source_panel = mforms.newPanel(mforms.TitledBoxPanel)
        source_panel.set_title('Source MySQL Server')

        box = mforms.newBox(False)
        box.set_spacing(12)
        box.set_padding(8)
        box.add(mforms.newLabel('Select the connection for the source MySQL server instance'), False, True)
        
        source_hbox = mforms.newBox(True)
        source_hbox.set_spacing(8)
        source_hbox.add(mforms.newLabel('Source Connection:'), False, True)
        self.source_selector = mforms.newSelector()
        source_hbox.add(self.source_selector, True, True)
        box.add(source_hbox, False, True)

        self.source_connection_status = mforms.newLabel('')
        self.source_connection_status.set_color('#aa3333')
        box.add(self.source_connection_status, True, True)
        self.source_selector.add_changed_callback(functools.partial(self.selector_changed, label=self.source_connection_status, selector=self.source_selector))

        source_panel.add(box)
        self.content.add(source_panel, False, True)

        target_panel = mforms.newPanel(mforms.TitledBoxPanel)
        target_panel.set_title('Destination MySQL Server')

        tbox = mforms.newBox(False)
        tbox.set_spacing(12)
        tbox.set_padding(8)
        tbox.add(mforms.newLabel('Select the connection object for the destination MySQL server instance'), False, True)
        
        target_hbox = mforms.newBox(True)
        target_hbox.set_spacing(8)
        target_hbox.add(mforms.newLabel('Target Connection:'), False, True)
        self.target_selector = mforms.newSelector()
        target_hbox.add(self.target_selector, True, True)
        tbox.add(target_hbox, False, True)

        self.target_connection_status = mforms.newLabel('')
        self.target_connection_status.set_color('#aa3333')
        tbox.add(self.target_connection_status, True, True)
        self.target_selector.add_changed_callback(functools.partial(self.selector_changed, label=self.target_connection_status, selector=self.target_selector))
        
        target_panel.add(tbox)
        self.content.add(target_panel, False, True)

        self.advanced_button.set_text('Test Connections')

        self.back_button.set_enabled(False)

        self.load_connections()
    
    def selector_changed(self, label, selector):
        self.connections_ok = False
        label.set_text('')

        if selector.get_selected_index() == selector.get_item_count()-1:
            grt.modules.Workbench.showConnectionManager()
            self.load_connections()

    def load_connections(self):
        def formatConnection(conn):
            i = conn.hostIdentifier
            return i[i.find('@')+1:]

        # filter out ssh connections until its supported
        self._connections = [conn for conn in grt.root.wb.rdbmsMgmt.storedConns if conn.driver and not conn.driver.name.endswith("SSH")]
        selector_items = ( ['Pick a Connection'] +
            ['%s (%s)' % (conn.name, formatConnection(conn)) for conn in self._connections] +
            ['-', 'Edit Connections...'] )

        self.source_selector.clear()
        self.source_selector.add_items(selector_items)
        self.source_connection_status.set_text('')
        self.source_selector.set_selected(0)
        
        self.target_selector.clear()
        self.target_selector.add_items(selector_items)
        self.target_connection_status.set_text('')
        self.target_selector.set_selected(0)

        self.connections_ok = False


    def test_connection(self, source, caption):
        info_label = self.source_connection_status if caption=='Source' else self.target_connection_status
        info_label.set_text('Testing network connectivity...')
        info_label.set_color('#aa3333')
        if test_connectivity(source.connection, 'Test %s DBMS Connection' % caption) == False:
            info_label.set_text('Server could not be contacted')
            return
        info_label.set_text('Testing connection to DBMS...')
 
        force_password = False
        attempt = 0
        while True:
            try:
                if not source.connect():
                    info_label.set_text('Could not connect to DBMS')
                    self.connections_ok = False
                    return
                info_label.set_color('#33aa33')
                info_label.set_text('Connection succeeded.')
                self.connections_ok = True
                break
            except (DBLoginError, SystemError), e:
                if attempt > 0:
                    if isinstance(e, DBLoginError) and not force_password:
                        force_password = True
                    else:
                        etext = str(e)
                        if etext.startswith('Error(') and ': error calling ' in etext:
                            try:
                                etext = eval(etext[7:etext.rfind('):')-1], {}, {})[1]
                            except:
                                pass
                        info_label.set_text('Could not connect to DBMS: %s' % etext)
                        self.connections_ok = False
                        return
                attempt += 1
                source.password = request_password(source.connection, force_password)
            except Exception, e:
                etext = str(e)
                if etext.startswith('Error(') and etext.endswith(')'):
                    etext = eval(etext[6:-1], {}, {})[1]
                info_label.set_text('Could not connect to DBMS: %s' % etext)
                self.connections_ok = False


    @property
    def source_connection(self):
        i = self.source_selector.get_selected_index()
        return  self._connections[i-1] if 0 < i < len(self._connections)+1 else None


    @property
    def target_connection(self):
        i = self.target_selector.get_selected_index()
        return  self._connections[i-1] if 0 < i < len(self._connections)+1 else None


    def go_advanced(self):
        if self.source_connection:
            self.main.plan.setSourceConnection(self.source_connection)
            self.test_connection(self.main.plan.migrationSource, 'Source')
        else:
            self.source_connection_status.set_text('Please select a connection')
            self.source_connection_status.set_color('#aa3333')
            self.connections_ok = False

        if self.target_connection and self.source_connection == self.target_connection:
            self.target_connection_status.set_text('Select different connections for source and target')
            self.target_connection_status.set_color('#aa3333')
            self.connections_ok = False
        elif self.target_connection:
            self.main.plan.setTargetConnection(self.target_connection)
            self.test_connection(self.main.plan.migrationTarget, 'Target')
        else:
            self.target_connection_status.set_text('Please select a connection')
            self.target_connection_status.set_color('#aa3333')
            self.connections_ok = False


    def go_next(self):
        if not self.connections_ok:
            self.go_advanced()
            if not self.connections_ok:
                return

        set_status_text = mforms.App.get().set_status_text
        set_status_text('Connecting to source DBMS...')
        # Set source connection and connect:
        self.main.plan.setSourceConnection(self.source_connection)
        self.main.plan.migrationSource.connect()

        # Set target connection and connect:
        set_status_text('Connecting to target DBMS...')
        self.main.plan.setTargetConnection(self.target_connection)
        self.main.plan.migrationTarget.connect()

        # Fetch schema names:
        set_status_text('Fetching schema names...')
        self.main.plan.migrationSource.doFetchSchemaNames()
        
        set_status_text('Ready')
        super(SourceTargetMainView, self).go_next()
