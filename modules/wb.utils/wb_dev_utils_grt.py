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
