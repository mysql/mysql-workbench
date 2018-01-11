# Copyright (c) 2009, 2018, Oracle and/or its affiliates. All rights reserved.
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

#  Copyright (c) 2011 Oracle and/or its associated. All rights reserved.
#

import subprocess
import os

# import the wb module
from wb import *
# import the grt module
import grt

import mforms

# define this Python module as a GRT module
ModuleInfo = DefineModule(name= "WbDevUtil", author= "Oracle.", version="1.0")


@ModuleInfo.export(grt.INT, grt.STRING, grt.STRING)
def generateManifest(moduleName, outpath):
    module = getattr(grt.modules, moduleName, None)
    if not module:
        raise ValueError("Invalid module name")

    dict = grt.Dict()

    dict["name"] = module.__name__
    dict["iconFile"] = os.path.basename(module.__iconpath__)
    pluginList = grt.List()
    dict["plugins"] = pluginList
    dict["author"] = module.__author__
    dict["description"] = module.__description__
    dict["version"] = module.__version__

    if not hasattr(module, "getPluginInfo"):
        raise ValueError("Module '%s' is not a plugin module" % moduleName)
    
    plugins = module.getPluginInfo()
    for plugin in plugins:
        entry = grt.Dict()
        entry["name"] = plugin.name
        entry["caption"] = plugin.caption
        entry["description"] = plugin.description
        pluginList.append(entry)

    grt.serialize(dict, outpath)

    return 0
