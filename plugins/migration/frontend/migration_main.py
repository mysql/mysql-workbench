# Copyright (c) 2012, 2016, Oracle and/or its affiliates. All rights reserved.
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

import sys
import os
import locale
import platform

import grt
import mforms

import migration

import migration_ui_style
import migration_toolbars
import migration_overview
import migration_project_management

import migration_source_selection
import migration_schema_selection
import migration_object_selection
import migration_object_migration
import migration_object_editing
import migration_schema_creation
import migration_data_transfer
import migration_summary


def plat_icon(icon):
    prefix, ext = os.path.splitext(icon)
    if sys.platform != "darwin" and not prefix.endswith("_win"):
        icon = prefix+"_win" + ext
    return icon

#===============================================================================
#
#===============================================================================
class Migration(mforms.AppView):
    def __init__(self):
        mforms.AppView.__init__(self, True, "migration", True)
        
        self.plan = migration.MigrationPlan()

        self.toolbars_mgr = migration_toolbars.ToolBarManager()
        self.active_tab = None
        self.old_active_tab = None

        self.set_spacing(6)
        
        self.ui_profile = migration_ui_style.UIProfile()

        self.left_side_cont = self.create_tasks_side()
        self.add(self.left_side_cont, False, True)
        self.tasks_side.add_section("Overview", "OVERVIEW", 0)
        self.tasks_side.add_section("SourceTarget", "SOURCE & TARGET", 0)
        self.tasks_side.add_section("ObjectMigration", "OBJECT MIGRATION", 0)
        self.tasks_side.add_section("DataMigration", "DATA MIGRATION", 0)
        self.tasks_side.add_section("Report", "REPORT", 0)

        self.tasks_side.add_on_section_command_callback(self.section_clicked)
        self.left_side_cont.set_size(220, -1)

        self.background = mforms.newPanel(mforms.StyledHeaderPanel)
        self.background.set_title("TITLE")
                
        self._page_trail = []
        self.tabs      = []
        self._name2page = {}

        if platform.system() == 'Windows':
            self.content_box = mforms.newBox(False)
            self.content_box.set_back_image("migration_background.png", mforms.TopRight)
            self.background.add(self.content_box)
            self.background.set_back_color("#ffffff")
        else:
            self.tabview   = mforms.newTabView(True)
            self.background.add(self.tabview)
            self.background.set_back_image("migration_background.png", mforms.TopRight)

        self.add(self.background, True, True)

        self._advancing = True
        self.content = {}
        self.add_content()
      
        self._selecting_entry = False
        
        # Load current user numeric locale:
        locale.setlocale(locale.LC_NUMERIC, '')
        
        self.tasks_side.select_entry(self._overview_page.identifier())
        self.section_clicked(self._overview_page.identifier())

    #---------------------------------------------------------------------------
    def close(self):
        # Restore default locale:
        locale.setlocale(locale.LC_NUMERIC, 'C')
        if self.on_close:
            self.on_close()
        super(Migration, self).close()
        
    
    def cleanup(self):
        if self.plan:
            self.plan.close()
        self.plan = None

    #---------------------------------------------------------------------------
    def add_content(self):
        self._overview_page = migration_overview.MainView(self)
        self.content[self._overview_page.identifier()] = self._overview_page

        #cont = migration_project_management.MainView(self)
        #self.content["project_management"] = cont


        cont = migration_source_selection.SourceMainView(self)
        self.content[cont.identifier()] = cont
        self._page_trail = [cont] # this is the initial wizard page

        cont = migration_source_selection.TargetMainView(self)
        self.content[cont.identifier()] = cont
        
        cont = migration_source_selection.FetchProgressView(self)
        self.content[cont.identifier()] = cont

        cont = migration_schema_selection.SchemaMainView(self)
        self.content[cont.identifier()] = cont

        cont = migration_schema_selection.ReverseEngineerProgressView(self)
        self.content[cont.identifier()] = cont

        cont = migration_object_selection.ObjectMainView(self)
        self.content[cont.identifier()] = cont
        
        #cont = migration_object_migration.MigrationOptionsView(self)
        #self.content[cont.identifier()] = cont
        
        cont = migration_object_migration.MigrationProgressView(self)
        self.content[cont.identifier()] = cont

        cont = migration_object_editing.MainView(self)
        self.content[cont.identifier()] = cont

        cont = migration_schema_creation.MainView(self)
        self.content[cont.identifier()] = cont

        cont = migration_schema_creation.CreationProgressView(self)
        self.content[cont.identifier()] = cont

        cont = migration_schema_creation.CreationReportView(self)
        self.content[cont.identifier()] = cont

        cont = migration_data_transfer.SetupMainView(self)
        self.content[cont.identifier()] = cont

        cont = migration_data_transfer.TransferMainView(self)
        self.content[cont.identifier()] = cont

        cont = migration_summary.FinalReportView(self)
        self.content[cont.identifier()] = cont


    #---------------------------------------------------------------------------
    def create_tasks_side(self):
        side_cont = mforms.newPanel(mforms.StyledHeaderPanel)
        side_cont.set_title("Migration Task List")
        self.tasks_side = mforms.newTaskSidebar("Simple")

        side_cont.add(self.tasks_side)
        return side_cont

    #---------------------------------------------------------------------------
    def section_clicked(self, entry_id):
        if self._selecting_entry:
            return
        
        new_active = self.content[entry_id]

        if new_active != self.active_tab:
            self._selecting_entry = True

            self.old_active_tab = self.active_tab
            self.active_tab = new_active

            if platform.system() == 'Windows':
                if self.old_active_tab:
                    self.old_active_tab.show(False)

                self.active_tab.show(True)
            else:
                i = self._name2page.get(entry_id, None)
                self.tabview.set_active_tab(i)

            self.tab_changed()
            self.tasks_side.select_entry(entry_id)

            self._selecting_entry = False

    #---------------------------------------------------------------------------
    def tab_changed(self):
        if self.old_active_tab and hasattr(self.old_active_tab, "page_deactivated"):
            self.old_active_tab.page_deactivated()

        if self.active_tab is not None and hasattr(self.active_tab, "page_activated"):
            self.active_tab.page_activated(self._advancing)

    #---------------------------------------------------------------------------
    def add_content_page(self, page, section_id, item_name, icon_name):
        entry = self.tasks_side.add_section_entry(section_id, page.identifier(), item_name, plat_icon(icon_name + ".png"), mforms.TaskEntryLink)
        
        if platform.system() == 'Windows':
            self.content_box.add(page, True, True)
            i = len(self._name2page)
            page.show(False)
        else:
            i = self.tabview.add_page(page, "")

        self.tabs.append(page)
        self._name2page[page.identifier()] = i

    #---------------------------------------------------------------------------
    def add_wizard_page(self, page, section_id, item_name):
        entry = self.tasks_side.add_section_entry(section_id, page.identifier(), item_name, plat_icon("migration_check_open.png"), mforms.TaskEntryPlainItem)
        
        if platform.system() == 'Windows':
            self.content_box.add(page, True, True)
            i = len(self._name2page)
            page.show(False)
        else:
            i = self.tabview.add_page(page, "")

        self.tabs.append(page)
        self._name2page[page.identifier()] = i

    #---------------------------------------------------------------------------
    
    
    def can_go_back(self):
        return len(self._page_trail) > 1

    def can_go_next(self):
        return not self._page_trail or self._page_trail[-1] != self.tabs[-1]


    def start(self):
        self._advancing = True

        next = self.tabs[1]
        self._page_trail = [next]
        self.tasks_side.set_section_entry_icon(next.identifier(), plat_icon("migration_check_current.png"))
        self.tasks_side.select_entry(next.identifier())
        self.section_clicked(next.identifier())
        return next


    def go_next_page(self, skip_count=1):
        self._advancing = True
    
        current = self._page_trail[-1]
        self.tasks_side.set_section_entry_icon(current.identifier(), plat_icon("migration_check_done.png"))
        
        i = self.tabs.index(current)+skip_count
        next = self.tabs[i]
        while True:
            if not next.should_skip():
                break
            next.page_skipped()
            i += 1
            next = self.tabs[i]
        self._page_trail.append(next)
        self.tasks_side.set_section_entry_icon(next.identifier(), plat_icon("migration_check_current.png"))
        self.tasks_side.select_entry(next.identifier())
        self.section_clicked(next.identifier())
        return next


    def go_previous_page(self):
        self._advancing = False

        current = self._page_trail[-1]
        
        self._page_trail.pop()
        next = self._page_trail[-1]
        self.tasks_side.set_section_entry_icon(next.identifier(), plat_icon("migration_check_current.png"))
        self.tasks_side.select_entry(next.identifier())
        self.section_clicked(next.identifier())
        return next

