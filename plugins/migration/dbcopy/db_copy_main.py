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

import locale
import platform
import mforms

import migration

import db_copy_overview
import db_copy_source_target
import db_copy_schema_selection
import db_copy_progress
import db_copy_report


#===============================================================================
#
#===============================================================================
class DBCopy(mforms.AppView):
    def __init__(self):
        mforms.AppView.__init__(self, False, 'db_copy', True)

        self.background = None
        self.content = mforms.newBox(False)

        if platform.system() == 'Windows':
            self.set_back_color("#FFFFFF")
            content_panel = mforms.newPanel(mforms.FilledPanel)
            #self.content.set_back_image("migration_background.png", mforms.TopRight)
            content_panel.set_back_color("#FFFFFF")
        else:
            content_panel = mforms.newPanel(mforms.StyledHeaderPanel)
            content_panel.set_back_image("migration_background.png", mforms.TopRight)
          
        content_panel.set_padding(8)
        self.header = mforms.newLabel("")
        self.header.set_style(mforms.WizardHeadingStyle)
        self.content.add(self.header, False)

        # On Windows doesn't use TabView as it is not transparent
        if platform.system() != 'Windows':
            self.tabview = mforms.newTabView(mforms.TabViewTabless)
            self.content.add(self.tabview, True, True)

        content_panel.add(self.content)
        self.add(content_panel, True, True)

        self._ui_created = False
        
        self._page_list = []
        self._page_trail = []
        self._current_page = 0

        # Load current user numeric locale:
        locale.setlocale(locale.LC_NUMERIC, '')

        self.plan = migration.MigrationPlan()
        self.create_ui()


    def advanced(self):
        self._page_trail[-1].advanced_clicked()


    def reset(self):
        self._current_page = 0
        self._page_trail = [self._page_list[0]]
        self.switch_page(advancing=True)


    def create_ui(self):
        if self._ui_created:
            return

        self._ui_created = True

        self._overview_page = db_copy_overview.MainView(self)
        self._source_target_page = db_copy_source_target.SourceTargetMainView(self)
        self._schema_selection_page = db_copy_schema_selection.SchemaMainView(self)
        self._progress_page = db_copy_progress.ProgressMainView(self)
        self._report_page = db_copy_report.ReportMainView(self)

        self._page_list = [self._overview_page,
                           self._source_target_page,
                           self._schema_selection_page,
                           self._progress_page,
                           self._report_page
                          ]

        if platform.system() == 'Windows':
            for p in self._page_list:
                self.content.add(p, True, True)
                p.show(False)
        else:
            for p in self._page_list:
                self.tabview.add_page(p, "")

        self._page_trail = [self._page_list[self._current_page]]
        self.switch_page(advancing=True)


    def switch_page(self, advancing):
        curpage = self._page_trail[-1]

        if platform.system() == 'Windows':
            for p in self._page_list:
                if p == curpage:
                    p.page_activated(advancing)
                p.show(p == curpage)
        else:
            i = self._page_list.index(curpage)
            self.tabview.set_active_tab(i)
            curpage.page_activated(advancing)


    def go_next_page(self):
        self._current_page += 1
        self._page_trail.append(self._page_list[self._current_page])
        self.switch_page(advancing=True)


    def go_previous_page(self):
        if len(self._page_trail) > 1:
            self._page_trail.pop()
            self._current_page = self._page_list.index(self._page_trail[-1])
            self.switch_page(advancing=False)


    def close(self):
        # Restore default locale:
        locale.setlocale(locale.LC_NUMERIC, 'C')
        super(DBCopy, self).close()
        
    
    def cleanup(self):
        if self.plan:
            self.plan.close()
        self.plan = None
