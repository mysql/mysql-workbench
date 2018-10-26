# Copyright (c) 2012, 2018, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms, as
# designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
# This program is distributed in the hope that it will be useful,  but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
# the GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

from wb import DefineModule
import grt
import mforms
import db_copy_main

ModuleInfo = DefineModule(name= "SchemaTransferWizard", author= "Oracle Corp.", version="1.0")

tab_reference = None

#-------------------------------------------------------------------------------
def handle_view_close(view):
    global tab_reference
    tab_reference = None
    view.cleanup()
    return True

#-------------------------------------------------------------------------------
@ModuleInfo.plugin("wb.db.copy.open", type="standalone", caption= "Schema Transfer Wizard",  pluginMenu= "Schema Transfer Wizard", accessibilityName="Schema Transfer Wizard")
@ModuleInfo.export(grt.INT)
def openDBCopy():
    global tab_reference
    app = mforms.App.get()
    if tab_reference:
        app.select_view(tab_reference)
        return

    view = db_copy_main.DBCopy()

    app.dock_view(view, "maintab")
    view.set_title("Schema Transfer Wizard")

    tab_reference = view
    view.on_close(lambda: handle_view_close(view)) # TODO: create weak ref

    app.set_status_text("Schema Transfer Wizard was opened")

    return 1
