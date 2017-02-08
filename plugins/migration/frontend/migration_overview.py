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

import mforms
import grt
from workbench.ui import WizardPage
from workbench.utils import server_version_str2tuple

class MainView(WizardPage):
    def __init__(self, main):
        WizardPage.__init__(self, main, "Overview", no_buttons=True)

        self.main = main

        self.main.add_content_page(self, "Overview", "Overview", "migration")

    def create_ui(self):
        label = mforms.newLabel("Welcome to the MySQL Workbench Migration Wizard")
        label.set_style(mforms.BigBoldStyle)
        self.content.add(label, False, True)
        self.content.set_spacing(12)
        self.content.set_padding(20)

        label = mforms.newLabel("This wizard will assist you in migrating tables and data from a supported database system to MySQL.\n"+
              "You can also use this wizard to copy databases from one MySQL instance to another.")
        self.content.add(label, False, True)
        label = mforms.newLabel("Prerequisites")
        label.set_style(mforms.BoldStyle)
        self.content.add(label, False, True)
        label = mforms.newLabel("Before starting, check the following preparation steps:\n\n"+
              "- The Migration Wizard uses ODBC to connect to the source database. You must have an ODBC driver for\n"
              "the source database installed and configured, as Workbench does not bundle any such drivers.\n"
              "For MySQL connections, the native client library is used.\n\n"+
              "- Ensure you can connect to both source and target RDBMS servers.\n\n"+
              "- Make sure you have privileges to read schema information and data from the source database and\n"+
              "create objects and insert data in the target MySQL server.\n\n"+
              "- The max_allowed_packet option in the target MySQL server must be large enough to fit\n"+ 
              "the largest field value to be copied from source (especially BLOBs and large TEXT fields).\n\n"+
              "\n"+
              "The wizard supports migrating from specific database systems, but a \"generic\" RDBMS support is also provided.\n"+
              "The generic support is capable of migrating tables from many RDBMS that can be connected to using ODBC,\n"+
              "although certain type mappings may not be performed correctly. A manual mapping step is provided for\n"+
              "reviewing and fixing any migration problems that could occur.")
        self.content.add(label, False, True)

        box = mforms.newBox(True)
        box.add(mforms.newLabel(""), True, True)
        button_start = mforms.newButton()
        button_start.set_text("Start Migration")
        button_start.add_clicked_callback(self.start)
        box.add(button_start, True, True)
        box.add(mforms.newLabel(""), True, True)
        button_odbc = mforms.newButton()
        button_odbc.set_text("Open ODBC Administrator")
        button_odbc.add_clicked_callback(self.start_odbc)
        box.add(button_odbc, True, True)
        box.add(mforms.newLabel(""), True, True)
        button_doc = mforms.newButton()
        button_doc.set_text("View Documentation")
        button_doc.add_clicked_callback(lambda: mforms.Utilities.open_url('http://dev.mysql.com/doc/workbench/en/wb-migration.html'))
        box.add(button_doc, True, True)
        box.add(mforms.newLabel(""), True, True)
        self.content.add_end(box, False, True)


    def start_odbc(self):
        if not grt.modules.PyWbUtils.startODBCAdmin():
            mforms.Utilities.show_error("Open ODBC Administrator", "ODBC Administrator utility could not be found.", "OK", "", "")
            

    def create_uix(self):
        container = mforms.newBox(True)
        container.set_spacing(30)

        left_side_box = mforms.newBox(False)
        left_side_box.set_padding(8)
        logo_image = mforms.newImageBox()
        logo_image.set_image('migration_logo.png')
        left_side_box.add(logo_image, False, True)
        container.add(left_side_box, False, True)

        # Main layout structure
        content = mforms.newBox(False)
        content.set_padding(8)
        content.set_spacing(12)
        title_image = mforms.newImageBox()
        title_image.set_image('migration_title.png')
        content.add(title_image, False, True)

        help_label = mforms.newLabel('''To perform a new migration click the [Start New Migration] button below. To re-run
a previous migration or to perform a new migration based on a previous
migration please double click one of the migration projects below.''')
        content.add(help_label, False, True)

        wrapper_button_box = mforms.newBox(True)
        wrapper_button_box.set_padding(8)
        button_new_migration = mforms.newButton()
        button_new_migration.set_text('Start New Migration')
        #button_new_migration.add_clicked_callback(lambda x: x)
        wrapper_button_box.add(button_new_migration, False, True)
        content.add(wrapper_button_box, False, True)

        project_box = mforms.newBox(False)
        project_box.set_spacing(8)
        project_label = mforms.newLabel('Project Overview')
        project_label.set_style(mforms.BoldStyle)
        project_box.add(project_label, False, True)
        project_tree = mforms.newTreeView(mforms.TreeDefault)
        project_box.add(project_tree, True, True)
        project_button_box = mforms.newBox(True)
        project_button_box.set_spacing(8)
        button_rerun_migration = mforms.newButton()
        button_rerun_migration.set_text('Re-Run Migration')
        button_edit_migration = mforms.newButton()
        button_edit_migration.set_text('Edit Migration Project')
        project_button_box.add(button_rerun_migration, False, True)
        project_button_box.add(button_edit_migration, False, True)
        project_box.add(project_button_box, False, True)
        content.add(project_box, False, True)

        container.add(content, False, True)

        # Right side layout structure
        right_side_box = mforms.newBox(False)
        right_side_image = mforms.newImageBox()
        right_side_image.set_image('migration_background.png')
        right_side_image.set_image_align(mforms.TopRight)
        right_side_box.add(right_side_image, False, True)
        container.add(right_side_box, True, True)

        self.add(container, True, True)


    def start(self):
        try:
            import pyodbc
        except ImportError:
            mforms.Utilities.show_message_and_remember('Dependency Warning',
                   'Could not import the pyodbc python module. You need pyodbc 2.1.8 or newer for migrations from RDBMSes other than MySQL.',
                   'OK', '', '',
                   'wb.migration.nopyodbc',
                   "Don't show this message again")
        else:
            pyodbc_version = server_version_str2tuple(pyodbc.version)
            if pyodbc_version < (2, 1, 8):
                mforms.Utilities.show_message_and_remember('Dependency Warning',
                    '''We have detected that you have pyodbc %s installed but the migration tool requires pyodbc 2.1.8 or newer for migrations from RDBMSes other than MySQL.
Please install a supported pyodbc version. You may proceed with the currently installed version, but the migration may not succeed.
To install the latest version of pyodbc, execute "sudo easy_install pyodbc" from a command line shell.''' % pyodbc.version, 'OK', '', '',
                    'wb.migration.oldpyodbc',
                    "Don't show this message again")

        self.main.start()
            
        
