# Copyright (c) 2010, 2014, Oracle and/or its affiliates. All rights reserved.
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

from mforms import Utilities

ModuleInfo = DefineModule(name= "WbDocs", author= "Oracle", version="1.0")


@ModuleInfo.plugin("wb.docs.open", type="standalone", caption= "Open Documentation Library",  pluginMenu= "Extensions")
@ModuleInfo.export(grt.INT)
def openDocLib():
    Utilities.open_url("http://dev.mysql.com/doc/refman/5.7/en/index.html")
    return 1

