--
-- TODO: Put general module description here
--


--
-- standard module/plugin functions
-- 

-- this function is called first by MySQL Workbench core to determine number of plugins in this module and basic plugin info
-- see the comments in the function body and adjust the parameters as appropriate
function getModuleInfo()
	return {
		name= "ModuleName",                     -- put your module name here; must be a valid identifier unique among 
                                                        -- all other plugin names
		author= "CompanyName",                  -- put your company name here
		version= "1.0",                         -- put module version string in form major.minor
		implements= "PluginInterface",          -- don't change this
		functions= {
                  "getPluginInfo:l<o@app.Plugin>:",     -- don't change this
                  "PluginFunctionName:i:o@db.Catalog"   -- list all your plugin function names and accepted argument types, 
                                                        -- keeping the rest unchanged; in this example there's only one 
                                                        -- function, function name is PluginFunctionName and argument type 
                                                        -- is db.Catalog
		}
	}
end


-- helper function to create a descriptor for an argument of a specific type of object
-- you don't need to change here anything
function objectPluginInput(type)
	return grtV.newObj("app.PluginObjectInput", {objectStructName= type})
end

-- this function is called by MySQL Workbench core after a successful call to getModuleInfo() 
-- to gather information about the plugins in this module and the functions that the plugins expose;
-- a plugin should expose only one function that will handle a menu command for a class of objects
-- see the comments in the function body and adjust the parameters as appropriate
function getPluginInfo()
    local l
    local plugin

    -- create the list of plugins that this module exports
    l= grtV.newList("object", "app.Plugin")

    -- create a new app.Plugin object for every plugin
    plugin= grtV.newObj("app.Plugin", {
		name= "wb.catalog.util.pluginFunctionName",      -- plugin namespace
                caption= "Menu item descrption",                 -- plugin textual description (will appear as menu item name)
		moduleName= "ModuleName",                        -- this should be in sync with what you sepcified previously for module 
                                                                 -- name in getModuleInfo()
		pluginType= "normal",                            -- don't change this
		moduleFunctionName= "pluginFunctionName",        -- the function that this plugin exposes
		inputValues= {objectPluginInput("db.Catalog")},  -- the type of object
		rating= 100,                                     -- don't change this
		showProgress= 0,                                 -- don't change this                
		groups= {"Catalog/Utilities", "Menu/Utilities"}  -- use "Catalog/Utilities" to show the menu item on the overview page,
                                                                 -- or "Model/Utilities" to show the menu item on the canvas;
                                                                 -- the "Menu/*" entries control how the plugin will appear in main menu
                                                                 -- the possible values for it are "Menu/Model", "Menu/Catalog", "Menu/Objects",
                                                                 -- "Menu/Database", "Menu/Utilities"
	})

    -- fixup owner
    plugin.inputValues[1].owner= plugin
  
    -- add to the list of plugins
    grtV.insert(l, plugin)
  
    return l
end

--    
-- pluginFunctionName definition
-- the obj argument is guaranteed to be of class or subclass of class
-- specififed in plugin definition
-- the plugins for db.Catalog and its cubclasses are automatically 
-- available and enabled in the main menu in "Plugins" submenu

function pluginFunctionName(obj)

  return 0
end


