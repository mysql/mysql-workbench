# Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.
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

import itertools

import mforms
import grt

from workbench.ui import WizardPage


class ReportMainView(WizardPage):

    def __init__(self, main):
        super(ReportMainView, self).__init__(main, 'Results')


    def create_ui(self):
        self.content.set_padding(20)

        self._report = mforms.newTextBox(mforms.VerticalScrollBar)
        self.content.add(self._report, True, True)
        
        self.next_button.set_text('Finish')
        
        
    def page_activated(self, advancing):
        super(ReportMainView, self).page_activated(advancing)
        if advancing:
            self.generate_report()


    def generate_report(self):
        self._schema_list = self.main._schema_selection_page.schema_selector.get_selected()
        self._report.set_value('%s transfered.\n' % ('1 schema' if len(self._schema_list)== 1 else
                                                     str(len(self._schema_list)) + ' schemas') )
        for schema in self._schema_list:
            self._report.append_text(str('\n' + 30*'=' + '\n' + ('Schema: ' + schema).center(30) + '\n' + 30*'=' + '\n'))
            try:
                idx = [log.logObject.owner.name for log in grt.root.wb.migration.dataTransferLog].index(schema)
            except ValueError:
                grt.log_warning('Wizard', 'Data transfer log entries for schema "%s" not found' % schema)
                continue
            schema_object = grt.root.wb.migration.dataTransferLog[idx].logObject.owner
            self._report.append_text('\tTables: %d\n\tViews: %d\n\tRoutines: %d\n' % (len(schema_object.tables),
                                                                                      len(schema_object.views),
                                                                                      len(schema_object.routines) )
                                    )

            self._report.append_text('\nData copy report:\n')
            for log in itertools.islice(grt.root.wb.migration.dataTransferLog, idx, None):
                if log.logObject.owner.__id__ != schema_object.__id__:
                    break
                self._report.append_text('\n'.join('\t' + entry.name for entry in log.entries) + '\n')

            self._report.append_text('\n')

        

    def go_next(self):
        self.main.close()
